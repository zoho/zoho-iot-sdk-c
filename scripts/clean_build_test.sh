#!/bin/bash
set -e

pwd

rm -rf build
mkdir build && cd build
cmake ..
make -j8
echo "\n\n\nRunning Tests:\n"
ctest -V
echo "Done!"
