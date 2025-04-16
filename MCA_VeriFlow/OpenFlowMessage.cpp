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

ofp_stats_request OpenFlowMessage::createFlowRequest()
{
	ofp_stats_request request;
	ofp_match toMatch;
	ofp_flow_stats_request toBody;

	// Calculate request size
	uint16_t request_size = sizeof(ofp_stats_request) + sizeof(ofp_flow_stats_request);

#ifdef __unix__
    // Construct our request
	uint32_t XID = (100 + (std::rand() % 4095 - 99));
	std::memset(&request, 0, sizeof(request));

	// Create our match request
	std::memset(&toMatch, 0, sizeof(toMatch));
	toMatch.wildcards = htonl((1 << 22) - 1); // Match all fields

	// Create our body of request
	std::memset(&toBody, 0, sizeof(toBody));
	toBody.table_id = 0xFF; // Match all tables
	toBody.out_port = htons(0xFFFF); // Match all output ports
	toBody.match = toMatch;

	// Set the OF header values
	request.header.version = OFP_10;
	request.header.type = OFPT_STATS_REQUEST;
	request.header.length = htons(request_size);
	request.header.xid = htonl(XID);

	request.type = htons(OFPST_FLOW);
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
