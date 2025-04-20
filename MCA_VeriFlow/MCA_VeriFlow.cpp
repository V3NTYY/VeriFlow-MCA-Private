#include "MCA_VeriFlow.h"
#include "Log.h"
#include "TCPAnalyzer.h"

// Function to split a string into a vector of words based on delimiters
std::vector<std::string> splitInput(std::string input, std::vector<std::string> delimiters) {
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

// Function to get user input, and verify the result as an int. Loops until a valid int is entered
int getUserInputInt(std::string prompt) {
    int result;
	while (true) {
		loggyMsg(prompt);
		std::cin >> result;
		if (std::cin.fail()) {
            // Clear the error flag, discard current input and attempt again
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			loggyMsg("Invalid input.\n\n");
		} else {
            // Input is valid, return the result
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			break;
		}
	}
	return result;
}

MCA_VeriFlow::MCA_VeriFlow()
{
    Topology t;
    topology = t;
    controller = Controller(&topology);
    controller_running = false;
    controller_linked = false;
    topology_initialized = false;
    runTCPDump = false;
}

MCA_VeriFlow::~MCA_VeriFlow()
{
    MCA_VeriFlow::stop();
}

void MCA_VeriFlow::run() 
{
    // Link the CCPDN to the veriflow server
    if (!controller.start()) {
		std::cerr << "Error linking to VeriFlow server." << std::endl;
		return;
	}

    // Start TCPDump thread
    TCPAnalyzer tcp;
    runTCPDump = true;
    std::thread tcpDumpThread(&TCPAnalyzer::thread, &tcp, &runTCPDump, &controller);
    tcpDumpThread.detach();
}

void MCA_VeriFlow::stop() {
    runTCPDump = false;
    controller_running = false;
	controller_linked = false;
    topology_initialized = false;

    // Kill any rogue xterm processes from the CCPDN
    std::ifstream pidFile("/tmp/ccpdn_xterm_pid");
    if (pidFile.is_open()) {
        std::string pid;
        std::getline(pidFile, pid);
        pidFile.close();
        if (!pid.empty()) {
            std::string killPID = "kill -9 " + pid;
            system(killPID.c_str());
        }

        // Remove the PID file
        std::string removePIDFile = "rm -f /tmp/ccpdn_xterm_pid";
        system(removePIDFile.c_str());
    } else {
        loggy << "[CCPDN-ERROR]: Could not kill xterm window!" << std::endl;
    }
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
    bool isControllerAdjacent = false;
    bool isHost = false; // false = switch, true = host
    while (std::getline(topology_file, line)) {
        std::vector<std::string> delimiters = { ":", ",", " " };
        std::vector<std::string> args = splitInput(line, delimiters);

        if (args.empty()) {
            continue;
        }

        if (args.at(0) == "TOP#") {
            topology_index++;
            isHost = false;
            continue;
        } else if (args.at(0) == "CA#") {
            isControllerAdjacent = true;
        } else if (args.at(0) == "S#") {
            isHost = false;
			continue;
		} else if (args.at(0) == "H#") {
			isHost = true;
			continue;
        } else if (args.at(0) == "R#") {
            // Used to add rules. But we don't add them statically here.
			continue;
		} else if (args.at(0) == "E!") {
            // Used to end file reading. But not necessary since we could have multiple topologies.
            continue;
        }

        //    S#  // Switch Definitions Section
        //    <SwitchID>:<NextHop1>, <NextHop2>, ...
        //    <SwitchID> : <NextHop1>, <NextHop2>, ...
        //    Example -> 10.0.0.1 : 10.0.0.2, 10.0.0.3
        //    10.0.0.2 :
        //    10.0.0.3 : 10.0.0.1
        //    Hosts are not considered as a "next hop". Only switches.

        //    H#  // Host Definitions Section
        //    <HostID> : <SwitchID>
        //    Example -> 10.0.0.31 : 10.0.0.1

        //    R#  // Rules Definitions Section
        //    <SwitchID>-<Prefix>-<NextHopId>
        //    Example -> 10.0.0.1 - 192.168.1.0 / 24 - 10.0.0.2
        //    This rule matches all traffic under the 192.168.1.0 / 24 subnet, and forwards it to 10.0.0.2
        //    For most rules, drop is essentially the default action -- everything added here is considered as a forward.

        //    E!  // End Marker. Use this at end of file, after rules section.

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
        topology.addNode(n);
    }
    return true;
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
            if (topology1.at(j).getLinks().size() > 0 && topology1.at(j).isSwitch()) {
                candidate_domain_nodes.push_back(topology1.at(j));
            }
        }

        for (int j = 0; j < topology2.size(); j++) {
            if (topology2.at(j).getLinks().size() > 0 && topology2.at(j).isSwitch()) {
                candidate_domain_nodes.push_back(topology2.at(j));
            }
        }

        /// Filter nodelist so it only contains inter-topology links
        std::vector<int> mark_for_removal;
        for (Node n : candidate_domain_nodes) {
            for (std::string link : n.getLinks()) {
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
            points -= n.getLinks().size();
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
        controller.addDomainNode(n);
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
            std::vector<std::string> currLinks = n->getLinks();

            if (currLinks.size() == 0) {
				continue;
			}

            for (std::string link : currLinks) {
				Node m = t.getNodeByIP(link);
                if (!n->isMatchingDomain(m) && !m.isEmptyNode()) {
					n->removeLink(link);
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
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                std::cerr << "Socket creation failed.\n";
                continue;
            }
        
            struct sockaddr_in server_addr{};
            struct hostent* server = gethostbyname(host.c_str());
            if (server == nullptr) {
                std::cerr << "No such host.\n";
                close(sockfd);
                continue;
            }
        
            memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
        
            auto start_time = std::chrono::high_resolution_clock::now();
        
            if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                std::cerr << "Connection failed.\n";
                close(sockfd);
                continue;
            }
        
            auto end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> rtt = end_time - start_time;
            rtts.push_back(rtt.count());
        
            const char* msg = "VERIFY\n";
            send(sockfd, msg, strlen(msg), 0);
        
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
    // Create the MCA_VeriFlow object
    MCA_VeriFlow* mca_veriflow = new MCA_VeriFlow();

    #ifdef _WIN32
    loggyMsg("WARNING: This app only runs on UNIX systems due to specific socket libraries. Most things won't work.\n\n");
    #endif

    // Check if we are running with root privileges
    #ifdef __unix__
        if (getuid() != 0) {
            loggyMsg("WARNING: This app requires root privileges to run properly.\n\n");
        }
    #endif

    loggyMsg("Welcome to the CCPDN. If you do not see >>>, it may have been overshadowed by console output.\n");
    loggyMsg("Type 'help' for a list of commands.\n\n");
   
    while (true) {

        // If link_controller is sending packets/receiving packets, wait for that to finish
        while (mca_veriflow->controller.linking) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::string input;
        loggyMsg(">>> ");
        std::cin.clear();
        std::getline(std::cin, input);
        loggyMsg("\n");
        if (input == "exit") {
			break;
		}

        // Parse args
        std::vector<std::string> delimiters = { " " };
        std::vector<std::string> args = splitInput(input, delimiters);
        if (args.size() == 0) {
            // Print nothing if no args are given
            continue;
		}

        // Help command, * means implemented, - means work in progress, x means nothing started on it yet
        else if (args.at(0) == "help") {
            loggy << "Commands:" << std::endl <<
                " * help:" << std::endl <<
                "   Display all commands and their parameters.\n" << std::endl <<
                " * exit:" << std::endl <<
                "   Exit the CCPDN App.\n" << std::endl <<
                " - start [veriflow-ip-address] [veriflow-port]:" << std::endl <<
                "   Start the CCPDN Service by linking to VeriFlow. Controller must be linked, and topology initialized.\n" << std::endl <<
                " - stop:" << std::endl <<
                "   Stop the CCPDN Service.\n" << std::endl <<
                " * rdn [topology_file]:" << std::endl <<
                "   Identifies and links all candidates for domain nodes based on given topology file.\n" << std::endl <<
                " * refactor-top [file-name]:" << std::endl <<
                "   Partition and output the current topology into a format for VeriFlow. Does not change the programs view, this command only outputs to a file.\n" << std::endl <<
                " * output-top [file-name]:" << std::endl <<
                "   Output the current topology list to a new file.\n" << std::endl <<
                " * link-controller [ip-address] [port]:" << std::endl <<
                "   Link a currently running Pox Controller to this app.\n" << std::endl <<
                " * reset-controller:" << std::endl <<
                "   Free the Pox Controller from this app.\n" << std::endl <<
                " - list-flows [switch-ip-address]:" << std::endl <<
                "   List all the flows associated with a switch based on its IP.\n" << std::endl <<
                " - add-flow [switch-ip-address] [rule-prefix] [next-hop-ip-address]" << std::endl <<
                "   Add a flow to the flow table of the specified switch based off the contents of a file.\n" << std::endl <<
                " - del-flow: [switch-ip-address] [rule-prefix] [next-hop-ip-address]" << std::endl <<
                "   Delete a flow from the flow table of the specified switch based off the contents of a file.\n" << std::endl <<
                " - run-tcp-test" << std::endl <<
                "   Run's the TCP connection setup latency test.\n" << std::endl <<
                " * test-method" << std::endl <<
                "   Run's a current method that needs to be tested. For development purposes only.\n" << std::endl <<
                "";
        }

        else if (args.at(0) == "list-flows") {
            if (!mca_veriflow->controller_running) {
                loggy << "Ensure CCPDN Service is started first.\n" << std::endl;
            }
            else if (args.size() < 2) {
                loggy << "Not enough arguments. Usage: list-flows [switch-ip-address]\n" << std::endl;
            }
            else {
                std::string targetIP = args.at(1);
                // Ensure IP exists within host topology
                Node n = mca_veriflow->topology.getNodeByIP(targetIP);
                if (n.isEmptyNode()) {
                    loggy << "No such switch in topology.\n" << std::endl;
                }

                // Request flows from controller
                std::vector<Flow> flows = mca_veriflow->controller.retrieveFlows(targetIP);

                // Print all flows
                loggy << "--- FLOWS " << targetIP << " ---" << std::endl;
                for (Flow f : flows) {
					loggy << "Rule Prefix: " << f.getRulePrefix() << std::endl;
					loggy << "Next Hop IP: " << f.getNextHopIP() << std::endl;
					loggy << "Action: " << (f.actionType() ? "Forward" : "Drop") << std::endl;
					loggy << std::endl;
				}
            }
        }

        else if (args.at(0) == "add-flow") {
			if (!mca_veriflow->controller_running) {
				loggy << "Ensure CCPDN Service is started first.\n" << std::endl;
			}
			else if (args.size() < 4) {
				loggy << "Not enough arguments. Usage: add-flow [switch-ip-address] [rule-prefix] [next-hop-ip-address]\n" << std::endl;
			}
            else {
				std::string targetIP = args.at(1);
				// Ensure IP exists within host topology
				Node n = mca_veriflow->topology.getNodeByIP(targetIP);
				if (n.isEmptyNode()) {
					loggy << "No such switch in topology.\n" << std::endl;
				}

                Flow add(args.at(1), args.at(2), args.at(3), true);

				// Add flow to controller
				mca_veriflow->controller.addFlowToTable(add);
			}
		}

		else if (args.at(0) == "del-flow") {
			if (mca_veriflow->controller_running) {
				loggy << "Ensure CCPDN Service is started first.\n" << std::endl;
			}
            else if (args.size() < 4) {
				loggy << "Not enough arguments. Usage: del-flow [switch-ip-address] [rule-prefix] [next-hop-ip-address]\n" << std::endl;
			}
			else {
                // Ensure IP exists within host topology
                Node n = mca_veriflow->topology.getNodeByIP(args.at(1));
                if (n.isEmptyNode()) {
                    loggy << "[CCPDN-ERROR]: No such switch in topology.\n" << std::endl;
                }

				Flow remove(args.at(1), args.at(2), args.at(3), false);

				// Delete flow from controller
                mca_veriflow->controller.removeFlowFromTable(remove);
            }
        }

        // link-controller command
        else if (args.at(0) == "link-controller") {
            if (args.size() < 3) {
				loggy << "Not enough arguments. Usage: link-controller [ip-address] [port]\n" << std::endl;
			}
            else if (mca_veriflow->controller_linked) {
                loggy << "Controller already linked. Try unlink-controller first\n" << std::endl;
            }
            else {
				mca_veriflow->controller.setControllerIP(args.at(1), args.at(2));
				mca_veriflow->controller_linked = mca_veriflow->controller.startController(&(mca_veriflow->controller_running));
			}
        }

        // reset-controller command
        else if (args.at(0) == "reset-controller") {
            if (!mca_veriflow->controller_linked) {
                loggy << "Controller not linked. Try link-controller first\n" << std::endl;
            }
            else {
                mca_veriflow->controller.freeLink();
                mca_veriflow->controller_linked = false;
            }
        }

        // output-top command
        else if (args.at(0) == "output-top") {
			if (!mca_veriflow->topology_initialized) {
				loggy << "Topology not initialized. Try rdn first.\n" << std::endl;
			}
			else if (args.size() < 2) {
				loggy << "Not enough arguments. Usage: output-top [file-name]\n" << std::endl;
			}
			else {
                if (mca_veriflow->topology.outputToFile(args.at(1))) {
                    loggy << "Topology output to " << args.at(1) << std::endl << std::endl;
				}
				else {
					loggy << "Error outputting topology to file.\n" << std::endl;
                }
			}
		}

        // refactor-top command
        else if (args.at(0) == "refactor-top") {
            if (!mca_veriflow->topology_initialized) {
                loggy << "Topology not initialized. Try rdn first.\n" << std::endl;
            }
			else if (args.size() < 2) {
				loggy << "Not enough arguments. Usage: refactor-top [file-name]\n" << std::endl;
			}
			else {
                // Use partitioning algorithm to split the topology into multiple topologies
                Topology partitioned_topologies = mca_veriflow->partitionTopology();

                for (int i = 0; i < partitioned_topologies.getTopologyCount(); i++) {
					loggy << "--- PARTITIONED TOPOLOGY " << i << " ---" << std::endl;
					loggy << partitioned_topologies.printTopology(i);

                    // Output new, partitioned topology file to give to VeriFlow
                    if (partitioned_topologies.extractIndexTopology(i).outputToFile(args.at(1) + std::to_string(i))) {
                        loggy << "Topology " << std::to_string(i) << " output to " << args.at(1) + std::to_string(i) << std::endl << std::endl;
                    }
                    else {
                        loggy << "Error outputting topology " << args.at(1) + std::to_string(i) << " to file.\n" << std::endl;
                    }
				}
			}
        }

        // rdn command
        else if (args.at(0) == "rdn") {
            if (!mca_veriflow->controller_linked) {
                loggy << "Controller not linked. Try link-controller first\n" << std::endl;
            }
            else if (args.size() < 2) {
                loggy << "Not enough arguments. Usage: rdn [topology_file]\n" << std::endl;
			}
            else {
                // Read the topology file and register it
                if (!mca_veriflow->registerTopologyFile(args.at(1))) {
                    loggy << "Error reading topology file. Ensure the file exists and is in the correct format.\n" << std::endl;
					continue;
                }

                // Verify the nodes exist in the topology
                loggy << "Performing ping test on all nodes for verification...\n" << std::endl;
                if (!mca_veriflow->verifyTopology()) {
                    loggy << "Topology verification failed. Are all switches reachable?\n" << std::endl;
                }

                // Find the best candidates for domain nodes, create them.
                mca_veriflow->createDomainNodes();

                // Print the topology list
                for (int i = 0; i < mca_veriflow->topology.getTopologyCount(); i++) {
                    loggy << "--- TOPOLOGY " << i << " ---" << std::endl;
                    loggy << mca_veriflow->topology.printTopology(i);
                }

                // Ask for user to identify which topology is the host (what topology this CCPDN is running on)
                bool userInputLoop = true;
                int HostIndex = 0;

                while (userInputLoop) {
                    loggy << std::endl;
                    HostIndex = getUserInputInt("Please enter the index of the topology this CCPDN is running on: ");

                    // Ensure the topology index is valid
                    if (HostIndex < 0 || HostIndex >= mca_veriflow->topology.getTopologyCount()) {
                        loggy << "Invalid topology index. Please try again.\n";
                        continue;
                    }
                    else {
                        loggy << std::endl;
                        userInputLoop = false;
                        break;
                    }
                }

                // Set the host index in the topology
                mca_veriflow->topology.hostIndex = HostIndex;

                mca_veriflow->topology_initialized = true;
            }
        }

        // run command
        else if (args.at(0) == "start") {
            if (!mca_veriflow->controller_linked || !mca_veriflow->topology_initialized) {
                loggy << "Cannot start CCPDN App. Ensure the controller is linked and topology is initialized.\n" << std::endl;
            } else if (args.size() < 3) {
                loggy << "Not enough arguments. Usage: start [veriflow-ip-address] [veriflow-port]\n" << std::endl;
            } else {
                mca_veriflow->controller.setVeriFlowIP(args.at(1), args.at(2));
                mca_veriflow->run();
                loggy << "[CCPDN]: App started.\n" << std::endl;
            }
        }

        // stop command
        else if (args.at(0) == "stop") {
            if (!mca_veriflow->controller_linked || !mca_veriflow->topology_initialized || !mca_veriflow->controller_running) {
                loggy << "CCPDN App is not running.\n" << std::endl;
            } else {
                mca_veriflow->stop();
                loggy << "[CCPDN]: App stopped.\n" << std::endl;
            }
        }
    

        else if (args.at(0) == "run-tcp-test") {
            if (args.size() < 3) {
                loggy << "Usage: run-tcp-test [target-ip] [port]\n";
            } else if (!mca_veriflow->controller_linked || !mca_veriflow->topology_initialized) {
                loggy << "Ensure topology is initialized and controller is linked first.\n\n";
            } else {
                loggy << "Running TCP test...\n" << std::endl;
                #ifdef __unix__
                    std::vector<double> times = mca_veriflow->measure_tcp_connection(args.at(1), std::stoi(args.at(2)), 10);
                    
                    loggy << "Verification latency measurements:\n";
                    for (size_t i = 0; i < times.size(); i++) {
                        loggy << "Test " << i+1 << ": " << times[i] * 1000 << " ms\n";
                    }
                    
                    // Calculate statistics
                    if (!times.empty()) {
                        double sum = std::accumulate(times.begin(), times.end(), 0.0);
                        double mean = sum / times.size();
                        double max = *std::max_element(times.begin(), times.end());
                        double min = *std::min_element(times.begin(), times.end());
                    
                        loggy << "\nAverage: " << mean * 1000 << " ms\n";
                        loggy << "Max: " << max * 1000 << " ms\n";
                        loggy << "Min: " << min * 1000 << " ms\n";
                    } else {
                        loggy << "No data returned from TCP test.\n";
                    }
                    
                #endif
            }
        }

        else if (args.at(0) == "test-method") {

            if (!mca_veriflow->controller_linked) {
                loggy << "Link controller first dummy.\n\n";
            }

            // Request flows from controller
            std::vector<Flow> flows = mca_veriflow->controller.retrieveFlows("127.0.0.1");

            // Print all flows
            loggy << "--- FLOWS ---" << std::endl;
            for (Flow f : flows) {
                loggy << "Switch IP: " << f.getSwitchIP() << std::endl;
                loggy << "Rule Prefix: " << f.getRulePrefix() << std::endl;
                loggy << "Next Hop IP: " << f.getNextHopIP() << std::endl;
                loggy << "Action: " << (f.actionType() ? "Forward" : "Drop") << std::endl;
                loggy << std::endl;
            }
            loggy << std::endl;
        }

        // Invalid response
        else {
            loggy << "Invalid command. Try entering 'help' to view commands.\n" << std::endl;
        }
    }

    return 0;
}