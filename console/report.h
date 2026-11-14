#pragma once

#include "params.h"
#include "simulator.h"

#include <iosfwd>

namespace console {

void printChannelCharacteristics(std::ostream& output, const crc::ChannelParams& channel);

void printFirstBlock(std::ostream& output, const crc::SimulationResult& result);

void printResults(std::ostream& output, const crc::SimulationResult& result, const crc::ChannelParams& channel);

}
