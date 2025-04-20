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

#ifdef __unix__
#include <pcap.h>
#endif

typedef std::vector<uint8_t> packet;

class TCPAnalyzer {
	public:
		// Thread method
		void thread(bool *run, std::string controllerPort) {
			loggy << "\n\n[CCPDN]: Starting TCPDump thread...\n";
			startPacketCapture("lo", "tcp port " + controllerPort, run);
		}

#ifdef __unix__
	static void packetHandler(u_char* userData, const struct pcap_pkthdr* pkthdr, const u_char* packet) {
		// Cast userData back to TCPAnalyzer instance
		TCPAnalyzer* analyzer = reinterpret_cast<TCPAnalyzer*>(userData);

		// Log packet details
		analyzer->loggy << "Captured a packet with length: " << pkthdr->len << " bytes" << std::endl;

		// Example: Print the first 16 bytes of the packet
		for (int i = 0; i < 16 && i < pkthdr->len; i++) {
			analyzer->loggy << std::hex << (int)packet[i] << " ";
		}
		analyzer->loggy << std::endl;

		// TODO: Add logic to parse OpenFlow messages (e.g., FlowMod, FlowRemoved)
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
		
	private:
};

#endif