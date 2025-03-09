#include "DomainNode.h"

DomainNode::DomainNode(std::string Node_IP, std::string Node_Port)
{
	// Initialize the domain node
}

std::string DomainNode::getIP()
{
	return nodeIP;
}

std::string DomainNode::getPort()
{
	return nodePort;
}

void DomainNode::print()
{
	// Print the domain node
	std::cout << "DOMAIN NODE --> IP: " << nodeIP << ":" << nodePort << std::endl;	
}