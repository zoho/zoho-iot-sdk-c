#!/bin/bash
pwd
rm -rf build

set -e

mkdir build && cd build
cmake -DENABLE_TLS=ON -DUSE_CLIENT_CERTS=ON ..
make -j8

echo "\n\n\nRunning Tests:\n"
export CMOCKA_MESSAGE_OUTPUT=XML
export CMOCKA_XML_FILE=unit-test-report.xml
ctest -V

echo "\nSuccess! Generating Reports:\n"
lcov -c -d CMakeFiles/zoho_iot_sdk.dir/src/ -o coverage.info
genhtml coverage.info -o report

#Cobertura Coverage Report
gcovr -x -r ../ -e ../lib -e ../samples -e ../test -o coverage.xml

cd -