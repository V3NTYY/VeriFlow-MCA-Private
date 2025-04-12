#include "Flow.h"

std::string Flow::flowToStr()
{
	// #switchIP-rulePrefix-nextHopIP
	std::string output;
	std::string actionStr = action ? "A" : "R";
	output = actionStr + "#" + switchIP + "-" + rulePrefix + "-" + nextHopIP;
	return output;
}

Flow* Flow::strToFlow(std::string payload)
{
	// #switchIP-rulePrefix-nextHopIP
	Flow* f = new Flow("", "", "", false);
	
	// Ensure correct size and format
	if (payload.size() < 3) {
		return nullptr;
	} else if (payload[1] != '#') {
		return nullptr;
	}
	else if (payload[0] != 'A' || payload[0] == 'B') {
		return nullptr;
	}

	if (payload[0] == 'A') {
		f->action = true;
	}
	else {
		f->action = false;
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

Flow::Flow(std::string SwitchIP, std::string RulePrefix, std::string NextHopIP, bool Action)
{
	switchIP = SwitchIP;
	rulePrefix = RulePrefix;
	nextHopIP = NextHopIP;
	action = Action;
}

Flow::Flow()
{
	switchIP = "";
	rulePrefix = "";
	nextHopIP = "";
	action = false;
}

Flow::~Flow()
{
}

bool Flow::actionType()
{
	return action;
}

void Flow::print()
{
	std::cout << "Switch IP: " << switchIP << std::endl;
	std::cout << "Rule Prefix: " << rulePrefix << std::endl;
	std::cout << "Next Hop IP: " << nextHopIP << std::endl;
	std::cout << "Action: " << (action ? "Add" : "Remove") << std::endl;
}

std::string Flow::getSwitchIP()
{
	return switchIP;
}

std::string Flow::getRulePrefix()
{
	return rulePrefix;
}

std::string Flow::getNextHopIP()
{
	return nextHopIP;
}
