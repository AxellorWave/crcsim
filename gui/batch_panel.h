#pragma once

#include "simulation_runner.h"

#include "plan.h"
#include "simulator.h"

#include <QWidget>

#include <cstddef>
#include <functional>
#include <vector>

class PlanTable;
class QCheckBox;
class QLabel;
class QProgressBar;
class QPushButton;

class BatchPanel : public QWidget {
public:
    using ResultHandler = std::function<void(const crc::SimulationResult&)>;

    explicit BatchPanel(QWidget* parent = nullptr);

    void setOnResult(ResultHandler handler);
    void setPlan(const crc::ExperimentPlan& plan, const QString& source);
    bool loadPlanFile(const QString& path);
    void loadMethodichkaPlan();
    void startAll();
    void stop();
    bool running() const;

private:
    QWidget* createPlanControls();
    QWidget* createRunControls();
    QWidget* createResultControls();
    void connectActions();

    void openPlan();
    void savePlan();
    void saveResults();
    void startExperiment(std::size_t index);
    void showProgress(const crc::Counters& counters);
    void finishExperiment(const crc::SimulationResult& result);
    void failExperiment(const QString& message);
    void finishBatch(const QString& status);
    void showOverallProgress(int currentPermille);
    void updateButtons();

    QPushButton* openButton_;
    QPushButton* methodichkaButton_;
    QCheckBox* gilbertCheck_;
    QPushButton* savePlanButton_;
    QLabel* planLabel_;
    PlanTable* table_;
    QPushButton* runButton_;
    QPushButton* stopButton_;
    QProgressBar* progressBar_;
    QLabel* statusLabel_;
    QPushButton* saveResultsButton_;
    SimulationRunner runner_;
    crc::ExperimentPlan plan_;
    std::vector<crc::SimulationResult> results_;
    std::size_t current_ = 0;
    bool stopRequested_ = false;
    ResultHandler onResult_;
};
