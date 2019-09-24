"""This module contains tests that verify CRC calculation by hw unit in STM32F103

Also algorithm for CRC calclulation on non-multiple-of-4 length buffers verified here.
CRC algorithm employed by STM32F103 (let's call it "ST") is variation (see below) of "CRC-32/MPEG-2" according to classification given
in "http://reveng.sourceforge.net/crc-catalogue/17plus.htm#crc.cat-bits.32". Its parameters are:
width=32
poly=0x04c11db7
init=0xffffffff
refin=false
refout=false
xorout=0x00000000
check=0x0376e6e7
residue=0x00000000
name="CRC-32/MPEG-2"


The test plan is:
 0. Test firmware protocol (open, close, add, possible errors)
 1. Test that "st_crc" gives same result as hw unit add_uint32
 2. Test that "mpeg_crc_bytes" is the same as "CRC-32/MPEG-2"
 3. Test that "st_crc_bytes" is the same as "mpeg_crc_bytes"
"""


import pytest
from test_utils import reset_delay, conn, reset
from binascii import hexlify, unhexlify


mpeg_poly = 0x04c11db7
mpeg_init = 0xffffffff
mpeg_check = 0x0376e6e7
mpeg_check_bytes = b"123456789"

checks = [
    b"1",
    b"12",
    b"123",
    b"1234",
    b"12345",
    b"123456",
    b"1234567",
    b"12345678",
    b"123456789",
    b"\x00"
]

def to_u32_be(data):
    """Converts sequence of bytes to big endian int32"""
    
    offset = 0
    l = len(data)
    assert(l <= 4)
    result = 0
    shift = 24
    while l:
        result |= (data[offset] << shift)
        l -= 1
        offset += 1
        shift -= 8
        
    return result


def pmod(m, lshift):
    m = m & 0xffffffff
    
    def high_bit():
        return m & 0x80000000
    
    for i in range(lshift):
        h = high_bit();
        m = (m << 1) & 0xffffffff
        if h:
            m = m ^ mpeg_poly

    return m


def st_crc(prev_crc, data):
    """Calculates CRC exactly as hardware unit in STM32F103 (fixed poly) does.

    prev_crc -- initial contents of CRC register as int
    data     -- 32-bit block encoded as int.
    """

    data = data & 0xffffffff
    crc = prev_crc & 0xffffffff

    crc = crc ^ data

    return pmod(crc, 32)


def st_crc_bytes(prev_crc, data):
    l = len(data)
    crc = prev_crc & 0xffffffff
    i = 0
    while (l > 3):
        crc = st_crc(crc, to_u32_be(data[i:i+4]))
        i = i + 4
        l = l - 4
        
    bits = 8 * l
    d = to_u32_be(data[i:]) >> (32 - bits)
    
    u2 = ((crc >> (32 - bits)) ^ d) & 0xffffffff
    # print(f'u2={u2:08x}')
    
    c = st_crc(0, u2) # == [u2:00000000] mod g
    # print(f'c={c:08x}')
    
    return c ^ ((crc << bits) & 0xffffffff)


def mpeg_crc_bytes(prev_crc, data):
    """
    Calculates "CRC-32/MPEG-2" crc of byte sequence.

    Operates on sequence of bytes instead of 32-bit block.

    prev_crc -- initial contents of CRC register as int
    data     -- sequence of bytes.
    """
    
    crc = prev_crc & 0xffffffff

    for b in data:
        b = (b & 0xff) << 24
        # print(f'b={b:08x}')
        crc = crc ^ b
        # print(f'crc={crc:08x}')
        crc = pmod(crc, 8)

    return crc


def test_to_u32_be():
    assert to_u32_be(bytes.fromhex("01")) == 0x01000000
    assert to_u32_be(bytes.fromhex("0102")) == 0x01020000
    assert to_u32_be(bytes.fromhex("010203")) == 0x01020300
    assert to_u32_be(bytes.fromhex("01020304")) == 0x01020304


def check_open(conn):
    conn.send_line(b"open")

    conn.expect_line(b"open")
    conn.expect(b"ch> ")


def check_close(conn):
    conn.send_line(b"close")

    conn.expect_line(b"close")
    conn.expect(b"ch> ")


@pytest.fixture
def dev_open(conn, reset):
    check_open(conn)

    
def test_open(dev_open):
    pass


def test_double_open(conn, dev_open):
    conn.send_line(b"open")

    conn.expect_line(b"open")
    conn.expect_line(b"device is already opened")
    conn.expect(b"ch> ")


def test_open_close(conn, dev_open):
    check_close(conn)


def test_not_opened(conn, reset):
    conn.send_line(b"add_uint32 0")
    conn.expect_line(b"device is not opened")
    conn.expect(b"ch> ")


def test_add_sequence_too_long(conn, dev_open):
    conn.send_line(b"crc " b"12345678" b"12345678" b"12345678" b"123456781")
    conn.expect_line(b"bad arg format, sequence too long \(max 32 digits\)")
    conn.expect(b"ch> ")


def test_add_odd_length(conn, dev_open):
    conn.send_line(b"crc 1")
    conn.expect_line(b"bad arg format, odd length sequence of hex digits")
    conn.expect(b"ch> ")


def test_add_non_hex(conn, dev_open):
    conn.send_line(b"crc ga")
    conn.expect_line(b"bad arg format, should be sequence of hex digits")
    conn.expect(b"ch> ")


def test_st_crc_same_as_hw_add_uint32(conn, dev_open):
    check_value = 0x34333231
    
    conn.send_line(bytes(f"add_uint32 {check_value:x}", "ascii"))

    m = conn.expect_line(b"new: ([0-9a-fA-F]{1,8})")

    assert int(m.group(1), 16) == st_crc(mpeg_init, check_value)


def test_mpeg_crc_bytes():
    assert mpeg_crc_bytes(mpeg_init, mpeg_check_bytes) == mpeg_check


def test_st_crc_bytes_same_as_mpeg_crc_bytes():
    for c in checks:
        assert st_crc_bytes(mpeg_init, c) == mpeg_crc_bytes(mpeg_init, c)


def test_hw_crc_same_as_st_crc_bytes(conn, dev_open):
    for c in checks:
        conn.send_line(b"crc " + hexlify(c))

        m = conn.expect_line(b"crc: ([0-9a-fA-F]{1,8})")
        assert int(m.group(1), 16) == st_crc_bytes(mpeg_init, c)
