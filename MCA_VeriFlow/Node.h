#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>
#include <algorithm>

class Node {
	static int nodeCount;
	static int topologyID;
	public:
		Node(int TopologyIndex, bool SwitchNode, std::string ip, std::vector<std::string> LinkList);
		Node();
		~Node();

		bool operator==(const Node& other) const {
			return (this->IP == other.IP);
		}

		void setDomainNode(bool domainNode, std::string topologies);
		bool isSwitch();
		bool isDomainNode();
		bool isMatchingDomain(Node node);
		bool isEmptyNode();
		bool removeLink(std::string IP);
		bool hasAdjacentController();
		void setControllerAdjacency(bool value);
		void setPingResult(bool value);

		int getTopologyID();
		bool getPingResult();
		std::string getIP();
		std::vector<std::string> getLinks();

		std::string print();
		std::string filePrint();

	private:
		int							topologyIndex;	// Which topology this node belongs to
		int							privateNodeID;
		bool						switchNode;
		bool						domainNode;
		std::string					linkingTopologies;
		bool						controllerAdjacency;
		std::string					IP;
		bool						pingResult;
		std::vector<std::string>	linkList;
};

#endif