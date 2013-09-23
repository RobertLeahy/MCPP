all: mcpp


.PHONY: mcpp
mcpp: bin/mcpp.so


bin/mcpp.so: \
$(OBJ) \
obj/aes_128_cfb_8.o \
obj/client.o \
obj/client_list.o \
obj/compression.o \
obj/concurrency_manager.o \
obj/hardware_concurrency.o \
obj/http_handler.o \
obj/http_request.o \
obj/metadata.o \
obj/mod.o \
obj/mod_loader.o \
obj/multi_scope_guard.o \
obj/nbt.o \
obj/network.o \
obj/noise.o \
obj/packet.o \
obj/packet_factory.o \
obj/packet_router.o \
obj/rsa_key.o \
obj/server.o \
obj/sha1.o \
obj/url.o \
obj/random.o \
obj/thread_pool.o \
obj/thread_pool_handle.o | \
bin/data_provider.so
	$(GPP) -shared -o $@ $^ $(LIB) bin/data_provider.so -lz -lcrypto -lcurl $(call LINK,$@)