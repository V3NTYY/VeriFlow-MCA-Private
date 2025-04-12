#include "Flow.h"

std::string Flow::flowToStr()
{
	// #switchIP-rulePrefix-nextHopIP
	std::string output;
	output = "#" + switchIP + "-" + rulePrefix + "-" + nextHopIP;
	return output;
}

Flow* Flow::strToFlow(std::string payload)
{
	// #switchIP-rulePrefix-nextHopIP
	Flow* f = new Flow("", "", "");
	
	// Ensure correct size and format
	if (payload.size() < 3) {
		return nullptr;
	} else if (payload[0] != '#') {
		return nullptr;
	}

	// Parse the string
	std::string swIP = payload.substr(1, payload.find('-') - 1);
	std::string rulePFX = payload.substr(payload.find('-') + 1, payload.rfind('-') - payload.find('-') - 1);
	std::string nextHIP = payload.substr(payload.rfind('-') + 1, payload.size() - payload.rfind('-') - 1);

	// Ensure we parsed something
	if (swIP.empty() || rulePFX.empty() || nextHIP.empty()) {
		return nullptr;
	}

	// Set the flow properties
	f->switchIP = swIP;
	f->rulePrefix = rulePFX;
	f->nextHopIP = nextHIP;

	return f;
}

Flow::Flow(std::string SwitchIP, std::string RulePrefix, std::string NextHopIP)
{
	switchIP = SwitchIP;
	rulePrefix = RulePrefix;
	nextHopIP = NextHopIP;
}

Flow::~Flow()
{
}

void Flow::print()
{
	std::cout << "Switch IP: " << switchIP << std::endl;
	std::cout << "Rule Prefix: " << rulePrefix << std::endl;
	std::cout << "Next Hop IP: " << nextHopIP << std::endl;
}
