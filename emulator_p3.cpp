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
#include <string.h>
#include "topology.cpp"
#include "test.cpp"
//#include "util.hpp"
#include <iostream>

#define MAXLINE (2000)
#define ROUTETRACESIZE (133)

/* Global Variables */
bool debug = true;
vector<Node> topology; // Array of the nodes in the network
vector<Node> top2;
vector<Node> mainNodes;
vector<TestClass> bestRoutes;
Topology top = Topology(true);
int topologySize = 0;
Node *emulator = new Node();	// Node representing this particular emulator
map<string, int> sequenceMap;	// Keep track of the last seq. no. seen for each Node
map<string, time_t> lastACKMap;	// Keep track of last ACK time for each node
int currentSequence = 0;		// Current seq. no. of linkstate packets sent by this emulator
time_t last_query_time = time(NULL);


/*
 * Reads a topology text file passed in by 'filename' and x
 *
 */
void readTopology(const char* filename) {
	cout << "--Running readTopology--" << endl;
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
		sequenceMap.insert(pair<string,int>(getNodeKey(n.getHostname(), n.getPort()), 0));
		
		// Add each other token in the line to the list of connections
		while((token = strtok_r(NULL, " \n", saveptr)) != NULL) {
			address = strtok(token, ",");
			port = strtok(NULL, " \n");
			topology[nodeCount].addNeighbor(*(new Node(address, port, true)));
		}
		
		nodeCount++; // New host node in topology for a new line
	}
	
	cout << nodeCount << endl;
	// Add the initial nodes; then add the neighbors of them
	top.addNodes(topology);
	cout << top.toString() << endl;
	for (unsigned int i = 0; i < topology.size(); i++) {
		top.addNeighbors(topology[i]);
	}
	
	cout << top.toString() << endl;
	
	
	if(debug) cout << endl;
	fclose(file);
}

/*
 * Called to update the routing tables for each node.  Uses a link-state
 * protocol.
 */
void createRoutes(Topology topology) {
	cout << "--Running createRoutes--" << topology.toString() << endl;
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
}


/**
 * Finds the shortest path from this emulator's node to each of its'
 * neighbors.  I'm not sure what exactly I'll need to do for this 
 * project, but one of these two createRoutes methods should help.
 */
