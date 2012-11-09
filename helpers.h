#ifndef HELPERS_H
#define HELPERS_H

#define BUFFER (5120)
#define MAXPACKETSIZE (5137)
#define HEADERSIZE (17)
#define MAXPAYLOADSIZE (5120)

// get sockaddr, IPv4 or IPv6:
void 
*get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void
printError(char* errorMessage) {
	fprintf(stderr, "An error has occured: %s\n", errorMessage);
}

void
usage_Emulator(char *prog) {
	fprintf(stderr, "usage: %s -p <port> -q <queue_size> -f <filename> -l <log> -d <debug>\n", prog);
	exit(1);
}

void
usage_Requester(char *prog) {
	fprintf(stderr, "usage: %s -p <port> -f <f_hostname> -h <f_port> -o <file_option> -w <window>\n", prog);
	exit(1);
}
void
usage_Sender(char *prog) {
	fprintf(stderr, "usage: %s -p <port> -g <requester_port> -r <rate> -q <seq_no> ", prog);
	fprintf(stderr, "-l <length> -f <f_hostname> -h <f_port> -i <priority> -t <timeout>\n");
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


#endif
