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
#define MAXPACKETSIZE (5137)
#define HEADERSIZE (17)
#define MAXPAYLOADSIZE (5120)
//array and array size tracker for global use
tracker_entry* tracker_array; 
int tracker_array_size;

// sender array for global use
sender_summary* sender_array;
int sender_array_size = 0;

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
int writeToFile(char* payload, char* file_name) {
  FILE *fp; 
  fp = fopen(file_name, "r+"); // Open file for read/write
  if (!fp) {
    perror("fopen");
    return -1;
  }
  if(fseek(fp, -1, SEEK_END) < 0) {
    fp = fopen(file_name, "r+"); // Nothing in file yet so reopen for read/write
  }
  
  fprintf(fp, "%s", payload);
  
  if (fclose(fp) != 0) {
    perror("fclose");
    return -1;
  }
  return 0;
}

/** 
 * Reads the tracker file and puts it into an array of structs, one for each
 * row in the table.It also sorts the table so that any rows with the same 
 * file name have sorted sequence numbers.
 * @return 0 if there are no issues, -1 if there is a problem
 */
int readTrackerFile() {
  int k;
  tracker_array = (tracker_entry*)malloc(sizeof(tracker_entry) * 100);//setting max size to 100.
  FILE *in_file = fopen("tracker.txt", "r");//read only
  tracker_array_size = 0;
  
  //test for not existing
  if (in_file == NULL) {
    printf("Error.Could not open file\n");
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
  
  fclose(in_file);
  return 0;
}

/**
 * Should be called for each packet that is sent to the requester.
 * Prints out the time, IP, sequence number and 4 bytes of the payload.
 */
int printInfoAtReceive(char* sender_ip, packet pkt) {
  printf("\n");
  struct timeb time;
  ftime(&time);
  char timeString[80];
  strftime(timeString, sizeof(timeString), "%H:%M:%S", localtime(&(time.time)));
  
  // Print E pkt info if that's the case
  if (pkt.type == 'E') {
     printf("Received END from %s packet at: %s.%d(ms).\n", sender_ip, timeString, time.millitm);
  }
  // For a data pkt, print out data as required
  else {
    printf("Received packet at: %s.%d(ms).\n\tSender IP: %s.\n\tSequence number: %d.\n\tLength: %d.\n\t",
	 timeString, time.millitm, sender_ip, pkt.sequence, pkt.length);
    printf("First 4 bytes of payload: %c%c%c%c\n", pkt.payload[0], pkt.payload[1], pkt.payload[2], pkt.payload[3]);
  }
  return 0;
}

/**
 * Searches through the sender array and prints out the information about each
 * sender, one at a time. New sender info will be on its' own line.
 */
int printSummaryInfo(struct sockaddr_in server) {
  int i;
  for (i = 0; i < sender_array_size; i++) {
    sender_summary s_info = sender_array[i];
    if ((strcmp(s_info.sender_ip, inet_ntoa(server.sin_addr)) == 0) &&
	    (s_info.sender_port == server.sin_port)) {
    
      double duration = difftime(s_info.end_time.time, s_info.start_time.time);
      double mills = s_info.end_time.millitm - s_info.start_time.millitm;
      duration += (mills / 1000.0);
    
      s_info.duration = duration;
      s_info.packets_per_second = ((double) s_info.num_data_pkts) / duration;
      
      printf("\n");
      printf("Info for sender %s:\n\tNum data packets: %d\n\tNum bytes received: %d\n\tAverage pkts per second: %f\n\tDuration: %f\n",
	     s_info.sender_ip, s_info.num_data_pkts, s_info.num_bytes,
	     s_info.packets_per_second, s_info.duration);
    }
    
    sender_array[i] = s_info; 
  }
  
  return 0;
}

int
main(int argc, char *argv[])
{
  char *buffer;
  buffer = malloc(MAXPACKETSIZE);
  bzero(buffer, sizeof(buffer));
  sender_array = (sender_summary*) malloc(sizeof(sender_summary) * 20);
  if(buffer == NULL) {
    printError("Buffer could not be allocated");
    return 0;
  }
  if(argc != 5) {
    printError("Incorrect number of arguments");
    return 0;
  }
  
  // Port on which the requester waits for packets
  int port= 0;
  // The name of the file that's being requested
  char* requested_file_name = malloc(MAXPAYLOADSIZE);
  
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
    printf("Error reading from tracker file.Exiting.\n");
    exit(-1);
  }
  
  // Clears the file once every time the program is run
  clearFile(requested_file_name);
  
  
  // CREATE REQUESTER ADDRESS
  int socketFD_Client;
  struct sockaddr_in client, server;
  int slen=sizeof(server);
  bzero(&client, sizeof(client));
  client.sin_family = AF_INET;
  client.sin_port = htons(port);
  client.sin_addr.s_addr = INADDR_ANY;
  printf("testing: %s", inet_ntoa(client.sin_addr));
  
  //if (inet_aton(/*client.sin_addr.s_addr*/SRV_IP, &client.sin_addr) == 0) {
  //  fprintf(stderr, "inet_aton() failed\n");
  //  exit(-1);
  //}

  // CREATE REQUESTER SOCKET
  socketFD_Client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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
  
  printf("testing: %s", inet_ntoa(client.sin_addr));
  /* 
   * Loops forever waiting for a message from recvfrom
   */
  bool done_requesting = false;
  while (1) {
    int i;
    for(i = 0; i < tracker_array_size && !done_requesting; i++) {
      
      /**
       * Request the given file name only. 
       * Sends the request in the form of a packet.
       */
      if(strcmp(tracker_array[i].file_name, requested_file_name) == 0) {
        address_server.sin_port = htons(tracker_array[i].sender_port);
	
	if (inet_aton(SRV_IP, &address_server.sin_addr)==0) {
	  fprintf(stderr, "inet_aton() failed\n");
	  exit(1);
	}
	
        // DEBUG print
	//printf("sin_addr and sin_port: %s %d\n", inet_ntoa(address_server.sin_addr), address_server.sin_port);
	
        // Fill out a struct for the request packet
	packet request;
	request.type = 'R';
	request.sequence = 0;
	request.length = 20;
      	request.payload = requested_file_name;
	
	// Serialize the request packet for sending
	uint payloadSize = strlen(requested_file_name);
	char* requestPacket = malloc(HEADERSIZE+payloadSize);
	memcpy(requestPacket, &request.type, sizeof(char));
	memcpy(requestPacket+1, &request.sequence, sizeof(uint32_t));
	memcpy(requestPacket+9, &payloadSize, sizeof(uint32_t));
	memcpy(requestPacket+HEADERSIZE, request.payload, payloadSize);
	
	usleep(100);
	// Send the request packet to the sender 	
	if (sendto(socketFD_Client, requestPacket, HEADERSIZE+payloadSize, 0, (struct sockaddr *)&address_server, sizeof(address_server))==-1) {
	  perror("sendto()");
	}
      }
      
      // Stop the application from endlessly requesting the same files
      if (i == tracker_array_size - 1) {
	done_requesting = true;
      }
    }
    
    bzero(buffer, MAXPACKETSIZE);
    // Listen for some kind of response.If one is given, fill in info
    if (recvfrom(socketFD_Client, buffer, MAXPACKETSIZE, 0, (struct sockaddr *)&server, (socklen_t *)&slen) == -1) {
      perror("recvfrom()");
    }
    
    
    // Create a packet from the received data
    packet PACKET;
    memcpy(&PACKET.type, buffer, sizeof(char));
    memcpy(&PACKET.sequence, buffer+1, sizeof(uint32_t));
    memcpy(&PACKET.length, buffer+9, sizeof(uint32_t));
    PACKET.payload = buffer+HEADERSIZE;
    
    printInfoAtReceive(inet_ntoa(server.sin_addr), PACKET);
    
    // If it's a DATA packet
    if (PACKET.type == 'D') {
      writeToFile(PACKET.payload, requested_file_name);
      
      // Add the information to the sender array. If the sender is not
      // currently in the array, add it to the array. 
      bool in_sender_array = false;
      int i;
      for (i = 0; i < sender_array_size; i++) {
	
        // If this statement succeeds, the sender has sent before
        if ((strcmp(sender_array[i].sender_ip, inet_ntoa(server.sin_addr)) == 0) && (sender_array[i].sender_port == server.sin_port)) {
          in_sender_array = true;
          sender_array[i].num_data_pkts++;
          sender_array[i].num_bytes += PACKET.length;
        }
      }
      
      // New sender; fill out info, add to sender array
      if (!in_sender_array) {
        sender_summary sender_details;
        sender_details.sender_ip = inet_ntoa(server.sin_addr);
        sender_details.sender_port = server.sin_port;
        sender_details.num_data_pkts = 1;
	sender_details.done_sending = 0;
        sender_details.num_bytes = PACKET.length;

        // Deal with start time for the sender
        ftime(&sender_details.start_time);
        strftime(sender_details.start_timeString, sizeof(sender_details.start_timeString), 
		 "%H:%M:%S", localtime(&(sender_details.start_time.time)));
	
        //printf("Start time is: %s.%d(ms).\n", sender_details.start_timeString, sender_details.start_time.millitm);
	
	
	// Put into the sender array
	sender_array[sender_array_size] = sender_details;
	sender_array_size++;
      }
    }
    // Other it should be an END packet
    else if (PACKET.type == 'E') {
      
      int i;
      bool all_done = true;
      for (i = 0; i < sender_array_size; i++) {
	
        // Only do this for the sender that sent the 'E' packet.
        if ((strcmp(sender_array[i].sender_ip, inet_ntoa(server.sin_addr)) == 0) && (sender_array[i].sender_port == server.sin_port)) {
	  
	  sender_array[i].done_sending = 1;
          // Deal with end time for the sender
          ftime(&sender_array[i].end_time);
          strftime(sender_array[i].end_timeString, 
		   sizeof(sender_array[i].end_timeString), 
		   "%H:%M:%S", 
		   localtime(&(sender_array[i].end_time.time)));
	  
          //printf("End time is: %s.%d(ms).\n", sender_array[i].end_timeString, sender_array[i].end_time.millitm);
        }
      }	
      
      /*
       * This loop checks to see if all senders are done sending.
       * If they any is not, all_done is false (we are not done)
       */
      for (i = 0; i < sender_array_size; i++) {
	if (sender_array[i].done_sending != 1) {
	  all_done = false;
	}
      }
      
      // For the sender that just finished
      printSummaryInfo(server);
      if (all_done) {
	goto end;
      }
      
    }
  }
  
 end:
  close(socketFD_Client);
  return 0;
} // END MAIN

