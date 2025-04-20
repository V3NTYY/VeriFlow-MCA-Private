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

// Function to split a string into a vector of words based on delimiters
std::vector<std::string> splitInputTCP(std::string input, std::vector<std::string> delimiters) {
    std::vector<std::string> words;
    std::string word;

    // If the line is empty, return early
    if (input.empty()) {
		return words;
	}

    // Iterate through the entire input string. If a delimiter is found, the first half is added as a word. The second half is then checked for delimiters again.
    for (size_t i = 0; i < input.length(); i++) {
		bool isDelimiter = false;
		for (std::string delimiter : delimiters) {
			if (input.substr(i, delimiter.length()) == delimiter) {
				isDelimiter = true;
				if (!word.empty()) {
					words.push_back(word);
					word.clear();
				}
				break;
			}
		}
		if (!isDelimiter) {
			word += input[i];
            // If this is the last word, push it back
            if (i == input.length() - 1) {
                words.push_back(word);
            }
		}
	}

    return words;
}

class TCPAnalyzer {
	public:
		// Thread method
		void thread(bool *run, std::string controllerPort) {
			loggy << "\n\n[CCPDN]: Starting TCPDump thread...\n";
			while (*run) {
				runTCPDump(controllerPort, run);
			}
		}

		void runTCPDump(std::string port, bool *run) {
			// Construct the tcpdump command
			std::string pidCommand = "echo $! > /tmp/ccpdn_xterm_pid";
			std::string sysCommand = "xterm -hold -e \"sudo tcpdump -v -i lo tcp port " + port + " -l -n  > /dev/null 2>&1\" & " + pidCommand;

			#ifdef __unix__
			// Open a pipe to read the output of tcpdump
			FILE* pipe = popen(sysCommand.c_str(), "r");
			if (!pipe) {
				loggyErr("[CCPDN-ERROR]: Failed to open pipe for tcpdump.\n");
				throw std::runtime_error("Failed to run tcpdump: " + std::string(strerror(errno)));
			}

			std::string readBuffer;
			char chunk[1024];
			while ((fgets(chunk, sizeof(chunk), pipe) != nullptr) && (*run)) {
				// Process our continuous stream of bytes
				readBuffer.append(chunk);

				// Process the buffer
				processBuffer(readBuffer);
			}

			// Close the pipe
			pclose(pipe);
			#endif
		}

		size_t findTimestamp(const std::string& buffer, size_t compare) {
			// Look for the first occurrence of a colon, FORMAT: [hour:minute:second], 24:00:00.000000
			size_t pos = buffer.find_first_of("0123456789:");

			size_t comparePos = compare;

			while (pos != std::string::npos) {
				// Check if the next characters match the timestamp format
				// If we are using "star
				if ((isdigit(buffer[pos])) && (buffer[pos + 2] == ':') && (buffer[pos + 5] == ':') && (pos != comparePos)) {
					return pos; // Found a timestamp
				}
				pos = buffer.find_first_of("0123456789:", pos + 1); // Continue searching
			}
			return std::string::npos; // No timestamp found
		}

		void processBuffer(std::string& buffer) {
			// Identify start of packet through timestamp
			size_t start = findTimestamp(buffer, -1);
			if (start == std::string::npos) { // wait until we have more data
				return;
			}

			// Identify end of packet by the start of next timestamp, the previous
			size_t end = findTimestamp(buffer, start) - 1;
			if (start == std::string::npos) { // wait until we have more data
				return;
			}

			// Extract the packet data from the buffer
			std::string packetData = buffer.substr(start, end - start + 1);
			buffer.erase(0, end + 1); // Remove the processed packet from the buffer

			// Process the packet data
			packet newPacket = analyzePacket(packetData);
			if (!newPacket.empty()) {
				this->buffer.push_back(newPacket); // Add the packet to the buffer
			}
		}

		packet analyzePacket(const std::string& packetData) {

			// Print the packet data for dbg
			loggy << "Captured Packet: " << packetData << std::endl;

			// Find start of OpenFlow section
			size_t start = packetData.find("OpenFlow");

			// Check if the packet is an OpenFlow packet
			if (start == std::string::npos) {
				return packet();
			}

			// Extract the relevant data from the packet
			std::string splicedData = packetData.substr(start);

			// Trim all newlines, tabs, and spaces from the string
			splicedData.erase(std::remove(splicedData.begin(), splicedData.end(), '\n'), splicedData.end());
			splicedData.erase(std::remove(splicedData.begin(), splicedData.end(), '\t'), splicedData.end());
			splicedData.erase(std::remove(splicedData.begin(), splicedData.end(), ' '), splicedData.end());
			splicedData.erase(std::remove(splicedData.begin(), splicedData.end(), '\r'), splicedData.end());

			// Split the spliced data into words by using a comma as a delimiter
			std::vector<std::string> words = splitInputTCP(splicedData, { "," });

			// For now, print this data to log for debugging
			loggy << "Spliced Data: " << splicedData << std::endl;
			loggy << "Words: ";
			for (const auto& word : words) {
				loggy << word << " ";
			}
			loggy << std::endl;

			return packet();
		}

		std::vector<packet> buffer;
	private:
};

#endif