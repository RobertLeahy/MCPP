mods: \
bin/mods/mcpp_chat.so \
bin/mods/mcpp_chat_global.so \
bin/mods/mcpp_chat_login_logout_broadcasts.so


#	CHAT


bin/mods/mcpp_chat.so: \
$(MOD_OBJ) \
obj/chat/chat_misc.o \
obj/chat/chat_parser.o \
obj/chat/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB) $(call LINK,$@)
	
	
CHAT_LIB:=$(MOD_LIB) bin/mods/mcpp_chat.so
	
	
#	GLOBAL CHAT


bin/mods/mcpp_chat_global.so: \
$(MOD_OBJ) \
obj/basic_chat/main.o | \
$(CHAT_LIB)
	$(GPP) -shared -o $@ $^ $(CHAT_LIB) $(call LINK,$@)
	
	
#	LOGIN/LOGOUT BROADCASTS


bin/mods/mcpp_chat_login_logout_broadcasts.so: \
$(MOD_OBJ) \
obj/chat_login/main.o | \
$(CHAT_LIB)
	$(GPP) -shared -o $@ $^ $(CHAT_LIB) $(call LINK,$@)
