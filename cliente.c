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
  char   recvline[MAXLINE + 1];
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

  // Chama a função getsockname para pegar o endereço ip e porta do socket local
  bzero(&_local, sizeof(_local));
  socklen_t len = sizeof(_local);
  if (getsockname (sockfd, (struct sockaddr *) &_local, &len) == -1) {
    perror("getsockname() failed");
    return -1;
  }

  //Imprime as informações do socket local
  //printf("IP SOCKET LOCAL: %s\nPORT SOCKET LOCAL: %d\n", inet_ntoa(_local.sin_addr), ntohs(_local.sin_port));

  //Pega as informações do socket remoto
  socklen_t lenRemote = sizeof(_remote);
  if (getpeername(sockfd, (struct sockaddr *) &_remote, &lenRemote) == -1) {
    perror("getpeername() failed hard");
    return -1;
  }
  //Imprime as informações do socket remoto
  //printf("\nIP SOCKET REMOTO: %s\nPORT SOCKET REMOTO: %d\n", inet_ntoa(_remote.sin_addr), ntohs(_remote.sin_port));

  int maxfdp1, stdineof;
  fd_set rset;
  
  stdineof = 0;
  FD_ZERO(&rset);

  // Troca de mensagens com o servidor
  do {
    if (stdineof == 0)
        FD_SET(fileno(stdin), &rset);

    FD_SET(sockfd, &rset);

    maxfdp1 = (fileno(stdin)>=sockfd ? fileno(stdin) : sockfd) + 1;
    
    select(maxfdp1, &rset, NULL, NULL, NULL);


    if (FD_ISSET(sockfd, &rset)) { /* socket is readable */
        // Escreve na saida padrao a informacao obtida do servidor

      if((n = read(sockfd, recvline, sizeof(recvline)-1)) == 0){
        return 0;
      }

      //printf("N: %d\n", n);
      recvline[n] ='\0';
      if (fputs(recvline, stdout) == EOF) {
        perror("fputs error");
        exit(1);
      }
      fflush(stdout);
      recvline[0] = '\0';
    } else{
        //printf("%s\n", "ELSE");
    }

    if (FD_ISSET(fileno(stdin), &rset)) {  /* input is readable */
        //recvline[n] = 0;
        // Le a entrada padrao
        buffer = (char *)malloc(msgsize * sizeof(char));
        if(getline(&buffer, &msgsize, stdin) != EOF){
            strcpy(msg, buffer);    
        }else{
            stdineof = 1;
            shutdown(sockfd, SHUT_WR);  /* send FIN */
            FD_CLR(fileno(stdin), &rset);
            continue;
        }
        //printf("PRÉ SEND TO SOCKET: %lu\n", sizeof(msg));
        usleep(500);
        if (send(sockfd, msg, strlen(msg), 0) < 0) {
            puts("Send failed.");
            return 1;
        }
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
