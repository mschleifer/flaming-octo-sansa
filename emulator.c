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
#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>

forwarding_entry* forwarding_table;
int forwarding_table_size;

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
		}
	}
	
	fclose(in_file);
	return 0;
}

void
printError(char* errorMessage) {
	fprintf(stderr, "An error has occured: %s\n", errorMessage);
}

void
usage(char *prog) {
	fprintf(stderr, "usage: %s -p <port> -q <queue_size> -f <filename> -l <log> -d <debug>\n", prog);
	exit(1);
}

int main(int argc, char *argv[]) {
	if(argc != 11) {
		printError("Incorrect number of arguments");
		usage(argv[0]);
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
			break;
		case 'd':
			if (atoi(optarg) == 1) {
				debug = true;
			}
			break;
		default:
			usage(argv[0]);
		}
	}
	
	char hostname[255];
  gethostname(hostname, 255);
	readForwardingTable(filename, hostname, port, debug);
	return 0;
}	// end main
