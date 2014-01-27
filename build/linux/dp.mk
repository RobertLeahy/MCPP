all: data_providers


.PHONY: data_providers
data_providers: bin/data_provider.so bin/mysql.ini


bin/data_provider.so: bin/data_providers/mysql_data_provider.so
	cp bin/data_providers/mysql_data_provider.so bin/data_provider.so
	

bin/data_providers/mysql_data_provider.so: \
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
$(LIB) \
bin/data_providers
	$(GPP) -shared -o bin/data_providers/data_provider.so $^ $(LIB) -lmysqlclient $(call LINK,data_provider.so)
	cp bin/data_providers/data_provider.so $@
	
	
bin/mysql.ini:
	cp mysql.ini bin/mysql.ini