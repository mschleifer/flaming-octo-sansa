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
#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>

forwarding_entry* forwarding_table;
int forwarding_table_size;


int logger(char* log_file, char* reason, log_info info) {
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
  
	fprintf(fp, "%s: src::port: %s::%s, dest::port: %s::%s, time: %s.%d, priority: %d, size: %d\n", 
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
 * Reads the contents of the given file (assumed to hold the forwarding table)
 * into an array in memory.  
 * Table will consider only options in table with same <hostname, port> combo.
 */
int readForwardingTable(char* filename, char* hostname, char* port, bool debug) {
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
	while( fscanf(in_file, "%s %s %s %s %s %s %d %f", 
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
    	forwarding_table[forwarding_table_size] = entry;
    	forwarding_table_size++;
		}
	}
	
	if (debug) {
		printf("forwarding table:\n");
		for (k = 0; k < forwarding_table_size; k++) {
			entry = forwarding_table[k];
			printf("\temulator: %s, %s\n\tdestination: %s, %s\n\tnext hop: %s, %s\n\tdelay: %d, loss: %f\n\n", 
							entry.emulator_hostname, entry.emulator_port,
							entry.destination_hostname, entry.destination_port,
							entry.next_hostname, entry.next_port,
							entry.delay, entry.loss_prob);


			// An example of setting up for and then calling the logger to log a problem
						// (will not be here in general, just needed to test it)
			log_info info;
			strcpy(info.src_hostname, entry.emulator_hostname);
			strcpy(info.src_port, entry.emulator_port);
			strcpy(info.destination_hostname, entry.destination_hostname);
			strcpy(info.destination_port, entry.destination_port);
			ftime(&info.time);
			strftime(info.timeString, sizeof(info.timeString), "%H:%M:%S", localtime(&(info.time.time)));
			info.priority = 1;
			info.size = 100;
			logger("logger.txt", "reasoning", info);
		}
	}
	
	fclose(in_file);
	return 0;
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

	char* port;						//port of emulator
	int queue_size;				//length of esch queue
	char* filename;				//name of file with forwarding table
	char* log_file;				//name of the log file
	bool debug = false;

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
	readForwardingTable(filename, hostname, port, debug);


	int socketFD_Emulator;

	struct addrinfo hints, *p, *servinfo;
	int rv, numbytes;
	struct sockaddr_storage addr;
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];
	
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

	while (true) {
		//printf("foo");
		/*if (select(socketFD_Emulator + 1, &readfds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(-1);
		}*/

		
		addr_len = sizeof(addr);
		if ((numbytes = recvfrom(socketFD_Emulator, buffer, MAXPACKETSIZE, 0, 
						(struct sockaddr*)&addr, &addr_len)) == -1) {
				//perror("recvfrom");
				//exit(1);
		}
		else {
			printf("emulator: got packet from %s\n", inet_ntop(addr.ss_family, 
				get_in_addr((struct sockaddr*)&addr), s, sizeof(s)));
			printf("emulator: packet is %d bytes long\n", numbytes);
		}
		/*if ( FD_ISSET(socketFD_Emulator, &readfds) ) {
			if ((numbytes = recvfrom(socketFD_Emulator, buffer, MAXPACKETSIZE, 0, 
						(struct sockaddr*)&addr, &addr_len)) == -1) {
				perror("recvfrom");
				exit(1);
			}

			else {
				printf("got data from something");
			}
		}*/
	}		// end while(true)
	

	
	/*if ((numbytes = recvfrom(socketFD_Emulator, buffer, MAXPACKETSIZE, 0, 
			(struct sockaddr*)&addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}*/

	
	printf("emulator: got packet from %s\n", inet_ntop(addr.ss_family, 
				get_in_addr((struct sockaddr*)&addr), s, sizeof(s)));
	printf("emulator: packet is %d bytes long\n", numbytes);

	
	return 0;
}	// end main
