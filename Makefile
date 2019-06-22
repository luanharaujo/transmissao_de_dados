all: cliente servidor proxy
cliente: cliente.c
	gcc cliente.c -o cliente
servidor: servidor.c
	gcc servidor.c -o servidor
proxy: proxy.c
	gcc proxy.c -o proxy
