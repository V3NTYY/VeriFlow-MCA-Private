#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include "Node.h"
#include <fstream>

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
		bool addNode(Node node);
		Node getNodeByIP(std::string IP);
		Node* getNodeReference(Node n);
		std::vector<Node> getTopology(int index);
		std::string printTopology(int index);
		int getTopologyCount();

		void clear();
		bool outputToFile(std::string filename);

		std::vector<std::vector<Node>> topologyList;

		// Each node in the topology is stored as a vector of node objects
		// Each node knows its own links and information
		// Each node understands which domain/topology it belongs to based on Node.topologyIndex

	private:
};

#endif