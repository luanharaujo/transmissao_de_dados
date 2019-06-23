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
//************************
//Inclusões dos cabeçalhos
//************************
#include <stdio.h> //cabeçalho para uso das bibliotecas padrão de entrada e saida
#include <string.h> //cabeçalho necessario para uso da função bzero dentre outras
#include <unistd.h> //cabeçalho necessario para uso das funções fork, close, read dentre outras
#include <stdlib.h> //cabeçalho necessario para uso da função exit dentre outras
#include <netdb.h> //cabeçalho para uso da função socket entre outras
#include <arpa/inet.h> //cabeçalho para uso da funçao inet_pton dentre outras

//************************
//		definições
//************************
#define	SA	struct sockaddr //estrutura de endereços de sockets
#define TAM_MAX 100000 //tamanho maximo de caracteres da mensagem
#define SERV_PORT 8080 //porta usada para comunicar com o navegador
#define LISTENQ 5 //quantidade de conexões simutaneas aceitas pelo servidor até que o processo filho seja criado 

//************************
//declarações das funções
//************************
int proxy(int sockfd);
int regras(char* requisicao);
int verifica_cache(char* requisicao);
int ler_requisicao(char* requisicao, int sockfd, char * nome_host);
int conecta_servidor(char* nome_host);
void enviar_requisicao(int sockfd, char * requisicao, int tam_req);
void repassar_objetos(int sockfd_server, int sockfd_navegador);

//************************
//			MAIN
//************************
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

		//criando um processo filho para cuidar da requisição
		if ( (childpid = fork()) == 0) {	
			close(listenfd);//para de aceitar conexão, pois o processo pai que cuida das conexoes
			proxy(connfd);//trata a requisição
			exit(0);//encerra o processo filho
		}
		close(connfd);//pai, fecha a conexao que será tratada pelo filho
	}

	//testando se consegui encerrar o programa corretamente
	printf("passei aqui\n");
	return 0;//retornando zero para o sistema operacional para indicar que o programa encerrou corretamente
}	//porém a primcipio o programa não deve encerrar, haha

//************************
//assinaturas das funções
//************************

//função que ferifica a lista preta e a lista branca
//retorna 1 caso o site seja liberado e 0 caso seja bloqueado
int regras(char* requisicao)
{
	return 1;
}

//verifica se a pagina já encontrasse no cache
//retorna 1 caso já esteja no cache e 0 caso não esteja
int verifica_cache(char* requisicao)
{
	char identificador[300];

	//recebendo a primeira linha da requisição que escolhi para identificar as entradas do cache
	sscanf(requisicao, "%[^\n]s", identificador);

	return 0;
}

//recebe uma requisicao do navegador e retorna na variável requisicao
//retrona o tamanho da requisição para faciliar o envio dela para o servidor posteriormente
int ler_requisicao(char* requisicao, int sockfd, char * nome_host)
{
	//variavel auxiliar para nao perder o ponteiro inicial
	char *aux = requisicao;
	//variaveis auxiliares para identificao do fim da requisicao
	char *inicio_linha = requisicao;
	char *fim_linha;
	
	int tamanho = 0;//guardar o tamnho da requisição
	int achou_host = 0;//flag para identificar o conhecimento ou não do nome do host

	//lendo um caractere de cada vez
	while(read(sockfd,aux++,1)>0){
		tamanho++;
		if(*(aux-1) == 10)//identificando o fim de uma linha
		{
			fim_linha = aux-1;//marcando o fim da linha

			//checando se o nome do host já foi encontrado
			if(!achou_host)
			{
				if(sscanf(inicio_linha,"Host: %s",nome_host))
				{
					//ativando a flag para identificar que o nome do host foi encontrado
					achou_host = 1;
				}
			}

			//identificando uma linha vazia que marca o fim de uma reuisiçao
			if((fim_linha - inicio_linha) == 1)
			{
				return(tamanho);//terminando a leitura da requisição
			}
				inicio_linha = aux;//marca o inicio de um nova linha na requisição
		}
	}
}

//função que trata cada requisição feita pelo navegador
int proxy(int socket_cliente)
{
	char objeto[TAM_MAX];
	char requisicao[TAM_MAX];
	int tam_req;
	int socket_servidor;
	char nome_host[300];

	
	tam_req = ler_requisicao(requisicao, socket_cliente, nome_host);
	printf("%s\n", nome_host);
	//log();
	if(regras(requisicao))
	{
		//passou nas regras para conexão
		if(!verifica_cache(nome_host))
		{
			//ler do servidor e criar cache
			socket_servidor = conecta_servidor(nome_host);
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

//	funcão que descrobre o ip a partir de um nome e conecta neste servidor
//	boa parte do codigo desta função foi adaptado dos exemplos do manual 
//	do unix da funcao getaddrinf() que pode ser acessado de qualquer ter
//	minal linux digitando: man getaddrinfo
int conecta_servidor(char* host_name)
{
	//pegar endereço ip da requisição
	int	sockfd;
	struct addrinfo hints;
	struct addrinfo *result, *rp;


	//inicializando a estrutura
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;          

	//obtendo endereço ip a partir do host name
	getaddrinfo(host_name, "80", &hints, &result);

	//preciso entender esta parte melhor, ainda nao comprendi totalmetne
	for (rp = result; rp != NULL; rp = rp->ai_next) {
       sockfd = socket(rp->ai_family, rp->ai_socktype,
                    rp->ai_protocol);
       if (sockfd == -1)
           continue;

       if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
           break;                  /* Success */

       close(sockfd);
   }

	//conectando com o servidor
	connect(sockfd, rp->ai_addr, rp->ai_addrlen);
	
	//retornardo o descritor do arquivo do sockt criado
	return (sockfd);
}

//função que envia requisição para o servidor
void enviar_requisicao(int sockfd, char *requisicao, int tam_req)
{
	write(sockfd, requisicao, tam_req);

	return;
}

//função que repassa objeto do servidor para o navegador
//e cria o cache
void repassar_objetos(int sockfd_server, int sockfd_navegador)
{
	char temp;

	while(read(sockfd_server,&temp,1)>0){
		//printf("%c", temp);
		write(sockfd_navegador,&temp,sizeof(temp));	
	}
	
	return;
}
