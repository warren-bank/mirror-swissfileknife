@echo off
..\sfk test workdir scripts >nul 2>&1
IF "%1"=="" GOTO xerr01
IF ERRORLEVEL 1 GOTO xerr02

cd ..
IF EXIST tmp-selftest rmdir /S /Q tmp-selftest
md tmp-selftest
cd tmp-selftest
xcopy ..\testfiles testfiles /S /H /I /R /K /Y >nul

set TCMD=%1 ..\scripts\10-sfk-selftest-db.txt

call ..\scripts\11-sub-test-win.bat

cd ..\scripts
GOTO done

:xerr01
echo supply cmp to run a regression test   (compare).
echo supply upd to add new test case datas (update).
echo supply rec to overwrite all test output signatures.
GOTO done

:xerr02
echo run this batch from the scripts directory please.
GOTO done

:done
