#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include "packets.h"
#include <arpa/inet.h>
#include <stdbool.h>

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

/**
 * Writes what is given to the given file name. 
 * Overwrites what is in the file.
 * @return 0 if there are no issues, -1 otherwise
 */
int writeToFile(packet pkt, char* file_name) {
  FILE *fp; 
  fp = fopen(file_name, "a+"); // Append to the file
  if (!fp) {
    perror("fopen");
    return -1;
  }

  fprintf(fp, "Packet info: \n\tType: %c\n\tSequence: %d\n\tLength: %d\n",
	  pkt.type, pkt.sequence, pkt.length);

  if (fclose(fp) != 0) {
    perror("fclose");
    return -1;
  }
  return 0;
}

/** 
 * Reads the tracker file and puts it into an array of structs, one for each
 * row in the table.  It also sorts the table so that any rows with the same 
 * file name have sorted sequence numbers.
 * @return 0 if there are no issues, -1 if there is a problem
 */
int readTrackerFile() {
  //printf("\n-----------------------\n\nReading 'tracker.txt' into array of structs\n");
  int i, k;
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
  
  /**
   * Go through the array and make sure any table entries with the same 
   * file name are sorted by their sequence numbers.
   */
  for (k = 0; k < tracker_array_size - 1; k++) {
    if (strcmp(tracker_array[k].file_name, tracker_array[k+1].file_name) == 0) {
      
      // If they are out of order, put them in order, restart loop
      if (tracker_array[k].sequence_id > tracker_array[k+1].sequence_id) {
	strcpy(entry.file_name, tracker_array[k].file_name);
	entry.sequence_id = tracker_array[k].sequence_id;
	strcpy(entry.sender_hostname, tracker_array[k].sender_hostname);
	entry.sender_port = tracker_array[k].sender_port;
	tracker_array[k] = tracker_array[k+1];
	tracker_array[k+1] = entry;
	k = 0;
      }
    }
  }
  //printf("tracker array/table size: %d\n", tracker_array_size);

  for (i = 0; i < tracker_array_size; i++) {
    //printf("Row %d: %s, %d, %s, %d\n", i, tracker_array[i].file_name, tracker_array[i].sequence_id, tracker_array[i].sender_hostname, tracker_array[i].sender_port);
    }
  
  fclose(in_file);
  //printf("\n---------------------------\nDone reading from tracker file.\n");
  return 0;
}

/**
 * Should be called for each packet that is sent to the requester.  
 * Prints out the time, IP, sequence number and 4 bytes of the payload.
 * TODO: We need to print a section of the payload..not sure how to do that
 */
int printInfoAtReceive(char* sender_ip, packet pkt) {
  struct timeb time;
  ftime(&time);
  char timeString[80];
  strftime(timeString, sizeof(timeString), "%H:%M:%S", localtime(&(time.time)));
  printf("Received packet at: %s.%d(ms).  Sender IP: %s.  Sequence number: %d.  Length: %d.  Payload: (null)\n",
	 timeString, time.millitm, sender_ip, pkt.sequence, pkt.length);
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
    printError("Incorrect port number\n");
    return 0;
  }
  
  // Read from tracker.txt 
  if (readTrackerFile() == -1) {
    printf("Error reading from tracker file.  Exiting.\n");
    exit(-1);
  }

  // Clears the file once every time the program is run
  clearFile(requested_file_name);
  
  /* Print args */
  //printf("ARGS: \tClient Port: %i\n\tRequested filename: %s\n", port, requested_file_name);
  
  // CREATE SOCKET
  int socketFD_Client;
  struct sockaddr_in client, server;
  int slen=sizeof(server);
  bzero(&client, sizeof(client));
  client.sin_family = AF_INET;
  client.sin_port = htons(port);
  
  if (inet_aton(SRV_IP, &client.sin_addr) == 0) {
    fprintf(stderr, "inet_aton() failed\n");
    exit(-1);
  }

  socketFD_Client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // 17 is UDP???
  if(socketFD_Client == -1) {
    perror("socket");
    close(socketFD_Client);
  }

  // Socket address to be used when sending file request
  struct sockaddr_in address_server;
  bzero(&address_server, sizeof(address_server));
  address_server.sin_family = AF_INET;

  // Assign the address to the socket
  if (bind(socketFD_Client, (struct sockaddr *)&client, sizeof(client))==-1)
    perror("bind");
  
  /* 
   * Loops forever, but in the end, is simply waiting for a message with
   * recvfrom, so nothing will happen.
   */
  bool done_requesting = false;
  while (1) {
    int i;
    for(i = 0; i < tracker_array_size && !done_requesting; i++) {
      
      /**
       * Request the given file name only. 
       * TODO: I'm not sure if we want to send the request as a packet.
       */
      if(strcmp(tracker_array[i].file_name, requested_file_name) == 0) {
	
	address_server.sin_port = htons(tracker_array[i].sender_port);
	if (inet_aton(SRV_IP, &address_server.sin_addr)==0) {
	  fprintf(stderr, "inet_aton() failed\n");
	  exit(1);
	}
	
	memcpy(buffer, &tracker_array[i].file_name, sizeof(tracker_array[i].file_name));
	printf("Sending message to sender with data: %s\n", tracker_array[i].file_name);
	
	// Send the request to the sender (do we want this to be a packet?)	
	if (sendto(socketFD_Client, buffer, BUFFER, 0, (struct sockaddr *)&address_server, sizeof(address_server))==-1) {
	  perror("sendto()");
	}
      }
      
      // Stop the application from endlessly requesting the same files
      if (i == tracker_array_size - 1) {
	done_requesting = true;
      }
    }
    
    // Listen for some kind of response.  If one is given, fill in info
    if (recvfrom(socketFD_Client, buffer, BUFFER, 0, (struct sockaddr *)&server, (socklen_t *)&slen) == -1) {
      perror("recvfrom()");
    }

    packet PACKET;
    memcpy(&PACKET, buffer, sizeof(packet));
    printInfoAtReceive(inet_ntoa(server.sin_addr), PACKET);

    writeToFile(PACKET, requested_file_name);
  }

  close(socketFD_Client);
  return 0;
}
