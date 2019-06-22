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
#include <arpa/inet.h> //cabeçalho para uso da funçao inet_pton dentre outras

#define	SA	struct sockaddr //estrutura de endereços de sockets
#define TAM_MAX 100000 //tamanho maximo de caracteres da mensagem
#define SERV_PORT 8080 //porta usada para comunicar com o navegador
#define LISTENQ 5 //quantidade de conexões simutaneas aceitas pelo servidor até que o processo filho seja criado 

//declaração das funções
int proxy(int sockfd);
int regras(char* requisicao);
int verifica_cache(char* requisicao);
int ler_requisicao(char* requisicao, int sockfd);
int conecta_servidor(char* requisicao);
void enviar_requisicao(int sockfd, char * requisicao, int tam_req);
void repassar_objetos(int sockfd_server, int sockfd_navegador);

int gambiarra;

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
		gambiarra = connfd;
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
int regras(char* requisicao)
{
	return 1;
}

int verifica_cache(char* requisicao)
{
	return 0;
}

//recebe uma requisicao do navegador e retorna na variavel requisicao
int ler_requisicao(char* requisicao, int sockfd)
{
	//variavel auxiliar para nao perder o ponteiro inicial
	char *aux = requisicao;
	//variaveis auxiliares para identificao do fim da requisicao
	char *inicio_linha = requisicao;
	char *fim_linha;
	int tamanho = 0;
	
	//lendo um caractere de cada vez
	while(read(sockfd,aux++,1)>0){
		tamanho++;
		if(*(aux-1) == 10)//identificando o fim de uma linha
		{
			fim_linha = aux-1;
			if((fim_linha - inicio_linha) == 1)//identificando uma linha vazia que marca o fim de uma reuisiçao
			{
				return(tamanho);//terminando a leitura da requisição
			}
				inicio_linha = aux;
		}
	}
}

//faz a poha toda
int proxy(int socket_cliente)
{
	char objeto[TAM_MAX];
	char requisicao[TAM_MAX];
	int tam_req;
	int socket_servidor;

	
	tam_req = ler_requisicao(requisicao, socket_cliente);
	printf("%s\n", requisicao);
	//log();
	if(regras(requisicao))
	{
		//passou nas regras para conexão
		if(!verifica_cache(requisicao))
		{
			//ler do servidor e criar cache
			socket_servidor = conecta_servidor(requisicao);
			enviar_requisicao(socket_servidor, requisicao, tam_req);
			repassar_objetos(socket_servidor, socket_cliente);
	
		}
		else
		{
			//repassar do cache
			//obejeto = ler_cache(requisicao);
		}
	}
	else
	{
		//reprovou nas regras para conexão
		//mensagem de erro apra o navegador
	}
	
	return 0;
}

int conecta_servidor(char* requisicao)
{
	char endereco_ip[100] = "164.41.102.70";
	int tamanho;

	//variavel auxiliar para nao perder o ponteiro inicial
	char *aux = requisicao;
	//variaveis auxiliares para identificao do fim da requisicao
	char *inicio_linha = requisicao;
	char *fim_linha;
	
	//pegar endereço ip da requisição

	int	sockfd;
	struct sockaddr_in	servaddr;

	//printf("inicio_conecta_servidor\n");

	//criando o socket IPV4 TCP
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	//printf("socket criado\n");

	//inicializando a estrutura
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(80);
	inet_pton(AF_INET, endereco_ip, &servaddr.sin_addr);

	//printf("estrutura iniciada\n");

	//conectando com o servidor
	connect(sockfd, (SA *) &servaddr, sizeof(servaddr));
	//printf("conetado\n");
	
	//termina aqui esta funcao 
	
	
	//printf("feito\n");
	return (sockfd);
}

//função que envia requisição para o servidor
void enviar_requisicao(int sockfd, char *requisicao, int tam_req)
{
	write(sockfd, requisicao, tam_req);

	return;
}

//função que repassa objeto do servidor para o navegador
void repassar_objetos(int sockfd_server, int sockfd_navegador)
{
	char temp;
	
	while(read(sockfd_server,&temp,1)>0){
		printf("%c", temp);
		write(sockfd_navegador,&temp,sizeof(temp));
		
	}
	return;
}
