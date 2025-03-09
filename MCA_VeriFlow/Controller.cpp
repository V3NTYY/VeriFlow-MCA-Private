#include "Controller.h"

// Constructor
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

#ifdef __unix__
bool Controller::linkController() {

	// Setup socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		std::cout << "Error creating socket" << std::endl;
		return false;
	}

	// Setup the address to connect to


	return true;
}
#endif

// Free the link to the domain node
bool Controller::freeLink()
{
	// TODO: Free the currently linked controller
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
}

void Controller::sendHello()
{
}

void Controller::receiveHello()
{
}

void Controller::listenerOpenFlow()
{
}

void Controller::parseOpenFlowPacket(const std::vector<uint8_t>& packet)
{
}
