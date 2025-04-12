#ifndef FLOW_H
#define FLOW_H

#include <string>
#include <fstream>
#include <iostream>

class Flow {
	public:
		// Constructors & destructors
		Flow(std::string SwitchIP, std::string RulePrefix, std::string NextHopIP);
		~Flow();

		// Marshalling methods
		std::string flowToStr();
		static Flow* strToFlow(std::string payload);

		// Misc methods
		void print();

	private:
		std::string switchIP;
		std::string rulePrefix;
		std::string nextHopIP;
};

#endif