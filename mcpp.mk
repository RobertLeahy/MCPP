all: mcpp


MCPP_LIB:=$(LIB) \
bin/data_provider.dll \
bin/libcurl.dll \
bin/libeay32.dll \
bin/ssleay32.dll \
bin/zlib1.dll


.PHONY: mcpp
mcpp: bin/mcpp.dll


bin/mcpp.dll: \
$(OBJ) \
obj/aes_128_cfb_8.o \
obj/base_64.o \
obj/client.o \
obj/client_list.o \
obj/client_list_iterator.o \
obj/compression.o \
obj/concurrency_manager.o \
obj/format.o \
obj/hardware_concurrency.o \
obj/http_handler.o \
obj/http_request.o \
obj/ip_address_range.o \
obj/json.o \
obj/mod.o \
obj/mod_loader.o \
obj/multi_scope_guard.o \
obj/nbt.o \
obj/network/connection.o \
obj/network/send_handle.o \
obj/network/windows/accept_command.o \
obj/network/windows/accept_data.o \
obj/network/windows/completion_command.o \
obj/network/windows/connect_command.o \
obj/network/windows/connection.o \
obj/network/windows/connection_handler.o \
obj/network/windows/error.o \
obj/network/windows/initializer.o \
obj/network/windows/iocp.o \
obj/network/windows/listening_socket.o \
obj/network/windows/make_socket.o \
obj/network/windows/receive_command.o \
obj/network/windows/reference_manager.o \
obj/network/windows/send_command.o \
obj/network/windows/send_handle.o \
obj/noise.o \
obj/packet.o \
obj/packet_router.o \
obj/rsa_key.o \
obj/serializer.o \
obj/server.o \
obj/sha1.o \
obj/url.o \
obj/random.o \
obj/thread_pool.o \
obj/thread_pool_handle.o \
obj/yggdrasil.o | \
$(MCPP_LIB) \
bin
	$(GPP) -shared -o $@ $^ $(MCPP_LIB) -lws2_32