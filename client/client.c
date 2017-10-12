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
#include <unistd.h>
#include <libgen.h>

#define MAXDATASIZE 4096
#define SMALLSIZE 256

int delete(int s, char * name, char * type);
char * yesOrNo(char * name);
int lengthAndName(int s, char * buf);
int download(int s, char * buf);
int upload(int s, char * buf);
int deleteFile(int s, char * buf);
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
    close(s);
    exit(1);
  }

  int retv = 0;
  while (1) {
    printf("Operation: ");
    fgets(buf, sizeof(buf), stdin);
    buf[MAXDATASIZE-1] = '\0';
    if      (!strncmp(buf,"DWLD\n",5)) retv = download(s, buf);
    else if (!strncmp(buf,"UPLD\n",5)) retv = upload(s, buf);
    else if (!strncmp(buf,"DELF\n",5)) retv = deleteFile(s, buf);
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

  close(s);
  printf("Session closed.\n");

  return retv;
}

int delete(int s, char * name, char * type) {
  short confirm = 0;
  if (recv(s, &confirm, sizeof(confirm), 0) < 0) {
    perror("client: receive error");
    return 1;
  }
  confirm = ntohs(confirm);

  if(confirm < 0) {
    printf("The %s does not exist on server\n", type);
    return 0;
  }

  char * answer = yesOrNo(name);

  if (send(s, answer, strlen(answer)+1, 0) == -1) {
    perror("client: send error");
    return 1;
  }

  if (!strncmp(answer,"No",2)) {
    printf("Delete abandoned by the user!\n");
    return 0;
  }

  if (recv(s, &confirm, sizeof(confirm), 0) < 0) {
    perror("client: receive error");
    return 1;
  }
  confirm = ntohs(confirm); 

  if(confirm < 0)
    printf("Failed to delete %s\n", type);
  else
    printf("Deleted %s\n", type); 

  return 0;
}  

char * yesOrNo(char * name) {

  char buf[4];

  while (1) {
    printf("Are you sure you want to delete %s? (Yes\\No)\n", name);
    fgets(buf, 4, stdin);
    if (!strncmp(buf,"Yes\n",4))
      return("Yes");
    if (!strncmp(buf,"No\n",3))
      return("No");   
  }
}

