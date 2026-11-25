#include "control_panel.h"

#include "help_button.h"
#include "input_parsing.h"

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>

#include <optional>

ControlPanel::ControlPanel(QWidget* parent)
    : QGroupBox(QStringLiteral("Управление моделированием"), parent)
    , stopModeCombo_(new QComboBox(this))
    , blockLimitLabel_(new QLabel(this))
{
    const crc::ControlParams defaults;
    stopModeCombo_->setObjectName(QStringLiteral("stopModeCombo"));
    stopModeCombo_->addItem(QStringLiteral("по числу блоков N"));
    stopModeCombo_->addItem(QStringLiteral("по числу ошибочных блоков"));
    blockLimitEdit_ = createEdit(QStringLiteral("blockLimitEdit"), formatCount(defaults.blockLimit));
    errorLimitEdit_ = createEdit(QStringLiteral("errorLimitEdit"), formatCount(defaults.errorLimit));
    seedEdit_ = createEdit(QStringLiteral("seedEdit"), QString());
    seedEdit_->setPlaceholderText(QStringLiteral("случайное"));

    auto* layout = new QFormLayout(this);
    layout->addRow(QStringLiteral("Остановка:"), withHelpButton(stopModeCombo_, HelpTopic::StopMode));
    layout->addRow(blockLimitLabel_, withHelpButton(blockLimitEdit_, HelpTopic::BlockLimit));
    layout->addRow(QStringLiteral("Ошибочных блоков:"), withHelpButton(errorLimitEdit_, HelpTopic::ErrorLimit));
    layout->addRow(QStringLiteral("Seed:"), withHelpButton(seedEdit_, HelpTopic::Seed));

    updateStopMode();
    connect(stopModeCombo_, &QComboBox::currentIndexChanged, this, [this] {
        updateStopMode();
        notifyChanged();
    });
}

QLineEdit* ControlPanel::createEdit(const QString& objectName, const QString& text)
{
    auto* edit = new QLineEdit(text, this);
    edit->setObjectName(objectName);
    connect(edit, &QLineEdit::textChanged, this, [this] { notifyChanged(); });
    return edit;
}

void ControlPanel::setOnChanged(std::function<void()> handler)
{
    onChanged_ = std::move(handler);
}

void ControlPanel::notifyChanged()
{
    if (onChanged_)
        onChanged_();
}

void ControlPanel::updateStopMode()
{
    const bool countsErrors = stopModeCombo_->currentIndex() == 1;
    params_.stopMode = countsErrors ? crc::StopMode::ErrorCount : crc::StopMode::BlockCount;
    blockLimitLabel_->setText(countsErrors ? QStringLiteral("Не более N блоков:") : QStringLiteral("Число блоков N:"));
    errorLimitEdit_->setEnabled(countsErrors);
}

void ControlPanel::readCount(
    QLineEdit* edit, const QString& message, std::uint64_t& target, std::vector<FieldError>& errors)
{
    const std::optional<std::uint64_t> value = parseUnsigned(edit->text());
    if (value)
        target = *value;
    else
        errors.push_back({edit, message});
}

void ControlPanel::readSeed(std::vector<FieldError>& errors)
{
    if (seedEdit_->text().trimmed().isEmpty()) {
        params_.seed = 0;
        return;
    }
    readCount(seedEdit_, QStringLiteral("seed: введите целое неотрицательное число или оставьте поле пустым"),
        params_.seed, errors);
}

std::vector<FieldError> ControlPanel::validate()
{
    std::vector<FieldError> errors;
    readCount(blockLimitEdit_, QStringLiteral("N: введите целое положительное число"), params_.blockLimit, errors);
    if (params_.stopMode == crc::StopMode::ErrorCount) {
        readCount(errorLimitEdit_, QStringLiteral("число ошибочных блоков: введите целое положительное число"),
            params_.errorLimit, errors);
    }
    readSeed(errors);
    for (const crc::ValidationError& error : crc::validateControl(params_)) {
        QWidget* widget = error.field == crc::Field::ErrorLimit ? errorLimitEdit_ : blockLimitEdit_;
        addUnlessReported(errors, {widget, QString::fromStdString(error.message)});
    }
    highlightFields({blockLimitEdit_, errorLimitEdit_, seedEdit_}, errors);
    return errors;
}

crc::ControlParams ControlPanel::params() const
{
    return params_;
}
