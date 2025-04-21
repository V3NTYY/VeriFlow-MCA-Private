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

		static bool pingFlag;
		static std::vector<std::vector<byte>> currentPackets;

		// Thread method
		void thread(bool *run, Controller *controller) {
			while (*run) {
				pingFlag = false;
				currentPackets.clear();
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				loggy << "\n\n[CCPDN]: Starting LibPCap thread...\n";
				startPacketCapture("lo", "tcp port " + controller->controllerPort, run);
			}
		}

#ifdef __unix__
	void packetHandler(const struct pcap_pkthdr* pkthdr, const u_char* packet) {
		// Ensure valid data
		if (pkthdr == nullptr || packet == nullptr) {
			return;
		}
		if (pkthdr->len <= 0) {
			return;
		}

		int ETHERNET_HEADER_SIZE = 14;
		// Extract IP Frame
		const u_char* ipHeader = packet + ETHERNET_HEADER_SIZE;
		int IP_HEADER_SIZE = ((ipHeader[0] & 0x0F) * 4);

		// Extract TCP Frame
		const u_char* tcpHeader = ipHeader + (IP_HEADER_SIZE);
		int TCP_HEADER_SIZE = ((tcpHeader[12] & 0xF0) >> 4) * 4;
		int TOTAL_PAYLOAD_SIZE = pkthdr->len - (ETHERNET_HEADER_SIZE + IP_HEADER_SIZE + TCP_HEADER_SIZE);

		// Extract TCP Payload as vector of bytes
		std::vector<byte> payload(tcpHeader + TCP_HEADER_SIZE, tcpHeader + pkthdr->len - ETHERNET_HEADER_SIZE - IP_HEADER_SIZE);

		// Utilize parsing methods from controller, and update controller remotely
		currentPackets.push_back(payload);
		pingFlag = true;
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

		const u_char* packet;
		struct pcap_pkthdr header;

		// Create thread loop
		while (*run) {

			// Give our other threads a notifier if they have packets to work on!
			if (currentPackets.size() > 0) {
				pingFlag = true;
			}

			// Capture packets and process them
			packet = pcap_next(handle, &header);
			if (packet != nullptr) {
				packetHandler(&header, packet);
			} else {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}

		// Close the handle
		if (handle != nullptr) {
			pcap_close(handle);
		}
		loggy << "[CCPDN]: Packet capture complete.\n";
#endif
	}
		
	private:
};

#endif