DEL=del /S /Q


INC_CURL=-I "G:/Downloads/curl-7.30.0-devel-mingw64/curl-7.30.0-devel-mingw64/include/curl"
INC_OPENSSL=-I "G:/Downloads/openssl-1.0.1e.tar/openssl-1.0.1e/openssl-1.0.1e/include/openssl" -I "G:/Downloads/openssl-1.0.1e.tar/openssl-1.0.1e/openssl-1.0.1e/include"
INC_ZLIB=-I "G:/Downloads/zlib128-dll/include/"
INC_MYSQL=-I "C:/Program Files/MySQL/MySQL Server 5.6/include/"
OPTIMIZATION=-O0 -g -fno-inline -fno-elide-constructors -DDEBUG
#OPTIMIZATION=-O3
OPTS_SHARED=-D_WIN32_WINNT=0x0600 -static-libgcc -static-libstdc++ -Wall -Wpedantic -fno-rtti -std=gnu++11 -I include $(INC_CURL) $(INC_OPENSSL) $(INC_ZLIB) $(INC_MYSQL)
GPP=G:\Downloads\x86_64-w64-mingw32-gcc-4.8.0-win64_rubenvb\mingw64\bin\g++.exe $(OPTS_SHARED) $(OPTIMIZATION)


#	DEFAULT

.PHONY: all
all: mods front_end test

.PHONY: clean
clean:
	$(DEL) obj\*
	
.PHONY: cleanall
cleanall: clean
	$(DEL) bin\*


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
obj/random.o \
obj/thread_pool.o \
obj/thread_pool_handle.o \
obj/sha1.o \
obj/new_delete.o \
bin/ssleay32.dll bin/libeay32.dll bin/libcurl.dll bin/zlib1.dll bin/data_provider.dll bin/rleahy_lib.dll
	$(GPP) -shared -o $@ obj/server.o obj/new_delete.o obj/mod.o obj/nbt.o obj/listen_handler.o obj/connection.o obj/connection_handler.o obj/connection_manager.o obj/send_handle.o obj/packet.o obj/packet_factory.o obj/packet_router.o obj/compression.o obj/rsa_key.o obj/aes_128_cfb_8.o obj/chunk.o obj/metadata.o obj/client.o obj/client_list.o obj/url.o obj/http_handler.o obj/http_request.o obj/mod_loader.o obj/random.o obj/thread_pool.o obj/thread_pool_handle.o obj/sha1.o bin/ssleay32.dll bin/libeay32.dll bin/libcurl.dll bin/zlib1.dll bin/data_provider.dll bin/rleahy_lib.dll -lws2_32

obj/server.o: src/server.cpp src/server_getters_setters.cpp src/server_setup.cpp
	$(GPP) src/server.cpp -c -o $@

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
	
obj/connection_handler.o: src/connection_handler.cpp src/send_thread.cpp src/receive_thread.cpp
	$(GPP) src/connection_handler.cpp -c -o $@
	
obj/connection_manager.o: src/connection_manager.cpp
	$(GPP) $? -c -o $@
	
obj/send_handle.o: src/send_handle.cpp
	$(GPP) $? -c -o $@
	

#	MINECRAFT COMMUNICATIONS
	
obj/packet.o: src/packet.cpp src/protocol_analysis.cpp
	$(GPP) src/packet.cpp -c -o $@
	
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
	
obj/http_handler.o: src/http_handler.cpp src/http_handler_callbacks.cpp
	$(GPP) src/http_handler.cpp -c -o $@
	
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
	
bin/data_providers/mysql_data_provider.dll: obj/mysql_data_provider.o obj/data_provider.o bin/libmysql.dll bin/rleahy_lib.dll obj/thread_pool.o obj/thread_pool_handle.o obj/new_delete.o
	$(GPP) -shared -o bin/data_providers/data_provider.dll obj/mysql_data_provider.o obj/data_provider.o bin/libmysql.dll bin/rleahy_lib.dll obj/thread_pool.o obj/thread_pool_handle.o obj/new_delete.o
	cmd /c "move bin\data_providers\data_provider.dll bin\data_providers\mysql_data_provider.dll"

