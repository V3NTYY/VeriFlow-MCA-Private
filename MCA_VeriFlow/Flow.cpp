#include "Flow.h"

#ifdef __unix__
	std::string Flow::toJSON(bool create_file, std::string file_dir) {
		// TODO: Create a JSON object given this flows information (make sure it matches the sample JSON)
		// Output it to a .json file if create_file is true to the file_dir location
		nlohmann::json j;
		
		j["flow_info"]["flow_id"] = flow_id;
		j["flow_info"]["flow_table"]["match_fields"]["eth_type"] = eth_type;
		j["flow_info"]["flow_table"]["match_fields"]["ip_proto"] = protocol;
		j["flow_info"]["flow_table"]["match_fields"]["ipv4_src"] = srcIP;
		j["flow_info"]["flow_table"]["match_fields"]["ipv4_dst"] = dstIP;
		j["flow_info"]["flow_table"]["match_fields"]["tcp_src"] = std::stoi(srcPort);
		j["flow_info"]["flow_table"]["match_fields"]["tcp_dst"] = std::stoi(dstPort);
		j["flow_info"]["flow_table"]["actions"] = {action}; 

		std::string json_str = j.dump(4);  // Pretty-print with 4-space indentation

		if (create_file) {
			std::ofstream file(file_dir);
			if (file.is_open()) {
				file << json_str;
				file.close();
			}
		}

		return json_str;
	}
#endif

#ifdef __unix__
	static Flow* fromJSON(std::string json) {
		// TODO: Create a Flow object from parsing a given JSON object (make sure it matches the sample JSON)
		try {
			nlohmann::json j = nlohmann::json::parse(json);
			
			std::string flow_id = j["flow_info"]["flow_id"];
			std::string eth_type = j["flow_info"]["flow_table"]["match_fields"]["eth_type"];
			std::string protocol = j["flow_info"]["flow_table"]["match_fields"]["ip_proto"];
			std::string srcIP = j["flow_info"]["flow_table"]["match_fields"]["ipv4_src"];
			std::string dstIP = j["flow_info"]["flow_table"]["match_fields"]["ipv4_dst"];
			std::string srcPort = std::to_string(j["flow_info"]["flow_table"]["match_fields"]["tcp_src"].get<int>());
			std::string dstPort = std::to_string(j["flow_info"]["flow_table"]["match_fields"]["tcp_dst"].get<int>());
			std::string action = j["flow_info"]["flow_table"]["actions"][0];

			return new Flow(flow_id, eth_type, protocol, srcIP, dstIP, srcPort, dstPort, action);
		} catch (nlohmann::json::exception& e) {
			std::cerr << "JSON Parsing Error: " << e.what() << std::endl;
			return nullptr;
		}
		return nullptr;
	}
#endif

Flow::Flow(std::string Flow_id, std::string Eth_type, std::string Protocol,
	std::string SrcIP, std::string DstIP, std::string SrcPort,
	std::string DstPort, std::string Action)
{
	flow_id =	Flow_id;
	eth_type =	Eth_type;
	protocol =	Protocol;
	srcIP =		SrcIP;
	dstIP =		DstIP;
	srcPort =	SrcPort;
	dstPort =	DstPort;
	action =	Action;
}

Flow::~Flow()
{
}

std::string Flow::getFlowID()
{
	return flow_id;
}

std::string Flow::getEthType()
{
	return eth_type;
}

std::string Flow::getProtocol()
{
	return protocol;
}

std::string Flow::getSrcIP()
{
	return srcIP;
}

std::string Flow::getDstIP()
{
	return dstIP;
}

std::string Flow::getSrcPort()
{
	return srcPort;
}

std::string Flow::getDstPort()
{
	return dstPort;
}

std::string Flow::getAction()
{
	return action;
}
