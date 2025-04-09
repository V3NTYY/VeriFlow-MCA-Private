#ifndef DIGEST_H
#define DIGEST_H

#include <string>
#include <iostream>
#include <fstream>
#include "json.hpp"
#include "Node.h"
#include "Controller.h"

class Digest {
private:
    bool synch_bit;
    bool update_bit;
    bool verification_bit;
    int hostIndex;
    int destinationIndex;
    std::string payload;
    std::string destination_ip;

    void setDestinationIP(); 

public:
    Digest(bool synch = false, bool update = false, bool verification = false, 
           int hIndex = 0, int dIndex = 0, const std::string& data = "");
    ~Digest();

    std::string toJson();
    void fromJson(const std::string& json_str);

    bool sendDigest();
    static int readDigest(const std::string& raw_data);

    bool getSynchBit();
    bool getUpdateBit();
    bool getVerificationBit();
    int getHostIndex();
    int getDestinationIndex();
    std::string getPayload();
    std::string getDestinationIP();
};

#endif