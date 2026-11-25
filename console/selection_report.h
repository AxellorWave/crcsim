#pragma once

#include "cli.h"

#include "selection.h"

#include <ostream>

namespace console {

crc::SelectionRequest selectionRequest(const CommandLine& commandLine);

void printSelection(std::ostream& output, const crc::SelectionRequest& request, const crc::SelectionOutcome& outcome);

int runSelection(const CommandLine& commandLine);

}
