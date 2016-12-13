#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXLINE 4096

int main(int argc, char **argv) {

  int    sockfd, n;
  char   error[MAXLINE + 1];
  char   msg[1000];
  char   *buffer;
  size_t msgsize = 1000;
  struct sockaddr_in servaddr, _local, _remote;

  // Verifica o argumento passada para o cliente pela entrada padrao
  if (argc != 2) {
    strcpy(error,"uso: ");
    strcat(error,argv[0]);
    strcat(error," <IPaddress>");
    perror(error);
    exit(1);
  }

  // Pega o socket para ser realizado a conexao
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket error");
    exit(1);
  }

  // Inicializa a variaevel de endereço do servidor
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port   = htons(13000);

  // Verifica a validade do IP passado como parametro
  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
    perror("inet_pton error");
    exit(1);
  }

  // Realiza a conexao com o servidor
  if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
    perror("connect error");
    exit(1);
  }

  //cria as variaveis utilizadas pelo select
  int maxfdp1, stdineof;
  fd_set rset;
  
  stdineof = 0;

  // Troca de mensagens com o servidor
  do {

    //reseta os vetores de sockets que devem ser olhados pelo select
    FD_ZERO(&rset);

    //se terminou de ler a entrada padrão não precisa mais passar ela pro select
    if (stdineof == 0)
        FD_SET(fileno(stdin), &rset);

    //adiciona o socket com o servidor na lista do select
    FD_SET(sockfd, &rset);

    //pega o maior id do socket para passar pro select
    maxfdp1 = (fileno(stdin)>=sockfd ? fileno(stdin) : sockfd) + 1;
    
    //chama o select e fecha o programa em caso de erro

    //ESSE É O PONTO ONDE O PROGRAMA FICA TRAVADO APÓS EXECUTAR shutdown(sockfd, SHUT_WR);
    //ele não passa do select mas deveria passar e retornar true para FD_ISSET(sockfd, &rset)
    int ret = select(maxfdp1, &rset, NULL, NULL, NULL);

    if(ret <=0)
        return 0;

    //verifica se o socket conectado ao servidor esta disponivel para leitura
    if (FD_ISSET(sockfd, &rset)) { 
    
        //cria o buffer de leitura do socket e realiza a leitura
        char recvline[MAXLINE + 1] = {'\0'};
        if((n = read(sockfd, recvline, sizeof(recvline)-1)) == 0){
            //se a leitura foi finalizada encerra o programa
            /* ESSA PARTE QUE NÃO ESTÁ FUNCIONANDO. APÓS FECHAR A CONEXÃO E TERMINAR A ESCRITA DEVERIA ENTRAR AQUI*/
            if(stdineof)
                return 0;
        }

        //Escreve na saída padrão o echo do servidor
        recvline[n] ='\0';

        //checca pelo fim do arquivo delimitado pelo conjunto de caracteres abaixo enviado pelo servidor antes de encerrar a conexão
        char *end = "&end&";
        if(strcmp(recvline, end)==0){

          //se a leitura do stdin ja terminou
          if(stdineof)
                return 0;

        }
        if (fputs(recvline, stdout) == EOF) {
            perror("fputs error");
            exit(1);
        }
        fflush(stdout);
        recvline[0] = '\0';

    }

    //verifica se a entrada padrão esta disponível para leitura
    if (FD_ISSET(fileno(stdin), &rset)) {  

        // Le a entrada padrao
        buffer = (char *)malloc(msgsize * sizeof(char));
        if(getline(&buffer, &msgsize, stdin) != EOF){
            strcpy(msg, buffer);    
        }else{
            //se chegar ao fim da entrada padrão seta a flag e encerra a conexão de escrita no socket
            stdineof = 1;
            shutdown(sockfd, SHUT_WR);  /* send FIN */
            //FD_CLR(fileno(stdin), &rset);
            FD_ZERO(&rset);
            continue;
        }
        //sleep para não mandar o conteúdo do arquivo inteiro de uma vez
        usleep(500);

        //escreve o conteúdo lido no socket para o servidor
        if (send(sockfd, msg, strlen(msg), 0) < 0) {
            puts("Send failed.");
            return 1;
        }

        //limpa as entradas e saídas padrões
        fflush(stdout);
        fflush(stdin);

    }
  } while (1);

  // Verifica por erro na conexao
  if (n < 0) {
    perror("read error");
    exit(1);
  }

  exit(0);
}
