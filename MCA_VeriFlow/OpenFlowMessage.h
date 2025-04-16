#include <cstdint>
#include <string>
#include <vector>
#include <cstring>
#include "Flow.h"
#ifdef __unix__
#include <arpa/inet.h>
#endif

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

enum ofp_stats_types {
	/* Description of this OpenFlow switch.
	* The request body is empty.
	* The reply body is struct ofp_desc_stats. */
	OFPST_DESC,
	/* Individual flow statistics.
	* The request body is struct ofp_flow_stats_request.
	* The reply body is an array of struct ofp_flow_stats. */
	OFPST_FLOW,
	/* Aggregate flow statistics.
	* The request body is struct ofp_aggregate_stats_request.
	* The reply body is struct ofp_aggregate_stats_reply. */
	OFPST_AGGREGATE,
	/* Flow table statistics.
	* The request body is empty.
	* The reply body is an array of struct ofp_table_stats. */
	OFPST_TABLE,
	/* Physical port statistics.
	* The request body is struct ofp_port_stats_request.
	* The reply body is an array of struct ofp_port_stats. */
	OFPST_PORT,
	/* Queue statistics for a port
	* The request body defines the port
	* The reply body is an array of struct ofp_queue_stats */
	OFPST_QUEUE,
	/* Vendor extension.
	* The request and reply bodies begin with a 32-bit vendor ID, which takes
	* the same form as in "struct ofp_vendor_header". The request and reply
	* bodies are otherwise vendor-defined. */
	OFPST_VENDOR = 0xffff
};
// This works since our controller is OF v1.0
#define OFPT_STATS_REQUEST 16
#define OFPT_STATS_REPLY 17

// 8 bytes
struct ofp_header {
	uint8_t version;
	uint8_t type;
	uint16_t length;
	uint32_t xid;
};

// 8 bytes
struct ofp_action_header { // Unused but required field
	uint16_t type; /* One of OFPAT_*. */
	uint16_t len; /* Length of action, including this
	header. This is the length of action,
	including any padding to make it
	64-bit aligned. */
	uint8_t pad[4];
};

// 40 bytes
struct ofp_match { // Struct used for matching SRC IP, next hop & rule prefix
	uint32_t wildcards; /* Wildcard fields. */
	uint16_t in_port; /* Input switch port. */
	uint8_t dl_src[6]; /* Ethernet source address. */
	uint8_t dl_dst[6]; /* Ethernet destination address. */
	uint16_t dl_vlan; /* Input VLAN id. */
	uint8_t dl_vlan_pcp; /* Input VLAN priority. */
	uint8_t pad1[1]; /* Align to 64-bits */
	uint16_t dl_type; /* Ethernet frame type. */
	uint8_t nw_tos; /* IP ToS (actually DSCP field, 6 bits). */
	uint8_t nw_proto; /* IP protocol or lower 8 bits of
	* ARP opcode. */
	uint8_t pad2[2]; /* Align to 64-bits */
	uint32_t nw_src; /* IP source address. */
	uint32_t nw_dst; /* IP destination address. */
	uint16_t tp_src; /* TCP/UDP source port. */
	uint16_t tp_dst; /* TCP/UDP destination port. */
};

// 48 bytes --  required for features reply
struct ofp_phy_port {
	uint16_t port_no;
	uint8_t hw_addr[6];
	char name[16]; /* Null-terminated */
	uint32_t config; /* Bitmap of OFPPC_* flags. */
	uint32_t state; /* Bitmap of OFPPS_* flags. */
	/* Bitmaps of OFPPF_* that describe features. All bits zeroed if
	* unsupported or unavailable. */
	uint32_t curr; /* Current features. */
	uint32_t advertised; /* Features being advertised by the port. */
	uint32_t supported; /* Features supported by the port. */
	uint32_t peer; /* Features advertised by peer. */
};

