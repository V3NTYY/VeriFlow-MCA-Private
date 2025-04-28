#ifndef MCA_VERIFLOW_H
#define MCA_VERIFLOW_H

#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <chrono>
#include <thread>
#include <numeric>
#include <cstring>
#include <algorithm>
#include "Topology.h"
#include "Controller.h"
#include "Log.h"

#ifdef __unix__
	#include <sys/socket.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <unistd.h>
#endif

class MCA_VeriFlow {
	public:
		MCA_VeriFlow();
		~MCA_VeriFlow();

		void run();
		void stop();
		bool registerTopologyFile(std::string filepath);
		bool createDomainNodes();
		Topology partitionTopology();
		bool verifyTopology();
		bool pingTest(Node n);
		void printPorts(int VeriFlowPort);
		void printStatus();

		void tcpTestThread(std::string IP, int port, int amount);
		std::vector<double> measure_tcp_connection(const std::string& host, int port, int num_pings);
		double test_tcp_connection_time(const std::string& host, int port, int timeout);

		Topology topology;
		Controller controller;
		bool controller_running;
		bool runService;
		bool controller_linked;
		bool flowhandler_linked;
		bool topology_initialized;

		bool runningTCPTest;

	private:
};

#endif