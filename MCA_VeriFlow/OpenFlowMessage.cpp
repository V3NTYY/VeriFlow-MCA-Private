#include "OpenFlowMessage.h"

// Define ntohll macro for network-byte conversion
#ifndef ntohll
#define ntohll(x) (((uint64_t)ntohl((uint32_t)((x << 32) >> 32))) << 32) | ntohl(((uint32_t)(x >> 32)))
#endif

ofp_header OpenFlowMessage::createHello()
{
	ofp_header header;
	std::memset(&header, 0, sizeof(header));

	// Set the OF header values
#ifdef __unix__
	header.version = OFP_10;
	header.type = OFPT_HELLO;
	header.length = htons(sizeof(ofp_header));
	header.xid = htonl(100 + (std::rand() % 4095 - 99)); // XID is not used in hello message
#endif
	
    return header;
}

// TODO: FULLY FORMAT TO FIX PSH,ACK
ofp_stats_request OpenFlowMessage::createFlowRequest()
{
	// Construct our stats_request, match and flow_stats_request structs
	ofp_stats_request request;
	std::memset(&request, 0, sizeof(request));
	ofp_match toMatch;
	std::memset(&toMatch, 0, sizeof(toMatch));
	ofp_flow_stats_request toBody;
	std::memset(&toBody, 0, sizeof(toBody));

	// XID can be random
	uint32_t XID = (100 + (std::rand() % 4095 - 99));

	// Calculate request size
	uint16_t request_size = sizeof(ofp_stats_request) + sizeof(ofp_flow_stats_request);
	loggy << "Request size: " << request_size << std::endl;

#ifdef __unix__

	// Set our match wildcards
	toMatch.wildcards = htonl((1 << 22) - 1); // Match all fields

	// Set our body values (and attach match to body)
	toBody.table_id = 0xFF; // Match all tables
	toBody.out_port = htons(0xFFFF); // Match all output ports
	toBody.match = toMatch;

	// Set the values of our request header
	request.header.version = OFP_10;
	request.header.type = OFPT_STATS_REQUEST;
	request.header.length = htons(request_size);
	request.header.xid = htonl(XID);

	// Set the values of our request
	request.type = htons(OFPST_FLOW);
	request.flags = 0;
#endif

	// Allocate a buffer for the request size
	std::vector<ofp_stats_request> buffer(request_size);
	std::memset(buffer.data(), 0, request_size);

	// Copy the request into the buffer
	std::memcpy(buffer.data(), &request, sizeof(ofp_stats_request));
	// Copy our body (flow_stats_request) into buffer
	std::memcpy(buffer.data() + sizeof(ofp_stats_request), &toBody, sizeof(ofp_flow_stats_request));

	// Create new ofp_stats_request object to return
	ofp_stats_request requestReturn = *reinterpret_cast<ofp_stats_request*>(buffer.data());

#ifdef __unix__
	// DEBUG: Print requestReturn data to ensure its correct
	loggy << "Request header version: " << requestReturn.header.version << std::endl;
	loggy << "Request header type: " << requestReturn.header.type << std::endl;
	loggy << "Request header length: " << ntohs(requestReturn.header.length) << std::endl;
	loggy << "Request header xid: " << ntohl(requestReturn.header.xid) << std::endl;
	loggy << "Request type: " << ntohs(requestReturn.type) << std::endl;
	loggy << "Request flags: " << ntohs(requestReturn.flags) << std::endl;

	uint8_t* ofp_flow_stats_ptr = reinterpret_cast<uint8_t*>(&requestReturn) + sizeof(ofp_stats_reply);

	// Cast ptr to access flow_stats struct
	ofp_flow_stats* flow_stats = reinterpret_cast<ofp_flow_stats*>(ofp_flow_stats_ptr);

	// Debug print our flow 2 send
	loggy << "Match fields: " << std::endl;
	loggy << "Match wildcards: " << ntohl(flow_stats->match.wildcards) << std::endl;
	loggy << "Match in_port: " << ntohs(flow_stats->match.in_port) << std::endl;

	loggy << "Flow stats table_id: " << flow_stats->table_id << std::endl;
	loggy << "Flow stats duration_sec: " << flow_stats->duration_sec << std::endl;
	loggy << "Flow stats duration_nsec: " << flow_stats->duration_nsec << std::endl;
	loggy << "Flow stats priority: " << flow_stats->priority << std::endl;
	loggy << "Flow stats idle_timeout: " << flow_stats->idle_timeout << std::endl;
	loggy << "Flow stats hard_timeout: " << flow_stats->hard_timeout << std::endl;
	loggy << "Flow stats cookie: " << flow_stats->cookie << std::endl;
	loggy << "Flow stats packet_count: " << flow_stats->packet_count << std::endl;
	loggy << "Flow stats byte_count: " << flow_stats->byte_count << std::endl;
	loggy << "Flow stats actions: " << flow_stats->actions[0].type << std::endl;
#endif

	return requestReturn;
}

