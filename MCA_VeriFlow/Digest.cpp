#include "Digest.h"
#include "Controller.h"

// Constructor
Digest::Digest(bool synch, bool update, bool verification, 
    int hIndex, int dIndex, const std::string& data)
    : synch_bit(synch), update_bit(update), verification_bit(verification),
      hostIndex(hIndex), destinationIndex(dIndex), payload(data) { }

// Destructor
Digest::~Digest()
{
}

std::string Digest::toJson() {
    nlohmann::json j;
    std::string flowString = appendedFlow.flowToStr(false);
    if (flowString == "R#--") {
        flowString = "";
    }

    j["synch_bit"] = synch_bit ? 1 : 0;
    j["update_bit"] = update_bit ? 1 : 0;
    j["verification_bit"] = verification_bit ? 1 : 0;
    j["hostIndex"] = hostIndex;
    j["destinationIndex"] = destinationIndex;
    j["payload"] = payload;
    j["flow_data"] = flowString;
    return j.dump();
}

void Digest::fromJson(const std::string& json_str) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_str);
        std::string flow_data;

        synch_bit = j["synch_bit"].get<int>() == 1;
        update_bit = j["update_bit"].get<int>() == 1;
        verification_bit = j["verification_bit"].get<int>() == 1;
        hostIndex = j["hostIndex"].get<int>();
        destinationIndex = j["destinationIndex"].get<int>();
        payload = j["payload"].get<std::string>();
        flow_data = j["flow_data"].get<std::string>();
        appendedFlow = Flow::strToFlow(flow_data);

    } catch (const std::exception& e) {
        loggyErr("JSON parsing error: ");
        loggyErr(e.what());
        loggyErr("\n");
    }
}

int Digest::readDigest(const std::string& data) {
    try {
        nlohmann::json j = nlohmann::json::parse(data);
        
        if (!j.contains("synch_bit") || !j.contains("update_bit") || 
            !j.contains("verification_bit")) {
            return -1;
        }

        bool synch = j["synch_bit"].get<int>() == 1;
        bool update = j["update_bit"].get<int>() == 1;
        bool verification = j["verification_bit"].get<int>() == 1;

        if (synch && !update && !verification) { // 100 -- topology update (new master)
            return 0;
        } 
        else if (!synch && update && !verification) { // 010 -- synchronize topology (applying update from master)
            return 1;
        }
        else if (!synch  && !update && verification) { // 001 -- perform verification
            return 2;
        }
        else if (!synch && update && verification) { // 011 -- verification success
            return 3;
        }
        else if (synch && update && verification) { // 111 -- verification fail
            return 4;
        }
        else if (!synch && !update && !verification) { // 000 -- requesting flow list from payload
            return 5;
        }
        else if (synch && update && !verification) { // 110 -- flow list attached to payload
            return 6;
        }
        else {
            return -1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Digest parsing error: " << e.what() << std::endl;
        return -1;
    }
}

Flow Digest::getFlow(const std::string &raw_data)
{
    nlohmann::json j = nlohmann::json::parse(raw_data);
    std::string parsedFlow = j["flow_data"].get<std::string>();

    return Flow::strToFlow(parsedFlow);
}

void Digest::appendFlow(Flow f)
{
    appendedFlow = f;
}

Flow Digest::getFlow() {
	return appendedFlow;
}

std::vector<Node> Digest::getTopology(std::string message)
{
    // Parse payload string from msg
    nlohmann::json j = nlohmann::json::parse(message);
    std::string parsedTop = j["payload"].get<std::string>();

    if (parsedTop.empty()) {
        return std::vector<Node>();
    }

    return Topology::string_toTopology(parsedTop);
}

bool Digest::getSynchBit()
{
    return synch_bit;
}

bool Digest::getUpdateBit() { 
    return update_bit;
}

bool Digest::getVerificationBit() { 
    return verification_bit; 
}

int Digest::getHostIndex() { 
    return hostIndex; 
}

int Digest::getDestinationIndex() {
	return destinationIndex; 
}

std::string Digest::getPayload() { 
    return payload; 
}

std::string Digest::getDestinationIP() { 
    return destination_ip; 
}
