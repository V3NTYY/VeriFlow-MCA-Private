// Class file for Controller

#include "Controller.h"

// Constructor
Controller::Controller(std::string IP, std::string Port)
{
	controllerIP = IP;
	controllerPort = Port;
	link = NULL;
}

// Destructor
Controller::~Controller()
{
	// TODO: Implement the destructor
}

// Link a domain node to the controller
bool Controller::linkDomainNode(std::string domainName, std::string domainIP)
{
	// TODO: Implement logic for linking the domain node
	return false;
}

// Free the link to the domain node
bool Controller::freeLink()
{
	// TODO: Free the link to the domain node
	return false;
}

// Print the controller information
void Controller::print()
{
	std::cout << "CONTROLLER --> IP:" << controllerIP << ":" << controllerPort << std::endl;
	// TODO: Print the linked domain node
}	