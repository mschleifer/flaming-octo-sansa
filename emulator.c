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
#include "queue.h"
#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>

// Global variables:
forwarding_entry* forwarding_table;
int forwarding_table_size;

// Queues for each of the three priorities
Queue* p1_queue;
Queue* p2_queue;
Queue* p3_queue;

char* log_file; 		// Log file name
bool debug = false;		// Debug flag

packet_plus delayed_pkt;
struct timeb delay_start;
char delay_timeString[80];

const char*
get_ip_address(struct sockaddr* addr) {
	char s[INET6_ADDRSTRLEN];
	// A hack that makes absolutely no sense whatsoever
	char* n = "";
	printf("%s", n);
	return inet_ntop( AF_INET, get_in_addr(addr), s, sizeof(s) ); 
}


/**
 * Appends to the logger file (given) and adds the information given from
 * the two other parameters passed in to write to the log file.
 * @param log_file The name of the log file_name
 * @param reason The reason we are logging 
 * @param info A 'log_info' struct filled out with information required
 * @return 0 unless the case of an error
 */
int logger(char* reason, log_info info) {
	FILE *fp; 
	fp = fopen(log_file, "r+"); // Open file for read/write
	if (!fp) {
		perror("fopen");
		return -1;
	}
	
	if(fseek(fp, 0, SEEK_END) < 0) {
		//fp = fopen(file_name, "r+"); // Nothing in file yet so reopen for read/write
		perror("fseek");
		return -1;
	}
  
	fprintf(fp, "%s: (src): %s::%s, (dest): %s::%s, (time): %s.%d, (priority): %d, (size): %d\n", 
						reason,
						info.src_hostname, info.src_port,
						info.destination_hostname, info.destination_port,
						info.timeString, info.time.millitm,
						info.priority, info.size);
  
	if (fclose(fp) != 0) {
		perror("fclose");
		return -1;
	}
	return 0;
}

/**
 * Fills out the logger entry for the case when the packet must be dropped because
 * there is not an entry found in the forwarding table.
 * The parameters are used to fill out the log entry with.
 * @return 0 if there are no problems in logging to the file
 */
int log_no_entry_found(char* destIP, char* destPort, char* srcIP, char* srcPort, uint8_t priority, uint32_t length) {
	struct hostent *he;
	struct in_addr ipv4addr;
	char srcName[32], destName[32];
	
	// Get the destination hostname
	inet_pton(AF_INET, destIP, &ipv4addr);
	he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	strcpy(destName, he->h_name);
	
	// Get the source hostname
	inet_pton(AF_INET, srcIP, &ipv4addr);
	he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	strcpy(srcName, he->h_name);
	
	// Fill out the log structure and call the logger function.
	log_info info;
	strcpy(info.src_hostname, srcName);
	strcpy(info.src_port, srcPort);
	strcpy(info.destination_hostname, destName);
	strcpy(info.destination_port, destPort);
	ftime(&info.time);
	strftime(info.timeString, sizeof(info.timeString), "%H:%M:%S", localtime(&(info.time.time)));
	info.priority = priority;
	info.size = length;
	return logger("no forwarding entry was found", info);
}


/**
 * Logs the given log message to the given log file, given the packet with
 * details required.
 * @param log_message The message to put the error into the log file with
 * @param pkt The packet full of information to log
 * @return 0 if no issues logging
 */
int log_entry(char* log_message, packet pkt) {
	struct hostent *he;
	struct in_addr ipv4addr;
	char srcName[32], destName[32];
	
	// Get the destination hostname
	inet_pton(AF_INET, pkt.destIP, &ipv4addr);
	he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	strcpy(destName, he->h_name);
	
	// Get the source hostname
	inet_pton(AF_INET, pkt.srcIP, &ipv4addr);
	he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	strcpy(srcName, he->h_name);
	
	// Fill out the log structure and call the logger function.
	log_info info;
	strcpy(info.src_hostname, srcName);
	strcpy(info.src_port, pkt.srcPort);
	strcpy(info.destination_hostname, destName);
	strcpy(info.destination_port, pkt.destPort);
	ftime(&info.time);
	strftime(info.timeString, sizeof(info.timeString), "%H:%M:%S", localtime(&(info.time.time)));
	info.priority = pkt.priority;
	info.size = pkt.length;
	return logger(/*log_file, */log_message, info);
}

