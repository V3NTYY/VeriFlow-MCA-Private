#include "MCA_VeriFlow.h"
#include "Controller.h"

// Function to split a string into a vector of words
std::vector<std::string> splitInput(std::string input) {
    std::vector<std::string> words;
    std::istringstream stream(input);
    std::string word;
    while (stream >> word) {
        words.push_back(word);
    }
    return words;
}

void MCA_VeriFlow::run() {

}

void MCA_VeriFlow::stop() {

}

bool MCA_VeriFlow::registerTopologyFile(std::string file) {
    // Clear the current topology since we're loading a new one
    topology.clear();

    std::ifstream topology_file(file);
    if (!topology_file.is_open()) {
        std::cerr << "Error opening file..." << std::endl;
        return false;
    }

    std::string line;
    int topology_index = 0;
    bool next_node_ca = false;
    while (std::getline(topology_file, line)) {
        std::vector<std::string> args = splitInput(line);
 
        // Skip non-complete lines
        if (args.size() < 3) {
            // If #NEW is encountered, we are looking at a new topology
            if (args.size() == 1 && args.at(0) == "#NEW") {
                topology_index++;
            }
            // If #CA is encountered, the next node is adjacent to the controller
            if (args.size() == 1 && args.at(0) == "#CA") {
                next_node_ca = true;
            }
            continue;
        }

        // Treat # as a comment
        if (args.at(0) == "#" || args.at(0) == "") {
			continue;
		}

        /// Format: datapathId ipAddress endDevice(0 = false, 1 = true) port1 nextHopIpAddress1 port2 nextHopIpAddress2 ...
        /// Whenever a line only contains #NEW, the rest of the line is ignored and a new topology is created
        /// All devices after #NEW are considered to be in a separate topology
        /// To designate a node as adjacent to the controller, use #CA

        /// Example:
        /// 0 10.0.0.1 0 1 10.0.0.6 2 10.0.0.7
        /// 1 10.0.0.6 1
        /// 2 10.0.0.6 2
        /// #NEW
        /// 3 10.0.0.2 0 1 10.0.0.8 2 10.0.0.9
        /// 4 10.0.0.8 1
        /// 5 10.0.0.9 1
        /// 
        /// This creates two topologies, one with nodes 0, 1, 2 and the other with nodes 3, 4, 5

        std::string datapathId = args.at(0);
        std::string ipAddress = args.at(1);
        bool endDevice = std::stoi(args.at(2));

        // Handle extra parameters for links
        std::vector<std::string> ports;
        std::vector<std::string> nextHops;
        for (int i = 3; i < args.size(); i++) {
            if (i % 2 == 1) {
                ports.push_back(args.at(i));
            }
            else {
                nextHops.push_back(args.at(i));
            }
        }

        // Create a new node
        Node n = Node(topology_index, true, datapathId, ipAddress, endDevice, nextHops, ports);
        n.setControllerAdjacency(next_node_ca);
        next_node_ca = false;
        topology.addNode(n);
    }
}

