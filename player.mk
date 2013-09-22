mods: bin/mods/mcpp_player.dll


bin/mods/mcpp_player.dll: \
$(MOD_OBJ) \
src/player/event_handlers.cpp \
src/player/get_player.cpp \
src/player/player.cpp \
src/player/players.cpp \
src/player/player_position.cpp \
src/player/set_spawn.cpp \
src/player/update_position.cpp | \
$(MOD_LIB) \
bin/mods/mcpp_entity_id.dll \
bin/mods/mcpp_world.dll
	$(GPP) -shared -o $@ $^ $(MOD_LIB) bin/mods/mcpp_entity_id.dll bin/mods/mcpp_world.dll