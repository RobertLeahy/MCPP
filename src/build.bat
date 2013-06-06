@echo off


set robocopy_shared=/NFL /NDL /NJH /NJS /nc /ns /np /MIR
set mysql_lib_loc="C:/Program Files/MySQL/MySQL Server 5.6/lib/"
set mysql_include_loc="C:/Program Files/MySQL/MySQL Server 5.6/include/"
set zlib_lib_loc="G:/Downloads/zlib128/zlib-1.2.8/"
set zlib_include_loc="G:/Downloads/zlib128-dll/include/"
set openssl_lib_loc="G:/Downloads/openssl-1.0.1e.tar/openssl-1.0.1e/openssl-1.0.1e"
set openssl_include_loc="G:/Downloads/openssl-1.0.1e.tar/openssl-1.0.1e/openssl-1.0.1e/include/openssl" -I "G:/Downloads/openssl-1.0.1e.tar/openssl-1.0.1e/openssl-1.0.1e/include"
set curl_include_loc="G:/Downloads/curl-7.30.0-devel-mingw64/curl-7.30.0-devel-mingw64/include/curl"
set curl_lib_loc="G:/Downloads/curl-7.30.0-devel-mingw64/curl-7.30.0-devel-mingw64/bin"


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
echo ====GETTING ZLIB====
echo.
robocopy %zlib_lib_loc% ../bin zlib1.dll %robocopy_shared% /XD * > nul 2> nul


echo.
echo ====GETTING OPENSSL====
echo.
robocopy %openssl_lib_loc% ../bin libeay32.dll %robocopy_shared% /XD * > nul 2> nul
robocopy %openssl_lib_loc% ../bin ssleay32.dll %robocopy_shared% /XD * > nul 2> nul

echo.
echo ====GETTING CURL====
echo.
robocopy %curl_lib_loc% ../bin libcurl.dll %robocopy_shared% /XD * > nul 2> nul


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


set main=server.cpp
set interfaces=mod.cpp
set nbt=nbt.cpp
set thread_pool=thread_pool.cpp thread_pool_handle.cpp
set network=listen_handler.cpp connection.cpp connection_handler.cpp connection_manager.cpp send_handle.cpp
set mc_comm=packet.cpp packet_factory.cpp packet_router.cpp rsa_key.cpp compression.cpp aes_128_cfb_8.cpp
set mc_data=chunk.cpp metadata.cpp
set server_data=client.cpp client_list.cpp
set misc=url.cpp sha1.cpp http_handler.cpp http_request.cpp
set source_all=%main% %nbt% %thread_pool% %network% %mc_comm% %mc_data% %server_data% %misc% %interfaces%
set include_all=-I %zlib_include_loc% -I %openssl_include_loc% -I %curl_include_loc%
set library_all=../bin/data_provider.dll ../bin/zlib1.dll ../bin/libeay32.dll ../bin/ssleay32.dll ../bin/libcurl.dll -lws2_32
set server_all=%include_all% %source_all% %library_all%
g++.exe %common_params% %server_all% -shared -o ../bin/mcpp.dll

set mcpp=../bin/mcpp.dll

echo.
echo ====BUILDING MODULES====
echo.

mkdir ..\bin\mods > nul 2> nul

g++.exe %include_all% %common_params% ping/main.cpp %mcpp% -shared -o ../bin/mods/ping.dll

echo.
echo ====BUILDING SERVER FRONT-END====
echo.

g++.exe %include_all% %common_params% main.cpp %mcpp% -o ../bin/server.exe

echo.
echo ====DONE====
echo.