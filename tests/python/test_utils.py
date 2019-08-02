import telnetlib
import argparse
import pytest
from time import sleep


def reset_delay():
    sleep(1)


lineend = b"\r\n"


class NoMatch(Exception):
    pass


class Connection:
    def __init__(self, port, host='localhost',
                 connect_timeout=0.5, response_timeout=0.5, debuglevel=0):
        self.connect_timeout = connect_timeout
        self.response_timeout = response_timeout
        self.telnet = telnetlib.Telnet(host, port, connect_timeout)
        self.telnet.set_debuglevel(debuglevel)

    def close(self):
        self.telnet.close()

    def set_response_timeout(self, timeout):
        self.response_timeout = timeout

    def send_line(self, line):
        self.telnet.write(line + lineend)

    def expect(self, line):
        index, match, bytes_read = self.telnet.expect(
            [line], self.response_timeout)
        if index == -1:
            raise NoMatch
        return match

    def expect_line(self, line):
        return self.expect(line + lineend)


@pytest.fixture(scope="module")
def conn(request):
    c = Connection(port=19021, connect_timeout=1, response_timeout=2, debuglevel=1)
    request.addfinalizer(c.close)
    c.expect_line(b"RTT ready")
    c.expect(b"ch> ")
    return c


@pytest.fixture
def reset(conn):
    conn.send_line(b"reset")
    reset_delay()
    conn.expect_line(b"RTT ready")
    conn.expect(b"ch> ")
    print("reset OK\n")
