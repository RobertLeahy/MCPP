all: front_end


.PHONY: front_end
front_end: bin/mcpp.exe


FE_LIB:=$(LIB) bin/mcpp.dll
	
	
bin/mcpp.exe: \
$(OBJ) \
obj/interactive_front_end/args.o \
obj/interactive_front_end/date_time.o \
obj/interactive_front_end/console.o \
obj/interactive_front_end/console_screen_buffer.o \
obj/interactive_front_end/main.o | \
$(FE_LIB) \
bin/data_provider.dll
	$(GPP) -o $@ $^ $(FE_LIB) bin/data_provider.dll