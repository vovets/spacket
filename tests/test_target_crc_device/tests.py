"""This module contains tests that verify CRC calculation by hw unit in STM32F103

Also algorithm for CRC calclulation on non-multiple-of-4 length buffers verified here.
CRC algorithm employed by STM32F103 is named "CRC-32/MPEG-2" according to classification given
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
"""


import pytest
from test_utils import reset_delay
from binascii import hexlify, unhexlify


poly = 0x04c11db7
init = 0xffffffff
check = 0x0376e6e7
check_bytes = b"123456789"


def st_crc(prev_crc, data):
    """Calculates CRC exactly as hardware unit in STM32F103 (fixed poly) does.

    prev_crc -- initial contents of CRC register as int
    data     -- 32-bit block encoded as int.
    """

    data = data & 0xffffffff
    crc = prev_crc & 0xffffffff
    crc = crc ^ data

    def high_bit():
        return crc & 0x80000000
    
    for i in range(32):
        h = high_bit();
        crc = (crc << 1) & 0xffffffff
        if h:
            crc = crc ^ poly

    return crc


def st_crc_bytes(prev_crc, data):
    """
    Calculates CRC exactly as hardware unit in STM32F103 (fixed poly) does.

    Operates on sequence of bytes instead of 32-bit block.

    prev_crc -- initial contents of CRC register as int
    data     -- sequence of bytes.
    """
    
    crc = prev_crc & 0xffffffff

    def high_bit():
        return crc & 0x80000000

    for b in data:
        b = b & 0xff
        crc = crc ^ (b << 24)
        for i in range(8):
            h = high_bit()
            crc = (crc << 1) & 0xffffffff
            if h:
                crc = crc ^ poly

    return crc


def to_u32_be(data):
    """Converts sequence of bytes to big endian int"""
    
    offset = 0
    l = len(data) - 1
    result = 0
    
    while l:
        result |= data[offset]
        result <<= 8
        l -= 1
        offset += 1
        
    result |= data[offset]
    return result

def st_crc_bytes2(prev_crc, data):
    """This function models CRC calculation using hardware unit in STM32F103

    Operates on sequence of bytes. The point is to account for non-multiple-of-4 length
    sequences. Value to be fed to hardware unit is prepared in such a way that hw register
    will contain leading zeroes before division. Also data bytes are xored with part of previous crc
    as in st_crc_bytes algorithm. The leading zeroes will not change register value.
    Then actual data bits will be fed through register, as in st_crc_bytes algorithm.
    Output value also adjusted according to differences with st_crc_bytes algorithm in contents of
    the register at the moment actual data bits are fed.

    prev_crc -- initial contents of CRC register as int
    data     -- sequence of bytes.
    """
    crc = prev_crc & 0xffffffff

    left = len(data)
    base = 0
    
    while left >= 4:
        crc = st_crc(crc, to_u32_be(data[base:base+4]))
        base += 4
        left -= 4
        
    if left > 0:
        val = to_u32_be(data[base:base+left])
        bits = left * 8
        val ^= crc ^ (crc >> (32 - bits))
        finXor = (crc << bits) & 0xffffffff
        crc = st_crc(crc, val)
        crc ^= finXor

    return crc


@pytest.fixture(scope="module")
def conn(request):
    import test_utils
    c = test_utils.Connection(port=19021, connect_timeout=1, response_timeout=1)
    request.addfinalizer(c.close)
    return c


@pytest.fixture
def reset(conn):
    conn.send_line(b"reset")
    reset_delay()
    conn.expect_line(b"RTT ready")
    conn.expect(b"ch> ")
    print("reset OK")

def test_reset(conn):
    conn.send_line(b"reset")
    conn.expect_line(b"RTT ready")
    conn.expect(b"ch> ")


def test_open(conn, reset):
    conn.send_line(b"open")

    conn.expect_line(b"open")
    conn.expect(b"ch> ")


def test_double_open(conn, reset):
    conn.send_line(b"open")

    conn.expect_line(b"open")
    conn.expect(b"ch> ")

    conn.send_line(b"open")

    conn.expect_line(b"open")
    conn.expect_line(b"device is already opened")
    conn.expect(b"ch> ")


def test_open_close(conn, reset):
    conn.send_line(b"open")

    conn.expect_line(b"open")
    conn.expect(b"ch> ")

    conn.send_line(b"close")

    conn.expect_line(b"close")
    conn.expect(b"ch> ")


def test_not_opened(conn, reset):
    conn.send_line(b"add 0")
    conn.expect_line(b"device is not opened")
    conn.expect(b"ch> ")


def test_to_u32_be():
    assert to_u32_be(bytes.fromhex("01")) == 0x01
    assert to_u32_be(bytes.fromhex("0102")) == 0x0102
    assert to_u32_be(bytes.fromhex("010203")) == 0x010203
    assert to_u32_be(bytes.fromhex("01020304")) == 0x01020304

    
def test_crc_int32_and_bytes_are_same():
    bs = b"1234"
    assert st_crc(init, to_u32_be(bs)) == st_crc_bytes(init, bs)


def test_crc_bytes():
    assert st_crc_bytes(init, check_bytes) == check


def test_crc_bytes2():
    assert st_crc_bytes2(init, check_bytes) == check


def test_dev_int(conn, reset):
    bs = b"1234"

    conn.send_line(b"open")

    conn.expect_line(b"open")
    conn.expect(b"ch> ")

    conn.send_line(bytes("add {}".format(str(hexlify(bs), "ascii")), "ascii"))

    m = conn.expect_line(b"new: ([0-9a-fA-F]{1,8})")

    assert to_u32_be(unhexlify(m.group(1))) == st_crc_bytes(init, bs)