ofp_switch_features OpenFlowMessage::createFeaturesReply(uint32_t XID)
{
	ofp_switch_features reply;
	std::memset(&reply, 0, sizeof(reply));

	// Set the OF header values
#ifdef __unix__
	reply.header.version = OFP_10;
	reply.header.type = OFPT_FEATURES_REPLY;
	reply.header.length = htons(sizeof(ofp_switch_features));
	reply.header.xid = XID;

	// Set the features -- buffers/tables should NOT be in network-endian order
	reply.n_buffers = 256;
	reply.n_tables = 0xFF;
	reply.capabilities = htonl(0xFFFF);
	reply.actions = htonl(0xFFFF);
#endif

	return reply;
}

// TODO: FULLY FORMAT TO FIX PSH,ACK
ofp_stats_reply OpenFlowMessage::createDescStatsReply(uint32_t XID) {
    // Create the stats reply
    ofp_stats_reply reply;
    std::memset(&reply, 0, sizeof(reply));

#ifdef __unix__
    // Set the OpenFlow header
    reply.header.version = OFP_10;
    reply.header.type = OFPT_STATS_REPLY;
    reply.header.xid = htonl(XID);

    // Set the stats reply type to OFPST_DESC
    reply.type = htons(OFPST_DESC);
    reply.flags = 0; // No more replies (set OFPSF_REPLY_MORE if there are more parts)

    // Create the ofp_desc_stats structure
    ofp_desc_stats desc;
    std::memset(&desc, 0, sizeof(desc));

    // Populate the description fields
    std::strncpy(desc.mfr_desc, "BensingtonInc", sizeof(desc.mfr_desc) - 1);
    std::strncpy(desc.hw_desc, "BensingtonRouter", sizeof(desc.hw_desc) - 1);
    std::strncpy(desc.sw_desc, "BensingtonSoftware", sizeof(desc.sw_desc) - 1);
    std::strncpy(desc.serial_num, "111111111", sizeof(desc.serial_num) - 1);
    std::strncpy(desc.dp_desc, "BensingtonPath", sizeof(desc.dp_desc) - 1);

    // Calculate the total size of the reply
    uint16_t reply_size = sizeof(ofp_stats_reply) + sizeof(ofp_desc_stats);
    reply.header.length = htons(reply_size);

    // Copy body of reply into buffer
    std::vector<uint8_t> buffer(reply_size);
    std::memcpy(buffer.data(), &reply, sizeof(ofp_stats_reply));
    std::memcpy(buffer.data() + sizeof(ofp_stats_reply), &desc, sizeof(ofp_desc_stats));
#endif

    return reply;
}

// TODO: FULLY FORMAT TO FIX PSH,ACK
ofp_header OpenFlowMessage::createBarrierReply(uint32_t XID)
{
    ofp_header header;
	std::memset(&header, 0, sizeof(header));

	// Set the OF header values
#ifdef __unix__
	header.version = OFP_10;
	header.type = OFPT_BARRIER_REPLY;
	header.length = htons(sizeof(ofp_header));
	header.xid = htonl(XID);
#endif
	
    return header;
}

std::string OpenFlowMessage::ipToString(uint32_t ip)
{
	// Output string. This method expects host-endian order
	std::string ip_str = "";

	// Convert to string
#ifdef __unix__
	struct in_addr ip_addr;
	ip_addr.s_addr = ntohl(ip);
	ip_str = inet_ntoa(ip_addr);
#endif

	return ip_str;
}

std::string OpenFlowMessage::getRulePrefix(uint32_t wildcards, uint32_t srcIP) // should be in format 192.168.0.0/24
{
	// Output string. This method expects host-endian order
	std::string output = "";

	// get subnet mask length for prefix
	int mask_length = 32 - (wildcards & 0xFF);
	
	// get ip address from nw_src
	std::string ip_str = ipToString(srcIP);
	
	// Check if we got an IP
	if (ip_str.empty()) {
		return output;
	}

	output += ip_str + "/" + std::to_string(mask_length);

	return output;
}
