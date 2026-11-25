#include "plan_table.h"

#include "input_parsing.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QHeaderView>

namespace {

const int statusPadding = 24;

enum Column { Number, CodeLength, DataBits, Generator, ChannelName, ChannelValues, Blocks, Seed, Status, ColumnCount };

QString blocksText(const crc::ControlParams& control)
{
    if (control.stopMode == crc::StopMode::BlockCount)
        return formatCount(control.blockLimit);
    return QStringLiteral("≤ %1, до %2 ош.").arg(formatCount(control.blockLimit), formatCount(control.errorLimit));
}

QString seedText(std::uint64_t seed)
{
    return seed == 0 ? QStringLiteral("случайный") : QString::number(static_cast<qulonglong>(seed));
}

QString statusText(ExperimentStatus status, int percent)
{
    switch (status) {
    case ExperimentStatus::Running:
        return QStringLiteral("идёт — %1 %").arg(percent);
    case ExperimentStatus::Done:
        return QStringLiteral("готово");
    case ExperimentStatus::Cancelled:
        return QStringLiteral("прервано");
    case ExperimentStatus::Waiting:
        break;
    }
    return QStringLiteral("ожидает");
}

QColor statusColor(ExperimentStatus status)
{
    switch (status) {
    case ExperimentStatus::Running:
        return QColor(0x1a, 0x5f, 0xb4);
    case ExperimentStatus::Done:
        return QColor(0x1e, 0x7a, 0x32);
    case ExperimentStatus::Cancelled:
        return QColor(0x9a, 0x60, 0x00);
    case ExperimentStatus::Waiting:
        break;
    }
    return QColor(0x80, 0x80, 0x80);
}

}

PlanTable::PlanTable(QWidget* parent)
    : QTableWidget(0, ColumnCount, parent)
{
    setObjectName(QStringLiteral("planTable"));
    setHorizontalHeaderLabels({QStringLiteral("№"), QStringLiteral("n"), QStringLiteral("k"), QStringLiteral("g(x)"),
        QStringLiteral("Канал"), QStringLiteral("Параметры канала"), QStringLiteral("N"), QStringLiteral("seed"),
        QStringLiteral("Статус")});
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setAlternatingRowColors(true);
    verticalHeader()->hide();
    verticalHeader()->setDefaultSectionSize(fontMetrics().height() + 8);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(ChannelValues, QHeaderView::Stretch);
    horizontalHeader()->setSectionResizeMode(Status, QHeaderView::Fixed);
    setColumnWidth(Status, fontMetrics().horizontalAdvance(QStringLiteral("идёт — 100 %")) + statusPadding);
}

void PlanTable::setCell(int row, int column, const QString& text, const QString& toolTip)
{
    auto* cell = new QTableWidgetItem(text);
    const bool leftAligned = column == Generator || column == ChannelName || column == ChannelValues || column == Status;
    cell->setTextAlignment(leftAligned ? Qt::AlignLeft | Qt::AlignVCenter : Qt::AlignRight | Qt::AlignVCenter);
    cell->setToolTip(toolTip);
    setItem(row, column, cell);
}

void PlanTable::fillRow(int row, const crc::PlannedExperiment& experiment)
{
    const crc::SimulationParams& params = experiment.params;
    const crc::CyclicCode code = crc::makeCode(params.coding);
    const bool bsc = params.channel.kind == crc::ChannelKind::Bsc;
    const QString label = QString::fromStdString(experiment.label);
    setCell(row, Number, QString::number(row + 1), label);
    setCell(row, CodeLength, QString::number(code.n), label);
    setCell(row, DataBits, QString::number(code.dataBitCount()), label);
    setCell(row, Generator, QString::fromStdString(crc::toBinary(code.generator)),
        QString::fromStdString(crc::toDegreeList(code.generator)));
    setCell(row, ChannelName, bsc ? QStringLiteral("ДСК") : QStringLiteral("Гилберт"), label);
    setCell(row, ChannelValues, QString::fromStdString(crc::channelParametersText(params.channel)), label);
    setCell(row, Blocks, blocksText(params.control), label);
    setSeed(row, params.control.seed);
    setStatus(row, ExperimentStatus::Waiting);
}

void PlanTable::showPlan(const crc::ExperimentPlan& plan)
{
    setRowCount(0);
    setRowCount(static_cast<int>(plan.size()));
    for (std::size_t index = 0; index < plan.size(); ++index)
        fillRow(static_cast<int>(index), plan[index]);
}

void PlanTable::setStatus(int row, ExperimentStatus status, int percent)
{
    setCell(row, Status, statusText(status, percent));
    QTableWidgetItem* cell = item(row, Status);
    cell->setForeground(QBrush(statusColor(status)));
    QFont font = cell->font();
    font.setBold(status == ExperimentStatus::Running);
    cell->setFont(font);
    if (status == ExperimentStatus::Running)
        scrollToItem(cell);
}

void PlanTable::setSeed(int row, std::uint64_t seed)
{
    setCell(row, Seed, seedText(seed));
}
