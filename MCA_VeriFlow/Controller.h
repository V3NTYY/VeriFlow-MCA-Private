#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "Flow.h"
#include "OpenFlowMessage.h"
#include "Node.h"
#include "Digest.h"
#include "Topology.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>

#ifdef __unix__
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <unistd.h>
#endif

class Controller {
	public:
		// Constructors and destructors
		Controller(Topology* t);
		~Controller();

		// Setters
		void setControllerIP(std::string Controller_IP, std::string Controller_Port);
		void setVeriFlowIP(std::string VeriFlow_IP, std::string VeriFlow_Port);

		// Controller setup/freeing functions
		bool startController();
		bool start();
		bool freeLink();

		// Thread loop functions
		void thread();

		// Command functions (for controller)
		bool sendOpenFlowMessage(OpenFlowMessage Message);
		bool sendVeriFlowMessage(std::string message);
		bool addFlowToTable(Flow f);
		bool removeFlowFromTable(Flow f);
		bool synchTopology(Digest d);
		bool sendUpdate(bool global, int destinationIndex);

		// Verification functions
		bool requestVerification(int destinationIndex, Flow f);
		bool performVerification(bool externalRequest, Flow f);

		// Misc functions
		bool addDomainNode(Node* n);
		std::vector<Node*> getDomainNodes();

		// Debugging functions
		void print();
	private:
		int						  sockfd;
		int						  sockvf;
		std::string				  controllerIP;
		std::string				  controllerPort;
		std::string				  veriflowIP;
		std::string				  veriflowPort;
		std::vector<Node*>		  domainNodes;
		bool					  activeThread;
		Topology*				  referenceTopology;
		char					  vfBuffer[1024];
		char					  ofBuffer[1024];

		// Private Functions
		bool linkVeriFlow();
		bool linkController();
		void openFlowHandshake();
		void veriFlowHandshake();
		void recvControllerMessages(bool thread);
		void parseOpenFlowPacket(const std::vector<uint8_t>& packet);
		void recvVeriFlowMessages(bool thread);
		std::string readBuffer(char* buf);
};

#endif