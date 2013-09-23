mods: bin/mods/mcpp_player.so


bin/mods/mcpp_player.so: \
$(MOD_OBJ) \
src/player/event_handlers.cpp \
src/player/get_player.cpp \
src/player/player.cpp \
src/player/players.cpp \
src/player/player_position.cpp \
src/player/set_spawn.cpp \
src/player/update_position.cpp | \
$(MOD_LIB) \
bin/mods/mcpp_entity_id.so \
bin/mods/mcpp_world.so
	$(GPP) -shared -o $@ $^ $(MOD_LIB) bin/mods/mcpp_entity_id.so bin/mods/mcpp_world.so $(call LINK,$@)