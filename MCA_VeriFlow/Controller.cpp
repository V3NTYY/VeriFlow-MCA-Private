#include "Controller.h"
#include <thread>

// MAIN THREADS
void Controller::controllerThread(bool* run)
{
	std::cout << "Start thread...\n";
	while (*run) {
		std::vector<Flow> flows;

		std::cout << "waiting for msg...\n";
		// Receive next message from our socket
		recvControllerMessages(false);

		// Convert our buffer to openflow message format -- first convert to bytes
		std::vector<uint8_t> packet(ofBuffer, ofBuffer + sizeof(ofBuffer));
		OpenFlowMessage msg = OpenFlowMessage::fromBytes(packet);
		std::cout << msg.toString() << std::endl;

		// If we detect a flow modification, flow removed, or multipart reply, we need to parse the flows
		if (msg.type == OFPT_FLOW_MOD || msg.type == OFPT_MULTIPART_REPLY || msg.type == OFPT_FLOW_REMOVED || msg.type  == OFPT_STATS_REPLY) {
			std::cout << "Flow modification detected, parsing...\n";
			// Cast the message buffer to a stats_reply
			ofp_stats_reply* stats_reply = reinterpret_cast<ofp_stats_reply*>(ofBuffer);

			// We only care if we have flow information in this
			if (stats_reply->type != OFPST_FLOW) {
				std::cout << "[CCPDN-ERROR]: Not a flow stats reply, cancelling read." << std::endl;
				std::cout << stats_reply->type << std::endl;
				continue;
			}

			// Calculate body size to help find num of variable objects
			size_t body_size = packet.size() - sizeof(ofp_stats_reply);
			// Count the number of flow_stats_reply objects in the body, for each one, parse the flow
			int count = body_size / sizeof(ofp_flow_stats);
			// Create pointer for traversal while parsing
			uint8_t* body_ptr = stats_reply->body;

			while (count > 0) {
				// Cast the body to a flow_stats_reply object
				ofp_flow_stats* flow_stats_reply = reinterpret_cast<ofp_flow_stats*>(body_ptr);

				// Ensure we have enough data
				if (body_size < sizeof(ofp_flow_stats)) {
					std::cout << "[CCPDN-ERROR]: In-complete flow, cancelling read." << std::endl;
					break;
				}

				// Parse the flow stats reply into a flow object
				flows.push_back(OpenFlowMessage::parseStatsReply(*flow_stats_reply));

				// Adjust traversal variables
				count--;
				body_size -= sizeof(ofp_flow_stats);
				body_ptr += sizeof(ofp_flow_stats);
			}
		}

		// Iterate through each flow, based on results, determine if we need to run verification
		for (Flow f : flows) {
			f.print();
		}
		
		// Update our shared list of flows
		sharedFlows = flows;

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

		rstControllerFlag();
	}
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
	recvVeriFlowMessages(false);
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
		openFlowHandshake();
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
	uint32_t xid = static_cast<uint32_t>(std::rand());
    OpenFlowMessage msg(OFPT_FLOW_MOD, OFP_10, xid, f.flowToStr());
    
    // Send the OpenFlow message to the controller
    if (!sendOpenFlowMessage(msg)) {
        std::cout << "[CCPDN]: Failed to send flow add message to controller" << std::endl;
        return false;
    }
    
    // Wait for confirmation from the controller
    recvControllerMessages(false);
    rstControllerFlag();
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
            
            uint32_t xid = static_cast<uint32_t>(std::rand());            
            OpenFlowMessage msg(OFPT_FLOW_MOD, OFP_10, xid, f.flowToStr());
            
            // Send the removal message
            if (!sendOpenFlowMessage(msg)) {
                std::cout << "[CCPDN]: Failed to send flow removal message to controller" << std::endl;
                return false;
            }
            
            // Wait for confirmation from the controller
            recvControllerMessages(false);
            rstControllerFlag();
            
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

bool Controller::sendOpenFlowMessage(OpenFlowMessage msg)
{
	std::vector<uint8_t> Msg = msg.toBytes();
	#ifdef __unix__
		// Recast message as char array and send it
		ssize_t bytes_sent = send(sockfd, Msg.data(), Msg.size(), 0);
		if (bytes_sent != Msg.size()) {
			std::cerr << "[CCPDN-ERROR]: Failed to send OpenFlow Message" << std::endl;
			return false;
		}
	#endif

	std::cout << "--- [CCPDN-MESSAGE-POX] ---\n";
	for (int i = 0; i < Msg.size(); ++i) {
		std::cout << std::hex << static_cast<int>(Msg[i]) << " ";
	}
	std::cout << std::dec << std::endl << std::endl;

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

	std::cout << "--- [CCPDN-REQUESTFLOWS-POX] ---\n\n\n";

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

	std::cout << "--- [CCPDN-SENDFEATURES-POX] ---\n\n\n";

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

// Print the controller information
void Controller::print()
{
	// TODO: Add more POX controller statistics to print
	std::cout << "[CCPDN] POX Controller -> " << controllerIP << ":" << controllerPort << std::endl;
}
void Controller::openFlowHandshake()
{
	sendOpenFlowMessage(OpenFlowMessage::helloMessage());
	sendOpenFlowMessage(OpenFlowMessage::createFeaturesReply());
}

void Controller::veriFlowHandshake()
{
	sendVeriFlowMessage("[CCPDN] Hello");
	recvVeriFlowMessages(false);
	rstVeriFlowFlag();
}

void Controller::recvControllerMessages(bool thread)
{
	while (thread) {
#ifdef __unix__
		// Receive a message from the socket -- only do so when our ofFlag is inactive, meaning we're NOT using the buffer
		if (!ofFlag) {
			std::memset(ofBuffer, 0, sizeof(ofBuffer)); // Clear the buffer before receiving data
			ssize_t bytes_received = recv(sockfd, ofBuffer, sizeof(ofBuffer), 0);
			std::cout << "--- [POX-MESSAGE-CCPDN] ---\n";
			for (int i = 0; i < bytes_received; ++i) {
				std::cout << std::hex << static_cast<int>(ofBuffer[i]) << " ";
			}
			std::cout << std::dec << std::endl << std::endl;
			ofFlag = true;
		}
#endif
	}

	// Receive a single message
	while (!thread) {
#ifdef __unix__
		// Receive a message from the socket -- only do so when our ofFlag is inactive, meaning we're NOT using the buffer
		if (!ofFlag) {
			std::memset(ofBuffer, 0, sizeof(ofBuffer)); // Clear the buffer before receiving data
			ssize_t bytes_received = recv(sockfd, ofBuffer, sizeof(ofBuffer), 0);
			std::cout << "--- [POX-MESSAGE-CCPDN] ---\n";
			for (int i = 0; i < bytes_received; ++i) {
				std::cout << std::hex << static_cast<int>(ofBuffer[i]) << " ";
			}
			std::cout << std::dec << std::endl << std::endl;
			ofFlag = true;
			break;
		}
#endif
	}
}

void Controller::parseOpenFlowPacket(const std::vector<uint8_t>& packet)
{
}

void Controller::recvVeriFlowMessages(bool thread)
{
	while (thread) {
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

	// Receive a single message
	while (!thread) {
#ifdef __unix__
		// Receive a message from the socket -- only do so when our vfFlag is inactive, meaning we're NOT using the buffer
		if (!vfFlag) {
			std::memset(vfBuffer, 0, sizeof(vfBuffer)); // Clear the buffer before receiving data
			ssize_t bytes_received = recv(sockvf, vfBuffer, sizeof(vfBuffer), 0);
			std::cout << "--- [VERIFLOW-MESSAGE-CCPDN] --- \n";
			std::cout << readBuffer(vfBuffer) << std::endl << std::endl;
			vfFlag = true;
			break;
		}
#endif
	}
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


