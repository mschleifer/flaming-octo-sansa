#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <map>

using namespace std;

class Topology {
  public:	
	Topology(bool debug_flag) {
		this->debug = debug_flag;
	}
	
	
	/**
	 * Adds the starting nodes (main nodes) to the topology.
	 */
	void addNodes(vector<Node> nodes) {
		for (unsigned int i = 0; i < nodes.size(); i++) {
			Node node = nodes[i];
			string nodeKey = node.getKey();
		
		  if (topology_nodes.count(nodeKey) == 0) {
			  Node n = Node(node);
			  topology_nodes[nodeKey] = n;
			
			  if (debug) {
				  //cout << "added node to topology" << endl;
			  }
		  }
		}
	}
	
	
	/**
	 * Adds the neighbors of a node to the topology.
	 */
	void addNeighbors(Node node) {
		string nodeKey = node.getKey();
		
		vector<Node> neighbors = node.getNeighbors();
		for (unsigned int i = 0; i < neighbors.size(); i++) {
			string key = neighbors[i].getKey();
			Node &nNode = getNode(key);
			topology_nodes[nodeKey].addNeighbor(nNode);
		}
		
		
	}
	

	/**
	 * Gets the node with the given key. 
	 * Will abruptly exit if there is nothing found.
	 */
	Node& getNode(string nodeKey) {
		if (topology_nodes.count(nodeKey) == 1) {
			return topology_nodes[nodeKey];
		}

		exit(-1);
	}
	
	
	
	/**
	 * Disables the node with the given hostname and port.
	 * Due to limitations with my skill with C++, very much slower 
	 * than it should be.
	 */
	void disableNode(string nodeKey) {
		vector<string> disabledKeys;
		if (topology_nodes.count(nodeKey) == 1) {
			Node &node = topology_nodes[nodeKey]; 
			node.setOffline();
			disabledKeys = node.getNeighborsKeys();
		}
		
		// makes the node offline for all others containing it; a hack
		for (unsigned int i = 0; i < disabledKeys.size(); i++) {
			Node &node = topology_nodes[disabledKeys[i]];
			Node *disableNode = node.getNeighbor(nodeKey);
			disableNode->setOffline();
		}
	}
	

	
	/**
	 * Enables the node with the given nodeKey
	 * @param nodeKey The key of the node, 'host:port'
	 */
	void enableNode(string nodeKey) {
		vector<string> enabledKeys;
		if (topology_nodes.count(nodeKey) == 1) {
			Node &node = topology_nodes[nodeKey];
			node.setOnline();
			enabledKeys = node.getNeighborsKeys();
		}
		
		// makes the node online for all others containing it; a hack
		for (unsigned int i = 0; i < enabledKeys.size(); i++) {
			Node &node = topology_nodes[enabledKeys[i]];
			Node *enableNode = node.getNeighbor(nodeKey);
			enableNode->setOnline();
		}
	}
	
	
	
	
	
	
	string toString() {
		stringstream out;
		map<string, Node>::iterator iter;
		
		out << "Topology struct:" << endl;
 		for (iter = topology_nodes.begin(); iter != topology_nodes.end(); iter++) {
 			Node node = iter->second;
 			out << node.toString();
			out << endl;
 		}
		
		string toString = out.str();
		return toString;
	}
	
  private:
	map<string, Node> topology_nodes;
	bool debug;
  
	string getKey(string host, string port) {
		stringstream key;
		key << host << ":" << port;
		return key.str();
	}
};