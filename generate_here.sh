#!/bin/sh -ex

spacket_root=$(realpath $(dirname $(realpath $0)))

[ -d "$1" ] || { echo Please specify existing source dir: $(basename $0) src_dir ; exit 1 ; }

mkdir -p build
cd build

readonly cache_file=../initial.cmake
cache_opt=$( [ -f ${cache_file} ] && echo "-C ${cache_file}" || echo "" )

readonly toolchain_file=${spacket_root}/cmake/toolchain_${2}.cmake
toolchain_opt=$( [ "${2}" ] && echo "-DCMAKE_TOOLCHAIN_FILE=${toolchain_file}" || echo "")

cmake -LA -G 'Ninja' -DSPACKET_ROOT=${spacket_root} ${cache_opt} ${toolchain_opt} "../$1"
