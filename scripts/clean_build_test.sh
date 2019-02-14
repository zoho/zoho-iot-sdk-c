#!/bin/bash
pwd
set -e

rm -rf build
mkdir build && cd build
cmake ..
make -j8

echo "\n\n\nRunning Tests:\n"
export CMOCKA_MESSAGE_OUTPUT=XML
export CMOCKA_XML_FILE=unit-test-report.xml
ctest -V

echo "\nSuccess! Generating Report:\n"
lcov -c -d CMakeFiles/zoho_iot_sdk.dir/src/ -o coverage.info
genhtml coverage.info -o report