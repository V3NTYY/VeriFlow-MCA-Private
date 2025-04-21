#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "Flow.h"
#include "OpenFlowMessage.h"
#include "Node.h"
#include "Digest.h"
#include "Log.h"
#include "Topology.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstddef>
#include <stdexcept>
#include <memory>
#include <cstdio>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

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
		Controller& operator=(const Controller&) = delete; // Disable assignment operator

		// Setters
		void setControllerIP(std::string Controller_IP, std::string Controller_Port);
		void setVeriFlowIP(std::string VeriFlow_IP, std::string VeriFlow_Port);
		void setFlowHandlerIP(std::string fh_IP, std::string fh_Port);

		// Controller setup/freeing functions
		bool startController(bool* thread);
		bool startFlow(bool* thread);
		bool start();
		bool freeLink();

		// Thread loop functions
		void controllerThread(bool* run);
		void flowHandlerThread(bool* run);
		void enqueuePacket(const std::vector<uint8_t>& packet);

		// Reading + Parsing functions
		bool parsePacket(std::vector<uint8_t>& packet);
		std::vector<uint8_t> recvControllerMessages();
		void recvVeriFlowMessages();
		void parseFlow(Flow f);

		// OpenFlow packet decode functions
		void handleStatsReply(ofp_stats_reply* reply);
		void handleFlowMod(ofp_flow_mod* mod);
		void handleFlowRemoved(ofp_flow_removed* removed);

		// Send msg functions (for controller)
		bool sendOpenFlowMessage(std::vector<unsigned char> data);
		bool sendVeriFlowMessage(std::string message);
		bool sendFlowHandlerMessage(std::string message);

		// Update functions
		bool synchTopology(Digest d);
		bool sendUpdate(bool global, int destinationIndex);

		// Flow functions
		bool addFlowToTable(Flow f);
		bool removeFlowFromTable(Flow f);
		std::vector<Flow> retrieveFlows(std::string IP);
		Flow adjustCrossTopFlow(Flow f);

		// Verification functions
		bool requestVerification(int destinationIndex, Flow f);
		bool performVerification(bool externalRequest, Flow f);

		// Misc functions
		bool addDomainNode(Node* n);
		std::vector<Node*> getDomainNodes();
		void rstControllerFlag();
		void rstVeriFlowFlag();
		int	 getDPID(std::string IP);
		int  getOutputPort(std::string srcIP, std::string dstIP);

		bool					  			  linking;
		std::string				  			  controllerPort;
		std::string				  			  veriflowPort;
		std::string				  			  flowPort;
		std::vector<Flow>		  			  sharedFlows;
		std::vector<std::vector<uint8_t>>	  sharedPackets;
		bool					  			  fhFlag;

	private:
		int						  sockfd;
		int						  sockvf;
		int						  sockfh;
		std::string				  controllerIP;
		std::string				  veriflowIP;
		std::string				  flowIP;
		std::vector<Node*>		  domainNodes;
		Topology*				  referenceTopology;
		char					  vfBuffer[1024];
		bool					  vfFlag;
		bool					  ofFlag;
		bool					  pause_rst;
		std::mutex				  packetMutex;
		std::condition_variable	  packetCondition;

		// Private Functions
		bool linkVeriFlow();
		bool linkController();
		bool linkFlow();
		void veriFlowHandshake();
		std::string readBuffer(char* buf);
};

#endif