int lengthAndName(int s, char * buf) {

  short len = strlen(buf);

  // strip newline
  if(buf[len-1] == '\n') {
    buf[len-1] = '\0';
    len--;
  }

  short sendNum = htons(len);
  // send file length
  if (send(s, &sendNum, sizeof(sendNum), 0) == -1 ) {
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
  fgets(buf, SMALLSIZE, stdin);
  buf[SMALLSIZE-1] = '\0';

  if(lengthAndName(s, buf) == 1)
    return 1;

  long receiveNum = 0;
  if (read(s, &receiveNum, sizeof(receiveNum)) == -1) {
    perror("client: receive error");
    return 1;
  }
  long fileLen = ntohl(receiveNum);

  if (fileLen == -1) {
    printf("File %s does not exist on server\n", buf);
    return 0;
  }

  FILE * fp = fopen(basename(buf), "w+");

  int i;
  for(i = 0; i < fileLen; i += MAXDATASIZE){
    if (read(s, buf, MAXDATASIZE) == -1) {
      perror("client: receive error");
      return 1;
    }
    if (fileLen - i < MAXDATASIZE)
      fwrite(buf, sizeof(char), fileLen-i, fp);
    else
      fwrite(buf, sizeof(char), MAXDATASIZE, fp);
  }

  fclose(fp);
  return 0;
}

int upload(int s, char * buf) {
  if (send(s, "UPLD", 5, 0) == -1) {
    perror("client: send error");
    return 1;
  }

  printf("File name: ");
  fgets(buf, SMALLSIZE, stdin);

  buf[strlen(buf)-1] = '\0';

  FILE * fp;
  long fileLength;
  if(access(buf, F_OK) == -1){//file doesn't exist
    printf("File %s does not exist on client\n", buf);
    return 0;
  }

  if(lengthAndName(s, buf) == 1)
    return 1;

  fp = fopen(buf, "r");
  fseek(fp, 0, SEEK_END);
  fileLength = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char ack[6];
  // get acknowledgement
  if (read(s, &ack, 6) == -1) {
    perror("client: receive error");
    return 1;
  }

  long convertedLength = htonl(fileLength);
  // send length
  if (send(s, &convertedLength, sizeof(convertedLength), 0) < 0 ) {
    perror("client: send error");
    return 1;
  }

  int bytes;
  while((bytes = fread(buf, sizeof(char), MAXDATASIZE, fp)) > 0){
    if(send(s, buf, bytes, 0) < 0){
      perror("client: send error");
      return 1;
    }
  }

  fclose(fp);
  return 0;
}

int deleteFile(int s, char * buf) {
  if (send(s, "DELF", 5, 0) == -1) {
    perror("client: send error");
    return 1;
  }

  printf("File name: ");
  fgets(buf, SMALLSIZE, stdin);
  buf[SMALLSIZE-1] = '\0';

  if(lengthAndName(s, buf) == 1)
    return 1;
 
  delete(s, buf, "file");
  return 0;
}

int list(int s, char * buf) {
  if (send(s, "LIST", 5, 0) == -1) {
    perror("client: send error");
    return 1;
  }

  long receiveNum = 0;
  if (recv(s, &receiveNum, sizeof(receiveNum), 0) < 0) {
    perror("client: receive error");
    return 1;
  }
  int size = ntohl(receiveNum);

  if (recv(s, buf, size, 0) < 0) {
    perror("client: receive error");
    return 1;
  }

  printf("%s\n", buf);

  return 0;
}

int makeDir(int s, char * buf) {
  if (send(s, "MDIR", 5, 0) == -1) {
    perror("client: send error");
    return 1;
  }

  printf("Directory name: ");
  fgets(buf, SMALLSIZE, stdin);
  buf[SMALLSIZE-1] = '\0';

  if(lengthAndName(s, buf) == 1)
    return 1;

  short receiveNum = 0;
  if (recv(s, &receiveNum, sizeof(receiveNum), 0) < 0) {
    perror("client: receive error");
    return 1;
  }

  receiveNum = ntohs(receiveNum);
  if (receiveNum == -2)
    printf("The directory already exists on server\n");
  else if (receiveNum == -1)
    printf("Error in making directory\n");
  else if(receiveNum > 0)
    printf("The directory was successfully made\n");

  return 0;
}

int removeDir(int s, char * buf) {
  if (send(s, "RDIR", 5, 0) == -1) {
    perror("client: send error");
    return 1;
  }

  printf("Directory name: ");
  fgets(buf, SMALLSIZE, stdin);
  buf[SMALLSIZE-1] = '\0';

  if(lengthAndName(s, buf) == 1)
    return 1;

  delete(s, buf, "directory");

  return 0;
}

int changeDir(int s, char * buf) {
  if (send(s, "CDIR", 5, 0) == -1) {
    perror("client: send error");
    return 1;
  }

  printf("Directory name: ");
  fgets(buf, SMALLSIZE, stdin);
  buf[SMALLSIZE-1] = '\0';

  if(lengthAndName(s, buf) == 1)
    return 1;

  short receiveNum = 0;
  if (recv(s, &receiveNum, sizeof(receiveNum), 0) < 0) {
    perror("client: receive error");
    return 1;
  }

  receiveNum = ntohs(receiveNum);
  if (receiveNum == -2)
    printf("The directory does not exist on server\n");
  else if (receiveNum == -1)
    printf("Error in changing directory\n");
  else if(receiveNum > 0)
    printf("Changed current directory\n");

  return 0;
}

int quit(int s) {
  if (send(s, "QUIT", 5, 0) == -1) {
    perror("client: send error");
    return 1;
  }

  return 0;
}
