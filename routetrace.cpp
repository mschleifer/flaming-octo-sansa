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
		//printf("ARGS: %s %s %s %s %s %s\n", tracePort, srcHostname, srcPort, dstHostname, dstPort, debug? "True":"False");
	}
  
	return 0;
}