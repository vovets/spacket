import test_utils


# test that packet size limit works
# constant originates in fw/constants.h
BUFFER_MAX_SIZE = 256
BUFFER_SIZE = BUFFER_MAX_SIZE + 1

def test(c):
    c.expect_line(b"RTT ready")
    c.expect(b"ch> ")

    c.send_line(b"test_loopback %d 100" % BUFFER_SIZE)

    c.expect_line(b"test_loopback %d 100" % BUFFER_SIZE)
    c.expect_line(b"FAILURE\[501:PacketCreateTooBig\]")
    c.expect(b"ch> ")

timeout = 1

if __name__ == "__main__":
    test_utils.main(test, timeout)
