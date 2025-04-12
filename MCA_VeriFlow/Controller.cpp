#include "Controller.h"

// MAIN THREAD LOOP
void Controller::thread()
{

}

bool Controller::requestVerification(int destinationIndex, Flow f)
{
	/// WARNING: Only call this function for cross-domain verification
	/// Additionally, make any changes you need to the flow rule as well (i.e. for resubmits)

	// Verify destination index exists within current topology
	if (destinationIndex < 0 || destinationIndex >= referenceTopology->getTopologyCount()) {
		return false;
	}

	Digest verificationMessage(false, false, true, referenceTopology->hostIndex, destinationIndex, "");
	verificationMessage.appendFlow(f);

	return verificationMessage.sendDigest(this);
}

bool Controller::performVerification(bool externalRequest, Flow f)
{
	/// Craft the packet
	std::string packet = "[CCPDN] FLOW ";
	packet += f.flowToStr();

	sendVeriFlowMessage(packet);
	recvVeriFlowMessages(false);

	// Ensure flow rule falls within our host topology
	Node host = referenceTopology->getNodeByIP(f.getSwitchIP(), referenceTopology->hostIndex);
	Node target = referenceTopology->getNodeByIP(f.getSwitchIP());
	if (host.isEmptyNode()) {
		// We have inter-topology verification now. use reqVerification
		int targetTopology = target.getTopologyID();
		std::cout << "[CCPDN]: Requesting verification from topology " << targetTopology << "..." << std::endl;
		Digest verificationMessage(false, false, true, referenceTopology->hostIndex, targetTopology, "");
	}
	else { } // Normal verification logic goes here

	// This just means that we should forward our results to the previous CCPDN instance
	if (externalRequest) {

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
	vfFlag = false;
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
	vfFlag = false;
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

bool Controller::startController()
{
	// Used for linking and confirming controller. Does not start the CCPDN service
	if (linkController()) {
		openFlowHandshake();
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
	// TODO: Invoke Openflow method to add flow to table
	return false;
}

bool Controller::removeFlowFromTable(Flow f)
{
	// TODO: Invoke Openflow method to match flow from table and remove it
	return false;
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
	#endif

	std::cout << "--- [CCPDN-MESSAGE-POX] ---\n";
	for (int i = 0; i < Msg.size(); ++i) {
		std::cout << std::hex << static_cast<int>(Msg[i]) << " ";
	}
	std::cout << std::dec << std::endl << std::endl;

	return false;
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

// Print the controller information
void Controller::print()
{
	// TODO: Add more POX controller statistics to print
	std::cout << "CONTROLLER --> IP:" << controllerIP << ":" << controllerPort << std::endl;
}
void Controller::openFlowHandshake()
{
	sendOpenFlowMessage(OpenFlowMessage::helloMessage());
	recvControllerMessages(false);
}

void Controller::veriFlowHandshake()
{
	sendVeriFlowMessage("[CCPDN] Hello");
	recvVeriFlowMessages(false);
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