/**
 * Reads the contents of the given file (assumed to hold the forwarding table)
 * into an array in memory.  
 * Table will consider only options in table with same <hostname, port> combo.
 * @param filename The filename of the file with the forwarding table
 * @param hostname The hostname that we care about in the forwarding table (our hostname)
 * @param port The port that we care about (our port)
 */
int readForwardingTable(char* filename, char* hostname, char* port) {
	if (debug) {
		printf("Reading forwarding table into array in memory.\n");
	}
	int k;
	forwarding_table = (forwarding_entry*)malloc(sizeof(forwarding_entry) * 5); // size = 5..
	FILE *in_file = fopen(filename, "r");	//read only
	forwarding_table_size = 0;
  
	//test for not existing
	if (in_file == NULL) {
		fprintf(stderr, "error: could not open file '%s'\n", filename);
		return -1;
	}

	if (debug) {
		printf("hostname: %s\n", hostname);
		printf("port: %s\n", port);
	}
  
	//read each row into struct, insert into array, increment size
	forwarding_entry entry;
	while( fscanf(in_file, "%s %s %s %s %s %s %f %f", 
			entry.emulator_hostname, 
			entry.emulator_port, 
			entry.destination_hostname, 											
			entry.destination_port,
			entry.next_hostname,
			entry.next_port,
			&entry.delay,
			&entry.loss_prob) == 8) {

		// Only add rows that correspond to this host and port
		if ( (strncmp(entry.emulator_hostname, hostname, 9) == 0)
					&& (strcmp(entry.emulator_port, port) == 0) ) {
		
			hostname_to_ip(entry.destination_hostname, entry.destination_IP);
			hostname_to_ip(entry.next_hostname, entry.next_IP);
			entry.delay /= 1000.0;
			forwarding_table[forwarding_table_size] = entry;
			forwarding_table_size++;
		}
	}
	
	if (debug) {
		printf("forwarding table:\n");
		for (k = 0; k < forwarding_table_size; k++) {
			entry = forwarding_table[k];
			printf("\temulator: %s, %s\n\tdestination: %s, %s, %s\n\tnext hop: %s, %s, %s\n\tdelay: %f, loss: %f\n\n", 
							entry.emulator_hostname, entry.emulator_port,
							entry.destination_hostname, entry.destination_port, entry.destination_IP,
							entry.next_hostname, entry.next_port, entry.next_IP,
							entry.delay, entry.loss_prob);
		}
	}
	
	fclose(in_file);
	return 0;
}


/**
 * Checks the priority of the packet and either puts the packet into the
 * specified queue or drops the packet and returns an error.
 * @param pkt The packet to add to a Queue
 * @param index The index of the forwarding table associated with the packet
 */
void queue_packet(packet pkt, int index) {
	packet_plus pkt_plus;
	pkt_plus.pkt = pkt;
	pkt_plus.fwd_table_index = index;
	/*
	 * Check the priority of the packet. Add to the queues. 
	 * If a queue is full, log the message.  If the packet priority 
	 * is invalid, log the message
	 */
	switch(pkt.priority) {
	  case 1:
		if ( enqueue(p1_queue, pkt_plus) == -1) {
			if (debug) printf("P1 queue was full. Logging.\n");
			log_entry("P1 Queue was full", pkt);
		}
		break;
	  case 2:
		if ( enqueue(p2_queue, pkt_plus) == -1) {
			if (debug) printf("P2 queue was full. Logging.\n");
			log_entry("P2 Queue was full", pkt);
		}
		break;
	  case 3:
		if ( enqueue(p3_queue, pkt_plus) == -1) {
			if (debug) printf("P3 queue was full. Logging.\n");
			log_entry("P3 Queue was full", pkt);
		}
		break;
	  default:
		if (debug) printf("Packet priority was invalid. Logging.\n");
		log_entry("Packet priority was invalid", pkt);
	}
}


/**
 * Gets and returns a random number in between the range 
 * of min and max
 * @param min The lower bound value to return
 * @param max The upper bound value to return
 * @return An integer between min and max
 */
int getRandom(int min, int max) {
	static bool init = false;
	int rc;
	
	if (!init) {
		srand(time(NULL));
		init = true;
	}
	
	rc = (rand() % (max - min + 1) + min);
	
	return rc;
}

