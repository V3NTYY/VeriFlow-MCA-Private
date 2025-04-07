#include "Topology.h"

std::vector<Node> Topology::string_toTopology(std::string payload)
{
	return std::vector<Node>();
}

std::string Topology::topology_toString(int index)
{
	return std::string();
}

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

	return Node();
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

Topology Topology::extractIndexTopology(int index)
{
	// Ensure index exists
	if (index < 0 || index >= topologyList.size()) {
		return Topology();
	}

	Topology returnTopology;
	returnTopology.topologyList.push_back(topologyList[index]);

	return returnTopology;
}

bool Topology::outputToFile(std::string filename)
{
	// Create the output file
	std::ofstream outputFile;
	outputFile.open(filename);

	// Return false if the file couldn't be created
	if (!outputFile.is_open()) {
		return false;
	}

	outputFile << "# Format: id ipAddress endDevice(0=false,1=true) port1 nextHopIpAddress1 port2 nextHopIpAddress2 ...\n";
	outputFile << "# The line '#NEW' separates topologies, and creates a new one\n";
	outputFile << "# The line '#CA' signifies that the next node is adjacent to the controller\n\n";

	// Iterate through all topologies
	for (int i = 0; i < topologyList.size(); i++) {
		// Don't add the #NEW line if it's the first topology
		outputFile << ((i == 0) ? "" : "#NEW\n");

		// Iterate through all nodes in the topology
		for (int j = 0; j < topologyList[i].size(); j++) {
			outputFile << topologyList[i][j].filePrint() << std::endl;
		}
	}

	return true;
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
