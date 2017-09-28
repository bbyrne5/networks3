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

  // send initial message
  if(sendto(s, "Hi", 3, 0, (struct sockaddr *)&sin, addr_len) == -1) {
    perror("Client Send Error!\n");
    exit(1);
  }

  // receive key
  if ((key_len = recvfrom(s, key, sizeof(key), 0, (struct sockaddr *)&sin, &addr_len)) == -1) {
    perror("Client Receive Error!\n");
    exit(1);
  }

  gettimeofday(&start, NULL);
  // send data
  if (sendto(s, buf, strlen(buf)+1, 0, (struct sockaddr *)&sin, addr_len) == -1) {
    perror("Client Send Error!\n");
    exit(1);
  }

  // receive data
  if ((data_len = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&sin, &addr_len)) == -1) {
    perror("Client Receive Error!\n");
    exit(1);
  }
  gettimeofday(&end, NULL);
  buf[data_len] = '\0';

  // decrypt data
  for(i = 0; i < data_len; i++)
    buf[i] = buf[i] ^ key[i % key_len];

  // RTT time  
  uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
  
  printf("%s\nRTT: %lu microseconds\n", buf, delta_us);
  close(s);

  return 0;
}
