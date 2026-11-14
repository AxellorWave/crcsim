#pragma once

#include "field_error.h"

#include "params.h"

#include <QGroupBox>

#include <functional>
#include <vector>

class QComboBox;
class QLabel;
class QLineEdit;
class QSpinBox;

class CodingPanel : public QGroupBox {
public:
    explicit CodingPanel(QWidget* parent = nullptr);

    void setOnChanged(std::function<void()> handler);
    std::vector<FieldError> validate();
    crc::CodingParams params() const;

private:
    void fillCodeList();
    void applySelectedCode();
    void selectMatchingCode();
    void handleFieldEdited();
    void showDerivedValues(bool valid);
    QWidget* widgetFor(crc::Field field) const;

    QComboBox* codeCombo_;
    QSpinBox* lengthSpin_;
    QLineEdit* generatorEdit_;
    QLabel* derivedLabel_;
    std::function<void()> onChanged_;
    bool applyingCode_ = false;
};
