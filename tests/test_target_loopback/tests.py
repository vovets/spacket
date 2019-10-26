import pytest
from test_utils import reset_delay, conn, reset
import time


def test_ctor_dtor(conn, reset):
    """test SerialDevice creation (driver start) and destruction (driver stop)"""

    conn.send_line(b"test_create")

    conn.expect_line(b"test_create")
    conn.expect_line(b"SUCCESS")
    conn.expect_line(b"SUCCESS")
    conn.expect(b"ch> ")


def test_rx_timeout_no_data(conn, reset):
    """test that timeout works (no data sent)"""
    
    conn.send_line(b"test_rx_timeout")

    conn.expect_line(b"test_rx_timeout")
    conn.expect_line(b"SUCCESS")
    conn.expect_line(b"SUCCESS")
    conn.expect(b"ch> ")


def test_idle_line(conn, reset):
    """test idle line detection

    different sizes with infinite timeout
    """
    rep = 20
    for size in range(1, 249):
        for i in range(0, 2):
            start = time.time()
            conn.send_line(b"test_loopback %d %d" % (size, rep))
            
            conn.expect_line(b"test_loopback %d %d" % (size, rep))
            elapsed = time.time() - start
            conn.expect_line(b"SUCCESS \d+ \d+ ns")
            conn.expect(b"ch> ")
            print("elapsed %.3f\n" % elapsed)

    # test more thoroughly range where bug was found
    rep = 80
    for size in range(69, 120):
        for i in range(0, 10):
            conn.send_line(b"test_loopback %d %d" % (size, rep))
            
            conn.expect_line(b"test_loopback %d %d" % (size, rep))
            conn.expect_line(b"SUCCESS \d+ \d+ ns")
            conn.expect(b"ch> ")


def test_packet_size_limit(conn, reset):
    """test that packet size limit works
    constant originates in fw/constants.h
    """
    
    BUFFER_MAX_SIZE = 256
    BUFFER_SIZE = BUFFER_MAX_SIZE + 1

    conn.send_line(b"test_loopback %d 100" % BUFFER_SIZE)

    conn.expect_line(b"test_loopback %d 100" % BUFFER_SIZE)
    conn.expect_line(b"FAILURE\[500:BufferCreateTooBig\]")
    conn.expect(b"ch> ")


def test_rx_timeout_no_data(conn, reset):
    """test that timeout works (no data sent)"""
    
    conn.send_line(b"test_rx_timeout")

    conn.expect_line(b"test_rx_timeout")
    conn.expect_line(b"SUCCESS")
    conn.expect_line(b"SUCCESS")
    conn.expect(b"ch> ")


def test_rx_timeout(conn, reset):
    """ test that timeout works

    1 ms should be not enough to receive 100 bytes at 921600, but
    due to the test implementation actual write can happen before
    the read starts counting its timeout. That can lead to the test on target
    actually succeeding and hence failing this test. So we use 200 bytes to
    be sure that read timeout occurs.
    """
    
    conn.send_line(b"test_loopback 200 10 1")

    conn.expect_line(b"test_loopback 200 10 1")
    for i in range(0, 10):
        conn.expect_line(b"FAILURE\[404:ReadTimeout\]")
    conn.expect(b"ch> ")


def test_info(conn, reset):
    conn.send_line(b"test_info")

    conn.expect_line(b"test_info")
    conn.expect_line(b"SUCCESS")
    conn.expect(b"ch> ")
