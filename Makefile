RMDIR=rmdir /S /Q


INC_CURL=-I "G:/Downloads/curl-7.30.0-devel-mingw64/curl-7.30.0-devel-mingw64/include"
INC_OPENSSL=-I "G:/Downloads/openssl-1.0.1e.tar/openssl-1.0.1e/openssl-1.0.1e/include"
INC_ZLIB=-I "G:/Downloads/zlib128-dll/include/"
INC_MYSQL=-I "C:/Program Files/MySQL/MySQL Server 5.6/include/"
OPTIMIZATION=-O0 -g -fno-inline -fno-elide-constructors -DDEBUG
#OPTIMIZATION=-O3 -march=native
OPTS_SHARED=-D_WIN32_WINNT=0x0600 -static-libgcc -static-libstdc++ -Wall -Wpedantic -Werror -fno-rtti -std=gnu++11 -I include $(INC_CURL) $(INC_OPENSSL) $(INC_ZLIB) $(INC_MYSQL)
GPP=G:\Downloads\x86_64-w64-mingw32-gcc-4.8.0-win64_rubenvb\mingw64\bin\g++.exe $(OPTS_SHARED) $(OPTIMIZATION)


#	DEFAULT

.PHONY: all
all: mcpp mods front_end

.PHONY: clean
clean:
	-$(RMDIR) obj

.PHONY: cleanall
cleanall: clean
	-$(RMDIR) bin
	
	
#	LIBRARIES

bin/libmysql.dll: | bin
	mysql.bat
	
bin/rleahy_lib.dll: | bin
	rleahy_lib.bat
	
bin/ssleay32.dll: | bin
	ssleay.bat
	
bin/libeay32.dll: | bin
	libeay.bat
	
bin/libcurl.dll: | bin
	curl.bat
	
bin/zlib1.dll: | bin
	zlib.bat
	
	
#	DIRECTORIES

bin:
	mkdir bin

obj:
	mkdir obj
	
bin/data_providers: | bin
	mkdir bin\data_providers
	
	
#	MCPP MAIN LIBRARY

.PHONY: mcpp
mcpp: bin/mcpp.dll

bin/mcpp.dll: \
obj/server.o \
obj/mod.o \
obj/nbt.o \
obj/network.o \
obj/packet.o \
obj/packet_factory.o \
obj/packet_router.o \
obj/compression.o \
obj/rsa_key.o \
obj/aes_128_cfb_8.o \
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
obj/noise.o \
bin/ssleay32.dll \
bin/libeay32.dll \
bin/libcurl.dll \
bin/zlib1.dll \
bin/rleahy_lib.dll \
include/data_provider.hpp | \
bin \
bin/data_provider.dll
	$(GPP) -shared -o $@ obj/server.o \
	obj/mod.o \
	obj/nbt.o \
	obj/network.o \
	obj/packet.o \
	obj/packet_factory.o \
	obj/packet_router.o \
	obj/compression.o \
	obj/rsa_key.o \
	obj/aes_128_cfb_8.o \
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
	obj/noise.o \
	bin/ssleay32.dll \
	bin/libeay32.dll \
	bin/libcurl.dll \
	bin/zlib1.dll \
	bin/data_provider.dll \
	bin/rleahy_lib.dll \
	-lws2_32
	
obj/server.o: \
src/server.cpp \
src/server_getters_setters.cpp \
src/server_setup.cpp \
include/server.hpp \
include/thread_pool.hpp \
include/network.hpp \
include/data_provider.hpp \
include/event.hpp \
include/typedefs.hpp \
include/http_handler.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/server.cpp
	
obj/mod.o: \
src/mod.cpp \
include/mod.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/mod.cpp
	
obj/client.o: \
src/client.cpp \
include/server.hpp \
include/network.hpp \
include/packet.hpp \
include/aes_128_cfb_8.hpp \
include/client.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/client.cpp
	
obj/client_list.o: \
src/client_list.cpp \
include/network.hpp \
include/packet.hpp \
include/aes_128_cfb_8.hpp \
include/client.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/client_list.cpp
	
obj/mod_loader.o: \
src/mod_loader.cpp \
include/mod.hpp \
include/typedefs.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/mod_loader.cpp
	
