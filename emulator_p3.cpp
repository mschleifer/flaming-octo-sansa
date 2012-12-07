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
#include "node.cpp"
#include "topology.cpp"
#include <iostream>

#define MAXLINE (440)

/* Global Variables */
bool debug = true;
Node *topology; // Array of the nodes in the network
				// TODO: may want to use something other than an array
int topologySize = 0;
Node *emulator = new Node();	// Node representing this particular emulator

/*
 * Reads a topology text file passed in by 'filename' and sets up the global
 * Node[] that represents the network.
 *
 */
void readTopology(const char* filename) {

	char* line = (char*)malloc(MAXLINE);	// Line in the file
	
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		fprintf(stderr, "Cannot open input file.\n");
		exit(1);
	}
	
	// We'll read the file once first to figure out how many nodes are in the
	// topology so we can create the Node[] dynamically
	while ( fgets ( line, MAXLINE, file ) != NULL ) // Read in a line
	{
		topologySize++;
	}
	topology = new Node[topologySize];
	
	// We'll read the file fo-real's this time and do some work
	rewind(file);
	
	char** saveptr = NULL;	// for strtok_r
	char* token = NULL;		// Each token returned by strtok
	
	char* address = NULL;	// address for a node
	char* port = NULL;		// port of a node
	
	int nodeCount = 0;		// Index of the host node we're on
	
	Topology top = Topology(true);
	while ( fgets ( line, MAXLINE, file ) != NULL ) // Read in a line
	{
		if(debug) cout << "LINE: " << line;
		
		// Parse the line one token (<addr,port> pair) at a time
		saveptr = &line;
		token = strtok_r(NULL, " \n", saveptr); // First token is the node
		address = strtok(token, ",");
		port = strtok(NULL, " \n");

		topology[nodeCount].setHostname(address); // Set up the host node
		topology[nodeCount].setPort(port);
		topology[nodeCount].setOnline(true);
		
		
		// Add each other token in the line to the list of connections
		while((token = strtok_r(NULL, " \n", saveptr)) != NULL) {
			address = strtok(token, ",");
			port = strtok(NULL, " \n");
			topology[nodeCount].addNeighbor(*(new Node(address, port, true)));
		}
		
		//Node node = Node(topology[nodeCount].getHostname(), topology[nodeCount].getPort(), true);
		top.addNode(topology[nodeCount]);
		
		nodeCount++; // New host node in topology for a new line
	}
	
	cout << top.toString() << endl;
	
	if(debug) cout << endl;
	fclose(file);
}

/*
 * Called to update the routing tables for each node.  Uses a link-state
 * protocol.
 */
void createRoutes() {
	// TODO: This seems to me like it'll be tough.  We're going to need to come
	// TODO: up with a number of structures to keep track of different data
	// TODO: elements (network graph, routing table for each node, etc.).
	// TODO: Wikipedia has a pretty detailed description (link-state routing).
}

/*
 * Determine where to forward a packet received by the emulator. Handles
 * both packets containing link-state info and those from routetrace
 */
void forwardPacket() {
	// TODO: Don't know if we should use this as the general packet sending
	// TODO: function or just specifically for forwarding.  We need to come up
	// TODO: with some kind of packet structure since none was given.
}

int main(int argc, char *argv[]) {

	if(argc != 5 && argc != 7) {
		printError("Incorrect number of arguments");
		usage_Emulator(argv[0]);
		return 0;
	}

	char* port = NULL;					//port of emulator
	char* filename = NULL;				//name of file with forwarding table
	int rv = -1;

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
	
	// Get the hostname where this emulator is running
	char hostname[255];
	gethostname(hostname, 255);
	
	int socketFD;
	struct addrinfo *addrInfo, hints, *p;	// Use these to getaddrinfo()
	
	// Set up the hints
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	//hints.ai_flags = AI_PASSIVE; Don't want to connect to "any" ip 
	
	// Try to get addrInfo for the specific hostname
	if ( (rv = getaddrinfo(hostname, port, &hints, &addrInfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}
	
	// Try to connect the address with a socket
	for (p = addrInfo; p != NULL; p = p->ai_next) {
		if ((socketFD = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("sender: socket");
			continue;
		}
		
		fcntl(socketFD, F_SETFL, O_NONBLOCK);
		// Use connect() instead of bind() because not AI_PASSIVE
		if (connect(socketFD, p->ai_addr, p->ai_addrlen) == -1) {
			close(socketFD);
			perror("sender: connect");
			continue;
		}
		break;
	}
	
	// Set up the local Node "emulator" with info about the current host
	string ipAddress =  inet_ntoa(((sockaddr_in*)(p->ai_addr))->sin_addr);
	emulator->setHostname(ipAddress);
	emulator->setPort(port);
	if(debug)
		cout << "ADDR/PORT for current emulator: " << emulator->toString() << endl << endl;
	
	/*if(debug) {
		cout << "TOPOLOGY ARRAY:" << endl;
		for(int i = 0; i < topologySize; i++) {
			cout << topology[i].toString() << endl;
		}
		cout << endl;
	}*/
	
	
	/*for(int i = 0; i < topologySize; i++) {
		if(topology[i].compareTo(*emulator) == 0) {
			if(debug)
				cout << "This 'emulator' node is topology[" << i << "]" << endl;
				cout << topology[i].toString() << endl << endl;
			
			// Set up the remaining members of the 'emulator' Node
			emulator->setOnline(true);
			emulator->addNeighbors(topology[i].getNeighbors());
		}
	}*/
	
	// TODO: now the real stuff happens. Loop repeatedly calling createRoutes()
	// TODO: and updating the shortest paths using link-state protocol.
	
	// TODO: Also need a forwardPacket() function for sending to neighbors
	
	return 0;
}	// end main

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
	
