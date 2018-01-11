#!/bin/sh -e

echo -n "test\r\n" | nc -w 3 localhost 19021 | grep "Final result: SUCCESS"
