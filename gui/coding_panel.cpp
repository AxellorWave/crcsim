#include "coding_panel.h"

#include "codes.h"
#include "poly.h"
#include "results.h"

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

#include <stdexcept>

namespace {

const int customCodeIndex = -1;

QString knownCodeTitle(const crc::KnownCode& code)
{
    return QStringLiteral("(%1, %2), t = %3: %4")
        .arg(code.n)
        .arg(code.k)
        .arg(code.correctableErrors)
        .arg(QString::fromStdString(code.binary));
}

bool sameGenerator(const QString& text, const std::string& binary)
{
    try {
        return crc::parsePolynomial(text.toStdString()) == crc::parsePolynomial(binary);
    } catch (const std::invalid_argument&) {
        return false;
    }
}

}

CodingPanel::CodingPanel(QWidget* parent)
    : QGroupBox(QStringLiteral("Кодирование"), parent)
    , codeCombo_(new QComboBox(this))
    , lengthSpin_(new QSpinBox(this))
    , generatorEdit_(new QLineEdit(this))
    , derivedLabel_(new QLabel(this))
{
    codeCombo_->setObjectName(QStringLiteral("codeCombo"));
    lengthSpin_->setObjectName(QStringLiteral("lengthSpin"));
    generatorEdit_->setObjectName(QStringLiteral("generatorEdit"));

    fillCodeList();
    lengthSpin_->setRange(crc::minCodeLength, crc::maxCodeLength);
    lengthSpin_->setToolTip(QStringLiteral("Длина кодового слова n, бит (от 3 до 64)"));
    generatorEdit_->setToolTip(
        QStringLiteral("Образующий многочлен: двоичная запись (10011) или список степеней ([4;1;0])"));
    generatorEdit_->setPlaceholderText(QStringLiteral("10011 или [4;1;0]"));
    derivedLabel_->setWordWrap(true);

    auto* layout = new QFormLayout(this);
    layout->addRow(QStringLiteral("Код:"), codeCombo_);
    layout->addRow(QStringLiteral("Длина n:"), lengthSpin_);
    layout->addRow(QStringLiteral("g(x):"), generatorEdit_);
    layout->addRow(QStringLiteral("Расчёт:"), derivedLabel_);

    const crc::CodingParams defaults;
    lengthSpin_->setValue(defaults.n);
    generatorEdit_->setText(QString::fromStdString(defaults.generator));
    selectMatchingCode();

    connect(codeCombo_, &QComboBox::currentIndexChanged, this, [this] { applySelectedCode(); });
    connect(lengthSpin_, &QSpinBox::valueChanged, this, [this] { handleFieldEdited(); });
    connect(generatorEdit_, &QLineEdit::textChanged, this, [this] { handleFieldEdited(); });
}

void CodingPanel::setOnChanged(std::function<void()> handler)
{
    onChanged_ = std::move(handler);
}

void CodingPanel::fillCodeList()
{
    const std::vector<crc::KnownCode>& codes = crc::knownCodes();
    for (std::size_t index = 0; index < codes.size(); ++index)
        codeCombo_->addItem(knownCodeTitle(codes[index]), static_cast<int>(index));
    codeCombo_->addItem(QStringLiteral("Свой многочлен"), customCodeIndex);
}

void CodingPanel::applySelectedCode()
{
    if (applyingCode_)
        return;
    const int index = codeCombo_->currentData().toInt();
    if (index != customCodeIndex) {
        const crc::KnownCode& code = crc::knownCodes()[static_cast<std::size_t>(index)];
        applyingCode_ = true;
        lengthSpin_->setValue(code.n);
        generatorEdit_->setText(QString::fromStdString(code.binary));
        applyingCode_ = false;
    }
    if (onChanged_)
        onChanged_();
}

void CodingPanel::selectMatchingCode()
{
    int match = codeCombo_->count() - 1;
    const std::vector<crc::KnownCode>& codes = crc::knownCodes();
    for (std::size_t index = 0; index < codes.size(); ++index) {
        if (codes[index].n == lengthSpin_->value() && sameGenerator(generatorEdit_->text(), codes[index].binary))
            match = static_cast<int>(index);
    }
    applyingCode_ = true;
    codeCombo_->setCurrentIndex(match);
    applyingCode_ = false;
}

void CodingPanel::handleFieldEdited()
{
    if (applyingCode_)
        return;
    selectMatchingCode();
    if (onChanged_)
        onChanged_();
}

crc::CodingParams CodingPanel::params() const
{
    crc::CodingParams coding;
    coding.n = lengthSpin_->value();
    coding.generator = generatorEdit_->text().trimmed().toStdString();
    return coding;
}

QWidget* CodingPanel::widgetFor(crc::Field field) const
{
    return field == crc::Field::CodeLength ? static_cast<QWidget*>(lengthSpin_) : generatorEdit_;
}

std::vector<FieldError> CodingPanel::validate()
{
    std::vector<FieldError> errors;
    for (const crc::ValidationError& error : crc::validateCoding(params()))
        errors.push_back({widgetFor(error.field), QString::fromStdString(error.message)});
    highlightFields({lengthSpin_, generatorEdit_}, errors);
    showDerivedValues(errors.empty());
    return errors;
}

void CodingPanel::showDerivedValues(bool valid)
{
    if (!valid) {
        derivedLabel_->setText(QStringLiteral("h = —, k = —, I = —"));
        return;
    }
    const crc::CyclicCode code = crc::makeCode(params());
    derivedLabel_->setText(QStringLiteral("h = %1 (проверочных), k = %2 (информационных),\nI = (n − k)/n = %3")
                               .arg(code.checkBitCount())
                               .arg(code.dataBitCount())
                               .arg(QString::fromStdString(crc::formatNumber(code.redundancy(), 3))));
}
