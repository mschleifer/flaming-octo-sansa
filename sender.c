#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "structs.h"
#include "helpers.h"
#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>


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
  // Send the end packet to the requester.  Sender is done sending.
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
	if(argc != 19) {
		printError("Incorrect number of arguments");
		usage(argv[0]);
		return 0;
	}
	
	
	int port; // Port on which the sender waits for requests
	char* s_port;

	int requester_port; // Port on which the requester is waiting
	char* requester_port_str;

	double rate;  // The number of packets sent per second
	int sequence_number; // The initial sequence of the packet exchange
	int length;  // The length of the payload in the packets (each chunk of the file part the sender has)
	char* f_hostname;			//hostname of emulator
	char* f_port;					//post of emulator
	uint8_t priority;			//priority of sent packets
	int timeout;					//timeout for retransmission
	
	// Get the commandline args
	int c;
	opterr = 0;
	while ((c = getopt(argc, argv, "p:g:r:q:l:f:h:i:t:")) != -1) {
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
			//sequence_number = atoi(optarg);
			sequence_number = 1;
			break;
		case 'l':
			length = atoi(optarg);
			break;
		case 'f':
			f_hostname = optarg;
			break;
		case 'h':
			f_port = optarg;
			break;
		case 'i':
			priority = atoi(optarg);
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		default:
			usage(argv[0]);
		}
	}
	
	// Checking for valid port numbers; not doing this for new additions
	if( (port < 1024 || port > 65536) || (requester_port < 1024 || requester_port > 65536) ) {
		printError("Incorrect port number(s).  Ports should be in range (1024 - 65536)");
		return 0;
	}
	
	int socketFD_Server;

	struct addrinfo hints, *p, *servinfo;
	int rv, numbytes;
	struct sockaddr_storage client_addr;
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];
	
	// Used to bind to the port on this host as a 'server'
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;
	if ( (rv = getaddrinfo(NULL, s_port, &hints, &servinfo)) != 0) {
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

	if (p == NULL) {
	  fprintf(stderr, "sender: failed to bind socket\n");
	  return -1;
	}

	freeaddrinfo(servinfo);

	
	addr_len = sizeof(client_addr);
	if ((numbytes = recvfrom(socketFD_Server, buffer, MAXPACKETSIZE, 0,
				 (struct sockaddr*)&client_addr, &addr_len)) == -1) {
	  perror("recvfrom");
	  exit(1);
	}

	
	printf("server: got packet from %s\n", inet_ntop(client_addr.ss_family,
							 get_in_addr((struct sockaddr*)&client_addr),
							 s, sizeof(s)));
	//printf("server: packet is %d bytes long\n", numbytes);


	packet request;
	memcpy(&request.type, buffer, sizeof(char));
	memcpy(&request.sequence, buffer + 1, sizeof(uint32_t));
	memcpy(&request.length, buffer + 9, sizeof(uint32_t));
	request.payload = malloc(request.length);
	memcpy(request.payload, buffer + 17, request.length);
	
	if (request.type == 'R' && request.sequence == 0) {
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
		  
	
			//printf("port: %d\n", ntohs( ((struct sockaddr_in*)&client_addr)->sin_port ));
		  printInfoAtSend(inet_ntoa( ((struct sockaddr_in*)&client_addr)->sin_addr ), response);


			// Send data to the requester
		  if ( sendto(socketFD_Server, responsePacket, HEADERSIZE+response.length, 0, 
								(struct sockaddr*) &client_addr, addr_len) ==-1 ) {
		    perror("sendto()");
		  }
		  
			sequence_number++;
		  //sequence_number += response.length; // Increase sequence_number by payload bytes
		  bzero(filePayload, length);

		  // sleep for the given time to adjust for rate
		  usleep(((double)1.0 / rate) * 1000000.0);
		}

		sendEndPkt(client_addr, addr_len, socketFD_Server);
	}

	close(socketFD_Server);
	return 0;

} // End Main

