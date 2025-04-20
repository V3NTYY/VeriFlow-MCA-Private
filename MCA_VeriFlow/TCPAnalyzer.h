#ifndef TCPANALYZER_H
#define TCPANALYZER_H

#include "Log.h"

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
		void thread(bool *run, std::string controllerPort) {
			loggy << "[CCPDN]: Starting TCPDump thread...\n";
			while (*run) {
				runTCPDump(controllerPort, run);
			}
		}

		void runTCPDump(std::string port, bool *run) {
			// Construct the tcpdump command
			std::string sysCommand = "xterm -hold -e \"sudo tcpdump -i lo tcp port " + port + " -l -n\" &";

			#ifdef __unix__
			// Open a pipe to read the output of tcpdump
			FILE* pipe = popen(sysCommand.c_str(), "r");
			if (!pipe) {
				loggyErr("[CCPDN-ERROR]: Failed to open pipe for tcpdump.\n");
				throw std::runtime_error("Failed to run tcpdump: " + std::string(strerror(errno)));
			}

			char buffer[1024];
			while ((fgets(buffer, sizeof(buffer), pipe) != nullptr) && (*run)) {
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