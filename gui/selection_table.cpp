#include "selection_table.h"

#include "input_parsing.h"
#include "probability_format.h"

#include "plan.h"
#include "text.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QHeaderView>
#include <QStringList>

namespace {

enum Column { Code, CheckBits, Redundancy, Undetected, Blocks, UndetectedShare, UpperBound, VerdictColumn, MeanAttempts,
    TransferTime, Badge, ColumnCount };

const QColor bestRowColor(0xe4, 0xee, 0xfb);
const QColor badgeColor(0x1a, 0x4f, 0x9c);

struct VerdictStyle {
    QColor background;
    QColor foreground;
};

VerdictStyle verdictStyle(crc::Verdict verdict)
{
    switch (verdict) {
    case crc::Verdict::Satisfied:
        return {QColor(0xd4, 0xef, 0xd8), QColor(0x16, 0x5c, 0x26)};
    case crc::Verdict::Borderline:
        return {QColor(0xfd, 0xe7, 0xb4), QColor(0x7a, 0x4b, 0x00)};
    case crc::Verdict::NotSatisfied:
        break;
    }
    return {QColor(0xee, 0xee, 0xee), QColor(0x70, 0x70, 0x70)};
}

QString significant(const std::optional<double>& value)
{
    return value ? QString::fromStdString(crc::formatSignificant(*value, 4, false)) : QStringLiteral("—");
}

QString badgeText(bool byRedundancy, bool byTime)
{
    QStringList badges;
    if (byRedundancy)
        badges.append(QStringLiteral("лучший по I"));
    if (byTime)
        badges.append(QStringLiteral("лучший по Tф"));
    return badges.join(QStringLiteral(" · "));
}

}

SelectionTable::SelectionTable(QWidget* parent)
    : QTableWidget(0, ColumnCount, parent)
{
    setObjectName(QStringLiteral("selectionTable"));
    setHorizontalHeaderLabels({QStringLiteral("Код"), QStringLiteral("h"), QStringLiteral("I"), QStringLiteral("Nлож"),
        QStringLiteral("N"), QStringLiteral("Pлож"), QStringLiteral("Pлож < (95 %)"), QStringLiteral("Вывод"),
        QStringLiteral("L"), QStringLiteral("Tф, с"), QStringLiteral("Отметка")});
    horizontalHeaderItem(UpperBound)->setToolTip(QStringLiteral("Верхняя граница Pлож с доверием 95 %"));
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionMode(QAbstractItemView::NoSelection);
    verticalHeader()->hide();
    verticalHeader()->setDefaultSectionSize(fontMetrics().height() + 10);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(Badge, QHeaderView::Stretch);
}

QTableWidgetItem* SelectionTable::setCell(int row, int column, const QString& text, const QString& toolTip)
{
    auto* cell = new QTableWidgetItem(text);
    const bool leftAligned = column == Code || column == VerdictColumn || column == Badge;
    cell->setTextAlignment(leftAligned ? Qt::AlignLeft | Qt::AlignVCenter : Qt::AlignRight | Qt::AlignVCenter);
    cell->setToolTip(toolTip);
    setItem(row, column, cell);
    return cell;
}

void SelectionTable::fillRow(int row, const crc::SelectionRow& selection)
{
    const crc::CyclicCode& code = selection.result.code;
    const crc::Counters& counters = selection.result.counters;
    const QString infinite = QStringLiteral("блок ни разу не принят верно, L и Tф бесконечны");
    setCell(row, Code, QString::fromStdString(crc::codeName(code)), QString::fromStdString(crc::toBinary(code.generator)));
    setCell(row, CheckBits, QString::number(code.checkBitCount()));
    setCell(row, Redundancy, QString::fromStdString(crc::formatNumber(code.redundancy(), 3)));
    setCell(row, Undetected, QString::number(static_cast<qulonglong>(counters.undetected)));
    setCell(row, Blocks, formatCount(counters.blocks));
    setCell(row, UndetectedShare,
        selection.undetected.observed() ? formatSmallProbability(selection.undetected.value()) : QStringLiteral("—"),
        selection.undetected.observed() ? QString() : QString::fromStdString(crc::describeUnobserved(selection.undetected)));
    setCell(row, UpperBound, formatSmallProbability(selection.upperBound));
    setCell(row, VerdictColumn, QString::fromStdString(crc::verdictText(selection.verdict)));
    setCell(row, MeanAttempts, significant(selection.meanAttempts), selection.meanAttempts ? QString() : infinite);
    setCell(row, TransferTime, significant(selection.transferTime), selection.transferTime ? QString() : infinite);
    setCell(row, Badge, QString());
    paintVerdict(row, selection.verdict);
}

void SelectionTable::paintVerdict(int row, crc::Verdict verdict)
{
    const VerdictStyle style = verdictStyle(verdict);
    QTableWidgetItem* cell = item(row, VerdictColumn);
    cell->setBackground(QBrush(style.background));
    cell->setForeground(QBrush(style.foreground));
}

void SelectionTable::markBest(int row, bool byRedundancy, bool byTime)
{
    for (int column = 0; column < ColumnCount; ++column) {
        QTableWidgetItem* cell = item(row, column);
        QFont font = cell->font();
        font.setBold(true);
        cell->setFont(font);
        if (column != VerdictColumn)
            cell->setBackground(QBrush(bestRowColor));
    }
    QTableWidgetItem* badge = item(row, Badge);
    badge->setText(badgeText(byRedundancy, byTime));
    badge->setForeground(QBrush(badgeColor));
}

void SelectionTable::showRows(const std::vector<crc::SelectionRow>& rows, const crc::BestCodes& best)
{
    setRowCount(static_cast<int>(rows.size()));
    for (std::size_t index = 0; index < rows.size(); ++index) {
        const int row = static_cast<int>(index);
        fillRow(row, rows[index]);
        const bool byRedundancy = best.byRedundancy == index;
        const bool byTime = best.byTime == index;
        if (byRedundancy || byTime)
            markBest(row, byRedundancy, byTime);
    }
}
