#include "MCA_VeriFlow.h"
#include <iostream>
#include <string>

class DomainNode
{
	public:
		// Constructors and destructors
		DomainNode(std::string Node_IP, std::string Node_Port);
		~DomainNode();

		// Functions
		std::string getIP();
		std::string getPort();

		// Debugging functions
		void print();
	private:
		std::string nodeIP;
		std::string nodePort;
};