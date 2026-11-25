#include "selection_table.h"

#include "input_parsing.h"
#include "probability_format.h"
#include "theme.h"

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

const int bestRole = Qt::UserRole;
const int verdictRole = Qt::UserRole + 1;

QColor verdictColor(crc::RequirementVerdict verdict)
{
    const Theme& theme = currentTheme();
    switch (verdict) {
    case crc::RequirementVerdict::Satisfied:
        return theme.successText;
    case crc::RequirementVerdict::Borderline:
        return theme.warningText;
    case crc::RequirementVerdict::NotSatisfied:
        break;
    }
    return theme.mutedText;
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
    keepThemed(this, [this] { recolorRows(); });
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

void SelectionTable::paintVerdict(int row, crc::RequirementVerdict verdict)
{
    QTableWidgetItem* cell = item(row, VerdictColumn);
    cell->setData(verdictRole, static_cast<int>(verdict));
    QFont font = cell->font();
    font.setBold(true);
    cell->setFont(font);
    cell->setForeground(QBrush(verdictColor(verdict)));
}

void SelectionTable::paintBest(int row)
{
    const Theme& theme = currentTheme();
    for (int column = 0; column < ColumnCount; ++column) {
        QTableWidgetItem* cell = item(row, column);
        cell->setBackground(QBrush(theme.highlightRow));
        if (column == Badge)
            cell->setForeground(QBrush(theme.badgeText));
        else if (column != VerdictColumn)
            cell->setForeground(QBrush(theme.highlightRowText));
    }
}

void SelectionTable::recolorRows()
{
    for (int row = 0; row < rowCount(); ++row) {
        QTableWidgetItem* verdict = item(row, VerdictColumn);
        if (!verdict)
            continue;
        verdict->setForeground(QBrush(verdictColor(static_cast<crc::RequirementVerdict>(verdict->data(verdictRole).toInt()))));
        if (item(row, Badge)->data(bestRole).toBool())
            paintBest(row);
    }
}

void SelectionTable::markBest(int row, bool byRedundancy, bool byTime)
{
    for (int column = 0; column < ColumnCount; ++column) {
        QTableWidgetItem* cell = item(row, column);
        QFont font = cell->font();
        font.setBold(true);
        cell->setFont(font);
    }
    QTableWidgetItem* badge = item(row, Badge);
    badge->setText(badgeText(byRedundancy, byTime));
    badge->setData(bestRole, true);
    paintBest(row);
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
