#pragma once

#include "field_error.h"

#include "params.h"

#include <QGroupBox>

#include <functional>
#include <vector>

class QComboBox;
class QLabel;
class QLineEdit;

class ControlPanel : public QGroupBox {
public:
    explicit ControlPanel(QWidget* parent = nullptr);

    void setOnChanged(std::function<void()> handler);
    std::vector<FieldError> validate();
    crc::ControlParams params() const;

private:
    QLineEdit* createEdit(const QString& objectName, const QString& text);
    void updateStopMode();
    void readCount(QLineEdit* edit, const QString& message, std::uint64_t& target, std::vector<FieldError>& errors);
    void readSeed(std::vector<FieldError>& errors);
    void notifyChanged();

    QComboBox* stopModeCombo_;
    QLabel* blockLimitLabel_;
    QLineEdit* blockLimitEdit_;
    QLineEdit* errorLimitEdit_;
    QLineEdit* seedEdit_;
    crc::ControlParams params_;
    std::function<void()> onChanged_;
};
