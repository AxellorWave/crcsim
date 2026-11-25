#include "transfer.h"

#include "results.h"

#include <cmath>

namespace crc {

namespace {

bool positive(double value)
{
    return std::isfinite(value) && value > 0.0;
}

}

std::vector<ValidationError> validateTransfer(const TransferSettings& transfer)
{
    std::vector<ValidationError> errors;
    if (!positive(transfer.fileBits))
        errors.push_back({Field::FileSize, "размер файла M должен быть больше 0"});
    if (!positive(transfer.bitRate))
        errors.push_back({Field::BitRate, "скорость передачи должна быть больше 0"});
    return errors;
}

std::optional<double> meanAttempts(const Counters& counters)
{
    if (counters.correct == 0)
        return std::nullopt;
    return static_cast<double>(counters.blocks) / static_cast<double>(counters.correct);
}

std::optional<double> transferTime(const SimulationResult& result, const TransferSettings& transfer)
{
    const std::optional<double> attempts = meanAttempts(result.counters);
    if (!attempts || !validateTransfer(transfer).empty())
        return std::nullopt;
    return fileTransferTime(transfer.fileBits, result.code, 1.0 / transfer.bitRate, *attempts);
}

}
