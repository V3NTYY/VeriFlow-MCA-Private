#ifndef OPENFLOWMESSAGE_H
#define OPENFLOWMESSAGE_H

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>
#include "Flow.h"
#include "Log.h"
#ifdef __unix__
#include <arpa/inet.h>
#endif

// OpenFlow header includes
#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

// Define CCPDN identifier
#define CCPDN_IDENTIFIER 0x1CCC56BA9ABCDEF0

// OpenFlow macros
#ifdef SWIG
#define OFP_ASSERT(EXPR)        /* SWIG can't handle OFP_ASSERT. */
#elif !defined(__cplusplus)
/* Build-time assertion for use in a declaration context. */
#define OFP_ASSERT(EXPR)                                                \
        extern int (*build_assert(void))[ sizeof(struct {               \
                    unsigned int build_assert_failed : (EXPR) ? 1 : -1; })]
#else /* __cplusplus */
#define OFP_ASSERT(_EXPR) typedef int build_assert_failed[(_EXPR) ? 1 : -1]
#endif /* __cplusplus */

#ifndef SWIG
#define OFP_PACKED __attribute__((packed))
#else
#define OFP_PACKED              /* SWIG doesn't understand __attribute. */
#endif

// OpenFlow enum pulled from https://opennetworking.org/wp-content/uploads/2013/04/openflow-spec-v1.0.0.pdf
enum ofp_type {
	/* Immutable messages. */
	OFPT_HELLO, /* Symmetric message */
	OFPT_ERROR, /* Symmetric message */
	OFPT_ECHO_REQUEST, /* Symmetric message */
	OFPT_ECHO_REPLY, /* Symmetric message */
	OFPT_VENDOR, /* Symmetric message */
	/* Switch configuration messages. */
	OFPT_FEATURES_REQUEST, /* Controller/switch message */
	OFPT_FEATURES_REPLY, /* Controller/switch message */
	OFPT_GET_CONFIG_REQUEST, /* Controller/switch message */
	OFPT_GET_CONFIG_REPLY, /* Controller/switch message */
	OFPT_SET_CONFIG, /* Controller/switch message */
	/* Asynchronous messages. */
	OFPT_PACKET_IN, /* Async message */
	OFPT_FLOW_REMOVED, /* Async message */
	OFPT_PORT_STATUS, /* Async message */
	/* Controller command messages. */
	OFPT_PACKET_OUT, /* Controller/switch message */
	OFPT_FLOW_MOD, /* Controller/switch message */
	OFPT_PORT_MOD, /* Controller/switch message */
	/* Statistics messages. */
	OFPT_STATS_REQUEST, /* Controller/switch message */
	OFPT_STATS_REPLY, /* Controller/switch message */
	/* Barrier messages. */
	OFPT_BARRIER_REQUEST, /* Controller/switch message */
	OFPT_BARRIER_REPLY, /* Controller/switch message */
	/* Queue Configuration messages. */
	OFPT_QUEUE_GET_CONFIG_REQUEST, /* Controller/switch message */
	OFPT_QUEUE_GET_CONFIG_REPLY /* Controller/switch message */
};

enum ofp_flow_mod_command {
	OFPFC_ADD, /* New flow. */
	OFPFC_MODIFY, /* Modify all matching flows. */
	OFPFC_MODIFY_STRICT, /* Modify entry strictly matching wildcards */
	OFPFC_DELETE, /* Delete all matching flows. */
	OFPFC_DELETE_STRICT /* Strictly match wildcards and priority. */
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

// 8 bytes -- // HEADER FIELD, ALWAYS HERE
struct ofp_header {
	uint8_t version;
	uint8_t type;
	uint16_t length;
	uint32_t xid;
};
OFP_ASSERT(sizeof(struct ofp_header) == 8);

// 8 bytes -- // UNUSED but required field
struct ofp_action_header { // Unused but required field
	uint16_t type; /* One of OFPAT_*. */
	uint16_t len; /* Length of action, including this
					header. This is the length of action,
					including any padding to make it
					64-bit aligned. */
	uint16_t port; /* Port number. */
	uint8_t pad[4];
};
OFP_ASSERT(sizeof(struct ofp_action_header) == 10);

// ADD OFP_ACTION_OUTPUT struct

// 40 bytes -- // MATCH STRUCT -- REQUIRED FOR FLOWS
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
OFP_ASSERT(sizeof(struct ofp_match) == 40);

// 48 bytes --  PHY-PORT -- REQUIRED FOR FEATURES REPLY
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
OFP_ASSERT(sizeof(struct ofp_phy_port) == 48);

// 32 bytes --  FEATURES REPLY -- REQUIRED
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
OFP_ASSERT(sizeof(struct ofp_switch_features) == 32);

// 12 bytes + variable length bytes
struct ofp_stats_request { // WRAPPER of message to request stats
	struct ofp_header header;
	uint16_t type;
	uint16_t flags;
	uint8_t body[0];
};
OFP_ASSERT(sizeof(struct ofp_stats_request) == 12);

// 44 bytes -- // BODY of message to request flow stats
struct ofp_flow_stats_request {
	struct ofp_match match; /* Fields to match. */
	uint8_t table_id;		/* ID of table to read (from ofp_table_stats),
							0xff for all tables or 0xfe for emergency. */
	uint8_t pad;			/* Align to 32 bits. */
	uint16_t out_port;		/* Require matching entries to include this
							as an output port. A value of OFPP_NONE (0xffff)
							indicates no restriction. */
};
OFP_ASSERT(sizeof(struct ofp_flow_stats_request) == 44);

// 12 + variable length bytes
struct ofp_stats_reply { // WRAPPER of message containing stats reply
	struct ofp_header header;
	uint16_t type; // Use ofp_stat_types to match, and infer how to process body
	uint16_t flags; /* OFPSF_REPLY_* flags. */
	uint8_t body[0]; /* Body of the reply. */
};
OFP_ASSERT(sizeof(struct ofp_stats_reply) == 12);

// 88 bytes -- // BODY of message containing flow stats reply
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
OFP_ASSERT(sizeof(struct ofp_flow_stats) == 88);

// 72 bytes
struct ofp_flow_mod {
    struct ofp_header header;
    struct ofp_match match;      /* Fields to match */
    uint64_t cookie;             /* Opaque controller-issued identifier. */

