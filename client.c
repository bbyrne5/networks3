/*
 * client.c
 * Brian Byrne bbyrne5, David Mellitt dmellitt
 * TCP client implementing FTP
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>

#define MAXDATASIZE 4096

int main(int argc, char * argv[] )
{
  FILE *fp = NULL;
  struct hostent *hp;
  struct sockaddr_in sin;
  struct timeval start, end;
  char *host;
  char *data;
  char buf[MAXDATASIZE];
  char key[MAXDATASIZE];
  int port, s, key_len, data_len, i;
  socklen_t addr_len;

  if (argc != 4) {
    fprintf(stderr, "usage: client host port data\n");
    exit(1);
  }

  host = argv[1];
  port = atoi(argv[2]);
  data = argv[3];
  fp = fopen(data, "r");
  if(!fp)
    strcpy(buf,data);
  else {
    fread(buf, sizeof(char), MAXDATASIZE, fp);
  }

  // translate host name into IP address
  hp = gethostbyname(host);
  if(!hp) {
    fprintf(stderr, "client: unknown host: %s\n", host);
    exit(1);
  }

  // build address
  bzero((char *)&sin, sizeof(sin));
  sin.sin_family = AF_INET;
  bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
  sin.sin_port = htons(port);

  // active open
  if ((s=socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("client: socket");
    exit(1);
  }

  addr_len = sizeof(struct sockaddr);
  
  if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
    perror("connect failed");
    exit(1);
  }
  
  shutdown(s, SHUT_RDWR);

  return 0;
}
