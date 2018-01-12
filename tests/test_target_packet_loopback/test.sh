#!/bin/sh -e

nc -w 3 localhost 19021 | grep "ch>"
