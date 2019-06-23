#!/bin/bash
make
rm cache/cache_[0-9]*
rm cache/cache_list.txt
cp cache/original_cache_list.txt cache/cache_list.txt
while [ "`netstat -na | grep 8080|wc -l`" != "0" ]
do 
	echo aguardando porta 8080 ser liberada
	sleep 2
done 

echo porta liberada

./proxy

#firefox www.unb.br
