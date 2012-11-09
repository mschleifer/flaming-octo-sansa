#ifndef STRUCTS_H
#define STRUCTS_H

// Packets as defined in the project specifications
typedef struct packet {
	uint8_t priority;
	char srcIP[32];
	char srcPort[16];
	char destIP[32];
	char destPost[16];
	uint32_t new_length;
  char type;
  uint32_t sequence;
  uint32_t length;
  char* payload;
} packet;

// Each can hold a row of data from the tracker.txt file
typedef struct tracker_entry {
  char file_name[32];
  int sequence_id;
  char sender_hostname[32];
  //int sender_port;
	char sender_port[32];
} tracker_entry;

// Holds summary information about each sender
typedef struct sender_summary {
  char* sender_ip;
  int sender_port;
  int num_data_pkts;
  int num_bytes;
  double packets_per_second;
  double duration;
  struct timeb start_time;
  char start_timeString[80];
  struct timeb end_time;
  char end_timeString[80];
  int done_sending;
} sender_summary;


// Holds information for the forwarding table (emulator)
typedef struct forwarding_entry {
	char emulator_hostname[32];
	char emulator_port[32];
	char destination_hostname[32];
	char destination_port[32];
	char next_hostname[32];
	char next_port[32];
	uint32_t delay;
	float loss_prob;
} forwarding_entry;


// To be filled out and passed into logger (emulator)
typedef struct log_info {
	char src_hostname[32];
	char src_port[32];
	char destination_hostname[32];
	char destination_port[32];
	struct timeb time;
	char timeString[80];
	uint8_t priority;
	uint32_t size;
} log_info;

#endif
