#ifndef HELPERS_H
#define HELPERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <string.h>
#include <iostream>
#include <vector>
#include "node.cpp"

using namespace std;

#define BUFFER (5120)

#define LINKPACKETHEADER (57)
#define MAXLINKPACKET (988)
#define MAXLINKPAYLOAD (931)

using namespace std;

// Forward declare, fix this hack later
LinkPacket getLinkPktFromBuffer(char* buffer);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* Needed up here for forward declaration */
vector<Node> getNodesFromLinkPacket(LinkPacket linkPacket) {
	vector<Node> neighbors;
	char hostname[32];
	char port[16];
	bool online;
	
	for(int i = 0; i < (int)linkPacket.length; i+=LINKPAYLOADNODE) {
		memcpy(&hostname, linkPacket.payload+i, 32);
		memcpy(&port, linkPacket.payload+i+32, 16);
		memcpy(&online, linkPacket.payload+i+48,1);
		
		neighbors.push_back(Node(hostname, port, online));
	}
	
	return neighbors;
}

/**
 * Prints all information in a packet, tabbed.
 * @param pkt A packet to print
 */
void print_LinkPacket(LinkPacket pkt) {
	printf("packet:\n\ttype: %c\n\tsequence: %d\n\tlength: %d\n\tsrcIP: %s\n\tsrcPort: %s\n\tpayload: ",
			pkt.type, pkt.sequence, pkt.length, pkt.srcIP, pkt.srcPort);
	vector<Node> n = getNodesFromLinkPacket(pkt);
	for(vector<Node>::iterator itr = n.begin(); itr != n.end(); itr++) {
		cout << itr->toString() << endl << "\t\t ";
	}
	cout << endl;
}

/**
 * Prints all information corresponding to a packet, in the buffer.
 * @param buffer The buffer containing the packet information to print.
 */
void print_LinkPacketBuffer(char* buffer) {
	cout << "buffer version ";
	LinkPacket pkt = getLinkPktFromBuffer(buffer);
	
	print_LinkPacket(pkt);
}


void print_RoutePacket(RoutePacket pkt) {
	printf("packet:\n\ttype: %c\n\tTTL: %d\n\tsrcIP: %s\n\tsrcPort: %s\n\tdstIP: %s\n\tdstPort: %s\n",
			pkt.type, pkt.ttl, pkt.srcIP, pkt.srcPort, pkt.dstIP, pkt.dstPort);
}

void print_RoutePacketBuffer(char* buffer) {
	printf("\nPACKET BUFFER\n\ttype: %c\n\tTTL: %d\n\tsrcIP: %s\n\tsrcPort: %s\n\tdstIP: %s\n\tdstPort: %s\n",
			(char)*buffer, (int)*(buffer+1), buffer+5, buffer+37, buffer+69, buffer+101);
}


/**
 * Simply prints the given message to stderr
 */
void printError(char const *errorMessage) {
	fprintf(stderr, "An error has occured: %s\n", errorMessage);
}

/**
 * Places data from pkt into buffer in a form that can be sent over the network
 * Expects buffer to point to the max packet-size worth of freespace.
 * The opposite of getPktFromBuffer().
 * @param pkt A packet to transmit into the empty buffer
 * @param buffer The buffer to fill with a packet's information (returned)
 */
void
serializeLinkPacket(LinkPacket pkt, char* buffer) {
	memcpy(buffer, &(pkt.type), sizeof(char));
	memcpy(buffer+1, &(pkt.sequence), sizeof(uint32_t));
	memcpy(buffer+5, &(pkt.length), sizeof(uint32_t));
	memcpy(buffer+9, &(pkt.srcIP), 32);
	memcpy(buffer+41, &(pkt.srcPort), 16);
	memcpy(buffer+LINKPACKETHEADER, pkt.payload, pkt.length);

}