obj/new_delete.o: \
src/new_delete.cpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/new_delete.cpp
	

#	MINECRAFT DATA FORMAT INTEROP

obj/nbt.o: \
src/nbt.cpp \
src/tag.cpp \
src/named_tag.cpp \
include/nbt.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/nbt.cpp
	
obj/compression.o: \
src/compression.cpp \
include/compression.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/compression.cpp
	
obj/metadata.o: \
include/metadata.hpp \
src/metadata.cpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/metadata.cpp

	
#	NETWORK STACK

obj/network.o: \
src/network_windows.cpp \
src/network.cpp \
include/thread_pool.hpp \
include/network.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/network.cpp
	

#	MINECRAFT COMMUNICATIONS

obj/packet.o: \
src/packet.cpp \
src/protocol_analysis.cpp \
include/metadata.hpp \
include/packet.hpp \
include/compression.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/packet.cpp
	
obj/packet_factory.o: \
src/packet_factory.cpp \
include/packet.hpp \
include/metadata.hpp \
include/compression.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/packet_factory.cpp
	
obj/packet_router.o: \
src/packet_router.cpp \
include/packet.hpp \
include/metadata.hpp \
include/compression.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/packet_router.cpp
	
obj/rsa_key.o: \
src/rsa_key.cpp \
include/rsa_key.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/rsa_key.cpp
	
obj/aes_128_cfb_8.o: \
src/aes_128_cfb_8.cpp \
include/aes_128_cfb_8.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/aes_128_cfb_8.cpp
	
obj/sha1.o: \
src/sha1.cpp \
include/sha1.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/sha1.cpp
	
	
#	HTTP

obj/url.o: \
src/url.cpp \
include/url.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/url.cpp
	
obj/http_handler.o: \
src/http_handler.cpp \
src/http_handler_callbacks.cpp \
include/http_handler.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/http_handler.cpp
	
obj/http_request.o: \
src/http_request.cpp \
include/http_handler.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/http_request.cpp
	
	
#	RANDOM NUMBER GENERATION

obj/random.o: \
src/random.cpp \
include/random.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/random.cpp
	
obj/noise.o: \
src/noise.cpp \
include/noise.hpp \
include/random.hpp \
include/fma.hpp
	$(GPP) -c -o $@ src/noise.cpp
	
	
#	THREAD POOL

obj/thread_pool.o: \
src/thread_pool.cpp \
include/thread_pool.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/thread_pool.cpp
	
obj/thread_pool_handle.o: \
src/thread_pool_handle.cpp \
include/thread_pool.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/thread_pool_handle.cpp
	
	
#	DATA PROVIDERS

.PHONY: data_providers
data_providers: bin/data_provider.dll

bin/data_provider.dll: bin/data_providers/mysql_data_provider.dll
	cmd /c "copy bin\data_providers\mysql_data_provider.dll bin\data_provider.dll"
	
bin/data_providers/mysql_data_provider.dll: \
include/mysql_data_provider/mysql_data_provider.hpp \
include/data_provider.hpp \
bin/libmysql.dll \
bin/rleahy_lib.dll \
obj/new_delete.o \
obj/data_provider.o \
src/mysql_data_provider/constructor.cpp \
src/mysql_data_provider/misc.cpp \
src/mysql_data_provider/factory.cpp \
src/mysql_data_provider/binary.cpp \
src/mysql_data_provider/log.cpp \
src/mysql_data_provider/settings.cpp \
src/mysql_data_provider/key_value.cpp \
src/mysql_data_provider/info.cpp | \
obj \
bin \
bin/data_providers
	$(GPP) -shared -o bin/data_providers/data_provider.dll \
	obj/new_delete.o \
	obj/data_provider.o \
	bin/rleahy_lib.dll \
	bin/libmysql.dll \
	src/mysql_data_provider/constructor.cpp \
	src/mysql_data_provider/misc.cpp \
	src/mysql_data_provider/factory.cpp \
	src/mysql_data_provider/binary.cpp \
	src/mysql_data_provider/log.cpp \
	src/mysql_data_provider/settings.cpp \
	src/mysql_data_provider/key_value.cpp \
	src/mysql_data_provider/info.cpp
	cmd /c "move bin\data_providers\data_provider.dll bin\data_providers\mysql_data_provider.dll"
	
