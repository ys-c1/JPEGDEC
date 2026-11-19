#!/bin/sh
# build.sh <out-binary> <src-dir> [extra CXXFLAGS]
set -e
out="$1"; srcdir="$2"; shift 2
g++ -O2 -D__LINUX__ -I"$srcdir" "$@" bench/bench.cpp "$srcdir/JPEGDEC.cpp" -o "$out"
