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

#define MAXDATASIZE 256

int lengthAndName(int s, char * buf);
int download(int s, char * buf);
int upload(int s, char * buf);
int delete(int s, char * buf);
int list(int s, char * buf);
int makeDir(int s, char * buf);
int removeDir(int s, char * buf);
int changeDir(int s, char * buf);
int quit(int s);

int main(int argc, char * argv[] )
{
  struct hostent *hp;
  struct sockaddr_in sin;
  char *host;
  char buf[MAXDATASIZE];
  int port, s;

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
  if ((s=socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("client: socket error");
    exit(1);
  }

  if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
    perror("client: connect failed");
    shutdown(s, SHUT_RDWR);
    exit(1);
  }

  int retv = 0;
  while (1) {

    printf("Operation: ");
    fgets(buf, sizeof(buf), stdin);
    buf[MAXDATASIZE-1] = '\0';

    if      (!strncmp(buf,"DWLD\n",5)) retv = download(s, buf);
    else if (!strncmp(buf,"UPLD\n",5)) retv = upload(s, buf);
    else if (!strncmp(buf,"DELF\n",5)) retv = delete(s, buf);
    else if (!strncmp(buf,"LIST\n",5)) retv = list(s, buf);
    else if (!strncmp(buf,"MDIR\n",5)) retv = makeDir(s, buf);
    else if (!strncmp(buf,"RDIR\n",5)) retv = removeDir(s, buf);
    else if (!strncmp(buf,"CDIR\n",5)) retv = changeDir(s, buf);
    else if (!strncmp(buf,"QUIT\n",5)) {
      retv = quit(s);
      break;
    }
    else
      printf("Unrecognized command.\n");

    if(retv) break;
  }

  shutdown(s, SHUT_RDWR);
  printf("Session closed.\n");

  return retv;
}

int lengthAndName(int s, char * buf) {
  fgets(buf, MAXDATASIZE, stdin);
  buf[MAXDATASIZE-1] = '\0';

  short len = strlen(buf);

  // strip newline
  if(buf[len-1] == '\n') {
    buf[len-1] = '\0';
    len--;
  }

  char numBuf[4];
  sprintf(numBuf, "%d", len);
  short digits;
  if(len <= 9)
    digits = 1;
  else if(len <= 99)
    digits = 2;
  else
    digits = 3;

  // send file length
  if (send(s, numBuf, digits + 1, 0) == -1 ) {
    perror("client: send error");
    return 1;
  }

  // send file name
  if (send(s, buf, len + 1, 0) == -1 ) {
    perror("client: send error");
    return 1;
  }

  return 0;
}

int download(int s, char * buf) {
  if (send(s, "DWLD", 5, 0) == -1) {
    perror("client: send error");
    return 1;
  }

  printf("File name: ");

  if(lengthAndName(s, buf) == 1)
    return 1;

  return 0;
}

int upload(int s, char * buf) {
  if (send(s, "UPLD", 5, 0) == -1) {
    perror("client: send error");
    return 1;
  }

  printf("File name: ");

  if(lengthAndName(s, buf) == 1)
    return 1;

  return 0;
}

int delete(int s, char * buf) {
  if (send(s, "DELF", 5, 0) == -1) {
    perror("client: send error");
    return 1;
  }

  printf("File name: ");

  if(lengthAndName(s, buf) == 1)
    return 1;

  return 0;
}

int list(int s, char * buf) {
  if (send(s, "LIST", 5, 0) == -1) {
    perror("client: send error");
    return 1;
  }

  return 0;
}

int makeDir(int s, char * buf) {
  if (send(s, "MDIR", 5, 0) == -1) {
    perror("client: send error");
    return 1;
  }

  printf("Directory name: ");

  if(lengthAndName(s, buf) == 1)
    return 1;

  return 0;
}

int removeDir(int s, char * buf) {
  if (send(s, "RDIR", 5, 0) == -1) {
    perror("client: send error");
    return 1;
  }

  printf("Directory name: ");

  if(lengthAndName(s, buf) == 1)
    return 1;

  return 0;
}

int changeDir(int s, char * buf) {
  if (send(s, "CDIR", 5, 0) == -1) {
    perror("client: send error");
    return 1;
  }

  printf("Directory name: ");

  if(lengthAndName(s, buf) == 1)
    return 1;

  return 0;
}

int quit(int s) {
  if (send(s, "QUIT", 5, 0) == -1) {
    perror("client: send error");
    return 1;
  }

  return 0;
}
