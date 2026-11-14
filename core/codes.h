#pragma once

#include <string>
#include <vector>

namespace crc {

struct KnownCode {
    int n;
    int k;
    int correctableErrors;
    std::string degrees;
    std::string binary;
};

const std::vector<KnownCode>& knownCodes();

}
