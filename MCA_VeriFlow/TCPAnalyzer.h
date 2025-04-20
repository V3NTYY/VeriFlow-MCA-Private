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

typedef std::vector<uint8_t> packet;

class TCPAnalyzer {
	public:
		// Thread method
		void thread(bool *run) {
			while (*run) {
				runTCPDump(6653);
			}
		}

		bool isOpenFlow(std::vector<uint8_t> packet) {
			return false;
		}

		void runTCPDump(int port) {
			// Run tcpdump and output to our byte buffer. Use /dev/null 2>&1 to hide output
			std::string sysCommand = "sudo tcpdump -i lo tcp port " + std::to_string(port) + " > /dev/null 2>&1";
			int result = system(sysCommand.c_str());
		}

		void runTCPDump(int port) {
			// Construct the tcpdump command
			std::string sysCommand = "sudo tcpdump -i lo tcp port " + std::to_string(port) + " -l -n";

			#ifdef __unix__
			// Open a pipe to read the output of tcpdump
			FILE* pipe = popen(sysCommand.c_str(), "r");
			if (!pipe) {
				throw std::runtime_error("Failed to run tcpdump: " + std::string(strerror(errno)));
			}

			char buffer[1024];
			while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
				// Process each line of tcpdump output
				std::string packetData(buffer);
				packet newPacket = analyzePacket(packetData);

				// Add our packet if it qualifies under analyzePacket
				if (!newPacket.empty()) {
					this->buffer.push_back(newPacket);
				}
			}

			// Close the pipe
			pclose(pipe);
			#endif
		}

		packet analyzePacket(const std::string& packetData) {

			// Print the packet data for dbg
			loggy << "Captured Packet: " << packetData << std::endl;

			// Check if the packet is an OpenFlow packet
			if (packetData.find("OpenFlow") != std::string::npos) {
				// Extract the relevant data from the packet
				packet newPacket(packetData.begin(), packetData.end());
				return newPacket;
			}
			// If not an OpenFlow packet, return an empty packet
			return packet();
		}

		std::vector<packet> buffer;
	private:
};

#endif