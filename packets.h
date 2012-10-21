#ifndef PACKETS_H
#define PACKETS_H

// Packets as defined in the project specifications
typedef struct packet {
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
  int sender_port;
} tracker_entry;

// Holds summary information about each sender
typedef struct sender_summary {
  char* sender_ip;
  int sender_port;
  int num_data_pkts;
  int num_bytes;
  double packets_per_second;
  double duration;   // this needs to be something else
  struct timeb start_time;
  char start_timeString[80];
  struct timeb end_time;
  char end_timeString[80];
} sender_summary;



#endif
