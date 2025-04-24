#include "Controller.h"
#include <thread>

bool TCPAnalyzer::pingFlag = false;
std::vector<TimestampPacket> TCPAnalyzer::currentPackets;
std::mutex TCPAnalyzer::currentPacketsMutex;
bool Controller::pauseOutput = false;

// MAIN THREADS
void Controller::controllerThread(bool* run)
{
	loggy << "[CCPDN]: Starting controller thread...\n";
	while (*run) {

		// Receive next message from our socket
		std::vector<uint8_t> packet = recvControllerMessages();

		// Parse the packet with no scrutiny to XID
		parsePacket(packet, false);

		// // Parse all received flows -- not needed for this thread, handled in flowHandler
		// for (Flow f : sharedFlows) {
		// 	parseFlow(f);
		// }

		// Use pause flag to hold off on using rstControllerFlag until command says its ok
		// Only command that does this is "retrieveFlows"
		while(pause_rst) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		// Reset the flag once we are finished parsing everything
		if (!noRst) {
			rstControllerFlag();
		}
	}
}

void Controller::flowHandlerThread(bool *run)
{
	loggy << "[CCPDN]: Starting flow handler thread...\n";
	pauseOutput = false;
	while (*run) {

		// Clear our current flow list
		tryClearSharedFlows();
		std::vector<byte> currPacket;

		{
			// Lock mutex to ensure thread safety
			std::lock_guard<std::mutex> lock(TCPAnalyzer::currentPacketsMutex);
			if (!TCPAnalyzer::currentPackets.empty()) {
				// Get the packet with the lowest timestamp
				auto it = std::min_element(TCPAnalyzer::currentPackets.begin(), TCPAnalyzer::currentPackets.end(),
					[](const TimestampPacket& a, const TimestampPacket& b) {
						return a.smallerTime(b);
					});
				currPacket = it->data;
				TCPAnalyzer::currentPackets.erase(it);
			}
		}

		// Parse packet with scrutiny to XID
		parsePacket(currPacket, true);

		// Create optimal vector of flows to parse -- remove duplicates
		std::vector<Flow> operatingFlows = sharedFlows;
		auto end = std::unique(operatingFlows.begin(), operatingFlows.end());
		operatingFlows.erase(end, operatingFlows.end());
		// Erase all "empty" flows
		Flow empty("", "", "", false);
		operatingFlows.erase(std::remove(operatingFlows.begin(), operatingFlows.end(), empty), operatingFlows.end());
		
		// Handle all received flows
		for (Flow f : operatingFlows) {
			parseFlow(f);
		}
	}
}

/// This method should setup the server receiver for CCPDN communication
void Controller::CCPDNThread(bool *run)
{
	// Handles receiving packets from currently connected sockCC connections (CCPDN connections)
	loggy << "[CCPDN]: Starting CCPDN thread...\n";
	
	while (*run) {
		// Check if we have any connections to read from by using select()
		#ifdef __unix__
		fd_set read_sockets;
		FD_ZERO(&read_sockets);
		int max_read = -1;

		// Add current sockets to our monitoring list
		for (int currentSock : acceptedCC) {
			FD_SET(currentSock, &read_sockets);
			if (currentSock > max_read) {
				max_read = currentSock;
			}
		}

		// Monitor all sockets
		int activity = select(max_read + 1, &read_sockets, nullptr, nullptr, nullptr);

		// Select() had an error -- add to error count
		if (activity < 0) {
			continue;
		} else if (activity == 0) { // No activity on sockets
			continue;
		}

		// If we are at this point, theres data on a socket. Find it and do recv() and parse it
		for (int currentSock : acceptedCC) {
			if (FD_ISSET(currentSock, &read_sockets)) {
				recvProcessCCPDN(currentSock);
			}
		}

		#endif
	}
}

// How do we handle received digests? (Call verification functions, topology updates or synchroncity)
void Controller::recvProcessCCPDN(int socket)
{
	// Clear packet and fix size before receival
	int hostIndex = referenceTopology->hostIndex;
	std::vector<uint8_t> packet;
	packet.clear();
	packet.resize(4096);

#ifdef __unix__
	ssize_t bytes_received = recv(socket, packet.data(), packet.size(), 0);
	if (bytes_received < 0) {
		loggyErr("[CCPDN-ERROR]: Failed to receive message from CCPDN instance\n");
		return;
	} else if (bytes_received == 0) {
		loggyErr("[CCPDN-ERROR]: Connection closed by CCPDN instance\n");
		closeAcceptedSocket(socket);
		return;
	}
#endif

	// Skip empty packets
	if (packet.empty()) {
		return;
	}

	// Convert packet to string format for JSON parsing
	std::string packet_str(packet.begin(), packet.end());

	// Check if we have a port number in the packet string
	if (packet_str.size() > 1 && packet_str[0] == 'P') {
		int connectingIndex = -1;
		try {
			connectingIndex = std::stoi(packet_str.substr(1, packet_str.size() - 1));
		} catch (std::exception& e) {
			return;
		}

		mapSocketToIndex(&socket, connectingIndex);
		loggy << "[CCPDN]: Established connection with CCPDN Instance #" << connectingIndex << std::endl;
		return;
	}

	// Based on code returned, apply functionality
	switch (Digest::readDigest(packet_str)) {
		case 0:
			loggy << "[CCPDN]: Calling sendUpdate..." << std::endl;
			break;
		case 1:
			loggy << "[CCPDN]: Calling synchTopology..." << std::endl;
			break;
		case 2:
			loggy << "[CCPDN]: Calling performVerification..." << std::endl;
			loggy << "Flow data: " << Digest::getFlow(packet_str).flowToStr(false) << std::endl;
			break;
		case 3:
			loggy << "[CCPDN]: Sending verification results... (TRUE)" << std::endl;
			break;
		case 4:
			loggy << "[CCPDN]: Sending verification results... (FALSE)" << std::endl;
			break;
		default: // Not recognized digest
			loggy << "[CCPDN]: Digest not recognized!" << std::endl;
			break;
	}
}

