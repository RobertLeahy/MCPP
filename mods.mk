all: mods


.PHONY: mods
mods:


MOD_OBJ:=$(OBJ)
MOD_LIB:=$(LIB) bin/mcpp.dll


include chat.mk
include command.mk
include info.mk
include player.mk
include world.mk


mods: \
bin/mods/mcpp_auth.dll \
bin/mods/mcpp_ban.dll \
bin/mods/mcpp_blacklist.dll \
bin/mods/mcpp_brand.dll \
bin/mods/mcpp_entity_id.dll \
bin/mods/mcpp_favicon.dll \
bin/mods/mcpp_handshake.dll \
bin/mods/mcpp_keep_alive.dll \
bin/mods/mcpp_save.dll \
bin/mods/mcpp_permissions.dll \
bin/mods/mcpp_ping.dll \
bin/mods/mcpp_player_list.dll \
bin/mods/mcpp_plugin_message.dll \
bin/mods/mcpp_time.dll \
bin/mods/mcpp_whitelist.dll


#	AUTHENTICATION


bin/mods/mcpp_auth.dll: \
$(MOD_OBJ) \
obj/auth/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB)
	
	
#	VANILLA AUTHENTICATION


bin/mods/mcpp_auth_vanilla.dll: \
$(MOD_OBJ) \
obj/auth/vanilla.o | \
$(MOD_LIB) \
bin/mods/mcpp_auth.dll
	$(GPP) -shared -o $@ $^ $(MOD_LIB) bin/mods/mcpp_auth.dll
	
	
#	BANS


bin/mods/mcpp_ban.dll: \
$(MOD_OBJ) \
obj/ban/main.o | \
$(MOD_LIB) \
bin/mods/mcpp_save.dll
	$(GPP) -shared -o $@ $^ $(MOD_LIB) bin/mods/mcpp_save.dll
	
	
#	BLACKLIST


bin/mods/mcpp_blacklist.dll: \
$(MOD_OBJ) \
obj/blacklist/main.o | \
$(MOD_LIB) \
bin/mods/mcpp_save.dll
	$(GPP) -shared -o $@ $^ $(MOD_LIB) bin/mods/mcpp_save.dll
	
	
#	BRAND IDENTIFICATION


bin/mods/mcpp_brand.dll: \
$(MOD_OBJ) \
obj/brand/main.o | \
$(MOD_LIB) \
bin/mods/mcpp_plugin_message.dll
	$(GPP) -shared -o $@ $^ $(MOD_LIB) bin/mods/mcpp_plugin_message.dll
	
	
#	ENTITY ID GENERATION


bin/mods/mcpp_entity_id.dll: \
$(MOD_OBJ) \
obj/entity_id/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB)
	
	
bin/mods/mcpp_favicon.dll: \
$(MOD_OBJ) \
obj/favicon/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB)
	
	
#	HANDSHAKE


bin/mods/mcpp_handshake.dll: \
$(MOD_OBJ) \
obj/handshake/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB)
	
	
#	KEEP ALIVE


bin/mods/mcpp_keep_alive.dll: \
$(MOD_OBJ) \
obj/keep_alive/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB)
	
	
#	SAVE LOOP


bin/mods/mcpp_save.dll: \
$(MOD_OBJ) \
obj/save/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB)
	
	
#	PERMISSIONS


bin/mods/mcpp_permissions.dll: \
$(MOD_OBJ) \
obj/permissions/permissions.o \
obj/permissions/permissions_handle.o \
obj/permissions/permissions_table_entry.o | \
$(MOD_LIB) \
bin/mods/mcpp_save.dll
	$(GPP) -shared -o $@ $^ $(MOD_LIB) bin/mods/mcpp_save.dll
	
	
#	PING


bin/mods/mcpp_ping.dll: \
$(MOD_OBJ) \
obj/ping/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB)
	
	
#	PLAYER LIST SUPPORT


bin/mods/mcpp_player_list.dll: \
$(MOD_OBJ) \
obj/player_list/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB)
	
	
#	PLUGIN MESSAGE SUPPORT


bin/mods/mcpp_plugin_message.dll: \
$(MOD_OBJ) \
obj/plugin_message/main.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB)
	
	
#	TIME


bin/mods/mcpp_time.dll: \
$(MOD_OBJ) \
obj/time/main.o | \
$(MOD_LIB) \
bin/mods/mcpp_save.dll
	$(GPP) -shared -o $@ $^ $(MOD_LIB) bin/mods/mcpp_save.dll
	
	
#	WHITELIST


bin/mods/mcpp_whitelist.dll: \
$(MOD_OBJ) \
obj/whitelist/main.o | \
$(MOD_LIB) \
bin/mods/mcpp_save.dll
	$(GPP) -shared -o $@ $^ $(MOD_LIB) bin/mods/mcpp_save.dll