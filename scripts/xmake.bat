@echo off
rem XMake 0.3 - cross network make of projects.
rem Sync sources, compile remote, receive error output,
rem forward compiled binaries to target device.
rem requires:
rem   sfk.exe on your Windows PC, download from
rem     stahlworks.com/sfk.exe
rem   xmakeserv.sh running on build server (which runs sfk)
rem example call:
rem   xmake.bat mp9 i686 200 clean
rem   meaning: compile product mp9 for architecture i686
rem            and target ip .200 from scratch (clean)

rem === short parameters ===
set PRODUCT=%1
set TOOLCHAIN=%2
set TARGET=%3
set CLEAN=%4

rem === dataway defaults ===
rem = all source code is edited in: LOCAL_WORK
rem = xmakeserv.sh runs on machine: BUILD_SERVER
rem = file transfer uses password : TRANSFER_PW
rem = your Windows PC running DView is  : LOG_TARGET
rem = the target devices are in a subnet: TARGET_NET

set LOCAL_WORKDRIVE=d:
set LOCAL_WORKDIR=d:\work
set BUILD_SERVER=192.168.1.101:2201
set TRANSFER_PW=mycmdpw123
set LOG_TARGET=192.168.1.100
set TARGET_NET=192.168.1

rem === internal inits ===
rem = this batch file logs to DView by network text
rem = (enable Setup / General / Network text).
rem = file sync should be non verbose.
rem = redirect output of some commands using TONETLOG.
rem = sfk uses ftp password from SFK_FTP_PW.

set SFK_LOGTO=term,net:%LOG_TARGET%
set FTPOPT=-nohead -quiet -noprog
set TONETLOG=sfk filt "-! files of*sent" -lshigh cyan "<*" +tonetlog
set SFK_FTP_PW=%TRANSFER_PW%

rem === auto extend short parms to full parms ===
rem = extend product "mc9" to "m9player".
rem = extend target "200" to "192.168.1.200".

IF "%PRODUCT%"=="mp8" ( set PRODUCT=m8player )
IF "%PRODUCT%"=="mp9" ( set PRODUCT=m9player )

sfk -quiet strlen "%TARGET%"
IF %ERRORLEVEL% LEQ 3 ( set TARGET=%TARGET_NET%.%TARGET% )

rem === show a hello in DView by network text (SFK_LOGTO) ===

sfk echo "[Blue]=== xmake : %PRODUCT% %TOOLCHAIN% for %TARGET% ===[def]" +tolog

rem === go into local workspace containing project folders ===

%LOCAL_WORKDRIVE%
cd %LOCAL_WORKDIR%

rem === 1. sync changed files to build server ===

rem enforce unix line endings before upload:
sfk -quiet=2 remcr xmakerem.sh
sfk select xmakerem.sh +sft %FTPOPT% %BUILD_SERVER% cput -yes | %TONETLOG%
sfk select -dir %PRODUCT% -file !.bak !.tmp +sft %FTPOPT% %BUILD_SERVER% cput -yes | %TONETLOG%

rem === 2. run build on server ===

sfk sft %FTPOPT% %BUILD_SERVER% run "bash xmakerem.sh %PRODUCT% %TOOLCHAIN% %TARGET% %CLEAN% %LOG_TARGET% >xmakerem.log 2>&1" -yes | %TONETLOG%

rem === 3. confirm completion ===

sfk echo "[Blue]=== xmake done. ===[def]" +tolog

rem === uncomment this to keep script window open ===
rem sfk echo "[Magenta]=== waiting for user to close script window. ===[def]" +tolog +then pause
