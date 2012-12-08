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
#include "node.cpp"
#include "topology.cpp"
#include "test.cpp"
#include "util.hpp"
#include <iostream>

using namespace std;

// made them global..what the heck, why not
string tracePort;
string srcHostname;
string srcPort;
string dstHostname;
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
	
	while ((c = getopt(argc, argv, "a:b:c:d:e:f:")) != -1) {
		switch (c) {
		  case 'a':
			tracePort = optarg;
			break;
		  case 'b':
			srcHostname = optarg;
			break;
		  case 'c':
			srcPort = optarg;
			break;
		  case 'd':
			dstHostname = optarg;
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
	
	int rv = -1;
	char hostname[255];
	gethostname(hostname, 255);
	cout << "Hostname: " << hostname << endl;
	
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
  
	return 0;
}