#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <map>

#ifndef UTIL_HPP
#define UTIL_HPP
#endif

using namespace std;

/**
 * Node class to be used in the topology.
 * The way I view it, the Topology has Nodes, which have neighbors (of Nodes)
 */
class Node {
	public:
		
		/**
		 * Empty constructor for a Node.  Sets online to be true. 
		 */
		Node() {
			node_online = false;
		}

		/**
		 *  Constructor for Node.  Takes in hostname, port combo.
		 *  Sets online to be true as well.
		 */
		Node(string host_str, int port_int) {
			node_host = host_str;
			node_port = port_int;
			node_online = true;
		}
		
		/**
		 * Third constructor for a Node.  Sets hostname, port, and online
		 * to what is passed in.
		 */
		Node(string host_str, int port_int, bool online_bool) {
			node_host = host_str;
			node_port = port_int;
			node_online = online_bool;
		}
		
		/**
		 * Sets the hostname of the Node.
		 * @return A pointer to the Node
		 */
		Node& setHostname(string host_str) { 
			node_host = host_str; 
			return *this;
		}
		
		/**
		 * Gets the hostname of the Node
		 * @return string hostname of the Node
		 */
		string getHostname() { 
			return node_host;
		}
		
		/**
		 * Sets the port of the Node
		 * @return A pointer to the Node
		 */
		Node& setPort(int port_int) { 
			node_port = port_int; 
			return *this; 
		}
		
		/**
		 * Gets the port of the Node
		 * @return int The port of the Node
		 */
		int getPort() { 
			return node_port; 
		}
		
		/**
		 * Sets the online bool of the Node
		 * @return A pointer to the Node
		 */
		Node& setOnline(bool online_bool) { 
			node_online = online_bool; 
			return *this; 
		}
		
		/**
		 * Gets the online boolean of the Node
		 * @return bool The boolean representing whether the Node is online or not.
		 */
		bool getOnline() { 
			return node_online; 
		}

		/**
		 * Adds the Node to this Nodes' list of neighbors if it does not
		 * already exist in the neighbor list.
		 * @return A pointer to this node
		 */
		Node& neighbor(Node neighbor) {
			string key = this->getKey(neighbor);
			
			// add if doesn't already exist
			if (!node_neighbors.count(key)) {
				node_neighbors[key] = neighbor;
				neighbor.neighbor(*this);
			}

		 	return *this;
		}
		
		/**
		 * Adds the list of neighbor nodes to this nodes list of neighbors.
		 */
		Node& neighbors(vector<Node> neighborsList) {
			
			for(unsigned int i = 0; i < neighborsList.size(); i++) {
				this->neighbor(neighborsList.at(i));
			}
		
			return *this;
		}
		
		/**
		 * Returns a list (vector) of all of the neighbors of the current node.
		 */
		vector<Node> neighbors() {
			map<string, Node>::iterator iter;
			vector<Node> n;
			
			// iterate through, fill out vector as we go
			for(iter = node_neighbors.begin(); iter != node_neighbors.end(); iter++) {
				Node node = iter->second;
				n.push_back(node);
			}
			
			return n;
		}

		/**
		 * Compares two nodes.  If the two have the same host and port, return 0.
		 * Otherwise return -1.
		 * @return 0 if values are same, -1 otherwise.
		 */
		int compare(Node n) {
			if(this->getHostname().compare(n.getHostname()) == 0 && this->getPort() == n.getPort()) {
				return 0;
			} else {
				return -1;
			}
		}

		
		/**
		 * A toString for the Node.  Prints out the Node along with all neighbors.
		 * @return A String representation of the Node.
		 */	
		string toString() {
			stringstream out;
			map<string, Node>::iterator iter;
			
			out << "Node ( " << node_host << " - " << node_port << " - " << node_online << " ) --> "; 
			out << "{ ";
			
			for(iter = node_neighbors.begin(); iter != node_neighbors.end(); iter++) {
				Node n = iter->second;
				
				out << "( " << n.getHostname() << " - " << n.getPort() << " - " << n.getOnline() << " ) ";
				out << ", ";
			}
			
			string toString = out.str();
			//output.erase(output.size()-2);
			
			toString += "}";
			
			return toString;
		}
		
	private:
		string node_host;
		int node_port;
		bool node_online;
		map<string, Node> node_neighbors;
		
		
		/**
		 * Returns the host, port combo of the Node.
		 * The format is 'key:port'.
		 */
		string getKey(string host, int port) {
			stringstream key;
			key << host << ":" << port;
			return key.str();
		}
		
		/**
		 * Returns the host, port combo of the Node.
		 * The format returned is 'key:port'.
		 */
		string getKey(Node n) {
			stringstream key;
			return this->getKey(n.getHostname(), n.getPort());
		}
};