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
#define DEBUG 1

#define	SA	struct sockaddr //estrutura de endereços de sockets
#define TAM_MAX 1000000 //tamanho maximo de caracteres da mensagem
#define SERV_PORT 8080 //porta usada para comunicar com o navegador
#define LISTENQ 0 //quantidade de conexões simutaneas aceitas pelo servidor até que o processo filho seja criado 

#define WHITELIST 1
#define BLACKLIST -1
#define NO_INFO 0

//************************
//declarações das funções
//************************
int proxy(int sockfd, int cache);
int regras(char* url_req);
int verifica_cache(char* requisicao, char* identificador);
int ler_requisicao(char* requisicao, int sockfd, char * nome_host);
int conecta_servidor(char* nome_host);
void enviar_requisicao(int sockfd, char * requisicao, int tam_req);
void repassar_objetos(int sockfd_server, int sockfd_navegador, int filtra);
void criar_cache(int sockfd,int numero_cache, char* identificador);
void cache2navegador(int numero_cache, int sockfd_navegador);
void acesso_negado(int sockfd_navegador);

//************************
//			MAIN
//************************
int main(int argc, char const *argv[])
{
	int					listenfd, connfd;
	pid_t				childpid;
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;
	int cache = 0;

	//analizando as chaves para ativar ou nao o modo cache
	if((argc > 1) && (strcmp("-c",argv[1])==0))
	{
		printf("Modo cache ativado\n");
		cache = 1;
	}
	else
	{
		printf("Modo cache desativado\n");
		cache = 0;
	}

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
			proxy(connfd, cache);//trata a requisição
			exit(0);//encerra o processo filho
		}
		close(connfd);//pai, fecha a conexao que será tratada pelo filho
		if(cache)
		{
			//tempo necessario para que a lista de cache seja
			//verificada e atualizanda sem que outra requisição interrompa
			sleep(1);
		}
	}

	//testando se consegui encerrar o programa corretamente
	//para isso preciso apreder a capturar o crontrol+c
	if(DEBUG) printf("Programa encerrado corretamente\n");
	return 0;//retornando zero para o sistema operacional para indicar que o programa encerrou corretamente
}	

//************************
//assinaturas das funções
//************************

//função que ferifica a lista preta e a lista branca
//retorna 1 caso o site esteja na whitelist, -1 caso
//esteja na blacklist e 0 caso não esteja em nenhuma das listas
int regras(char* url_req)
{
	FILE* fp;
	char url[300];
	
	//verificando se o url esta na whitelist
	fp = fopen("whitelist","r");
	while(fscanf(fp,"%s",url)!=EOF)
	{
		if(strcmp(url,url_req)==0)
		{
			fclose(fp);
			if(DEBUG) printf("Ta na whitelist\n");
			return(WHITELIST);		
		}
	}
	fclose(fp);
	
	//verificando se o url esta na blacklist
	fp = fopen("blacklist","r");
	while(fscanf(fp,"%s",url)!=EOF)
	{
		if(strcmp(url,url_req)==0)
		{
			fclose(fp);
			if(DEBUG) printf("Ta na blacklist\n");
			return(BLACKLIST);		
		}
	}
	fclose(fp);
	
	if(DEBUG) printf("Não consta em nenhuma lista\n");

	return (NO_INFO);
}

//verifica se a pagina já encontrasse no cache
//retorna o numero do arquivo cache caso já esteja 
//no cache e um valor negativo correspondete ao 
//proximo numero disponiovel caso não esteja
int verifica_cache(char* requisicao, char* identificador)
{
	char line[300];
	int numero_arquivo_cache;
	char aux;
	int i;
	FILE *fp;
	

	//abrindo o arquivo ande esta indexado os arquivos de cache
	fp = fopen("cache/cache_list.txt","r");

	if(!fp)//caso o arquivo ainda não exista siguinifica que ainda não existe nenhum cache
	{
		return(0);
	}

	//recebendo a primeira linha da requisição que escolhi para identificar as entradas do cache
	sscanf(requisicao, "%[^\n]s", identificador);

	//buscando se a requisiçao já existe no cache
	numero_arquivo_cache = 1;
	do//percorrendo cada linha do arquivo
	{
		i = 0;
		do//percorrendo as letras de cada linha
		{
			fscanf(fp,"%c",&line[i++]);
		}while((line[i-1] != EOF) && (line[i-1] != '\n'));//verificando se encontrou o fim da linha ou do arquivo
		aux = line[i-1];//cuidando para nao perder o EOF
		line[i-1] = '\0';//arrumando o fim da string
		if(strcmp(line,identificador)==0)//checando esta linha
		{
			//caso encontre: fecha o arquivo, retorna o valor, encerra a rotina, escova os dentes, apaga a luz e vai dormir
			fclose(fp);
			return(numero_arquivo_cache);
		}
		numero_arquivo_cache++;
	}while(aux != EOF);

	//fecehndo o arquivo
	fclose(fp);
	//retorna zero para indicar que nao encontrou
	return (-numero_arquivo_cache);
}

