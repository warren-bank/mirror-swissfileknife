@echo off

if exist "%~dp0.\.path.bat" call "%~dp0.\.path.bat"

if [%1] == [] (
  set ftp_root_dir="%cd%"
) else (
  set ftp_root_dir="%~1"
)

sfk.exe ftpserv -port=21 -timeout=3600 -rw -noclose -usedir %ftp_root_dir%
