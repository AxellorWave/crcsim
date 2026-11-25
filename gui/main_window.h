#pragma once

#include "simulation_runner.h"

#include "simulator.h"

#include <QMainWindow>

class AnalysisPanel;
class ChannelPanel;
class CodingPanel;
class ControlPanel;
class ModelDiagram;
class QLabel;
class QProgressBar;
class QPushButton;
class QTabWidget;
class ResultsTable;
class WalkthroughPanel;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    QWidget* createParameterColumn();
    QWidget* createRunColumn();
    QWidget* createSimulationTab();
    QWidget* createRunControls();
    QWidget* createTableControls();
    void connectActions();

    void revalidate();
    void setRunning(bool running);
    void startSimulation();
    void showProgress(const crc::Counters& counters);
    void finishSimulation(const crc::SimulationResult& result);
    void failSimulation(const QString& message);
    void saveResults();
    void clearResults();
    void updateTableButtons();
    void updateAnalysisCode(bool codingValid);
    void updateWalkthrough(const QString& codingProblem, bool channelValid);

    CodingPanel* codingPanel_;
    ChannelPanel* channelPanel_;
    ControlPanel* controlPanel_;
    QLabel* errorLabel_;
    ModelDiagram* diagram_;
    QPushButton* startButton_;
    QPushButton* stopButton_;
    QProgressBar* progressBar_;
    QLabel* countersLabel_;
    QLabel* statusLabel_;
    ResultsTable* resultsTable_;
    QPushButton* clearButton_;
    QPushButton* saveButton_;
    AnalysisPanel* analysisPanel_;
    QTabWidget* tabs_;
    WalkthroughPanel* walkthroughPanel_;
    SimulationRunner runner_;
    crc::ControlParams runningControl_;
    bool parametersValid_ = false;
};
