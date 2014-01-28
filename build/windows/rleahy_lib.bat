@echo off

robocopy "Z:\Code Store\RLeahyLib\release\bin" ./bin

echo %ERRORLEVEL%

IF ERRORLEVEL 0 goto include
IF ERRORLEVEL 1 goto include

exit /B 1

:include

robocopy "Z:\Code Store\RLeahyLib\release\include" ./include/rleahylib /MIR

IF ERRORLEVEL 0 exit /B 0
IF ERRORLEVEL 1 exit /B 0

exit /B 1