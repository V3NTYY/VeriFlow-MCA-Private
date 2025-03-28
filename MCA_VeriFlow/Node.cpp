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
	linkingTopologies =		"null";
	pingResult =			false;

	// Assign node identifiers based on number of nodes
	privateNodeID = nodeCount;
	nodeCount++;

	// Assign topology identifier
	topologyIndex = TopologyIndex;
}

Node::~Node() {

}

Node::Node()
{
	switchNode =			false;
	datapathID =			-1;
	IP =					"-1";
	endDevice =				false;
	linkList =				std::vector<std::string>();
	portList =				std::vector<std::string>();
	domainNode =			false;
	controllerAdjacency =	false;
	linkingTopologies =		"null";
	privateNodeID =			-1;
	pingResult =			false;

	// Assign topology identifier
	topologyIndex =			-1;
}

bool Node::removeLink(std::string IP) {
	for (int i = 0; i < linkList.size(); i++) {
		if (linkList.at(i) == IP) {
			linkList.erase(linkList.begin() + i);
			portList.erase(portList.begin() + i);
			return true;
		}
	}
	return false;
}

void Node::setDomainNode(bool DomainNode, std::string topologies) {
	/// This method is very messy and terrible. But it works.

	domainNode = DomainNode;
	if (linkingTopologies == "null") {
		linkingTopologies = topologies;
	}
	else {
		// Splice the current list into its values (format is topindex1:topindex2:...)
		std::vector<std::string> splitList;
		std::string temp = "";
		for (int i = 0; i < linkingTopologies.size(); i++) {
			if (linkingTopologies.at(i) == ':') {
				splitList.push_back(temp);
				temp = "";
			}
			else {
				temp += linkingTopologies.at(i);
			}
		}
		splitList.push_back(temp);
		temp = "";
		for (int i = 0; i < topologies.size(); i++) {
			if (topologies.at(i) == ':') {
				splitList.push_back(temp);
				temp = "";
			}
			else {
				temp += topologies.at(i);
			}
		}
		splitList.push_back(temp);

		// Eliminate duplicate values in splitList
		std::sort(splitList.begin(), splitList.end());
		splitList.erase(std::unique(splitList.begin(), splitList.end()), splitList.end());

		// Reappend splitList to linkingTopologies
		linkingTopologies = "";
		for (int i = 0; i < splitList.size(); i++) {
			linkingTopologies += splitList.at(i);
			if (i != splitList.size() - 1) {
				linkingTopologies += ":";
			}
		}
	}
}

bool Node::isSwitch() {
	return switchNode;
}

bool Node::isDomainNode() {
	return domainNode;
}

bool Node::isEmptyNode() {
	if (IP == "-1" || topologyIndex == -1) {
		return true;
	}
	return false;
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

	std::string domainNodeString = (domainNode ? ("Domains: [ " + linkingTopologies + " ]\n") : "");

	return isSwitch + datapathID + "] " + IP + " " + isEndDevice + "\n" + links + domainNodeString;
}

std::vector<std::string> Node::getLinkedIPs()
{
	return linkList;
}

void Node::setEndDevice(bool value)
{
	endDevice = value;
}

void Node::setPingResult(bool value) {
	pingResult = value;
}

bool Node::getPingResult() {
	return pingResult;
}

std::string Node::filePrint()
{
	/// Format: datapathId ipAddress endDevice(0 = false, 1 = true) port1 nextHopIpAddress1 port2 nextHopIpAddress2 ...
	/// #CA means controller adjacency

	std::string output = "";
	if (controllerAdjacency) {
		output += "#CA\n";
	}
	std::string temp = datapathID + " " + IP + " " + (endDevice ? "1" : "0") + " ";
	output += temp;
	if (linkList.size() == portList.size()) {
		for (int i = 0; i < linkList.size(); i++) {
			output += portList.at(i) + " " + linkList.at(i) + " ";
		}
	}
	else {
		output = "# ERROR adding: " + temp;
	}

	return output;
}
