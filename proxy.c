/*
*	Universidade de Brasília
*	Departamento de Ciências da Computação
*	Transmissão de dados
*	Trabalho Final
*	Luan Haickel Araújo
*	12/0125781
*	luanharaujo@gmail.com
*
*	Este trabalho foi feito usando como base os 
*	códigos do livro "Unix Network Programing" 
*	do "Richard Stevens" e os exemplos dos manuais 
*	do linux/unix
*
*/

#include <stdio.h> //cabeçalho para uso das bibliotecas padrão de entrada e saida
#include <string.h> //cabeçalho necessario para uso da função bzero dentre outras
#include <unistd.h> //cabeçalho necessario para uso das funções fork, close, read dentre outras
#include <stdlib.h> //cabeçalho necessario para uso da função exit dentre outras
#include <netdb.h> //cabeçalho para uso da função socket entre outras

#define	SA	struct sockaddr //estrutura de endereços de sockets
#define TAM_MAX 100000 //tamanho maximo de caracteres da mensagem
#define SERV_PORT 8080 //porta usada para comunicar com o navegador
#define LISTENQ 5 //quantidade de conexões simutaneas aceitas pelo servidor até que o processo filho seja criado

//declaração das funções
int proxy(int sockfd);
int regras();
int verifica_cache();
void ler_requisicao(char* requisicao, int sockfd);

int main()
{
	int					listenfd, connfd;
	pid_t				childpid;
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;

	//abrindo o socket de rede internet (IPV4) do tipo stream (TCP)
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	//inicializando a estrutura
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT);

	//vinculando o socket com o endereçamento do Linux
	bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	//ablitando conexões
	listen(listenfd, LISTENQ);

	//loop infinito para atender a multiplas requisições
	while(1) { 
		clilen = sizeof(cliaddr);
		
		//aguardando a conexao do navegador
		connfd = accept(listenfd, (SA *) &cliaddr, &clilen);

		//criando um processo filho para coidar da requisição
		if ( (childpid = fork()) == 0) {	
			close(listenfd);//para de aceitar conexão, pois o processo pai que cuida das conexoes
			proxy(connfd);//faz tudo
			exit(0);//encerra o processo filho
		}
		close(connfd);//pai, fecha a conexao que será tratada pelo filho
	}

	//testando se consegui encerrar o programa corretamente
	printf("passei aqui\n");
	return 0;//retornando zero para o sistema operacional para indicar que o programa encerrou corretamente
}	//porém a primcipio o programa não deve encerrar, haha

//assinaturas das funções
int regras()
{
	return 1;
}

int verifica_cache()
{
	return 0;
}

//recebe uma requisicao do navegador e retorna na variavel requisicao
void ler_requisicao(char* requisicao, int sockfd)
{
	//variavel auxiliar para nao perder o ponteiro inicial
	char *aux = requisicao;
	//variaveis auxiliares para identificao do fim da requisicao
	char *inicio_linha = requisicao;
	char *fim_linha;
	
	//lendo um caractere de cada vez
	while(read(sockfd,aux++,1)>0){
		if(*(aux-1) == 10)//identificando o fim de uma linha
		{
			fim_linha = aux-1;
			if((fim_linha - inicio_linha) == 1)//identificando uma linha vazia que marca o fim de uma reuisiçao
			{
				return;//terminando a leitura da requisição
			}
				inicio_linha = aux;
		}
	}
}

//faz a poha toda
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
