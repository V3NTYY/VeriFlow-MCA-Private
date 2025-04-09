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
    std::vector<Node*> domainNodes = Controller::getDomainNodes();
    
    // Find the domain node that matches the destination index
    Node* destinationNode = nullptr;
    for (Node* node : domainNodes) {
        if (node->getTopologyID() == destinationIndex) {
            destinationNode = node;
            break;
        }
    }
    
    if (!destinationNode) {
        std::cerr << "Error: No domain node found" << destinationIndex << std::endl;
        return false;
    }
    
    destination_ip = destinationNode->getIP();
    
    std::string jsonData = toJson();
    
    // Create a socket and send the data
    #ifdef __unix__
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            std::cerr << "Error creating socket" << std::endl;
            return false;
        }

        struct sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(6633);
        inet_pton(AF_INET, destination_ip.c_str(), &server_address.sin_addr);

        if (connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
            std::cerr << "Error connecting to destination controller at " << destination_ip << std::endl;
            close(sockfd);
            return false;
        }

        ssize_t bytes_sent = send(sockfd, jsonData.c_str(), jsonData.size(), 0);
        if (bytes_sent < 0) {
            std::cerr << "Error sending data" << std::endl;
            close(sockfd);
            return false;
        }

        close(sockfd);
    #endif
    
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
