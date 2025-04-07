#include "Digest.h"

// Constructor
Digest::Digest(bool synch, bool update, bool verification, 
    int hIndex, int dIndex, const std::string& data)
    : synch_bit(synch), update_bit(update), verification_bit(verification),
      hostIndex(hIndex), destinationIndex(dIndex), payload(data) {
    setDestinationIP();
}

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
    return j.dump();
}

void Digest::fromJson(const std::string& json_str) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_str);
        synch_bit = j["synch_bit"].get<int>() == 1;
        update_bit = j["update_bit"].get<int>() == 1;
        verification_bit = j["verification_bit"].get<int>() == 1;
        hostIndex = j["hostIndex"].get<int>();
        destinationIndex = j["destinationIndex"].get<int>();
        payload = j["payload"].get<std::string>();
        destination_ip = j["destination_ip"].get<std::string>();
    } catch (const std::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
    }
}

void Digest::setDestinationIP() {
}

bool Digest::sendDigest() {
    return true;
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

bool Digest::getSynchBit() { 
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
