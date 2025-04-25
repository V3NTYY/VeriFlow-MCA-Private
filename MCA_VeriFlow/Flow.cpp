#include "Flow.h"

std::string Flow::flowToStr(bool printDPID)
{
	// #switchIP-rulePrefix-nextHopIP
	// or #switchDPID-rulePrefix-outputPort
	std::string output;
	std::string actionStr = action ? "A" : "R";
	std::string outputSrcIP = printDPID ? switchDPID : switchIP;
	std::string outputHopIP = printDPID ? outPort : nextHopIP;
	std::string actionPrefix = printDPID ? "" : (actionStr + "#");
	output = actionPrefix + outputSrcIP + "-" + rulePrefix + "-" + outputHopIP;
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
	else if (payload[0] != 'A' && payload[0] != 'R') {
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
	isFlowMod = false;
	switchDPID = "";
	outPort = "";
	Modification = false;
}

Flow::Flow()
{
	switchIP = "";
	rulePrefix = "";
	nextHopIP = "";
	switchDPID = "";
	outPort = "";
	action = false;
	isFlowMod = false;
	Modification = false;
}

Flow::~Flow()
{
}

bool Flow::actionType()
{
	return action;
}

bool Flow::isMod()
{
    return isFlowMod;
}

void Flow::setMod(bool mod)
{
	isFlowMod = mod;
}

void Flow::print()
{
	loggyMsg("Switch IP: ");
	loggyMsg(switchIP);
	loggyMsg("\n");

	loggyMsg("Rule Prefix: ");
	loggyMsg(rulePrefix);
	loggyMsg("\n");

	loggyMsg("Next Hop IP: ");
	loggyMsg(nextHopIP);
	loggyMsg("\n");

	loggyMsg("Action: ");
	loggyMsg((action ? "Add" : "Remove"));
	loggyMsg("\n");
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
