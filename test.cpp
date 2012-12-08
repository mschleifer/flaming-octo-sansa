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
		
		TestClass(string startkey, string endkey) {
			this->startkey = startkey;
			this->endkey = endkey;
			this->cost = 100000;
		}
		
		TestClass(string startkey, string endkey, vector<Node> path, int cost) {
			this->startkey = startkey;
			this->endkey = endkey;
			this->path = path;
			this->cost = cost;
		}
		
		void findPath(Topology topology) {
			//cout << "findPath in TestClass: " << endl << endl;
			Node start = topology.getNode(startkey);
			vector<Node> startNeighbors = start.getNeighbors();
			
			
			queue< map<string, Node> > pathQueue;
			vector<string> visitedKeys;
			vector< map<string, Node> > pathQueueVector;
			visitedKeys.push_back(startkey);
			
			// add neighbors to the queue
			for (unsigned int i = 0; i < start.getNeighbors().size(); i++) {
				map<string, Node> map;
				map[startkey] = topology.getNode(startNeighbors[i].getKey());
				pathQueue.push(map);
				pathQueueVector.push_back(map);
				visitedKeys.push_back(startNeighbors[i].getKey());
			}
			
			while (!pathQueue.empty()) {
				//cout << "pathQueue size: " << pathQueue.size() << endl;
				map<string, Node> testMap = pathQueue.front();
				Node node = testMap.begin()->second;
				
				//cout << "node.toString: " << node.toString() << endl;
				if (node.getOnline() && node.compareTo(this->endkey) == 0) {
					//cout << "final node: " << node.toString() << endl;
					//cout << "final node key: " << node.getKey() << endl;
					
					this->path.push_back(node);
					this->cost = 1;
					string endStr = this->endkey;
					for (unsigned int index = 0; index < pathQueueVector.size(); index++) {
						map<string, Node> poppedMap = pathQueueVector[index];
						//cout << "parentKey: " << poppedMap.begin()->first << ", ";
						//cout << "key: " << poppedMap.begin()->second.getKey() << endl;
						if (poppedMap.begin()->second.getKey().compare(endStr) == 0) {
							string parentkey = poppedMap.begin()->first;
							this->path.push_back(topology.getNode(parentkey));
							this->cost++;
							//cout << "endStr: " << endStr << ", parentkey: " << parentkey << endl;
							endStr = parentkey;
							index = -1;
						}
					}
					
					reverse(this->path.begin(), this->path.end());
					return;
				}
				
				vector<Node> neighbors = node.getNeighbors();
				for (unsigned int j = 0; j < neighbors.size(); j++ ) {
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
			}
			
			
// 			pathQueue.push_back(
		}
		
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