obj/mysql_data_provider.o: src/mysql_data_provider.cpp src/login_info.cpp
	$(GPP) src/mysql_data_provider.cpp -c -o $@
	
obj/data_provider.o: src/data_provider.cpp
	$(GPP) $? -c -o $@
	

#	THREAD POOL

.PHONY: thread_pool
thread_pool: obj/thread_pool.o obj/thread_pool_handle.o

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
bin/mods/chat.dll \
bin/mods/disconnect.dll \
bin/mods/player_list.dll \
bin/mods/op.dll \
bin/mods/ban.dll \
bin/chat_mods/basic_chat.dll \
bin/chat_mods/whisper.dll \
bin/chat_mods/chat_login.dll \
bin/chat_mods/ban_info.dll \
bin/chat_mods/info.dll \
bin/chat_mods/chat_op.dll \
bin/chat_mods/kick.dll


#	PING SUPPORT

bin/mods/ping.dll: src/ping/main.cpp bin/mcpp.dll bin/rleahy_lib.dll obj/new_delete.o
	$(GPP) -shared -o $@ src/ping/main.cpp obj/new_delete.o bin/mcpp.dll bin/rleahy_lib.dll
	

#	AUTHENTICATION SUPPORT

bin/mods/auth.dll: src/auth/main.cpp bin/mcpp.dll bin/rleahy_lib.dll obj/new_delete.o
	$(GPP) -shared -o $@ src/auth/main.cpp obj/new_delete.o bin/mcpp.dll bin/rleahy_lib.dll
	
	
#	KEEP ALIVE SUPPORT

bin/mods/keep_alive.dll: src/keep_alive/main.cpp bin/mcpp.dll bin/rleahy_lib.dll obj/new_delete.o
	$(GPP) -shared -o $@ src/keep_alive/main.cpp obj/new_delete.o bin/mcpp.dll bin/rleahy_lib.dll
	
	
#	DISCONNECT SUPPORT

bin/mods/disconnect.dll: src/disconnect/main.cpp bin/mcpp.dll bin/rleahy_lib.dll obj/new_delete.o
	$(GPP) -shared -o $@ src/disconnect/main.cpp obj/new_delete.o bin/mcpp.dll bin/rleahy_lib.dll
	
	
#	CHAT SUPPORT

bin/mods/chat.dll: src/chat/main.cpp bin/mcpp.dll bin/rleahy_lib.dll obj/new_delete.o src/chat/chat_misc.cpp src/chat/chat_parser.cpp
	$(GPP) -shared -o $@ src/chat/main.cpp obj/new_delete.o bin/mcpp.dll bin/rleahy_lib.dll src/chat/chat_misc.cpp src/chat/chat_parser.cpp
	
	
#	PLAYER LIST SUPPORT

bin/mods/player_list.dll: bin/mcpp.dll bin/rleahy_lib.dll obj/new_delete.o src/player_list/main.cpp
	$(GPP) -shared -o $@ bin/mcpp.dll bin/rleahy_lib.dll obj/new_delete.o src/player_list/main.cpp
	
	
#	OPERATOR SUPPORT

bin/mods/op.dll: bin/mcpp.dll bin/rleahy_lib.dll bin/data_provider.dll src/op/main.cpp obj/new_delete.o
	$(GPP) -shared -o $@ bin/mcpp.dll bin/data_provider.dll bin/rleahy_lib.dll obj/new_delete.o src/op/main.cpp
	
	
#	BAN SUPPORT

bin/mods/ban.dll: bin/mcpp.dll bin/rleahy_lib.dll bin/data_provider.dll src/ban/main.cpp obj/new_delete.o
	$(GPP) -shared -o $@ bin/mcpp.dll bin/data_provider.dll bin/rleahy_lib.dll obj/new_delete.o src/ban/main.cpp
	

#	CHAT SUB-MODULES


#	GLOBAL CHAT

