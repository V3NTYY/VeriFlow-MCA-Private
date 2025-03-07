#include "DomainNode.h"

// Constructor
DomainNode::DomainNode(std::string IP, std::string Port)
{
	// Initialize the domain node
}

// Destructor
DomainNode::~DomainNode()
{
	// Clean up the domain node
}

std::string DomainNode::getIP()
{
	return IP;
}

std::string DomainNode::getPort()
{
	return Port;
}

void DomainNode::print()
{
	// Print the domain node
	std::cout << "DOMAIN NODE --> IP: " << IP << ":" << Port << std::endl;	
}