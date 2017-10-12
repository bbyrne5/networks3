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
#include <libgen.h>

#define MAXDATASIZE 4096
#define SMALLSIZE 256

short nameLen(int);
int download(int);
int upload(int);
int delete(int);
int list(int);
int makeDir(int);
int removeDir(int);
int changeDir(int);

int main(int argc, char * argv[] )
{
  struct sockaddr_in sin, client_addr;
  int rqst;
  char buf[MAXDATASIZE];
  int port, s;
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
    shutdown(s, SHUT_RDWR);
    exit(1);
  }

  addr_len = sizeof(client_addr);
  
  if (listen(s, 5) < 0) {
    perror("listen failed");
    close(s);
    exit(1);
  }
 
  int retv; 
  while (1){
    if ((rqst = accept(s, (struct sockaddr *)&client_addr, &addr_len)) < 0){
        perror("server: accept failed");
        close(s);
        exit(1);
    }

    while(1) {
      bzero((char*)&buf, sizeof(buf));

      while(buf[0]== '\0') {
        if(read(rqst, buf, SMALLSIZE) == -1) {
          perror("server: receive failed");
          close(rqst);
          exit(1);
        }
      }

      if (!strncmp(buf,"DWLD",4)) retv = download(rqst);
      else if (!strncmp(buf,"UPLD",4)) retv = upload(rqst);
      else if (!strncmp(buf,"DELF",4)) retv = delete(rqst);
      else if (!strncmp(buf,"LIST",4)) retv = list(rqst);
      else if (!strncmp(buf,"MDIR",4)) retv = makeDir(rqst);
      else if (!strncmp(buf,"RDIR",4)) retv = removeDir(rqst);
      else if (!strncmp(buf,"CDIR",4)) retv = changeDir(rqst);
      else if (!strncmp(buf,"QUIT",4)) break;
      else
        printf("Unrecognized command %s.\n", buf);

      if (retv == 1){
        printf("error caused: closing connection\n");
        break;
      }
    }
    close(rqst);
  }

  return 0;
}

short nameLen(int rqst) {
  //get name length
  short nameLength = 0;
  if(read(rqst, &nameLength, sizeof(nameLength)) == -1) {
    perror("server: read error");
    return -1;
  }
  return ntohs(nameLength);
}


int download(int rqst) {

  short nameLength;
  if((nameLength = nameLen(rqst)) < 0)
    return 1;

  //get the name
  char fileName[nameLength+1];
  if(read(rqst, fileName, nameLength) == -1) {
    perror("server: read error");
    return 1;
  }
  fileName[nameLength] = '\0';

  uint32_t fileLength = 0;
  FILE * fp;
  //check for file
  if(access(fileName, F_OK) != -1){//file does exist
    fp = fopen(fileName, "r");
    fseek(fp, 0, SEEK_END);
    fileLength = ftell(fp);
    fseek(fp, 0, SEEK_SET);
  }
  else {//file does not exist
    fileLength = -1;
  }

  uint32_t convertedLength = htonl(fileLength); 
  // send length
  if (send(rqst, &convertedLength, sizeof(convertedLength), 0) < 0 ) {
    perror("server: send error");
    return 1;
  }

  if(fileLength == -1){
    return 0;
  }

  // send file
  char buf[MAXDATASIZE];
  int bytes;
  while((bytes = fread(buf, sizeof(char), MAXDATASIZE, fp)) > 0){
    if(send(rqst, buf, bytes, 0) < 0){
      perror("server: send error");
      return 1;
    } 
  }

  fclose(fp);
  return 0;
}

int upload(int rqst) {
  short nameLength;
  if((nameLength = nameLen(rqst)) < 0)
    return 1;

  //get the name
  char fileName[nameLength+1];
  if(read(rqst, fileName, nameLength) == -1) {
    perror("server: read error");
    return 1;
  }
  fileName[nameLength] = '\0';
  
  if(send(rqst, "READY", 6, 0) < 0){
    perror("server: send error");
    return 1;
  }
  
  uint32_t len = 0;
  while(!len) {
    if (read(rqst, &len, sizeof(len)) == -1) {
      perror("server: receive error");
      return 1;
    }
  }
  len = ntohl(len);

  // open/create file
  FILE * fp = fopen(basename(fileName), "w+");
  int i;
  char buf[MAXDATASIZE];
  // read file
  for(i = 0; i < len; i += MAXDATASIZE){
    if (read(rqst, buf, MAXDATASIZE) < 0) {
      perror("server: receive error");
      return 1;
    }
    if (len - i < MAXDATASIZE)
      fwrite(buf, sizeof(char), len-i, fp);
    else
      fwrite(buf, sizeof(char), MAXDATASIZE, fp);
  }

  fclose(fp);
  return 0;
}

