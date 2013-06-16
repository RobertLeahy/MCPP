@echo off

robocopy "C:/Program Files/MySQL/MySQL Server 5.6/lib/" ./bin libmysql.dll /E /XD *

IF ERRORLEVEL 0 exit /B 0
IF ERRORLEVEL 1 exit /B 0

exit /B 1