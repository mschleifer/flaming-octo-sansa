#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include<sys/timeb.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>

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



void
printPacketInfo(int requesterIP, int sequenceNumber) {//, char* payload) {
	//TODO: Print some info about the sender
	//TODO: These parameters might not be quite right or we might want
	//	differnt ones/names/etc.
	struct timeb time;
    	ftime(&time);
    	char timeString[80];
    	strftime(timeString, sizeof(timeString), "%H:%M:%S", localtime(&(time.time)));
    	printf("TIME: %s:%d\nREQUESTER IP: %i\nSEQUENCE NUBMER: %i\nPAYLOAD: \n", timeString, time.millitm, requesterIP, sequenceNumber);
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
	
	/* Print args */
	printf("Port: %i\nFileOption: %s\n", port, fileOption);

	// CREATE SOCKET
  	int socketFD;
 	 socketFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // 17 is UDP???
  	if(socketFD == -1) {
   	 perror("socket");
   	 close(socketFD);
  	}
  	else {
   	 printf("Socket created. FD: %i\n", socketFD);
  	}
  
  	// BIND SOCKET
  	struct sockaddr_in address;
  
  	/* type of socket created in socket() */
  	address.sin_family = AF_INET;
  	address.sin_addr.s_addr = INADDR_ANY;
 	 /* 7000 is the port to use for connections */
  	address.sin_port = htons(port);
 	 /* bind the socket to the port specified above */
  	if(bind(socketFD,(struct sockaddr *)&address,sizeof(address)) == -1) {
   		perror("bind");
  		close(socketFD);
  	}
  	else {
    	printf("Socket bound. FD: %i\n", socketFD);
  	}
  	int addrLength = sizeof(struct sockaddr_in);
	while(1) {
    	if(recvfrom(socketFD, buffer, BUFFER, 0, (struct sockaddr *)&address, (socklen_t *) &addrLength) == -1) {
      		perror("recvfrom");
    	}
  	}	

	return 0;
}
