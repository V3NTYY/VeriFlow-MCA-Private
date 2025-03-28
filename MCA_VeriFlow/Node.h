#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>
#include <algorithm>

class Node {
	static int nodeCount;
	static int topologyID;
	public:
		Node(int TopologyIndex, bool SwitchNode, std::string DatapathID, std::string ip,
			bool Enddevice, std::vector<std::string> LinkList, std::vector<std::string> PortList);
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

		bool hasAdjacentController();
		void setControllerAdjacency(bool value);

		int getTopologyID();
		std::string getDatapathID();
		std::string getIP();
		bool isEndDevice();
		std::vector<std::string> getLinkList();
		std::vector<std::string> getLinkedIPs();

		std::string print();

	private:
		int							topologyIndex;	// Which topology this node belongs to
		int							privateNodeID;
		bool						switchNode;
		bool						domainNode;
		std::string					linkingTopologies;
		bool						controllerAdjacency;
		std::string					datapathID;
		std::string					IP;
		bool						endDevice;
		std::vector<std::string>	linkList;
		std::vector<std::string>	portList;
};

#endif