#ifndef PACKETS_H
#define PACKETS_H

// Packets as defined in the project specifications
typedef struct packet {
  char type;
  uint32_t sequence;
  uint32_t length;
  unsigned char* payload;
} packet;

// Each can hold a row of data from the tracker.txt file
typedef struct tracker_entry {
  char file_name[32];
  int sequence_id;
  char sender_hostname[32];
  int sender_port;
} tracker_entry;

#endif
