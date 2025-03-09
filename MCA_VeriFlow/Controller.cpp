#include "Controller.h"

// Constructor
Controller::Controller() {
	controllerIP = "";
	controllerPort = "";
	sockfd = -1;
}

Controller::Controller(std::string Controller_IP, std::string Controller_Port)
{
	controllerIP = Controller_IP;
	controllerPort = Controller_Port;
	sockfd = -1;
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

bool Controller::start()
{
	if (linkController()) {
		openFlowHandshake();
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

bool Controller::linkDomainNode(DomainNode d)
{
	domainNodes.push_back(d);
	return false;
}

bool Controller::parseDigest(std::string digest)
{
	// TODO: Break down JSON format digest, and issue the appropriate commands.
	// Only returns true if a command was issued as result of digest

	// Example: Digest contained the DN bit, so we will link it as a domain node
	//	DomainNode d("parseIP", "parsePort");
	//	linkDomainNode(d);

	// Example 2: Digest containe the Synch bit, so we will issue a synchronization command 
	//	synchronize();

	return false;
}

bool Controller::synchronize()
{
	// TODO: Issue command to POX controller to ask all neighboring controllers for topology data
	// May require direct connection to controller. We can change the method as we need
	return false;
}

// Print the controller information
void Controller::print()
{
	// TODO: Add more POX controller statistics to print
	std::cout << "CONTROLLER --> IP:" << controllerIP << ":" << controllerPort << std::endl;
}
void Controller::openFlowHandshake()
{
	sendHello();
	receiveHello();
}

void Controller::sendHello()
{
	// OpenFlow message format. Length is split into two bytes. XID, or transaction ID is split into 4 bytes.
	char msg[8];
	msg[0] = 0x01; // Version 1.0
	msg[1] = 0x00; // Type (HELLO = 0x00)
	msg[2] = 0x00; // Length (most significant byte in length value)
	msg[3] = 0x08; // Length (least significant byte in length value)
	msg[4] = 0x00; // XID (Most significant byte in XID)
	msg[5] = 0x00; // XID (2nd most significant byte in XID)
	msg[6] = 0x00; // XID (2nd least significant byte in XID)
	msg[7] = 0x01; // XID (Least significant byte in XID)

	#ifdef __unix__
		// Recast message as char array and send it
		ssize_t bytes_sent = send(sockfd, msg, sizeof(msg), 0);
	#endif

	std::cout << "[CCPDN-REQUEST-POX]: " << msg << std::endl;
	std::cout << "Message content: ";
	for (int i = 0; i < sizeof(msg); ++i) {
		std::cout << std::hex << static_cast<int>(msg[i]) << " ";
	}
	std::cout << std::dec << std::endl;
}

void Controller::receiveHello()
{
	char buf[1024];
	#ifdef __unix__
		ssize_t bytes_received = recv(sockfd, buf, sizeof(buf), 0);
		std::cout << "[POX-RESPONSE-CCPDN]: Length:" << bytes_received << "bytes." << std::endl;
		std::cout << "Response:";
		for (int i = 0; i < bytes_received; ++i) {
			std::cout << std::hex << static_cast<int>(buf[i]) << " ";
		}
		std::cout << std::dec << std::endl;
	#endif
}

void Controller::listenerOpenFlow()
{
}

void Controller::parseOpenFlowPacket(const std::vector<uint8_t>& packet)
{
}
