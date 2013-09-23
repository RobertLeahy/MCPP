SUFFIXES+=.mk


OPTIMIZATION:=-O0 -g -fno-inline -fno-elide-constructors -DDEBUG -fsanitize=address -fno-omit-frame-pointer
#OPTIMIZATION=-O3 -march=native
OPTS_SHARED:=-static-libgcc -static-libstdc++ -Wall -Wpedantic -Werror -fno-rtti -std=gnu++11 -I include -Dlinux -fPIC
GPP:=g++ $(OPTS_SHARED) $(OPTIMIZATION)
comma:=,
LINK=-Wl,-rpath,'$$ORIGIN' -Wl,-rpath-link,bin -Wl,-rpath-link,bin/mods $(if $(1),-Wl$(comma)-soname$(comma)$(notdir $(1)))


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
	./mkdir_nofail.sh bin

bin/mods:
	./mkdir_nofail.sh bin/mods
	
bin/data_providers: | bin
	./mkdir_nofail.sh bin/data_providers
	
	
NODEPS:=clean cleanall cleandeps
	
	
ifeq (0,$(words $(findstring $(MAKECMDGOALS),$(NODEPS))))

	-include $(patsubst %.cpp,%.mk,$(shell find src/ -name "*.cpp"))

endif


#	COMMON LIBRARIES AND OBJECTS


LIB:=bin/rleahylib.so
OBJ:=obj/new_delete.o
	
	
makefiles/%.mk:
	./mkdir_nofail.sh $(dir $@)
	$(GPP) -MM -MT "$(patsubst makefiles/%.mk,obj/%.o,$@) $@" $(patsubst makefiles/%.mk,src/%.cpp,$@) -MF $@
	
	
obj/%.o:
	./mkdir_nofail.sh $(dir $@)
	$(GPP) -c -o $@ $(patsubst obj/%.o,src/%.cpp,$@)
	
	
include dp.mk
include front_end.mk
include mcpp.mk
include mods.mk