/*
 * sender.c
 *
 *  Created on: Sep. 30 2012
 *
 *  Matthew Schleifer
 *  Adam Eggum
 *
 *
 sender -p <port> -g <requester port> -r <rate> -q <seq_no> -l <length>

    port is the port on which the sender waits for requests,
    requester port is the port on which the requester is waiting,
    rate is the number of packets to be sent per second,
    seq_no is the initial sequence of the packet exchange, and
    length is the length of the payload in the packets (each chunk of the file part that the sender has),

 Additional notes for the parameters:

    sender and requester port should be in this range: 1024<port<65536
    for implementing the rate parameter the sending interval should be evenly distributed, i.e. when rate is 10 packets per second the sender
    has to send one packet at about every 100 milliseconds. It should not send them all in a short time and wait for the remaining time in the second.

 The sender must print the following information for each packet sent to the requester, with each packet's information in a separate line.

    The time that the packet was sent with milisecond granularity,
    The IP of the requester,
    The sequence number, and
    The first 4 bytes of the payload
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "packets.h"
#include <arpa/inet.h>

#define BUFFER (512)

//array and array size tracker for global use
tracker_entry* tracker_array;
int tracker_array_size;

void
printError(char* errorMessage) {
  fprintf(stderr, "An error has occured: %s\n", errorMessage);
}

void
usage(char *prog) {
  fprintf(stderr, "usage: %s -p <port> -g <requester port> -r <rate> -q <seq_no> -l <length>\n", prog);
  exit(1);
}

void
printPacketInfo(int requesterIP, int sequenceNumber) {//, char* payload) {
  printf("\n----------------------------\n\nPrinting packet info.\n");
  //TODO: Print some info about the sender
  //TODO: These parameters might not be quite right or we might want
  //	differnt ones/names/etc.
  struct timeb time;
  ftime(&time);
  char timeString[80];
  strftime(timeString, sizeof(timeString), "%H:%M:%S", localtime(&(time.time)));
  printf("TIME: %s:%d\nREQUESTER IP: %i\nSEQUENCE NUBMER: %i\nPAYLOAD: \n", timeString, time.millitm, requesterIP, sequenceNumber);
}

int
main(int argc, char *argv[])
{
  char *buffer;
  buffer = malloc(BUFFER);
  if(buffer == NULL) {
    printError("Buffer could not be allocated");
    return 0;
  }
  if(argc != 11) {
    printError("Incorrect number of arguments");
    usage(argv[0]);
    return 0;
  }
  
  // arguments
  int port  = 0;
  int requesterPort = 0;
  int rate = 0;
  int sequenceNumber = 0;
  int length = 0;

    // input params
  int c;
  opterr = 0;
  while ((c = getopt(argc, argv, "p:g:r:q:l:")) != -1) {
    switch (c) {
    case 'p':
      port = atoi(optarg);
      break;
    case 'g':
      requesterPort = atoi(optarg);
      break;
    case 'r':
      rate = atoi(optarg);
      break;
    case 'q':
      sequenceNumber = atoi(optarg);
      break;
    case 'l':
      length = atoi(optarg);
      break;
    default:
      usage(argv[0]);
    }
  }
  if(port < 1024 || port > 65536) {
    printError("Incorrect port number");
    return 0;
  }
  if(requesterPort < 1024 || requesterPort > 65536) {
    printError("Incorrent requester port number");
    return 0;
  }
  printf(" Port: %i\n Requester Port: %i\n Rate: %i\n Seq_no: %i\n Length: %i\n",
			port, requesterPort, rate, sequenceNumber, length);

  printPacketInfo(requesterPort,sequenceNumber);
  
  struct sockaddr_in si_me, si_other;
  int s, i, slen=sizeof(si_other);
  char buf[BUFFER];
  
  if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    perror("socket");
  
  memset((char *) &si_me, 0, sizeof(si_me));
  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(port);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me))==-1)
    perror("bind");
  
  for (i=0; i<10; i++) {
    if (recvfrom(s, buf, BUFFER, 0, (struct sockaddr *)&si_other, (socklen_t *)&slen)==-1) {
      perror("recvfrom()");
    }
    //printf("Received pkt from %s:%d\nData: %s", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);
    packet PACKET;
    memcpy(&PACKET, buf, sizeof(packet));
    struct timeb time;
    ftime(&time);
    char timeString[80];
    strftime(timeString, sizeof(timeString), "%H:%M:%S", localtime(&(time.time)));
    printf("Received pkt from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
    printf("Packet info: type: %c, sequence: %d, length: %d\n", PACKET.type, PACKET.sequence, PACKET.length);
    printf("pkt received at : %s:%d\n", timeString, time.millitm);
    printf("------------------------------\n");
  }
  
  close(s);
  
  
/*
  // CREATE SOCKET
  int socketFD;
  socketFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // 17 is UDP???
  if(socketFD == -1) {
    perror("socket");
    close(socketFD);
  }
  else {
    printf("Socket created. FD: %i\n", socketFD);
  }

  // BIND SOCKET
  struct sockaddr_in address;

  // type of socket created in socket()
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  // 7000 is the port to use for connections
  address.sin_port = htons(port);
  // bind the socket to the port specified above
  if(bind(socketFD,(struct sockaddr *)&address,sizeof(address)) == -1) {
    perror("bind");
    close(socketFD);
  }
  else {
    printf("Socket bound. FD: %i\n", socketFD);
  }
  int addrLength = sizeof(struct sockaddr_in);

  //Fill out a packet and print it out
  packet PACKET;
  PACKET.type = 'M';
  PACKET.sequence = htonl(sequenceNumber); //convert with htonl and ntohl
  PACKET.length = length;
  printf("PACKET Details: type(%c), sequence(%d), length(%d)\n", PACKET.type, PACKET.sequence, PACKET.length);

  while(1) {
    if(recvfrom(socketFD, buffer, BUFFER, 0, (struct sockaddr *)&address, (socklen_t *) &addrLength) == -1) {
      perror("recvfrom");
      exit(-1);
    }
  }
*/

  return 0;
}
