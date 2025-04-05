#ifndef DIGEST_H
#define DIGEST_H

#include <string>
#include <iostream>
#include <fstream>
#include "json.hpp"

class Digest {
private:
    bool synch_bit;
    bool update_bit;
    bool verification_bit;
    int index;
    std::string payload;
    std::string destination_ip;

    void setDestinationIP(); 

public:
    Digest(bool synch = false, bool update = false, bool verification = false, 
           int idx = 0, const std::string& data = "");
    ~Digest();

    std::string toJson();
    void fromJson(const std::string& json_str);

    bool sendDigest();
    int readDigest(const std::string& raw_data);

    bool getSynchBit();
    bool getUpdateBit();
    bool getVerificationBit();
    int getIndex();
    std::string getPayload();
    std::string getDestinationIP();
};

#endif