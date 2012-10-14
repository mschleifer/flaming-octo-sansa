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
#include <stdbool.h>

#define BUFFER (512)
#define SRV_IP "127.0.0.1"

//array and array size tracker for global use
//tracker_entry* tracker_array;
//int tracker_array_size;

void
printError(char* errorMessage) {
  fprintf(stderr, "An error has occured: %s\n", errorMessage);
}

void
usage(char *prog) {
  fprintf(stderr, "usage: %s -p <port> -g <requester port> -r <rate> -q <seq_no> -l <length>\n", prog);
  exit(1);
}

/**
 * Should be called for each packet that is sent to the requester.  
 * Prints out the time, IP, sequence number and 4 bytes of the payload.
 * TODO: We need to print a section of the payload
 */
int printInfoAtSend(int requester_ip, packet pkt) {
  struct timeb time;
  ftime(&time);
  char timeString[80];
  strftime(timeString, sizeof(timeString), "%H:%M:%S", localtime(&(time.time)));
  printf("Sending packet at: %s.%d(ms).  Requester IP: %d.  Sequence number: %d.  Payload: (null)\n",
	 timeString, time.millitm, requester_ip, pkt.sequence);
  return 0;
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
  
  // Port on which the sender waits for requests
  int port  = 0;

  // Port on which the requester is waiting
  int requester_port = 0;

  // The number of packets sent per second
  int rate;

  // The initial sequence of the packet exchange
  int sequence_number;

  // The length of the payload in the packets (each chunk of the file part the sender has)
  int length;

    
  int c;
  opterr = 0;
  while ((c = getopt(argc, argv, "p:g:r:q:l:")) != -1) {
    switch (c) {
    case 'p':
      port = atoi(optarg);
      break;
    case 'g':
      requester_port = atoi(optarg);
      break;
    case 'r':
      rate = atoi(optarg);
      break;
    case 'q':
      sequence_number = atoi(optarg);
      break;
    case 'l':
      length = atoi(optarg);
      break;
    default:
      usage(argv[0]);
    }
  }

  if( (port < 1024 || port > 65536) || (requester_port < 1024 || requester_port > 65536) ) {
    printError("Incorrect port number(s).  Ports should be in range (1024 - 65536)");
    return 0;
  }

  
  //printf(" Port: %i\n Requester Port: %i\n Rate: %i\n Seq_no: %i\n Length: %i\n",port, requester_port, rate, sequence_number, length);

  
  struct sockaddr_in si_me, si_other;
  int s, slen=sizeof(si_other);
  char buf[BUFFER];
  
  if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    perror("socket");
  
  memset((char *) &si_me, 0, sizeof(si_me));
  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(port);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me))==-1)
    perror("bind");
  

  while (1) {
    if (recvfrom(s, buf, BUFFER, 0, (struct sockaddr *)&si_other, (socklen_t *)&slen)==-1) {
      perror("recvfrom()");
    }

    printf("Buffer received (filename request): %s\n", buf);
    
    int socketFD_Client;
    socketFD_Client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // 17 is UDP???
    if(socketFD_Client == -1) {
      perror("socket");
      close(socketFD_Client);
    }
    
    
    if(strcmp(buf, "split.txt") == 0) {
      struct sockaddr_in address_server;
      
      memset((char *) &address_server, 0, sizeof(address_server));
      address_server.sin_family = AF_INET;
      address_server.sin_port = htons(requester_port);
      if (inet_aton(SRV_IP, &address_server.sin_addr)==0) {
	fprintf(stderr, "inet_aton() failed\n");
	exit(1);
      }
      int j;
      for (j=0; j<1; j++) {
	rate = rate;  //TODO: We have to use rate somewhere, this is for the compiler
	packet PACKET;
	PACKET.type = 'D';
	PACKET.sequence = sequence_number;
	PACKET.length = length;
	memcpy(buffer, &PACKET, sizeof(packet));
	
	// Wait for the requester to get set up.. not sure we want to do this but currently have to
	sleep(1);  
      
	// Print info and then send the packet to the requester
	printInfoAtSend(requester_port, PACKET);
	if (sendto(socketFD_Client, buffer, BUFFER, 0, (struct sockaddr *)&address_server, sizeof(address_server))==-1)
	  perror("sendto()");
      }
    }
    close(socketFD_Client);
  }
  
  //close:
  close(s);

  /*int socketFD_Client;
  socketFD_Client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // 17 is UDP???
  if(socketFD_Client == -1) {
    perror("socket");
    close(socketFD_Client);
  }

  for(i = 0; i < tracker_array_size; i++) {
    if(strcmp(tracker_array[i].file_name, "split.txt") == 0) {
      struct sockaddr_in address_server;
      
      memset((char *) &address_server, 0, sizeof(address_server));
      address_server.sin_family = AF_INET;
      address_server.sin_port = htons(tracker_array[i].sender_port);
      if (inet_aton(SRV_IP, &address_server.sin_addr)==0) {
	fprintf(stderr, "inet_aton() failed\n");
	exit(1);
      }
      int j;
      for (j=0; j<1; j++) {
	packet PACKET;
	PACKET.type = 'D';
	PACKET.sequence = 1;
	PACKET.length = 1;
	memcpy(buffer, &PACKET, sizeof(packet));
	
	// Print info and then send the packet to the requester
	printInfoAtSend(10, PACKET);
	if (sendto(socketFD_Client, buffer, BUFFER, 0, (struct sockaddr *)&address_server, sizeof(address_server))==-1)
	  perror("sendto()");
      }
    }
  }
  
  close(socketFD_Client);  */
  return 0;
}
