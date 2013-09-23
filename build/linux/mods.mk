all: mods


.PHONY: mods
mods:


MOD_OBJ:=$(OBJ)
MOD_LIB:=$(LIB) bin/mcpp.so


include chat.mk
include command.mk
include info.mk
include player.mk
include world.mk


mods: \
bin/mods/mcpp_auth.so \
bin/mods/mcpp_ban.so \
bin/mods/mcpp_disconnect.so \
bin/mods/mcpp_entity_id.so \
bin/mods/mcpp_keep_alive.so \
bin/mods/mcpp_op.so \
bin/mods/mcpp_ping.so \
bin/mods/mcpp_player_list.so \
bin/mods/mcpp_time.so


#	MINECRAFT.NET AUTHENTICATION


bin/mods/mcpp_auth.so: \
$(MOD_OBJ) \
obj/auth/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB) $(call LINK,$@)
	
	
#	BANS


bin/mods/mcpp_ban.so: \
$(MOD_OBJ) \
obj/ban/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB) $(call LINK,$@)
	
	
#	DISCONNECT PACKET HANDLING


bin/mods/mcpp_disconnect.so: \
$(MOD_OBJ) \
obj/disconnect/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB) $(call LINK,$@)
	
	
#	ENTITY ID GENERATION


bin/mods/mcpp_entity_id.so: \
$(MOD_OBJ) \
obj/entity_id/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB) $(call LINK,$@)
	
	
#	KEEP ALIVE


bin/mods/mcpp_keep_alive.so: \
$(MOD_OBJ) \
obj/keep_alive/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB) $(call LINK,$@)


#	SERVER OPERATORS


bin/mods/mcpp_op.so: \
$(MOD_OBJ) \
obj/op/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB) $(call LINK,$@)
	
	
#	PING


bin/mods/mcpp_ping.so: \
$(MOD_OBJ) \
obj/ping/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB) $(call LINK,$@)
	
	
#	PLAYER LIST SUPPORT


bin/mods/mcpp_player_list.so: \
$(MOD_OBJ) \
obj/player_list/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB) $(call LINK,$@)
	
	
#	TIME


bin/mods/mcpp_time.so: \
$(MOD_OBJ) \
obj/time/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB) $(call LINK,$@)