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
#include "Flow.h"
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

#ifdef __unix__
#include <pcap.h>
#endif

typedef uint8_t byte;
typedef std::vector<byte> packet;

struct TimestampPacket {
	std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;
	packet data;

	const bool smallerTime(const TimestampPacket other) const {
		return timestamp < other.timestamp;
	}
};

class TCPAnalyzer {

	public:

		static bool pingFlag;
		static std::vector<TimestampPacket> currentPackets;
		static std::mutex currentPacketsMutex;

		// Thread method
		void thread(bool *run, std::string controllerPort) {
			while (*run) {
				pingFlag = false;
				currentPackets.clear();
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				loggy << "\n\n[CCPDN]: Starting LibPCap thread...\n";
				startPacketCapture("lo", "tcp port " + controllerPort, run);
			}
		}

#ifdef __unix__
	void packetHandler(const struct pcap_pkthdr* pkthdr, const u_char* packet) {

		// Give our other threads a notifier if they have packets to work on!
		if (currentPackets.size() > 0) {
			pingFlag = true;
		}

		// Ensure valid data
		if (pkthdr == nullptr || packet == nullptr) {
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

		if (payload.empty()) {
			return;
		}

		// Create a timestamped packet
		TimestampPacket tsPacket = {std::chrono::high_resolution_clock::now(), payload};

		// Utilize parsing methods from controller, and update controller remotely
		{
			// Lock mutex to ensure thread safety
			std::lock_guard<std::mutex> lock(currentPacketsMutex);
			currentPackets.push_back(tsPacket);
			pingFlag = true;
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

		pcap_set_buffer_size(handle, 10 * 1024 * 1024); // 10 MB buffer
		pcap_set_immediate_mode(handle, 1);

		// Start capturing packets
		loggy << "[CCPDN]: Successfully started packet capture\n";

		const u_char* packet;
		struct pcap_pkthdr header;

		// Create thread loop
		while (*run) {

			// Give our other threads a notifier if they have packets to work on!
			if (currentPackets.size() > 0) {
				pingFlag = true;
			}

			// Capture packets
			pcap_loop(handle, 0, [](u_char* user, const struct pcap_pkthdr* pkthdr, const u_char* packet) {
				TCPAnalyzer* analyzer = reinterpret_cast<TCPAnalyzer*>(user);
				analyzer->packetHandler(pkthdr, packet);
			}, reinterpret_cast<u_char*>(this));
		}

		// Close the handle
		if (handle != nullptr) {
			pcap_close(handle);
		}
		loggy << "[CCPDN]: Packet capture complete\n";
#endif
	}
		
	private:
};

#endif