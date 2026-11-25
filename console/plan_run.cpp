#include "plan_run.h"

#include "exit_codes.h"

#include "plan.h"
#include "results.h"
#include "simulator.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace console {

namespace {

crc::RunControl planProgress(std::size_t index, std::size_t count, const crc::ControlParams& control)
{
    crc::RunControl runControl;
    runControl.progressInterval = std::max<std::uint64_t>(1, control.blockLimit / 100);
    runControl.onProgress = [index, count, limit = control.blockLimit](const crc::Counters& counters) {
        const std::uint64_t percent = std::min<std::uint64_t>(100, counters.blocks * 100 / limit);
        std::cerr << "\r  опыт " << index + 1 << " из " << count << ": " << percent << " %   " << std::flush;
    };
    return runControl;
}

crc::SimulationResult runExperiment(const crc::PlannedExperiment& experiment, std::size_t index, std::size_t count, bool quiet)
{
    crc::Simulator simulator(experiment.params);
    if (quiet)
        return simulator.run();
    return simulator.run(planProgress(index, count, experiment.params.control));
}

void clearProgress(bool quiet)
{
    if (!quiet)
        std::cerr << "\r                              \r" << std::flush;
}

}

int runPlan(const CommandLine& commandLine)
{
    crc::ExperimentPlan plan;
    try {
        plan = crc::loadPlan(commandLine.planPath);
    } catch (const std::runtime_error& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
        return exitInvalidInput;
    }
    std::cout << crc::csvHeader() << '\n' << std::flush;
    for (std::size_t index = 0; index < plan.size(); ++index) {
        const crc::SimulationResult result = runExperiment(plan[index], index, plan.size(), commandLine.quiet);
        clearProgress(commandLine.quiet);
        std::cout << crc::csvRow(result) << '\n' << std::flush;
        if (commandLine.csvPath.empty())
            continue;
        try {
            crc::appendCsv(commandLine.csvPath, result);
        } catch (const std::runtime_error& error) {
            std::cerr << "Ошибка: " << error.what() << '\n';
            return exitOutputFailure;
        }
    }
    return exitSuccess;
}

}