obj/data_provider.o: \
src/data_provider.cpp \
include/data_provider.hpp \
bin/rleahy_lib.dll | \
obj
	$(GPP) -c -o $@ src/data_provider.cpp
	
	
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
bin/command_mods/whisper.dll \
bin/chat_mods/chat_login.dll \
bin/chat_mods/chat_op.dll \
bin/chat_mods/kick.dll \
bin/mods/world.dll \
bin/mods/player.dll \
bin/chat_mods/command.dll \
bin/command_mods/test.dll \
bin/command_mods/info.dll \
bin/info_mods/ban_info.dll \
bin/info_mods/dp_info.dll \
bin/info_mods/op_info.dll


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
	

#	WORLD

bin/mods/world.dll: \
src/world/block_id.cpp \
src/world/column_container.cpp \
src/world/column_id.cpp \
src/world/generator.cpp \
src/world/generators.cpp \
src/world/has_skylight.cpp \
src/world/set_seed.cpp \
src/world/world_generator.cpp \
src/world/world_lock.cpp \
src/world/world_lock_info.cpp \
src/world/world_lock_request.cpp \
include/world/world.hpp \
obj/new_delete.o | \
bin/rleahy_lib.dll \
bin/mcpp.dll
	$(GPP) -shared -o $@ bin/mcpp.dll bin/rleahy_lib.dll obj/new_delete.o \
	src/world/block_id.cpp \
	src/world/column_container.cpp \
	src/world/column_id.cpp \
	src/world/generator.cpp \
	src/world/generators.cpp \
	src/world/has_skylight.cpp \
	src/world/set_seed.cpp \
	src/world/world_generator.cpp \
	src/world/world_lock.cpp \
	src/world/world_lock_info.cpp \
	src/world/world_lock_request.cpp
	
	
#	PLAYER

bin/mods/player.dll: bin/mcpp.dll bin/rleahy_lib.dll obj/new_delete.o src/player/player_position.cpp include/player/player.hpp include/world/world.hpp bin/mods/world.dll
	$(GPP) -shared -o $@ bin/mcpp.dll bin/rleahy_lib.dll obj/new_delete.o src/player/player_position.cpp bin/mods/world.dll
	

#	CHAT SUB-MODULES


#	COMMAND HANDLING

bin/chat_mods/command.dll: \
src/command/main.cpp \
obj/new_delete.o \
include/chat/chat.hpp \
bin/mcpp.dll | \
bin/rleahy_lib.dll \
bin/chat_mods \
bin/mods/chat.dll
	$(GPP) -shared -o $@ src/command/main.cpp obj/new_delete.o bin/mcpp.dll bin/rleahy_lib.dll bin/mods/chat.dll

#	GLOBAL CHAT

bin/chat_mods/basic_chat.dll: bin/mods/chat.dll src/basic_chat/main.cpp obj/new_delete.o bin/rleahy_lib.dll bin/mcpp.dll
	$(GPP) -shared -o $@ src/basic_chat/main.cpp obj/new_delete.o bin/mcpp.dll bin/rleahy_lib.dll bin/mods/chat.dll
	
#	LOGIN/LOGOUT BROADCASTS

bin/chat_mods/chat_login.dll: bin/mods/chat.dll src/chat_login/main.cpp obj/new_delete.o bin/rleahy_lib.dll bin/mcpp.dll
	$(GPP) -shared -o $@ src/chat_login/main.cpp obj/new_delete.o bin/mcpp.dll bin/rleahy_lib.dll bin/mods/chat.dll
	
#	OP/DEOP THROUGH CHAT

bin/chat_mods/chat_op.dll: bin/mods/chat.dll bin/mods/op.dll src/chat_op/main.cpp obj/new_delete.o bin/rleahy_lib.dll bin/mcpp.dll
	$(GPP) -shared -o $@ bin/mods/chat.dll bin/mods/op.dll src/chat_op/main.cpp obj/new_delete.o bin/rleahy_lib.dll bin/mcpp.dll
	
#	KICKING THROUGH CHAT

