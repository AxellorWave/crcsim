#include "simulator.h"

#include "decoder.h"
#include "encoder.h"

#include <bitset>
#include <stdexcept>

namespace crc {

namespace {

const SimulationParams& requireValid(const SimulationParams& params)
{
    const std::vector<ValidationError> errors = validate(params);
    if (!errors.empty())
        throw std::invalid_argument(joinMessages(errors));
    return params;
}

int weight(Word errors)
{
    return static_cast<int>(std::bitset<maxWordBits>(errors).count());
}

void classify(Counters& counters, Word errors, Word blockSyndrome)
{
    ++counters.blocks;
    const int errorWeight = weight(errors);
    counters.corruptedBits += static_cast<std::uint64_t>(errorWeight);
    ++counters.errorWeights[static_cast<std::size_t>(errorWeight)];
    if (errors == 0)
        ++counters.correct;
    else if (blockSyndrome != 0)
        ++counters.detected;
    else
        ++counters.undetected;
}

bool cancelRequested(const RunControl& runControl)
{
    return runControl.cancelRequested != nullptr && runControl.cancelRequested->load();
}

}

std::string joinMessages(const std::vector<ValidationError>& errors)
{
    std::string text;
    for (const ValidationError& error : errors) {
        if (!text.empty())
            text += "; ";
        text += error.message;
    }
    return text;
}

Simulator::Simulator(const SimulationParams& params)
    : Simulator(makeCode(requireValid(params).coding), makeChannel(params.channel), params.control)
{
}

Simulator::Simulator(const CyclicCode& code, std::unique_ptr<Channel> channel, const ControlParams& control)
    : code_(code)
    , channel_(std::move(channel))
    , control_(control)
{
}

bool Simulator::limitReached(const Counters& counters) const
{
    if (counters.blocks >= control_.blockLimit)
        return true;
    return control_.stopMode == StopMode::ErrorCount && counters.erroneous() >= control_.errorLimit;
}

SimulationResult Simulator::run(const RunControl& runControl)
{
    SimulationResult result;
    result.code = code_;
    result.channelDisplayName = channel_->displayName();
    result.channelCsvName = channel_->csvName();
    result.meanBitErrorProbability = channel_->meanBitErrorProbability();
    result.seed = control_.seed != 0 ? control_.seed : makeRandomSeed();

    Counters& counters = result.counters;
    counters.errorWeights.assign(static_cast<std::size_t>(code_.n) + 1, 0);
    const Word dataMask = lowBitsMask(code_.dataBitCount());

    Random random(result.seed);
    channel_->start(random);
    while (!limitReached(counters)) {
        const Word data = random.nextWord() & dataMask;
        const Word codeword = encode(code_, data);
        const Word errors = channel_->errorVector(random, code_.n);
        const Word received = codeword ^ errors;
        const Word blockSyndrome = syndrome(code_, received);
        if (counters.blocks == 0)
            result.firstBlock = {data, checkBits(code_, data), codeword, errors, received, blockSyndrome};
        classify(counters, errors, blockSyndrome);

        if (counters.blocks % runControl.progressInterval == 0) {
            if (runControl.onProgress)
                runControl.onProgress(counters);
            if (cancelRequested(runControl)) {
                result.cancelled = true;
                break;
            }
        }
    }
    if (runControl.onProgress)
        runControl.onProgress(counters);
    return result;
}

}
