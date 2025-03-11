#include "OpenFlowMessage.h"

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
	length = 8 + static_cast<uint16_t>(payload.size());
}

std::vector<uint8_t> OpenFlowMessage::toBytes()
{
	std::vector<uint8_t> output;
	output[0] = type;
	output[1] = version;
	output[2] = length >> 8;
	output[3] = length >> 0xFF;
	output[4] = ((xid >> 24) & 0xFF);
	output[5] = ((xid >> 16) & 0xFF);
	output[6] = ((xid >> 8) & 0xFF);
	output[7] = xid & 0xFF;

	return output;
}