#ifndef FLOW_H
#define FLOW_H

#include <string>
#include <fstream>
#include <iostream>
#include "Log.h"

class Flow {
	public:
		// Constructors & destructors
		Flow(std::string SwitchIP, std::string RulePrefix, std::string NextHopIP, bool Action);
		Flow();
		~Flow();

		// Marshalling methods
		std::string flowToStr();
		static Flow* strToFlow(std::string payload);

		// Misc methods
		void print();

		// Getters
		std::string getSwitchIP();
		std::string getRulePrefix();
		std::string getNextHopIP();
		bool actionType();

	private:
		std::string switchIP;
		std::string rulePrefix;
		std::string nextHopIP;
		bool action;
};

#endif