#include <string>

class Flow {
	public:

		// Constructors & destructors
		Flow(std::string flow_id, std::string eth_type, std::string protocol, 
			std::string srcIP, std::string dstIP, std::string srcPort, 
			std::string dstPort, std::string action);
		~Flow();

		// JSON Marshalling method
		std::string toJSON(bool create_file, std::string file_dir);

		// JSON Unmarshalling method
		static Flow* fromJSON(std::string json);

		// Getters
		std::string getFlowID();
		std::string getEthType();
		std::string getProtocol();
		std::string getSrcIP();
		std::string getDstIP();
		std::string getSrcPort();
		std::string getDstPort();
		std::string getAction();

	private:
		std::string flow_id;
		std::string eth_type;
		std::string protocol;
		std::string srcIP;
		std::string dstIP;
		std::string srcPort;
		std::string dstPort;
		std::string action;
};