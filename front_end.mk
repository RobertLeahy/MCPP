all: front_end


.PHONY: front_end
front_end: bin/server.exe


FE_LIB:=$(LIB) bin/mcpp.dll


bin/server.exe: \
$(OBJ) \
obj/test_front_end/main.o | \
$(FE_LIB)
	$(GPP) -shared -o $@ $^ $(FE_LIB)