//recebe uma requisicao do navegador e retorna na variável requisicao
//retrona o tamanho da requisição para faciliar o envio dela para o servidor posteriormente
int ler_requisicao(char* requisicao, int sockfd, char * nome_host)
{
	char *aux = requisicao;
	//variavel auxiliar para nao perder o ponteiro inicial
	
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
int proxy(int socket_cliente, int cache)
{
	static char objeto[TAM_MAX];
	static char requisicao[TAM_MAX];
	static int tam_req;
	static int socket_servidor;
	static char nome_host[300];
	static int numero_cache;
	static char identificador_requisicao[300];

	//zerado todas as strings
	bzero(objeto,sizeof(objeto));
	bzero(nome_host,sizeof(nome_host));
	bzero(requisicao,sizeof(requisicao));
	bzero(identificador_requisicao,sizeof(identificador_requisicao));
	
	tam_req = ler_requisicao(requisicao, socket_cliente, nome_host);
	if(DEBUG) printf("Nova requisição recebina: %s\n", nome_host);
	//log();
	switch(regras(nome_host)) 
	{
		case WHITELIST :
			if(cache)
			{
				//verificando se a resposta para esta requisição já esta no cache
				numero_cache = verifica_cache(requisicao,identificador_requisicao);
				if(DEBUG) printf("\nNumero cache: %d\n", numero_cache);
				if(numero_cache < 0)//caso ainda não esteja no cache
				{
					//criar cache
					numero_cache *= -1;//tornarndo o novo numero de cache positivo

					//conecta no servidor
					socket_servidor = conecta_servidor(nome_host);
					//envia a requisição para o servidor
					enviar_requisicao(socket_servidor, requisicao, tam_req);
					//recebe o objeto e guarda em um cache
					criar_cache(socket_servidor,numero_cache, identificador_requisicao);
					//passa o conteudo do cache para o navegardor
					cache2navegador(numero_cache, socket_cliente);
				}
				else
				{
					//repassar do cache
					if(DEBUG) printf("CACHE DESTA REQUISICAO EXISTE\n");
					cache2navegador(numero_cache, socket_cliente);
				}
			}
			else//caso o programa esteja rodando sem cache
			{
				//conecta no servidor
				socket_servidor = conecta_servidor(nome_host);
				//envia a requisição para o servidor
				enviar_requisicao(socket_servidor, requisicao, tam_req);
				//passar mensagem direto do servidor para o navegador
				repassar_objetos(socket_servidor, socket_cliente,0);
			}
		break;
		case BLACKLIST :
			//gerar entrada no log
			//enviar mensagem de bloqueio ao navegador
			acesso_negado(socket_cliente);
		break;
		case NO_INFO :
			//verificar lista de termos proibidos
			if(cache)
				{
					//verificando se a resposta para esta requisição já esta no cache
					numero_cache = verifica_cache(requisicao,identificador_requisicao);
					if(DEBUG) printf("\nNumero cache: %d\n", numero_cache);
					if(numero_cache < 0)//caso ainda não esteja no cache
					{
						//criar cache
						numero_cache *= -1;//tornarndo o novo numero de cache positivo

						//conecta no servidor
						socket_servidor = conecta_servidor(nome_host);
						//envia a requisição para o servidor
						enviar_requisicao(socket_servidor, requisicao, tam_req);
						//recebe o objeto e guarda em um cache
						criar_cache(socket_servidor,numero_cache, identificador_requisicao);
						//passa o conteudo do cache para o navegardor
						cache2navegador(numero_cache, socket_cliente);
					}
					else
					{
						//repassar do cache
						if(DEBUG) printf("CACHE DESTA REQUISICAO EXISTE\n");
						cache2navegador(numero_cache, socket_cliente);
					}
				}
				else//caso o programa esteja rodando sem cache
				{
					//conecta no servidor
					socket_servidor = conecta_servidor(nome_host);
					//envia a requisição para o servidor
					enviar_requisicao(socket_servidor, requisicao, tam_req);
					//passar mensagem direto do servidor para o navegador
					repassar_objetos(socket_servidor, socket_cliente,1);
				}
		break;
	}
	if(DEBUG) printf("Tratamento da requisição encerrado: %s\n", nome_host);
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

	//percorrendo a lista de endereços IPs fornecida pelo DNS
	for (rp = result; rp != NULL; rp = rp->ai_next) {
       sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
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
void repassar_objetos(int sockfd_server, int sockfd_navegador, int filtra)
{
	char temp;
	char palavra[TAM_MAX];
	char palavra_modificada[TAM_MAX];
	int i, j;
	ssize_t aux;
	FILE *fp;
	char negados[1000][100];
	int n_termos;

	bzero(negados,sizeof(negados));

	if(filtra)
	{
		//carregando termos proibidos
		fp = fopen("deny_terms","r");
		n_termos = 0;
		while(fscanf(fp,"%s",negados[n_termos++])>0){}
		n_termos--;
	}
	
	//passando o pacote para o navegador palavra por palavra para permitir 
	//a analisa dos deny_terms
	do
	{
		//lendo cada palavra
		i = 0;
		while((aux = read(sockfd_server,&palavra[i],1))>0 && palavra[i]!=' '){
			i++;//lendo cada letra
		}
		if(aux) {//acressentando o espaço na lalavra caso não seja o fim do arquivo
			i++;	
		}
		palavra[i] = '\0';
		if(filtra)//caso o filtro esteja ablitado
		{
			strcpy(palavra_modificada,palavra);//copiando a palavra para remover o espaço para facilitar acompração
			palavra_modificada[strlen(palavra_modificada)-1] = '\0';//removendo o espaço
			for(j = 0; j< n_termos; j++)//percorrendo a lista de termos proibidos
			{
				if(strcmp(palavra_modificada,negados[j])==0)
				{
					//caso encontrado termo proibido enviar a pagina de conexão negada e finalizar conexão
					if(DEBUG) printf("PALAVRA PROIBIDA ENCONTRADA: %s\n", palavra_modificada);
					acesso_negado(sockfd_navegador);
					return;
				}
			}
		}
		//printf("%s\n", palavra);
		write(sockfd_navegador,palavra,i);
	}
	while(aux>0);//repetir até o fim do arquivo
	
	return;
}

//função que repassa objeto do cache para o navegador
void cache2navegador(int numero_cache, int sockfd_navegador)
{
	char temp;
	FILE* fp;
	char arquivo_cache[50];

	//abrindo o cache
	sprintf(arquivo_cache,"cache/cache_%d.txt",numero_cache);
	fp = fopen(arquivo_cache,"r");


	while(fscanf(fp,"%c",&temp)>0){
		//if(DEBUG) printf("%c", temp);
		write(sockfd_navegador,&temp,sizeof(temp));	
	}
	
	//fechando o arquivo
	fclose(fp);

	return;
}

//funcao que cria um arquivo cache do site requisitado
//e atualiza o arquivo com as listas de cache
void criar_cache(int sockfd,int numero_cache, char *identificador)
{
	char temp;
	FILE *fp;
	char arquivo_cache[30];

	//atualizando a lista de arquivos
	if(DEBUG) printf("Atualizando a lista de cache\n");
	fp = fopen("cache/cache_list.txt","a");
	fprintf(fp, "\n%s", identificador);
	fclose(fp);
	if(DEBUG) printf("Lista de chache atualizada\n");


	if(DEBUG) printf("Criando arquivo cache!\n");
	//criando o novo arquivo cache
	sprintf(arquivo_cache,"cache/cache_%d.txt",numero_cache);
	fp = fopen(arquivo_cache,"w");

	//gravando no arquivo 
	while(read(sockfd,&temp,1)>0){
		fprintf(fp, "%c", temp);
	}

	//fechando o arquivo
	fclose(fp);

	if(DEBUG) printf("Arquivo cache criado!\n");
	
	return;
}

//funcao que envia a mensagem de acesso negado apra o navegador
void acesso_negado(int sockfd_navegador)
{
	FILE *fp;
	char temp;

	fp = fopen("acesso_negado.html","r");

	while(fscanf(fp,"%c",&temp)>0){
		//if(DEBUG) printf("%c", temp);
		write(sockfd_navegador,&temp,sizeof(temp));	
	}
	
	//fechando o arquivo
	fclose(fp);

	return;
}
