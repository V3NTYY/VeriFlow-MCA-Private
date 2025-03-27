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
