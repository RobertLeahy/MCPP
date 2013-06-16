DEL=del /S /Q


INC_CURL=-I "G:/Downloads/curl-7.30.0-devel-mingw64/curl-7.30.0-devel-mingw64/include/curl"
INC_OPENSSL=-I "G:/Downloads/openssl-1.0.1e.tar/openssl-1.0.1e/openssl-1.0.1e/include/openssl" -I "G:/Downloads/openssl-1.0.1e.tar/openssl-1.0.1e/openssl-1.0.1e/include"
INC_ZLIB=-I "G:/Downloads/zlib128-dll/include/"
INC_MYSQL=-I "C:/Program Files/MySQL/MySQL Server 5.6/include/"
OPTIMIZATION=-O0 -g -fno-inline -fno-elide-constructors -DDEBUG
OPTS_SHARED=-D_WIN32_WINNT=0x0600 -static-libgcc -static-libstdc++ -Wall -Wpedantic -fno-rtti -std=gnu++11 -I include $(INC_CURL) $(INC_OPENSSL) $(INC_ZLIB) $(INC_MYSQL)
GPP=g++.exe $(OPTS_SHARED) $(OPTIMIZATION)


#	DEFAULT

.PHONY: all
all: mods front_end

.PHONY: clean
clean:
	$(DEL) obj\*
	
.PHONY: cleanall
cleanall: clean
	$(DEL) bin\*
	$(DEL) bin\data_providers\*
	$(DEL) bin\mods\*
	$(DEL) bin\chat_mods\*


#	LIBRARIES

bin/libmysql.dll:
	mysql.bat
	
bin/rleahy_lib.dll:
	rleahy_lib.bat
	
bin/ssleay32.dll:
	ssleay.bat
	
bin/libeay32.dll:
	libeay.bat
	
bin/libcurl.dll:
	curl.bat
	
bin/zlib1.dll:
	zlib.bat
	
	
#	MCPP MAIN LIBRARY

.PHONY: mcpp
mcpp: bin/mcpp.dll

bin/mcpp.dll: \
obj/server.o \
obj/mod.o \
obj/nbt.o \
obj/listen_handler.o \
obj/connection.o \
obj/connection_handler.o \
obj/server.o \
obj/connection_manager.o \
obj/send_handle.o \
obj/packet.o \
obj/packet_factory.o \
obj/packet_router.o \
obj/compression.o \
obj/rsa_key.o \
obj/aes_128_cfb_8.o \
obj/chunk.o \
obj/metadata.o \
obj/client.o \
obj/client_list.o \
obj/url.o \
obj/http_handler.o \
obj/http_request.o \
obj/mod_loader.o \
obj/new_delete.o \
obj/random.o \
obj/thread_pool.o \
obj/thread_pool_handle.o \
obj/sha1.o \
bin/ssleay32.dll bin/libeay32.dll bin/libcurl.dll bin/zlib1.dll bin/rleahy_lib.dll bin/data_provider.dll
	$(GPP) $? -shared -o $@ -lws2_32

obj/server.o: src/server.cpp
	$(GPP) $? -c -o $@

obj/mod.o: src/mod.cpp
	$(GPP) $? -c -o $@
	
obj/client.o: src/client.cpp
	$(GPP) $? -c -o $@
	
obj/client_list.o: src/client_list.cpp
	$(GPP) $? -c -o $@
	
obj/mod_loader.o: src/mod_loader.cpp
	$(GPP) $? -c -o $@
	
obj/new_delete.o: src/new_delete.cpp
	$(GPP) $? -c -o $@

	
#	MINECRAFT DATA FORMAT INTEROP
	
obj/nbt.o: src/nbt.cpp
	$(GPP) $? -c -o $@
	
obj/compression.o: src/compression.cpp
	$(GPP) $? -c -o $@
	
obj/chunk.o: src/chunk.cpp
	$(GPP) $? -c -o $@
	
obj/metadata.o: src/metadata.cpp
	$(GPP) $? -c -o $@
	
	
