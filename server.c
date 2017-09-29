/*
 * server.cpp
 * Brian Byrne bbyrne5, David Mellitt dmellitt
 * TCP server that implements FTP
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>

#define MAXDATASIZE 4200

int main(int argc, char * argv[] )
{
  struct sockaddr_in sin, client_addr;
  struct timeval tv;
  struct tm *info;
  int rqst;
  char *key;
  char buf[MAXDATASIZE];
  int port, s, len;
  socklen_t addr_len;

  if (argc != 3) {
    fprintf(stderr, "Usage: server port key\n");
    exit(1);
  }

  port = atoi(argv[1]);
  key = argv[2];

  // build address
  bzero((char *)&sin, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);

  // passive open
  if ((s=socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("Server Socket Error!\n");
    exit(1);
  }

  if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
    perror("Server Bind Error!\n");
    exit(1);
  }

  addr_len = sizeof(client_addr);
  
  if (listen(s, 5) < 0) {
    perror("listen failed");
    exit(1);
  }
  
  while (1){
    while ((rqst = accept(s, (struct sockaddr *)&client_addr, &addr_len)) < 0){
        perror("accept failed");
        exit(1);
    }
  }

  shutdown(s, SHUT_RDWR);

  return 0;
}
