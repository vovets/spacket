import test_utils


def test(c):
    c.expect_line(b"RTT ready")
    c.expect(b"ch> ")

    # test idle line detection
    # different sizes with infinite timeout
    rep = 20
    for size in range(1, 249):
        for i in range(0, 1):
            c.send_line(b"test_loopback %d %d 0" % (size, rep))
            
            c.expect_line(b"test_loopback %d %d 0" % (size, rep))
            c.expect_line(b"SUCCESS \d+ \d+")
            c.expect(b"ch> ")

timeout = 1

if __name__ == "__main__":
    test_utils.main(test, timeout)
