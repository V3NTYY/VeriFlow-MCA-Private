#include "Node.h"

// Initialize static vars (since inline doesn't work on unix)
int Node::nodeCount = 0;
int Node::topologyID = 0;

Node::Node(int TopologyIndex, bool SwitchNode, std::string ip, std::vector<std::string> LinkList) {

	switchNode =			SwitchNode;
	IP =					ip;
	linkList =				LinkList;
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
	IP =					"-1";
	linkList =				std::vector<std::string>();
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

std::string Node::getIP()
{
	return IP;
}

std::string Node::print()
{
	std::string isSwitch = (switchNode ? "[Switch] " : "[Host] ");
	std::string links = "Links: [ ";
	for (int i = 0; i < linkList.size(); i++) {
		links += linkList.at(i) + " ";
	}
	links += "]\n";

	if (linkList.size() == 0) {
		links = "";
	}

	std::string domainNodeString = (domainNode ? ("Domains: [ " + linkingTopologies + " ]\n") : "");

	return isSwitch + IP + "\n" + links + domainNodeString;
}

bool Node::connectsToTopology(int topologyIndex) {
    if (!domainNode || linkingTopologies == "null") {
        return false;
    }

    // Parse the linkingTopologies string
    std::vector<std::string> connectedTopologies;
    std::string current;
    for (char c : linkingTopologies) {
        if (c == ':') {
            if (!current.empty()) {
                connectedTopologies.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        connectedTopologies.push_back(current);
    }

    // Check if topology is in  list
    for (const std::string& topo : connectedTopologies) {
        if (std::stoi(topo) == topologyIndex) return true;
    }

    return false;
}

bool Node::isLinkedTo(std::string IP)
{
	bool success = false;
	for (int i = 0; i < linkList.size(); i++) {
		if (linkList.at(i) == IP) {
			success = true;
			break;
		}
	}

    return success;
}

std::vector<std::string> Node::getLinks()
{
	return linkList;
}

void Node::setPingResult(bool value) {
	pingResult = value;
}

bool Node::getPingResult() {
	return pingResult;
}

std::string Node::filePrint()
{
	std::string output = "";
	if (controllerAdjacency) {
		output += "CA#\n";
	}
	
	output += IP + ":";
	for (int i = 0; i < linkList.size(); i++) {
		output += linkList.at(i);
		if (i != linkList.size() - 1) {
			output += ",";
		}
	}

	return output;
}
