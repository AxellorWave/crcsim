#pragma once

#include "analysis_runner.h"

#include "analysis.h"
#include "simulator.h"

#include <QWidget>

#include <functional>
#include <optional>
#include <vector>

class AnalysisPlot;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QTableWidget;

class AnalysisPanel : public QWidget {
public:
    using SpectrumHandler = std::function<void(const crc::CyclicCode&, const crc::WeightSpectrum&)>;

    explicit AnalysisPanel(QWidget* parent = nullptr);

    void setCode(const std::optional<crc::CyclicCode>& code);
    void setResults(const std::vector<crc::SimulationResult>& results);
    void setOnSpectrum(SpectrumHandler handler);
    void cancel();

private:
    QWidget* createControls();
    QWidget* createSummaryColumn();
    QWidget* createPlotColumn();

    void startAnalysis();
    void finishAnalysis(const crc::CodeAnalysis& analysis);
    void updateControls();
    void updateStatus();
    void showSummary();
    void fillSpectrumTable();
    void updatePoints();
    void updateProbability();

    QPushButton* calculateButton_;
    QProgressBar* busyIndicator_;
    QLabel* statusLabel_;
    QLabel* summaryLabel_;
    QTableWidget* spectrumTable_;
    AnalysisPlot* plot_;
    QLineEdit* probabilityEdit_;
    QLabel* probabilityLabel_;
    AnalysisRunner runner_;
    std::optional<crc::CyclicCode> currentCode_;
    std::optional<crc::CodeAnalysis> analysis_;
    std::vector<crc::SimulationResult> results_;
    SpectrumHandler onSpectrum_;
};
