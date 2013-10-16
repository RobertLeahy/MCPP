SUFFIXES+=.mk


INC_CURL:=-I deps/libcurl7300/include
INC_OPENSSL:=-I deps/openssl101e/include
INC_ZLIB:=-I deps/zlib128
INC_MYSQL:=-I "C:/Program Files/MySQL/MySQL Server 5.6/include/"
OPTIMIZATION:=-O0 -g -fno-inline -fno-elide-constructors -DDEBUG -fno-omit-frame-pointer
#OPTIMIZATION=-O3
GPP:=gcc48\bin\g++.exe $(OPTS_SHARED) $(OPTIMIZATION)
#-static-libgcc -static-libstdc++
OPTS_SHARED:=-D_WIN32_WINNT=0x0600 -Wall -Wpedantic -Werror -fno-rtti -std=gnu++11 -I include $(INC_CURL) $(INC_OPENSSL) $(INC_ZLIB) $(INC_MYSQL)
MKDIR=@mkdir_nofail.bat $(subst /,\,$(dir $(1)))


#	DEFAULT

.PHONY: all
all:

.PHONY: clean
clean: cleandeps
	@rmdir_nofail.bat obj

.PHONY: cleanall
cleanall: clean
	@rmdir_nofail.bat bin
	
.PHONY: cleandeps
cleandeps:
	@rmdir_nofail.bat makefiles
	
	
#	LIBRARIES

bin/libmysql.dll: | bin
	mysql.bat
	
bin/rleahy_lib.dll include/rleahylib/*.hpp: | bin
	rleahy_lib.bat
	
bin/ssleay32.dll: | bin
	ssleay.bat
	
bin/libeay32.dll: | bin
	libeay.bat
	
bin/libcurl.dll: | bin
	curl.bat
	
bin/zlib1.dll: | bin
	zlib.bat
	
	
#	DIRECTORIES

bin:
	@mkdir_nofail.bat bin
	@mkdir_nofail.bat bin\mods
	
bin/data_providers: | bin
	@mkdir_nofail.bat bin\data_providers
	
	
NODEPS:=clean cleanall cleandeps
	
	
ifeq (0,$(words $(findstring $(MAKECMDGOALS),$(NODEPS))))

	-include $(subst .cpp,.mk,$(subst src,makefiles,$(subst \,/,$(subst $(shell echo %CD%)\,,$(shell dir /b /s src\*.cpp)))))

endif


#	COMMON LIBRARIES AND OBJECTS


LIB:=bin/rleahy_lib.dll
OBJ:=obj/new_delete.o
	
	
makefiles/%.mk:
	$(call MKDIR,$@)
	$(GPP) -MM -MT "$(patsubst makefiles/%.mk,obj/%.o,$@) $@" $(patsubst makefiles/%.mk,src/%.cpp,$@) -MF $@
	
	
obj/%.o:
	$(call MKDIR,$@)
	$(GPP) -c -o $@ $(patsubst obj/%.o,src/%.cpp,$@)
	
	
include dp.mk
include front_end.mk
include mcpp.mk
include mods.mk