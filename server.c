/*
 * server.c
 * David Mellitt dmellitt
 * Simple UDP server that sends an encryption key, receive a message,
 * append a timestamp to the message, encrypt the message, and send
 * it back to the client
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
  char *key;
  char buf[MAXDATASIZE];
  int port, s, len;
  socklen_t addr_len;

  if (argc != 2) {
    fprintf(stderr, "Usage: ./myftpd port\n");
    exit(1);
  }

  port = atoi(argv[1]);

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

  while (1){
    if (recvfrom(s, buf, sizeof(buf), 0,  (struct sockaddr *)&client_addr, &addr_len)==-1){
      perror("Server Receive Error!\n");
      exit(1);
    }
    printf("Received: %s\n", buf);

    bzero((char*)&buf, sizeof(buf));

  }
 
  close(s);

  return 0;
}