/// This method should establish a connection to each CCPDN instance within the topology file
bool Controller::initCCPDN()
{
	int hostIndex = referenceTopology->hostIndex;
	bool successFlag = true;
	// For each topology instance, send a message to expected ports of horizontal CCPDN instances
	for (int i = 0; i < referenceTopology->getTopologyCount(); i++) {

		// Skip all instances until we are at the host index
		if (i < hostIndex) {
			continue;
		}

		// Skip any instances we've already connected to
		if (socketTopologyMap.find(i) != socketTopologyMap.end()) {
			continue;
		}

		// Initiate connections by sending connect() to each instance (CCPDN shares controller IP)
		int expectedPort = basePort + i;
		int expectedSock = -1;
#ifdef __unix__
		expectedSock = socket(AF_INET, SOCK_STREAM, 0);
		if (expectedSock < 0) {
			loggy << "[CCPDN-ERROR]: Could not create CCPDN socket." << std::endl;
			pauseOutput = false;
			successFlag = false;
			continue;
		}

		// Setup the address to connect to
		struct sockaddr_in server_address;
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(expectedPort);
		inet_pton(AF_INET, controllerIP.c_str(), &server_address.sin_addr);

		// Connect to the CCPDN instance
		if (connect(expectedSock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
			loggy << "[CCPDN-ERROR]: Could not connect to CCPDN Instance." << std::endl;
			close(expectedSock);
			pauseOutput = false;
			successFlag = false;
			continue;
		}

		// Display successful connection with host sockaddr ip and port
		struct sockaddr_in local_address;
		socklen_t address_length = sizeof(local_address);
		if (getsockname(expectedSock, (struct sockaddr*)&local_address, &address_length) == 0) {
			loggy << "[CCPDN]: Successfully connected to CCPDN Instance #" << std::to_string(i) << " using port " << ntohs(local_address.sin_port) << std::endl;
		}
#endif
		// Update our socket-topology mapping
		mapSocketToIndex(&expectedSock, i);
		acceptedCC.push_back(expectedSock);

		// Update our new connection with our topology index
		sendCCPDNMessage(expectedSock, "P" + std::to_string(hostIndex));
	}

    return successFlag;
}

#ifdef __unix__
void Controller::CCPDNServerThread(int port, bool *run)
{
	/// INITIALIZATION LOGIC
	int hostIndex = referenceTopology->hostIndex;
	int opt = 1;

	// Create socket descriptor
	if ((sockCC = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		loggy << "[CCPDN-ERROR]: Could not create socket." << std::endl;
		pauseOutput = false;
		*run = false;
		return;
	}

	// Set socket options for server reuse
	if (setsockopt(sockCC, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		loggy << "[CCPDN-ERROR]: Could not set socket options." << std::endl;
		close(sockCC);
		sockCC = -1;
		pauseOutput = false;
		*run = false;
		return;
	}

	// Setup the address to connect to (use controller IP and a new port)
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(port);

	// Bind the socket to port (allows for listening)
	if (bind(sockCC, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
		loggy << "[CCPDN-ERROR]: Could not bind to port " << std::to_string(port) << std::endl;
		close(sockCC);
		sockCC = -1;
		pauseOutput = false;
		*run = false;
		return;
	}

	// Start CCPDN Server Thread
	loggy << "[CCPDN]: Starting server on port " << std::to_string(port) << std::endl;

	int errorCount = 0;
	/// LOOP LOGIC
	while(*run) {
		// Check if our errors have exceeded 5+
		if (errorCount > 4) {
			loggy << "[CCPDN-ERROR]: Too many failures. Stopping CCPDN service" << std::endl;
			close(sockCC);
			sockCC = -1;
			pauseOutput = false;
			*run = false;
			return;
		}

		// Listen for incoming connections (non-blocking call)
		if (listen(sockCC, referenceTopology->getTopologyCount()) < 0) {
			loggy << "[CCPDN-ERROR]: Could not listen on port " << std::to_string(port) << std::endl;
			pauseOutput = false;
			errorCount++;
			continue;
		}

		// Only decrement error count if we had a failure
		if (errorCount > 0) {
			errorCount--;
		}

		//  Accept incoming connections -- modifies the socket to be used for communication
		socklen_t addrLen = sizeof(server_address);
		int acceptedConnection = -1;
		if ((acceptedConnection = accept(sockCC, (struct sockaddr*)&server_address, &addrLen)) < 0) {

			// Check if we just had a sys interrupt (try again in that case)
			if (errno == EINTR) {
				continue;
			}

			loggy << "[CCPDN-ERROR]: Could not accept incoming connection." << std::endl;
			pauseOutput = false;
			errorCount++;
			continue;
		}

		// Add new connection to list of our connections
		acceptedCC.push_back(acceptedConnection);
		// Print our shiny new connection :)
		std::string clientIP = inet_ntoa(server_address.sin_addr);
		int clientPort = ntohs(server_address.sin_port);
		loggy << "[CCPDN]: Accepted new connection from " << clientIP << ":" << clientPort << std::endl;
	}

	// Server and socket closure incase we somehow exit loop
	if (sockCC != -1) {
		close(sockCC);
		sockCC = -1;
	}
	stopCCPDNServer();
	*run = false;
}
#endif

bool Controller::stopCCPDNServer()
{
	// Close all connected CCPDN sockets
	if (sockCC != -1) {
	#ifdef __unix__
		close(sockCC);
		sockCC = -1;
	#endif
	}

	for (int i = 0; i < acceptedCC.size(); i++) {
	#ifdef __unix__
		close(acceptedCC[i]);
		acceptedCC[i] = -1;
	#endif
	}

    return true;
}

void Controller::closeAcceptedSocket(int socket)
{
	#ifdef __unix__
		// Find the index that has the socket in acceptedCC
		auto it = std::find(acceptedCC.begin(), acceptedCC.end(), socket);
		if (it != acceptedCC.end()) {
			acceptedCC.erase(it);
		}
		close(socket); // Close the socket
	#endif
}

void Controller::parseFlow(Flow f)
{
	// Error checking:
	if (f.getSwitchIP() == "" || f.getNextHopIP() == "") {
		recvSharedFlag = true;
		return;
	}

	// Case 0: Verification request, reason: Target IP and forward hops are all within host topology
	bool isLocal = referenceTopology->isLocal(f.getSwitchIP(), f.getNextHopIP(), f.isMod());
	if (f.isMod() && isLocal) {
		// Run verification on the flow rule
		recvSharedFlag = true;
		performVerification(false, f);
		pauseOutput = false;
		return;
	}

	// Case 1:
	// Flow rule (target IP and forward hops) are NOT ALL within host topology
	// Action: run inter-topology verification method on flow rule
	if (f.isMod() && !isLocal) {
        // Find the domain node
        Node* domainNode = nullptr;
        for (Node* dn : domainNodes) {
            if (dn->connectsToTopology(referenceTopology->getTopologyIndex(f.getNextHopIP()))) {
                domainNode = dn;
                break;
            }
        }

        if (domainNode) {
            // Create adjusted flow that routes through domain node
            Flow adjustedFlow(f.getSwitchIP(), f.getRulePrefix(), domainNode->getIP(), true);
            recvSharedFlag = true;
            performVerification(false, adjustedFlow);
            
            // Forward verification request to next topology
            int destTopology = referenceTopology->getTopologyIndex(f.getNextHopIP());
            if (destTopology != -1) {
                requestVerification(destTopology, f);
            }
        }
        pauseOutput = false;
        return;
    }

	// Case 2:
	// For whatever reason, flow rule belongs to separate topology
	// Action: cross-reference global topology -- if it doesn't exist at all, its a black hole and deny
	if (referenceTopology->getTopologyIndex(f.getSwitchIP()) == -1 ||
        referenceTopology->getTopologyIndex(f.getNextHopIP()) == -1) {
        loggy << "[CCPDN-WARNING]: Black hole detected for flow: " << f.flowToStr(false) << std::endl;
        pauseOutput = false;
        return;
    }

	pauseOutput = false;
}

bool Controller::parsePacket(std::vector<uint8_t>& packet, bool xidCheck) {

	// Ensure we have a valid packet
	if (packet.empty() || packet.size() < 0) {
		return false;
	}

	size_t offset = 0;
#ifdef __unix
	while (offset <= packet.size()) {

		// Ensure we have at least a header?
		if (packet.size() - offset < sizeof(ofp_header)) {
			return false;
		}

		// Parse the header
		ofp_header* header = reinterpret_cast<ofp_header*>(packet.data() + offset);
		uint8_t header_type = header->type;
		uint16_t msg_length = ntohs(header->length);
		uint32_t host_endian_XID = ntohl(header->xid);

		// Drop packet if we are not intended to process it based on XID
		int hostTopologyLower = referenceTopology->hostIndex * 1000;
		int hostTopologyUpper = hostTopologyLower + 999;
		if (((static_cast<int>(host_endian_XID) < hostTopologyLower) && xidCheck)
		|| ((static_cast<int>(host_endian_XID) > hostTopologyUpper) && xidCheck)) {
			return false;
		}

		// Based on header type, process our packet
		switch (header_type) {
			case OFPT_HELLO: {
				// Confirms connection was established
				loggy << "[CCPDN]: Received Hello." << std::endl;
				sendOpenFlowMessage(OpenFlowMessage::createHello(host_endian_XID));
				break;
			}
			case OFPT_FEATURES_REQUEST: {
				// Send a features reply -- required for OF protocol
				loggy << "[CCPDN]: Received Features_Request." << std::endl;
				sendOpenFlowMessage(OpenFlowMessage::createFeaturesReply(host_endian_XID));
				break;
			}
			case OFPT_STATS_REQUEST: {
				// Send a stats reply -- required for OF protocol
				ofp_stats_request* request = reinterpret_cast<ofp_stats_request*>(packet.data() + offset);
				uint16_t request_type = ntohs(request->type);
				uint16_t request_flags = ntohs(request->flags);

				// determine type of response
				switch (request_type) {
					case OFPST_DESC: {
						loggy << "[CCPDN]: Received Desc Stats Request." << std::endl;
						sendOpenFlowMessage(OpenFlowMessage::createDescStatsReply(host_endian_XID));
						break;
					}
				}
				break;
			}
			case OFPT_BARRIER_REQUEST: {
				// Send a barrier reply -- required for OF protocol
				loggy << "[CCPDN]: Received Barrier_Request." << std::endl;
				sendOpenFlowMessage(OpenFlowMessage::createBarrierReply(host_endian_XID));
				pauseOutput = false;
				break;
			}
			case OFPT_STATS_REPLY: {
				// Handle stats reply -- used for listing flows. Set fHFlag to true for list-flows
				loggy << "[CCPDN]: Received Stats_Reply." << std::endl;
				ofp_stats_reply* reply = reinterpret_cast<ofp_stats_reply*>(packet.data() + offset);

				handleStatsReply(reply);
				break;
			}
			case OFPT_FLOW_MOD: {
				// Handle flow modification -- used for verification
				loggy << "[CCPDN]: Received Flow_Mod." << std::endl;
				ofp_flow_mod* mod = reinterpret_cast<ofp_flow_mod*>(packet.data() + offset);
				handleFlowMod(mod);
				break;
			}
			case OFPT_FLOW_REMOVED: {
				// Handle flow removal -- used for verification
				loggy << "[CCPDN]: Received Flow_Removed." << std::endl;
				ofp_flow_removed* removed = reinterpret_cast<ofp_flow_removed*>(packet.data() + offset);
				handleFlowRemoved(removed);
				break;
			}
			case OFPT_SET_CONFIG: {
				// Do nothingn
				loggy << "[CCPDN]: Received Set_Config." << std::endl;
				break;
			}
			default:
				// loggy << "[CCPDN]: Unknown message type received. Type: " << std::to_string(static_cast<int>(header_type)) << std::endl;
				break;
		}

		// Move to next message
		offset += msg_length;
	}
#endif

	return true;
}

std::string exec(const std::string& command, std::string match) {
    std::vector<char> buffer(128);
    std::string result;

    #ifdef __unix__
		// Open a pipe to read the output of a command
		FILE* pipe = popen(command.c_str(), "r");
		if (!pipe) {
			throw std::runtime_error("Failed to pipe command: " + std::string(strerror(errno)));
		}

		while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
			// Return the first line that contains the match string, if match is -1 then return first line
			if (strstr(buffer.data(), match.c_str()) != nullptr) {
				result = buffer.data();
				break;
			}
			if (match == "-1") {
				result = buffer.data();
				break;
			}
		}

		// Close the pipe
		pclose(pipe);
	#endif

    return result;
}

bool Controller::requestVerification(int destinationIndex, Flow f)
{
	/// WARNING: Only call this function for cross-domain verification

	// Modify flow rule as necessary for our verification
	Flow verifyFlow = adjustCrossTopFlow(f);

	// Verify destination index exists within current topology
	if (destinationIndex < 0 || destinationIndex >= referenceTopology->getTopologyCount()) {
		return false;
	}

	Digest verificationMessage(false, false, true, referenceTopology->hostIndex, destinationIndex, "");
	verificationMessage.appendFlow(verifyFlow);

	// Send digest method here
	return true;
}

bool Controller::performVerification(bool externalRequest, Flow f)
{
	// Craft the packet
	std::string packet = "[CCPDN] FLOW ";
	packet += f.flowToStr(false);

	// Send the packet, wait for response
	sendVeriFlowMessage(packet);
	recvVeriFlowMessages();
	rstVeriFlowFlag();

	// Decode response
	std::string response = readBuffer(vfBuffer);

	if (response == "[VERIFLOW] Success") {
		return true;
	}

	return false;
}

Controller::Controller()
{
	controllerIP = "";
	controllerPort = "";
	veriflowIP = "";
	veriflowPort = "";
	sockfd = -1;
	sockvf = -1;
	sockfh = -1;
	sockCC = -1;
	referenceTopology = nullptr;
	ofFlag = false;
	vfFlag = false;
	fhFlag = false;
	pauseOutput = false;
	pause_rst = false;
	noRst = false;
	fhXID = -1;
	recvSharedFlag = true;
	basePort = -1;

	acceptedCC.clear();
	sharedFlows.clear();
	sharedPacket.clear();
	domainNodes.clear();
}

// Constructor
Controller::Controller(Topology* t) {
	controllerIP = "";
	controllerPort = "";
	veriflowIP = "";
	veriflowPort = "";
	sockfd = -1;
	sockvf = -1;
	sockfh = -1;
	sockCC = -1;
	referenceTopology = t;
	ofFlag = false;
	vfFlag = false;
	fhFlag = false;
	pauseOutput = false;
	pause_rst = false;
	noRst = false;
	fhXID = -1;
	recvSharedFlag = true;
	basePort = -1;

	acceptedCC.clear();
	sharedPacket.clear();
	sharedFlows.clear();
	domainNodes.clear();
}

// Destructor
Controller::~Controller()
{
	stopCCPDNServer();
	closeSockets();
	acceptedCC.clear();
}

void Controller::setControllerIP(std::string Controller_IP, std::string Controller_Port)
{
	controllerIP = Controller_IP;
	controllerPort = Controller_Port;
}

void Controller::setVeriFlowIP(std::string VeriFlow_IP, std::string VeriFlow_Port)
{
	veriflowIP = VeriFlow_IP;
	veriflowPort = VeriFlow_Port;
}

void Controller::setFlowHandlerIP(std::string fh_IP, std::string fh_Port)
{
	flowIP = fh_IP;
	flowPort = fh_Port;
}

bool Controller::linkVeriFlow()
{
#ifdef __unix__
	// Setup socket
	sockvf = socket(AF_INET, SOCK_STREAM, 0);
	if (sockvf < 0) {
		loggy << "[CCPDN-ERROR]: Could not create veriflow socket." << std::endl;
		pauseOutput = false;
		return false;
	}

	// Setup the address to connect to
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(std::stoi(veriflowPort));
	inet_pton(AF_INET, veriflowIP.c_str(), &server_address.sin_addr);

	// Connect to the controller
	if (connect(sockvf, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
		std::cout << "[CCPDN-ERROR]: Could not connect to veriflow." << std::endl;
		pauseOutput = false;
		return false;
	}
#endif
	return true;
}

bool Controller::linkController() {

	#ifdef __unix__
		// Setup socket
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) {
			pauseOutput = false;
			loggy << "[CCPDN-ERROR]: Could not create controller socket." << std::endl;
			return false;
		}

		// Setup the address to connect to
		struct sockaddr_in server_address;
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(std::stoi(controllerPort));
		inet_pton(AF_INET, controllerIP.c_str(), &server_address.sin_addr);

		// Connect to the controller
		if (connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
			pauseOutput = false;
			loggy << "[CCPDN-ERROR]: Could not connect to controller." << std::endl;
			return false;
		}

		// Display successful connection with host sockaddr ip and port
		struct sockaddr_in local_address;
		socklen_t address_length = sizeof(local_address);
		if (getsockname(sockfd, (struct sockaddr*)&local_address, &address_length) == 0) {
			loggy << "[CCPDN]: Successfully connected to controller using port " << ntohs(local_address.sin_port) << std::endl;
		}

	#endif

	return true;
}

bool Controller::linkFlow()
{
    #ifdef __unix__
		// Setup socket
		sockfh = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfh < 0) {
			loggy << "[CCPDN-ERROR]: Could not create controller socket." << std::endl;
			pauseOutput = false;
			return false;
		}

		// Setup the address to connect to
		struct sockaddr_in server_address;
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(std::stoi(flowPort));
		inet_pton(AF_INET, flowIP.c_str(), &server_address.sin_addr);

		// Connect to the controller
		if (connect(sockfh, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
			pauseOutput = false;
			loggy << "[CCPDN-ERROR]: Could not connect to FlowHandler." << std::endl;
			return false;
		}

		// Display successful connection with host sockaddr ip and port
		struct sockaddr_in local_address;
		socklen_t address_length = sizeof(local_address);
		if (getsockname(sockfh, (struct sockaddr*)&local_address, &address_length) == 0) {
			pauseOutput = false;
			loggy << "[CCPDN]: Successfully connected to FlowHandler using port " << ntohs(local_address.sin_port) << std::endl;
		}

	#endif
	return true;
}

bool Controller::startController(bool* thread)
{
	*thread = true;

	// Start the controller thread
	std::thread controllerThread(&Controller::controllerThread, this, thread);
	controllerThread.detach();

	// Used for linking and confirming controller. Does not start the CCPDN service
	if (linkController()) {
		return true;
	}
	return false;
}

bool Controller::startFlow(bool *thread)
{
	*thread = true;

	// Start the flow handler thread
	std::thread flowThread(&Controller::flowHandlerThread, this, thread);
	flowThread.detach();

	if (linkFlow()) {
		return true;
	}

    return false;
}

bool Controller::start()
{
	if (linkVeriFlow()) {
		// Send hello to VeriFlow, and wait for response. If received, we're good to start our thread
		veriFlowHandshake();
		return true;
	}
	return false;
}

// Kill the controller thread, free the socket, and reset the flag
bool Controller::freeLink()
{
	if (sockfd != -1) {
		#ifdef __unix__
			close(sockfd);
		#endif
		sockfd = -1;
		return true;
	}
	rstControllerFlag();
	return false;
}

bool Controller::addFlowToTable(Flow f)
{    
	// Set reference DPIDs
	std::string switchDP = std::to_string(getDPID(f.getSwitchIP()));
    std::string output = std::to_string(getOutputPort(f.getSwitchIP(), f.getNextHopIP()));
	f.setDPID(switchDP, output);

	// Update XID mapping, use to track the return flow
	int genXID = generateXID(referenceTopology->hostIndex);
	updateXIDMapping(genXID, f.getSwitchIP(), f.getNextHopIP());

    // Send the OpenFlow message to the flowhandler, flow already should have DPIDs
	return sendFlowHandlerMessage("addflow-" + f.flowToStr(true) + "-" + std::to_string(genXID)); // true for add action
}

bool Controller::removeFlowFromTable(Flow f)
{
	// Craft an OpenFlow message with our given flow rule, ask for removal and send
    std::string switchIP = f.getSwitchIP();
    std::vector<Flow> flows = retrieveFlows(switchIP, false);
    
    // Search for matching flow
    for (Flow existingFlow : flows) {
        if (existingFlow == f) {
			// Set our DPIDs for reference
			std::string switchDP = std::to_string(getDPID(existingFlow.getSwitchIP()));
			std::string output = std::to_string(getOutputPort(existingFlow.getSwitchIP(), existingFlow.getNextHopIP()));
			existingFlow.setDPID(switchDP, output);

			// Update XID mapping, use to track the return flow
			int genXID = generateXID(referenceTopology->hostIndex);
			updateXIDMapping(genXID, existingFlow.getSwitchIP(), existingFlow.getNextHopIP());
            
			// Send the removal message to the controller
			return sendFlowHandlerMessage("removeflow-" + existingFlow.flowToStr(true) + "-" + std::to_string(genXID));
        }
    }
    
    // No matching flow found
    loggyErr("[CCPDN-ERROR]: No matching flow found to remove\n");
	pauseOutput = false;
	recvSharedFlag = true;
    return false;
}

std::vector<Flow> Controller::retrieveFlows(std::string IP, bool pause)
{
	std::vector<Flow> flows;

	// Wait for ofFlag to be set to true, indicating we have received the flow list
	bool sent = false;
	int localCount = 0;
	pause_rst = true;
	fhFlag = false;
	while (!fhFlag) {
		// Timeout
		if (localCount > 15) {
			loggyErr("[CCPDN-ERROR]: Timeout waiting for flow list from controller\n");
			pause_rst = false;
			if (pause) {
				pauseOutput = false;
			}
			return flows;
		}
		if (!sent) {
			// Get the DPID associated with the given IP address
			std::string dpid = std::to_string(getDPID(IP));

			// Update XID mapping, use to track the return flow
			int genXID = generateXID(referenceTopology->hostIndex);
			fhXID = genXID;
			updateXIDMapping(genXID, IP, "");

			// Send the FlowHandler message and wait for response
			if (!sendFlowHandlerMessage("listflows-" + dpid + "-" + std::to_string(genXID))) {
				loggyErr("[CCPDN-ERROR]: Failed to retrieve flow list\n");
				pause_rst = false;
				if (pause) {
					pauseOutput = false;
				}
				return flows;
			}
			else {
				sent = true;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		localCount++;
	}

	// Reset fHFlag since the statsreply packet has been received
	fhFlag = false;
	if (pause) {
		pauseOutput = false;
	}

	for (Flow f : sharedFlows) {
		if (f.getSwitchIP() == IP && !f.isMod()) {
			flows.push_back(f);
		}
	}

	pause_rst = false;
	recvSharedFlag = true;
	return flows;
}

bool Controller::addDomainNode(Node* n)
{
	if (n->isDomainNode()) {
		domainNodes.push_back(n);
		return true;
	}
	return false;
}

bool Controller::sendOpenFlowMessage(std::vector<unsigned char> data)
{
	std::string msg = "";
	std::string warningString = "";
#ifdef __unix__
	// Send the header
	ssize_t bytes_sent = send(sockfd, data.data(), data.size(), 0);
	if (bytes_sent < 0) {
		loggyErr("[CCPDN-ERROR]: Failed to send OpenFlow Message.\n");
		return false;
	} else if (bytes_sent != data.size()) {
		warningString = std::to_string(bytes_sent) + " bytes transmitted but expected " + std::to_string(data.size()) + " bytes.";
	}
#endif

	// Grab header (unsafe, only for logs)
	ofp_header Header;
	if (!(data.size() < sizeof(ofp_header))) { // Only copy if we have 8 bytes or more
		std::memcpy(&Header, data.data(), sizeof(ofp_header));
	}

	// Based on type, print specific message
	switch (Header.type) {
		case OFPT_HELLO:
			msg = "Hello";
			break;
		case OFPT_ECHO_REQUEST:
			msg = "Echo Request";
			break;
		case OFPT_ECHO_REPLY:
			msg = "Echo Reply";
			break;
		case OFPT_VENDOR:
			msg = "Vendor";
			break;
		case OFPT_FEATURES_REQUEST:
			msg = "Features Request";
			break;
		case OFPT_FEATURES_REPLY:
			msg = "Features Reply";
			break;
		case OFPT_GET_CONFIG_REQUEST:
			msg = "Get Config Request";
			break;
		case OFPT_GET_CONFIG_REPLY:
			msg = "Get Config Reply";
			break;
		case OFPT_SET_CONFIG:
			msg = "Set Config";
			break;
		case OFPT_PACKET_IN:
			msg = "Packet In";
			break;
		case OFPT_FLOW_REMOVED:
			msg = "Flow Removed";
			break;
		case OFPT_PORT_STATUS:
			msg = "Port Status";
			break;
		case OFPT_PACKET_OUT:
			msg = "Packet Out";
			break;
		case OFPT_FLOW_MOD:
			msg = "Flow Mod";
			break;
		case OFPT_PORT_MOD:
			msg = "Port Mod";
			break;
		case OFPT_STATS_REQUEST:
			msg = "Stats Request";
			break;
		case OFPT_STATS_REPLY:
			msg = "Stats Reply";
			break;
		case OFPT_BARRIER_REQUEST:
			msg = "Barrier Request";
			break;
		case OFPT_BARRIER_REPLY:
			msg = "Barrier Reply";
			break;
		default:
			msg = "Unknown-Type";
			break;
	}

	bool warning = warningString.empty() ? false : true;
	std::string CCPDN_Type = warning ? "[CCPDN-WARNING]: " : "[CCPDN]: ";

	loggyMsg(CCPDN_Type + "Sent " + msg + " message. " + warningString + "\n");

	return true;
}

bool Controller::sendVeriFlowMessage(std::string message)
{
	// Convert message to sendable format, add null-terminating char
	std::vector<char> Msg(message.begin(), message.end());
	Msg.push_back('\0');

#ifdef __unix__
	// Recast message as char array and send it
	ssize_t bytes_sent = send(sockvf, Msg.data(), Msg.size(), 0);
#endif

	// Print send message
	loggyMsg("[CCPDN]: Sent VeriFlow Message.\n");
	for (int i = 0; i < Msg.size(); ++i) {
		loggyMsg(Msg[i]);
	}
	loggyMsg("\n");
	return true;
}

bool Controller::sendFlowHandlerMessage(std::string message)
{
	// Convert message to sendable format
	std::vector<char> Msg(message.begin(), message.end());

#ifdef __unix__
	// Recast message as char array and send it
	ssize_t bytes_sent = send(sockfh, Msg.data(), Msg.size(), 0);
#endif

	// Print send message
	loggyMsg("[CCPDN]: Sent FlowHandler Request.\n");

    return true;
}

bool Controller::sendCCPDNMessage(int socket, std::string message)
{
    // Convert message to sendable format, add null-terminating char
	std::vector<char> Msg(message.begin(), message.end());

#ifdef __unix__
	// Recast message as char array and send it
	ssize_t bytes_sent = send(socket, Msg.data(), Msg.size(), 0);
#endif

	// Print send message
	loggyMsg("[CCPDN]: Sent CCPDN Message.\n");
	for (int i = 0; i < Msg.size(); ++i) {
		loggyMsg(Msg[i]);
	}
	loggyMsg("\n");
	return true;
}

bool Controller::synchTopology(Digest d)
{
	// Create vector of nodes to hold our topology data from payload
	std::vector<Node> topologyData = Topology::string_toTopology(d.getPayload());

	if (topologyData.empty()) {
		loggyErr("[CCPDN-ERROR]: Failed to parse topology data from payload.\n");
		return false;
	}

	// Override the current topology located at the "host index" field from the digest
	int hostIndex = d.getHostIndex();
	// Clears the vector of node objects at the host index
	referenceTopology->topologyList[hostIndex].clear();
	// Replace them with our new topology data
	referenceTopology->topologyList[hostIndex] = topologyData;

	return true;
}

bool Controller::sendUpdate(bool global, int destinationIndex)
{
	// Verify destination index exists within current topology
	if (destinationIndex < 0 || destinationIndex >= referenceTopology->getTopologyCount()) {
		return false;
	}

	// Get the index that this CCPDN instance is located on
	int hostIndex = referenceTopology->hostIndex;
	if (hostIndex == destinationIndex) {
		loggyErr("[CCPDN-ERROR]: Cannot send update to self.\n");
		return false;
	}

	// Grab string format of local topology
	std::string topOutput = referenceTopology->topology_toString(hostIndex);

	if (topOutput.empty()) {
		loggyErr("[CCPDN-ERROR]: Failed to convert topology to string.\n");
		return false;
	}

	// Create a digest object with topOutput as payload
	Digest singleMessage(false, true, false, hostIndex, destinationIndex, topOutput);

	// If the global var is true, send this digest to all known topologies (forcing them all to update)
	if (global) {
		bool success = true;
		for (int i = 0; i < referenceTopology->getTopologyCount(); i++) {
			if (i != hostIndex) {
				Digest message(false, true, false, hostIndex, i, topOutput);
				// Send the digest (use sendDigest from controller)
				if (false) {
					success = false;
				}
			}
		}
		return success;
	}

	// Send the digest (use sendDigest from controller)
	return true;
}

std::vector<Node*> Controller::getDomainNodes()
{
	return domainNodes;
}

void Controller::rstControllerFlag()
{
	ofFlag = false;
}

void Controller::rstVeriFlowFlag()
{
	vfFlag = false;
}

int Controller::getDPID(std::string IP)
{
    // Ensure IP exists within current topology
	int hostIndex = referenceTopology->hostIndex;
	if (hostIndex < 0 || hostIndex >= referenceTopology->getTopologyCount()) {
		return -1;
	}
	bool IPExists = false;
	for (Node n : referenceTopology->topologyList[hostIndex]) {
		if (n.getIP() == IP) {
			IPExists = true;
			break;
		}
	}

	// If IP doesn't exist, return a fail
	if (!IPExists) {
		return -1;
	}

	// Run ifconfig to display interface and inet address, filter everything else out
	std::string command1 = "ifconfig | grep -E '^[a-zA-Z0-9]|inet ' | awk '/^[a-zA-Z0-9]/ {iface=$1} /inet / {print iface, $2}' | sed 's/addr://'";
	// Output format "interface ip-address"

	// Match the interface name to the given parameter IP, use the interface name to get the DPID
	std::string sysCommand = command1;
    std::string output = exec(sysCommand.c_str(), IP);
	// Use ' ' as delimiter, only take everything before the delimiter
	if (output.empty()) {
		return -1;
	}
	if (output.find(' ') == std::string::npos) {
		return -1;
	}
	std::string interface = output.substr(0, output.find(' '));
	
	// Run sudo ovs-ofctl and parse output for dpid
	sysCommand = "sudo ovs-ofctl show " + interface + " | sed -n 's/.*dpid:\\([0-9a-fA-F]*\\).*/\\1/p'";
	// Output format is just "dpid"
	std::string dpid = exec(sysCommand.c_str(), "-1");

	return std::stoi(dpid);
}

int Controller::getOutputPort(std::string srcIP, std::string dstIP)
{
    // Ensure both IPs exist within current topology
	int hostIndex = referenceTopology->hostIndex;
	if (hostIndex < 0 || hostIndex >= referenceTopology->getTopologyCount()) {
		return -1;
	}

	bool srcIPExists = false;
	bool dstIPExists = false;

	for (Node n : referenceTopology->topologyList[hostIndex]) {
		if (n.getIP() == srcIP) {
			srcIPExists = true;
		}
		if (n.getIP() == dstIP) {
			dstIPExists = true;
		}
	}

	// If either IP doesn't exist, return a fail
	if (!srcIPExists || !dstIPExists) {
		return -1;
	}

	// Get nodes associated with srcIP and dstIP, and analyze their links
	Node srcNode = referenceTopology->getNodeByIP(srcIP);
	Node dstNode = referenceTopology->getNodeByIP(dstIP);

	// Get all links connected to srcNode, sort by their DPIDs
	std::vector<std::string> srcLinks = srcNode.getLinks();
	std::vector<int> dpidLinks;
	int dstDPID = getDPID(dstIP);

	for (std::string link : srcLinks) {
		// Get the DPID associated with the link
		int dpid = getDPID(link);
		dpidLinks.push_back(dpid);
	}

	// Select the DPID matching the dstNode IP, and return the index of the link
	for (int i = 0; i < dpidLinks.size(); ++i) {
		if (dpidLinks[i] == dstDPID) {
			return i; // Return the index of the link
		}
	}

	return -1; // No matching link found
}

std::string Controller::getIPFromOutputPort(std::string srcIP, int outputPort)
{
    // Ensure srcIP exists within topology
	int hostIndex = referenceTopology->hostIndex;
	if (hostIndex < 0 || hostIndex >= referenceTopology->getTopologyCount()) {
		return "";
	}

	bool srcIPExists = false;
	for (Node n : referenceTopology->topologyList[hostIndex]) {
		if (n.getIP() == srcIP) {
			srcIPExists = true;
			break;
		}
	}

	if (!srcIPExists) {
		return "";
	}

	// Get the node associated with srcIP
	Node srcNode = referenceTopology->getNodeByIP(srcIP);

	// Get all links connected to srcNode, sort by their DPIDs
	std::vector<std::string> srcLinks = srcNode.getLinks();
	std::vector<int> dpidLinks;

	for (std::string link : srcLinks) {
		// Get the DPID associated with the link
		int dpid = getDPID(link);
		dpidLinks.push_back(dpid);
	}

	// Return the link based using the outputPort as an index
	return srcLinks[outputPort];
}

void Controller::tryClearSharedFlows()
{
	if (!recvSharedFlag) {
		return;
	}

	// Clear the shared flows vector
	sharedFlows.clear();
}

Flow Controller::adjustCrossTopFlow(Flow f)
{
	/// Since we are only handling loops, this method should only handle loops logic (w/ resubmits)
	
	// Check if the switch IP exists in the global topology. If it doesn't, return an empty flow to indicate no verification to be done (black hole)

	// First, grab all flows associated with the target switch for this flow rule (Node_T)
	std::vector<Flow> totalFlows = {};

	// Craft a new flow rule (Flow_A) with the host topology domain node to be used as the new target switch (Node_D)

	// Add all of the flow rules from Node_T to the new target switch (Node_D, aka domain node)

	// Add totalFlows to Node_D, run verification -- if valid:

	// Return the new flow rule (Flow_A) to be used for verification

	return Flow();
}

bool Controller::updateXIDMapping(uint32_t xid, std::string srcIP, std::string dstIP)
{
	// Return true if mapping was added successfully
	// Return false if mapping already exists, and had to be overwritten

	bool returnVal = true;
	if (xidFlowMap.find(xid) != xidFlowMap.end()) {
		returnVal = false;
	}

	xidFlowMap[xid] = std::make_pair(srcIP, dstIP);

	return returnVal;
}

std::string Controller::getSrcFromXID(uint32_t xid)
{
	std::pair<std::string, std::string> IPs = xidFlowMap[xid];
	if (IPs.first.empty()) {
		return "";
	}

	return IPs.first;
}

std::string Controller::getDstFromXID(uint32_t xid)
{
    std::pair<std::string, std::string> IPs = xidFlowMap[xid];
	if (IPs.second.empty()) {
		return "";
	}

	return IPs.second;
}

int Controller::generateXID(int topologyIndex)
{
	// Each topology will have a range of 1000 values, so topology 0 has 0-999, topology 1 has 1000-1999, etc.
	int baseXID = topologyIndex * 1000;
	int randomXID = rand() % 1000; // Random number between 0 and 999
	
    return baseXID + randomXID;
}

void Controller::veriFlowHandshake()
{
	sendVeriFlowMessage("[CCPDN] Hello");
	recvVeriFlowMessages();
	rstVeriFlowFlag();
}

std::vector<uint8_t> Controller::recvControllerMessages()
{
	std::vector<uint8_t> packet;
	// Clear packet and fix size before receival
	packet.clear();
	packet.resize(4096);

#ifdef __unix__

	if (!ofFlag) {
			
		// Receive a message from the socket -- only do so when our ofFlag is inactive, meaning we're NOT using the buffer
		ssize_t bytes_received = recv(sockfd, packet.data(), packet.size(), 0);
		if (bytes_received < 0) {
			loggyErr("[CCPDN-ERROR]: Failed to receive message from controller\n");
			ofFlag = true;
			noRst = true;
			return {};
		} else if (bytes_received == 0) {
			loggyErr("[CCPDN-ERROR]: Connection closed by controller\n");
			ofFlag = true;
			noRst = true;
			return {};
		}

		// Resize to bytes received
		packet.resize(bytes_received);

		// Enable our safety flag
		ofFlag = true;
	}
#endif
	return packet;
}

void Controller::recvVeriFlowMessages()
{
#ifdef __unix__
	// Receive a message from the socket -- only do so when our vfFlag is inactive, meaning we're NOT using the buffer
	if (!vfFlag) {
		std::memset(vfBuffer, 0, sizeof(vfBuffer)); // Clear the buffer before receiving data
		ssize_t bytes_received = recv(sockvf, vfBuffer, sizeof(vfBuffer), 0);
		loggyMsg("[CCPDN]: Message from VeriFlow\n");
		loggyMsg(readBuffer(vfBuffer));
		loggyMsg("\n");
		vfFlag = true;
	}
#endif
}

void Controller::handleStatsReply(ofp_stats_reply* reply)
{
	// Null check
	if (reply == nullptr) {
		return;
	}

#ifdef __unix__
	// Fix endian-ness of reply
	reply->header.length = ntohs(reply->header.length);
	reply->header.xid = ntohl(reply->header.xid);
	reply->type = ntohs(reply->type);

	// Only stats reply we care about are flows
	if (reply->type != OFPST_FLOW) {
		return;
	}

	// Calculate body size
	size_t body_size = reply->header.length - sizeof(ofp_stats_reply);
	size_t offset = 0;

	// Iterate through each flow_stat given in the packet
	while (body_size >= sizeof(ofp_flow_stats)) {

		// Cast ptr to access flow_stats struct
		ofp_flow_stats* flow_stats = reinterpret_cast<ofp_flow_stats*>(reply->body + offset);

		// Process length of current entry -- handle end of ptr
		size_t flow_length = ntohs(flow_stats->length);
		if (flow_length == 0 || flow_length > body_size) {
			break;
		}

		// Cast ptr to access ofp_action_header struct
		ofp_action_header* action_header = reinterpret_cast<ofp_action_header*>(flow_stats->actions);

		// Flow processing
		uint32_t rulePrefixIP = ntohl(flow_stats->match.nw_src);
		uint32_t wildcards = ntohl(flow_stats->match.wildcards);
		uint16_t output_port = ntohs(action_header->port);

		// Create string formats
		std::string targetSwitch = getSrcFromXID(reply->header.xid);
		std::string nextHop = getIPFromOutputPort(targetSwitch, output_port);
		std::string rulePrefix = OpenFlowMessage::getRulePrefix(wildcards, rulePrefixIP);

		// Add flow to shared flows
		recvSharedFlag = false;
		Flow f = Flow(targetSwitch, rulePrefix, nextHop, true);
		f.setMod(false);
		sharedFlows.push_back(f);

		// Set fhFlag if we are expecting this as a list-flows return
		if (reply->header.xid == fhXID) {
			fhXID = -1;
			fhFlag = true;
		}

		// Move to next entry
		offset += flow_length;
		body_size -= flow_length;
	}
#endif
}

void Controller::handleFlowMod(ofp_flow_mod *mod)
{
	// Null check
	if (mod == nullptr) {
		return;
	}

#ifdef __unix__
	// Fix endianness on header, look at header
	mod->header.length = ntohs(mod->header.length);
	mod->header.xid = ntohl(mod->header.xid);

	// Ensure our packet matches the minimum size of an ofp_flow_mod
	if (mod->header.length < sizeof(ofp_flow_mod)) {
		return;
	}

	// Flow processing
	uint32_t rulePrefixIP = ntohl(mod->match.nw_src);
	uint32_t wildcards = ntohl(mod->match.wildcards);
	bool command = mod->command == OFPFC_ADD ? true : false;
	bool flow_mod = mod->command == (OFPFC_MODIFY || OFPFC_MODIFY_STRICT) ? true : false;

	// Create string formats
	std::string targetSwitch = getSrcFromXID(mod->header.xid);
	std::string nextHop = getDstFromXID(mod->header.xid);
	std::string rulePrefix = OpenFlowMessage::getRulePrefix(wildcards, rulePrefixIP);

	// Check if the flow rule is valid
	if (targetSwitch == "0" || nextHop == "0") {
		loggyErr("[CCPDN-ERROR]: Parsed flow rule contains no flow information.\n");
		return;
	}
	
	// Add flow to shared flows -- since it is added, do true
	recvSharedFlag = false;
	Flow f = Flow(targetSwitch, rulePrefix, nextHop, command);
	f.setMod(true);
	sharedFlows.push_back(f);
#endif
}

void Controller::handleFlowRemoved(ofp_flow_removed *removed)
{
	// Null check
	if (removed == nullptr) {
		return;
	}

#ifdef __unix__
	// Fix endianness on header, look at header
	removed->header.length = ntohs(removed->header.length);
	removed->header.xid = ntohl(removed->header.xid);

	// Ensure our packet matches the minimum size of an ofp_flow_removed
	if (removed->header.length < sizeof(ofp_flow_removed)) {
		return;
	}

	// Flow processing
	uint32_t rulePrefixIP = ntohl(removed->match.nw_src);
	uint32_t wildcards = ntohl(removed->match.wildcards);

	// Create string formats
	std::string targetSwitch = getSrcFromXID(removed->header.xid);
	std::string nextHop = getDstFromXID(removed->header.xid);
	std::string rulePrefix = OpenFlowMessage::getRulePrefix(wildcards, rulePrefixIP);

	// Check if the flow rule is valid
	if (targetSwitch == "0" || nextHop == "0") {
		loggyErr("[CCPDN-ERROR]: Parsed flow rule contains no flow information.\n");
		return;
	}

	// Add flow to shared flows -- since it is added, do true
	// recvSharedFlag = false;
	Flow f = Flow(targetSwitch, rulePrefix, nextHop, false);
	f.setMod(true);
	sharedFlows.push_back(f);
#endif
}

std::string Controller::readBuffer(char* buf)
{
	// Dereference the buffer as a char[1024], then convert to a string
	std::string output(buf);
	// Check if the string is empty
	if (output.empty()) {
		loggyErr("[CCPDN-ERROR]: Buffer is empty.\n");
		return std::string();
	}

	return output;
}

void Controller::testVerificationTime(int numFlows) {
    std::vector<std::string> switchIPs;
    for (Node n : referenceTopology->getTopology(referenceTopology->hostIndex)) {
        if (n.isSwitch() && switchIPs.size() < 5) {
            switchIPs.push_back(n.getIP());
        }
    }

    if (switchIPs.size() < 2) {
        loggy << "Not enough switches in topology (need at least 2)" << std::endl;
        return;
    }

    // Create test flows using these switches
    std::vector<Flow> testFlows;
	for (int i = 0; i < numFlows; i++) {
		bool willRemove = std::rand() % 2;
		int randMask = std::rand() % 32 + 1;
		int randIPs[4];
		for (int j = 0; j < 4; j++) {
			randIPs[j] = std::rand() % 256;
		}
		std::string randPrefix = std::to_string(randIPs[0]) + "." + std::to_string(randIPs[1]) 
		+ "." + std::to_string(randIPs[2]) + "." + std::to_string(randIPs[3]) + "/" + std::to_string(randMask);

		Flow add = Flow(switchIPs[0], randPrefix, switchIPs[1], true);
		Flow remove = Flow(switchIPs[0], randPrefix, switchIPs[1], false);

		// Add flows to the vector
		testFlows.push_back(add);
		if (willRemove && (i == (numFlows - 1))) {
			testFlows.push_back(remove);
		}
	}

	// Order all adds to be first in the vector, then remove flows
	std::sort(testFlows.begin(), testFlows.end(), [](Flow& a, Flow& b) {
		return a.actionType() && !b.actionType();
	});

    std::vector<double> verificationTimes;

    // Test each flow and record verification time
    for (Flow& flow : testFlows) {
        auto start = std::chrono::high_resolution_clock::now();
        performVerification(false, flow);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        verificationTimes.push_back(duration.count());
        
        loggy << "Flow verification: " << flow.flowToStr(false) 
              << " took " << duration.count() * 1000 << " ms" << std::endl;
    }

    // Calculate statistics
    if (!verificationTimes.empty()) {
        std::sort(verificationTimes.begin(), verificationTimes.end());
        
        double sum = std::accumulate(verificationTimes.begin(), verificationTimes.end(), 0.0);
        double average = sum / verificationTimes.size();
        double median = verificationTimes[verificationTimes.size()/2];
        double lowest = verificationTimes.front();
        double highest = verificationTimes.back();

        loggy << "\nVerification Time Statistics:" << std::endl;
        loggy << "Average: " << average * 1000 << " ms" << std::endl;
        loggy << "Median: " << median * 1000 << " ms" << std::endl;
        loggy << "Lowest: " << lowest * 1000 << " ms" << std::endl;
        loggy << "Highest: " << highest * 1000 << " ms" << std::endl;
    }
}

void Controller::closeSockets()
{
	if (sockvf != -1) {
        #ifdef __unix__
            close(sockvf);
        #endif
        sockvf = -1;
    }
    if (sockfd != -1) {
        #ifdef __unix__
            close(sockfd);
        #endif
        sockfd = -1;
    }
    if (sockfh != -1) {
        #ifdef __unix__
            close(sockfh);
        #endif
        sockfh = -1;
    }
}

void Controller::mapSocketToIndex(int* socket, int index)
{
	// Map the socket to the index in the socketMap
	socketTopologyMap[index] = socket;
}

int* Controller::getSocketFromIndex(int index)
{
	// Check if the index exists in the socketMap
	if ((socketTopologyMap.find(index) != socketTopologyMap.end()) && index != referenceTopology->hostIndex) {
		return socketTopologyMap[index];
	}

	return nullptr;
}
