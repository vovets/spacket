import test_utils


def test(c):
    c.expect_line(b"RTT ready")
    c.expect(b"ch> ")

    # test SerialDevice creation (driver start) and destruction (driver stop)
    c.send_line(b"test_create")

    c.expect_line(b"test_create")
    c.expect_line(b"SUCCESS")
    c.expect_line(b"SUCCESS")
    c.expect(b"ch> ")

    # test that timeout works (no data sent)
    c.send_line(b"test_rx_timeout")

    c.expect_line(b"test_rx_timeout")
    c.expect_line(b"SUCCESS")
    c.expect_line(b"SUCCESS")
    c.expect(b"ch> ")

    # test that packet size limit works
    c.send_line(b"test_loopback 249 100")

    c.expect_line(b"test_loopback 249 100")
    c.expect_line(b"FAILURE\[700:PoolAllocatorObjectTooBig\]")
    c.expect(b"ch> ")

    # test that timeout works
    # 1 ms should be not enough to receive 100 bytes at 921600
    c.send_line(b"test_loopback 100 10 1")

    c.expect_line(b"test_loopback 100 10 1")
    for i in range(0, 10):
        c.expect_line(b"created : \[100\]\d+")
        c.expect_line(b"received: \[\d\d\]\d+")
        c.expect_line(b"FAILURE\[Packets differ\].*")

    # test idle line detection
    # different sizes with infinite timeout
    rep = 80
    for size in range(98, 100):
        for i in range(0, 40):
            c.send_line(b"test_loopback %d %d 0" % (size, rep))
            
            c.expect_line(b"test_loopback %d %d 0" % (size, rep))
            c.expect_line(b"SUCCESS")
            c.expect(b"ch> ")

timeout = 1

def timeout_handler():
    exit(2)

if __name__ == "__main__":
    test_utils.main(test, timeout, timeout_handler)
