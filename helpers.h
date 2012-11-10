#ifndef HELPERS_H
#define HELPERS_H

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

void print_packet(packet pkt) {
	printf("packet:\n\tpriority: %d\n\tsrcIP: %s\n\tsrcPort: %s\n\tdestIP: %s\n\tdestPort: %s\n\tnew_length: %d\n\ttype: %c\n\tsequence: %d\n\tlength: %d\n\tpayload: %s\n",
			pkt.priority, pkt.srcIP, pkt.srcPort, pkt.destIP, pkt.destPort, pkt.new_length,
			pkt.type, pkt.sequence, pkt.length, pkt.payload);
}
void
print_packetBuffer(char* buffer) {
	printf("\nPACKET BUFFER\n\tpriority: %d\n\tsrcIP: %s\n\tsrcPort: %s\n\tdestIP: %s\n\tdestPort: %s\n\tnew_length: %d\n\ttype: %c\n\tsequence: %d\n\tlength: %d\n\tpayload: %s\n",
			(uint8_t)*buffer, buffer+1, buffer+33, buffer+49, buffer+81, (int)*buffer+97,
			(uint32_t)*buffer+P2_HEADERSIZE, (int)*buffer+P2_HEADERSIZE+1, (int)*buffer+P2_HEADERSIZE+9,
			buffer+P2_HEADERSIZE+HEADERSIZE);
}

void
printError(char* errorMessage) {
	fprintf(stderr, "An error has occured: %s\n", errorMessage);
}

/*
 * Places data from pkt into buffer in a form that can be sent over the network
 * Expects buffer to point to the max packet-size worth of freespace
 */
 // TODO: Add code to fill in the P2HEADER part of the packet buffer
void
serializePacket(packet pkt, char* buffer) {
	memcpy(buffer+P2_HEADERSIZE, &pkt.type, sizeof(char));
	memcpy(buffer+P2_HEADERSIZE+1, &pkt.sequence, sizeof(uint32_t));
	memcpy(buffer+P2_HEADERSIZE+9, &pkt.length, sizeof(uint32_t));
	memcpy(buffer+P2_HEADERSIZE+HEADERSIZE, pkt.payload, pkt.length);
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
