@echo off

robocopy ./deps/openssl101e ./bin libeay32.dll /E /XD *

IF ERRORLEVEL 0 exit /B 0
IF ERRORLEVEL 1 exit /B 0

exit /B 1