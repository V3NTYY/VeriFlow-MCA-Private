#include "Controller.h"
#include <thread>

// MAIN THREADS
void Controller::controllerThread(bool* run)
{
	std::cout << "Start thread...\n";
	while (*run) {

		// Clear our current flow list
		sharedFlows.clear();

		// Receive next message from our socket
		std::vector<uint8_t> packet = recvControllerMessages();

		// Parse the packet
		parsePacket(packet);

		// For now, print any flows received
		for (Flow f : sharedFlows) {
			f.print();
		}

		// Parse the given flow to determine actions to take
		// int code = parseFlow(recvFlow);

		// Case 0:
		// Flow rule (target IP and forward hops) is within host topology
		// Action: run verification on flow rule

		// Case 1:
		// Flow rule (target IP and forward hops) are NOT ALL within host topology
		// Action: run inter-topology verification method on flow rule

		// Case 3:
		// For whatever reason, flow rule belongs to separate topology
		// Action: cross-reference global topology -- if it doesn't exist at all, its a black hole and deny

		// Case 4:
		// We are receiving an openflow message with return flow list.
		// Action: Do nothing, another method will be handling this

		// Reset the flag once we are finished parsing everything
		rstControllerFlag();
	}
}

bool Controller::parsePacket(std::vector<uint8_t>& packet) {

	// Ensure we have a valid packet
	if (packet.empty()) {
		std::cout << "[CCPDN-ERROR]: Empty packet, cancelling read." << std::endl;
		return false;
	}

	// Create pointers to cast to
	ofp_header* ofHeader = reinterpret_cast<ofp_header*>(packet.data());
	ofp_stats_reply* ofStatsReply = reinterpret_cast<ofp_stats_reply*>(packet.data());

	// Parse the header first
	handleHeader(ofHeader);
	handleStatsReply(ofStatsReply);

	return true;
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

	return verificationMessage.sendDigest(this);
}

bool Controller::performVerification(bool externalRequest, Flow f)
{
	// Craft the packet
	std::string packet = "[CCPDN] FLOW ";
	packet += f.flowToStr();

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
	activeThread = false;
	referenceTopology = nullptr;
	ofFlag = false;
}

// Constructor
Controller::Controller(Topology* t) {
	controllerIP = "";
	controllerPort = "";
	veriflowIP = "";
	veriflowPort = "";
	sockfd = -1;
	sockvf = -1;
	activeThread = false;
	referenceTopology = t;
	ofFlag = false;
}

