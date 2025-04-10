#include "Topology.h"

std::vector<Node> Topology::string_toTopology(std::string payload)
{
	return std::vector<Node>();
}

std::string Topology::topology_toString(int index)
{
	// Ensure index exists
	if (index < 0 || index >= topologyList.size()) {
		return "";
	}

	std::string output = "";

	// Iterate through all nodes in the topology
	for (int j = 0; j < topologyList[index].size(); j++) {
		output += topologyList[index][j].filePrint();
		if (j != topologyList[index].size() - 1) {
			output += "\n";
		}
	}

	return output;
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

	// Iterate through all topologies
	for (int i = 0; i < topologyList.size(); i++) {
		// Don't add the TOP# line if it's the first topology
		outputFile << ((i == 0) ? "" : "TOP#\n");

		// Split the topology list into switches, then hosts. Print switches first, then hosts
		std::vector<Node> switches;
		std::vector<Node> hosts;
		for (int j = 0; j < topologyList[i].size(); j++) {
			if (topologyList[i][j].isSwitch()) {
				switches.push_back(topologyList[i][j]);
			}
			else {
				hosts.push_back(topologyList[i][j]);
			}
		}

		// Print the switches
		outputFile << "S#\n";
		for (int j = 0; j < switches.size(); j++) {
			outputFile << switches[j].filePrint() << "\n";
		}

		// Print the hosts
		outputFile << "H#\n";
		for (int j = 0; j < hosts.size(); j++) {
			outputFile << hosts[j].filePrint() << "\n";
		}

		// Print remaining, unused config
		outputFile << "R#\nE!";
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
