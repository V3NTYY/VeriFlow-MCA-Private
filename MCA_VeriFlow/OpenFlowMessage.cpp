#include "OpenFlowMessage.h"

OpenFlowMessage OpenFlowMessage::helloMessage() {

	OpenFlowMessage msg(0, 1, 8, 0, "");
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
	output[3] = length & 0xFF;
	output[4] = xid >> 24;
	output[5] = xid >> 16;
	output[6] = xid >> 8;
	output[7] = xid & 0xFF;

	return output;
}