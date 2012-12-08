#include <string>
#include <iostream>
#include <vector>
#include <queue>
#include <sstream>
#include <map>
#include <algorithm>

#ifndef UTIL_HPP
#define UTIL_HPP
#endif

using namespace std;


class TestClass {
	public:
		
		TestClass(string startkey, string endkey, Topology topology) {
			this->startkey = startkey;
			this->endkey = endkey;
			this->cost = 999999;
			this->findPath(topology);
		}
		
		TestClass(string startkey, string endkey, vector<Node> path, int cost) {
			this->startkey = startkey;
			this->endkey = endkey;
			this->path = path;
			this->cost = cost;
		}
		
		
		/**
		 * Finds the path for this startkey and endkey, given the specific
		 * Topology. The cheapest path will be found.
		 */
		void findPath(Topology topology) {
			//cout << "findPath in TestClass: " << endl << endl;
			Node start = topology.getNode(this->startkey);
			
			// If starting node if offline, do nothing
			if (!start.getOnline()) {
				return;
			}
			
			
			// Queue used to do BFS.  
			queue< map<string, Node> > pathQueue;
			
			// Vector to keep track of already visited nodes
			vector<string> visitedKeys;
			visitedKeys.push_back(startkey);
			
			// Vector to mirror the Queue.  Used when solution is found.
			vector< map<string, Node> > pathQueueVector;
			
			
			
			// starting neighbors to get this thing started
			vector<Node> startNeighbors = start.getNeighbors();
			
			// Add neighbor data to the various structures
			for (unsigned int i = 0; i < startNeighbors.size(); i++) {
				// Don't do anything for offline Nodes
				if (!startNeighbors[i].getOnline()) {
					continue;
				}
				
				map<string, Node> map;
				map[startkey] = topology.getNode(startNeighbors[i].getKey());
				pathQueue.push(map);
				pathQueueVector.push_back(map);
				visitedKeys.push_back(startNeighbors[i].getKey());
			}
			
			// Normal BFS algorithm with some funky things going on
			while (!pathQueue.empty()) {
				//cout << "pathQueue size: " << pathQueue.size() << endl;
				map<string, Node> testMap = pathQueue.front();
				Node node = testMap.begin()->second;
				
				//cout << "node.toString: " << node.toString() << endl;
				
				// End case; found online node that is the endKey
				if (node.getOnline() && node.compareTo(this->endkey) == 0) {
					//cout << "final node: " << node.toString() << endl;
					//cout << "final node key: " << node.getKey() << endl;
					
					// Start filling out the path (in reverse)
					this->path.push_back(node);
					this->cost = 1;
					string endStr = this->endkey;
					
					// Go through the vector and look for 'parents'
					for (unsigned int index = 0; index < pathQueueVector.size(); index++) {
						map<string, Node> poppedMap = pathQueueVector[index];
						//cout << "parentKey: " << poppedMap.begin()->first << ", ";
						//cout << "key: " << poppedMap.begin()->second.getKey() << endl;
						
						// We found an entry that matched; go through loop again
						if (poppedMap.begin()->second.getKey().compare(endStr) == 0) {
							string parentkey = poppedMap.begin()->first;
							this->path.push_back(topology.getNode(parentkey));
							this->cost++;
							//cout << "endStr: " << endStr << ", parentkey: " << parentkey << endl;
							endStr = parentkey;
							index = -1;		// a complete hack job
						}
					}
					
					// We were filling the path in reverse; this fixes that.
					reverse(this->path.begin(), this->path.end());
					return;
				}
				
				// Continue with BFS.  Put neighbors into queue and other structs
				vector<Node> neighbors = node.getNeighbors();
				for (unsigned int j = 0; j < neighbors.size(); j++ ) {
					// Do nothing for offline Nodes
					if (!neighbors[j].getOnline()) {
						continue;
					}
					
					// if !(visitedKeys.contains(key)) ..
					if (!(std::find(visitedKeys.begin(), visitedKeys.end(), neighbors[j].getKey()) != visitedKeys.end())) {
						visitedKeys.push_back(neighbors[j].getKey());
						// cout << neighbors[j].toString() << endl;
						map<string, Node> map;
						map[node.getKey()] = topology.getNode(neighbors[j].getKey());
						pathQueue.push(map);
						pathQueueVector.push_back(map);
					}
				  
				}
				
				pathQueue.pop();
			
			} // while !Q.empty()
		}
		
		
		/**********************************************************************
		 * GETTERS AND SETTERS (Java style)
		 *********************************************************************/
		
		void setStartkey(string startkey) {
			this->startkey = startkey;
		}
		
		string getStartkey() {
			return this->startkey;
		}
		
		void setEndkey(string endkey) {
			this->endkey = endkey;
		}
		
		string getEndkey() {
			return this->endkey;
		}
		
		void setPath(vector<Node> path) {
			this->path = path;
		}
		
		vector<Node> getPath() {
			return this->path;
		}
		
		void setCost(int cost) {
			this->cost = cost;
		}
		
		int getCost() {
			return this->cost;
		}
	  
	  
		/**
		 * Simple toString(). Will return the startkey, endkey, cost,
		 * and if viable, the path.
		 */
		string toString() {
			stringstream out;
			out << "TestClass:" << endl;
			out << "startkey: " << startkey << endl;
			out << "endkey: " << endkey << endl;
			out << "Cost: " << cost << endl;
			
			for (unsigned int i = 0; i < path.size(); i++) {
				out << path[i].toStringNodeOnly() << " > ";
			}
			
			string toString = out.str();
			toString.erase(toString.size()-2);
			toString += "\n";
			return toString;
		}
		
		
	private:
		string startkey;
		string endkey;
		vector<Node> path;
		int cost;
};