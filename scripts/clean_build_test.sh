#!/bin/bash
pwd
rm -rf build

set -e

mkdir build && cd build

if [ $# -eq 0 ]
  then
    cmake .. -DZ_ENABLE_TLS=true -DZ_USE_CLIENT_CERTS=true
  else
    echo "using build parameters:  ${1}"
    cmake $1 ..
fi

#cmake -DZ_ENABLE_TLS=ON -DZ_USE_CLIENT_CERTS=ON ..
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