// Destructor
Controller::~Controller()
{
	#ifdef __unix__
		close(sockfd);
	#endif
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

bool Controller::linkVeriFlow()
{
#ifdef __unix__
	// Setup socket
	sockvf = socket(AF_INET, SOCK_STREAM, 0);
	if (sockvf < 0) {
		std::cout << "Error creating socket" << std::endl;
		return false;
	}

	// Setup the address to connect to
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(std::stoi(veriflowPort));
	inet_pton(AF_INET, veriflowIP.c_str(), &server_address.sin_addr);

	// Connect to the controller
	if (connect(sockvf, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
		std::cout << "Error connecting to veriflow" << std::endl;
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
			std::cout << "Error creating socket" << std::endl;
			return false;
		}

		// Setup the address to connect to
		struct sockaddr_in server_address;
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(std::stoi(controllerPort));
		inet_pton(AF_INET, controllerIP.c_str(), &server_address.sin_addr);

		// Connect to the controller
		if (connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
			std::cout << "Error connecting to controller" << std::endl;
			return false;
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
		sendOpenFlowMessage(OpenFlowMessage::createHello());
		return true;
	}
	return false;
}

bool Controller::start()
{
	// Prompt user for veriflow ip and port
	std::cout << "Enter VeriFlow IP: ";
	std::cin >> veriflowIP;
	std::cout << "Enter VeriFlow Port: ";
	std::cin >> veriflowPort;

	if (linkVeriFlow()) {
		// Send hello to VeriFlow, and wait for response. If received, we're good to start our thread
		veriFlowHandshake();
		return true;
	}
	return false;
}

// Free the link to the domain node
bool Controller::freeLink()
{
	if (sockfd != -1) {
		#ifdef __unix__
			close(sockfd);
		#endif
		sockfd = -1;
		return true;
	}
	return false;
}

bool Controller::addFlowToTable(Flow f)
{
	// Craft an OpenFlow message with our given flow rule, ask to add and send
	uint32_t xid = static_cast<uint32_t>(100 + (std::rand() % 4095 - 99));
    // Craft flow mod message
    
    // Send the OpenFlow message to the controller

	return false;
}

bool Controller::removeFlowFromTable(Flow f)
{
	// Craft an OpenFlow message with our given flow rule, ask for removal and send
    std::string switchIP = f.getSwitchIP();
    std::vector<Flow> flows = retrieveFlows(switchIP);
    
    // Search for matching flow
    for (Flow existingFlow : flows) {
        if (existingFlow.getSwitchIP() == f.getSwitchIP() &&
            existingFlow.getRulePrefix() == f.getRulePrefix() &&
            existingFlow.getNextHopIP() == f.getNextHopIP()) {
            
            uint32_t xid = static_cast<uint32_t>(100 + (std::rand() % 4095 - 99));            
            // Craft flow removal message
            
            // Send the removal message to the controller
            
            return true;
        }
    }
    
    // No matching flow found
    std::cout << "[CCPDN]: No matching flow found to remove" << std::endl;
    return false;
}

std::vector<Flow> Controller::retrieveFlows(std::string IP)
{
	std::vector<Flow> flows;

	// Wait for ofFlag to be set to true, indicating we have received the flow list
	bool sent = false;
	while (!ofFlag) {
		if (!sent) {
			// Send this request to controller
			if (!sendOpenFlowMessage(OpenFlowMessage::createFlowRequest())) {
				std::cout << "[CCPDN]: Failed to send flow list request to controller" << std::endl;
				return flows;
			}
			else {
				sent = true;
			}
		}
	}

	flows = sharedFlows;
	rstControllerFlag();
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

bool Controller::sendOpenFlowMessage(ofp_header Header)
{
#ifdef __unix__
	// Send the header
	ssize_t bytes_sent = send(sockfd, &Header, sizeof(Header), 0);
	if (bytes_sent != sizeof(Header)) {
		std::cerr << "[CCPDN-ERROR]: Failed to send OpenFlow Header" << std::endl;
		return false;
	}
#endif

	std::cout << "--- [CCPDN-MESSAGE-POX] ---\n";

	return true;
}

bool Controller::sendOpenFlowMessage(ofp_stats_request Request)
{
#ifdef __unix__
	// Send the OFP_Stats Request
	ssize_t bytes_sent = send(sockfd, &Request, sizeof(Request), 0);
	if (bytes_sent != sizeof(Request)) {
		std::cerr << "[CCPDN-ERROR]: Failed to send Flow Request" << std::endl;
		return false;
	}
#endif

	std::cout << "--- [CCPDN-REQUESTFLOWS-POX] ---\n";

	return true;
}

bool Controller::sendOpenFlowMessage(ofp_switch_features Features)
{
#ifdef __unix__
	// Send the Features reply
	ssize_t bytes_sent = send(sockfd, &Features, sizeof(Features), 0);
	if (bytes_sent != sizeof(Features)) {
		std::cerr << "[CCPDN-ERROR]: Failed to send Features" << std::endl;
		return false;
	}
#endif

	std::cout << "--- [CCPDN-SENDFEATURES-POX] ---\n";

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
	std::cout << "--- [CCPDN-MESSAGE-VERIFLOW] --- \n";
	for (int i = 0; i < Msg.size(); ++i) {
		std::cout << Msg[i];
	}
	std::cout << std::endl << std::endl;;
	return false;
}

bool Controller::synchTopology(Digest d)
{
	// Create vector of nodes to hold our topology data from payload
	std::vector<Node> topologyData = Topology::string_toTopology(d.getPayload());

	if (topologyData.empty()) {
		std::cout << "[CCPDN]: ERROR. Failed to parse topology data from payload." << std::endl;
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
		std::cout << "[CCPDN]: ERROR. Cannot send update to self.\n";
		return false;
	}

	// Grab string format of local topology
	std::string topOutput = referenceTopology->topology_toString(hostIndex);

	if (topOutput.empty()) {
		std::cout << "[CCPDN]: ERROR. Failed to convert topology to string." << std::endl;
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
				// Send the digest
				if (!message.sendDigest(this)) {
					success = false;
				}
			}
		}
		return success;
	}

	// Send the digest
	return singleMessage.sendDigest(this);
}

std::vector<Node*> Controller::getDomainNodes()
{
	return domainNodes;
}

void Controller::rstControllerFlag()
{
	std::cout << "[DEBUG]: Resetting controller flag" << std::endl;
	ofFlag = false;
}

void Controller::rstVeriFlowFlag()
{
	vfFlag = false;
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
			std::cerr << "[CCPDN-ERROR]: Failed to receive message from controller" << std::endl;
			ofFlag = true;
			return {};
		} else if (bytes_received == 0) {
			std::cout << "[CCPDN-ERROR]: Connection closed by controller" << std::endl;
			ofFlag = true;
			return {};
		}

		// Resize to bytes received
		packet.resize(bytes_received);

		// Print received data
		std::cout << "--- [POX-MESSAGE-CCPDN] ---\n";
		for (size_t i = 0; i < bytes_received; ++i) {
			std::cout << std::hex << static_cast<int>(packet[i]) << " ";
		}
		std::cout << std::dec << std::endl;

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
		std::cout << "--- [VERIFLOW-MESSAGE-CCPDN] --- \n";
		std::cout << readBuffer(vfBuffer) << std::endl << std::endl;
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
		std::cout << "[CCPDN-ERROR]: Not a flow stats reply, cancelling read. Code: " << reply->type << std::endl;
		return;
	}

	// Calculate pointer and body size information to find current flow_stat object
	uint8_t* ofp_flow_stats_ptr = reinterpret_cast<uint8_t*>(reply) + sizeof(ofp_stats_reply);
	size_t body_size = reply->header.length - sizeof(ofp_stats_reply);
	
	// Iterate through each flow_stat given in the packet
	while (body_size >= sizeof(ofp_flow_stats)) {

		// Cast ptr to access flow_stats struct
		ofp_flow_stats* flow_stats = reinterpret_cast<ofp_flow_stats*>(ofp_flow_stats_ptr);

		// Process length of current entry -- handle end of ptr
		size_t flow_length = ntohs(flow_stats->length);
		if (flow_length == 0 || flow_length > body_size) {
			break;
		}

		// Flow processing
		uint32_t srcIP = ntohl(flow_stats->match.nw_src);
		uint32_t dstIP = ntohl(flow_stats->match.nw_dst);
		uint32_t wildcards = ntohl(flow_stats->match.wildcards);

		// Create string formats
		std::string targetSwitch = std::to_string(srcIP);
		std::string nextHop = std::to_string(dstIP);
		std::string rulePrefix = OpenFlowMessage::getRulePrefix(wildcards, srcIP);
		
		// Add flow to shared flows
		sharedFlows.push_back(Flow(targetSwitch, rulePrefix, nextHop, true));

		// Move to next entry
		ofp_flow_stats_ptr += flow_length;
		body_size -= flow_length;
	}
#endif
}

void Controller::handleHeader(ofp_header* header)
{
	if (header == nullptr) {
		return;
	}
#ifdef __unix__
	header->length = ntohs(header->length);
	header->xid = ntohl(header->xid);
#endif

	// If we receive type OFPT_FEATURES_REQUEST, automatically send our features
	if (header->type == OFPT_FEATURES_REQUEST) {
		sendOpenFlowMessage(OpenFlowMessage::createFeaturesReply(header->xid));
	}

	// For debugging purposes, print out the contents
	std::cout << "Version: " << header->version << std::endl;
	std::cout << "Type: " << header->type << std::endl;
	std::cout << "Length: " << header->length << std::endl;
	std::cout << "XID: " << header->xid << std::endl;
}

std::string Controller::readBuffer(char* buf)
{
	// Dereference the buffer as a char[1024], then convert to a string
	std::string output(buf);
	// Check if the string is empty
	if (output.empty()) {
		std::cout << "[CCPDN-ERROR]: Buffer is empty." << std::endl;
		return std::string();
	}

	return output;
}


