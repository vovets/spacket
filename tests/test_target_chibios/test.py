import test_utils


def test(c):
    c.expect(b"ch> ")

    c.send(b"test\r")
    
    c.expect(b"Final result: SUCCESS\r\n")
    c.expect(b"ch> ")

timeout = 30

if __name__ == "__main__":
    test_utils.main(test, timeout=timeout)
