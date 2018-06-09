import telnetlib
import argparse
from time import sleep


def reset_delay():
    sleep(0.05)


lineend = b"\r\n"


class NoMatch(Exception):
    pass


class Connection:
    def __init__(self, port, host='localhost',
                 connect_timeout=0.5, response_timeout=0.5):
        self.connect_timeout = connect_timeout
        self.response_timeout = response_timeout
        self.telnet = telnetlib.Telnet(host, port, connect_timeout)

    def close(self):
        self.telnet.close()

    def send_line(self, line):
        self.telnet.write(line + lineend)

    def expect(self, line):
        index, match, bytes_read = self.telnet.expect(
            [line], self.response_timeout)
        print("read: ", bytes_read)
        if index == -1:
            raise NoMatch
        return match

    def expect_line(self, line):
        return self.expect(line + lineend)
