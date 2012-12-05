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
#include "queue.h"
#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>

#define MAXLINE (440)

// Global Variables
bool debug = false;

void readTopology(const char* filename) {

	char* line = (char*)malloc(MAXLINE);
	FILE *file = fopen(filename, "r"); //read only
	if (file == NULL) {
		fprintf(stderr, "Cannot open input file.\n");
		exit(1);
	}
	
	char** saveptr = NULL;
	char* token = NULL;
	char* address = NULL;
	int port = -1;
	
	while ( fgets ( line, MAXLINE, file ) != NULL ) // Read in a line
	{
		if(debug)
			printf("LINE: %s", line);
		
		// Parse the line one token (<addr,port> pair) at a time
		saveptr = &line;
		while((token = strtok_r(NULL, " \n", saveptr)) != NULL) {
			address = strtok(token, ",");
			port = atoi(strtok(NULL, " \n"));
			//TODO: want to put these in some kind of structure
			
			if(debug) {
				printf("ADDRESS: %s | ", address);
				printf("PORT: %d\n", port);
			}
		}
	}
	
	fclose(file);
}

int main(int argc, char *argv[]) {
	char *buffer;
	buffer = (char*)malloc(P2_MAXPACKETSIZE);
	if(buffer == NULL) {
		printError("Buffer could not be allocated");
		return 0;
	}
	if(argc != 5 && argc != 7) {
		printError("Incorrect number of arguments");
		usage_Emulator(argv[0]);
		return 0;
	}

	char* port = NULL;					//port of emulator
	char* filename = NULL;				//name of file with forwarding table

	// Get the commandline args
	int c;
	opterr = 0;
	
	while ((c = getopt(argc, argv, "p:f:d:")) != -1) {
		switch (c) {
		case 'p':
			port = optarg;
			break;
		case 'f':
			filename = optarg;
			break;
		case 'd':
			if(atoi(optarg) > 0) {
				debug = true;
			}
			break;
		default:
			usage_Emulator(argv[0]);
		}
	}
	if(debug)
		printf("ARGS: %s %s %s\n", port, filename, debug ? "True" : "False");
	
	readTopology(filename); // Read the topology file
	
	
	

/*	
	char hostname[255];
	gethostname(hostname, 255);
	readForwardingTable(filename, hostname, port);


	int socketFD_Emulator;

	struct addrinfo hints, *p, *servinfo;
	int rv, numbytes;
	struct sockaddr_storage addr;
	socklen_t addr_len;
	// char s[INET6_ADDRSTRLEN];
	
	// Used to bind to the port on this host as a 'server'
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;
	
	// PASSIVE and NULL, so we are open to all connections on port
	if ( (rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	

	
	// loop through all the results and bind to the first that we can (NON BLOCKING)
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((socketFD_Emulator = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("sender: socket");
			continue;
		}
		
		fcntl(socketFD_Emulator, F_SETFL, O_NONBLOCK);
		if (bind(socketFD_Emulator, p->ai_addr, p->ai_addrlen) == -1) {
			close(socketFD_Emulator);
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

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(socketFD_Emulator, &readfds);
	bool packet_delayed = false;
	
	
	while (true) {
		addr_len = sizeof(addr);

		bzero(buffer, P2_MAXPACKETSIZE); // Need to zero the buffer
		// If no data is being sent to us, determine if packets need to be dealt with
		if ((numbytes = recvfrom(socketFD_Emulator, buffer, P2_MAXPACKETSIZE, 0, 
						(struct sockaddr*)&addr, &addr_len)) <= -1) {
			
			// Deal with the delay or set one up
			if (packet_delayed) {
				packet_delayed = dealWithDelay(socketFD_Emulator);
			}
			
			else {
				packet_delayed = delayPacket();
			}
		}
		else {
			if (debug) {
				printf("emulator: got packet from %s\n", get_ip_address( (struct sockaddr*) &addr )); 
				printf("emulator: packet is %d bytes long\n", numbytes);
			}
			
			
			
			// Gets a packet from the buffer from recvfrom
			packet pkt = getPktFromBuffer(buffer);
			
			// Print less obtrusive information about the packet. Cleaner.
			print_clean_pkt_info(pkt);
			
			if (debug) {
				print_packet(pkt);
			}
			
			bool no_entry_found = true;
			int index;
			
			 //
			 // Look through the forwarding table for an entry that matches the destination
			 // of the packet.
			 //
			for (index = 0; index < forwarding_table_size; index++) {
				forwarding_entry entry = forwarding_table[index];
				
				// The destinations do not line up; drop packet and log issue.
				if ( (strcmp(entry.destination_IP, pkt.destIP) != 0) || 
					  (strcmp(entry.destination_port, pkt.destPort) != 0) ) {
					//log_no_entry_found(pkt.destIP, pkt.destPort, pkt.srcIP, pkt.srcPort, pkt.priority, pkt.length);
				}
				else {
					if (debug) {
						printf("A forwarding table-packet destination match was found.\n");
					}
				  
					// Queue the packet 
					queue_packet(pkt, index);
					no_entry_found = false;
					
					// Deal with the delay or set one up
					if (packet_delayed) {
						packet_delayed = dealWithDelay(socketFD_Emulator);
					}
					else {
						packet_delayed = delayPacket();
					}
				}
	
			}
			
			// If no destination was found, drop packet and log issue
			if (no_entry_found) {
				log_no_entry_found(pkt.destIP, pkt.destPort, pkt.srcIP, pkt.srcPort, pkt.priority, pkt.length);
			}
		}
	}
	*/

	
	return 0;
}	// end main
