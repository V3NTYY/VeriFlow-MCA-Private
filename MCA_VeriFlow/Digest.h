#ifndef DIGEST_H
#define DIGEST_H

#include <string>
#include <iostream>
#include <fstream>
#include "json.hpp"
#include "Node.h"
#include "Flow.h"
#include "Log.h"

class Digest {
private:
    bool synch_bit;
    bool update_bit;
    bool verification_bit;
    int hostIndex;
    int destinationIndex;
    std::string payload;
    std::string destination_ip;
    Flow appendedFlow;

public:
    Digest(bool synch = false, bool update = false, bool verification = false, 
           int hIndex = 0, int dIndex = 0, const std::string& data = "");
    ~Digest();

    // JSON Marshalling methods
    std::string toJson();
    void fromJson(const std::string& json_str);

    // Digest methods
    static int readDigest(const std::string& raw_data);
    static Flow getFlow(const std::string& raw_data);

    // Flow methods
    void appendFlow(Flow f);
    Flow getFlow();

    // Misc/getters
    bool getSynchBit();
    bool getUpdateBit();
    bool getVerificationBit();
    int getHostIndex();
    int getDestinationIndex();
    std::string getPayload();
    std::string getDestinationIP();
};

#endif