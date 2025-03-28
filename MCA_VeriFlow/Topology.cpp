#include "Topology.h"

bool Topology::addNode(Node node)
{
	std::vector<Node> newTopology;

	// Add an empty topology if none exist
	if (topologyList.empty()) {
		topologyList.push_back(newTopology);
	}

	// If the topology ID doesn't exist, create new topologies until it does
	while (node.getTopologyID() >= topologyList.size()) {
		topologyList.push_back(newTopology);
	}

	// Add the node to the topology
	topologyList[node.getTopologyID()].push_back(node);

	return true;
}

Node Topology::getNodeByIP(std::string IP)
{
	Node emptyNode(-1, false, "-1", "-1", false, std::vector<std::string>(), std::vector<std::string>());

	// Iterate through all topologies
	for (int i = 0; i < topologyList.size(); i++) {
		// Iterate through all nodes in the topology
		for (int j = 0; j < topologyList[i].size(); j++) {
			// If the IP matches, return the node
			if (topologyList[i][j].getIP() == IP) {
				return topologyList[i][j];
			}
		}
	}

	return emptyNode;
}

Node* Topology::getNodeReference(Node n)
{
	// Iterate through all topologies
	for (int i = 0; i < topologyList.size(); i++) {
		// Iterate through all nodes in the topology
		for (int j = 0; j < topologyList[i].size(); j++) {
			// If the IP matches, return the node
			if (topologyList[i][j] == n) {
				return &topologyList[i][j];
			}
		}
	}

	return nullptr;
}

std::vector<Node> Topology::getTopology(int index)
{
	// Ensure index exists
	if (index < 0 || index >= topologyList.size()) {
		return std::vector<Node>();
	}

	return topologyList[index];
}

int Topology::getTopologyCount()
{
	return topologyList.size();
}

void Topology::clear()
{
	topologyList.clear();
}

std::string Topology::printTopology(int index)
{
	// Ensure index exists
	if (index < 0 || index >= topologyList.size()) {
		return "Topology does not exist";
	}

	std::string output = "";
	for (int i = 0; i < topologyList[index].size(); i++) {
		output += topologyList[index][i].print() + "\n";
	}

	return output;
}
