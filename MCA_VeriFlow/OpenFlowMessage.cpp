#include "OpenFlowMessage.h"

// Define ntohll macro for network-byte conversion
#ifndef ntohll
#define ntohll(x) (((uint64_t)ntohl((uint32_t)((x << 32) >> 32))) << 32) | ntohl(((uint32_t)(x >> 32)))
#endif

ofp_stats_request OpenFlowMessage::createFlowRequest() {
	// Construct our request
	uint32_t xid = std::rand();
	ofp_stats_request request;
	std::memset(&request, 0, sizeof(request));

	// Create our match request
	ofp_match toMatch;
	toMatch.wildcards = ((1 << 22) - 1); // Match all fields

	// Create our body of request
	ofp_flow_stats_request toBody;
	std::memset(&toBody, 0, sizeof(toBody));
	toBody.table_id = 0xFF; // Match all tables
	toBody.match = toMatch;

	// Calculate request size
	uint16_t request_size = sizeof(ofp_stats_request) + sizeof(ofp_flow_stats_request);

#ifdef __unix__
	// Set the OF header values
	request.header.version = OFP_10;
	request.header.type = OFPT_STATS_REQUEST;
	request.header.length = htons(request_size);
	request.header.xid = htonl(xid);

	request.type = OFPST_FLOW;
	request.flags = 0;
#endif

	// Allocate a buffer for the request size
	std::vector<uint8_t> buffer(request_size);
	std::memset(buffer.data(), 0, request_size);

	// Copy the request header into the buffer
	std::memcpy(buffer.data(), &request, sizeof(ofp_stats_request));
	// Copy the request body into the buffer
	std::memcpy(buffer.data() + sizeof(ofp_stats_request), &toBody, sizeof(ofp_flow_stats_request));

	// Create new ofp_stats_request object to return
	ofp_stats_request* request_ptr = reinterpret_cast<ofp_stats_request*>(buffer.data());

	return *request_ptr;
}

Flow OpenFlowMessage::parseStatsReply(ofp_flow_stats reply)
{
	Flow returnFlow("", "", "", false);

	// If there is no length, no reply
	if (reply.length == 0) {
		return returnFlow;
	}

	// Iterate through flow stats, parsing each one into an actual flow object
	std::string srcIP = ipToString(reply.match.nw_src);
	std::string nextHopIP = ipToString(reply.match.nw_dst);
	std::string rulePrefix = getRulePrefix(reply.match);
	bool action = true;

	returnFlow = Flow(srcIP, rulePrefix, nextHopIP, action);

	return returnFlow;
}

OpenFlowMessage OpenFlowMessage::helloMessage() {

	OpenFlowMessage msg(OFPT_HELLO, OFP_10, 0, "");
	return msg;
}

OpenFlowMessage::OpenFlowMessage(uint8_t Type, uint8_t Version, uint32_t Xid, std::string Payload)
{
	type = Type;
	version = Version;
	xid = Xid;
	payload = Payload;
	length = (uint16_t)(8) + static_cast<uint16_t>(Payload.size());
}

std::vector<uint8_t> OpenFlowMessage::toBytes()
{
	// Ensure correct vector size
	std::vector<uint8_t> output(8 + payload.size());

	output[0] = version;
	output[1] = type;
	output[2] = length >> 8;
	output[3] = length & 0xFF;
	output[4] = ((xid >> 24) & 0xFF);
	output[5] = ((xid >> 16) & 0xFF);
	output[6] = ((xid >> 8) & 0xFF);
	output[7] = xid & 0xFF;

	// Copy payload into vector
	std::copy(payload.begin(), payload.end(), output.begin() + 8);

	return output;
}

std::vector<Flow> OpenFlowMessage::parse()
{
	std::vector<Flow> returnFlows;

	// If the type is an openflow mod/removal, we need to parse the payload data into a flow (Action#switch ip - rule prefix - next hop)
	if (type == OFPT_FLOW_MOD || type == OFPT_PACKET_OUT || type == OFPT_STATS_REPLY) {

		// If we don't have a payload then what are we doing
		if (payload.empty()) {
			return returnFlows;
		}

		// Parse the flow stats from our message
		std::vector<ofp_flow_stats> flow_stats;

		// Iterate through flow stats, parsing each one into an actual flow object
		for (const ofp_flow_stats& f : flow_stats) {
			std::string srcIP = ipToString(f.match.nw_src);
			std::string nextHopIP = ipToString(f.match.nw_dst);
			std::string rulePrefix = getRulePrefix(f.match);
			bool action = true;

			returnFlows.push_back(Flow(srcIP, rulePrefix, nextHopIP, action));
		}
	}

	return returnFlows;
}

std::string OpenFlowMessage::ipToString(uint32_t ip)
{
	std::string ip_str = "";

	// Convert to string
#ifdef __unix__
	struct in_addr ip_addr;
	ip_addr.s_addr = ntohl(ip);
	ip_str = inet_ntoa(ip_addr);
#endif

	return ip_str;
}

std::string OpenFlowMessage::getRulePrefix(ofp_match match) // should be in format 192.168.0.0/24
{
	std::string output = "";

	// get subnet mask length for prefix
	int mask_length = 32 - (match.wildcards & 0xFF);
	
	// get ip address from nw_src
	std::string ip_str = ipToString(match.nw_src);
	
	// Check if we got an IP
	if (ip_str.empty()) {
		return output;
	}

	output += ip_str + "/" + std::to_string(mask_length);

	return output;
}

OpenFlowMessage OpenFlowMessage::fromBytes(std::vector<uint8_t> bytes)
{
	OpenFlowMessage msg(0,0,0,"");

	if (bytes.size() < 8) {
		return msg;
	}

	msg.version = bytes[0];
	msg.type = bytes[1];
	msg.length = (bytes[2] << 8) | bytes[3]; // Shift bits[2] to the left by 8 bits, the remaining 8 bits on the right then become bytes[3] with an OR
	msg.xid = (bytes[4] << 24) | (bytes[5] << 16) | (bytes[6] << 8) | bytes[7]; // Same applies here, except this will be 32-bits long
	return msg;
}
