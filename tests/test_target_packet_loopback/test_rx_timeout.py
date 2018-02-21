import test_utils


def test(c):
    c.expect_line(b"RTT ready")
    c.expect(b"ch> ")

    # test that timeout works
    # 1 ms should be not enough to receive 100 bytes at 921600
    c.send_line(b"test_loopback 100 10 1")

    c.expect_line(b"test_loopback 100 10 1")
    for i in range(0, 10):
        c.expect_line(b"created : \[100\]\d+")
        c.expect_line(b"received: \[\d\d\]\d+")
        c.expect_line(b"FAILURE\[Packets differ\].*")

timeout = 1

if __name__ == "__main__":
    test_utils.main(test, timeout)