int delete(int rqst) {

  char buf[SMALLSIZE];
  buf[0] = '\0';

  short nameLength;
  if((nameLength = nameLen(rqst)) < 0)
    return 1;

  //get the name
  char dirName[nameLength+1];
  if(read(rqst, dirName, nameLength) == -1) {
    perror("server: read error");
    return 1;
  }
  dirName[nameLength] = '\0';
  
  //check for file
  short success = htons(1);
  short failure = htons(-1);
  if(access(dirName, F_OK) != -1){//file does exist
    if(send(rqst, &success, sizeof(success), 0) < 0) {
      perror("server: send error");
      return 1;
    }
  }
  else { // doesn't exist
    if(send(rqst, &failure, sizeof(failure), 0) < 0) {
      perror("server: send error");
      return 1;
    }

    return 0;
  }
  
  // get Yes or No
  while(buf[0] == '\0')
    if(read(rqst, buf, SMALLSIZE) < 0) {
      perror("server: read error");
      return 1;
    }

  if(strncmp(buf, "Yes", 3) == 0){
    // delete
    int result = unlink(dirName);
    if(result == 0) {
      if(send(rqst, &success, sizeof(success), 0) < 0) {
        perror("server: send error");
        return 1;
      }
    }
    else {
      if(send(rqst, &failure, sizeof(failure), 0) < 0) {
        perror("server: send error");
        return 1;
      }
    }
  }

  return 0;
}

int list(int rqst) {
  // list contents
  FILE* p = popen("ls -la", "r");
  if (!p) return 1;
  char buff[MAXDATASIZE];
  buff[0] = '\0';
  // put listing in buffer
  while (fgets(buff+strlen(buff), sizeof(buff), p)) {
  }
  pclose(p);
  int size = strlen(buff);
  int tmp = htonl((uint32_t)size);
  if(send(rqst, &tmp, sizeof(tmp), 0) < 0){
    perror("server: send error:");
    return 1;
  }
  if(send(rqst, buff, strlen(buff), 0) < 0){
    perror("server: send error:");
    return 1;
  }
  return 0;
}

int makeDir(int rqst) {
  short nameLength;
  if((nameLength = nameLen(rqst)) < 0)
    return 1;

  //get the name
  char dirName[nameLength+1];
  if(read(rqst, dirName, nameLength) == -1) {
    perror("server: read error");
    return 1;
  }
  dirName[nameLength] = '\0';
  
  struct stat st = {0};
  //check if dir exists and create it if not
  short success = htons(1);
  short failure = htons(-1);
  short extraFailure = htons(-2);

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
  char buf[SMALLSIZE];
  buf[0] = '\0';

  short nameLength;
  if((nameLength = nameLen(rqst)) < 0)
    return 1;

  //get the name
  char dirName[nameLength+1];
  if(read(rqst, dirName, nameLength) == -1) {
    perror("server: read error");
    return 1;
  }
  dirName[nameLength] = '\0';

  struct stat st = {0};
  //check for file
  short success = htons(1);
  short failure = htons(-1);
  if (stat(dirName, &st) == 0 && S_ISDIR(st.st_mode)) {
    if(send(rqst, &success, sizeof(success), 0) < 0) {
      perror("server: send error");
      return 1;
    }
  }
  else {
    if(send(rqst, &failure, sizeof(failure), 0) < 0) {
      perror("server: send error");
      return 1;
    }

    return 0;
  }
 
  // get Yes or No 
  while(buf[0] == '\0')
    if(read(rqst, buf, SMALLSIZE) < 0) {
      perror("server: read error");
      return 1;
    }

  if(strncmp(buf, "Yes", 3) == 0){
    // remove if empty
    int result = rmdir(dirName);
    if(result == 0) {
      if(send(rqst, &success, sizeof(success), 0) < 0) {
        perror("server: send error");
        return 1;
      }
    }
    else {
      if(send(rqst, &failure, sizeof(failure), 0) < 0) {
        perror("server: send error");
        return 1;
      }
    }
  }

  return 0;
}

int changeDir(int rqst) {
  short nameLength;
  if((nameLength = nameLen(rqst)) < 0)
    return 1;

  //get the name
  char dirName[nameLength+1];
  if(read(rqst, dirName, nameLength) == -1) {
    perror("server: read error");
    return 1;
  }
  dirName[nameLength] = '\0';

  struct stat st = {0};
  //check for file
  short success = htons(1);
  short failure = htons(-1);
  short failTwo = htons(-2);
  if (stat(dirName, &st) != 0 || !S_ISDIR(st.st_mode)) {
    if(send(rqst, &failTwo, sizeof(failTwo), 0) < 0) {
      perror("server: send error");
      return 1;
    }
  }

  // attempt to change directory
  int result = chdir(dirName);
  if(result == 0) {
    if(send(rqst, &success, sizeof(success), 0) < 0) {
      perror("server: send error");
      return 1;
    }
  }
  else {
    if(send(rqst, &failure, sizeof(failure), 0) < 0) {
      perror("server: send error");
      return 1;
    }
  }

  return 0;
}

