all: mcpp


.PHONY: mcpp
mcpp: bin/mcpp.so


bin/mcpp.so: \
$(OBJ) \
obj/aes_128_cfb_8.o \
obj/base_64.o \
obj/client.o \
obj/client_list.o \
obj/client_list_iterator.o \
obj/compression.o \
obj/concurrency_manager.o \
obj/dns_handler.o \
obj/format.o \
obj/hardware_concurrency.o \
obj/http_handler.o \
obj/ip_address_range.o \
obj/json.o \
obj/mod.o \
obj/mod_loader.o \
obj/multi_scope_guard.o \
obj/nbt.o \
obj/network/connection.o \
obj/network/linux/notification.o \
obj/network/linux/notifier.o \
obj/network/posix/channel_base.o \
obj/network/posix/command.o \
obj/network/posix/connection.o \
obj/network/posix/connection_handler.o \
obj/network/posix/error.o \
obj/network/posix/follow_up.o \
obj/network/posix/listening_socket.o \
obj/network/posix/misc.o \
obj/network/posix/send_buffer.o \
obj/network/posix/strerror_r.o \
obj/network/posix/worker_channel.o \
obj/network/posix/worker.o \
obj/noise.o \
obj/packet.o \
obj/packet_router.o \
obj/recursive_mutex.o \
obj/rsa_key.o \
obj/serializer.o \
obj/server.o \
obj/sha1.o \
obj/socketpair.o \
obj/url.o \
obj/random.o \
obj/thread_pool.o \
obj/yggdrasil.o | \
bin \
bin/data_provider.so
	$(GPP) -shared -o $@ $^ $(LIB) bin/data_provider.so -lz -lcrypto -lcurl -lcares -lpthread $(call LINK,$@)