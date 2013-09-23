mods: \
bin/mods/mcpp_command.so \
bin/mods/mcpp_command_chat_log.so \
bin/mods/mcpp_command_kick.so \
bin/mods/mcpp_command_op.so \
bin/mods/mcpp_command_time.so \
bin/mods/mcpp_command_whisper.so \


#	COMMAND
	
	
bin/mods/mcpp_command.so: \
$(MOD_OBJ) \
obj/command/main.o | \
$(MOD_LIB) \
bin/mods/mcpp_chat.so
	$(GPP) -shared -o $@ $^ $(MOD_LIB) bin/mods/mcpp_chat.so $(call LINK,$@)
	
	
COMMAND_LIB:=$(MOD_LIB) bin/mods/mcpp_chat.so bin/mods/mcpp_command.so


#	SERVER LOG THROUGH CHAT


bin/mods/mcpp_command_chat_log.so: \
$(MOD_OBJ) \
obj/log/main.o | \
$(COMMAND_LIB) \
bin/mods/mcpp_op.so \
bin/data_provider.so
	$(GPP) -shared -o $@ $^ $(COMMAND_LIB) bin/mods/mcpp_op.so bin/data_provider.so $(call LINK,$@)
	
	
#	KICK


bin/mods/mcpp_command_kick.so: \
$(MOD_OBJ) \
obj/kick/main.o | \
$(COMMAND_LIB) \
bin/mods/mcpp_op.so
	$(GPP) -shared -o $@ $^ $(COMMAND_LIB) bin/mods/mcpp_op.so $(call LINK,$@)
	
	
#	OP/DEOP


bin/mods/mcpp_command_op.so: \
$(MOD_OBJ) \
obj/op/command.o | \
$(COMMAND_LIB) \
bin/mods/mcpp_op.so
	$(GPP) -shared -o $@ $^ $(COMMAND_LIB) bin/mods/mcpp_op.so $(call LINK,$@)
	
	
#	DISPLAY TIME


bin/mods/mcpp_command_time.so: \
$(MOD_OBJ) \
obj/time/display.o | \
$(COMMAND_LIB) \
bin/mods/mcpp_time.so
	$(GPP) -shared -o $@ $^ $(COMMAND_LIB) bin/mods/mcpp_time.so $(call LINK,$@)
	
	
#	WHISPERS
	
	
bin/mods/mcpp_command_whisper.so: \
$(MOD_OBJ) \
obj/whisper/main.o | \
$(COMMAND_LIB)
	$(GPP) -shared -o $@ $^ $(COMMAND_LIB) $(call LINK,$@)
	
	
