#!/bin/sh -e

git submodule init
git submodule update

(
    cd tmp
    echo "Downloading boost..."
    curl 'https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.bz2' -LO
    echo "Checking boost checksum..."
    sha256sum -c sha256sum
)
echo "Extracting boost..."
tar -C external -xjf tmp/boost_1_66_0.tar.bz2
