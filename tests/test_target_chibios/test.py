import test_utils


def test(c):
    c.expect(b"ch> ")

    c.send_line(b"test")
    
    c.expect_line(b"Final result: SUCCESS")
    c.expect(b"ch> ")

timeout = 30

if __name__ == "__main__":
    test_utils.main(test, timeout=timeout)
