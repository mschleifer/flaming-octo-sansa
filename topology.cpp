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
	
	
	void addStartingNodes(vector<Node> nodes) {
		for (unsigned int i = 0; i < nodes.size(); i++) {
			Node node = nodes[i];
			string nodeKey = node.getKey();
		
		  if (topology_nodes.count(nodeKey) == 0) {
			  Node n = Node(node);//	node.getHostname(), node.getPort(), node.getOnline());
			  topology_nodes[nodeKey] = n;
			  cout << &topology_nodes[nodeKey] << endl;
			  if (debug) {
				  //cout << "added node to topology" << endl;
			  }
		  }
		}
	}
	
	void addNeighbors(Node parentNode) {
		string nodeKey = parentNode.getKey();
		Node& parentNodePtr = getNode(nodeKey);
		cout << &parentNodePtr << endl;
		
		vector<Node> neighbors = parentNode.getNeighbors();
		for (unsigned int i = 0; i < neighbors.size(); i++) {
			Node neighbor = neighbors[i];
			nodeKey = neighbor.getKey();
			Node &neighborPtr = getNode(nodeKey);
 			cout << &neighborPtr << endl;
 			parentNodePtr.addNeighbor(neighborPtr);
		}
		
	}
	
	/**
	 * Adds a node to the topology. All neighbors and etc are added
	 * automatically.
	 */
	void addNode(Node node) {
		string nodeKey = node.getKey();
		
		/*if (topology_nodes.count(nodeKey) == 0) {
			Node n = Node(node.getHostname(), node.getPort(), node.getOnline());
			topology_nodes[nodeKey] = n;
			if (debug) {
				//cout << "added node to topology" << endl;
			}
		}*/
		
		vector<Node> neighbors = node.getNeighbors();
		//node.getNeighbors().clear();
		for (unsigned int i = 0; i < neighbors.size(); i++) {
			//cout << neighbors[i].toString() << endl;
			string key = neighbors[i].getKey();
			Node &nNode = getNode(key);
			topology_nodes[nodeKey].addNeighbor(nNode);
			cout << "foo";
		}
		
	}
	
	Node& getNode(string hostname, string port) {
		string nodeKey = this->getKey(hostname, port);
		return getNode(nodeKey);
	}
	
	Node& getNode(string nodeKey) {
		if (topology_nodes.count(nodeKey) == 1) {
			return topology_nodes[nodeKey];
		}

		exit(-1);
	}
	
	void disableNode(string hostname, string port) {
		string nodeKey = this->getKey(hostname, port);
		if (topology_nodes.count(nodeKey) == 1) {
			Node &node = topology_nodes[nodeKey]; 
			cout << &node << endl;
			node.setOffline();
		}
	}
	
	void disableNode(Node node) {
		string nodeKey = node.getKey();
		if (topology_nodes.count(nodeKey) == 1) {
			topology_nodes[nodeKey].setOffline();
		}
	}
	
	void enableNode(Node node) {
		string nodeKey = node.getKey();
		if (topology_nodes.count(nodeKey) == 1) {
			topology_nodes[nodeKey].setOnline();
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


/*

	string contents = file_get_contents(filename);

		vector<string> lines = explode("\n", contents);
		for(unsigned int i = 0; i < lines.size(); i++) {
			vector<string> nodes = explode(" ", lines.at(i));

			vector<string> node = explode(",", nodes.at(0));
			Node n = Node().host(node[0]).port(atoi(node[1].c_str())).online(true);
			this->add(n);

			Node &me = this->get(n);

			for(unsigned int j = 1; j < nodes.size(); j++) {
				vector<string> node = explode(",", nodes.at(j));
				// Node n = Node().host(node[0]).port(atoi(node[1].c_str())).online(true);
				int p = atoi(node[1].c_str());
				this->add(node[0], p);
				Node &neighb = this->get(node[0], p);

				me.neighbor(neighb);
			}

		}
	};
	
	*/