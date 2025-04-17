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
			parseFlow(f);
		}

		// Reset the flag once we are finished parsing everything
		rstControllerFlag();
	}
}

void Controller::parseFlow(Flow f)
{
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
}

bool Controller::parsePacket(std::vector<uint8_t>& packet) {

	// Ensure we have a valid packet
	if (packet.empty()) {
		loggyErr("[CCPDN-ERROR]: Empty packet, cancelling read.\n");
		return false;
	}

	size_t offset = 0;
#ifdef __unix
	while (offset < packet.size()) {
		// Ensure we have at least a header?
		if (packet.size() - offset < sizeof(ofp_header)) {
			return false;
		}

		// Parse the header
		ofp_header* header = reinterpret_cast<ofp_header*>(packet.data() + offset);
		uint8_t header_type = header->type;
		uint16_t msg_length = ntohs(header->length);
		uint32_t host_endian_XID = ntohl(header->xid);

		// Ensure our message has valid length
		if (msg_length < sizeof(ofp_header) || msg_length > packet.size() - offset) {
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
			}
			case OFPT_STATS_REQUEST: {
				// Send a stats reply -- required for OF protocol
				loggy << "[CCPDN]: Received Stats_Request." << std::endl;
				sendOpenFlowMessage(OpenFlowMessage::createDescStatsReply(host_endian_XID));
			}
			case OFPT_BARRIER_REQUEST: {
				// Send a barrier reply -- required for OF protocol
				loggy << "[CCPDN]: Received Barrier_Request." << std::endl;
				sendOpenFlowMessage(OpenFlowMessage::createBarrierReply(host_endian_XID));
				break;
			}
			case OFPT_STATS_REPLY: {
				// Handle stats reply -- used for listing flows
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
			default:
				loggy << "[CCPDN]: Unknown message type received. Type: " << header_type << std::endl;
		}

		// Move to next message
		offset += msg_length;
	}
#endif

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
		std::cout << "[CCPDN-ERROR]: Could not create veriflow socket." << std::endl;
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
			std::cout << "[CCPDN-ERROR]: Could not create controller socket." << std::endl;
			return false;
		}

		// Setup the address to connect to
		struct sockaddr_in server_address;
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(std::stoi(controllerPort));
		inet_pton(AF_INET, controllerIP.c_str(), &server_address.sin_addr);

		// Connect to the controller
		if (connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
			std::cout << "[CCPDN-ERROR]: Could not connect to controller." << std::endl;
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

bool Controller::start()
{
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
    std::cout << "[CCPDN-ERROR]: No matching flow found to remove" << std::endl;
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
				std::cout << "[CCPDN-ERROR]: Failed to send flow list request to controller" << std::endl;
				return flows;
			}
			else {
				sent = true;
			}
		}
	}

	for (Flow f : sharedFlows) {
		if (f.getSwitchIP() == IP) {
			flows.push_back(f);
		}
	}

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

bool Controller::sendOpenFlowMessage(std::vector<unsigned char> data)
{
	// Add a 5ms delay to ensure we don't send too fast
	std::this_thread::sleep_for(std::chrono::milliseconds(5));

	std::string msg = "";
#ifdef __unix__
	// Send the header
	ssize_t bytes_sent = send(sockfd, data.data(), data.size(), 0);
	if (bytes_sent != data.size()) {
		loggyErr("[CCPDN-ERROR]: Failed to send OpenFlow Header\n");
		return false;
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

	loggyMsg("[CCPDN]: Sent " + msg + " message.\n");
#ifdef __unix__
	loggy << "XID: " << ntohl(Header.xid) << std::endl;
#endif

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
	loggyMsg("\n\n");
	return false;
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
	loggyMsg("[CCPDN-DEBUG]: Resetting controller flag\n");
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
			loggyErr("[CCPDN-ERROR]: Failed to receive message from controller\n");
			ofFlag = true;
			return {};
		} else if (bytes_received == 0) {
			loggyErr("[CCPDN-ERROR]: Connection closed by controller\n");
			ofFlag = true;
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
		loggyMsg("\n\n");
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
	uint32_t srcIP = ntohl(mod->match.nw_src);
	uint32_t dstIP = ntohl(mod->match.nw_dst);
	uint32_t wildcards = ntohl(mod->match.wildcards);

	// Create string formats
	std::string targetSwitch = std::to_string(srcIP);
	std::string nextHop = std::to_string(dstIP);
	std::string rulePrefix = OpenFlowMessage::getRulePrefix(wildcards, srcIP);

	// Check if the flow rule is valid
	if (targetSwitch == "0" || nextHop == "0") {
		loggyErr("[CCPDN-ERROR]: Parsed flow rule contains no flow information.\n");
		return;
	}
	
	// Add flow to shared flows -- since it is added, do true
	sharedFlows.push_back(Flow(targetSwitch, rulePrefix, nextHop, true));
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
	uint32_t srcIP = ntohl(removed->match.nw_src);
	uint32_t dstIP = ntohl(removed->match.nw_dst);
	uint32_t wildcards = ntohl(removed->match.wildcards);

	// Create string formats
	std::string targetSwitch = std::to_string(srcIP);
	std::string nextHop = std::to_string(dstIP);
	std::string rulePrefix = OpenFlowMessage::getRulePrefix(wildcards, srcIP);

	// Check if the flow rule is valid
	if (targetSwitch == "0" || nextHop == "0") {
		loggyErr("[CCPDN-ERROR]: Parsed flow rule contains no flow information.\n");
		return;
	}

	// Add flow to shared flows -- since it is added, do true
	sharedFlows.push_back(Flow(targetSwitch, rulePrefix, nextHop, false));
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