#	NETWORK STACK
	
obj/listen_handler.o: src/listen_handler.cpp
	$(GPP) $? -c -o $@
	
obj/connection.o: src/connection.cpp
	$(GPP) $? -c -o $@
	
obj/connection_handler.o: src/connection_handler.cpp
	$(GPP) $? -c -o $@
	
obj/connection_manager.o: src/connection_manager.cpp
	$(GPP) $? -c -o $@
	
obj/send_handle.o: src/send_handle.cpp
	$(GPP) $? -c -o $@
	

#	MINECRAFT COMMUNICATIONS
	
obj/packet.o: src/packet.cpp
	$(GPP) $? -c -o $@
	
obj/packet_factory.o: src/packet_factory.cpp
	$(GPP) $? -c -o $@
	
obj/packet_router.o: src/packet_router.cpp
	$(GPP) $? -c -o $@
	
obj/rsa_key.o: src/rsa_key.cpp
	$(GPP) $? -c -o $@
	
obj/aes_128_cfb_8.o: src/aes_128_cfb_8.cpp
	$(GPP) $? -c -o $@
	
obj/sha1.o: src/sha1.cpp
	$(GPP) $? -c -o $@
	

#	HTTP

obj/url.o: src/url.cpp
	$(GPP) $? -c -o $@
	
obj/http_handler.o: src/http_handler.cpp
	$(GPP) $? -c -o $@
	
obj/http_request.o: src/http_request.cpp
	$(GPP) $? -c -o $@
	
	
#	RANDOM NUMBER GENERATION

obj/random.o: src/random.cpp
	$(GPP) $? -c -o $@


#	DATA PROVIDERS

.PHONY: data_providers
data_providers: bin/data_provider.dll

bin/data_provider.dll: bin/data_providers/mysql_data_provider.dll
	cmd /c "copy bin\data_providers\mysql_data_provider.dll bin\data_provider.dll"
	
bin/data_providers/mysql_data_provider.dll: obj/mysql_data_provider.o obj/data_provider.o bin/libmysql.dll bin/rleahy_lib.dll obj/thread_pool.o obj/thread_pool_handle.o
	$(GPP) $? -shared -o bin/data_providers/data_provider.dll
	cmd /c "move bin\data_providers\data_provider.dll bin\data_providers\mysql_data_provider.dll"

obj/mysql_data_provider.o: src/mysql_data_provider.cpp
	$(GPP) $? -c -o $@
	
obj/data_provider.o: src/data_provider.cpp
	$(GPP) $? -c -o $@
	

#	THREAD POOL

obj/thread_pool.o: src/thread_pool.cpp
	$(GPP) $? -c -o $@
	
obj/thread_pool_handle.o: src/thread_pool_handle.cpp
	$(GPP) $? -c -o $@
	
	
#	MODULES

.PHONY: mods
mods: \
bin/mods/ping.dll \
bin/mods/auth.dll \
bin/mods/keep_alive.dll \
bin/mods/chat.dll


#	PING SUPPORT

bin/mods/ping.dll: src/ping/main.cpp bin/rleahy_lib.dll bin/mcpp.dll
	$(GPP) $? -shared -o $@
	

#	AUTHENTICATION SUPPORT

bin/mods/auth.dll: src/auth/main.cpp bin/rleahy_lib.dll bin/mcpp.dll
	$(GPP) $? -shared -o $@
	
	
#	KEEP ALIVE SUPPORT

bin/mods/keep_alive.dll: src/keep_alive/main.cpp bin/rleahy_lib.dll bin/mcpp.dll
	$(GPP) $? -shared -o $@
	
	
#	CHAT SUPPORT

bin/mods/chat.dll: src/chat/main.cpp bin/rleahy_lib.dll bin/mcpp.dll
	$(GPP) $? -shared -o $@
	
	
#	SERVER FRONT-END

.PHONY: front_end
front_end: bin/server.exe

bin/server.exe: bin/mcpp.dll src/main.cpp bin/rleahy_lib.dll
	$(GPP) $? -o $@