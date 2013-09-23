all: front_end


.PHONY: front_end
front_end: bin/server.exe


FE_LIB:=$(LIB) bin/mcpp.so


bin/server.exe: \
$(OBJ) \
obj/test_front_end/main.o | \
$(FE_LIB)
	$(GPP) -o $@ $^ $(FE_LIB) $(call LINK)