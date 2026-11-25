#pragma once

#include "plan.h"

#include <QTableWidget>

#include <cstdint>

enum class ExperimentStatus { Waiting, Running, Done, Cancelled };

class PlanTable : public QTableWidget {
public:
    explicit PlanTable(QWidget* parent = nullptr);

    void showPlan(const crc::ExperimentPlan& plan);
    void setStatus(int row, ExperimentStatus status, int percent = 0);
    void setSeed(int row, std::uint64_t seed);

private:
    void setCell(int row, int column, const QString& text, const QString& toolTip = QString());
    void fillRow(int row, const crc::PlannedExperiment& experiment);
};
