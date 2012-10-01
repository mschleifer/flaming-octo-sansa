#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include<sys/timeb.h>

#define BUFFER (514)

void
printError(char* errorMessage) {
	fprintf(stderr, "An error has occured: %s\n", errorMessage);
}

void
usage(char *prog) {
    fprintf(stderr, "usage: %s -p <port> -o <file option>\n", prog);
    exit(1);
}

int
main(int argc, char *argv[])
{
	char *buffer;
	buffer = malloc(BUFFER);
	if(buffer == NULL) {
		printError("Buffer could not be allocated");
		return 0;
	}
	if(argc != 5) {
		printError("Incorrect number of arguments");
		return 0;
	}

	// arguments
	int port  = 0;
	char* fileOption = malloc(BUFFER);

	// input params
	int c;
	opterr = 0;
	while ((c = getopt(argc, argv, "p:o:")) != -1) {
		switch(c) {
		case 'p':
			port = atoi(optarg);
			break;
		case 'o':
			fileOption = optarg;
			break;
		default: 
			usage(argv[0]);
		}
	}

	printf("Port: %i\nFileOption: %s\n", port, fileOption);
}
