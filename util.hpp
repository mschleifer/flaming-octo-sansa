#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <sstream>
#include <stdlib.h>
#include <netdb.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/timeb.h>
#include <cstdlib>

using namespace std;

/**************************************************
 * CPP utilities and other functions can go here
 *************************************************/

/**
 * Returns the hostname of the current machine.
 * I think this only works in the mumble labs.
 */
string getHostname() {
	char hostname[255];
	gethostname(hostname, 255);
	string str_hostname(hostname);
	int strIndex = str_hostname.find_first_of(".");
	str_hostname = str_hostname.substr(0,strIndex);	
	return str_hostname;
}

/**
 *  Gets the ip of the current machine.
 */
string getIP() {
	char hostname[255];
	gethostname(hostname, 255);

	struct hostent *he;

	if ((he = gethostbyname(hostname)) == NULL) {
		cout << "ERROR: error getting localhost's IP." << endl;
		exit(1);
	}

	in_addr *address = (in_addr *)he->h_addr_list[0];
	string ip_address = inet_ntoa(*address);

	return ip_address;
}