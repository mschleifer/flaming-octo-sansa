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
//#include "queue.h"
#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include "topology.cpp"
#include "test.cpp"
#include "util.hpp"
#include <iostream>

#define MAXLINE (440)
#define ROUTETRACESIZE (133)

/* Global Variables */
bool debug = true;
vector<Node> topology; // Array of the nodes in the network
vector<Node> top2;
vector<Node> mainNodes;
Topology top = Topology(true);
int topologySize = 0;
Node *emulator = new Node();	// Node representing this particular emulator


/*
 * Reads a topology text file passed in by 'filename' and x
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
	
	// We'll read the file fo-real's this time and do some work
	rewind(file);
	
	char** saveptr = NULL;	// for strtok_r
	char* token = NULL;		// Each token returned by strtok
	
	char* address = NULL;	// address for a node
	char* port = NULL;		// port of a node
	
	int nodeCount = 0;		// Index of the host node we're on
	
	
	
	while ( fgets ( line, MAXLINE, file ) != NULL ) // Read in a line
	{
		if(debug) cout << "LINE: " << line;
		
		// Parse the line one token (<addr,port> pair) at a time
		saveptr = &line;
		token = strtok_r(NULL, " \n", saveptr); // First token is the node
		address = strtok(token, ",");
		port = strtok(NULL, " \n");

		Node n = Node(address, port, true);
		topology.push_back(n);
		top2.push_back(n);
		
		// Add each other token in the line to the list of connections
		while((token = strtok_r(NULL, " \n", saveptr)) != NULL) {
			address = strtok(token, ",");
			port = strtok(NULL, " \n");
			topology[nodeCount].addNeighbor(*(new Node(address, port, true)));
		}
		
		nodeCount++; // New host node in topology for a new line
	}
	
	// Add the initial nodes; then add the neighbors of them
	top.addNodes(topology);
	for (unsigned int i = 0; i < topology.size(); i++) {
		top.addNeighbors(topology[i]);
	}
	
	cout << "Topology after readTopology():" << endl << top.toString() << endl;
	
	
	if(debug) cout << endl;
	fclose(file);
}

/*
 * Called to update the routing tables for each node.  Uses a link-state
 * protocol.
 */
void createRoutes(Topology topology) {
	cout << "inside createRoutes: " << topology.toString() << endl;
	vector<Node> mainNodes = topology.getNodes();
	vector<TestClass> testMap;
	//string startkey = mainNodes[0].getKey();
	//string endkey = mainNodes[7].getKey();
	
	/**
	 * Finds the shortest path from every node to every other node 
	 * in the entire topology.
	 */
	for (unsigned int i = 0; i < mainNodes.size(); i++) {
		for (unsigned int j = 0; j < mainNodes.size(); j++) {
			if (i == j) continue;
			string startkey = mainNodes[i].getKey();
			string endkey = mainNodes[j].getKey();
			TestClass tc = TestClass(startkey, endkey, topology);
			testMap.push_back(tc);
		}
	}
	
	for (unsigned int i = 0; i < testMap.size(); i++) {
		cout << testMap[i].toString();
		cout << endl;
	}
	
	cout << "Finished and didn't take long? Yep!" << endl << endl;
	cout << "So at the very least, we have some way that we can give our program a "
		  << "start node, end node, and topology, and it will find the best paths. "
		  << "You are more familiar with what exactly I need to do with link state "
		  << "packets and sending w/ sequence numbers and etc, " 
		  << "so you'll have to explain what I should attempt to do " 
		  << "with the class.  I think this was definitely a big part of it though." << endl;
	// TODO: This seems to me like it'll be tough.  We're going to need to come
	// TODO: up with a number of structures to keep track of different data
	// TODO: elements (network graph, routing table for each node, etc.).
	// TODO: Wikipedia has a pretty detailed description (link-state routing).
}


/**
 * Finds the shortest path from this emulator's node to each of its'
 * neighbors.  I'm not sure what exactly I'll need to do for this 
 * project, but one of these two createRoutes methods should help.
 */
