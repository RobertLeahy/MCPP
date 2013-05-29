@echo off


set robocopy_shared=/NFL /NDL /NJH /NJS /nc /ns /np /MIR
set mysql_lib_loc="C:/Program Files/MySQL/MySQL Server 5.6/lib/"
set mysql_include_loc="C:/Program Files/MySQL/MySQL Server 5.6/include/"


rmdir ..\bin /S /Q > nul 2> nul
mkdir ..\bin > nul 2> nul


echo.
echo ====GETTING LATEST VERSION OF RLEAHYLIB====
echo.
robocopy ../../RLeahyLib/release/bin ../bin %robocopy_shared% > nul 2> nul
robocopy ../../RLeahyLib/release/include ../include/rleahylib %robocopy_shared% > nul 2> nul


echo.
echo ====GETTING LIBMYSQL====
echo.
robocopy %mysql_lib_loc% ../bin libmysql.dll %robocopy_shared% /XD * > nul 2> nul
echo.


set shared_params=-D_WIN32_WINNT=0x0600 -static-libgcc -static-libstdc++ -Wall -Wpedantic -fno-rtti -std=gnu++11 -I ../include/ ../bin/rleahy_lib.dll
IF /I "%~1"=="-release" set common_params=-O3 %shared_params%
IF /I NOT "%~1"=="-release" set common_params=-O0 -g -fno-inline -fno-elide-constructors -DDEBUG %shared_params%


IF /I "%~1"=="-release" echo ====BUILDING RELEASE====
IF /I NOT "%~1"=="-release" echo ====BUILDING DEBUG====
echo.
echo.


echo ====BUILDING DATA PROVIDERS====
echo.


mkdir ..\bin\data_providers > nul 2> nul


g++.exe %common_params% -I %mysql_include_loc% -o ../bin/data_providers/data_provider.dll -shared mysql_data_provider.cpp data_provider.cpp thread_pool.cpp thread_pool_handle.cpp ../bin/libmysql.dll
move ..\bin\data_providers\data_provider.dll ..\bin\data_providers\mysql_data_provider.dll > nul 2> nul


echo.


echo ====INSTALLING DATA PROVIDER====
echo.


copy ..\bin\data_providers\mysql_data_provider.dll ..\bin\data_provider.dll > nul 2> nul


echo.


echo ====BUILDING SERVER====
echo.


set server_all=main.cpp nbt.cpp thread_pool.cpp thread_pool_handle.cpp listen_handler.cpp connection.cpp connection_handler.cpp connection_manager.cpp server.cpp packet.cpp packet_factory.cpp metadata.cpp send_handle.cpp client.cpp client_list.cpp packet_router.cpp
g++.exe %common_params% %server_all% ../bin/data_provider.dll -o ../bin/server.exe


echo.


echo ====DONE====
echo.