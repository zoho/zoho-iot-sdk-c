set -e
cd build
make -j8
ctest -V
#lcov -c -d CMakeFiles/zoho_iot_sdk.dir/src/ -o coverage.info && genhtml coverage.info -o report
cd -
echo "Done!"