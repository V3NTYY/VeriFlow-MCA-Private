#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include "Node.h"
#include "Log.h"
#include <fstream>
#include <sstream>

/// This class is a little confusing.
///
/// Each topology is stored as a vector of nodes
/// In multi-controller VeriFlow, we have multiple topologies
/// Topology.h is a master class that handles ALL topologies in a given network
/// The "master" list, topologyList, is a vector of all topologies
/// 
/// When nodes are added, they contain metadata that identifies which topology they belong to
/// If the topology ID doesn't exist, a new topology is created

class Topology {
	public:

		// Equal operator (for finding dupes)
		bool operator==(const Topology& other) const {
			return topologyList == other.topologyList;
		}

		// Node getters
		Node getNodeByIP(std::string IP);
		Node getNodeByIP(std::string IP, int index);
		Node* getNodeReference(Node n);

		// Add node to topology
		bool addNode(Node node);

		// Get and print topology data
		std::vector<Node> getTopology(int index);
		int getTopologyCount();
		std::string printTopology(int index);

		// Return a topology object containg only a single topology (by index)
		Topology extractIndexTopology(int index);

		// Misc functions
		void clear();
		bool outputToFile(std::string filename);
		bool isLocal(std::string firstIP, std::string secondIP, bool print);

		// Marshalling functions
		static std::vector<Node> string_toTopology(std::string payload);
		std::string topology_toString(int index);

		// Variables
		std::vector<std::vector<Node>> topologyList;
		bool verified;
		int hostIndex;
		
		int getTopologyIndex(const std::string& ip);

		// Each node in the topology is stored as a vector of node objects
		// Each node knows its own links and information
		// Each node understands which domain/topology it belongs to based on Node.topologyIndex

	private:
};

#endif