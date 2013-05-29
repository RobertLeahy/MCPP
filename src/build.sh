#!/bin/bash

path_to_gcc="/opt/gcc-4.8.0/bin/g++"
path_to_gdb="/opt/gcc-4.8.0/bin/gdb"
server_all="-L ../bin/ -I ../include/ main.cpp server.cpp messages.cpp handler.cpp connection.cpp format.cpp -lrleahy"

echo

echo "====GETTING LATEST VERSION OF RLEAHYLIB===="

cp ../../RLeahyLib/release/bin/*.so ../bin
cp ../../RLeahyLib/release/include/*.hpp ../include/rleahylib

echo
echo

if [ "$1" = "-release" ]
then

	common_params="-O3 -Wall -Wpedantic -fno-rtti -std=gnu++11 -static-libgcc -static-libstdc++"
	
	echo "====BUILDING RELEASE===="

else

	common_params="-O0 -g -Wall -Wpedantic -fno-inline -fno-elide-constructors -fno-rtti -static-libgcc -static-libstdc++ -DDEBUG -std=gnu++11"
	
	echo "====BUILDING DEBUG===="

fi

if [ "$1" = "-debug" ]
then

	echo
	echo

	echo "====BUILDING FOR GDB===="
	
	eval $path_to_gcc -o gdb_test $common_params $server_all
	
	echo
	echo
	
	eval $path_to_gdb gdb_test
	
	rm -f ./gdb_test

else

	eval $path_to_gcc -o ../bin/server $common_params $server_all

fi

echo
echo

echo "====DONE===="

echo