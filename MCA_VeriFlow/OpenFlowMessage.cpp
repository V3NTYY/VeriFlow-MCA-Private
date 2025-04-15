#include "OpenFlowMessage.h"

void sendStatsRequest(int sockfd, uint32_t xid) {
	//// Create FlowStatsRequest message
	//ofp_stats_reply stats_req;
	//std::memset(&stats_req, 0, sizeof(stats_req));

	//// set openflowmessage params
	//stats_req.header.version = OFP_10;
	//stats_req.header.type = OFPT_STATS_REQUEST;
	//stats_req.header.length = htons(sizeof(stats_req));
	//stats_req.header.xid = htonl(xid);

	//// set type+flags
	//stats_req.type = htons(OFPST_FLOW);
	//stats_req.flags = 0;

	////set request fields
	//stats_req.table_id = 0;
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

Flow OpenFlowMessage::parse()
{
	Flow f;

	// If the type is an openflow mod/removal, we need to parse the payload data into a flow (Action#switch ip - rule prefix - next hop)
	if (type == OFPT_FLOW_MOD || type == OFPT_PACKET_OUT) {
		// If we don't have a payload then what are we doing
		if (payload.empty()) {
			return f;
		}

		// Create a vector of bytes from the payload string
		std::vector<uint8_t> bytes(payload.begin(), payload.end());

		// Parse the flow from the byte vector
		// some flow_tobytes method here
	}

	return f;
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
