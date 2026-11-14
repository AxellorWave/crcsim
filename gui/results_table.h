#pragma once

#include "simulator.h"

#include <QTableWidget>

#include <vector>

class ResultsTable : public QTableWidget {
public:
    explicit ResultsTable(QWidget* parent = nullptr);

    void addResult(const crc::SimulationResult& result);
    void clearResults();
    const std::vector<crc::SimulationResult>& results() const;

private:
    void setCell(int row, int column, const QString& text, const QString& toolTip = QString());

    std::vector<crc::SimulationResult> results_;
};
