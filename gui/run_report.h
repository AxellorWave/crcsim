#pragma once

#include "params.h"
#include "simulator.h"

#include <QString>

QString countersText(const crc::Counters& counters);

int progressPermille(const crc::Counters& counters, const crc::ControlParams& control);

QString runSummaryText(const crc::SimulationResult& result);