bin/chat_mods/kick.dll: bin/mods/chat.dll bin/mods/op.dll src/kick/main.cpp obj/new_delete.o bin/rleahy_lib.dll bin/mcpp.dll
	$(GPP) -shared -o $@ bin/mods/chat.dll bin/mods/op.dll src/kick/main.cpp obj/new_delete.o bin/rleahy_lib.dll bin/mcpp.dll
	
	
#	COMMANDS


#	INFORMATION

bin/command_mods/info.dll: \
obj/new_delete.o \
src/info/main.cpp \
include/chat/chat.hpp \
include/server.hpp \
include/command/command.hpp \
include/op/op.hpp | \
bin/mods/chat.dll \
bin/chat_mods/command.dll \
bin/mcpp.dll \
bin/rleahy_lib.dll \
bin/mods/op.dll
	$(GPP) -shared -o $@ \
	bin/mods/chat.dll \
	bin/chat_mods/command.dll \
	bin/rleahy_lib.dll \
	bin/mcpp.dll \
	bin/mods/op.dll \
	obj/new_delete.o \
	src/info/main.cpp


#	WHISPERS

bin/command_mods/whisper.dll: \
obj/new_delete.o \
src/whisper/main.cpp \
include/chat/chat.hpp \
include/server.hpp \
include/client.hpp \
include/command/command.hpp | \
bin/mods/chat.dll \
bin/chat_mods/command.dll \
bin/mcpp.dll \
bin/rleahy_lib.dll \
bin/mods/op.dll
	$(GPP) -shared -o $@ \
	bin/mods/chat.dll \
	bin/chat_mods/command.dll \
	bin/rleahy_lib.dll \
	bin/mcpp.dll \
	obj/new_delete.o \
	src/whisper/main.cpp
	
	
#	INFORMATION SUB-MODULES


#	BAN INFORMATION

bin/info_mods/ban_info.dll: \
obj/new_delete.o \
src/ban/info.cpp \
include/ban/ban.hpp \
include/info/info.hpp \
include/chat/chat.hpp | \
bin/mods/chat.dll \
bin/command_mods/info.dll \
bin/mcpp.dll \
bin/rleahy_lib.dll
	$(GPP) -shared -o $@ \
	bin/mods/chat.dll \
	bin/command_mods/info.dll \
	bin/mcpp.dll \
	bin/rleahy_lib.dll \
	bin/mods/ban.dll \
	obj/new_delete.o \
	src/ban/info.cpp
	
#	DATA PROVIDER INFORMATION

bin/info_mods/dp_info.dll: \
obj/new_delete.o \
src/info/dp.cpp \
include/info/info.hpp \
include/server.hpp \
include/chat/chat.hpp | \
bin/mods/chat.dll \
bin/mcpp.dll \
bin/rleahy_lib.dll \
bin/command_mods/info.dll
	$(GPP) -shared -o $@ \
	bin/mods/chat.dll \
	bin/command_mods/info.dll \
	bin/mcpp.dll \
	bin/rleahy_lib.dll \
	obj/new_delete.o \
	src/info/dp.cpp
	
#	SERVER OPERATORS INFORMATION

bin/info_mods/op_info.dll: \
obj/new_delete.o \
src/op/info.cpp \
include/info/info.hpp \
include/op/op.hpp \
include/chat/chat.hpp | \
bin/mods/chat.dll \
bin/mcpp.dll \
bin/mods/op.dll \
bin/rleahy_lib.dll \
bin/command_mods/info.dll
	$(GPP) -shared -o $@ \
	bin/mods/chat.dll \
	bin/command_mods/info.dll \
	bin/mcpp.dll \
	bin/rleahy_lib.dll \
	bin/mods/op.dll \
	obj/new_delete.o \
	src/op/info.cpp
	
	
#	SERVER FRONT-END

.PHONY: front_end
front_end: bin/server.exe

bin/server.exe: src/test_front_end/main.cpp src/test_front_end/test.cpp bin/mcpp.dll bin/rleahy_lib.dll obj/new_delete.o
	$(GPP) -o $@ src/test_front_end/main.cpp bin/mcpp.dll bin/rleahy_lib.dll obj/new_delete.o