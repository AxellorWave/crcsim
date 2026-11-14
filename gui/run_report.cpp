#include "run_report.h"

#include "input_parsing.h"

#include "results.h"

#include <QStringList>

#include <algorithm>

namespace {

QString number(double value, int digits)
{
    return QString::fromStdString(crc::formatNumber(value, digits));
}

double share(std::uint64_t done, std::uint64_t limit)
{
    return limit == 0 ? 0.0 : static_cast<double>(done) / static_cast<double>(limit);
}

}

QString countersText(const crc::Counters& counters)
{
    return QStringLiteral("N = %1    Nв = %2    Nобн = %3    Nлож = %4")
        .arg(formatCount(counters.blocks), formatCount(counters.correct), formatCount(counters.detected),
            formatCount(counters.undetected));
}

int progressPermille(const crc::Counters& counters, const crc::ControlParams& control)
{
    double done = share(counters.blocks, control.blockLimit);
    if (control.stopMode == crc::StopMode::ErrorCount)
        done = std::max(done, share(counters.erroneous(), control.errorLimit));
    return static_cast<int>(std::min(1.0, done) * 1000.0);
}

QString runSummaryText(const crc::SimulationResult& result)
{
    const crc::Summary summary = crc::summarize(result);
    QStringList parts;
    parts.append(result.cancelled ? QStringLiteral("Прервано пользователем") : QStringLiteral("Завершено"));
    parts.append(QStringLiteral("seed = %1").arg(static_cast<qulonglong>(result.seed)));
    parts.append(QStringLiteral("доля искажённых бит = %1").arg(number(summary.measuredBitErrorRate, 4)));
    parts.append(summary.meanAttempts ? QStringLiteral("L = N/Nв = %1").arg(number(*summary.meanAttempts, 4))
                                      : QStringLiteral("L = N/Nв: Nв = 0"));
    if (result.channelCsvName == "BSC") {
        const double theory = crc::theoreticalCorrectShare(result.code.n, result.meanBitErrorProbability);
        parts.append(QStringLiteral("теоретическая Pв = (1 − p)^n = %1").arg(number(theory, 4)));
    }
    return parts.join(QStringLiteral("; "));
}
