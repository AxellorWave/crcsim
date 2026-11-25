#pragma once

#include "params.h"
#include "simulator.h"

#include <optional>
#include <vector>

namespace crc {

struct TransferSettings {
    double fileBits = 1000000.0;
    double bitRate = 9600.0;
};

std::vector<ValidationError> validateTransfer(const TransferSettings& transfer);

std::optional<double> meanAttempts(const Counters& counters);

std::optional<double> transferTime(const SimulationResult& result, const TransferSettings& transfer);

}
