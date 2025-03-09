@echo off
rem instantly view all text files in directory tree

sfk pathfind -quiet dview.exe
IF %ERRORLEVEL%==0 GOTO havedview
echo To view all text files instantly, download Depeche View:
echo sfk wget http://stahlworks.com/dview.exe
echo Then re-run this batch file.
GOTO done

:havedview
start dview -wrap .
GOTO done

:done
