import test_utils


def test(c):
    c.expect_line(b"RTT ready")
    c.expect(b"ch> ")

    c.send_line(b"test_create")

    c.expect_line(b"test_create")
    c.expect_line(b"SUCCESS")
    c.expect_line(b"SUCCESS")
    c.expect(b"ch> ")

    c.send_line(b"test_rx_timeout")

    c.expect_line(b"test_rx_timeout")
    c.expect_line(b"SUCCESS")
    c.expect_line(b"SUCCESS")
    c.expect(b"ch> ")

timeout = 1

if __name__ == "__main__":
    test_utils.main(test, timeout)
