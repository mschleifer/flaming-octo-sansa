#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdbool.h>
#include <string.h>

using namespace std;

typedef struct LinkPacket {
	char type;
	uint32_t sequence;
	uint32_t length;
	uint32_t srcIP;
	uint16_t srcPort;
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
