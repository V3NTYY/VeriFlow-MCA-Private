#include "OpenFlowMessage.h"

OpenFlowMessage OpenFlowMessage::helloMessage() {

	OpenFlowMessage msg(OFPT_HELLO, OFP_10, 8, 0, "");
	return msg;
}

OpenFlowMessage::OpenFlowMessage(uint8_t Type, uint8_t Version, uint16_t Length, uint32_t Xid, std::string Payload)
{
	type = Type;
	version = Version;
	length = Length;
	xid = Xid;
	payload = Payload;
}

std::array<char, 8> OpenFlowMessage::toChar()
{
	std::array<char, 8> output;
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