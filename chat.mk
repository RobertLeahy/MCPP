mods: \
bin/mods/mcpp_chat.dll \
bin/mods/mcpp_chat_global.dll \
bin/mods/mcpp_chat_login_logout_broadcasts.dll


#	CHAT


bin/mods/mcpp_chat.dll: \
$(MOD_OBJ) \
obj/chat/chat_misc.o \
obj/chat/chat_parser.o \
obj/chat/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB)
	
	
CHAT_LIB:=$(MOD_LIB) bin/mods/mcpp_chat.dll
	
	
#	GLOBAL CHAT


bin/mods/mcpp_chat_global.dll: \
$(MOD_OBJ) \
obj/basic_chat/main.o | \
$(CHAT_LIB)
	$(GPP) -shared -o $@ $^ $(CHAT_LIB)
	
	
#	LOGIN/LOGOUT BROADCASTS


bin/mods/mcpp_chat_login_logout_broadcasts.dll: \
$(MOD_OBJ) \
obj/chat_login/main.o | \
$(CHAT_LIB)
	$(GPP) -shared -o $@ $^ $(CHAT_LIB)
