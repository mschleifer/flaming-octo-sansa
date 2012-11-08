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
#include <netdb.h>
#include <stdbool.h>

#define SRV_IP "127.0.0.1"
#define MAXPACKETSIZE (5137)
#define HEADERSIZE (17)
#define MAXPAYLOADSIZE (5120)
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
 * Helps to get the IP address of a given sockaddr. 
 * Search in file for example usage (with inet_ntop)
 */
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
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
  if(fseek(fp, 0, SEEK_END) < 0) {
    fp = fopen(file_name, "r+"); // Nothing in file yet so reopen for read/write
		//perror("fseek");
		//return -1;
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
  while( fscanf(in_file, "%s %d %s %s", entry.file_name, &entry.sequence_id, entry.sender_hostname, 											entry.sender_port) == 4) {
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
				strcpy(entry.sender_port, tracker_array[k].sender_port);
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
 * Prints out information about the sender given.
 */
int printSummaryInfo(struct sockaddr_in server, sender_summary sender) {
    if ((strcmp(sender.sender_ip, inet_ntoa(server.sin_addr)) == 0) &&
	    (sender.sender_port == server.sin_port)) {
    
      double duration = difftime(sender.end_time.time, sender.start_time.time);
      double mills = sender.end_time.millitm - sender.start_time.millitm;
      duration += (mills / 1000.0);
    
      sender.duration = duration;
      sender.packets_per_second = ((double) sender.num_data_pkts) / duration;
      
      printf("\n");
      printf("Info for sender %s:\n\tNum data packets: %d\n\tNum bytes received: %d\n\tAverage pkts per second: %f\n\tDuration: %f\n-------------------------------------------------\n",
	     sender.sender_ip, sender.num_data_pkts, sender.num_bytes,
	     sender.packets_per_second, sender.duration);
    }
  
  return 0;
}

int
main(int argc, char *argv[])
{
  char *buffer;
  buffer = malloc(MAXPACKETSIZE);
  bzero(buffer, sizeof(buffer));
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
	char* port_str;
  // The name of the file that's being requested
  char* requested_file_name = malloc(MAXPAYLOADSIZE);
  
  // Deal with command-line arguments
  int c;
  opterr = 0;
  while ((c = getopt(argc, argv, "p:o:")) != -1) {
    switch(c) {
    case 'p':
      port = atoi(optarg);
      	port_str = optarg;
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
  
  
  int socketFD_Client;
  struct sockaddr_in server;
  int slen=sizeof(server);

  int rv, numbytes;
  struct addrinfo hints;
  struct addrinfo *servinfo, *p;
  
  bzero(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  
	// Hostname used for binding.  May want to bind to other hostname (tracking sheet?)
  char hostname[255];
  gethostname(hostname, 255);
  printf("requester: hostname is %s\n", hostname);

  int i;
  for(i = 0; i < tracker_array_size; i++) {
      
    /**
     * Request the given file name only. 
     * Sends the request in the form of a packet.
     */
    if(strcmp(tracker_array[i].file_name, requested_file_name) == 0) {
			if ((rv = getaddrinfo(hostname, tracker_array[i].sender_port, &hints, &servinfo)) != 0) {
  			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
  			return -1;
			}
			

			// loop through all the results and make a socket
			for(p = servinfo; p != NULL; p = p->ai_next) {
  			if ((socketFD_Client = socket(p->ai_family, p->ai_socktype,
		     	  p->ai_protocol)) == -1) {
          	perror("talker: socket");
         	 	continue;
  			}

 		 		break;
			}

			if (p == NULL) {
  			fprintf(stderr, "requester: failed to bind socket.\n");
  			return -1;
			}

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

			// Send the request packet to the sender 	
			if ((numbytes = sendto(socketFD_Client, requestPacket, 
									HEADERSIZE+payloadSize, 0, p->ai_addr, p->ai_addrlen))==-1) {
			  perror("requester: sendto");
 			 	return -1;
			}

			/*char s[INET6_ADDRSTRLEN];
			printf("requester: sent packet to %s\n", inet_ntop(AF_INET,
						 get_in_addr((struct sockaddr*)p->ai_addr),
						 s, sizeof(s)));*/


			bzero(buffer, MAXPACKETSIZE);

			bool done_with_sender = false;
			bool first_packet = true;
			sender_summary pkt_sender;
			bzero(&pkt_sender, sizeof(sender_summary));

			// We go until we get an 'E' packet
			while (!done_with_sender) {
				bzero(buffer, MAXPACKETSIZE);
	  		// Listen for some kind of response. If one is given, fill in info
	  		if (recvfrom(socketFD_Client, buffer, MAXPACKETSIZE, 0, 
								(struct sockaddr *)&server, (socklen_t *)&slen) == -1) {
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
	    	
					// The first packet from the sender
					if (first_packet) {
						first_packet = false;
						pkt_sender.sender_ip = inet_ntoa(server.sin_addr);
						pkt_sender.sender_port = server.sin_port;
						pkt_sender.num_data_pkts = 1;
						pkt_sender.done_sending = 0;
						pkt_sender.num_bytes = PACKET.length;

						ftime(&pkt_sender.start_time);
	      		strftime(pkt_sender.start_timeString, sizeof(pkt_sender.start_timeString), 
		 							"%H:%M:%S", localtime(&(pkt_sender.start_time.time)));
					}

					else {
						pkt_sender.num_data_pkts++;
						pkt_sender.num_bytes += PACKET.length;
					}
	   
	  		} // END IF D-Packet
	  		else if (PACKET.type == 'E') {
	    		done_with_sender = true;
					pkt_sender.done_sending = 1;
					ftime(&pkt_sender.end_time);
					strftime(pkt_sender.end_timeString, sizeof(pkt_sender.end_timeString), "%H:%M:%S", 
										localtime(&(pkt_sender.end_time.time)));
					printSummaryInfo(server, pkt_sender);
	  		} // END ELSE-IF E-Packet
						
			} 	// END while not done with sender
   	
		}	 		// END strcmp
        
  } 			// END for each in tracker array

  close(socketFD_Client);
  return 0;
} // END MAIN

