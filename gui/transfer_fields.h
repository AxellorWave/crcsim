#pragma once

#include "transfer.h"

#include <QWidget>

#include <functional>
#include <optional>

class QLabel;
class QLineEdit;

class TransferFields : public QWidget {
public:
    explicit TransferFields(QWidget* parent = nullptr);

    void setOnChanged(std::function<void()> handler);
    std::optional<crc::TransferSettings> settings();
    void copyFrom(const TransferFields& other);

private:
    QLineEdit* createEdit(const QString& objectName, const QString& text, const QString& toolTip);
    void notifyChanged();

    QLineEdit* fileEdit_;
    QLineEdit* rateEdit_;
    QLabel* errorLabel_;
    std::function<void()> onChanged_;
};
