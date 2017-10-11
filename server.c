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
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>

#define MAXDATASIZE 4200
#define SMALLSIZE 256


int download(int);
int upload(int);
int delete(int);
int list(int);
int makeDir(int);
int removeDir(int);
int changeDir(int);
int quit();

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
  if ((s=socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Server Socket Error!\n");
    exit(1);
  }

  int opt = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(int));

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
    int valread;
    while ((rqst = accept(s, (struct sockaddr *)&client_addr, &addr_len)) < 0){
        perror("accept failed");
        exit(1);
    }
    valread = read(rqst, buf, MAXDATASIZE);
    if (!strncmp(buf,"DWLD",4)) download(rqst);
    else if (!strncmp(buf,"UPLD",4)) upload(rqst);
    else if (!strncmp(buf,"DELF",4)) delete(rqst);
    else if (!strncmp(buf,"LIST",4)) list(rqst);
    else if (!strncmp(buf,"MDIR",4)) makeDir(rqst);
    else if (!strncmp(buf,"RDIR",4)) removeDir(rqst);
    else if (!strncmp(buf,"CDIR",4)) changeDir(rqst);
    else if (!strncmp(buf,"QUIT",4)) {
      quit();
      break;
    }
    else
      printf("Unrecognized command.\n");
  }

  shutdown(s, SHUT_RDWR);

  return 0;
}

int download(int rqst) {
  //get name length
  char sizeBuf[256];
  int valread = read(rqst, sizeBuf, SMALLSIZE);
  sizeBuf[valread] = 0;
  int size = atoi(sizeBuf);

  //get the name
  char fileName[size+1];
  valread = read(rqst, fileName, size);
  fileName[valread] = 0;

  //check for file
  if(access(fileName, F_OK) != -1){//file does exist

  }
  else {//file does not exist
    printf("File does not exist");
  }
  
  return 0;
}

int upload(int rqst) {
  return 0;
}

int delete(int rqst) {
  //get name length
  char buf[256];
  int valread = read(rqst, buf, SMALLSIZE);
  buf[valread] = 0;
  int size = atoi(buf);

  //get the name
  char dirName[size+1];
  valread = read(rqst, dirName, size);
  dirName[valread] = 0;

  //check for file
  int success = 1;
  int failure = -1;
  if(access(dirName, F_OK) != -1){//file does exist
    send(rqst, &success, sizeof(success), 0);
    read(rqst, buf, SMALLSIZE);
    unlink(dirName);
  }
  else {//file does not exist
    send(rqst, &failure, sizeof(failure), 0);
    printf("File does not exist");
  }

  return 0;
}

int list(int rqst) {
  FILE* p = popen("ls -la", "r");
  if (!p) return 1;
  char buff[1024];
  while (fgets(buff, sizeof(buff), p)) {
    printf("%s", buff);
  }
  pclose(p);
  int size = strlen(buff);
  int tmp = htonl((uint32_t)size);
  send(rqst, &tmp, sizeof(tmp), 0);
  send(rqst, buff, strlen(buff), 0);
  return 0;
}

int makeDir(int rqst) {
  //get name length
  int size;
  int valread = read(rqst, &size, sizeof(size));

  //get the name
  char dirName[size+1];
  valread = read(rqst, dirName, size);
  dirName[valread] = 0;

  struct stat st = {0};
  //check if dir exists and create it if not
  int success = 1;
  int failure = -1;
  int extraFailure = -2;
  if (stat(dirName, &st) == -1){
    int create = mkdir(dirName, 0700);
    if (create > 0)
      send(rqst, &success, sizeof(success), 0); 
    else 
      send(rqst, &failure, sizeof(failure), 0);
  } 
  else {
    send(rqst, &extraFailure, sizeof(extraFailure), 0);
  } 

  return 0;
}

int removeDir(int rqst) {
  //get name length
  int32_t size;
  int valread = read(rqst, &size, sizeof(size));

  //get the name
  char dirName[size+1];
  valread = read(rqst, dirName, size);
  dirName[valread] = 0;

  struct stat st = {0};
  //check if dir exists and create it if not
  int success = 1;
  int failure = -1;
  int extraFailure = -2;
  if (stat(dirName, &st) == -1){
    send(rqst, &extraFailure, sizeof(extraFailure), 0);
  } 
  else {
    int create = mkdir(dirName, 0700);
    if (create > 0)
      send(rqst, &success, sizeof(success), 0); 
    else 
      send(rqst, &failure, sizeof(failure), 0);
  } 

  return 0;
}

int changeDir(int rqst) {
  return 0;
}

int quit(int rqst) {
  return 0;
}
