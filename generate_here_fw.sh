#!/bin/sh -ex

spacket_root=$(realpath $(dirname $(realpath $0)))

exec ${spacket_root}/generate_here.sh $1 gcc_arm-none-eabi
