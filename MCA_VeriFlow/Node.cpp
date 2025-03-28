#include "Node.h"

// Initialize static vars (since inline doesn't work on unix)
int Node::nodeCount = 0;
int Node::topologyID = 0;

Node::Node(int TopologyIndex, bool SwitchNode, std::string DatapathID, std::string ip,
	bool Enddevice, std::vector<std::string> LinkList, std::vector<std::string> PortList) {

	switchNode =			SwitchNode;
	datapathID =			DatapathID;
	IP =					ip;
	endDevice =				Enddevice;
	linkList =				LinkList;
	portList =				PortList;
	domainNode =			false;
	controllerAdjacency =	false;

	// Assign node identifiers based on number of nodes
	privateNodeID = nodeCount;
	nodeCount++;

	// Assign topology identifier
	topologyIndex = TopologyIndex;
}

Node::~Node() {

}

void Node::setDomainNode(bool DomainNode) {
	domainNode = DomainNode;
}

bool Node::isSwitch() {
	return switchNode;
}

bool Node::isDomainNode() {
	return domainNode;
}

bool Node::isMatchingDomain(Node n)
{
	if (n.getTopologyID() == topologyIndex) {
		return true;
	}
	return false;
}

int Node::getTopologyID()
{
	return topologyIndex;
}

bool Node::hasAdjacentController()
{
	return controllerAdjacency;
}

void Node::setControllerAdjacency(bool value)
{
	controllerAdjacency = value;
}



std::string Node::getDatapathID()
{
	return datapathID;
}

std::string Node::getIP()
{
	return IP;
}

bool Node::isEndDevice()
{
	return endDevice;
}

std::vector<std::string> Node::getLinkList()
{
	std::vector<std::string> returnList;
	for (int i = 0; i < linkList.size(); i++) {
		returnList.push_back(portList.at(i));
		returnList.push_back(linkList.at(i));
		// Returns a list with format: port0, link0, port1, link1, ...
	} 
	return returnList;
}

std::string Node::print()
{
	std::string isSwitch = (switchNode ? "[Switch-" : "[Host-");
	std::string isEndDevice = (endDevice ? "[ENDDEVICE] " : "");
	std::string links = "Links: [ ";
	for (int i = 0; i < linkList.size(); i++) {
		links += linkList.at(i) + " ";
	}
	links += "]\n";

	if (linkList.size() == 0) {
		links = "";
	}

	return isSwitch + datapathID + "] " + IP + " " + isEndDevice + "\n" + links;
}