void 
serializeRoutePacket(RoutePacket pkt, char* buffer) {
	memcpy(buffer, &(pkt.type), sizeof(char));
	memcpy(buffer+1, &(pkt.ttl), sizeof(uint32_t));
	memcpy(buffer+5, &(pkt.srcIP), 32);
	memcpy(buffer+37, &(pkt.srcPort), 32);
	memcpy(buffer+69, &(pkt.dstIP), 32);
	memcpy(buffer+101, &(pkt.dstPort), 32);
}

/**
 * Takes in a buffer that is assumed to contain everything that a packet
 * would contain, and it returns the packet with all information filled
 * out.  The opposite of serializePacket().
 * @param buffer A buffer filled with the information a packet would contain
 * @return The packet with the information from the buffer
 */
LinkPacket getLinkPktFromBuffer(char* buffer) {
	LinkPacket pkt;
	memcpy(&(pkt.type), buffer, sizeof(char));
	memcpy(&(pkt.sequence), buffer+1, sizeof(uint32_t));
	memcpy(&(pkt.length), buffer+5, sizeof(uint32_t));
	memcpy(&(pkt.srcIP), buffer+9, 32);
	memcpy(&(pkt.srcPort), buffer+41, 32);
	pkt.payload = buffer+LINKPACKETHEADER;

	return pkt;
}

RoutePacket getRoutePktFromBuffer(char* buffer) {
	RoutePacket pkt;
	memcpy(&(pkt.type), buffer, sizeof(char));
	memcpy(&(pkt.ttl), buffer+1, sizeof(uint32_t));
	memcpy(&(pkt.srcIP), buffer+5, 32);
	memcpy(&(pkt.srcPort), buffer+37, 32);
	memcpy(&(pkt.dstIP), buffer+69, 32);
	memcpy(&(pkt.dstPort), buffer+101, 32);
	return pkt;
}


/**
 * Takes in a hostname and an ip, both strings, and gets the ip
 * address related to the hostname and puts it into the ip string.
 * @param hostname The hostname of the ip address you want
 * @param ip The ip address associated with the given hostname (returned)
 */
int hostname_to_ip(const char *hostname , char *ip)
{
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_in *h;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	if ( (rv = getaddrinfo( hostname , "http" , &hints , &servinfo)) != 0) 
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) 
	{
		h = (struct sockaddr_in *) p->ai_addr;
		strcpy(ip , inet_ntoa( h->sin_addr ) );
	}
	
	freeaddrinfo(servinfo); // all done with this structure
	return 0;
}

/*
 * Usage for each program
 */
void
usage_Emulator(char *prog) {
	fprintf(stderr, "usage: %s -p <port>  -f <filename> -d <debug>\n", prog);
	exit(1);
}

void 
usage_routes(char *prog) {
	cout << "usage: " << "trace -a <routetrace port> -b < source hostname> -c <source port> -d <destination hostname> -e <destination port> -f <debug option>" << endl;
	exit(-1);
}

/**
 * Clears the given file by opening it with the 'w' tag, then closing it.
 * @return 0 if successful, -1 in the case of an error
 */
int clearFile(char* file_name) {
  FILE *fp;
  fp = fopen(file_name, "w");
  
  if (!fp) {
    perror("fopen");
    return -1;
  }
  if (fclose(fp) != 0) {
    perror("fclose");
    return -1;
  }
  
  return 0;
}

/* 
 * Expects payload to have been allocated before calling.
 */
void createLinkPacketPayload(char* payload, vector<Node> neighbors) {
	vector<Node>::iterator itr;
	int offset = 0;
	for(itr = neighbors.begin(); itr != neighbors.end(); itr++) {
		bool online = (*itr).getOnline();
	
		memcpy(payload+offset, (*itr).getHostname().c_str(), 32);
		memcpy(payload+offset+32, (*itr).getPort().c_str(), 16);
		memcpy(payload+offset+48, &online, 1);
		
		offset += LINKPAYLOADNODE;
	}
}

#endif
