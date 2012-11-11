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


const char*
get_ip_address(struct sockaddr* addr) {
	char s[INET6_ADDRSTRLEN];
	char* n = "";
	printf("%s", n);
	return inet_ntop( AF_INET, get_in_addr(addr), s, sizeof(s) ); 
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
	// Send the end packet to the requester.  Sender is done sending.
	packet endPkt;
	// TODO: fill in other members of packet
	
	endPkt.type = 'E';
	endPkt.sequence = 0;
	endPkt.length = 0;
	endPkt.payload = "END.";
	endPkt.length = strlen(endPkt.payload);
  
	char* endPacketBuffer = malloc(P2_HEADERSIZE + HEADERSIZE + endPkt.length);
	serializePacket(endPkt, endPacketBuffer);
  
	if (sendto(socketFD_Server, endPacketBuffer, P2_HEADERSIZE+HEADERSIZE+endPkt.length,
			0, (struct sockaddr*)&client_addr, addr_len) == -1) {
		perror("sendto()");
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	char *buffer;
	buffer = malloc(P2_MAXPACKETSIZE);
	if(buffer == NULL) {
		printError("Buffer could not be allocated");
		return 0;
	}
	if(argc != 19) {
		printError("Incorrect number of arguments");
		usage_Sender(argv[0]);
		return 0;
	}
	
	
	int port; 				// Port on which the sender waits for requests
	char* s_port;

	int requester_port; 	// Port on which the requester is waiting
	char* requester_port_str;

	double rate;  			// The number of packets sent per second
	int sequence_number; 	// The initial sequence of the packet exchange
	int length;  			// The length of the payload in the packets
	char* f_hostname;		// hostname of emulator
	char* f_port;			// post of emulator
	uint8_t priority;		// priority of sent packets
	int timeout;			// timeout for retransmission
	
	int i, j, k; 			// Loop counters
	int packetsRead;
	int ACKCounter;
	packet ACKPacket;
	time_t currentTime;
	
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
			sequence_number = 1; // Always start at 1 for P2
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
			usage_Sender(argv[0]);
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
	//char s[INET6_ADDRSTRLEN];
	
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

	// Wait to receive a request
	addr_len = sizeof(client_addr);
	if ((numbytes = recvfrom(socketFD_Server, buffer, P2_MAXPACKETSIZE, 0,
				 (struct sockaddr*)&client_addr, &addr_len)) == -1) {
	  perror("recvfrom");
	  exit(1);
	}

	printf( "server: got packet from %s\n", get_ip_address((struct sockaddr*) &client_addr) ); 
	printf("server: packet is %d bytes long\n", numbytes);
	
	// Construct a packet from the data recv'd
	packet request;
	request = getPktFromBuffer(buffer);
	
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
		int window_size = request.new_length;
		bool readComplete = false;
		bool allACKReceived = false;
		
		senderPacketNode senderPacketNodeArray[window_size];
		char * senderDataBufferArray[window_size];
		// Create array of pointers each pointing to 'length' worth of free space
		for(i = 0; i < window_size; i ++) {
			senderDataBufferArray[i] = malloc(length);
			bzero(senderDataBufferArray[i], length);
		}

		while(!readComplete) {

			for(j = 0; j < window_size; j++) {
				if((bytesRead = read(fd, (void*)&filePayload, length)) > 0) {
					// Copy the data we read into the senderDataBufferArray
					memcpy(senderDataBufferArray[j], &filePayload, length);
			
					// Set up packet
					//TODO: setup P2 packet members
					senderPacketNodeArray[j].packet.type = 'D';
					senderPacketNodeArray[j].packet.sequence = sequence_number;
					senderPacketNodeArray[j].packet.length = bytesRead;
					senderPacketNodeArray[j].packet.payload = senderDataBufferArray[j];
			
					// Reset the senderPacketNode
					senderPacketNodeArray[j].timeSent = 0;
					senderPacketNodeArray[j].ACKReceived = false;
					senderPacketNodeArray[j].retryCount = 0;
			
					sequence_number++;
					bzero(filePayload, length);
				}
				else {
					readComplete = true;
					break;
				}
			}
			packetsRead = j;
		
			for(k = 0; k < packetsRead; k++) {
	
				// serialize response packet
				char* responsePacket = malloc(P2_HEADERSIZE+HEADERSIZE+senderPacketNodeArray[k].packet.length);
				serializePacket(senderPacketNodeArray[k].packet, responsePacket);
		  

				//printf("port: %d\n", ntohs( ((struct sockaddr_in*)&client_addr)->sin_port ));
				printInfoAtSend(inet_ntoa( ((struct sockaddr_in*)&client_addr)->sin_addr ), senderPacketNodeArray[k].packet);

				// Send data to the requester
				if (sendto(socketFD_Server, responsePacket,
						P2_HEADERSIZE+HEADERSIZE+senderPacketNodeArray[k].packet.length,
						0, (struct sockaddr*) &client_addr, addr_len) == -1) {
					perror("sendto()");
				}

				// Initialize timeout timer
				senderPacketNodeArray[k].timeSent = time(NULL);

				// sleep for the given time to adjust for rate
				usleep(((double)1.0 / rate) * 1000000.0);
		
				// We'd leak memory like crazy otherwise
				free(responsePacket);
			}
	
			allACKReceived = false;
	
			// TODO: Need to set up a non-blocking socket
			// TODO: does this work??? I dont know...
			fcntl(socketFD_Server, F_SETFL, O_NONBLOCK);
	
			while(!allACKReceived) {
				if ((numbytes = recvfrom(socketFD_Server, buffer, P2_MAXPACKETSIZE, 0,
						 (struct sockaddr*)&client_addr, &addr_len)) == -1) {
					// Don't do anything b/c we're non-blocking.
					//perror("recvfrom");
					//exit(1);
				}
				if(numbytes > 0) {
					ACKPacket = getPktFromBuffer(buffer);
					if(ACKPacket.type == 'A')  {
						for(k = 0; k < packetsRead; k++) {
							if(senderPacketNodeArray[k].packet.sequence == ACKPacket.sequence) {
								senderPacketNodeArray[k].ACKReceived = true;
								printf("Received ACK for packet %d\n", senderPacketNodeArray[k].packet.sequence);
							}
						}
					}
					else {
						printf("\nERROR: sender got a packet that's not an ACK\n");
					}
				}

				ACKCounter = 0;
				for(k = 0; k < packetsRead; k++) {
					if(senderPacketNodeArray[k].ACKReceived) {
						ACKCounter++;
					}
					else if(senderPacketNodeArray[k].timeSent + ((senderPacketNodeArray[k].retryCount + 1)*(timeout/1000)) < (currentTime = time(NULL))) {
						printf("TIMEOUTEXPIRED\n");				
						if(senderPacketNodeArray[k].retryCount < 5) {
							char* responsePacket = malloc(P2_HEADERSIZE+HEADERSIZE+senderPacketNodeArray[k].packet.length);
							serializePacket(senderPacketNodeArray[k].packet, responsePacket);
					
							if (sendto(socketFD_Server, responsePacket,
									P2_HEADERSIZE+HEADERSIZE+senderPacketNodeArray[k].packet.length,
									0, (struct sockaddr*) &client_addr, addr_len) == -1) {
								perror("sendto()");
							}
		
							senderPacketNodeArray[k].retryCount++;
							printf("RESENDING packet %d, attempt number %d\n",senderPacketNodeArray[k].packet.sequence, senderPacketNodeArray[k].retryCount);
					
							// I'm suddenly concerned with memory leaks
							free(responsePacket);
						}
						else {
							printf("Gave up on sending packet with sequence number: %d\n",
									senderPacketNodeArray[k].packet.sequence);
							senderPacketNodeArray[k].ACKReceived = true; //So we know not to check anymore
						}
					}
				}
				if(ACKCounter == packetsRead) {
					allACKReceived = true;
				}
			} // END while(!allACKReceived)
		} // END while(!readComplete)

		sendEndPkt(client_addr, addr_len, socketFD_Server);
	}

	close(socketFD_Server);
	return 0;

} // End Main

