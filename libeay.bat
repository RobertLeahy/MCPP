@echo off

robocopy "G:/Downloads/openssl-1.0.1e.tar/openssl-1.0.1e/openssl-1.0.1e" ./bin libeay32.dll /E /XD *

IF ERRORLEVEL 0 exit /B 0
IF ERRORLEVEL 1 exit /B 0

exit /B 1