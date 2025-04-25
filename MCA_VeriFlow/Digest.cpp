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
    j["synch_bit"] = synch_bit ? 1 : 0;
    j["update_bit"] = update_bit ? 1 : 0;
    j["verification_bit"] = verification_bit ? 1 : 0;
    j["hostIndex"] = hostIndex;
    j["destinationIndex"] = destinationIndex;
    j["payload"] = payload;
    j["destination_ip"] = destination_ip;
    j["flow_data"] = appendedFlow.flowToStr(false);
    return j.dump();
}

void Digest::fromJson(const std::string& json_str) {
    try {
        loggy << "Parsing JSON: " << json_str << std::endl;
        nlohmann::json j = nlohmann::json::parse(json_str);
        std::string flow_data;

        synch_bit = j["synch_bit"].get<int>() == 1;
        update_bit = j["update_bit"].get<int>() == 1;
        verification_bit = j["verification_bit"].get<int>() == 1;
        hostIndex = j["hostIndex"].get<int>();
        destinationIndex = j["destinationIndex"].get<int>();
        payload = j["payload"].get<std::string>();
        destination_ip = j["destination_ip"].get<std::string>();
        flow_data = j["flow_data"].get<std::string>();
        appendedFlow = *Flow::strToFlow(flow_data);

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

        if (synch && !update && !verification) {
            return 0;
        } 
        else if (update && !synch && !verification) {
            return 1;
        }
        else if (verification && !synch && !update) {
            return 2;
        }
        else if (verification && update && !synch) {
            return 3;
        }
        else if (synch && verification && update) {
            return 4;
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

    Flow f = *Flow::strToFlow(parsedFlow);
    Flow empty;

    // If we couldn't parse anything, return an empty flow object
    if (f == empty) {
        return Flow("", "", "", false);
    }

    return f;
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
