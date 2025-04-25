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
    runService = false;
    flowhandler_linked = false;
}

MCA_VeriFlow::~MCA_VeriFlow()
{
    MCA_VeriFlow::stop();
}

void MCA_VeriFlow::run() 
{
    // Link the CCPDN to the veriflow server
    if (!controller.start()) {
		loggyErr("Error linking to VeriFlow server.\n");
		return;
	}

    runService = true;
    // Create CCPDN server, bind to the correct port and listen for connections
    int portCC = std::stoi(controller.veriflowPort) + topology.hostIndex + 1;
    
    // Start CCPDN server connection handler thread (for accepting new connections)
    std::thread ccpdnServerThread(&Controller::CCPDNServerThread, &controller, portCC, &runService);
    ccpdnServerThread.detach();

    // Start CCPDN message receiver thread (for digests)
    std::thread ccpdnMessageThread(&Controller::CCPDNThread, &controller, &runService);
    ccpdnMessageThread.detach();

    // Initiate CCPDN connections
    controller.initCCPDN();

    // Start TCPDump thread to listen for controller messages
    TCPAnalyzer tcp;
    std::thread tcpDumpThread(&TCPAnalyzer::thread, &tcp, &runService, controller.controllerPort);
    tcpDumpThread.detach();
}

void MCA_VeriFlow::stop() {

    // Close current connections
    controller.stopCCPDNServer();

    // Close any connected sockets
    controller.closeSockets();

    // Signal all flags
    runService = false;
    controller_running = false;
	controller_linked = false;
    topology_initialized = false;
    flowhandler_linked = false;
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
		loggyErr("Not enough topologies to partition.\n");
		return t;
	}

    /// Partitioning algorithm -- partitioning no longer needed, instead this method mostly outputs separate files
    t = topology;

    // Iterate through each topologies node list and check the links
    for (int i = 0; i < t.getTopologyCount(); i++) {
        std::vector<Node> nodes = t.getTopology(i);
        for (int j = 0; j < nodes.size(); j++) {
            // If the nodes contains links that are out of topology, ONLY remove those links unless they are a domain node
            Node* n = t.getNodeReference(nodes.at(j));
            std::vector<std::string> currLinks = n->getLinks();

            if (currLinks.size() == 0) {
				continue;
            }

            // Remove inter-topology links only if they are not through a domain node
            for (std::string link : currLinks) {
				Node m = t.getNodeByIP(link);
                if (!n->isMatchingDomain(m) && !m.isEmptyNode() && (!m.isDomainNode() && !n->isDomainNode())) {
                    // If the node is not a domain node, remove the link
					n->removeLink(link);
                }
            }
        }
    }

    return t;
}

