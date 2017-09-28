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

  while (1){
    if (recvfrom(s, buf, sizeof(buf), 0,  (struct sockaddr *)&client_addr, &addr_len)==-1){
      perror("Server Receive Error!\n");
      exit(1);
    }
    bzero((char*)&buf, sizeof(buf));

    if (sendto(s, key, strlen(key), 0, (struct sockaddr *)&client_addr, addr_len) == -1) {
      perror("Server Send Error!\n");
      exit(1);
    }

    if (recvfrom(s, buf, sizeof(buf), 0,  (struct sockaddr *)&client_addr, &addr_len)==-1){
      perror("Server Receive Error!\n");
      exit(1);
    }

    gettimeofday( &tv, NULL );
    info = localtime( &tv.tv_sec );

    sprintf(buf, "%s Timestamp: %02d:%02d:%02d.%d", buf, info->tm_hour, info->tm_min, info->tm_sec, (int)tv.tv_usec );
    len = strlen(buf);
   
    int i;
    // encrypt data
    for(i = 0; i < len; i++)
      buf[i] = buf[i] ^ key[i % strlen(key)];

    // send encrypted message
    if (sendto(s, buf, len, 0, (struct sockaddr *)&client_addr, addr_len) == -1) {
      perror("Server Send Error!\n");
      exit(1);
    }
  }

  close(s);

  return 0;
}
