mods: info


.PHONY: info
info: \
bin/mods/mcpp_info.dll \
bin/mods/mcpp_info_ban.dll \
bin/mods/mcpp_info_blacklist.dll \
bin/mods/mcpp_info_brand.dll \
bin/mods/mcpp_info_client.dll \
bin/mods/mcpp_info_data_provider.dll \
bin/mods/mcpp_info_handler.dll \
bin/mods/mcpp_info_mcpp.dll \
bin/mods/mcpp_info_mods.dll \
bin/mods/mcpp_info_os.dll \
bin/mods/mcpp_info_permissions.dll \
bin/mods/mcpp_info_pool.dll \
bin/mods/mcpp_info_save.dll \
bin/mods/mcpp_info_time.dll \
bin/mods/mcpp_info_whitelist.dll \
bin/mods/mcpp_info_world.dll


#	INFORMATION PROVIDER


bin/mods/mcpp_info.dll: \
$(MOD_OBJ) \
obj/info/main.o | \
$(MOD_LIB) \
bin/mods/mcpp_chat.dll \
bin/mods/mcpp_command.dll \
bin/mods/mcpp_permissions.dll
	$(GPP) -shared -o $@ $^ $(MOD_LIB) bin/mods/mcpp_chat.dll bin/mods/mcpp_command.dll bin/mods/mcpp_permissions.dll
	
	
INFO_LIB:=$(MOD_LIB) bin/mods/mcpp_info.dll bin/mods/mcpp_chat.dll


#	BANS


bin/mods/mcpp_info_ban.dll: \
$(MOD_OBJ) \
obj/ban/info.o | \
$(INFO_LIB) \
bin/mods/mcpp_ban.dll
	$(GPP) -shared -o $@ $^ $(INFO_LIB) bin/mods/mcpp_ban.dll
	
	
#	BLACKLIST


bin/mods/mcpp_info_blacklist.dll: \
$(MOD_OBJ) \
obj/blacklist/info.o | \
$(INFO_LIB) \
bin/mods/mcpp_blacklist.dll
	$(GPP) -shared -o $@ $^ $(INFO_LIB) bin/mods/mcpp_blacklist.dll
	
	
#	BRANDS


bin/mods/mcpp_info_brand.dll: \
$(MOD_OBJ) \
obj/brand/info.o | \
$(INFO_LIB) \
bin/mods/mcpp_brand.dll
	$(GPP) -shared -o $@ $^ $(INFO_LIB) bin/mods/mcpp_brand.dll
	
	
#	CLIENTS


bin/mods/mcpp_info_client.dll: \
$(MOD_OBJ) \
obj/info/clients.o | \
$(INFO_LIB)
	$(GPP) -shared -o $@ $^ $(INFO_LIB)
	
	
#	DATA PROVIDER


bin/mods/mcpp_info_data_provider.dll: \
$(MOD_OBJ) \
obj/info/dp.o | \
$(INFO_LIB)
	$(GPP) -shared -o $@ $^ $(INFO_LIB)
	
	
#	HANDLER


bin/mods/mcpp_info_handler.dll: \
$(MOD_OBJ) \
obj/info/handler.o | \
$(INFO_LIB)
	$(GPP) -shared -o $@ $^ $(INFO_LIB)
	
	
#	MCPP


bin/mods/mcpp_info_mcpp.dll: \
$(MOD_OBJ) \
obj/info/mcpp.o | \
$(INFO_LIB)
	$(GPP) -shared -o $@ $^ $(INFO_LIB)
	
	
#	MODULES/MODULE LOADER


bin/mods/mcpp_info_mods.dll: \
$(MOD_OBJ) \
obj/info/mods.o | \
$(INFO_LIB)
	$(GPP) -shared -o $@ $^ $(INFO_LIB)
	
	
#	OPERATING SYSTEM


bin/mods/mcpp_info_os.dll: \
$(MOD_OBJ) \
obj/info/os.o | \
$(INFO_LIB)
	$(GPP) -shared -o $@ $^ $(INFO_LIB)
	
	
#	PERMISSIONS


bin/mods/mcpp_info_permissions.dll: \
$(MOD_OBJ) \
obj/permissions/info.o | \
$(INFO_LIB) \
bin/mods/mcpp_permissions.dll
	$(GPP) -shared -o $@ $^ $(INFO_LIB) bin/mods/mcpp_permissions.dll
	

#	THREAD POOL


bin/mods/mcpp_info_pool.dll: \
$(MOD_OBJ) \
obj/info/pool.o | \
$(INFO_LIB)
	$(GPP) -shared -o $@ $^ $(INFO_LIB)
	
	
#	SAVE SYSTEM


bin/mods/mcpp_info_save.dll: \
$(MOD_OBJ) \
obj/save/info.o | \
$(INFO_LIB) \
bin/mods/mcpp_save.dll
	$(GPP) -shared -o $@ $^ $(INFO_LIB) bin/mods/mcpp_save.dll
	
	
#	TIME/TICKS


bin/mods/mcpp_info_time.dll: \
$(MOD_OBJ) \
obj/time/info.o | \
$(INFO_LIB) \
bin/mods/mcpp_time.dll
	$(GPP) -shared -o $@ $^ $(INFO_LIB) bin/mods/mcpp_time.dll
	
	
#	WHITELIST


bin/mods/mcpp_info_whitelist.dll: \
$(MOD_OBJ) \
obj/whitelist/info.o | \
$(INFO_LIB) \
bin/mods/mcpp_whitelist.dll
	$(GPP) -shared -o $@ $^ $(INFO_LIB) bin/mods/mcpp_whitelist.dll
	
	
#	WORLD


bin/mods/mcpp_info_world.dll: \
$(MOD_OBJ) \
obj/world/info.o | \
$(INFO_LIB) \
bin/mods/mcpp_world.dll
	$(GPP) -shared -o $@ $^ $(INFO_LIB) bin/mods/mcpp_world.dll