/**
 * Deal with the delay of the packet.  If the time the packet has been delayed is
 * greater than or equal to the delay time for the packet's forwarding table entry, 
 * then randomly decide to send it off or drop it.
 * @return true if the delay should continue on, false if the packet is dropped or sent
 */
bool dealWithDelay(int socketFD_Emulator) {
	struct timeb cur_time;
	char cur_timeString[80];
	ftime(&cur_time);
	strftime(cur_timeString, sizeof(cur_timeString), "%H:%M:%S", localtime(&(cur_time.time)));
	double duration = difftime(cur_time.time, delay_start.time);
	double mills = cur_time.millitm - delay_start.millitm;
	duration += (mills / 1000.0);
	
	int delayed_pkt_fwd_index = delayed_pkt.fwd_table_index;
	bool end_delay = (duration >= forwarding_table[delayed_pkt_fwd_index].delay);
	
	if (end_delay) {
		if (debug) {
			printf("Should be ending the delay and sending packet to %s\n", forwarding_table[delayed_pkt_fwd_index].next_IP);
		}
		
		int loss_prob = 100 - forwarding_table[delayed_pkt_fwd_index].loss_prob;
		int random;
		random = getRandom(0, 100);
		
		bool drop = (random > loss_prob);
		if (drop && delayed_pkt.pkt.type != 'R' && delayed_pkt.pkt.type != 'E') {
			if (debug) {
				printf("Randomly dropping a packet.\n");
			}
			packet log = delayed_pkt.pkt;
			log_entry("Random loss event occurred", log);
		}
		else {
			if (debug) {
				printf("Sending packet to next hop.\n");
			}
			
			/*
			 * Fills out a sockaddr_in that we will use to send 
			 * the packet to the next hop.  Fill out the 
			 * ip addr and port of the struct using the forwarding table
			 * and send the data using sendto()
			 */ 
			struct sockaddr_in sock_sendto;
			socklen_t sendto_len;
			sock_sendto.sin_family = AF_INET;
			sock_sendto.sin_port = htons( atoi(forwarding_table[delayed_pkt_fwd_index].next_port) );
			inet_pton(AF_INET, forwarding_table[delayed_pkt_fwd_index].next_IP, &sock_sendto.sin_addr);
			memset(sock_sendto.sin_zero, '\0', sizeof(sock_sendto.sin_zero));
			
			char* sendPkt = malloc(P2_HEADERSIZE+HEADERSIZE+delayed_pkt.pkt.length);
			serializePacket(delayed_pkt.pkt, sendPkt);
			sendto_len = sizeof(sock_sendto);
			
			// Send the packet to the next hop
			if ( sendto(socketFD_Emulator, sendPkt, P2_HEADERSIZE+HEADERSIZE+delayed_pkt.pkt.length, 0, 
								(struct sockaddr*) &sock_sendto, sendto_len) == -1 ) {
				perror("sendto()");
			}
		}
		return false;
	}
	
	return true;
}

/**
 * Delay the top priority packet, looking at the three queues in the program.
 * Sets the delayed_pkt struct, and returns true if there was a packet delayed.
 * @return true if a packet is now being delayed, false otherwise
 */
bool delayPacket() {
	bool packet_delayed = false;
	
	if ( !isEmpty(p1_queue) ) {
		if (debug) printf("Delaying packet from p1 queue.\n");
		delayed_pkt = first(p1_queue);
		dequeue(p1_queue);
		packet_delayed = true;
	}
	else if ( !isEmpty(p2_queue) ) {
		if (debug) printf("Delaying packet from p2 queue.\n");
		delayed_pkt = first(p2_queue);
		dequeue(p2_queue);
		packet_delayed = true;
	}
	else if ( !isEmpty(p3_queue) ) {
		if (debug) printf("Delaying packet from p3 queue.\n");
		delayed_pkt = first(p3_queue);
		dequeue(p3_queue);
		packet_delayed = true;
	}
	
	// If we got a delayed packet, set the start time
	if (packet_delayed) {
		ftime(&delay_start);
		strftime(delay_timeString, sizeof(delay_timeString), "%H:%M:%S", localtime(&(delay_start.time)));
	}
	
	return packet_delayed;
}


