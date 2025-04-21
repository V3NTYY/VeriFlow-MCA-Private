#include "OpenFlowMessage.h"

// Define ntohll macro for network-byte conversion
#ifndef ntohll
#define ntohll(x) (((uint64_t)ntohl((uint32_t)((x << 32) >> 32))) << 32) | ntohl(((uint32_t)(x >> 32)))
#endif

// Define htonll macro for host-byte conversion
#ifndef htonll
#define htonll(x) (((uint64_t)htonl((uint32_t)((x << 32) >> 32))) << 32) | htonl(((uint32_t)(x >> 32)))
#endif

std::vector<unsigned char> OpenFlowMessage::createHello(uint32_t XID)
{
	// Initialize header struct
	ofp_header header;
	std::memset(&header, 0, sizeof(header));

#ifdef __unix__
	// Set the OF header values
	header.version = OFP_10;
	header.type = OFPT_HELLO;
	header.length = htons(sizeof(ofp_header));
	header.xid = htonl(XID);
#endif
	
    // Create an unsigned char (blessed casting type) vector to store our struct
	std::vector<unsigned char> buffer(sizeof(ofp_header));
	// Store the first 8 bytes (ofp_header struct, byte 1-8)
	std::memcpy(buffer.data(), &header, sizeof(ofp_header));

	return buffer;
}

std::vector<unsigned char> OpenFlowMessage::createFlowRequest()
{
	// Construct our ofp_stats_request struct, and initialize it
	ofp_stats_request request;
	std::memset(&request, 0, sizeof(ofp_stats_request));

	// Construct our ofp_flow_stats_request struct, and initialize it as well
	ofp_flow_stats_request flow_request;
	std::memset(&flow_request, 0, sizeof(ofp_flow_stats_request));

#ifdef __unix__
	// Populate ofp_stats_request struct fields
	request.header.version = OFP_10;
	request.header.type = OFPT_STATS_REQUEST;
	request.header.length = htons(sizeof(ofp_stats_request) + sizeof(ofp_flow_stats_request));
	request.header.xid = htonl(100 + (std::rand() % 4095 - 99)); // XID is not used in hello message
	request.type = htons(OFPST_FLOW);
	request.flags = htons(0); // No flags set

	// Populate our ofp_flow_stats_request struct fields
	flow_request.match.wildcards = htonl((1 << 22) - 1); // Match all fields
	flow_request.match.in_port = htons(0); // Match all ports
	flow_request.table_id = 0xFF; // Match all tables
	flow_request.pad = 0; // Padding to align to 32 bits
	flow_request.out_port = 0xFFFF; // Match all ports. HTONL not necessary since all bytes are the same here.
#endif

	// Create an unsigned char (blessed casting type) vector to store our struct
	std::vector<unsigned char> buffer(sizeof(ofp_stats_request) + sizeof(ofp_flow_stats_request));
	// Store the first 12 bytes (ofp_stats_request struct, byte 1-12)
	std::memcpy(buffer.data(), &request, sizeof(ofp_stats_request));
	// Store the next 44 bytes (ofp_flow_stats_request struct, byte 13-56)
	std::memcpy(buffer.data() + sizeof(ofp_stats_request), &flow_request, sizeof(ofp_flow_stats_request));

	return buffer;
}

std::vector<unsigned char> OpenFlowMessage::createFeaturesReply(uint32_t XID)
{
	// Initialize ofp_switch_features struct
	ofp_switch_features reply;
	std::memset(&reply, 0, sizeof(reply));

#ifdef __unix__
	// Set the OF header values
	reply.header.version = OFP_10;
	reply.header.type = OFPT_FEATURES_REPLY;
	reply.header.length = htons(sizeof(ofp_switch_features));
	reply.header.xid = htonl(XID);

	// Set the features -- buffers/tables should NOT be in network-endian order
	reply.datapath_id = htonll(CCPDN_IDENTIFIER);
	reply.n_buffers = 256;
	reply.n_tables = 0xFF;
	reply.capabilities = htonl(0xFFFF);
	reply.actions = htonl(0xFFFF);
#endif

	// Create an unsigned char (blessed casting type) vector to store our struct
	std::vector<unsigned char> buffer(sizeof(ofp_switch_features));
	// Store the bytes (ofp_switch_features struct)
	std::memcpy(buffer.data(), &reply, sizeof(ofp_switch_features));

	return buffer;
}