// 32 bytes --  required for features reply
struct ofp_switch_features {
	struct ofp_header header;
	uint64_t datapath_id; /* Datapath unique ID. The lower 48-bits are for
	a MAC address, while the upper 16-bits are
	implementer-defined. */
	uint32_t n_buffers; /* Max packets buffered at once. */
	uint8_t n_tables; /* Number of tables supported by datapath. */
	uint8_t pad[3]; /* Align to 64-bits. */
	/* Features. */
	uint32_t capabilities; /* Bitmap of support "ofp_capabilities". */
	uint32_t actions; /* Bitmap of supported "ofp_action_type"s. */
	/* Port info.*/
	struct ofp_phy_port ports[0]; /* Port definitions. The number of ports
	is inferred from the length field in
	the header. */
};

// 12 bytes -- might not use this? not sure.
struct ofp_stats_request { // THIS IS THE WRAPPER for sending a request
	struct ofp_header header;
	uint16_t type;
	uint16_t flags;
	uint8_t body[0];
};

// 12 bytes
struct ofp_stats_reply { // THIS IS THE WRAPPER for receiving a response
	struct ofp_header header;
	uint16_t type; // Use ofp_stat_types to match, and infer how to process body
	uint16_t flags; /* OFPSF_REPLY_* flags. */
	uint8_t body[0]; /* Body of the reply. */
};

// 88 bytes -- THIS IS THE RESPONSE for receiving a flow
struct ofp_flow_stats {
	uint16_t length; /* Length of this entry. */
	uint8_t table_id; /* ID of table flow came from. */
	uint8_t pad;
	struct ofp_match match; /* Description of fields. */
	uint32_t duration_sec; /* Time flow has been alive in seconds. */
	uint32_t duration_nsec; /* Time flow has been alive in nanoseconds beyond
	duration_sec. */
	uint16_t priority; /* Priority of the entry. Only meaningful
	when this is not an exact-match entry. */
	uint16_t idle_timeout; /* Number of seconds idle before expiration. */
	uint16_t hard_timeout; /* Number of seconds before expiration. */
	uint8_t pad2[6]; /* Align to 64-bits. */
	uint64_t cookie; /* Opaque controller-issued identifier. */
	uint64_t packet_count; /* Number of packets in flow. */
	uint64_t byte_count; /* Number of bytes in flow. */
	struct ofp_action_header actions[0]; /* Actions. */
};

// 44 bytes -- // THIS IS THE BODY REQUEST for sending a flow, match all/no restrictions
struct ofp_flow_stats_request {
	struct ofp_match match; /* Fields to match. */
	uint8_t table_id;		/* ID of table to read (from ofp_table_stats),
							0xff for all tables or 0xfe for emergency. */
	uint8_t pad;			/* Align to 32 bits. */
	uint16_t out_port;		/* Require matching entries to include this
							as an output port. A value of OFPP_NONE (0xffff)
							indicates no restriction. */
};

class OpenFlowMessage {
	public:
		OpenFlowMessage(uint8_t type, uint8_t version, uint32_t xid, std::string payload);
		std::vector<uint8_t> toBytes();
		static OpenFlowMessage fromBytes(std::vector<uint8_t> bytes);
		static OpenFlowMessage helloMessage();

		// OpenFlow helper methods
		static ofp_stats_request createFlowRequest();
		static ofp_switch_features createFeaturesReply();
		static Flow parseStatsReply(ofp_flow_stats reply);
		static std::string ipToString(uint32_t ip);
		static std::string getRulePrefix(ofp_match match);

		// Parser methods
		std::vector<Flow> parse();

		// Debug methods
		std::string toString();

		// Header data
		uint8_t		version;	// 0x01 = 1.0
		uint8_t		type;		// One of the OFPT constants
		uint16_t	length;		// Length of this header
		uint32_t	xid;		// Transaction id associated with this packet
		std::string	payload;	// Payload of the message
	private:
};