bin/chat_mods/basic_chat.dll: bin/mods/chat.dll src/basic_chat/main.cpp obj/new_delete.o bin/rleahy_lib.dll bin/mcpp.dll
	$(GPP) -shared -o $@ src/basic_chat/main.cpp obj/new_delete.o bin/mcpp.dll bin/rleahy_lib.dll bin/mods/chat.dll
	
#	WHISPERS

bin/chat_mods/whisper.dll: bin/mods/chat.dll src/whisper/main.cpp obj/new_delete.o bin/rleahy_lib.dll bin/mcpp.dll
	$(GPP) -shared -o $@ src/whisper/main.cpp obj/new_delete.o bin/mcpp.dll bin/rleahy_lib.dll bin/mods/chat.dll
	
#	LOGIN/LOGOUT BROADCASTS

bin/chat_mods/chat_login.dll: bin/mods/chat.dll src/chat_login/main.cpp obj/new_delete.o bin/rleahy_lib.dll bin/mcpp.dll
	$(GPP) -shared -o $@ src/chat_login/main.cpp obj/new_delete.o bin/mcpp.dll bin/rleahy_lib.dll bin/mods/chat.dll
	
#	INFORMATION THROUGH CHAT

bin/chat_mods/info.dll: bin/mods/chat.dll src/info/main.cpp obj/new_delete.o bin/rleahy_lib.dll bin/mcpp.dll bin/mods/op.dll
	$(GPP) -shared -o $@ src/info/main.cpp obj/new_delete.o bin/mcpp.dll bin/rleahy_lib.dll bin/mods/chat.dll bin/mods/op.dll
	
#	INFORMATION ABOUT BANS THROUGH CHAT

bin/chat_mods/ban_info.dll: bin/mods/chat.dll bin/mods/op.dll bin/mods/ban.dll src/ban_info/main.cpp obj/new_delete.o bin/rleahy_lib.dll bin/mcpp.dll
	$(GPP) -shared -o $@ bin/mods/chat.dll bin/mods/op.dll bin/mods/ban.dll src/ban_info/main.cpp obj/new_delete.o bin/rleahy_lib.dll bin/mcpp.dll
	
#	OP/DEOP THROUGH CHAT

bin/chat_mods/chat_op.dll: bin/mods/chat.dll bin/mods/op.dll src/chat_op/main.cpp obj/new_delete.o bin/rleahy_lib.dll bin/mcpp.dll
	$(GPP) -shared -o $@ bin/mods/chat.dll bin/mods/op.dll src/chat_op/main.cpp obj/new_delete.o bin/rleahy_lib.dll bin/mcpp.dll
	
#	KICKING THROUGH CHAT

bin/chat_mods/kick.dll: bin/mods/chat.dll bin/mods/op.dll src/kick/main.cpp obj/new_delete.o bin/rleahy_lib.dll bin/mcpp.dll
	$(GPP) -shared -o $@ bin/mods/chat.dll bin/mods/op.dll src/kick/main.cpp obj/new_delete.o bin/rleahy_lib.dll bin/mcpp.dll
	
	
#	SERVER FRONT-END

.PHONY: front_end
front_end: bin/server.exe bin/mcpp.exe

bin/server.exe: src/test_front_end/main.cpp src/test_front_end/test.cpp bin/mcpp.dll bin/rleahy_lib.dll obj/new_delete.o
	$(GPP) -o $@ src/test_front_end/main.cpp bin/mcpp.dll bin/rleahy_lib.dll obj/new_delete.o
	
bin/mcpp.exe: src/interactive_front_end/main.cpp bin/mcpp.dll bin/rleahy_lib.dll bin/data_provider.dll obj/new_delete.o
	$(GPP) -o $@ src/interactive_front_end/main.cpp bin/mcpp.dll bin/rleahy_lib.dll bin/data_provider.dll obj/new_delete.o
	
	
#	TEST


.PHONY: test
test: bin/test.exe

bin/test.exe: bin/rleahy_lib.dll src/test/test.cpp
	$(GPP) -o $@ bin/rleahy_lib.dll src/test/test.cpp