std::vector<unsigned char> OpenFlowMessage::createDescStatsReply(uint32_t XID) {
    // Initialize the stats_reply struct
    ofp_stats_reply reply;
    std::memset(&reply, 0, sizeof(reply));

	// Initialize the ofp_desc_stats struct
    ofp_desc_stats desc;
    std::memset(&desc, 0, sizeof(desc));

#ifdef __unix__
    // Set the OpenFlow header
    reply.header.version = OFP_10;
    reply.header.type = OFPT_STATS_REPLY;
    reply.header.xid = htonl(XID);
	reply.header.length = htons(sizeof(ofp_stats_reply) + sizeof(ofp_desc_stats));

    // Set the stats reply type to OFPST_DESC
    reply.type = htons(OFPST_DESC);
    reply.flags = 0; // No more replies (set OFPSF_REPLY_MORE if there are more parts)

    // Populate the description fields
    std::strncpy(desc.mfr_desc, "BensingtonInc", sizeof(desc.mfr_desc) - 1);
    std::strncpy(desc.hw_desc, "BensingtonRouter", sizeof(desc.hw_desc) - 1);
    std::strncpy(desc.sw_desc, "BensingtonSoftware", sizeof(desc.sw_desc) - 1);
    std::strncpy(desc.serial_num, "111111111", sizeof(desc.serial_num) - 1);
    std::strncpy(desc.dp_desc, "BensingtonPath", sizeof(desc.dp_desc) - 1);
#endif

    // Create an unsigned char (blessed casting type) vector to store our struct
	std::vector<unsigned char> buffer(sizeof(ofp_stats_reply) + sizeof(ofp_desc_stats));
	// Store the bytes (ofp_stats_reply struct)
	std::memcpy(buffer.data(), &reply, sizeof(ofp_stats_reply));
	// Store the extra bytes (ofp_desc_stats struct)
	std::memcpy(buffer.data() + sizeof(ofp_stats_reply), &desc, sizeof(ofp_desc_stats));

	return buffer;
}

std::vector<unsigned char> OpenFlowMessage::createFlowStatsReply(uint32_t XID)
{
	// Initialize the stats_reply struct
    ofp_stats_reply reply;
    std::memset(&reply, 0, sizeof(reply));

	// Initialize the ofp_flow_stats struct
    ofp_flow_stats flow;
    std::memset(&flow, 0, sizeof(flow));

#ifdef __unix__
    // Set the OpenFlow header
    reply.header.version = OFP_10;
    reply.header.type = OFPT_STATS_REPLY;
    reply.header.xid = htonl(XID);
	reply.header.length = htons(sizeof(ofp_stats_reply) + sizeof(ofp_flow_stats));

    // Set the stats reply type to OFPST_FLOW
    reply.type = htons(OFPST_FLOW);
    reply.flags = 0; // No more replies (set OFPSF_REPLY_MORE if there are more parts)
#endif

    // Create an unsigned char (blessed casting type) vector to store our struct
	std::vector<unsigned char> buffer(sizeof(ofp_stats_reply) + sizeof(ofp_flow_stats));
	// Store the bytes (ofp_stats_reply struct)
	std::memcpy(buffer.data(), &reply, sizeof(ofp_stats_reply));
	// Store the extra bytes (ofp_desc_stats struct)
	std::memcpy(buffer.data() + sizeof(ofp_stats_reply), &flow, sizeof(ofp_flow_stats));

	return buffer;
}

std::vector<unsigned char> OpenFlowMessage::createBarrierReply(uint32_t XID)
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
	
    // Create an unsigned char (blessed casting type) vector to store our struct
	std::vector<unsigned char> buffer(sizeof(ofp_header));
	// Store the first 8 bytes (ofp_header struct, byte 1-8)
	std::memcpy(buffer.data(), &header, sizeof(ofp_header));

	return buffer;
}

std::vector<unsigned char> OpenFlowMessage::createFlowAdd(Flow f, uint32_t XID)
{
	// Initialize the flow_mod struct
	ofp_flow_mod flow_mod;
	std::memset(&flow_mod, 0, sizeof(flow_mod));

	return std::vector<unsigned char>();
}

std::vector<unsigned char> OpenFlowMessage::createFlowRemove(Flow f, uint32_t XID)
{
    return std::vector<unsigned char>();
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

	// Use wildcard to get subnet mask
    int wildcard_bits = (wildcards >> 6) & 0x3F;
    int mask_length = 32 - wildcard_bits;
	
	// get ip address from nw_src
	std::string ip_str = ipToString(srcIP);
	
	// Check if we got an IP
	if (ip_str.empty()) {
		return output;
	}

	output += ip_str + "/" + std::to_string(mask_length);

	return output;
}
