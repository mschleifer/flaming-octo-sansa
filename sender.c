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
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>

#define BUFFER (5120)
#define MAXPACKETSIZE (5137)
#define HEADERSIZE (17)
#define SRV_IP "127.0.0.1"

void
printError(char* errorMessage) {
	fprintf(stderr, "An error has occured: %s\n", errorMessage);
}

void
usage(char *prog) {
	fprintf(stderr, "usage: %s -p <port> -g <requester port> -r <rate> -q <seq_no> -l <length>\n", prog);
	exit(1);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/**
* Should be called for each packet that is sent to the requester.
* Prints out the time, IP, sequence number and 4 bytes of the payload.
* Does some things a little different if it's an END packet.
*/
int printInfoAtSend(char* requester_ip, packet pkt) {
  printf("\n");
  struct timeb time;
  ftime(&time);
  char timeString[80];
  strftime(timeString, sizeof(timeString), "%H:%M:%S", localtime(&(time.time)));
  
  if (pkt.type == 'E') {
    printf("Sending END packet to requester at ip %s at %s.%d(ms).\n", requester_ip, timeString, time.millitm);
  }
  else {
    printf("Sending packet at: %s.%d(ms).\n\tRequester IP: %s.\n\tSequence number: %d.\n\t",
	   timeString, time.millitm, requester_ip, pkt.sequence);
    printf("First 4 bytes of payload: %c%c%c%c\n", pkt.payload[0], pkt.payload[1], pkt.payload[2], pkt.payload[3]);
  }
  return 0;
}

int sendEndPkt(struct sockaddr_storage client_addr, socklen_t addr_len, int socketFD_Server) {
  // Send the end packet to the requester.  Done sending.
  packet endPkt;
  endPkt.type = 'E';
  endPkt.sequence = 0;
  endPkt.length = 0;
  endPkt.payload = "END.";
  endPkt.length = strlen(endPkt.payload);
  
  char* endPacket = malloc(HEADERSIZE + endPkt.length);
  memcpy(endPacket, &endPkt.type, sizeof(char));
  memcpy(endPacket+1, &endPkt.sequence, sizeof(uint32_t));
  memcpy(endPacket+9, &endPkt.length, sizeof(uint32_t));
  memcpy(endPacket+HEADERSIZE, endPkt.payload, endPkt.length);
  
  	//TODO
	/*inet_ntop(AF_INET,
							 get_in_addr((struct sockaddr*)p->ai_addr),
							 s, sizeof(s)));*/
  //printInfoAtSend(inet_ntoa(client.sin_addr), endPkt);
  //if (sendto(socketFD_Server, endPacket, HEADERSIZE+endPkt.length, 0, (struct sockaddr *)&client, sizeof(client))==-1) {
	if (sendto(socketFD_Server, endPacket, HEADERSIZE+endPkt.length, 0, (struct sockaddr*)&client_addr, addr_len) == -1) {
    perror("sendto()");
  }

  return 0;
}

int
main(int argc, char *argv[])
{
	char *buffer;
	buffer = malloc(MAXPACKETSIZE);
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
	int port;
	char* s_port;

	// Port on which the requester is waiting
	int requester_port;
	char* requester_port_str;

	// The number of packets sent per second
	double rate;

	// The initial sequence of the packet exchange
	int sequence_number;

	// The length of the payload in the packets (each chunk of the file part the sender has)
	int length;

	// Get the commandline args
	int c;
	opterr = 0;
	while ((c = getopt(argc, argv, "p:g:r:q:l:")) != -1) {
		switch (c) {
		case 'p':
			port = atoi(optarg);
			s_port = optarg;
			break;
		case 'g':
			requester_port = atoi(optarg);
			requester_port_str = optarg;
			break;
		case 'r':
			rate = atof(optarg);
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
	
	//char hostname[255];
	//gethostname(hostname, 255);
	
	int socketFD_Server;
	
	struct addrinfo hints, *p, *servinfo;
	int rv, numbytes;
	struct sockaddr_storage client_addr;
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;
	if ( (rv = getaddrinfo(NULL/*"localhost"*//*hostname*/,  s_port, &hints, &servinfo)) != 0) {
	  fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	  return -1;
	}

	// loop through all the results and bind to the first that we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
	  if ((socketFD_Server = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
	    perror("sender: socket");
	    continue;
	  }

	  if (bind(socketFD_Server, p->ai_addr, p->ai_addrlen) == -1) {
	    close(socketFD_Server);
	    perror("sender: bind");
	    continue;
	  }

	  break;
	}

	// Should print out 0.0.0.0
	/*printf("sender: %s\n", inet_ntop(AF_INET,
					get_in_addr((struct sockaddr*)p->ai_addr),
					s, sizeof(s)));*/

	if (p == NULL) {
	  fprintf(stderr, "sender: failed to bind socket\n");
	  return -1;
	}

	freeaddrinfo(servinfo);

	printf("sender: waiting to recvfrom...\n");
	
	addr_len = sizeof(client_addr);
	if ((numbytes = recvfrom(socketFD_Server, buffer, MAXPACKETSIZE, 0,
				 (struct sockaddr*)&client_addr, &addr_len)) == -1) {
	  perror("recvfrom");
	  exit(1);
	}

	

	printf("server: got packet from %s\n", inet_ntop(client_addr.ss_family,
							 get_in_addr((struct sockaddr*)&client_addr),
							 s, sizeof(s)));
	printf("server: packet is %d bytes long\n", numbytes);


	packet request;
	memcpy(&request.type, buffer, sizeof(char));
	memcpy(&request.sequence, buffer + 1, sizeof(uint32_t));
	memcpy(&request.length, buffer + 9, sizeof(uint32_t));
	request.payload = malloc(request.length);
	memcpy(request.payload, buffer + 17, request.length);
	
	if (request.type == 'R' && request.sequence == 0) {
		//printf("request payload: %s\n", request.payload);
		// Open the requested file for reading
		int fd;
		if ((fd = open(request.payload, S_IRUSR )) == -1) {
		perror("open");
		  sendEndPkt(client_addr, addr_len, socketFD_Server);
		  return -1;
		}
		

		char filePayload[length];
		int bytesRead;
		bzero(filePayload, length);
		// Read through the file 'length' bytes at a time
		while((bytesRead = read(fd, (void*)&filePayload, length)) > 0) {
		  // Set up a response(DATA) packet
		  packet response;
		  response.type = 'D';
		  response.sequence = sequence_number;
		  response.length = bytesRead;
		  response.payload = filePayload;
		  
		  // serialize response packet
		  char* responsePacket = malloc(HEADERSIZE + response.length);
		  memcpy(responsePacket, &response.type, sizeof(char));
		  memcpy(responsePacket+1, &response.sequence, sizeof(uint32_t));
		  memcpy(responsePacket+9, &response.length, sizeof(uint32_t));
		  memcpy(responsePacket+HEADERSIZE, response.payload, response.length);
			/*if ((responsePacket[HEADERSIZE + response.length - 1]) == '\n') {
				printf("test");
			}*/
			printf("payload: %s\n", responsePacket+HEADERSIZE);
			printf("length: %d\n", response.length);
		  
		  //printInfoAtSend(inet_ntoa(client.sin_addr), response);
		  //socketFD_Client = socketFD_Client;
		  if (sendto(socketFD_Server, responsePacket, HEADERSIZE+response.length, 0, (struct sockaddr*)&client_addr, addr_len)==-1) {
		    perror("sendto()");
		  }
		  
		  sequence_number += response.length; // Increase sequence_number by payload bytes
  
		  bzero(filePayload, length);

		  // sleep for the given time to adjust for rate
		  usleep(((double)1.0 / rate) * 1000000.0);
		}

		sendEndPkt(client_addr, addr_len, socketFD_Server);
	}

	close(socketFD_Server);
	return 0;

} // End Main