bool MCA_VeriFlow::createDomainNodes()
{
    int expectedDomainNodes = topology.getTopologyCount() - 1;
    bool success = true;
    if (topology.getTopologyCount() <= 1) {
        std::cerr << "Not enough topologies for domain nodes." << std::endl;
        return !success;
    }

    /// Use loop to shift scope to only two topologies at a time
    for (int i = 0; i < expectedDomainNodes; i++) {

        std::vector<Node> topology1 = topology.getTopology(i);
        std::vector<Node> topology2 = topology.getTopology(i + 1);
        std::vector<Node> candidate_domain_nodes;

        /// Filter nodelist to only nodes that have links and are switches
        for (int j = 0; j < topology1.size(); j++) {
            if (topology1.at(j).getLinkList().size() > 0 && topology1.at(j).isSwitch()) {
                candidate_domain_nodes.push_back(topology1.at(j));
            }
        }

        for (int j = 0; j < topology2.size(); j++) {
            if (topology2.at(j).getLinkList().size() > 0 && topology2.at(j).isSwitch()) {
                candidate_domain_nodes.push_back(topology2.at(j));
            }
        }

        /// Filter nodelist so it only contains inter-topology links
        std::vector<int> mark_for_removal;
        for (Node n : candidate_domain_nodes) {
            for (std::string link : n.getLinkedIPs()) {
                // Check if the current node is linked to any node in the other topology based on IP
                bool found = false;
                Node m = topology.getNodeByIP(link);
                if (!n.isMatchingDomain(m) && !m.isEmptyNode()) {
					found = true;
                    break;
				}
                
                if (!found) {
                    // Get index of current node n
                    for (int k = 0; k < candidate_domain_nodes.size(); k++) {
                        if (candidate_domain_nodes.at(k) == n) {
							mark_for_removal.push_back(k);
							break;
						}
					}
                    break;
                }
            }
        }

        // Commit the removal of non-candidates
        std::sort(mark_for_removal.rbegin(), mark_for_removal.rend());
        for (int j = 0; j < mark_for_removal.size(); j++) {
            candidate_domain_nodes.erase(candidate_domain_nodes.begin() + mark_for_removal.at(j));
		}

        // There are no domain node candidates if the list is empty now
        if (candidate_domain_nodes.size() == 0) {
            std::cerr << "Could not find domain node candidates between topology " << i << " and topology " << i + 1 << std::endl;
            success = false;
            continue;
        }

        /// Handle node preference
        std::vector<int> preferencePoints;
        for (Node n : candidate_domain_nodes) {
            int points = 0;
            // Create preference for nodes that are adjacent to the controller
            if (n.hasAdjacentController()) {
                points += 3;
            }

            // Create preference for nodes with the fewest links
            points -= n.getLinkList().size();
            preferencePoints.push_back(points);
        }

        /// Select the best domain node
        int domainNodeIndex = 0;
        int domainNodePoints = preferencePoints.at(0);
        for (int j = 1; j < preferencePoints.size(); j++) {
            if (preferencePoints.at(j) > domainNodePoints) {
                domainNodeIndex = j;
				domainNodePoints = preferencePoints.at(j);
            }
        }

        // Create the domain node via node flag
        std::string connectingTopologies = std::to_string(i) + ":" + std::to_string(i + 1);
        Node* n = topology.getNodeReference(candidate_domain_nodes.at(domainNodeIndex));
        n->setDomainNode(true, connectingTopologies);
    }

    return success;
}

Topology MCA_VeriFlow::partitionTopology()
{
    bool success = true;
	Topology t;
    if (topology.getTopologyCount() <= 1) {
		std::cerr << "Not enough topologies to partition." << std::endl;
		return t;
	}

    /// Partitioning algorithm -- partitioning no longer needed, instead this method mostly outputs separate files
    t = topology;

    // Iterate through each topologies node list and check the links
    for (int i = 0; i < t.getTopologyCount(); i++) {
        std::vector<Node> nodes = t.getTopology(i);
        for (int j = 0; j < nodes.size(); j++) {
            // If the nodes contains links that are out of topology, ONLY remove those links
            Node* n = t.getNodeReference(nodes.at(j));
            std::vector<std::string> currLinks = n->getLinkList();

            if (currLinks.size() == 0) {
				continue;
			}

            for (std::string link : currLinks) {
				Node m = t.getNodeByIP(link);
                if (!n->isMatchingDomain(m) && !m.isEmptyNode()) {
					n->removeLink(link);

                    // If we have no more links, this is now an end device
                    if (n->getLinkList().size() == 0) {
                        n->setEndDevice(true);
                    }
                }
            }
        }
    }

    return t;
}

bool MCA_VeriFlow::verifyTopology() {
    // Iterate through topology list, run a ping test on each node
    bool success = true;
    for (int i = 0; i < topology.getTopologyCount(); i++) {
        for (Node n : topology.getTopology(i)) {
            Node* ref = topology.getNodeReference(n);
            // If our node is pingable, set the ping result to true for each node
            if (!pingTest(n)) {
                ref->setPingResult(false);
                success = false;
			} else {
				ref->setPingResult(true);
			}
        }
    }
    
    // If a single node is not pingable, the topology is not completely validated
    return success;
}

bool MCA_VeriFlow::registerDomainNodes() {
    
    return false;
}

bool MCA_VeriFlow::pingTest(Node n)
{
    // Ping on windows requires admin, we don't really need to do that since this is unix based.
    #ifdef _WIN32
    return false;
    #endif

    // If the node is reachable via ICMP ping, return true. Use /dev/null 2>&1 to hide output
    std::string sysCommand = "ping -c 1 " + n.getIP() + " > /dev/null 2>&1";
    int result = system(sysCommand.c_str());

    return result == 0;
}

