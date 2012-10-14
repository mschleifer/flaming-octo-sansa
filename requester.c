#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include<sys/timeb.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include "packets.h"
#include <arpa/inet.h>

#define SRV_IP "127.0.0.1"
#define BUFFER (512)
//array and array size tracker for global use
tracker_entry* tracker_array; 
int tracker_array_size;

void
printError(char* errorMessage) {
	fprintf(stderr, "An error has occured: %s\n", errorMessage);
}

void
usage(char *prog) {
    fprintf(stderr, "usage: %s -p <port> -o <file option>\n", prog);
    exit(1);
}

int readTrackerFile() {
  printf("\n-----------------------\n\nReading 'tracker.txt' into array of structs\n");
  
  tracker_array = (tracker_entry*)malloc(sizeof(tracker_entry) * 100);  //setting max size to 100.
  FILE *in_file = fopen("tracker.txt", "r");  //read only
  tracker_array_size = 0;
  
  //test for not existing
  if (in_file == NULL) {
    printf("Error.  Could not open file\n");
    return -1;
  }
  
  
  //read each row into struct, insert into array, increment size
  tracker_entry entry;  
  while( fscanf(in_file, "%s %d %s %d", entry.file_name, &entry.sequence_id, entry.sender_hostname, &entry.sender_port) == 4) {
    tracker_array[tracker_array_size] = entry;
    tracker_array_size++;
  }
  
  printf("tracker array/table size: %d\n", tracker_array_size);
  int i = 0;
  for (i = 0; i < tracker_array_size; i++) {
    printf("Row %d: %s, %d, %s, %d\n", i, tracker_array[i].file_name, tracker_array[i].sequence_id, tracker_array[i].sender_hostname, tracker_array[i].sender_port);
  }
  fclose(in_file);
  printf("\n---------------------------\nDone reading from tracker file.\n");
  return 0;
}

/**
 * Should be called for each packet that is sent to the requester.  
 * Prints out the time, IP, sequence number and 4 bytes of the payload.
 * TODO: We need to print a section of the payload
 */
int printInfoAtSend(int requester_ip, packet pkt) {
  struct timeb time;
  ftime(&time);
  char timeString[80];
  strftime(timeString, sizeof(timeString), "%H:%M:%S", localtime(&(time.time)));
  printf("Sending packet at: %s.%d(ms).  Requester IP: %d.  Sequence number: %d.  Payload: (null)\n",
	 timeString, time.millitm, requester_ip, pkt.sequence);
  return 0;
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
  
  // Port on which the requester waits for packets
  int port  = 0;
  // The name of the file that's being requested
  char* requested_file_name = malloc(BUFFER);
  
  // Deal with command-line arguments
  int c;
  opterr = 0;
  while ((c = getopt(argc, argv, "p:o:")) != -1) {
    switch(c) {
    case 'p':
      port = atoi(optarg);
      break;
    case 'o':
      requested_file_name = optarg;
      break;
    default: 
      usage(argv[0]);
    }
  }

  if(port < 1024 || port > 65536) {
    printError("Incorrect port number");
    return 0;
  }
  
  // Read from tracker.txt 
  if (readTrackerFile() == -1) {
    printf("Error reading from tracker file.  Exiting.");
    exit(-1);
  }
  
  /* Print args */
  printf("Client Port: %i\nRequested filename: %s\n", port, requested_file_name);
  
  // CREATE SOCKET
  int socketFD_Client;
  socketFD_Client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // 17 is UDP???
  if(socketFD_Client == -1) {
    perror("socket");
    close(socketFD_Client);
  }

  int i;

  
  for(i = 0; i < tracker_array_size; i++) {
    if(strcmp(tracker_array[i].file_name, requested_file_name) == 0) {
      struct sockaddr_in address_server;
      
      memset((char *) &address_server, 0, sizeof(address_server));
      address_server.sin_family = AF_INET;
      address_server.sin_port = htons(tracker_array[i].sender_port);
      if (inet_aton(SRV_IP, &address_server.sin_addr)==0) {
	fprintf(stderr, "inet_aton() failed\n");
	exit(1);
      }
      int j;
      for (j=0; j<1; j++) {
	packet PACKET;
	PACKET.type = 'D';
	PACKET.sequence = 1;
	PACKET.length = 1;
	memcpy(buffer, &PACKET, sizeof(packet));
	
	// Print info and then send the packet to the requester
	printInfoAtSend(10, PACKET);
	if (sendto(socketFD_Client, buffer, BUFFER, 0, (struct sockaddr *)&address_server, sizeof(address_server))==-1)
	  perror("sendto()");
      }
    }
  }
  
  // SET UP sockaddr_in WITH CORRESPONDING SERVER SOCKETS
  
  
  /*
  // BIND SOCKET
  // type of socket created in socket()
  address_server.sin_family = AF_INET;
  address_server.sin_addr.s_addr = INADDR_ANY;
  // 7000 is the port to use for connections
  address_server.sin_port = htons(port);
  // bind the socket to the port specified above
  if(bind(socketFD_Client,(struct sockaddr *)&address_server,sizeof(address_server)) == -1) {
  perror("bind");
  close(socketFD_Client);
  }
  else {
  printf("Socket bound. FD: %i\n", socketFD_Client);
  }
  
  // Read from tracker.txt 
  if (readTrackerFile() == -1) {
  printf("Error reading from tracker file.  Exiting.");
  exit(-1);
  }
  
  int addrLength = sizeof(struct sockaddr_in);
  while(1) {
  if(recvfrom(socketFD_Client, buffer, BUFFER, 0, (struct sockaddr *)&address_server, (socklen_t *) &addrLength) == -1) {
  perror("recvfrom");
  }
  }
  */
  close(socketFD_Client);
  return 0;
}
