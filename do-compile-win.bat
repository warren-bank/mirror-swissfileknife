@rem windows compile of swiss file knife using msvc 7.
@echo off

IF "%1"=="trace" GOTO xtrace

cl %2 -DSFK_WINPOPUP_SUPPORT sfk.cpp sfknet.cpp patch.cpp inst.cpp kernel32.lib user32.lib gdi32.lib ws2_32.lib advapi32.lib
set MTK_TRACE=
GOTO xdone

:xtrace
echo "compiling trace version"
cl %2 -DWITH_TRACING -DVERBOSE_MEM -DSFK_WINPOPUP_SUPPORT sfk.cpp sfknet.cpp patch.cpp inst.cpp kernel32.lib user32.lib gdi32.lib ws2_32.lib advapi32.lib
set MTK_TRACE=file:twexb,filename:log.txt
GOTO xdone

:xdone