void createRoutes(Topology topology, Node emulator) {
	cout << "Inside createRoutes: " << endl;
	vector<Node> mainNodes = topology.getNodes();
	vector<TestClass> testMap;
	string startkey = emulator.getKey();
	unsigned int startIndex = 99999;

	// Get the index in the topology where 'this' node is located
	for (unsigned int i = 0; i < mainNodes.size(); i++) {
		if (emulator.getKey().compare(mainNodes[i].getKey()) == 0) {
			startIndex = i;
		}
	}
	
	// If we didn't find 'this' node in the topology, something went wrong, exit
	if (startIndex == 99999) {
		cout << "Something went wrong.  (createRoutes(Topology, Node))..." << endl;
		exit(-1);
	}
	
	// Find the shortest path to each of the other nodes from this node
	for (unsigned int j = 0; j < mainNodes.size(); j++) {
		if (startIndex == j) continue;
		string startkey = mainNodes[startIndex].getKey();
		string endkey = mainNodes[j].getKey();
		TestClass tc = TestClass(startkey, endkey, topology);
		testMap.push_back(tc);
	}
	
	
	// Print out the results 
	if (debug) {
		for (unsigned int i = 0; i < testMap.size(); i++) {
			cout << testMap[i].toString();
			cout << endl;
		}
	}
}


string emulatorIP;
string emulatorPort;
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

	char* buffer = (char*)malloc(1024);
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
			emulatorPort = port;		//string, same thing
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
	
	// Set up the local Node "emulator" with info about the current host
	emulator = &top.getNode(getNodeKey(getIP(), port));
	if(debug)
		cout << "Node for current emulator: " << emulator->toString() << endl << endl;
	
	// Get the hostname where this emulator is running
	char hostname[255];
	gethostname(hostname, 255);
	char ip[32];
	hostname_to_ip(hostname, ip);
	emulatorIP = ip;
	cout << "Hostname: " << hostname << endl;
	cout << "Emulator IP::Port: " << emulatorIP << "::" << emulatorPort << endl;
	
	int socketFD;
	struct addrinfo *addrInfo, hints, *p;	// Use these to getaddrinfo()
	
	// Set up the hints
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	//hints.ai_flags = AI_PASSIVE; //Don't want to connect to "any" ip 
	
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
		if (bind(socketFD, p->ai_addr, p->ai_addrlen) == -1) {
			close(socketFD);
			perror("sender: connect");
			continue;
		}
		break;
	}
	
	// TODO: Disable different nodes and you will see the difference
	top.disableNode("5.0.0.0:5");
	//top.disableNode("3.0.0.0:3");
	//top.disableNode("4.0.0.0:4");
	//top.disableNode("3.0.0.0:3");
	createRoutes(top);
	//createRoutes(top, *emulator);
	
	
	// Send a message to each of the emulator's neighbors
	// TODO: Probably put this all in its own function for organization
	struct sockaddr_in sock_sendto;
	socklen_t sendto_len;
	
	vector<Node> neighbors = emulator->getNeighbors();
	
	for(int i = 0; i < (int)neighbors.size(); i++) {
	
		sock_sendto.sin_family = AF_INET;
		sock_sendto.sin_port = htons( atoi(neighbors[i].getPort().c_str()) );
		inet_pton(AF_INET, neighbors[i].getHostname().c_str(), &sock_sendto.sin_addr);
		memset(sock_sendto.sin_zero, '\0', sizeof(sock_sendto.sin_zero));
		sendto_len = sizeof(sock_sendto);

		LinkPacket linkstatePacket;
		linkstatePacket.type = 'L';
		linkstatePacket.sequence = 1;
		linkstatePacket.length = MAXLINKPAYLOAD;
		memcpy(linkstatePacket.srcIP, emulator->getHostname().c_str(), emulator->getHostname().size());
		memcpy(linkstatePacket.srcPort, emulator->getPort().c_str(), emulator->getPort().size());
		linkstatePacket.payload = (char*)malloc(emulator->getNeighbors().size() * LINKPAYLOADNODE);
		
		createLinkPacketPayload(linkstatePacket.payload, emulator->getNeighbors());
		
		char* sendPkt = (char*)malloc(MAXLINKPACKET);
		serializeLinkPacket(linkstatePacket, sendPkt);
		

		if(debug)
			cout << "Sending message to: " << neighbors[i].getHostname().c_str()
				<< ":" << neighbors[i].getPort() << endl;

		if ( sendto(socketFD, (void*)sendPkt, LINKPACKETHEADER+linkstatePacket.length, 0, 
						(struct sockaddr*) &sock_sendto, sendto_len) == -1 ) {
			perror("sendto()");
		}
		
		free(sendPkt);
		free(linkstatePacket.payload);
	}
	
	
	
	struct sockaddr_storage addr;
	socklen_t addr_len;
	
	while (true) {
		addr_len = sizeof(addr);

		int numbytes;
		memset(buffer, 0,  1024); // Need to zero the buffer

		if ((numbytes = recvfrom(socketFD, buffer, MAXLINKPACKET, 0, 
						(struct sockaddr*)&addr, &addr_len)) <= -1) {
			
			// Nothing received
		}
		else {
		
			char type;
			memcpy(&(type), buffer, sizeof(char));
			cout << "packet type: " << type << endl;
			
			if (type == 'L') {
				LinkPacket linkstatePacket = getLinkPktFromBuffer(buffer);
				printf("MESSAGE: %s\n", linkstatePacket.payload);
			}
			else if (type == 'T') {
				RoutePacket routePacket = getRoutePktFromBuffer(buffer);
				print_RoutePacket(routePacket);
	
				if (routePacket.ttl == 0) {
				  
					if(debug) {
						cout << "emulator: (sending) routetrace back to to: " << routePacket.srcIP << "::" << routePacket.srcPort << endl;
					}
		
					// send packet back to routetrace
					sock_sendto.sin_family = AF_INET;
					sock_sendto.sin_port = htons( atoi( routePacket.srcPort) );
					inet_pton(AF_INET, routePacket.srcIP, &sock_sendto.sin_addr);
					memset(sock_sendto.sin_zero, '\0', sizeof(sock_sendto.sin_zero));
					sendto_len = sizeof(sock_sendto);
					
					strcpy(routePacket.srcIP, emulatorIP.c_str());
					strcpy(routePacket.srcPort, emulatorPort.c_str());
					char* sendPkt = (char*)malloc(ROUTETRACESIZE);
					serializeRoutePacket(routePacket, sendPkt);
					
					if ( sendto(socketFD, (void*)sendPkt, ROUTETRACESIZE, 0, 
							  (struct sockaddr*) &sock_sendto, sendto_len) == -1 ) {
						perror("sendto()");
					}
					
					free(sendPkt);
				}
				
				else {
					cout << "Now we have to do something more.." << endl;
				}
				
			}
		}
	}
	
	// TODO: now the real stuff happens. Loop repeatedly calling createRoutes()
	// TODO: and updating the shortest paths using link-state protocol.
	
	// TODO: Also need a forwardPacket() function for sending to neighbors
	
	return 0;
}	// end main

