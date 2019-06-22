#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <netdb.h>

#define	SA	struct sockaddr
#define TAM_MAX 100000
#define SERV_PORT 8080
#define LISTENQ 5

int proxy(int sockfd);

int main(int argc, char **argv)
{
	int					listenfd, connfd;
	pid_t				childpid;
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;


	listenfd = socket(AF_INET, SOCK_STREAM, 0);


	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT);

	bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	listen(listenfd, LISTENQ);


	for ( ; ; ) {
		clilen = sizeof(cliaddr);
		connfd = accept(listenfd, (SA *) &cliaddr, &clilen);

		if ( (childpid = fork()) == 0) {	/* child process */
			close(listenfd);	/* close listening socket */
			proxy(connfd);	/* process the request */
			exit(0);
		}
		close(connfd);			/* parent closes connected socket */
	}
	return 0;
}

int regras()
{
	return 1;
}

int verifica_cache()
{
	return 0;
}

void ler_requisicao(char* requisicao, int sockfd)
{
	char *aux = requisicao;
	char *inicio_linha = requisicao;
	char *fim_linha;
	
	while(read(sockfd,aux++,1)>0){
		if(*(aux-1) == 10)
		{
			fim_linha = aux-1;
			if((fim_linha - inicio_linha) == 1)
			{
				return;
			}
				inicio_linha = aux;
		}
	}
}

int proxy(int sockfd)
{
	char objeto[TAM_MAX];
	char requisicao[TAM_MAX];

	
	ler_requisicao(requisicao, sockfd);
	//log();
	if(regras())
	{
		//passou nas regras para conexão
		if(!verifica_cache(requisicao))
		{
			//ler do servidor
			// conecta_servidor(requisicao);
			// obejeto = recebe_objetos();
			// guarda_cache(objeto);
			// desconecta_servidor(requisicao);
		}
		else
		{
			//ler do cache
			//obejeto = ler_cache(requisicao);
		}
		//reponde_navegador(objeto);
	}
	else
	{
		//reprovou nas regras para conexão
		//mensagem de erro apra o navegador
	}
	
	return 0;
}
