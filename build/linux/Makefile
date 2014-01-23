SUFFIXES+=.mk


OPTIMIZATION:=-O0 -g -fno-inline -fno-elide-constructors -DDEBUG -fsanitize=address -fno-omit-frame-pointer
#OPTIMIZATION=-O3
#-static-libgcc -static-libstdc++
OPTS_SHARED:=-Wall -Wpedantic -Werror -fno-rtti -std=gnu++11 -I include -Dlinux -fPIC
GPP:=g++-4.8 $(OPTS_SHARED) $(OPTIMIZATION)
comma:=,
LINK=-Wl,-rpath,'$$ORIGIN' -Wl,-rpath-link,bin -Wl,-rpath-link,bin/mods $(if $(1),-Wl$(comma)-soname$(comma)$(notdir $(1)))
MKDIR=mkdir -p $(dir $(1))


#	DEFAULT

.PHONY: all
all:

.PHONY: clean
clean: cleandeps
	-@rm -r -f obj

.PHONY: cleanall
cleanall: clean
	-@rm -r -f bin
	
.PHONY: cleandeps
cleandeps:
	@rm -r -f makefiles
	
	
#	LIBRARIES



	
	
#	DIRECTORIES

bin:
	-@mkdir bin

bin/mods:
	-@mkdir -p bin/mods
	
bin/data_providers:
	-@mkdir -p bin/data_providers
	
	
NODEPS:=clean cleanall cleandeps
	
	
ifeq (0,$(words $(findstring $(MAKECMDGOALS),$(NODEPS))))

	-include $(patsubst src/%.cpp,makefiles/%.mk,$(shell find src/ -name "*.cpp"))

endif


#	COMMON LIBRARIES AND OBJECTS


LIB:=bin/rleahylib.so
OBJ:=obj/new_delete.o


obj/%.o:
	$(call MKDIR,$(patsubst obj/%.o,makefiles/%.mk,$@))
	$(GPP) -MM -MT "$@" $(patsubst obj/%.o,src/%.cpp,$@) -MF $(patsubst obj/%.o,makefiles/%.mk,$@)
	$(call MKDIR,$@)
	$(GPP) -c -o $@ $(patsubst obj/%.o,src/%.cpp,$@)
	
	
include dp.mk
include front_end.mk
include mcpp.mk
include mods.mk