import test_utils


def test(c):
    c.expect(b"RTT ready\n")
    c.expect(b"ch> ")

    c.send(b"test_create\r")

    c.expect(b"test_create\n")
    c.expect(b"SUCCESS\n\n")
    c.expect(b"ch> ")

if __name__ == "__main__":
    test_utils.main(test)
