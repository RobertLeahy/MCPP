mods: \
bin/mods/mcpp_command.dll \
bin/mods/mcpp_command_blacklist.dll \
bin/mods/mcpp_command_kick.dll \
bin/mods/mcpp_command_permissions.dll \
bin/mods/mcpp_command_save.dll \
bin/mods/mcpp_command_settings.dll \
bin/mods/mcpp_command_shutdown.dll \
bin/mods/mcpp_command_time.dll \
bin/mods/mcpp_command_verbose.dll \
bin/mods/mcpp_command_whisper.dll \


#	COMMAND
	
	
bin/mods/mcpp_command.dll: \
$(MOD_OBJ) \
obj/command/command.o \
obj/command/commands.o | \
$(MOD_LIB) \
bin/mods/mcpp_chat.dll
	$(GPP) -shared -o $@ $^ $(MOD_LIB) bin/mods/mcpp_chat.dll
	
	
COMMAND_LIB:=$(MOD_LIB) bin/mods/mcpp_chat.dll bin/mods/mcpp_command.dll


#	BLACKLIST


bin/mods/mcpp_command_blacklist.dll: \
$(MOD_OBJ) \
obj/blacklist/command.o | \
$(COMMAND_LIB) \
bin/mods/mcpp_permissions.dll \
bin/mods/mcpp_blacklist.dll
	$(GPP) -shared -o $@ $^ $(COMMAND_LIB) bin/mods/mcpp_permissions.dll bin/mods/mcpp_blacklist.dll
	
	
#	KICK


bin/mods/mcpp_command_kick.dll: \
$(MOD_OBJ) \
obj/kick/main.o | \
$(COMMAND_LIB) \
bin/mods/mcpp_op.dll
	$(GPP) -shared -o $@ $^ $(COMMAND_LIB) bin/mods/mcpp_op.dll
	
	
#	PERMISSIONS


bin/mods/mcpp_command_permissions.dll: \
$(MOD_OBJ) \
obj/permissions/command.o | \
$(COMMAND_LIB) \
bin/mods/mcpp_permissions.dll
	$(GPP) -shared -o $@ $^ $(COMMAND_LIB) bin/mods/mcpp_permissions.dll
	
	
#	SAVE


bin/mods/mcpp_command_save.dll: \
$(MOD_OBJ) \
obj/save/command.o | \
$(COMMAND_LIB) \
bin/mods/mcpp_permissions.dll \
bin/mods/mcpp_save.dll
	$(GPP) -shared -o $@ $^ $(COMMAND_LIB) bin/mods/mcpp_permissions.dll bin/mods/mcpp_save.dll
	
	
#	CONFIGURATION


bin/mods/mcpp_command_settings.dll: \
$(MOD_OBJ) \
obj/settings/main.o | \
$(COMMAND_LIB) \
bin/mods/mcpp_permissions.dll
	$(GPP) -shared -o $@ $^ $(COMMAND_LIB) bin/mods/mcpp_permissions.dll
	
	
#	SHUTDOWN/RESTART SERVER


bin/mods/mcpp_command_shutdown.dll: \
$(MOD_OBJ) \
obj/shutdown/main.o | \
$(COMMAND_LIB) \
bin/mods/mcpp_permissions.dll
	$(GPP) -shared -o $@ $^ $(COMMAND_LIB) bin/mods/mcpp_permissions.dll
	
	
#	DISPLAY TIME


bin/mods/mcpp_command_time.dll: \
$(MOD_OBJ) \
obj/time/display.o | \
$(COMMAND_LIB) \
bin/mods/mcpp_time.dll
	$(GPP) -shared -o $@ $^ $(COMMAND_LIB) bin/mods/mcpp_time.dll
	
	
#	VERBOSE


bin/mods/mcpp_command_verbose.dll: \
$(MOD_OBJ) \
obj/verbose/main.o | \
$(COMMAND_LIB) \
bin/mods/mcpp_op.dll
	$(GPP) -shared -o $@ $^ $(COMMAND_LIB) bin/mods/mcpp_op.dll
	
	
#	WHISPERS
	
	
bin/mods/mcpp_command_whisper.dll: \
$(MOD_OBJ) \
obj/whisper/main.o | \
$(COMMAND_LIB)
	$(GPP) -shared -o $@ $^ $(COMMAND_LIB)
	
	
