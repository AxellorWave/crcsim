#pragma once

#include "theory_cache.h"

#include "simulator.h"
#include "transfer.h"

#include <QTableWidget>

#include <optional>
#include <vector>

class ResultsTable : public QTableWidget {
public:
    explicit ResultsTable(QWidget* parent = nullptr);

    void addResult(const crc::SimulationResult& result);
    void clearResults();
    const std::vector<crc::SimulationResult>& results() const;
    void setTransfer(const std::optional<crc::TransferSettings>& transfer);

private:
    void setCell(int row, int column, const QString& text, const QString& toolTip = QString());
    QString undetectedToolTip(const crc::SimulationResult& result);
    void fillTransferCells(int row);

    TheoryCache theory_;
    std::optional<crc::TransferSettings> transfer_ = crc::TransferSettings{};

    std::vector<crc::SimulationResult> results_;
};
