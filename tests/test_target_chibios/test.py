import pytest
from test_utils import reset_delay


@pytest.fixture(scope="module")
def conn(request):
    import test_utils
    c = test_utils.Connection(port=19021, connect_timeout=1, response_timeout=30)
    request.addfinalizer(c.close)
    return c


def test(conn):
    conn.expect(b"ch> ")

    conn.send_line(b"test")
    
    conn.expect_line(b"Final result: SUCCESS")
    conn.expect(b"ch> ")
