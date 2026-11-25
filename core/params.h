#pragma once

#include "channel.h"
#include "code.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace crc {

constexpr int minCodeLength = 3;
constexpr int maxCodeLength = 64;
constexpr std::uint64_t maxBlockLimit = 1000000000;

enum class ChannelKind { Bsc, Gilbert };

enum class StopMode { BlockCount, ErrorCount };

struct CodingParams {
    int n = 15;
    std::string generator = "10011";
};

struct ChannelParams {
    ChannelKind kind = ChannelKind::Bsc;
    BscSettings bsc;
    GilbertSettings gilbert;
};

struct ControlParams {
    StopMode stopMode = StopMode::BlockCount;
    std::uint64_t blockLimit = 1000000;
    std::uint64_t errorLimit = 1000;
    std::uint64_t seed = 0;
};

struct SimulationParams {
    CodingParams coding;
    ChannelParams channel;
    ControlParams control;
};

enum class Field {
    CodeLength,
    Generator,
    BitErrorProbability,
    GoodToBad,
    BadToGood,
    ErrorInBad,
    ErrorInGood,
    BlockLimit,
    ErrorLimit,
    FileSize,
    BitRate,
    Target,
};

struct ValidationError {
    Field field;
    std::string message;
};

std::vector<ValidationError> validateCoding(const CodingParams& coding);
std::vector<ValidationError> validateChannel(const ChannelParams& channel);
std::vector<ValidationError> validateControl(const ControlParams& control);
std::vector<ValidationError> validate(const SimulationParams& params);

CyclicCode makeCode(const CodingParams& coding);
std::unique_ptr<Channel> makeChannel(const ChannelParams& channel);
double meanBitErrorProbability(const ChannelParams& channel);

}
