all: data_providers


DP_LIB:=$(LIB) \
bin/libmysql.dll


.PHONY: data_providers
data_providers: bin/data_provider.dll bin/mysql.ini


bin/data_provider.dll: bin/data_providers/mysql_data_provider.dll | bin
	cmd /c "copy bin\data_providers\mysql_data_provider.dll bin\data_provider.dll"
	

bin/data_providers/mysql_data_provider.dll: \
$(OBJ) \
obj/mysql_data_provider/mysql_connection.o \
obj/mysql_data_provider/mysql_data_provider.o \
obj/mysql_data_provider/log.o \
obj/mysql_data_provider/factory.o \
obj/mysql_data_provider/binary.o \
obj/mysql_data_provider/key_value.o \
obj/mysql_data_provider/settings.o \
obj/mysql_data_provider/info.o \
obj/data_provider.o | \
$(DP_LIB) \
bin/data_providers
	$(GPP) -shared -o bin/data_providers/data_provider.dll $^ $(DP_LIB)
	cmd /c "move bin\data_providers\data_provider.dll $@"
	
	
bin/mysql.ini: | bin
	cmd /c "copy mysql.ini bin\mysql.ini"