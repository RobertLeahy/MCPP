all: front_end


.PHONY: front_end
front_end: bin/mcpp.exe bin/mcpp_cli.dll


FE_LIB:=$(LIB) bin/mcpp.dll


bin/mcpp_cli.dll: \
$(OBJ) \
obj/cli/args.o \
obj/cli/cli.o \
obj/cli/cli_provider.o \
obj/cli/console.o \
obj/cli/console_screen_buffer.o \
obj/cli/date_time.o \
obj/cli/error.o | \
$(LIB) \
bin/data_provider.dll
	$(GPP) -shared -o $@ $^ $(LIB) bin/data_provider.dll
	
	
bin/mcpp.exe: \
$(OBJ) \
obj/front_ends/cli/main.o | \
$(FE_LIB) \
bin/mcpp_cli.dll
	$(GPP) -o $@ $^ $(FE_LIB) bin/mcpp_cli.dll