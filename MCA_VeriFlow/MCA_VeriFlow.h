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
#include "Topology.h"
#include "Controller.h"

#ifdef __unix__
	#include <sys/socket.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <unistd.h>
#endif

class MCA_VeriFlow {
	public:
		void run();
		void stop();
		bool registerTopologyFile(std::string filepath);
		bool createDomainNodes();
		Topology partitionTopology();
		bool verifyTopology();
		bool registerDomainNodes();
		bool pingTest(Node n);

		std::vector<double> measure_tcp_connection(const std::string& host, int port, int num_pings);
		double test_tcp_connection_time(const std::string& host, int port, int timeout);

		Topology topology;
		Controller controller;

	private:
};

#endif