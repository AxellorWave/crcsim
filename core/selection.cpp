#include "selection.h"

#include "codes.h"
#include "plan.h"
#include "text.h"

#include <algorithm>
#include <stdexcept>

namespace crc {

namespace {

constexpr double boundTolerance = 1e-9;

bool satisfiedBy(double upperBound, double target)
{
    return upperBound <= target * (1.0 + boundTolerance);
}

bool betterByRedundancy(const SelectionRow& candidate, const SelectionRow& current)
{
    const double candidateRedundancy = candidate.result.code.redundancy();
    const double currentRedundancy = current.result.code.redundancy();
    if (candidateRedundancy != currentRedundancy)
        return candidateRedundancy < currentRedundancy;
    if (!candidate.transferTime || !current.transferTime)
        return candidate.transferTime.has_value() && !current.transferTime.has_value();
    return *candidate.transferTime < *current.transferTime;
}

bool betterByTime(const SelectionRow& candidate, const SelectionRow& current)
{
    if (*candidate.transferTime != *current.transferTime)
        return *candidate.transferTime < *current.transferTime;
    return candidate.result.code.redundancy() < current.result.code.redundancy();
}

std::optional<std::size_t> bestSatisfied(const std::vector<SelectionRow>& rows,
    const std::function<bool(const SelectionRow&)>& eligible,
    const std::function<bool(const SelectionRow&, const SelectionRow&)>& better)
{
    std::optional<std::size_t> best;
    for (std::size_t index = 0; index < rows.size(); ++index) {
        const SelectionRow& row = rows[index];
        if (row.verdict != RequirementVerdict::Satisfied || !eligible(row))
            continue;
        if (!best || better(row, rows[*best]))
            best = index;
    }
    return best;
}

std::string lengthText(const SelectionRequest& request)
{
    return request.codeLength ? "n = " + std::to_string(*request.codeLength) : std::string("всех длин n");
}

std::string channelText(const ChannelParams& channel)
{
    if (channel.kind == ChannelKind::Bsc)
        return "ДСК p = " + formatWithComma(channel.bsc.bitErrorProbability, 4);
    const GilbertSettings& gilbert = channel.gilbert;
    return "канале Гилберта P_ХП = " + formatWithComma(gilbert.goodToBad, 4) + ", P_ПХ = "
        + formatWithComma(gilbert.badToGood, 4) + ", p_П = " + formatWithComma(gilbert.errorInBad, 4)
        + ", p_Х = " + formatWithComma(gilbert.errorInGood, 4);
}

std::string redundancyPart(const SelectionRow& row)
{
    return codeName(row.result.code) + ", I = " + formatWithComma(row.result.code.redundancy(), 3);
}

std::string timePart(const std::vector<SelectionRow>& rows, const BestCodes& best)
{
    if (!best.byTime)
        return "не определён (Nв = 0, Tф бесконечно)";
    const SelectionRow& row = rows[*best.byTime];
    return codeName(row.result.code) + ", Tф = " + formatSignificant(*row.transferTime, 4, true) + " с";
}

bool hasBorderline(const std::vector<SelectionRow>& rows)
{
    return std::any_of(rows.begin(), rows.end(), [](const SelectionRow& row) { return row.verdict == RequirementVerdict::Borderline; });
}

}

std::vector<ValidationError> validateSelection(const SelectionRequest& request)
{
    std::vector<ValidationError> errors = validateChannel(request.channel);
    ControlParams control;
    control.blockLimit = request.blocks;
    const std::vector<ValidationError> controlErrors = validateControl(control);
    errors.insert(errors.end(), controlErrors.begin(), controlErrors.end());
    if (!(request.target > 0.0 && request.target < 1.0))
        errors.push_back({Field::Target, "требование Pлож должно быть в интервале (0; 1)"});
    const std::vector<ValidationError> transferErrors = validateTransfer(request.transfer);
    errors.insert(errors.end(), transferErrors.begin(), transferErrors.end());
    if (candidateCodes(request.codeLength).empty())
        errors.push_back({Field::CodeLength, "нет известных кодов длины n = " + std::to_string(*request.codeLength)});
    return errors;
}

std::vector<CyclicCode> candidateCodes(std::optional<int> codeLength)
{
    std::vector<CyclicCode> codes;
    for (const KnownCode& known : knownCodes()) {
        if (!codeLength || known.n == *codeLength)
            codes.push_back(CyclicCode{known.n, parsePolynomial(known.binary)});
    }
    return codes;
}

std::vector<int> knownCodeLengths()
{
    std::vector<int> lengths;
    for (const KnownCode& known : knownCodes()) {
        if (std::find(lengths.begin(), lengths.end(), known.n) == lengths.end())
            lengths.push_back(known.n);
    }
    return lengths;
}

SimulationParams candidateParams(const CyclicCode& code, const SelectionRequest& request)
{
    SimulationParams params;
    params.coding = CodingParams{code.n, toBinary(code.generator)};
    params.channel = request.channel;
    params.control.blockLimit = request.blocks;
    params.control.seed = request.seed;
    return params;
}

RequirementVerdict judge(const ProbabilityEstimate& undetected, double target)
{
    if (satisfiedBy(undetected.upperBound95(), target))
        return RequirementVerdict::Satisfied;
    if (undetected.value() > target)
        return RequirementVerdict::NotSatisfied;
    return RequirementVerdict::Borderline;
}

SelectionRow evaluateCandidate(const SimulationResult& result, const SelectionRequest& request)
{
    SelectionRow row;
    row.result = result;
    row.undetected = {result.counters.undetected, result.counters.blocks};
    row.upperBound = row.undetected.upperBound95();
    row.verdict = judge(row.undetected, request.target);
    row.meanAttempts = meanAttempts(result.counters);
    row.transferTime = transferTime(result, request.transfer);
    return row;
}

BestCodes chooseBest(const std::vector<SelectionRow>& rows)
{
    BestCodes best;
    best.byRedundancy = bestSatisfied(rows, [](const SelectionRow&) { return true; }, betterByRedundancy);
    best.byTime = bestSatisfied(rows, [](const SelectionRow& row) { return row.transferTime.has_value(); }, betterByTime);
    return best;
}

SelectionOutcome selectCode(const SelectionRequest& request, const SelectionControl& control)
{
    const std::vector<ValidationError> errors = validateSelection(request);
    if (!errors.empty())
        throw std::invalid_argument(joinMessages(errors));
    const std::vector<CyclicCode> codes = candidateCodes(request.codeLength);
    SelectionOutcome outcome;
    for (std::size_t index = 0; index < codes.size(); ++index) {
        RunControl runControl;
        runControl.cancelRequested = control.cancelRequested;
        runControl.progressInterval = control.progressInterval;
        if (control.onProgress) {
            runControl.onProgress = [&control, index, count = codes.size()](const Counters& counters) {
                control.onProgress(index, count, counters);
            };
        }
        const SimulationResult result = Simulator(candidateParams(codes[index], request)).run(runControl);
        if (result.cancelled) {
            outcome.cancelled = true;
            break;
        }
        outcome.rows.push_back(evaluateCandidate(result, request));
    }
    outcome.best = chooseBest(outcome.rows);
    return outcome;
}

std::string verdictText(RequirementVerdict verdict)
{
    switch (verdict) {
    case RequirementVerdict::Satisfied:
        return "выполнено";
    case RequirementVerdict::Borderline:
        return "пограничный — увеличьте N";
    case RequirementVerdict::NotSatisfied:
        break;
    }
    return "не выполнено";
}

std::string selectionConclusion(const SelectionRequest& request, const std::vector<SelectionRow>& rows, const BestCodes& best)
{
    const std::string scope = "Для " + lengthText(request) + " на " + channelText(request.channel);
    if (!best.byRedundancy) {
        const std::string hint = hasBorderline(rows) ? "; есть пограничные коды — увеличьте N" : "";
        return scope + " ни один код не выполнил требование по Pлож" + hint + ".";
    }
    return scope + ": лучший по избыточности — " + redundancyPart(rows[*best.byRedundancy]) + "; лучший по времени — "
        + timePart(rows, best) + ".";
}

}