#ifdef __unix__
    std::vector<double> MCA_VeriFlow::measure_tcp_connection(const std::string& host, int port, int num_pings) {
        std::vector<double> rtts;
        for (int i = 0; i < num_pings; i++) {
                int sockfd;
                struct sockaddr_in server_addr;
                struct hostent* server;
                auto start_time = std::chrono::high_resolution_clock::now();
                // create socket and text connection
                sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    std::cerr << "Socket creation failed.\n";
                    return {};
                }

                server = gethostbyname(host.c_str());
                if (server == nullptr) {
                    std::cerr << "Error: No such host.\n";
                    close(sockfd);
                    return {};
                }

                memset(&server_addr, 0, sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
                server_addr.sin_port = htons(port);

                if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                    std::cerr << "Connection failed.\n";
                    close(sockfd);
                    return {};
                }

                auto end_time = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> rtt = end_time - start_time;
                rtts.push_back(rtt.count());

                close(sockfd);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return rtts;
    }
#endif

#ifdef __unix__
    // Function to test TCP connection time
    double MCA_VeriFlow::test_tcp_connection_time(const std::string& host, int port, int timeout) {
            int sockfd;
            struct sockaddr_in server_addr;
            struct hostent* server;
            auto start_time = std::chrono::high_resolution_clock::now();
            // create socket and text connection
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                std::cerr << "Socket creation failed.\n";
                return -1;
            }

            server = gethostbyname(host.c_str());
            if (server == nullptr) {
                std::cerr << "Error: No such host.\n";
                close(sockfd);
                return -1;
            }

            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
            server_addr.sin_port = htons(port);

            struct timeval tv;
            tv.tv_sec = timeout;
            tv.tv_usec = 0;
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

            if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                std::cerr << "Connection failed.\n";
                close(sockfd);
                return -1;
            }
        
            auto end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> connection_time = end_time - start_time;
            close(sockfd);
            return connection_time.count();
    }
#endif

