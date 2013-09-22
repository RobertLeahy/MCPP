mods: \
bin/mods/mcpp_info.dll \
bin/mods/mcpp_info_ban.dll \
bin/mods/mcpp_info_client.dll \
bin/mods/mcpp_info_data_provider.dll \
bin/mods/mcpp_info_mcpp.dll \
bin/mods/mcpp_info_op.dll \
bin/mods/mcpp_info_os.dll \
bin/mods/mcpp_info_pool.dll \
bin/mods/mcpp_info_world.dll


#	INFORMATION PROVIDER


bin/mods/mcpp_info.dll: \
$(MOD_OBJ) \
obj/info/main.o | \
$(MOD_LIB) \
bin/mods/mcpp_chat.dll \
bin/mods/mcpp_command.dll \
bin/mods/mcpp_op.dll
	$(GPP) -shared -o $@ $^ $(MOD_LIB) bin/mods/mcpp_chat.dll bin/mods/mcpp_command.dll bin/mods/mcpp_op.dll
	
	
INFO_LIB:=$(MOD_LIB) bin/mods/mcpp_info.dll bin/mods/mcpp_chat.dll


#	BANS


bin/mods/mcpp_info_ban.dll: \
$(MOD_OBJ) \
obj/ban/info.o | \
$(INFO_LIB) \
bin/mods/mcpp_ban.dll
	$(GPP) -shared -o $@ $^ $(INFO_LIB) bin/mods/mcpp_ban.dll
	
	
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
	
	
#	MCPP


bin/mods/mcpp_info_mcpp.dll: \
$(MOD_OBJ) \
obj/info/mcpp.o | \
$(INFO_LIB)
	$(GPP) -shared -o $@ $^ $(INFO_LIB)
	
	
#	SERVER OPERATORS


bin/mods/mcpp_info_op.dll: \
$(MOD_OBJ) \
obj/op/info.o | \
$(INFO_LIB) \
bin/mods/mcpp_op.dll
	$(GPP) -shared -o $@ $^ $(INFO_LIB) bin/mods/mcpp_op.dll
	
	
#	OPERATING SYSTEM


bin/mods/mcpp_info_os.dll: \
$(MOD_OBJ) \
obj/info/os.o | \
$(INFO_LIB)
	$(GPP) -shared -o $@ $^ $(INFO_LIB)
	

#	THREAD POOL


bin/mods/mcpp_info_pool.dll: \
$(MOD_OBJ) \
obj/info/pool.o | \
$(INFO_LIB)
	$(GPP) -shared -o $@ $^ $(INFO_LIB)
	
	
#	WORLD


bin/mods/mcpp_info_world.dll: \
$(MOD_OBJ) \
obj/world/info.o | \
$(INFO_LIB) \
bin/mods/mcpp_world.dll
	$(GPP) -shared -o $@ $^ $(INFO_LIB) bin/mods/mcpp_world.dll