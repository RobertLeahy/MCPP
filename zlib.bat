@echo off

robocopy "G:/Downloads/zlib128/zlib-1.2.8/" ./bin zlib1.dll /E /XD *

IF ERRORLEVEL 0 exit /B 0
IF ERRORLEVEL 1 exit /B 0

exit /B 1