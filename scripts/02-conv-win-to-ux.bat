@echo off
sfk filter 11-sub-test-win.bat >12-sub-test-ux.bat -rep _\_/_ -rep /%%TCMD%%/$TCMD/ -rep /!/:/ -rep "x>nulx>/dev/nullx" -rep "_../sfk_../sfk -nocol_"
copy 13-sfk-selftest-ux-pre.bat 03-sfk-selftest-ux.bat >nul
sfk remcrlf 03-sfk-selftest-ux.bat >nul
sfk patch -nopid -qs 14-sfk-selftest-ux-fix.txt
sfk remcrlf 12-sub-test-ux.bat >nul
echo created: 03-sfk-selftest-ux.bat
echo created: 12-sub-test-ux.bat
echo "now transfer and run 03-sfk-selftest-ux.bat"

