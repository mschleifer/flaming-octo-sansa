#ifndef HELPERS_H
#define HELPERS_H

#define BUFFER (5120)
#define MAXPACKETSIZE (5137)
#define HEADERSIZE (17)

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
usage(char *prog) {
	fprintf(stderr, "usage: %s -p <port> -q <queue_size> -f <filename> -l <log> -d <debug>\n", prog);
	exit(1);
}

#endif
