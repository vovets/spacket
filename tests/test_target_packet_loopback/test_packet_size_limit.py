import test_utils


def test(c):
    c.expect_line(b"RTT ready")
    c.expect(b"ch> ")

    # test that packet size limit works
    c.send_line(b"test_loopback 249 100")

    c.expect_line(b"test_loopback 249 100")
    c.expect_line(b"FAILURE\[700:PoolAllocatorObjectTooBig\]")
    c.expect(b"ch> ")

timeout = 1

if __name__ == "__main__":
    test_utils.main(test, timeout)
