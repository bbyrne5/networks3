/*
 * client.c
 * David Mellitt dmellitt
 * Simple UDP client that contacts a server, recieves a key,
 * sends a message, decrypts a message from the server, and
 * prints the round trip time
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

int download();
int upload();
int delete();
int list();
int makeDir();
int removeDir();
int changeDir();
int quit();

int main(int argc, char * argv[] )
{
  FILE *fp = NULL;
  struct hostent *hp;
  struct sockaddr_in sin;
  struct timeval start, end;
  char *host;
  char buf[MAXDATASIZE];
  char key[MAXDATASIZE];
  int port, s, key_len, data_len, i;
  socklen_t addr_len;

  if (argc != 3) {
    fprintf(stderr, "usage: ./myftp host port\n");
    exit(1);
  }

  host = argv[1];
  port = atoi(argv[2]);

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

  while (1) {
    fgets(buf, sizeof(buf), stdin);
    buf[MAXDATASIZE-1] = '\0';

    if (!strncmp(buf,"DWLD",4)) download();
    else if (!strncmp(buf,"UPLD",4)) upload();
    else if (!strncmp(buf,"DELF",4)) delete();
    else if (!strncmp(buf,"LIST",4)) list();
    else if (!strncmp(buf,"MDIR",4)) makeDir();
    else if (!strncmp(buf,"RDIR",4)) removeDir();
    else if (!strncmp(buf,"CDIR",4)) changeDir();
    else if (!strncmp(buf,"QUIT",4)) {
      quit();
      break;
    }
    else
      printf("Unrecognized command.\n");
  }

  close(s);

  return 0;
}

int download() {
  return 0;
}

int upload() {
  return 0;
}

int delete() {
  return 0;
}

int list() {
  return 0;
}

int makeDir() {
  return 0;
}

int removeDir() {
  return 0;
}

int changeDir() {
  return 0;
}

int quit() {
  return 0;
}
