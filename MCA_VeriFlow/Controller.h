#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "MCA_VeriFlow.h"
#include "DomainNode.h"
#include "Flow.h"
#include "OpenFlowMessage.h"
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
		Controller();
		Controller(std::string Controller_IP, std::string Controller_Port);
		~Controller();

		// Setters
		void setControllerIP(std::string Controller_IP, std::string Controller_Port);

		// Functions
		bool start();
		bool freeLink();
		bool parseDigest(std::string digest);

		// Command functions (for controller)
		bool sendOpenFlowMessage(OpenFlowMessage Message);
		bool addFlowToTable(Flow f);
		bool removeFlowFromTable(Flow f);
		bool linkDomainNode(DomainNode d);
		bool synchronize();

		// Debugging functions
		void print();
	private:
		int						sockfd;
		std::string				controllerIP;
		std::string				controllerPort;
		std::vector<DomainNode> domainNodes;

		// Functions
		bool linkController();
		void openFlowHandshake();
		void recvControllerMessages(bool thread);
		void listenerOpenFlow();
		void parseOpenFlowPacket(const std::vector<uint8_t>& packet);
};

#endif