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
#include <cstddef>

#ifdef __unix__
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <unistd.h>
#endif

class Controller {
	public:
		// Constructors and destructors
		Controller();
		Controller(Topology* t);
		~Controller();

		// Setters
		void setControllerIP(std::string Controller_IP, std::string Controller_Port);
		void setVeriFlowIP(std::string VeriFlow_IP, std::string VeriFlow_Port);

		// Controller setup/freeing functions
		bool startController(bool* thread);
		bool start();
		bool freeLink();

		// Thread loop functions
		void controllerThread(bool* run);

		// Reading + Parsing functions
		bool parsePacket(std::vector<uint8_t>& packet);
		std::vector<uint8_t> recvControllerMessages();
		void recvVeriFlowMessages();

		// OpenFlow packet decode functions
		void handleStatsReply(ofp_stats_reply* reply);
		void handleHeader(ofp_header* header);
		void handleFlowMod(ofp_flow_mod* mod);
		void handleFlowRemoved(ofp_flow_removed* removed);

		// Command functions (for controller)
		bool sendOpenFlowMessage(ofp_header Header);
		bool sendOpenFlowMessage(ofp_stats_request Request);
		bool sendOpenFlowMessage(ofp_switch_features Features);
		bool sendVeriFlowMessage(std::string message);
		bool addFlowToTable(Flow f);
		bool removeFlowFromTable(Flow f);
		std::vector<Flow> retrieveFlows(std::string IP);
		bool synchTopology(Digest d);
		bool sendUpdate(bool global, int destinationIndex);

		// Verification functions
		bool requestVerification(int destinationIndex, Flow f);
		bool performVerification(bool externalRequest, Flow f);

		// Misc functions
		bool addDomainNode(Node* n);
		std::vector<Node*> getDomainNodes();
		void rstControllerFlag();
		void rstVeriFlowFlag();
		Flow adjustCrossTopFlow(Flow f);

	private:
		int						  sockfd;
		int						  sockvf;
		std::string				  controllerIP;
		std::string				  controllerPort;
		std::string				  veriflowIP;
		std::string				  veriflowPort;
		std::vector<Node*>		  domainNodes;
		std::vector<Flow>		  sharedFlows;
		bool					  activeThread;
		Topology*				  referenceTopology;
		char					  vfBuffer[1024];
		bool					  vfFlag;
		bool					  ofFlag;

		// Private Functions
		bool linkVeriFlow();
		bool linkController();
		void veriFlowHandshake();
		std::string readBuffer(char* buf);
};

#endif