int main(int argc, char *argv[]) {
	char *buffer;
	buffer = malloc(MAXPACKETSIZE);
	if(buffer == NULL) {
		printError("Buffer could not be allocated");
		return 0;
	}
	if(argc != 11) {
		printError("Incorrect number of arguments");
		usage_Emulator(argv[0]);
		return 0;
	}

	char* port;					//port of emulator
	int queue_size;				//length of esch queue
	char* filename;				//name of file with forwarding table
	//char* log_file;				
	//bool debug = false;

	// Get the commandline args
	int c;
	opterr = 0;
	while ((c = getopt(argc, argv, "p:q:f:l:d:")) != -1) {
		switch (c) {
		case 'p':
			port = optarg;
			break;
		case 'q':
			queue_size = atoi(optarg);
			p1_queue = newQueue(queue_size);
			p2_queue = newQueue(queue_size);
			p3_queue = newQueue(queue_size);
			break;
		case 'f':
			filename = optarg;
			break;
		case 'l':
			log_file = optarg;
			clearFile(log_file);
			break;
		case 'd':
			if (atoi(optarg) == 1) {
				debug = true;
			}
			break;
		default:
			usage_Emulator(argv[0]);
		}
	}
	
	char hostname[255];
	gethostname(hostname, 255);
	readForwardingTable(filename, hostname, port);


	int socketFD_Emulator;

	struct addrinfo hints, *p, *servinfo;
	int rv, numbytes;
	struct sockaddr_storage addr;
	socklen_t addr_len;
	// char s[INET6_ADDRSTRLEN];
	
	// Used to bind to the port on this host as a 'server'
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;
	if ( (rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	

	
	// loop through all the results and bind to the first that we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((socketFD_Emulator = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("sender: socket");
			continue;
		}
		
		fcntl(socketFD_Emulator, F_SETFL, O_NONBLOCK);
		if (bind(socketFD_Emulator, p->ai_addr, p->ai_addrlen) == -1) {
			close(socketFD_Emulator);
			perror("sender: bind");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "sender: failed to bind socket\n");
		return -1;
	}

	freeaddrinfo(servinfo);

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(socketFD_Emulator, &readfds);
	bool packet_delayed = false;
	
	
	while (true) {
		addr_len = sizeof(addr);
		if ((numbytes = recvfrom(socketFD_Emulator, buffer, MAXPACKETSIZE, 0, 
						(struct sockaddr*)&addr, &addr_len)) <= -1) {
			
			// Deal with the delay or set one up
			if (packet_delayed) {
				packet_delayed = dealWithDelay(socketFD_Emulator);
			}
			
			else {
				packet_delayed = delayPacket();
			}
		}
		else {
			if (debug) {
				printf("emulator: got packet from %s\n", get_ip_address( (struct sockaddr*) &addr )); 
				printf("emulator: packet is %d bytes long\n", numbytes);
			}
			packet pkt = getPktFromBuffer(buffer);
			
			if (debug) {
				print_packet(pkt);
			}
			
			bool no_entry_found = true;
			int index;
			for (index = 0; index < forwarding_table_size; index++) {
				forwarding_entry entry = forwarding_table[index];
				
				// The destinations do not line up; drop packet and log issue.
				if ( (strcmp(entry.destination_IP, pkt.destIP) != 0) || 
					  (strcmp(entry.destination_port, pkt.destPort) != 0) ) {
					//log_no_entry_found(pkt.destIP, pkt.destPort, pkt.srcIP, pkt.srcPort, pkt.priority, pkt.length);
				}
				else {
					if (debug) {
						printf("A forwarding table-packet destination match was found.\n");
					}
				  
					no_entry_found = false;
					// Queue the packet 
					queue_packet(pkt, index);
					
					// Deal with the delay or set one up
					if (packet_delayed) {
						packet_delayed = dealWithDelay(socketFD_Emulator);
					}
					else {
						packet_delayed = delayPacket();
					}
				}
	
			}
			
			// If no destination was found, drop packet and log issue
			if (no_entry_found) {
				log_no_entry_found(pkt.destIP, pkt.destPort, pkt.srcIP, pkt.srcPort, pkt.priority, pkt.length);
			}
		}
	}


	
	return 0;
}	// end main
