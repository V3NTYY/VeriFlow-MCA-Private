#include "Controller.h"

// Constructor
Controller::Controller(std::string Controller_IP, std::string Controller_Port)
{
	controllerIP = Controller_IP;
	controllerPort = Controller_Port;
}

// Destructor
Controller::~Controller()
{
	// TODO: Implement the destructor
}

bool Controller::linkController(std::string Controller_IP, std::string Controller_port) {
	// TODO: Connect to the running POX controller
	return false;
}

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

void Controller::parseDigest(std::string digest)
{
	// TODO: Break down JSON format digest, and issue the appropriate commands

	// Example: Digest contained the DN bit, so we will link it as a domain node
	//	DomainNode d("parseIP", "parsePort");
	//	linkDomainNode(d);
}

// Print the controller information
void Controller::print()
{
	// TODO: Add more POX controller statistics to print
	std::cout << "CONTROLLER --> IP:" << controllerIP << ":" << controllerPort << std::endl;
}	