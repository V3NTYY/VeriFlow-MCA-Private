#include "Topology.h"

// Copy-paste of MCA_VeriFlow splitInput function. Not efficient I know but im just trying to save time.
std::vector<std::string> splitInputDupe(std::string input, std::vector<std::string> delimiters) {
	std::vector<std::string> words;
	std::string word;

	// If the line is empty, return early
	if (input.empty()) {
		return words;
	}

	// Iterate through the entire input string. If a delimiter is found, the first half is added as a word. The second half is then checked for delimiters again.
	for (size_t i = 0; i < input.length(); i++) {
		bool isDelimiter = false;
		for (std::string delimiter : delimiters) {
			if (input.substr(i, delimiter.length()) == delimiter) {
				isDelimiter = true;
				if (!word.empty()) {
					words.push_back(word);
					word.clear();
				}
				break;
			}
		}
		if (!isDelimiter) {
			word += input[i];
			// If this is the last word, push it back
			if (i == input.length() - 1) {
				words.push_back(word);
			}
		}
	}

	return words;
}

std::vector<Node> Topology::string_toTopology(std::string payload)
{
	std::vector<Node> topology;
	std::string line;
	std::istringstream stream(payload);
	int topology_index = 0;
	bool isControllerAdjacent = false;
	bool isHost = false; // false = switch, true = host

	while (std::getline(stream, line)) {
		std::vector<std::string> delimiters = { ":", ",", " " };
		std::vector<std::string> args = splitInputDupe(line, delimiters);

		if (args.empty()) {
			continue;
		}

		// This shouldn't be in a digest for string_toTopology purposes
		if (args.at(0) == "TOP#") {
			topology_index++;
			isHost = false;
			continue;
		}
		else if (args.at(0) == "CA#") {
			isControllerAdjacent = true;
		}
		else if (args.at(0) == "S#") {
			isHost = false;
			continue;
		}
		else if (args.at(0) == "H#") {
			isHost = true;
			continue;
		}
		else if (args.at(0) == "R#") {
			// Used to add rules. But we don't add them statically here.
			continue;
		}
		else if (args.at(0) == "E!") {
			// Used to end file reading. But not necessary since we could have multiple topologies.
			continue;
		}

		std::vector<std::string> links;

		// Get node parameters
		std::string ipAddress = args.at(0);
		for (int i = 1; i < args.size(); i++) {
			links.push_back(args.at(i));
		}

		// Create a new node
		Node n = Node(topology_index, !isHost, ipAddress, links);
		n.setControllerAdjacency(isControllerAdjacent);
		isControllerAdjacent = false;
		topology.push_back(n);
	}
	return topology;
}

std::string Topology::topology_toString(int index)
{
	// Ensure index exists
	if (index < 0 || index >= topologyList.size()) {
		return "";
	}

	std::string output = "";
	int topology_index = 0;
	bool isControllerAdjacent = false;
	bool isHost = false; // false = switch, true = host

	// Split the topology list into switches, then hosts. Print switches first, then hosts
	std::vector<Node> switches;
	std::vector<Node> hosts;
	for (int j = 0; j < topologyList[index].size(); j++) {
		if (topologyList[index][j].isSwitch()) {
			switches.push_back(topologyList[index][j]);
		}
		else {
			hosts.push_back(topologyList[index][j]);
		}
	}

	// Print the switches
	output += "S#\n";
	for (int j = 0; j < switches.size(); j++) {
		output += switches[j].filePrint() + "\n";
	}

	// Print the hosts
	output += "H#\n";
	for (int j = 0; j < hosts.size(); j++) {
		output += hosts[j].filePrint() + "\n";
	}

	// Print remaining, unused config
	output += "R#\nE!";

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

Node Topology::getNodeByIP(std::string IP, int index)
{
	// Iterate through all nodes in the topology
	for (int j = 0; j < topologyList[index].size(); j++) {
		// If the IP matches, return the node
		if (topologyList[index][j].getIP() == IP) {
			return topologyList[index][j];
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

bool Topology::isLocal(std::string firstIP, std::string secondIP, bool print)
{
	if (print) {
		loggy << "[CCPDN]: Checking if " << firstIP << " and " << secondIP << " are local." << std::endl;
	}
	// Ensure firstIP exists within current topology
	if (hostIndex < 0 || hostIndex >= topologyList.size()) {
		return false;
	}

	bool firstIPExists = false;
	for (Node n : topologyList[hostIndex]) {
		if (n.getIP() == firstIP) {
			firstIPExists = true;
			break;
		}
	}

	// If firstIP doesn't exist, return a fail
	if (!firstIPExists) {
		return false;
	}

	// Ensure secondIP exists within current topology
	bool secondIPExists = false;
	for (Node n : topologyList[hostIndex]) {
		if (n.getIP() == secondIP) {
			secondIPExists = true;
			break;
		}
	}

	return secondIPExists;
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
