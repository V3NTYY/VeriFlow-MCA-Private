#include "MCA_VeriFlow.h"
#include "DomainNode.h"
#include "Flow.h"
#include <iostream>
#include <vector>
#include <string>

class Controller {
	public:
		// Constructors and destructors
		Controller(std::string Controller_IP, std::string Controller_Port);
		~Controller();

		// Functions
		bool linkController(std::string Controller_IP, std::string Controller_Port);
		bool freeLink();
		bool parseDigest(std::string digest);

		// Command functions (for controller)
		bool addFlowToTable(Flow f);
		bool removeFlowFromTable(Flow f);
		bool linkDomainNode(DomainNode d);
		bool synchronize();

		// Debugging functions
		void print();
	private:
		std::string				controllerIP;
		std::string				controllerPort;
		std::vector<DomainNode> domainNodes;
};