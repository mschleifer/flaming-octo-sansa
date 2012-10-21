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

#define BUFFER (5120)
#define MAXPACKETSIZE (5137)
#define HEADERSIZE (17)
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
*/
int printInfoAtSend(char* requester_ip, packet pkt) {
  printf("\n");
  struct timeb time;
  ftime(&time);
  char timeString[80];
  strftime(timeString, sizeof(timeString), "%H:%M:%S", localtime(&(time.time)));
  if (pkt.type == 'E') {
    printf("Sending END packet at %s.%d(ms).\n", timeString, time.millitm);
  }
  else {
    printf("Sending packet at: %s.%d(ms).\n\tRequester IP: %s.\n\tSequence number: %d.\n\t",
	   timeString, time.millitm, requester_ip, pkt.sequence);
    printf("First 4 bytes of payload: %c%c%c%c\n", pkt.payload[0], pkt.payload[1], pkt.payload[2], pkt.payload[3]);
  }
  return 0;
}


int sendEndPkt(struct sockaddr_in client, int socketFD_Server) {
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
  
  
  printInfoAtSend(inet_ntoa(client.sin_addr), endPkt);
  if (sendto(socketFD_Server, endPacket, HEADERSIZE+endPkt.length, 0, (struct sockaddr *)&client, sizeof(client))==-1) {
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
	int port  = 0;

	// Port on which the requester is waiting
	int requester_port = 0;

	// The number of packets sent per second
	int rate;

	// The initial sequence of the packet exchange
	int sequence_number;

	// The length of the payload in the packets (each chunk of the file part the sender has)
	int length = 0;
	length = length;

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

	struct sockaddr_in server, client;
	int socketFD_Server, slen=sizeof(client);

	// Create a sockety
	if ((socketFD_Server=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) {
		perror("socket");
		close(socketFD_Server);
		exit(-1);
	}

	// Zero out server socket address and set-up
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	// Bind the socket to the address
	if (bind(socketFD_Server, (struct sockaddr *)&server, sizeof(server))==-1) {
	  perror("bind");
	}

	packet request;
	// Endlessly wait for some kind of a request
	if (recvfrom(socketFD_Server, buffer, MAXPACKETSIZE, 0, (struct sockaddr *)&client, (socklen_t *)&slen)==-1) {
		perror("recvfrom()");
	}
	// Once we receive something, set up info about client
	bzero(&client, sizeof(client));
	client.sin_family = AF_INET;
	client.sin_port = htons(requester_port);
	if (inet_aton(SRV_IP, &client.sin_addr) == 0) {
		fprintf(stderr, "inte_aton() failed\n");
		exit(-1);
	}

	// Create packet from the request
	memcpy(&request.type, buffer, sizeof(char));
	memcpy(&request.sequence, buffer + 1, sizeof(uint32_t));
	memcpy(&request.length, buffer + 9, sizeof(uint32_t));
	request.payload = malloc(request.length);
	memcpy(request.payload, buffer + 17, request.length);


	// Make sure the request packet is formatted correctly
	if (request.type == 'R' && request.sequence == 0) {
		printf("Request packet received: %c %u %u %s\n", request.type, request.sequence, request.length, request.payload);

		// Open the requested file for reading
		int fd;
		if ((fd = open(request.payload, S_IRUSR )) == -1) {
		  perror("open");
		  sendEndPkt(client, socketFD_Server);
		  return -1;
		}
		
		// Read the file into the buffer
		char filePayload[length];
		int bytesRead;
		bzero(filePayload, length);
		while((bytesRead = read(fd, (void*)&filePayload, length)) > 0) {
		  // Set up a response packet
		  packet response;
		  response.type = 'D';
		  response.sequence = sequence_number;
		  response.length = bytesRead;
		  response.payload = filePayload;
		  
		  //serialize response packet
		  char* responsePacket = malloc(HEADERSIZE + response.length);
		  memcpy(responsePacket, &response.type, sizeof(char));
		  memcpy(responsePacket+1, &response.sequence, sizeof(uint32_t));
		  memcpy(responsePacket+9, &response.length, sizeof(uint32_t));
		  memcpy(responsePacket+HEADERSIZE, response.payload, response.length);
		  
		  printInfoAtSend(inet_ntoa(client.sin_addr), response);
		  if (sendto(socketFD_Server, responsePacket, HEADERSIZE+response.length, 0, (struct sockaddr *)&client, sizeof(client))==-1) {
		    perror("sendto()");
		  }
		  
		  sequence_number += response.length; // Increase sequence_number by payload bytes
		  rate = rate;  //TODO: We have to use rate somewhere, this is for the compiler
		  bzero(filePayload, length);
		}
	}
	else { // Request packet didn't have type R && sequence 0
	  printf("Error: Sender received a packet that was not a request.");
	}
	
	sendEndPkt(client, socketFD_Server);
		
	// Close when finished sending; Each send only sends one file
	// It is the receiver's job to handle reliability, but implementing a FIN
	// type scheme would be better
	close(socketFD_Server);
	return 0;
} // End Main