vector<TestClass> createRoutes(Topology topology, Node emulator) {
	cout << "--Running createRoutes--" << endl;
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
	/*if (debug) {
		for (unsigned int i = 0; i < testMap.size(); i++) {
			cout << testMap[i].toString();
			cout << endl;
		}
	}*/
	
	return testMap;
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
	//cout << "Hostname: " << hostname << endl;
	//cout << "Emulator IP::Port: " << emulatorIP << "::" << emulatorPort << endl;
	
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
		//fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		perror("getaddrinfo");
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
	
	// Find the best routes
	bestRoutes = createRoutes(top, *emulator);
	
	struct sockaddr_in sock_sendto; // Sockaddr_in to use for sending
	
	// Setup lastACKMap before we start looping forever
	vector<Node> neighbors = emulator->getNeighbors(); // Used in other areas too
	for(int i = 0; i < (int)neighbors.size(); i++) {
		lastACKMap.insert(pair<string,time_t>(getNodeKey(neighbors[i].getHostname(), 
							neighbors[i].getPort()), time(NULL)));
	}
	
	struct sockaddr_storage addr;
	socklen_t addr_len;
	bool topologyChanged = false;	// Cheap way to keep track of topology changes
	
	while (true) {
		addr_len = sizeof(addr);

		int numbytes;
		memset(buffer, 0,  1024); // Need to zero the buffer
		
// CHECK IF YOUR NEIGHBORS ARE ONLINE
		if (time(NULL) - last_query_time > 1.0) {
		neighbors = emulator->getNeighbors();
		for(int i = 0; i < (int)neighbors.size(); i++) { // for each neighbor 
			// send QUERY
			sock_sendto.sin_family = AF_INET;
			sock_sendto.sin_port = htons( atoi(neighbors[i].getPort().c_str()) );
			inet_pton(AF_INET, neighbors[i].getHostname().c_str(), &sock_sendto.sin_addr);
			memset(sock_sendto.sin_zero, '\0', sizeof(sock_sendto.sin_zero));

			QueryPacket queryPacket;
			queryPacket.type = 'Q';
			strcpy(queryPacket.srcIP, emulator->getHostname().c_str());
			strcpy(queryPacket.srcPort, emulator->getPort().c_str());
			
			char* sendPkt = (char*)malloc(MAXQUERYPACKET);
			memset(sendPkt, 0, MAXQUERYPACKET);
			serializeQueryPacket(queryPacket, sendPkt);
			// TODO: ^^ Might want to move this outside of loop
			
			if ( sendto(socketFD, (void*)sendPkt, MAXQUERYPACKET, 0, 
					(struct sockaddr*) &sock_sendto, sizeof(sock_sendto)) == -1 ) {
				perror("QUERY - sendto()");
			}
			
			free(sendPkt); // TODO: This too
			
			// check currentTime - lastACKMap > TIMEOUT (expired if true)
			if(difftime(time(NULL), lastACKMap[getNodeKey(neighbors[i].getHostname(), 
							neighbors[i].getPort())]) > TIMEOUT) {
				// If the timeout is expired AND we thought the Node was online
				if(neighbors[i].getOnline()) {
				
					string offlineNodeKey = getNodeKey(neighbors[i].getHostname(), neighbors[i].getPort());
					if(debug) cout << "TIMEOUT EXPIRED - " << offlineNodeKey << endl;
				
					// take node offline
					top.disableNode(offlineNodeKey);
				
					topologyChanged = true;
				}				
			}
			last_query_time = time(NULL);
		}
		}
		
		// If there was a change send out linkstate packets and rerun createRoutes()
		if(topologyChanged) {
			cout << "***TOPOLOGY CHANGED" << endl;
			bestRoutes = createRoutes(top, *emulator);
			cout << top.toString();
			
			// Send out linkstate packets
			for(int i = 0; i < (int)neighbors.size(); i++) {
				currentSequence++;
				
				sock_sendto.sin_family = AF_INET;
				sock_sendto.sin_port = htons( atoi(neighbors[i].getPort().c_str()) );
				inet_pton(AF_INET, neighbors[i].getHostname().c_str(), &sock_sendto.sin_addr);
				memset(sock_sendto.sin_zero, '\0', sizeof(sock_sendto.sin_zero));

				LinkPacket linkstatePacket;
				linkstatePacket.type = 'L';
				linkstatePacket.sequence = currentSequence;
				linkstatePacket.length = 0;
				strcpy(linkstatePacket.srcIP, emulator->getHostname().c_str());
				strcpy(linkstatePacket.srcPort, emulator->getPort().c_str());
				linkstatePacket.payload = (char*)malloc(emulator->getNeighbors().size() * LINKPAYLOADNODE);
				linkstatePacket.length = emulator->getNeighbors().size() * LINKPAYLOADNODE;
		
				createLinkPacketPayload(linkstatePacket.payload, emulator->getNeighbors());
		
				char* sendPkt = (char*)malloc(MAXLINKPACKET);
				memset(sendPkt, 0, MAXLINKPACKET);
				serializeLinkPacket(linkstatePacket, sendPkt);
		

				if(debug)
					cout << "Sending new LinkPacket to: " << neighbors[i].getHostname().c_str()
						<< ":" << neighbors[i].getPort() << endl;
					print_LinkPacket(linkstatePacket);

				if ( sendto(socketFD, (void*)sendPkt, LINKPACKETHEADER+linkstatePacket.length, 0, 
								(struct sockaddr*) &sock_sendto, sizeof(sock_sendto)) == -1 ) {
					perror("NEW LINK OFFLINE - sendto()");
				}
		
				free(sendPkt);
				free(linkstatePacket.payload);
			}
			
			topologyChanged = false;
		}

// RECEIVE BEHAVIOR
		if ((numbytes = recvfrom(socketFD, buffer, MAXLINKPACKET, 0, 
						(struct sockaddr*)&addr, &addr_len)) <= -1) {
			
			// Nothing received
		}
		else {
			char type;
			memcpy(&(type), buffer, sizeof(char));
			if(type != 'Q' && type != 'A') // Too much output with these included
				cout << "Received packet type: " << type << endl;
				
// RECEIVE L PACKET
			if (type == 'L') {
				// Parse incoming data
				LinkPacket linkstatePacket = getLinkPktFromBuffer(buffer);
				vector<Node> recNodes = getNodesFromLinkPacket(linkstatePacket);
				
				// If our neighbor forwarded us back our own packet
				// This is poor optimization, but I honestly don't really care
				if(strcmp(linkstatePacket.srcIP, emulator->getHostname().c_str()) == 0 &&
				   strcmp(linkstatePacket.srcPort, emulator->getPort().c_str()) == 0 ) {
					
					if(debug) cout << "Got our own LinkState Packet back" << endl;
				}
				
				// Else if this is a newer linkstate packet from the source
				else if(sequenceMap[getNodeKey(linkstatePacket.srcIP,linkstatePacket.srcPort)] < 
						(int)linkstatePacket.sequence)
				{
					// Update the sequenceMap
					sequenceMap[getNodeKey(linkstatePacket.srcIP,linkstatePacket.srcPort)] = linkstatePacket.sequence;
					
					// Update your view of the topology
					// For the nodes received in the linkstatePacket
					for(vector<Node>::iterator itr = recNodes.begin(); itr != recNodes.end(); itr++) {
						// If the matching node in the topology has a different "online" value
						if(itr->getOnline() != top.getNode(getNodeKey(itr->getHostname(),itr->getPort())).getOnline()) {
							if(itr->getOnline()) {
								top.enableNode(getNodeKey(itr->getHostname(),itr->getPort()));
								cout << "Enabled " << itr->toString() << endl;
							}
							else {
								top.disableNode(getNodeKey(itr->getHostname(),itr->getPort()));
								cout << "Disabled " << itr->toString() << endl;
							}
						}
					}
					
					if(debug) {
						cout << "Going to forward " << endl << "\t";
						print_LinkPacket(getLinkPktFromBuffer(buffer));
					}
					
					// Forward the packet on to your neighbors
					vector<Node> n = emulator->getNeighbors();
					for(vector<Node>::iterator itr = n.begin();
							itr != n.end(); itr++) {
						
						//printf("\tForwarding to %s:%s\n", itr->getHostname().c_str(),itr->getPort().c_str());
						sock_sendto.sin_family = AF_INET;
						sock_sendto.sin_port = htons( atoi( itr->getPort().c_str()) );
						inet_pton(AF_INET, itr->getHostname().c_str(), &sock_sendto.sin_addr);
						memset(sock_sendto.sin_zero, '\0', sizeof(sock_sendto.sin_zero));
						
						if ( sendto(socketFD, (void*)buffer, LINKPACKETHEADER+linkstatePacket.length, 0, 
							  (struct sockaddr*) &sock_sendto, sizeof(sock_sendto)) == -1 ) {
							perror("FORWARD LINK - sendto()");
						}
					}
					// Don't forget to update the routes
					bestRoutes = createRoutes(top, *emulator);
					cout << top.toString();
				}
				// Else we got a linkstate packet from a source but it was old
				// This probably shouldn't happen to us
				else {
					if(debug)
						cout << "Got a packet with old sequence number" << endl;
				}
				
			}
// RECEIVE T PACKET
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
					
					strcpy(routePacket.srcIP, emulatorIP.c_str());
					strcpy(routePacket.srcPort, emulatorPort.c_str());
					char* sendPkt = (char*)malloc(ROUTETRACESIZE);
					serializeRoutePacket(routePacket, sendPkt);
					
					if ( sendto(socketFD, (void*)sendPkt, ROUTETRACESIZE, 0, 
							  (struct sockaddr*) &sock_sendto, sizeof(sock_sendto)) == -1 ) {
						perror("ROUTE HOME - sendto()");
					}
					
					free(sendPkt);
				}
				
				else {
					// send packet back to next hop
					
					string gotoIp;
					string gotoPort;
					for (unsigned int i = 0; i < bestRoutes.size(); i++ ) {
						//cout << bestRoutes[i].toString();
						if (bestRoutes[i].getEndkey().
						  compare(getNodeKey(routePacket.dstIP, routePacket.dstPort)) == 0) {
							gotoIp = keyToIp(bestRoutes[i].getNextkey());
							gotoPort = keyToPort(bestRoutes[i].getNextkey());
							//cout << gotoIp << "::" << gotoPort << endl;
						}
					}
					
					sock_sendto.sin_family = AF_INET;
					sock_sendto.sin_port = htons( atoi(gotoPort.c_str()) );
					inet_pton(AF_INET, gotoIp.c_str(), &sock_sendto.sin_addr);
					memset(sock_sendto.sin_zero, '\0', sizeof(sock_sendto.sin_zero));
					
					routePacket.ttl--;
					char* sendPkt = (char*)malloc(ROUTETRACESIZE);
					serializeRoutePacket(routePacket, sendPkt);
					
					if ( sendto(socketFD, (void*)sendPkt, ROUTETRACESIZE, 0, 
							  (struct sockaddr*) &sock_sendto, sizeof(sock_sendto)) == -1 ) {
						perror("ROUTE NEXT - sendto()");
					}
					
					free(sendPkt);
				}
				
			}
// RECEIVE Q PACKET
			else if(type == 'Q') {
			
				QueryPacket queryPacket = getQueryPktFromBuffer(buffer);
				
				// Send an ACK back to the source of the Query
				sock_sendto.sin_family = AF_INET;
				sock_sendto.sin_port = htons( atoi(queryPacket.srcPort) );
				inet_pton(AF_INET, queryPacket.srcIP, &sock_sendto.sin_addr);
				memset(sock_sendto.sin_zero, '\0', sizeof(sock_sendto.sin_zero));

				QueryPacket ACKPacket;
				ACKPacket.type = 'A';
				strcpy(ACKPacket.srcIP, emulator->getHostname().c_str());
				strcpy(ACKPacket.srcPort, emulator->getPort().c_str());
		
				char* sendPkt = (char*)malloc(MAXQUERYPACKET);
				memset(sendPkt, 0, MAXQUERYPACKET);
				serializeQueryPacket(ACKPacket, sendPkt);

				// Uncomment at your own peril
				//cout << "Sending ACK to: " << queryPacket.srcIP << ":" << queryPacket.srcPort << endl;

				if ( sendto(socketFD, (void*)sendPkt, MAXQUERYPACKET, 0, 
						(struct sockaddr*) &sock_sendto, sizeof(sock_sendto)) == -1 ) {
					perror("ACK - sendto()");
				}
		
				free(sendPkt);
			}
// RECEIVE A PACKET
			else if(type == 'A') {
				// Abandon all hope ye who uncomment here
				//cout << "GOT ACK" << endl;
				
				// Parse the incoming data and update lastACKMap
				QueryPacket ACKPacket = getQueryPktFromBuffer(buffer);
				string recNodeKey = getNodeKey(ACKPacket.srcIP, ACKPacket.srcPort);
				lastACKMap[recNodeKey] = time(NULL);
				
				// If we get an ACK from a Node we didn't think was online
				if(!top.getNode(recNodeKey).getOnline()) {
					top.enableNode(recNodeKey);
					cout << endl << "***TOPOLOGY CHANGED: NODE ONLINE - " << recNodeKey << endl;
					bestRoutes = createRoutes(top, *emulator);
					cout << top.toString();
					
					// Send out linkstate packets to neighbors
					for(int i = 0; i < (int)neighbors.size(); i++) {
						currentSequence++;
				
						sock_sendto.sin_family = AF_INET;
						sock_sendto.sin_port = htons( atoi(neighbors[i].getPort().c_str()) );
						inet_pton(AF_INET, neighbors[i].getHostname().c_str(), &sock_sendto.sin_addr);
						memset(sock_sendto.sin_zero, '\0', sizeof(sock_sendto.sin_zero));

						LinkPacket linkstatePacket;
						linkstatePacket.type = 'L';
						linkstatePacket.sequence = currentSequence;
						linkstatePacket.length = 0;
						strcpy(linkstatePacket.srcIP, emulator->getHostname().c_str());
						strcpy(linkstatePacket.srcPort, emulator->getPort().c_str());
						linkstatePacket.payload = (char*)malloc(emulator->getNeighbors().size() * LINKPAYLOADNODE);
						linkstatePacket.length = emulator->getNeighbors().size() * LINKPAYLOADNODE;
		
						createLinkPacketPayload(linkstatePacket.payload, emulator->getNeighbors());
		
						char* sendPkt = (char*)malloc(MAXLINKPACKET);
						memset(sendPkt, 0, MAXLINKPACKET);
						serializeLinkPacket(linkstatePacket, sendPkt);
						// TODO:^^ Might be worth moving everything above here outside of loop

						if(debug)
							cout << "Sending new LinkPacket to: " << neighbors[i].getHostname().c_str()
								<< ":" << neighbors[i].getPort() << endl;
							print_LinkPacket(linkstatePacket);

						if ( sendto(socketFD, (void*)sendPkt, LINKPACKETHEADER+linkstatePacket.length, 0, 
										(struct sockaddr*) &sock_sendto, sizeof(sock_sendto)) == -1 ) {
							perror("NEW LINK ONLINE - sendto()");
						}
		
						free(sendPkt); // TODO:^^ This too
						free(linkstatePacket.payload); // TODO:^^ And this
					}
				}
				
			}
		}
	}
	
	return 0;
}	// end main
