#pragma once

#include "field_error.h"
#include "parameter_help.h"

#include "params.h"

#include <QGroupBox>
#include <QString>

#include <functional>
#include <vector>

class QComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QStackedWidget;

class ChannelPanel : public QGroupBox {
public:
    explicit ChannelPanel(QWidget* parent = nullptr);

    void setOnChanged(std::function<void()> handler);
    std::vector<FieldError> validate();
    crc::ChannelParams params() const;

private:
    struct ProbabilityInput {
        crc::Field field;
        QString name;
        QLineEdit* edit;
        double* target;
    };

    QLineEdit* addProbabilityEdit(QFormLayout* layout, const QString& label, const QString& objectName, double value,
        HelpTopic topic);
    QWidget* createBscPage();
    QWidget* createGilbertPage();
    void showPage(int index);
    std::vector<ProbabilityInput> activeInputs();
    QWidget* widgetFor(crc::Field field) const;
    void showGilbertSummary(bool valid);
    void notifyChanged();

    QComboBox* kindCombo_;
    QStackedWidget* pages_;
    QLineEdit* bitErrorEdit_ = nullptr;
    QLineEdit* goodToBadEdit_ = nullptr;
    QLineEdit* badToGoodEdit_ = nullptr;
    QLineEdit* errorInBadEdit_ = nullptr;
    QLineEdit* errorInGoodEdit_ = nullptr;
    QLabel* gilbertSummaryLabel_ = nullptr;
    crc::ChannelParams params_;
    std::function<void()> onChanged_;
};
