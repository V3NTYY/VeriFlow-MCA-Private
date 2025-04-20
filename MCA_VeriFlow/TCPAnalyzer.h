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

		int ETHERNET_HEADER_SIZE = 14;
		// Extract IP Frame
		const u_char* ipHeader = packet + ETHERNET_HEADER_SIZE;
		int IP_HEADER_SIZE = ((ipHeader[0] & 0x0F) * 4);
		loggy << "IP Header Size: " << IP_HEADER_SIZE << std::endl;

		// Extract TCP Frame
		const u_char* tcpHeader = ipHeader + (IP_HEADER_SIZE);
		int TCP_HEADER_SIZE = ((tcpHeader[12] & 0xF0) >> 4) * 4;
		loggy << "TCP Header Size: " << TCP_HEADER_SIZE << std::endl;
		int TOTAL_PAYLOAD_SIZE = pkthdr->len - (ETHERNET_HEADER_SIZE + IP_HEADER_SIZE + TCP_HEADER_SIZE);

		// Extract TCP Payload as vector of bytes
		std::vector<byte> payload(tcpHeader + TCP_HEADER_SIZE, tcpHeader + pkthdr->len - ETHERNET_HEADER_SIZE - IP_HEADER_SIZE);

		// Print timestamp
		loggy << "Timestamp: " << pkthdr->ts.tv_sec << "." << pkthdr->ts.tv_usec << std::endl;
		loggy << "Payload size: " << payload.size() << std::endl;
		loggy << "Payload data: ";
		for (int i = 0; i < payload.size(); i++) {
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