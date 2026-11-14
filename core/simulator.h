#pragma once

#include "channel.h"
#include "code.h"
#include "params.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace crc {

struct Counters {
    std::uint64_t blocks = 0;
    std::uint64_t correct = 0;
    std::uint64_t detected = 0;
    std::uint64_t undetected = 0;
    std::uint64_t corruptedBits = 0;
    std::vector<std::uint64_t> errorWeights;

    std::uint64_t erroneous() const { return detected + undetected; }
};

struct BlockTrace {
    Word data = 0;
    Word check = 0;
    Word codeword = 0;
    Word errors = 0;
    Word received = 0;
    Word syndrome = 0;
};

struct SimulationResult {
    CyclicCode code;
    std::string channelDisplayName;
    std::string channelCsvName;
    double meanBitErrorProbability = 0.0;
    std::uint64_t seed = 0;
    Counters counters;
    BlockTrace firstBlock;
    bool cancelled = false;
};

using ProgressCallback = std::function<void(const Counters&)>;

struct RunControl {
    ProgressCallback onProgress;
    const std::atomic<bool>* cancelRequested = nullptr;
    std::uint64_t progressInterval = 65536;
};

class Simulator {
public:
    explicit Simulator(const SimulationParams& params);
    Simulator(const CyclicCode& code, std::unique_ptr<Channel> channel, const ControlParams& control);

    SimulationResult run(const RunControl& runControl = {});

private:
    bool limitReached(const Counters& counters) const;

    CyclicCode code_;
    std::unique_ptr<Channel> channel_;
    ControlParams control_;
};

std::string joinMessages(const std::vector<ValidationError>& errors);

}
