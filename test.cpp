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
			cout << "findPath in TestClass" << endl;
			Node start = topology.getNode(startkey);
			vector<Node> startNeighbors = start.getNeighbors();
			
			
			queue< map<string, Node> > pathQueue;
			vector<string> visitedKeys;
			visitedKeys.push_back(startkey);
			
			for (unsigned int i = 0; i < start.getNeighbors().size(); i++) {
				
				// if a neighbor matches and is online, we're done w/ cost=1
				/*if (startNeighbors[i].getOnline() && startNeighbors[i].compareTo(endkey) == 0) {
					this->path.push_back(startNeighbors[i]);
					this->cost = 1;
					return;
				}*/
				map<string, Node> map;
				map[startkey] = topology.getNode(startNeighbors[i].getKey());
				//cout << topology.getNode(startNeighbors[i].getKey()).toString() << endl;
				
				// start filling the pathQueue just in case
				//pathQueue.push(topology.getNode(startNeighbors[i].getKey()));
				pathQueue.push(map);
				visitedKeys.push_back(startNeighbors[i].getKey());
			}
			
			cout << "pathQueue size: " << pathQueue.size() << endl;
			while (!pathQueue.empty()) {
				map<string, Node> testMap = pathQueue.front();
				Node node = testMap.begin()->second;
				pathQueue.pop();
				
				cout << "node.toString: " << node.toString() << endl;
				if (node.getOnline() && node.compareTo(endkey) == 0) {
					this->path.push_back(node);
					this->cost = 1;
					return;
				}
				
				vector<Node> neighbors = node.getNeighbors();
				for (unsigned int j = 0; j < neighbors.size(); j++ ) {
					// if !(visitedKeys.contains(key)) ..
					if (!(std::find(visitedKeys.begin(), visitedKeys.end(), neighbors[j].getKey()) != visitedKeys.end())) {
						visitedKeys.push_back(neighbors[j].getKey());
						cout << neighbors[j].toString() << endl;
						map<string, Node> map;
						map[node.getKey()] = topology.getNode(neighbors[j].getKey());
						//pathQueue.push(topology.getNode(neighbors[j].getKey()));
						pathQueue.push(map);
					}
				  
				}
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