/*
 * sender.c
 *
 *  Created on: Sep. 30 2012
 *  
 *  Matthew Schleifer
 *  Adam Eggum
 *
 *
 sender -p <port> -g <requester port> -r <rate> -q <seq_no> -l <length>

    port is the port on which the sender waits for requests,
    requester port is the port on which the requester is waiting,
    rate is the number of packets to be sent per second,
    seq_no is the initial sequence of the packet exchange, and
    length is the length of the payload in the packets (each chunk of the file part that the sender has),

 Additional notes for the parameters:

    sender and requester port should be in this range: 1024<port<65536
    for implementing the rate parameter the sending interval should be evenly distributed, i.e. when rate is 10 packets per second the sender 
    has to send one packet at about every 100 milliseconds. It should not send them all in a short time and wait for the remaining time in the second.

 The sender must print the following information for each packet sent to the requester, with each packet's information in a separate line.

    The time that the packet was sent with milisecond granularity,
    The IP of the requester,
    The sequence number, and
    The first 4 bytes of the payload 
 */

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
    fprintf(stderr, "usage: %s -p <port> -g <requester port> -r <rate> -q <seq_no> -l <length>\n", prog);
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
    	printf("%s:%d", timeString, time.millitm);
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
	if(argc != 11) {
		printError("Incorrect number of aruegments");
		return 0;
	}

	// arguments
    int port  = 0;
    int requesterPort = 0;
    int rate = 0;
    int sequenceNumber = 0;
    int length = 0;

    // input params
    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "p:g:r:q:l:")) != -1) {
		switch (c) {
		case 'p':
	    	port = atoi(optarg);
	    	break;
		case 'g':
	    	requesterPort = atoi(optarg);
	    	break;
		case 'r':
	    	rate = atoi(optarg);
	    	break;
		case 'q':
	    	sequenceNumber = atoi(optarg);
	    	break;
		case 'l':
	    	length = atoi(optarg);
	    	break;
		default:
	    	usage(argv[0]);
		}
    }
    if(port < 1024 || port > 65536) {
	    	printError("Incorrect port number");
	    	return 0;
	    }
	if(requesterPort < 1024 || requesterPort > 65536) {
	    	printError("Incorrent requester port number");
	    	return 0;
	    }
    	printf("Port: %i\n Requester Port: %i\n Rate: %i\n Seq_no: %i\n Length: %i\n", port, requesterPort, rate, sequenceNumber, length);

	return 0;
}

