#include <cstdint>
#include <string>
#include <array>

// OpenFlow enum pulled from https://opennetworking.org/wp-content/uploads/2014/10/openflow-switch-v1.5.1.pdf
enum ofp_type {
	/* Immutable messages. */
	OFPT_HELLO = 0, /* Symmetric message */
	OFPT_ERROR = 1, /* Symmetric message */
	OFPT_ECHO_REQUEST = 2, /* Symmetric message */
	OFPT_ECHO_REPLY = 3, /* Symmetric message */
	OFPT_EXPERIMENTER = 4, /* Symmetric message */
	/* Switch configuration messages. */
	OFPT_FEATURES_REQUEST = 5, /* Controller/switch message */
	OFPT_FEATURES_REPLY = 6, /* Controller/switch message */
	OFPT_GET_CONFIG_REQUEST = 7, /* Controller/switch message */
	OFPT_GET_CONFIG_REPLY = 8, /* Controller/switch message */
	OFPT_SET_CONFIG = 9, /* Controller/switch message */
	/* Asynchronous messages. */
	OFPT_PACKET_IN = 10, /* Async message */
	OFPT_FLOW_REMOVED = 11, /* Async message */
	OFPT_PORT_STATUS = 12, /* Async message */
	/* Controller command messages. */
	OFPT_PACKET_OUT = 13, /* Controller/switch message */
	OFPT_FLOW_MOD = 14, /* Controller/switch message */
	OFPT_GROUP_MOD = 15, /* Controller/switch message */
	OFPT_PORT_MOD = 16, /* Controller/switch message */
	OFPT_TABLE_MOD = 17, /* Controller/switch message */
	/* Multipart messages. */
	OFPT_MULTIPART_REQUEST = 18, /* Controller/switch message */
	OFPT_MULTIPART_REPLY = 19, /* Controller/switch message */
	/* Barrier messages. */
	OFPT_BARRIER_REQUEST = 20, /* Controller/switch message */
	OFPT_BARRIER_REPLY = 21, /* Controller/switch message */
	/* Controller role change request messages. */
	OFPT_ROLE_REQUEST = 24, /* Controller/switch message */
	OFPT_ROLE_REPLY = 25, /* Controller/switch message */
	/* Asynchronous message configuration. */
	OFPT_GET_ASYNC_REQUEST = 26, /* Controller/switch message */
	OFPT_GET_ASYNC_REPLY = 27, /* Controller/switch message */
	OFPT_SET_ASYNC = 28, /* Controller/switch message */
	/* Meters and rate limiters configuration messages. */
	OFPT_METER_MOD = 29, /* Controller/switch message */
	/* Controller role change event messages. */
	OFPT_ROLE_STATUS = 30, /* Async message */
	/* Asynchronous messages. */
	OFPT_TABLE_STATUS = 31, /* Async message */
	/* Request forwarding by the switch. */
	OFPT_REQUESTFORWARD = 32, /* Async message */
	/* Bundle operations (multiple messages as a single operation). */
	OFPT_BUNDLE_CONTROL = 33, /* Controller/switch message */
	OFPT_BUNDLE_ADD_MESSAGE = 34, /* Controller/switch message */
	/* Controller Status async message. */
	OFPT_CONTROLLER_STATUS = 35, /* Async message */
};

enum ofp_version {
	OFP_10 = 0x01,
	OFP_11 = 0x02,
	OFP_12 = 0x03,
	OFP_13 = 0x04,
	OFP_14 = 0x05,
	OFP_15 = 0x06,
	OFP_16 = 0x07,
	OFP_17 = 0x08,
	OFP_18 = 0x09,
	OFP_19 = 0x0A,
	OFP_20 = 0x0B,
	OFP_21 = 0x0C,
	OFP_22 = 0x0D,
	OFP_23 = 0x0E,
	OFP_24 = 0x0F,
	OFP_25 = 0x10,
	OFP_26 = 0x11,
	OFP_27 = 0x12,
	OFP_28 = 0x13,
	OFP_29 = 0x14,
	OFP_30 = 0x15,
	OFP_31 = 0x16,
	OFP_32 = 0x17,
	OFP_33 = 0x18,
	OFP_34 = 0x19,
	OFP_35 = 0x1A,
};

class OpenFlowMessage {
	public:
		OpenFlowMessage(uint8_t type, uint8_t version, uint16_t length, uint32_t xid, std::string payload);
		std::array<char,8> toChar();
		static OpenFlowMessage helloMessage();

		// Header data
		uint8_t		version;	// 0x01 = 1.0
		uint8_t		type;		// One of the OFPT constants
		uint16_t	length;		// Length of this header
		uint32_t	xid;		// Transaction id associated with this packet
		std::string	payload;	// Payload of the message
	private:
};