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

bool vectorContains( vector<Node>::iterator itr, vector<Node> list, Node node) {
   for(itr=list.begin(); itr != list.end(); itr++) {
       if((*itr).compareTo(node) == 0) {
           return true;
       }
   }
   return false;
}


/**
 * converts a ip:port key and returns the ip
 */
string keyToIp(string key) {
	string token;
	istringstream ss(key);
	while ( getline(ss, token, ':') ) {
		return token;
	}
	return token;
}

/**
 * returns the port of the ip:port key
 */
string keyToPort(string key) {
	string token;
	istringstream ss(key);
	while ( getline(ss, token, ':') ) {
		
	}
	return token;
}

/**
 * returns a key in the ip:port combo
 */
string getNodeKey(string host, string port) {
		stringstream key;
		key << host << ":" << port;
		return key.str();
}

