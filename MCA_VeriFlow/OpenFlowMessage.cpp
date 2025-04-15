#include "OpenFlowMessage.h"

ofp_stats_full_req OpenFlowMessage::createFlowRequest() {
	// Construct our request
	uint32_t xid = std::rand();
	ofp_stats_full_req flow_stats_request;
	std::memset(&flow_stats_request, 0, sizeof(flow_stats_request));

#ifdef __unix__
	// Set the OF header values
	flow_stats_request.stats_req.header.version = 0x01; // OpenFlow 1.0
	flow_stats_request.stats_req.header.type = OFPT_STATS_REQUEST;
	flow_stats_request.stats_req.header.length = htons(sizeof(flow_stats_request));
	flow_stats_request.stats_req.header.xid = htonl(xid);

	// Request all flows
	flow_stats_request.stats_req.type = htons(OFPST_FLOW);
	flow_stats_request.stats_req.flags = 0;

	// We dont care about the specific stats
	flow_stats_request.table_id = 0xff;
	flow_stats_request.out_port = htonl(0xffffffff);
	flow_stats_request.out_group = htonl(0xffffffff);
	flow_stats_request.cookie = 0;
	flow_stats_request.cookie_mask = 0;
#endif
	return flow_stats_request;
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
		std::vector<ofp_flow_stats> flow_stats = parseFlowStats();

		// Iterate through flow stats, parsing each one into an actual flow object
		for (const ofp_flow_stats f : flow_stats) {
			std::string srcIP = ipToString(f.match.nw_src);
			std::string nextHopIP = ipToString(f.match.nw_dst);
			std::string rulePrefix = getRulePrefix(f.match);
			bool action = true;

			returnFlows.push_back(Flow(srcIP, rulePrefix, nextHopIP, action));
		}
	}

	return returnFlows;
}

std::vector<ofp_flow_stats> OpenFlowMessage::parseFlowStats()
{
	std::vector<ofp_flow_stats> flow_stats;
	int offset = 0;

	// Find count of amount of flow_stats headers within message
	int count = (length - sizeof(ofp_header)) / sizeof(ofp_flow_stats);
	// Loop through each section of message and craft flow_stats headers
	while (count > 0) {
		ofp_flow_stats flow_stat;
		std::memcpy(&flow_stat, payload.data() + offset, sizeof(ofp_flow_stats));

		// Use ntohs to ensure correct byte endian order with struct matching
#ifdef __unix__
		flow_stat.length = ntohs(flow_stat.length);
		// flow_stat.table_id = flow_stat.table_id; // No conversion needed
		flow_stat.duration_sec = ntohl(flow_stat.duration_sec);
		flow_stat.duration_nsec = ntohl(flow_stat.duration_nsec);
		flow_stat.priority = ntohs(flow_stat.priority);
		flow_stat.idle_timeout = ntohs(flow_stat.idle_timeout);
		flow_stat.hard_timeout = ntohs(flow_stat.hard_timeout);
		flow_stat.flags = ntohs(flow_stat.flags);
		flow_stat.cookie = ntohll(flow_stat.cookie);
		flow_stat.packet_count = ntohll(flow_stat.packet_count);
		flow_stat.byte_count = ntohll(flow_stat.byte_count);

		// Handle match field
		flow_stat.match.wildcards = ntohl(flow_stat.match.wildcards);
		flow_stat.match.in_port = ntohs(flow_stat.match.in_port);
		flow_stat.match.dl_vlan = ntohs(flow_stat.match.dl_vlan);
		flow_stat.match.dl_type = ntohs(flow_stat.match.dl_type);
		flow_stat.match.nw_src = ntohl(flow_stat.match.nw_src);
		flow_stat.match.nw_dst = ntohl(flow_stat.match.nw_dst);
		flow_stat.match.tp_src = ntohs(flow_stat.match.tp_src);
		flow_stat.match.tp_dst = ntohs(flow_stat.match.tp_dst);
		// Handle action field
		flow_stat.actions.type = ntohs(flow_stat.actions.type);
		flow_stat.actions.len = ntohs(flow_stat.actions.len);
		flow_stat.actions.port = ntohl(flow_stat.actions.port);
		flow_stat.actions.max_len = ntohs(flow_stat.actions.max_len);

#endif
		flow_stats.push_back(flow_stat);

		offset += sizeof(ofp_flow_stats);
		count--;
		
		if (offset >= length) {
			break;
		}

		if (flow_stats.size() < count) {
			break;
		}
		
	}

	return flow_stats;
}

std::string OpenFlowMessage::ipToString(uint32_t ip)
{
	std::string ip_str = "";

	// Convert to string
#ifdef __unix__
	struct in_addr ip_addr;
	ip_addr.s_addr = ntohl(match.nw_src);
	ip_str = std::to_string(inet_ntoa(ip_addr));
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
