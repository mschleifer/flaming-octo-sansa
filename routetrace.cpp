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

using namespace std;

// made them global..what the heck, why not
string tracePort;
string srcHostname;
string srcIP;
string srcPort;
string dstHostname;
string dstIP;
string dstPort;
bool debug = false;

int main(int argc, char *argv[]) {
	if (argc != 13) {
		printError("Incorrect number of arguments");
		usage_routes(argv[0]);
		return 0;
	}
  
	// Get the commandline args
	int c;
	opterr = 0;
	int ttl = 0;;
	
	
	while ((c = getopt(argc, argv, "a:b:c:d:e:f:")) != -1) {
		switch (c) {
		  case 'a':
			tracePort = optarg;
			break;
		  case 'b':
			srcHostname = optarg;
			char ip[32];
			hostname_to_ip(srcHostname.c_str(), ip);
			srcIP = ip;
			break;
		  case 'c':
			srcPort = optarg;
			break;
		  case 'd':
			dstHostname = optarg;
			hostname_to_ip(dstHostname.c_str(), ip);
			dstIP = ip;
			break;
		  case 'e':
			dstPort = optarg;
			break;
		  case 'f':
			if (atoi(optarg) > 0) {
				debug = true;
			}
			break;
		  default:
			usage_routes(argv[0]);
		}
	}
	
	if (debug) {
		cout << "ARGS: " << tracePort << " " << srcHostname << "::" << srcPort << " " << dstHostname << "::" << dstPort << " " << debug << endl;
	}
	
	
	char hostname[255];
	gethostname(hostname, 255);
	cout << "Hostname: " << hostname << endl;
	
	int rv;
	int socketFD;
	struct addrinfo *addrInfo, hints, *p;	// Use these to getaddrinfo()
	
	// Set up the hints
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE; 
	
	// Try to get addrInfo for the specific hostname
	if ( (rv = getaddrinfo(hostname, tracePort.c_str(), &hints, &addrInfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}
	
	// Try to connect the address with a socket
	for (p = addrInfo; p != NULL; p = p->ai_next) {
		if ((socketFD = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("routetrace: socket");
			continue;
		}
		
		//fcntl(socketFD, F_SETFL, O_NONBLOCK);
		if (bind(socketFD, p->ai_addr, p->ai_addrlen) == -1) {
			close(socketFD);
			perror("routetrace: bind");
			continue;
		}
		break;
	}
	
	if (p == NULL) {
		fprintf(stderr, "routetrace: failed to bind socket.\n");
		return -1;
	}
	
	
	
	struct sockaddr_in sock_sendto;
	socklen_t sendto_len;
	
	sock_sendto.sin_family = AF_INET;
	sock_sendto.sin_port = htons( atoi(srcPort.c_str()) );
	inet_pton(AF_INET, srcIP.c_str(), &sock_sendto.sin_addr);
	memset(sock_sendto.sin_zero, '\0', sizeof(sock_sendto.sin_zero));
	sendto_len = sizeof(sock_sendto);
	
	RoutePacket routeTracePkt;
	routeTracePkt.type = 'T';
	routeTracePkt.ttl = ttl;
	strcpy(routeTracePkt.srcIP, srcIP.c_str());
	strcpy(routeTracePkt.srcPort, srcPort.c_str());
	strcpy(routeTracePkt.dstIP, dstIP.c_str());
	strcpy(routeTracePkt.dstPort, dstPort.c_str());
	
	char* sendPkt = (char*)malloc(161);
	serializeRoutePacket(routeTracePkt, sendPkt);
	print_RoutePacket(routeTracePkt);
	print_RoutePacketBuffer(sendPkt);
	RoutePacket pkt2 = getRoutePktFromBuffer(sendPkt);
	print_RoutePacket(pkt2);
	
	
	/*if(debug) {
		cout << "Sending message to: " << neighbors[i].getHostname().c_str() << ":" << neighbors[i].getPort() << endl;
	}
	if ( sendto(socketFD, (void*)sendPkt, LINKPACKETHEADER+linkstatePacket.length, 0, 
			  (struct sockaddr*) &sock_sendto, sendto_len) == -1 ) {
		perror("sendto()");
	}
	
	free(sendPkt);*/
  
	return 0;
}
