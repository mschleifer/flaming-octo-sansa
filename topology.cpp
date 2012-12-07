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
	 * Adds a node to the topology. All neighbors and etc are added
	 * automatically.
	 */
	void addNode(Node node) {
		string nodeKey = node.getKey();
		
		if (topology_nodes.count(nodeKey) == 0) {
			topology_nodes[nodeKey] = node;
			if (debug) {
				//cout << "added node to topology" << endl;
			}
		}
		
		/*vector<Node> neighbors = node.getNeighbors();
		for (unsigned int i = 0; i < neighbors.size(); i++) {
			cout << neighbors[i].toString() << endl;
		}*/
		
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