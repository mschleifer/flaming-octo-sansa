#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdbool.h>
#include <string.h>

using namespace std;

#define LINKPAYLOADNODE (7) // Each discrete item in the LinkPacket.payload is
							// is a Node with 4byte IP, 2byte port, 1byte online

typedef struct LinkPacket {
	char type;
	uint32_t sequence;
	uint32_t length;
	char srcIP[32];
	char srcPort[16];
	char* payload;
} LinkPacket;

// Not currently same as the 'recommended' packet struct on the website
typedef struct RoutePacket {
	char type;
	uint32_t ttl;
	char srcIP[32];
	char srcPort[32];
	char dstIP[32];
	char dstPort[32];
} RoutePacket;

#endif
