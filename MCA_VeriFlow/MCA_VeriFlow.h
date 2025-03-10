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
		std::vector<double> measure_tcp_connection(const std::string& host, int port, int num_pings);
		double test_tcp_connection_time(const std::string& host, int port, int timeout);
	private:
	
};

#endif