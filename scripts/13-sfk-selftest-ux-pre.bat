cd ..
rm -rf tmp-selftest
mkdir tmp-selftest
cd tmp-selftest
cp -R ../testfiles testfiles

export TCMD="cmp ../scripts/10-sfk-selftest-db.txt"

. ../scripts/12-sub-test-ux.bat

cd ../scripts
