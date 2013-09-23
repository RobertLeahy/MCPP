mods: \
bin/mods/mcpp_info.so \
bin/mods/mcpp_info_ban.so \
bin/mods/mcpp_info_client.so \
bin/mods/mcpp_info_data_provider.so \
bin/mods/mcpp_info_mcpp.so \
bin/mods/mcpp_info_op.so \
bin/mods/mcpp_info_os.so \
bin/mods/mcpp_info_pool.so \
bin/mods/mcpp_info_world.so


#	INFORMATION PROVIDER


bin/mods/mcpp_info.so: \
$(MOD_OBJ) \
obj/info/main.o | \
$(MOD_LIB) \
bin/mods/mcpp_chat.so \
bin/mods/mcpp_command.so \
bin/mods/mcpp_op.so
	$(GPP) -shared -o $@ $^ $(MOD_LIB) bin/mods/mcpp_chat.so bin/mods/mcpp_command.so bin/mods/mcpp_op.so $(call LINK,$@)
	
	
INFO_LIB:=$(MOD_LIB) bin/mods/mcpp_info.so bin/mods/mcpp_chat.so


#	BANS


bin/mods/mcpp_info_ban.so: \
$(MOD_OBJ) \
obj/ban/info.o | \
$(INFO_LIB) \
bin/mods/mcpp_ban.so
	$(GPP) -shared -o $@ $^ $(INFO_LIB) bin/mods/mcpp_ban.so $(call LINK,$@)
	
	
#	CLIENTS


bin/mods/mcpp_info_client.so: \
$(MOD_OBJ) \
obj/info/clients.o | \
$(INFO_LIB)
	$(GPP) -shared -o $@ $^ $(INFO_LIB) $(call LINK,$@)
	
	
#	DATA PROVIDER


bin/mods/mcpp_info_data_provider.so: \
$(MOD_OBJ) \
obj/info/dp.o | \
$(INFO_LIB)
	$(GPP) -shared -o $@ $^ $(INFO_LIB) $(call LINK,$@)
	
	
#	MCPP


bin/mods/mcpp_info_mcpp.so: \
$(MOD_OBJ) \
obj/info/mcpp.o | \
$(INFO_LIB)
	$(GPP) -shared -o $@ $^ $(INFO_LIB) $(call LINK,$@)
	
	
#	SERVER OPERATORS


bin/mods/mcpp_info_op.so: \
$(MOD_OBJ) \
obj/op/info.o | \
$(INFO_LIB) \
bin/mods/mcpp_op.so
	$(GPP) -shared -o $@ $^ $(INFO_LIB) bin/mods/mcpp_op.so $(call LINK,$@)
	
	
#	OPERATING SYSTEM


bin/mods/mcpp_info_os.so: \
$(MOD_OBJ) \
obj/info/os.o | \
$(INFO_LIB)
	$(GPP) -shared -o $@ $^ $(INFO_LIB) $(call LINK,$@)
	

#	THREAD POOL


bin/mods/mcpp_info_pool.so: \
$(MOD_OBJ) \
obj/info/pool.o | \
$(INFO_LIB)
	$(GPP) -shared -o $@ $^ $(INFO_LIB) $(call LINK,$@)
	
	
#	WORLD


bin/mods/mcpp_info_world.so: \
$(MOD_OBJ) \
obj/world/info.o | \
$(INFO_LIB) \
bin/mods/mcpp_world.so
	$(GPP) -shared -o $@ $^ $(INFO_LIB) bin/mods/mcpp_world.so $(call LINK,$@)