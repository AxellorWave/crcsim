#pragma once

#include "simulation_runner.h"

#include "selection.h"

#include <QWidget>

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

class QComboBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class SelectionTable;
class TransferFields;

class SelectionPanel : public QWidget {
public:
    explicit SelectionPanel(QWidget* parent = nullptr);

    void setChannel(const std::optional<crc::ChannelParams>& channel);
    TransferFields* transferFields();
    void setOnTransferChanged(std::function<void()> handler);
    void startSelection();
    void stop();
    bool running() const;

private:
    QWidget* createSettings();
    QWidget* createRunControls();
    QLineEdit* createEdit(const QString& objectName, const QString& text, const QString& toolTip);
    void connectActions();

    QWidget* widgetFor(crc::Field field) const;
    std::optional<crc::SelectionRequest> readRequest();
    void startCandidate(std::size_t index);
    void showProgress(const crc::Counters& counters);
    void finishCandidate(const crc::SimulationResult& result);
    void failCandidate(const QString& message);
    void finishSelection(const QString& status);
    void refreshRows();
    void transferChanged();
    void updateButtons();

    QComboBox* lengthCombo_;
    QLabel* channelLabel_;
    QLineEdit* blocksEdit_;
    QLineEdit* seedEdit_;
    QLineEdit* targetEdit_;
    TransferFields* transferFields_;
    QLabel* errorLabel_;
    QPushButton* selectButton_;
    QPushButton* stopButton_;
    QProgressBar* progressBar_;
    QLabel* statusLabel_;
    SelectionTable* table_;
    QLabel* conclusionLabel_;
    SimulationRunner runner_;
    std::optional<crc::ChannelParams> channel_;
    crc::SelectionRequest request_;
    std::vector<crc::CyclicCode> candidates_;
    std::vector<crc::SimulationResult> results_;
    std::size_t current_ = 0;
    bool stopRequested_ = false;
    std::function<void()> onTransferChanged_;
};