bool MCA_VeriFlow::verifyTopology() {
    // Iterate through topology list, run a ping test on each node (only switches)
    bool success = true;
    for (int i = 0; i < topology.getTopologyCount(); i++) {
        for (Node n : topology.getTopology(i)) {
            Node* ref = topology.getNodeReference(n);
            // If our node is pingable, set the ping result to true for each node
            if (!pingTest(n)) {
                ref->setPingResult(false);
                if (n.isSwitch()) { // We only care about switches being reachable
                    success = false;
                }
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

void MCA_VeriFlow::printPorts(int VeriFlowPort)
{
    // Print VeriFlowPort + 1. For each topology instance, increment this and print it out
    loggy << " ----CCPDN Ports---- " << std::endl;
    for (int i = 0; i < topology.getTopologyCount(); i++) {
        int port = VeriFlowPort + i;
        if (i != topology.hostIndex) {
            loggy << "[CCPDN-" << i << "] " << std::to_string(VeriFlowPort+i) << std::endl;
        } else {
            loggy << "[CCPDN-" << i << "] " << std::to_string(port) << " (This instance)" << std::endl;
        }
    }
}

void MCA_VeriFlow::printStatus()
{
    loggy << "CCPDN Status:" << std::endl;
    loggy << " - Topology " << (topology_initialized ? "[ACTIVE]" : "[INACTIVE]") << std::endl;
    loggy << " - Controller " << (controller_linked ? "[ACTIVE]" : "[INACTIVE]") << std::endl;
    loggy << " - FlowHandler " << (flowhandler_linked ? "[ACTIVE]" : "[INACTIVE]") << std::endl;
    loggy << " - CCPDN Base Port Set " << (controller.basePort != -1 ? "[ACTIVE]" : "[INACTIVE]") << std::endl;
    loggy << " - CCPDN Service " << (runService ? "[ACTIVE]" : "[INACTIVE]") << std::endl;
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
    loggyMsg("WARNING: This app only runs on UNIX systems due to specific socket libraries. Most things won't work.\n");
    #endif

    // Check if we are running with root privileges
    #ifdef __unix__
        if (getuid() != 0) {
            loggyMsg("WARNING: This app requires root privileges to run properly.\n");
        }
    #endif

    loggyMsg("Welcome to the CCPDN. If you do not see >>>, it may have been overshadowed by console output.\n");
    loggyMsg("Type 'help' for a list of commands.\n");
   
    while (true) {

        // If link_controller is sending packets/receiving packets, wait for that to finish
        while (Controller::pauseOutput) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::string input;
        loggy << std::endl;
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

        // Help command, - means implemented, * means work in progress, x means nothing started on it yet
        else if (args.at(0) == "help") {
            loggy << "Commands:" << std::endl <<
                " - help:" << std::endl <<
                "   Display all commands and their parameters.\n" << std::endl <<
                " - exit:" << std::endl <<
                "   Exit the CCPDN App.\n" << std::endl <<
                " - start [veriflow-ip-address] [veriflow-port]:" << std::endl <<
                "   Start the CCPDN Service by linking to VeriFlow (default port = 6657).\n" << std::endl <<
                " - stop:" << std::endl <<
                "   Stop the CCPDN Service.\n" << std::endl <<
                " - status" << std::endl <<
                "   Display the status of the CCPDN instance.\n" << std::endl <<
                " - reg-top [topology_file]:" << std::endl <<
                "   Registers a given topology file to this CCPDN instance and identifies domain nodes.\n" << std::endl <<
                // DEPRECATED COMMAND" - refactor-top [file-name]:" << std::endl <<
                // DEPRECATED COMMAND"   Partition and output the current topology into a format for VeriFlow. Does not change the programs view, this command only outputs to a file.\n" << std::endl <<
                " - output-top [file-name]:" << std::endl <<
                "   Outputs the configuration for this local topology's VeriFlow instance.\n" << std::endl <<
                " - list-top [index]:" << std::endl <<
                "   List the topology at a given index.\n" << std::endl <<
                " - ccpdn-ports [base-port]:" << std::endl <<
                "   Assigns the port range that all CCPDN instances will use (range: [Base Port] -> [Base Port + Amount of Topologies])." << std::endl <<
                " - Please ensure that the value you enter is the same across all CCPDN instances. Use this command again if you need to reset it before running start.\n" << std::endl <<
                " - retry-ccpdn" << std::endl <<
                "   Retry the connection to all CCPDN instances.\n" << std::endl <<
                " - link-controller [ip-address] [port]:" << std::endl <<
                "   Link a currently running Pox Controller to this app (default port = 6653).\n" << std::endl <<
                " - reset-controller:" << std::endl <<
                "   Free the Pox Controller from this app.\n" << std::endl <<
                " - link-flowhandler [ip-address] [port]:" << std::endl <<
                "   Link this app to the flow handler, allowing for dynamic flow access (default port = 6655).\n" << std::endl <<
                " - reset-fh" << std::endl <<
                "   Free the flowhandler connection from this app.\n" << std::endl <<
                " - list-flows [switch-ip-address]:" << std::endl <<
                "   List all the flows associated with a switch based on its IP.\n" << std::endl <<
                " - add-flow [switch-ip-address] [rule-prefix] [next-hop-ip-address]" << std::endl <<
                "   Add a flow to the flow table of the specified switch based off the contents of a file.\n" << std::endl <<
                " - del-flow: [switch-ip-address] [rule-prefix] [next-hop-ip-address]" << std::endl <<
                "   Delete a flow from the flow table of the specified switch based off the contents of a file.\n" << std::endl <<
                " - run-tcp-test" << std::endl <<
                "   Run's the TCP connection setup latency test.\n" << std::endl <<
                " * test-verification-time [num-flows]" << std::endl <<
                "   Test verification time for a given number of flows.\n" <<
                "";
        }

        else if (args.at(0) == "status") {
            mca_veriflow->printStatus();
        }

        // link-flowhandler command
        else if (args.at(0) == "link-flowhandler") {
            if (args.size() < 3) {
                loggy << "Not enough arguments. Usage: link-flowhandler [ip-address] [port]" << std::endl;
                continue;
            } else if (mca_veriflow->flowhandler_linked) {
                loggy << "FlowHandler already linked. Try reset-fh first" << std::endl;
                continue;
            } else if (mca_veriflow->runService) {
                loggy << "All services are running, please use stop first." << std::endl;
                continue;
            } else if (!mca_veriflow->controller_linked) {
                loggy << "Controller not linked! Try link-controller first" << std::endl;
                continue;
            } else {
                mca_veriflow->controller.setFlowHandlerIP(args.at(1), args.at(2));
                Controller::pauseOutput = true;
                mca_veriflow->flowhandler_linked = mca_veriflow->controller.startFlow(&(mca_veriflow->flowhandler_linked));
            }
        }

        // reset-fh command
        else if (args.at(0) == "reset-fh") {
            if (!mca_veriflow->flowhandler_linked) {
                loggy << "FlowHandler not linked. Try link-flowhandler first" << std::endl;
                continue;
            } else if (mca_veriflow->runService) {
                loggy << "All services are running, please use stop first." << std::endl;
                continue;
            }
            else {
                // Once this is false, thread will auto-cleanup
                mca_veriflow->flowhandler_linked = false;
            }
        }

        else if (args.at(0) == "ccpdn-ports") {
            if (args.size() < 2) {
                loggy << "Not enough arguments. Usage: ccpdn-ports [veriflow-port]" << std::endl;
                continue;
            } else if (!mca_veriflow->topology_initialized) {
                loggy << "Topology not initialized. Try reg-top [topology_file] first." << std::endl;
                continue;
            } else if (mca_veriflow->runService) {
                loggy << "All services are running, please use stop first." << std::endl;
                continue;
            } else {
                int BasePort = 6658; // Default port
                try {
                    BasePort = std::stoi(args.at(1));
                } catch (const std::invalid_argument& e) {
                    loggy << "Invalid port number. Usage: ccpdn-ports [base-port]" << std::endl;
                    continue;
                }
                int upperLimit = BasePort + mca_veriflow->topology.getTopologyCount() - 1;
                if (BasePort < 0 || BasePort > 65535) {
                    loggy << "Port number should range from 0-65535. Usage: ccpdn-ports [base-port]" << std::endl;
                    continue;
                } else if (upperLimit > 65535) {
                    loggy << "Upper port limit for this base port value exceeds 65535. Usage: ccpdn-ports [base-port]" << std::endl;
                    continue;
                }

                // Set the base port on the controller
                mca_veriflow->controller.basePort = BasePort;
                mca_veriflow->printPorts(BasePort);
            }
        }

        else if (args.at(0) == "retry-ccpdn") {
            if (!mca_veriflow->runService) {
                loggy << "Ensure CCPDN Service is started first." << std::endl;
                continue;
            }  else {
                mca_veriflow->controller.initCCPDN();
            }
        }

        // list-flows command
        else if (args.at(0) == "list-flows") {
            if (!mca_veriflow->runService) {
                loggy << "Ensure CCPDN Service is started first." << std::endl;
                continue;
            }
            else if (args.size() < 2) {
                loggy << "Not enough arguments. Usage: list-flows [switch-ip-address]" << std::endl;
                continue;
            }
            else {
                std::string targetIP = args.at(1);
                // Ensure IP exists within host topology
                Node n = mca_veriflow->topology.getNodeByIP(targetIP);
                if (n.isEmptyNode()) {
                    loggy << "No such switch in topology." << std::endl;
                    continue;
                }

                // Request flows from controller
                Controller::pauseOutput = true;
                std::vector<Flow> flows = mca_veriflow->controller.retrieveFlows(targetIP, true);

                // Print all flows
                loggy << "--- FLOWS " << targetIP << " ---" << std::endl;
                for (int i = 0; i < flows.size(); i++) {
                    Flow f = flows.at(i);
					loggy << "Rule Prefix: " << f.getRulePrefix() << std::endl;
					loggy << "Next Hop IP: " << f.getNextHopIP() << std::endl;
					loggy << "Action: " << (f.actionType() ? "Forward" : "Drop") << std::endl;
                    if (i < (flows.size() - 1)) {
                        loggy << std::endl;
                    }
				}
            }
        }

        // add-flow command
        else if (args.at(0) == "add-flow") {
			if (!mca_veriflow->runService) {
				loggy << "Ensure CCPDN Service is started first." << std::endl;
                continue;
			}
			else if (args.size() < 4) {
				loggy << "Not enough arguments. Usage: add-flow [switch-ip-address] [rule-prefix] [next-hop-ip-address]" << std::endl;
                continue;
			}
            else {
				std::string targetIP = args.at(1);
				// Ensure IP exists within host topology
				Node n = mca_veriflow->topology.getNodeByIP(targetIP);
				if (n.isEmptyNode()) {
					loggy << "No such switch in topology." << std::endl;
                    continue;
				}

                Flow add(args.at(1), args.at(2), args.at(3), true);

				// Add flow to controller
                Controller::pauseOutput = true;
				mca_veriflow->controller.addFlowToTable(add);
			}
		}

        // del-flow command
		else if (args.at(0) == "del-flow") {
			if (!mca_veriflow->runService) {
				loggy << "Ensure CCPDN Service is started first." << std::endl;
                continue;
			}
            else if (args.size() < 4) {
				loggy << "Not enough arguments. Usage: del-flow [switch-ip-address] [rule-prefix] [next-hop-ip-address]" << std::endl;
                continue;
			}
			else {
                // Ensure IP exists within host topology
                Node n = mca_veriflow->topology.getNodeByIP(args.at(1));
                if (n.isEmptyNode()) {
                    loggy << "[CCPDN-ERROR]: No such switch in topology." << std::endl;
                    continue;
                }

				Flow remove(args.at(1), args.at(2), args.at(3), false);

				// Delete flow from controller
                Controller::pauseOutput = true;
                mca_veriflow->controller.removeFlowFromTable(remove);
            }
        }

        // link-controller command
        else if (args.at(0) == "link-controller") {
            if (args.size() < 3) {
				loggy << "Not enough arguments. Usage: link-controller [ip-address] [port]" << std::endl;
                continue;
			} else if (mca_veriflow->controller_linked) {
                loggy << "Controller already linked. Try reset-controller first" << std::endl;
                continue;
            } else if (!mca_veriflow->topology_initialized) {
                loggy << "Topology not initialized. Try reg-top [topology_file] first." << std::endl;
                continue;
            } else {
				mca_veriflow->controller.setControllerIP(args.at(1), args.at(2));
                Controller::pauseOutput = true;
				mca_veriflow->controller_linked = mca_veriflow->controller.startController(&(mca_veriflow->controller_running));
			}
        }

        // reset-controller command
        else if (args.at(0) == "reset-controller") {
            if (!mca_veriflow->controller_linked) {
                loggy << "Controller not linked. Try link-controller first" << std::endl;
                continue;
            } else if (mca_veriflow->flowhandler_linked) {
                loggy << "FlowHandler is linked. Try reset-fh first or stopping all services" << std::endl;
                continue;
            } else if (mca_veriflow->runService) {
                loggy << "All services are running, please use stop first" << std::endl;
                continue;
            }
            else {
                mca_veriflow->controller.freeLink();
                mca_veriflow->controller_linked = false;
            }
        }

        // output-top command
        else if (args.at(0) == "output-top") {
			if (!mca_veriflow->topology_initialized) {
				loggy << "Topology not initialized. Try reg-top [topology_file] first." << std::endl;
                continue;
			} else if (args.size() < 2) {
				loggy << "Not enough arguments. Usage: output-top [file-name]" << std::endl;
                continue;
			} else {
                // Use partitioning algorithm to split the topology into multiple topologies
                Topology partitioned_topologies = mca_veriflow->partitionTopology();
                int hostIndex = mca_veriflow->topology.hostIndex;

                // If we couldn't partition the topology, just return the current config
                if (partitioned_topologies == Topology()) {
                    mca_veriflow->topology.extractIndexTopology(hostIndex).outputToFile(args.at(1) + std::to_string(hostIndex));
                    loggy << "Topology " << std::to_string(hostIndex) << " output to " << args.at(1) + std::to_string(hostIndex) << std::endl;
                    continue;
                }

                // Output partitioned local topology to give to VeriFlow
                if (partitioned_topologies.extractIndexTopology(hostIndex).outputToFile(args.at(1) + std::to_string(hostIndex))) {
                    loggy << "Topology " << std::to_string(hostIndex) << " output to " << args.at(1) + std::to_string(hostIndex) << std::endl;
                    continue;
                } else {
                    loggy << "Error outputting topology " << args.at(1) + std::to_string(hostIndex) << " to file." << std::endl;
                    continue;
                }
			}
		}

        // // refactor-top command -- DEPRECATED
        // else if (args.at(0) == "refactor-top") {
        //     if (!mca_veriflow->topology_initialized) {
        //         loggy << "Topology not initialized. Try reg-top [topology_file] first.\n" << std::endl;
        //     }
		// 	else if (args.size() < 2) {
		// 		loggy << "Not enough arguments. Usage: refactor-top [file-name]\n" << std::endl;
		// 	}
		// 	else {
        //         // Use partitioning algorithm to split the topology into multiple topologies
        //         Topology partitioned_topologies = mca_veriflow->partitionTopology();

        //         for (int i = 0; i < partitioned_topologies.getTopologyCount(); i++) {
		// 			loggy << "--- PARTITIONED TOPOLOGY " << i << " ---" << std::endl;
		// 			loggy << partitioned_topologies.printTopology(i);

        //             // Output new, partitioned topology file to give to VeriFlow
        //             if (partitioned_topologies.extractIndexTopology(i).outputToFile(args.at(1) + std::to_string(i))) {
        //                 loggy << "Topology " << std::to_string(i) << " output to " << args.at(1) + std::to_string(i) << std::endl << std::endl;
        //             }
        //             else {
        //                 loggy << "Error outputting topology " << args.at(1) + std::to_string(i) << " to file.\n" << std::endl;
        //             }
		// 		}
		// 	}
        // }

        else if (args.at(0) == "list-top") {
            if (args.size() < 2) {
                loggy << "Not enough arguments. Usage: list-top [index]" << std::endl;
                continue;
            } else if (!mca_veriflow->topology_initialized) {
                loggy << "Topology not initialized. Try reg-top [topology_file] first" << std::endl;
                continue;
            } else {
                int index = 0;
                try {
                    index = std::stoi(args.at(1));
                } catch (std::exception& e) {
                    loggy << "Invalid index. Usage: list-top [index]" << std::endl;
                    continue;
                }

                if (index < 0 || index >= mca_veriflow->topology.getTopologyCount()) {
                    loggy << "Invalid topology index. Please try again.\n" << std::endl;
                    continue;
                } else {
                    loggy << "--- TOPOLOGY " << index << " ---" << std::endl;
                    loggy << mca_veriflow->topology.printTopology(index);
                }
            }
        }

        // reg-top command
        else if (args.at(0) == "reg-top") {
            if (args.size() < 2) {
                loggy << "Not enough arguments. Usage: reg-top [topology_file]" << std::endl;
                continue;
			} else if (mca_veriflow->controller_linked) {
                loggy << "Controller already linked. Try reset-controller first or stopping any existing services." << std::endl;
                continue;
            } else {
                // Read the topology file and register it
                if (!mca_veriflow->registerTopologyFile(args.at(1))) {
                    loggy << "Error reading topology file. Ensure the file exists and is in the correct format." << std::endl;
					continue;
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
                    HostIndex = getUserInputInt("Please enter the index of the topology this CCPDN is running on: ");

                    // Ensure the topology index is valid
                    if (HostIndex < 0 || HostIndex >= mca_veriflow->topology.getTopologyCount()) {
                        loggy << "Invalid topology index. Please try again.\n" << std::endl;
                        continue;
                    }
                    else {
                        userInputLoop = false;
                        break;
                    }
                }

                // Set the host index in the topology
                mca_veriflow->topology.hostIndex = HostIndex;

                // // Verify the nodes exist in the topology -- DEPRECATED
                // loggy << "Performing ping test on all nodes for verification..." << std::endl;
                // if (!mca_veriflow->verifyTopology()) {
                //     loggy << "Topology verification failed. Are all switches reachable?" << std::endl;
                // } else {
                //     loggy << "Topology verification successful. All switches are reachable." << std::endl;
                // }

                mca_veriflow->topology_initialized = true;
            }
        }

        // run command
        else if (args.at(0) == "start") {
            if (!mca_veriflow->controller_linked || !mca_veriflow->topology_initialized || !mca_veriflow->flowhandler_linked) {
                loggy << "Cannot start CCPDN App. Ensure the controller is linked, Flow Handler is linked and topology is initialized." << std::endl;
                continue;
            } else if (args.size() < 3) {
                loggy << "Not enough arguments. Usage: start [veriflow-ip-address] [veriflow-port]" << std::endl;
                continue;
            } else if (mca_veriflow->controller.basePort == -1) {
                loggy << "CCPDN Base Port not set. Use ccpdn-ports [base-port] first." << std::endl;
                continue;
            } else if (mca_veriflow->runService) {
                loggy << "CCPDN App is already running." << std::endl;
                continue;
            } else {
                mca_veriflow->controller.setVeriFlowIP(args.at(1), args.at(2));
                Controller::pauseOutput = true;
                mca_veriflow->run();
                loggy << "[CCPDN]: App started." << std::endl;
                continue;
            }
        }

        // stop command
        else if (args.at(0) == "stop") {
            if (!mca_veriflow->runService || !mca_veriflow->controller_linked || !mca_veriflow->topology_initialized || !mca_veriflow->flowhandler_linked) {
                loggy << "CCPDN App is not running." << std::endl;
                continue;
            } else {
                mca_veriflow->stop();
                loggy << "[CCPDN]: App stopped." << std::endl;
                continue;
            }
        }
    

        else if (args.at(0) == "run-tcp-test") {
            if (args.size() < 3) {
                loggy << "Usage: run-tcp-test [target-ip] [port]" << std::endl;
                continue;
            } else if (!mca_veriflow->controller_linked || !mca_veriflow->topology_initialized) {
                loggy << "Ensure topology is initialized and controller is linked first." << std::endl;
                continue;
            } else {
                loggy << "Running TCP test..." << std::endl;
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
                continue;
            }
        }

        else if (args.at(0) == "test-verification-time") {
            if (!mca_veriflow->controller_linked || !mca_veriflow->topology_initialized) {
                loggy << "Ensure topology is initialized and controller is linked first." << std::endl;
                continue;
            } else if (args.size() < 2) {
                loggy << "Not enough arguments. Usage: test-verification-time [num-flows]" << std::endl;
                continue;
            } else {
                int numFlows = 0;
                try {
                    numFlows = std::stoi(args.at(1));
                } catch (const std::exception& e) {
                    loggy << "Invalid number of flows. Usage: test-verification-time [num-flows]" << std::endl;
                    continue;
                }

                mca_veriflow->controller.testVerificationTime(numFlows);
            }
        }

        else if (args.at(0) == "test-method") {
            if (args.size() < 2) {
                loggy << "Not enough arguments. Usage: test-method [integer]" << std::endl;
                continue;
            } else if (!mca_veriflow->runService) {
                loggy << "Service should be running before we test." << std::endl;
                continue;
            } else {
                int testMethod = 0;
                try {
                    testMethod = std::stoi(args.at(1));
                } catch (const std::exception& e) {
                    loggy << "Invalid argument. Usage: test-method [integer]" << std::endl;
                    continue;
                }

                // Re-init CCPDN connections
                mca_veriflow->controller.initCCPDN();

                // Send different CCPDN digests
                Flow f("10.0.0.5", "192.168.1.1/24", "10.0.0.6", true);
                Digest reqVer(0, 0, 1, 0, 0, ""); // asks for perform verification with flow
                reqVer.appendFlow(f);

                // Get topology in string format
                std::string topologyString = mca_veriflow->topology.topology_toString(mca_veriflow->topology.hostIndex); // get topology at index n
                Digest sendUp(0, 1, 0, 0, 0, topologyString); // makes the topology located at index n the most up-to-date

                // Get topology
                int* socket = mca_veriflow->controller.getSocketFromIndex(testMethod);

                if (socket == nullptr) {
                    loggy << "[CCPDN-ERROR]: nullptr return for that socket." << std::endl;
                    continue;
                }

                // Send flow request to other CCPDN
                mca_veriflow->controller.sendCCPDNMessage(*socket, reqVer.toJson());

                // Send topology update to other CCPDN
                mca_veriflow->controller.sendCCPDNMessage(*socket, sendUp.toJson());
            }
        }

        // Invalid response
        else {
            loggy << "Invalid command. Try entering 'help' to view commands." << std::endl;
            continue;
        }
    }

    return 0;
}