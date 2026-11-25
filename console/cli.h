#pragma once

#include "params.h"
#include "selection.h"
#include "transfer.h"

#include <string>
#include <vector>

namespace console {

struct CommandLine {
    crc::SimulationParams params;
    std::string csvPath;
    std::string planPath;
    crc::TransferSettings transfer;
    double target = crc::defaultSelectionTarget;
    bool csvOutput = false;
    bool quiet = false;
    bool help = false;
    bool listCodes = false;
    bool analyze = false;
    bool select = false;
    bool allLengths = false;
    bool blocksGiven = false;
    bool seedGiven = false;
};

CommandLine parseCommandLine(const std::vector<std::string>& arguments);

std::string usage();

}
