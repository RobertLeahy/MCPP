@echo off

set rleahylib=../RLeahyLib/release

robocopy %rleahylib%/bin ./bin

echo %ERRORLEVEL%

IF ERRORLEVEL 0 goto include
IF ERRORLEVEL 1 goto include

exit /B 1

:include

robocopy %rleahylib%/include ./include/rleahylib /MIR

IF ERRORLEVEL 0 exit /B 0
IF ERRORLEVEL 1 exit /B 0

exit /B 1