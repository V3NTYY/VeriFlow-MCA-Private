// Header file for DomainNode
#include "MCA_VeriFlow.h"
#include <iostream>
#include <string>

class DomainNode
{
	public:
		// Constructors and destructors
		DomainNode(std::string IP, std::string Port);
		~DomainNode();

		// Functions
		std::string getIP();
		std::string getPort();

		// Debugging functions
		void print();
	private:
		std::string dNodeIP;
		std::string dNodePort;
};