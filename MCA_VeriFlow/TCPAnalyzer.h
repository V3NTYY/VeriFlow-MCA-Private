#ifndef TCPANALYZER_H
#define TCPANALYZER_H

#include <string>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>
#include "Log.h"
#include "OpenFlowMessage.h"
#include "Controller.h"
#include "Flow.h"

#ifdef __unix__
#include <pcap.h>
#endif

typedef uint8_t byte;
typedef std::vector<byte> packet;

class TCPAnalyzer {
	public:
		// Thread method
		void thread(bool *run, Controller *controller) {
			loggy << "\n\n[CCPDN]: Starting LibPCap thread...\n";

			con = controller;
			if (con == nullptr) {
				return;
			}

			startPacketCapture("lo", "tcp port " + controller->controllerPort, run);
		}

#ifdef __unix__
	static void packetHandler(u_char* userData, const struct pcap_pkthdr* pkthdr, const u_char* packet) {
		// Cast userData back to TCPAnalyzer instance
		TCPAnalyzer* analyzer = reinterpret_cast<TCPAnalyzer*>(userData);

		// Log packet details
		loggy << "Captured a packet with length: " << pkthdr->len << " bytes" << std::endl;

		// Example: Print the first 16 bytes of the packet
		for (int i = 0; i < pkthdr->len; i++) {
			loggy << std::hex << (int)packet[i] << " ";
		}
		loggy << std::endl;

		// Extract lengths of each header (ethernet, IP, TCP, etc.)
		int ETHERNET_HEADER_SIZE = 14;
		int IP_HEADER_SIZE = 20; // Not always, but most of the time
		int TCP_HEADER_SIZE = packet[ETHERNET_HEADER_SIZE + IP_HEADER_SIZE + 13] / 4; // value of 13th byte of tcp frame divided by 4
		int TOTAL_HEADER_SIZE = ETHERNET_HEADER_SIZE + IP_HEADER_SIZE + TCP_HEADER_SIZE;

		// Extract payload data
		std::vector<byte> payload(packet + TOTAL_HEADER_SIZE, packet + pkthdr->len);
		loggy << "Payload data: ";
		for (int i = 0; i < payload_size; i++) {
			loggy << std::hex << (int)payload[i] << " ";
		}
		loggy << std::endl;

		// Utilize parsing methods from controller, and update controller remotely
		analyzer->con->parsePacket(payload);

		// For now, print any flows received
		for (Flow f : analyzer->con->sharedFlows) {
			f.print();
			analyzer->con->parseFlow(f);
		}
	}
#endif

	void startPacketCapture(const std::string& interface, const std::string& filterExp, bool* run) {
#ifdef __unix
		char errbuf[PCAP_ERRBUF_SIZE];
		pcap_t* handle;

		// Open the specified interface for packet capture
		handle = pcap_open_live(interface.c_str(), BUFSIZ, 1, 1000, errbuf);
		if (handle == nullptr) {
			loggyErr("Error opening device: ");
			loggyErr(errbuf);
			loggyErr("\n");
			return;
		}

		// Compile and set the filter
		struct bpf_program filter;
		if (pcap_compile(handle, &filter, filterExp.c_str(), 0, PCAP_NETMASK_UNKNOWN) == -1) {
			loggy << "[CCPDN-ERROR]: Couldn't compile pcap filter: " << pcap_geterr(handle) << std::endl;
			return;
		}
		if (pcap_setfilter(handle, &filter) == -1) {
			loggy << "[CCPDN-ERROR]: Couldn't set pcap filter: " << pcap_geterr(handle) << std::endl;
			return;
		}

		// Start capturing packets
		loggy << "[CCPDN]: Starting packet capture...\n";

		// Create thread to monitor kill signal for loop
		std::thread killThread([handle, run]() {
			while (*run) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			pcap_breakloop(handle);
		});

		pcap_loop(handle, 0, packetHandler, reinterpret_cast<u_char*>(this));

		// Close the handle
		pcap_close(handle);
		loggy << "[CCPDN]: Packet capture complete.\n";
#endif
	}

	Controller* con;
		
	private:
};

#endif