mods: \
bin/mods/mcpp_world.dll \
bin/mods/mcpp_world_default_generator.dll


#	WORLD


bin/mods/mcpp_world.dll: \
$(MOD_OBJ) \
obj/world/add_client.o \
obj/world/block_id.o \
obj/world/column_container.o \
obj/world/column_id.o \
obj/world/events.o \
obj/world/generator.o \
obj/world/generators.o \
obj/world/get_block.o \
obj/world/get_column.o \
obj/world/get_info.o \
obj/world/has_skylight.o \
obj/world/interest.o \
obj/world/key.o \
obj/world/load.o \
obj/world/maintenance.o \
obj/world/save.o \
obj/world/set_block.o \
obj/world/set_seed.o \
obj/world/populator.o \
obj/world/populators.o \
obj/world/process.o \
obj/world/transaction.o \
obj/world/world.o \
obj/world/world_lock.o | \
$(MOD_LIB)
	$(GPP) -shared -o $@ $^ $(MOD_LIB)
	
	
#	DEFAULT GENERATOR


bin/mods/mcpp_world_default_generator.dll: \
$(MOD_OBJ) \
obj/generators/default/main.o | \
$(MOD_LIB) \
bin/mods/mcpp_world.dll
	$(GPP) -shared -o $@ $^ $(MOD_LIB) bin/mods/mcpp_world.dll