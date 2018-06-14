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

BUT, if we want to employ DMA to feed hw unit from some buffer we need to take into account that 32-bit reads are little-endian. So crc calculated using hw+dma will differ from "CRC-32/MPEG-2". Basicly it's the same algorithm, but order in which bytes are read from buffer is different.

Suppose we have this byte sequence for which we wish to calculate CRC:
1 2 3 4 5 6 7

Then "CRC-32/MPEG-2" pushes bytes through crc register as follows:
1 2 3 4 5 6 7

Because of endianness of DMA access "ST" will push bytes through it's register in this order:
4 3 2 1  7 6 5
It must be noted that accesses are 32-bit so "leftover" 3 to 1 bytes need special treatment.

The test plan is:
 0. Test firmware protocol (open, close, add, possible errors)
 1. Test that "st_crc" gives same result as hw unit add_uint32
 2. Test that "mpeg_crc_bytes" is the same as "CRC-32/MPEG-2"
 3. Test that "crc_bytes_hw" with BE reads is the same as "mpeg_crc_bytes"
 4. Test that "crc_bytes_hw" with LE reads is the same as "mpeg_crc_bytes" with altered bytes order
 5. Test that "crc_bytes_hw" with LE reads is the same as hw unit add
"""


import pytest
from test_utils import reset_delay
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
]

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
            crc = crc ^ mpeg_poly

    return crc


def mpeg_crc_bytes(prev_crc, data):
    """
    Calculates "CRC-32/MPEG-2" crc of byte sequence.

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
                crc = crc ^ mpeg_poly

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


def test_to_u32_be():
    assert to_u32_be(bytes.fromhex("01")) == 0x01
    assert to_u32_be(bytes.fromhex("0102")) == 0x0102
    assert to_u32_be(bytes.fromhex("010203")) == 0x010203
    assert to_u32_be(bytes.fromhex("01020304")) == 0x01020304


def to_u32_le(data):
    """Converts sequence of bytes to little endian int"""
    
    offset = 0
    l = len(data)
    result = 0
    
    while offset < l:
        result |= (data[offset] << (offset * 8))
        offset += 1
        
    return result


def test_to_u32_le():
    assert to_u32_le(bytes.fromhex("01")) == 0x01
    assert to_u32_le(bytes.fromhex("0102")) == 0x0201
    assert to_u32_le(bytes.fromhex("010203")) == 0x030201
    assert to_u32_le(bytes.fromhex("01020304")) == 0x04030201


def crc_bytes_hw(prev_crc, data, read_uint32):
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
        crc = st_crc(crc, read_uint32(data[base:base+4]))
        base += 4
        left -= 4
        
    if left > 0:
        val = read_uint32(data[base:base+left])
        bits = left * 8
        val ^= crc ^ (crc >> (32 - bits))
        finXor = (crc << bits) & 0xffffffff
        crc = st_crc(crc, val)
        crc ^= finXor

    return crc


def crc_bytes_hw_be(prev_crc, data):
    return crc_bytes_hw(prev_crc, data, to_u32_be)


def crc_bytes_hw_le(prev_crc, data):
    return crc_bytes_hw(prev_crc, data, to_u32_le)


@pytest.fixture(scope="module")
def conn(request):
    import test_utils
    c = test_utils.Connection(port=19021, connect_timeout=1, response_timeout=1)
    request.addfinalizer(c.close)
    return c


@pytest.fixture
def dev_reset(conn):
    conn.send_line(b"reset")
    reset_delay()
    conn.expect_line(b"RTT ready")
    conn.expect(b"ch> ")
    print("reset OK")


def check_open(conn):
    conn.send_line(b"open")

    conn.expect_line(b"open")
    conn.expect(b"ch> ")


def check_close(conn):
    conn.send_line(b"close")

    conn.expect_line(b"close")
    conn.expect(b"ch> ")


@pytest.fixture
def dev_open(conn, dev_reset):
    check_open(conn)

    
def test_reset(conn):
    conn.send_line(b"reset")
    conn.expect_line(b"RTT ready")
    conn.expect(b"ch> ")


def test_open(dev_open):
    pass


def test_double_open(conn, dev_open):
    conn.send_line(b"open")

    conn.expect_line(b"open")
    conn.expect_line(b"device is already opened")
    conn.expect(b"ch> ")


def test_open_close(conn, dev_open):
    check_close(conn)


def test_not_opened(conn, dev_reset):
    conn.send_line(b"add_uint32 0")
    conn.expect_line(b"device is not opened")
    conn.expect(b"ch> ")


def test_add_sequence_too_long(conn, dev_open):
    conn.send_line(b"add " b"12345678" b"12345678" b"12345678" b"123456781")
    conn.expect_line(b"bad arg format, sequence too long \(max 32 digits\)")
    conn.expect(b"ch> ")


def test_add_odd_length(conn, dev_open):
    conn.send_line(b"add 1")
    conn.expect_line(b"bad arg format, odd length sequence of hex digits")
    conn.expect(b"ch> ")


def test_add_non_hex(conn, dev_open):
    conn.send_line(b"add ga")
    conn.expect_line(b"bad arg format, should be sequence of hex digits")
    conn.expect(b"ch> ")


def test_st_crc_same_as_hw_add_uint32(conn, dev_open):
    check_value = 0x34333231
    
    conn.send_line(bytes("add_uint32 {:x}".format(check_value), "ascii"))

    m = conn.expect_line(b"new: ([0-9a-fA-F]{1,8})")

    assert int(m.group(1), 16) == st_crc(mpeg_init, check_value)


def test_mpeg_crc_bytes():
    assert mpeg_crc_bytes(mpeg_init, mpeg_check_bytes) == mpeg_check


def test_crc_bytes_hw_be_same_as_mpeg():
    checks = [b"123456", b"1234567", b"12345678"]
    assert crc_bytes_hw_be(mpeg_init, mpeg_check_bytes) == mpeg_check
    for c in checks:
        assert crc_bytes_hw_be(mpeg_init, c) == mpeg_crc_bytes(mpeg_init, c)


def iter_le(data):
    l = len(data)
    offset = 0
    while l >= 4:
        for i in [3, 2, 1, 0]:
            yield data[offset+i]
        l -= 4
        offset += 4
    for i in range(l - 1, -1, -1):
        yield data[offset+i]


def test_iter_le():
    assert bytes(iter_le(b"12345678")) == b"43218765"
    assert bytes(iter_le(b"1234567"))  == b"4321765"
    assert bytes(iter_le(b"123456"))   == b"432165"
    assert bytes(iter_le(b"12345"))    == b"43215"

    assert bytes(iter_le(b"123"))      == b"321"
    assert bytes(iter_le(b"12"))       == b"21"
    assert bytes(iter_le(b"1"))        == b"1"


def test_crc_bytes_hw_le_same_as_mpeg_alt():
    for c in checks:
        assert crc_bytes_hw_le(mpeg_init, c) == mpeg_crc_bytes(mpeg_init, iter_le(c))


def test_crc_bytes_hw_le_same_as_hw_add(conn, dev_reset):
    for c in checks:
        check_open(conn)
        
        conn.send_line(bytes("add {}".format(str(hexlify(c), "ascii")), "ascii"))

        m = conn.expect_line(b"new: ([0-9a-fA-F]{1,8})")

        print("mpeg={:x}, hw_le={:x}".format(mpeg_crc_bytes(mpeg_init, iter_le(c)),
                                             crc_bytes_hw_le(mpeg_init, c)))
        assert int(m.group(1), 16) == crc_bytes_hw_le(mpeg_init, c)

        check_close(conn)
