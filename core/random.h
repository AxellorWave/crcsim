#pragma once

#include "poly.h"

#include <cstdint>
#include <random>

namespace crc {

class Random {
public:
    explicit Random(std::uint64_t seed);

    Word nextWord();
    double nextUniform();

private:
    std::mt19937_64 engine_;
};

std::uint64_t makeRandomSeed();

}