// NOTES ON THE LINK-STATE PROTOCOL (from Wikipedia)

// DISTRIBUTING THE MAP INFORMATION
	// First, each node needs to determine what other ports it is connected to
	// it does this using a simple reachability protocol which it runs each of its neighbors
	
	// Next, each node periodically and in case of connectivity changes makes up 
	// a short message, the link-state advertisement, which:
    	// Identifies the node which is producing it.
    	// Identifies all the other nodes (either routers or networks) to which it is directly connected.
    	// Includes a sequence number, which increases every time the source node makes up a new version of the message.
	// This message is then flooded throughout the network. Each node in the network remembers, 
	// for every other node in the network, the sequence number of the last 
	// link-state message which it received from that node
	
	// A Node sends a copy to all of its neighbors. When a link-state advertisement
	// is received at a node, the node looks up the sequence number it has stored for
	// the source of that link-state message. If this message is newer (i.e. has a higher
	// sequence number), it is saved, and a copy is sent in turn to each of that node's neighbors
	
// CREATING THE MAP
	// Finally, with the complete set of link-state advertisements 
	// (one from each node in the network) in hand, it is "obviously" easy to 
	// produce the graph for the map of the network.
	
	// The algorithm iterates over the collection of link-state advertisements; 
	// for each one
		// make links on the map of the network, from the node which sent that message
		// to all the nodes which that message indicates are neighbours of the sending node
	
	// (Potentiall optional for use I think)
	// if one node reports that it is connected to another, but the other node 
	// does not report that it is connected to the first, there is a problem, 
	// and the link is not included on the map
	
	// Link-state message giving information about the neighbors is recomputed, 
	// and then flooded throughout the network, whenever there is a change in the
	// connectivity between the node and its neighbors, e.g. when a link fails. 
	// Any such change will be detected by the reachability protocol which each 
	// node runs with its neighbors.
	
// CREATING THE ROUTING TABLE
