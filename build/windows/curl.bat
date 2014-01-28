@echo off

robocopy ./deps/libcurl7300/bin ./bin libcurl.dll /E /XD *

IF ERRORLEVEL 0 exit /B 0
IF ERRORLEVEL 1 exit /B 0

exit /B 1