#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "Flow.h"
#include "OpenFlowMessage.h"
#include "Node.h"
#include "Digest.h"
#include "Log.h"
#include "Topology.h"
#include "TCPAnalyzer.h"
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
#include <utility>
#include <unordered_map>

#ifdef __unix__
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <unistd.h>
#endif

class Controller {
	public:
		static bool pauseOutput;
		static std::mutex sharedFlowsMutex;

		// Constructors and destructors
		Controller();
		Controller(Topology* t);
		~Controller();

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
		void CCPDNServerThread(int port, bool* run);
		void CCPDNThread(bool* run);

		// CCPDN communication funcs
		bool initCCPDN();
		bool stopCCPDNServer();
		void closeAcceptedSocket(int socket);

		// Reading + Parsing functions
		bool parsePacket(std::vector<uint8_t>& packet, bool xidCheck);
		std::vector<uint8_t> recvControllerMessages();
		void recvVeriFlowMessages();
		void recvProcessCCPDN(int socket);
		void parseFlow(Flow f);

		// OpenFlow packet decode functions
		void handleStatsReply(ofp_stats_reply* reply);
		void handleFlowMod(ofp_flow_mod* mod);
		void handleFlowRemoved(ofp_flow_removed* removed);

		// Send msg functions
		bool sendOpenFlowMessage(std::vector<unsigned char> data);
		bool sendVeriFlowMessage(std::string message);
		bool sendFlowHandlerMessage(std::string message);
		bool sendCCPDNMessage(int socket, std::string message);

		// Update functions
		bool synchTopology(Digest d);
		bool sendUpdate(bool global, int destinationIndex);

		// Flow functions
		bool addFlowToTable(Flow f);
		bool removeFlowFromTable(Flow f);
		std::vector<Flow> retrieveFlows(std::string IP, bool pause);
		Flow adjustCrossTopFlow(Flow f);

		// XID Mapping functions
		bool updateXIDMapping(uint32_t xid, std::string srcIP, std::string dstIP);
		std::string getSrcFromXID(uint32_t xid);
		std::string getDstFromXID(uint32_t xid);
		int generateXID(int topologyIndex);

		// Verification functions
		bool requestVerification(int destinationIndex, Flow f);
		bool performVerification(bool externalRequest, Flow f);
		bool undoVerification(Flow f, int topologyIndex);
		bool modifyFlowTableWithoutVerification(Flow f, bool success);

		// All funcs related to external verification
		bool remapVerify(Flow newFlow);
		std::vector<std::string> getLinkPathToNode(std::string srcIP, std::string dstIP);
		std::vector<Flow> getRelatedFlows(std::string IP); //Unused
		std::vector<Flow> filterFlows(std::vector<Flow> flows, std::string domainNodeIP, int topologyIndex); //Unused
		std::vector<std::vector<Flow>> translateFlows(std::vector<Flow> flows, std::string originalIP, std::string newIP);
		Node getBestDomainNode(int firstIndex, int secondIndex);

		// Get link/interface funcs
		int getNumLinks(std::string IP, bool Switch);
		std::vector<std::string> getInterfaces(std::string IP);
		void addPortToMap(std::string srcIP, std::string dstIP, int port);
		int getPortFromMap(std::string srcIP, std::string dstIP);
		void addIPToMap(std::string srcIP, int port, std::string dstIP);
		std::string getIPFromMap(std::string srcIP, int port);

		// Misc functions
		bool 			   addDomainNode(Node* n);
		std::vector<Node*> getDomainNodes();
		void 			   rstControllerFlag();
		void 			   rstVeriFlowFlag();
		int	 			   getDPID(std::string IP);
		int  			   getOutputPort(std::string srcIP, std::string dstIP);
		std::string		   getIPFromOutputPort(std::string srcIP, int outputPort);
		void			   tryClearSharedFlows();
		void               testVerificationTime(int numFlows, bool interTopology);
		void			   closeSockets();
		void			   mapSocketToIndex(int* socket, int index);
		int*			   getSocketFromIndex(int index);
		bool			   validateFlow(Flow f);

		// Map every XID to a flow, specifically the source and destination IPs
		std::unordered_map<uint32_t, std::pair<std::string, std::string>> xidFlowMap; 
		// Map each connection (socket) to the corresponding topology index
		std::unordered_map<int, int*> socketTopologyMap;
		// Map each (srcSwitch, dstSwitch) -> outputPort pair
		std::unordered_map<std::string, int> portMap;
		std::unordered_map<std::string, std::string> portMapReverse;

		std::string				  controllerPort;
		std::string				  veriflowPort;
		std::string				  flowPort;
		std::vector<Flow>		  sharedFlows;
		std::vector<uint8_t>	  sharedPacket;
		bool					  fhFlag;
		int						  fhXID;
		int						  expFlowXID;
		bool					  gotFlowMod;
		bool					  recvSharedFlag;
		int						  basePort;
		std::vector<Flow>		  CCPDN_FLOW_RESPONSE;
		std::vector<Flow>		  CCPDN_FLOW_FAIL;
		std::vector<Flow>		  CCPDN_FLOW_SUCCESS;
		bool					  ALLOW_CCPDN_RECV;
		std::vector<Flow>		  ignoreFlows;

	private:
		int						  sockfd;
		int						  sockvf;
		int						  sockfh;
		int						  sockCC;
		std::vector<int>		  acceptedCC;
		std::string				  controllerIP;
		std::string				  veriflowIP;
		std::string				  flowIP;
		std::vector<Node*>		  domainNodes;
		Topology*				  referenceTopology;
		char					  vfBuffer[1024];
		bool					  vfFlag;
		bool					  ofFlag;
		bool					  pause_rst;
		bool					  noRst;
		bool					  forceStopShared;

		// Private Functions
		bool linkVeriFlow();
		bool linkController();
		bool linkFlow();
		void veriFlowHandshake();
		std::string readBuffer(char* buf);
};

#endif