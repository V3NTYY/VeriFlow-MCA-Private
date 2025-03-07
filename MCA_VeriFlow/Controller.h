#include "MCA_VeriFlow.h"
#include "DomainNode.h"
#include <iostream>
#include <string>

class Controller {
	public:
		// Constructors and destructors
		Controller(std::string IP, std::string Port);
		~Controller();

		// Functions
		bool linkDomainNode(std::string domainName, std::string domainIP);
		bool freeLink();

		// Debugging functions
		void print();
	private:
		std::string controllerIP;
		std::string controllerPort;
		DomainNode *link;

};