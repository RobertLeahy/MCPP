@echo off

robocopy ./deps/zlib128 ./bin zlib1.dll /E /XD *

IF ERRORLEVEL 0 exit /B 0
IF ERRORLEVEL 1 exit /B 0

exit /B 1