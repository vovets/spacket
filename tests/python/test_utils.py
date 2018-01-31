import telnetlib
import argparse


lineend = b"\r\n"

class NoMatch(Exception):
    pass

class Connection:
    def __init__(self, port, host='localhost',
                 connect_timeout=0.5, response_timeout=0.5):
        self.connect_timeout = connect_timeout
        self.response_timeout = response_timeout
        self.telnet = telnetlib.Telnet(host, port, connect_timeout)

    def send_line(self, line):
        self.telnet.write(line + lineend)

    def expect(self, line):
        index, _, bytes_read = self.telnet.expect(
            [line], self.response_timeout)
        print("read: ", bytes_read)
        if index == -1:
            raise NoMatch

    def expect_line(self, line):
        self.expect(line + lineend)

def main(test, timeout=0.5):
    parser = argparse.ArgumentParser(description='Run test using telnet protocol.')
    parser.add_argument('-p', metavar='P', type=int, nargs=1, default=19021,
                        help='port to connect to')

    args = parser.parse_args()
    
    try:
        test(Connection(port=args.p, response_timeout=timeout))
    except NoMatch:
        print("FAILED")
        exit(1)
