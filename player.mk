mods: bin/mods/mcpp_player.dll


bin/mods/mcpp_player.dll: \
$(MOD_OBJ) \
obj/player/event_handlers.o \
obj/player/get_player.o \
obj/player/player.o \
obj/player/players.o \
obj/player/player_position.o \
obj/player/set_spawn.o \
obj/player/update_position.o | \
$(MOD_LIB) \
bin/mods/mcpp_entity_id.dll \
bin/mods/mcpp_world.dll
	$(GPP) -shared -o $@ $^ $(MOD_LIB) bin/mods/mcpp_entity_id.dll bin/mods/mcpp_world.dll