all: data_providers


.PHONY: data_providers
data_providers: bin/data_provider.dll bin/mysql.ini


bin/data_provider.dll: bin/data_providers/mysql_data_provider.dll | bin
	cmd /c "copy bin\data_providers\mysql_data_provider.dll bin\data_provider.dll"
	

bin/data_providers/mysql_data_provider.dll: \
$(OBJ) \
obj/data_provider.o \
obj/mysql_data_provider/blob.o \
obj/mysql_data_provider/connection.o \
obj/mysql_data_provider/data_provider.o \
obj/mysql_data_provider/factory.o \
obj/mysql_data_provider/get_buffer.o \
obj/mysql_data_provider/prepared_statement.o | \
$(LIB) \
bin/libmysql.dll \
bin/data_providers
	$(GPP) -shared -o bin/data_providers/data_provider.dll $^ $(LIB) bin/libmysql.dll
	cmd /c "move bin\data_providers\data_provider.dll $@"
	
	
bin/mysql.ini: | bin
	cmd /c "copy mysql.ini bin\mysql.ini"