#pragma once

#include "theory_cache.h"

#include "simulator.h"

#include <QTableWidget>

#include <vector>

class ResultsTable : public QTableWidget {
public:
    explicit ResultsTable(QWidget* parent = nullptr);

    void addResult(const crc::SimulationResult& result);
    void clearResults();
    const std::vector<crc::SimulationResult>& results() const;
    void rememberSpectrum(const crc::CyclicCode& code, const crc::WeightSpectrum& spectrum);

private:
    void setCell(int row, int column, const QString& text, const QString& toolTip = QString());
    QString undetectedToolTip(const crc::SimulationResult& result);

    TheoryCache theory_;

    std::vector<crc::SimulationResult> results_;
};
