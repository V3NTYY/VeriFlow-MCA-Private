#ifndef FLOW_H
#define FLOW_H

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include "Log.h"

class Flow {
	public:
		// Constructors & destructors
		Flow(std::string SwitchIP, std::string RulePrefix, std::string NextHopIP, bool Action);
		Flow();
		~Flow();

		// Equality operator (for finding dupes)
		bool operator==(const Flow& other) const {
			return (switchIP == other.switchIP && rulePrefix == other.rulePrefix && nextHopIP == other.nextHopIP);
		}

		// Marshalling methods
		std::string flowToStr(bool printDPID);
		static Flow* strToFlow(std::string payload);
		static std::vector<std::string> splitFlowString(std::string flow);

		// Misc methods
		void print();

		// Getters
		std::string getSwitchIP();
		std::string getRulePrefix();
		std::string getNextHopIP();
		bool actionType();
		Flow inverseFlow();

		bool isMod();
		void setMod(bool mod);

		bool isFlowModify() { return Modification; }
		void setFlowModify(bool mod) { Modification = mod; }

		void setDPID(std::string switchDP, std::string hopDP) { switchDPID = switchDP; outPort = hopDP; }

	private:
		std::string switchDPID;
		std::string outPort;

		std::string switchIP;
		std::string rulePrefix;
		std::string nextHopIP;
		bool action;
		bool isFlowMod;
		bool Modification;
};

#endif