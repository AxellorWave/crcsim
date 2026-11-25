#include "results_table.h"

#include "probability_format.h"

#include "results.h"

#include <QHeaderView>
#include <QStringList>

namespace {

enum Column { CodeLength, DataBits, Generator, ChannelName, BitError, Blocks, Correct, Detected, Undetected,
    CorrectShare, DetectedShare, UndetectedShare, Redundancy, ColumnCount };

QString number(double value, int digits)
{
    return QString::fromStdString(crc::formatNumber(value, digits));
}

QString count(std::uint64_t value)
{
    return QString::number(static_cast<qulonglong>(value));
}

QString shortChannelName(const crc::SimulationResult& result)
{
    return result.channelCsvName == "BSC" ? QStringLiteral("ДСК") : QStringLiteral("Гилб.");
}

QString probabilityText(const crc::ProbabilityEstimate& estimate)
{
    return estimate.observed() ? number(estimate.value(), 4) : QStringLiteral("—");
}

QString probabilityToolTip(const crc::ProbabilityEstimate& estimate)
{
    return estimate.observed() ? QString() : QString::fromStdString(crc::describeUnobserved(estimate));
}

bool sameCode(const crc::CyclicCode& left, const crc::CyclicCode& right)
{
    return left.n == right.n && left.generator == right.generator;
}

}

ResultsTable::ResultsTable(QWidget* parent)
    : QTableWidget(0, ColumnCount, parent)
{
    setHorizontalHeaderLabels({QStringLiteral("n"), QStringLiteral("k"), QStringLiteral("g(x)"),
        QStringLiteral("Канал"), QStringLiteral("p"), QStringLiteral("N"), QStringLiteral("Nв"),
        QStringLiteral("Nобн"), QStringLiteral("Nлож"), QStringLiteral("Pв"), QStringLiteral("Pобн"),
        QStringLiteral("Pлож"), QStringLiteral("I")});
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setAlternatingRowColors(true);
    verticalHeader()->setDefaultSectionSize(fontMetrics().height() + 8);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(Generator, QHeaderView::Stretch);
}

void ResultsTable::setCell(int row, int column, const QString& text, const QString& toolTip)
{
    auto* item = new QTableWidgetItem(text);
    item->setTextAlignment(column == Generator ? Qt::AlignLeft | Qt::AlignVCenter : Qt::AlignRight | Qt::AlignVCenter);
    item->setToolTip(toolTip);
    setItem(row, column, item);
}

void ResultsTable::addResult(const crc::SimulationResult& result)
{
    results_.push_back(result);
    const crc::Summary summary = crc::summarize(result);
    const crc::Counters& counters = result.counters;
    const int row = rowCount();
    insertRow(row);
    setCell(row, CodeLength, QString::number(result.code.n));
    setCell(row, DataBits, QString::number(result.code.dataBitCount()));
    setCell(row, Generator, QString::fromStdString(crc::toBinary(result.code.generator)),
        QString::fromStdString(crc::toDegreeList(result.code.generator)));
    setCell(row, ChannelName, shortChannelName(result), QString::fromStdString(result.channelDisplayName));
    setCell(row, BitError, number(result.meanBitErrorProbability, 3));
    setCell(row, Blocks, count(counters.blocks),
        result.cancelled ? QStringLiteral("моделирование прервано") : QString());
    setCell(row, Correct, count(counters.correct));
    setCell(row, Detected, count(counters.detected));
    setCell(row, Undetected, count(counters.undetected));
    setCell(row, CorrectShare, probabilityText(summary.correct), probabilityToolTip(summary.correct));
    setCell(row, DetectedShare, probabilityText(summary.detected), probabilityToolTip(summary.detected));
    setCell(row, UndetectedShare, probabilityText(summary.undetected), undetectedToolTip(result));
    setCell(row, Redundancy, number(summary.redundancy, 3));
    scrollToBottom();
}

QString ResultsTable::undetectedToolTip(const crc::SimulationResult& result)
{
    const QString observed = probabilityToolTip(crc::summarize(result).undetected);
    if (result.channelCsvName != "BSC")
        return observed;
    const std::optional<double> theory = theory_.undetectedProbability(result.code, result.meanBitErrorProbability);
    if (!theory)
        return observed;
    const QString theoryText = QStringLiteral("теория: ") + formatSmallProbability(*theory);
    return observed.isEmpty() ? theoryText : observed + QLatin1Char('\n') + theoryText;
}

void ResultsTable::rememberSpectrum(const crc::CyclicCode& code, const crc::WeightSpectrum& spectrum)
{
    theory_.store(code, spectrum);
    for (int row = 0; row < rowCount(); ++row) {
        const crc::SimulationResult& result = results_[static_cast<std::size_t>(row)];
        if (sameCode(result.code, code))
            item(row, UndetectedShare)->setToolTip(undetectedToolTip(result));
    }
}

void ResultsTable::clearResults()
{
    results_.clear();
    setRowCount(0);
}

const std::vector<crc::SimulationResult>& ResultsTable::results() const
{
    return results_;
}
