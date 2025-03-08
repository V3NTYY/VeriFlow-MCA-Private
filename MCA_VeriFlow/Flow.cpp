#include "Flow.h"

std::string Flow::toJSON(bool create_file, std::string file_dir) {
	// TODO: Create a JSON object given this flows information (make sure it matches the sample JSON)
	// Output it to a .json file if create_file is true to the file_dir location

	return "";
}

static Flow* fromJSON(std::string json) {
	// TODO: Create a Flow object from parsing a given JSON object (make sure it matches the sample JSON)
	return nullptr;
}

Flow::Flow(std::string Flow_id, std::string Eth_type, std::string Protocol,
	std::string SrcIP, std::string DstIP, std::string SrcPort,
	std::string DstPort, std::string Action)
{
	flow_id =	Flow_id;
	eth_type =	Eth_type;
	protocol =	Protocol;
	srcIP =		SrcIP;
	dstIP =		DstIP;
	srcPort =	SrcPort;
	dstPort =	DstPort;
	action =	Action;
}

Flow::~Flow()
{
}

std::string Flow::getFlowID()
{
	return flow_id;
}

std::string Flow::getEthType()
{
	return eth_type;
}

std::string Flow::getProtocol()
{
	return protocol;
}

std::string Flow::getSrcIP()
{
	return srcIP;
}

std::string Flow::getDstIP()
{
	return dstIP;
}

std::string Flow::getSrcPort()
{
	return srcPort;
}

std::string Flow::getDstPort()
{
	return dstPort;
}

std::string Flow::getAction()
{
	return action;
}
