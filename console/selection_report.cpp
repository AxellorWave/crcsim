#include "selection_report.h"

#include "exit_codes.h"

#include "plan.h"
#include "results.h"
#include "text.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <locale>
#include <optional>
#include <sstream>
#include <vector>

namespace console {

namespace {

const std::vector<std::size_t> columnWidths = {10, 4, 7, 6, 9, 11, 12, 7, 9, 28, 0};

std::size_t displayWidth(const std::string& text)
{
    return static_cast<std::size_t>(
        std::count_if(text.begin(), text.end(), [](char symbol) { return (static_cast<unsigned char>(symbol) & 0xC0) != 0x80; }));
}

std::string padded(const std::string& text, std::size_t width)
{
    const std::size_t length = displayWidth(text);
    return length >= width ? text + ' ' : text + std::string(width - length + 1, ' ');
}

void printLine(std::ostream& output, const std::vector<std::string>& cells)
{
    std::string line;
    for (std::size_t index = 0; index < cells.size(); ++index)
        line += padded(cells[index], columnWidths[index]);
    while (!line.empty() && line.back() == ' ')
        line.pop_back();
    output << "  " << line << '\n';
}

std::string scientific(double value)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::scientific << std::setprecision(3) << value;
    return stream.str();
}

std::string optionalNumber(const std::optional<double>& value)
{
    return value ? crc::formatSignificant(*value, 4, false) : std::string("—");
}

std::string badge(std::size_t index, const crc::BestCodes& best)
{
    const bool byRedundancy = best.byRedundancy == index;
    const bool byTime = best.byTime == index;
    if (byRedundancy && byTime)
        return "лучший по I и по Tф";
    if (byRedundancy)
        return "лучший по I";
    return byTime ? "лучший по Tф" : "";
}

std::vector<std::string> rowCells(const crc::SelectionRow& row, std::size_t index, const crc::BestCodes& best)
{
    const crc::CyclicCode& code = row.result.code;
    return {crc::codeName(code), std::to_string(code.checkBitCount()), crc::formatNumber(code.redundancy(), 3),
        std::to_string(row.result.counters.undetected), std::to_string(row.result.counters.blocks),
        crc::formatProbability(row.undetected), scientific(row.upperBound), optionalNumber(row.meanAttempts),
        optionalNumber(row.transferTime), crc::verdictText(row.verdict), badge(index, best)};
}

std::string channelSummary(const crc::ChannelParams& channel)
{
    const std::string name = channel.kind == crc::ChannelKind::Bsc ? "ДСК" : "канал Гилберта";
    return name + ", " + crc::channelParametersText(channel);
}

crc::SelectionControl selectionProgress()
{
    crc::SelectionControl control;
    control.progressInterval = 100000;
    control.onProgress = [](std::size_t index, std::size_t count, const crc::Counters& counters) {
        std::cerr << "\r  код " << index + 1 << " из " << count << ": N = " << counters.blocks << "      " << std::flush;
    };
    return control;
}

}

crc::SelectionRequest selectionRequest(const CommandLine& commandLine)
{
    crc::SelectionRequest request;
    request.codeLength = commandLine.allLengths ? std::nullopt : std::optional<int>(commandLine.params.coding.n);
    request.channel = commandLine.params.channel;
    if (commandLine.blocksGiven)
        request.blocks = commandLine.params.control.blockLimit;
    if (commandLine.seedGiven)
        request.seed = commandLine.params.control.seed;
    request.target = commandLine.target;
    request.transfer = commandLine.transfer;
    return request;
}

void printSelection(std::ostream& output, const crc::SelectionRequest& request, const crc::SelectionOutcome& outcome)
{
    const std::string length = request.codeLength ? "n = " + std::to_string(*request.codeLength) : "все длины n";
    output << "Подбор кода: " << length << ", " << channelSummary(request.channel) << ", N = " << request.blocks
           << ", seed = " << request.seed << ", требование Pлож ≤ " << crc::formatNumber(request.target, 3) << '\n';
    output << "Файл M = " << crc::formatNumber(request.transfer.fileBits, 10) << " бит, скорость "
           << crc::formatNumber(request.transfer.bitRate, 10) << " бит/с\n\n";
    printLine(output, {"Код", "h", "I", "Nлож", "N", "Pлож", "верх. 95 %", "L", "Tф, с", "Вывод", ""});
    for (std::size_t index = 0; index < outcome.rows.size(); ++index)
        printLine(output, rowCells(outcome.rows[index], index, outcome.best));
    output << '\n' << crc::selectionConclusion(request, outcome.rows, outcome.best) << '\n';
}

int runSelection(const CommandLine& commandLine)
{
    const crc::SelectionRequest request = selectionRequest(commandLine);
    const crc::SelectionOutcome outcome
        = commandLine.quiet ? crc::selectCode(request) : crc::selectCode(request, selectionProgress());
    if (!commandLine.quiet)
        std::cerr << "\r                                        \r" << std::flush;
    printSelection(std::cout, request, outcome);
    return exitSuccess;
}

}
