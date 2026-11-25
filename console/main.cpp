#include "analysis_report.h"
#include "cli.h"
#include "codes.h"
#include "dialog.h"
#include "exit_codes.h"
#include "plan_run.h"
#include "report.h"
#include "results.h"
#include "selection_report.h"
#include "simulator.h"
#include "terminal.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {

using console::exitInvalidInput;
using console::exitOutputFailure;
using console::exitSuccess;

std::uint64_t percentDone(const crc::Counters& counters, const crc::ControlParams& control)
{
    const std::uint64_t byBlocks = counters.blocks * 100 / control.blockLimit;
    if (control.stopMode == crc::StopMode::BlockCount)
        return byBlocks;
    return std::max(byBlocks, counters.erroneous() * 100 / control.errorLimit);
}

crc::RunControl progressPrinter(const crc::ControlParams& control)
{
    crc::RunControl runControl;
    runControl.progressInterval = std::max<std::uint64_t>(1, control.blockLimit / 1000);
    runControl.onProgress = [control, lastShown = std::uint64_t{101}](const crc::Counters& counters) mutable {
        const std::uint64_t percent = std::min<std::uint64_t>(100, percentDone(counters, control));
        if (percent == lastShown)
            return;
        lastShown = percent;
        std::cerr << "\r  выполнено " << percent << " %" << std::flush;
    };
    return runControl;
}

crc::SimulationResult simulate(const crc::SimulationParams& params, bool showProgress)
{
    crc::Simulator simulator(params);
    if (!showProgress)
        return simulator.run();
    const crc::SimulationResult result = simulator.run(progressPrinter(params.control));
    std::cerr << "\r                    \r";
    return result;
}

int saveCsv(const std::string& path, const crc::SimulationResult& result)
{
    try {
        crc::appendCsv(path, result);
        std::cout << "Строка добавлена в " << path << '\n';
        return exitSuccess;
    } catch (const std::runtime_error& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
        return exitOutputFailure;
    }
}

void printKnownCodes()
{
    std::cout << "n;k;t;g\n";
    for (const crc::KnownCode& code : crc::knownCodes())
        std::cout << code.n << ';' << code.k << ';' << code.correctableErrors << ';' << code.binary << '\n';
}

int runInteractive()
{
    std::cout << "=== Моделирование циклического кода: режим обнаружения ошибок ===\n\n";
    const auto params = console::askParams(std::cin, std::cout);
    if (!params)
        return exitInvalidInput;
    std::cout << "\nМоделирование...\n";
    const crc::SimulationResult result = simulate(*params, true);
    console::printFirstBlock(std::cout, result);
    console::printResults(std::cout, result, params->channel);
    const auto path = console::askCsvPath(std::cin, std::cout);
    return path ? saveCsv(*path, result) : exitSuccess;
}

int runAnalysis(const crc::SimulationParams& params)
{
    crc::ChannelParams bsc;
    bsc.bsc = params.channel.bsc;
    std::vector<crc::ValidationError> errors = crc::validateCoding(params.coding);
    const std::vector<crc::ValidationError> channelErrors = crc::validateChannel(bsc);
    errors.insert(errors.end(), channelErrors.begin(), channelErrors.end());
    if (!errors.empty())
        throw std::invalid_argument(crc::joinMessages(errors));
    const crc::CodeAnalysis analysis = crc::analyzeCode(crc::makeCode(params.coding));
    console::printAnalysis(std::cout, analysis, bsc.bsc.bitErrorProbability);
    if (analysis.computable())
        return exitSuccess;
    std::cerr << "Ошибка: " << analysis.failureReason << '\n';
    return exitInvalidInput;
}

int runBatch(const console::CommandLine& commandLine)
{
    const crc::SimulationResult result = simulate(commandLine.params, !commandLine.quiet && !commandLine.csvOutput);
    if (commandLine.csvOutput)
        std::cout << crc::csvHeader() << '\n' << crc::csvRow(result) << '\n';
    else
        console::printResults(std::cout, result, commandLine.params.channel);
    if (commandLine.csvPath.empty())
        return exitSuccess;
    if (commandLine.csvOutput) {
        try {
            crc::appendCsv(commandLine.csvPath, result);
            return exitSuccess;
        } catch (const std::runtime_error& error) {
            std::cerr << "Ошибка: " << error.what() << '\n';
            return exitOutputFailure;
        }
    }
    return saveCsv(commandLine.csvPath, result);
}

}

int main(int argc, char** argv)
{
    console::prepareTerminal();
    if (argc == 1)
        return runInteractive();
    try {
        const console::CommandLine commandLine = console::parseCommandLine(std::vector<std::string>(argv + 1, argv + argc));
        if (commandLine.listCodes) {
            printKnownCodes();
            return exitSuccess;
        }
        if (commandLine.help) {
            std::cout << console::usage();
            return exitSuccess;
        }
        if (commandLine.allLengths && !commandLine.select)
            throw std::invalid_argument("--n all можно указать только вместе с --select");
        if (commandLine.select)
            return console::runSelection(commandLine);
        if (!commandLine.planPath.empty())
            return console::runPlan(commandLine);
        if (commandLine.analyze)
            return runAnalysis(commandLine.params);
        return runBatch(commandLine);
    } catch (const std::invalid_argument& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
        return exitInvalidInput;
    }
}
