#!/bin/bash
set -e

cd "$(dirname "$0")"

make clean
make memb0rk

./stupid_assembler.py examples/hello-world.mb{a,x}

./memb0rk examples/hello-world.mbx