    /* Flow actions. */
    uint16_t command;             /* One of OFPFC_*. */
    uint16_t idle_timeout;        /* Idle time before discarding (seconds). */
    uint16_t hard_timeout;        /* Max time before discarding (seconds). */
    uint16_t priority;            /* Priority level of flow entry. */
    uint32_t buffer_id;           /* Buffered packet to apply to (or -1).
                                     Not meaningful for OFPFC_DELETE*. */
    uint16_t out_port;            /* For OFPFC_DELETE* commands, require
                                     matching entries to include this as an
                                     output port.  A value of OFPP_NONE
                                     indicates no restriction. */
    uint16_t flags;               /* One of OFPFF_*. */
    struct ofp_action_header actions[0]; /* The action length is inferred
                                            from the length field in the
                                            header. */
};
OFP_ASSERT(sizeof(struct ofp_flow_mod) == 72);

struct ofp_flow_removed {
    struct ofp_header header;
    struct ofp_match match;   /* Description of fields. */
    uint64_t cookie;          /* Opaque controller-issued identifier. */

    uint16_t priority;        /* Priority level of flow entry. */
    uint8_t reason;           /* One of OFPRR_*. */
    uint8_t pad[1];           /* Align to 32-bits. */

    uint32_t duration_sec;    /* Time flow was alive in seconds. */
    uint32_t duration_nsec;   /* Time flow was alive in nanoseconds beyond
                                 duration_sec. */
    uint16_t idle_timeout;    /* Idle timeout from original flow mod. */
    uint8_t pad2[2];          /* Align to 64-bits. */
    uint64_t packet_count;
    uint64_t byte_count;
};
OFP_ASSERT(sizeof(struct ofp_flow_removed) == 88);

#define DESC_STR_LEN 256
#define SERIAL_NUM_LEN 32
struct ofp_desc_stats {
	char mfr_desc[DESC_STR_LEN]; /* Manufacturer description. */
	char hw_desc[DESC_STR_LEN]; /* Hardware description. */
	char sw_desc[DESC_STR_LEN]; /* Software description. */
	char serial_num[SERIAL_NUM_LEN]; /* Serial number. */
	char dp_desc[DESC_STR_LEN]; /* Human readable description of datapath. */
};
OFP_ASSERT(sizeof(struct ofp_desc_stats) == 1056);

class OpenFlowMessage {
	public:
		// Message creation -- all XIDS are expected to be passed in as host-endian order
		static std::vector<unsigned char> createHello(uint32_t XID);
		static std::vector<unsigned char> createFlowRequest();
		static std::vector<unsigned char> createFeaturesReply(uint32_t XID);
		static std::vector<unsigned char> createDescStatsReply(uint32_t XID);
		static std::vector<unsigned char> createFlowStatsReply(uint32_t XID);
		static std::vector<unsigned char> createBarrierReply(uint32_t XID);
		static std::vector<unsigned char> createFlowAdd(Flow f, uint32_t XID);
		static std::vector<unsigned char> createFlowRemove(Flow f, uint32_t XID);

		// Helper methods
		static std::string ipToString(uint32_t ip);
		static std::string getRulePrefix(uint32_t wildcards, uint32_t srcIP);
		
	private:
};

#endif