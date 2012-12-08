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

#endif