int main() {
    MCA_VeriFlow* mca_veriflow = new MCA_VeriFlow();
    Controller* c = new Controller();

    #ifdef _WIN32
    std::cout << "WARNING: This app only runs on UNIX systems due to specific socket libraries. Most things won't work." << std::endl;
    #endif

    bool controller_linked = false;
    bool topology_initialized = false;
   
    while (true) {

        std::string input;
        std::cout << ">>> ";
        std::getline(std::cin, input);
        std::cout << std::endl;
        if (input == "exit") {
			break;
		}

        // Parse args
        std::vector<std::string> args = splitInput(input);
        if (args.size() == 0) {
			std::cout << "Invalid command" << std::endl;
		}

        // Help command
        else if (args.at(0) == "help") {
            std::cout << "Commands:" << std::endl <<
                " * help:                                   Display all commands and their parameters." << std::endl <<
                " * run:                                    Start the CCPDN App. Controller must be linked, and topology initialized." << std::endl <<
                " * stop:                                   Stop the CCPDN App." << std::endl <<
                " * rdn [topology_file]:                    Identifies and links all candidates for domain nodes based on given topology file." << std::endl <<
                " * refactor-top [file-name]:               Partition and output the current topology into a format for VeriFlow. Does not change the programs view, this command only outputs to a file." << std::endl <<
                " * output-top [file-name]:                 Output the current topology list to a new file." << std::endl <<
                " * list-devices:                           List all the devices connected to the Pox Controller." << std::endl <<
                " * list-flows [switch-ip-address]:         List all the flows associated with a switch based on its IP." << std::endl <<
                " * link-controller [ip-address] [port]:    Link a currently running Pox Controller to this app." << std::endl <<
                " * unlink-controller:                      Free the Pox Controller from this app." << std::endl <<
                " * add-flow [flow_id] [eth_type], [protocol] [source-ip] [destination-ip] [source-port] [destination-port] [action]" << std::endl <<
                "                                           Add a flow to the flow table." << std::endl <<
                " * del-flow:                               Delete a flow from the flow table." << std::endl <<
                " * run-tcp-test                            Run's the TCP connection setup latency test." << std::endl <<
                "" << std::endl;
        }

        // link-controller command
        else if (args.at(0) == "link-controller") {
            if (args.size() < 3) {
				std::cout << "Not enough arguments. Usage: link-controller [ip-address] [port]" << std::endl;
			}
            else if (controller_linked) {
                std::cout << "Controller already linked. Try unlink-controller first" << std::endl;
            }
            else {
				c->setControllerIP(args.at(1), args.at(2));
				controller_linked = c->start();
			}
        }

        // output-top command
        else if (args.at(0) == "output-top") {
			if (!topology_initialized) {
				std::cout << "Topology not initialized. Try rdn first." << std::endl;
			}
			else if (args.size() < 2) {
				std::cout << "Not enough arguments. Usage: output-top [file-name]" << std::endl;
			}
			else {
                if (mca_veriflow->topology.outputToFile(args.at(1))) {
                    std::cout << "Topology output to " << args.at(1) << std::endl << std::endl;
				}
				else {
					std::cout << "Error outputting topology to file." << std::endl << std::endl;
                }
			}
		}

        // refactor-top command
        else if (args.at(0) == "refactor-top") {
            if (!topology_initialized) {
                std::cout << "Topology not initialized. Try rdn first." << std::endl;
            }
			else if (args.size() < 2) {
				std::cout << "Not enough arguments. Usage: refactor-top [file-name]" << std::endl;
			}
			else {
                // Use partitioning algorithm to split the topology into multiple topologies
                Topology partitioned_topologies = mca_veriflow->partitionTopology();

                for (int i = 0; i < partitioned_topologies.getTopologyCount(); i++) {
					std::cout << "--- PARTITIONED TOPOLOGY " << i << " ---" << std::endl;
					std::cout << partitioned_topologies.printTopology(i) << std::endl;

                    // Output new, partitioned topology file to give to VeriFlow
                    if (partitioned_topologies.extractIndexTopology(i).outputToFile(args.at(1) + std::to_string(i))) {
                        std::cout << "Topology " << std::to_string(i) << " output to " << args.at(1) + std::to_string(i) << std::endl << std::endl;
                    }
                    else {
                        std::cout << "Error outputting topology " << args.at(1) + std::to_string(i) << " to file." << std::endl << std::endl;
                    }
				}
			}
        }

        // rdn command
        else if (args.at(0) == "rdn") {
            if (!controller_linked) {
                std::cout << "Controller not linked. Try link-controller first" << std::endl;
            }
            else if (args.size() < 2) {
                std::cout << "Not enough arguments. Usage: rdn [topology_file]" << std::endl;
			}
            else {
                mca_veriflow->registerTopologyFile(args.at(1));

                // Create a thread to handle this method
                if (!mca_veriflow->verifyTopology()) {
                    std::cout << "Topology verification failed. Are all switches reachable?" << std::endl;
                }

                // Find the best candidates for domain nodes, create them.
                mca_veriflow->createDomainNodes();

                // Print the topology list
                for (int i = 0; i < mca_veriflow->topology.getTopologyCount(); i++) {
                    std::cout << "--- TOPOLOGY " << i << " ---" << std::endl;
                    std::cout << mca_veriflow->topology.printTopology(i) << std::endl;
                }

                // TODO: Register domain nodes via handshake. May need a separate thread, or to implement a listener for this
                mca_veriflow->registerDomainNodes();

                topology_initialized = true;
            }
        }

        // unlink-controller command
        else if (args.at(0) == "unlink-controller") {
            if (!controller_linked) {
				std::cout << "Controller not linked!" << std::endl;
			}
			else {
                // TODO: Stop any services involving listening to the controller
                // TODO: Destroy references with current controller
				controller_linked = false;
                topology_initialized = false;
			}
        }

        // run command
        else if (args.at(0) == "run") {
            if (!controller_linked || !topology_initialized) {
                std::cout << "Cannot start CCPDN App. Ensure the controller is linked and topology is initialized." << std::endl;
            } else {
                mca_veriflow->run();
                std::cout << "CCPDN App started." << std::endl;
            }
        }

        // stop command
        else if (args.at(0) == "stop") {
            if (!controller_linked || !topology_initialized) {
                std::cout << "CCPDN App is not running." << std::endl;
            } else {
                mca_veriflow->stop();
                std::cout << "CCPDN App stopped." << std::endl;
            }
        }
    

        else if (args.at(0) == "run-tcp-test") {
            std::cout << "Running TCP test..." << std::endl;
            #ifdef __unix__
                std::vector<double> measured_rtts;
                measured_rtts = mca_veriflow->measure_tcp_connection("google.com", 80, 10);
                for (int i = 0; i < measured_rtts.size(); i++) {
					std::cout << "RTT " << i << ": " << measured_rtts.at(i) << std::endl;
				}
            #endif
        }

        // Invalid response
        else {
            std::cout << "Invalid command" << std::endl;
        }
    }

    return 0;
}