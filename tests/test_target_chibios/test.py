import pytest
from test_utils import conn


def test(conn):
    conn.set_response_timeout(30)
    conn.send_line(b"test")
    
    conn.expect_line(b"Final result: SUCCESS")
    conn.expect(b"ch> ")
