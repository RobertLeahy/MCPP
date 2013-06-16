@echo off

robocopy "G:/Downloads/curl-7.30.0-devel-mingw64/curl-7.30.0-devel-mingw64/bin" ./bin libcurl.dll /E /XD *

IF ERRORLEVEL 0 exit /B 0
IF ERRORLEVEL 1 exit /B 0

exit /B 1