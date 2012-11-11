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

#define BUFFER (5120)

#define HEADERSIZE (17)
#define MAXPACKETSIZE (5137)
#define MAXPAYLOADSIZE (5120)

#define P2_MAXPACKETSIZE (5238)
#define P2_HEADERSIZE (101)

// get sockaddr, IPv4 or IPv6:
void 
*get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/**
 * Prints all information in a packet, tabbed.
 * @param pkt A packet to print
 */
void print_packet(packet pkt) {
	printf("packet:\n\tpriority: %d\n\tsrcIP: %s\n\tsrcPort: %s\n\tdestIP: %s\n\tdestPort: %s\n\tnew_length: %d\n\ttype: %c\n\tsequence: %d\n\tlength: %d\n\tpayload: %s\n",
			pkt.priority, pkt.srcIP, pkt.srcPort, pkt.destIP, pkt.destPort, pkt.new_length,
			pkt.type, pkt.sequence, pkt.length, pkt.payload);
}

/**
 * Prints all information corresponding to a packet, in the buffer.
 * @param buffer The buffer containing the packet information to print.
 */
void print_packetBuffer(char* buffer) {
	printf("\nPACKET BUFFER\n\tpriority: %d\n\tsrcIP: %s\n\tsrcPort: %s\n\tdestIP: %s\n\tdestPort: %s\n\tnew_length: %d\n\ttype: %c\n\tsequence: %d\n\tlength: %d\n\tpayload: %s\n",
			(uint8_t)*buffer, buffer+1, buffer+33, buffer+49, buffer+81, (int)*(buffer+97),
			(uint32_t)*(buffer+P2_HEADERSIZE), (int)*(buffer+P2_HEADERSIZE+1), (int)*(buffer+P2_HEADERSIZE+9),
			buffer+P2_HEADERSIZE+HEADERSIZE);
}


/**
 * Simply prints the given message to stderr
 */
void printError(char* errorMessage) {
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
serializePacket(packet pkt, char* buffer) {
	memcpy(buffer+P2_HEADERSIZE, &pkt.type, sizeof(char));
	memcpy(buffer+P2_HEADERSIZE+1, &pkt.sequence, sizeof(uint32_t));
	memcpy(buffer+P2_HEADERSIZE+9, &pkt.length, sizeof(uint32_t));
	memcpy(buffer+P2_HEADERSIZE+HEADERSIZE, pkt.payload, pkt.length);
	memcpy(buffer, &pkt.priority, sizeof(uint8_t));
	memcpy(buffer+1, pkt.srcIP, 32);
	memcpy(buffer+33, pkt.srcPort, 16);
	memcpy(buffer+49, pkt.destIP, 32);
	memcpy(buffer+81, pkt.destPort, 16);
	memcpy(buffer+97, &pkt.new_length, sizeof(uint32_t));
}

/**
 * Takes in a buffer that is assumed to contain everything that a packet
 * would contain, and it returns the packet with all information filled
 * out.  The opposite of serializePacket().
 * @param buffer A buffer filled with the information a packet would contain
 * @return The packet with the information from the buffer
 */
packet getPktFromBuffer(char* buffer) {
	packet pkt;
	memcpy(&pkt.priority, buffer, sizeof(uint8_t));
	memcpy(&pkt.srcIP, buffer+1, 32);
	memcpy(&pkt.srcPort, buffer+33, 16);
	memcpy(&pkt.destIP, buffer+49, 32);
	memcpy(&pkt.destPort, buffer+81, 16);
	memcpy(&pkt.new_length, buffer+97, 4);
	memcpy(&pkt.type, buffer+P2_HEADERSIZE, sizeof(char));
	memcpy(&pkt.sequence, buffer+P2_HEADERSIZE+1, sizeof(uint32_t));
	memcpy(&pkt.length, buffer+P2_HEADERSIZE+9, sizeof(uint32_t));
	pkt.payload = buffer+P2_HEADERSIZE+HEADERSIZE;
	return pkt;
}


/**
 * Takes in a hostname and an ip, both strings, and gets the ip
 * address related to the hostname and puts it into the ip string.
 * @param hostname The hostname of the ip address you want
 * @param ip The ip address associated with the given hostname (returned)
 */
int hostname_to_ip(char *hostname , char *ip)
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
	fprintf(stderr, "usage: %s -p <port> -q <queue_size> -f <filename> -l <log> -d <debug>\n", prog);
	exit(1);
}

void
usage_Requester(char *prog) {
	fprintf(stderr, "usage: %s -p <port> -f <f_hostname> -h <f_port> -o <file_option> -w <window>\n", prog);
	exit(1);
}
void
usage_Sender(char *prog) {
	fprintf(stderr, "usage: %s -p <port> -g <requester_port> -r <rate> -q <seq_no> ", prog);
	fprintf(stderr, "-l <length> -f <f_hostname> -h <f_port> -i <priority> -t <timeout>\n");
	exit(1);
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


#endif
