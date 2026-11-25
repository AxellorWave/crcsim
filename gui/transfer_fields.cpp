#include "transfer_fields.h"

#include "field_error.h"
#include "input_parsing.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QStringList>

#include <vector>

namespace {

const int editWidth = 110;

void readPositive(QLineEdit* edit, const QString& message, double& target, std::vector<FieldError>& errors)
{
    const std::optional<double> value = parseDecimal(edit->text());
    if (value)
        target = *value;
    else
        errors.push_back({edit, message});
}

}

TransferFields::TransferFields(QWidget* parent)
    : QWidget(parent)
    , errorLabel_(new QLabel(this))
{
    const crc::TransferSettings defaults;
    fileEdit_ = createEdit(QStringLiteral("fileBitsEdit"), formatCount(static_cast<std::uint64_t>(defaults.fileBits)),
        QStringLiteral("Размер передаваемого файла M в битах"));
    rateEdit_ = createEdit(QStringLiteral("bitRateEdit"), formatCount(static_cast<std::uint64_t>(defaults.bitRate)),
        QStringLiteral("Скорость передачи по каналу, бит/с; длительность бита T = 1/скорость"));
    errorLabel_->setStyleSheet(QStringLiteral("color: #b01010;"));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new QLabel(QStringLiteral("Файл M, бит:")));
    layout->addWidget(fileEdit_);
    layout->addSpacing(12);
    layout->addWidget(new QLabel(QStringLiteral("Скорость, бит/с:")));
    layout->addWidget(rateEdit_);
    layout->addSpacing(12);
    layout->addWidget(errorLabel_, 1);
}

QLineEdit* TransferFields::createEdit(const QString& objectName, const QString& text, const QString& toolTip)
{
    auto* edit = new QLineEdit(text, this);
    edit->setObjectName(objectName);
    edit->setToolTip(toolTip);
    edit->setFixedWidth(editWidth);
    connect(edit, &QLineEdit::textChanged, this, [this] { notifyChanged(); });
    return edit;
}

void TransferFields::setOnChanged(std::function<void()> handler)
{
    onChanged_ = std::move(handler);
}

void TransferFields::notifyChanged()
{
    if (onChanged_)
        onChanged_();
}

std::optional<crc::TransferSettings> TransferFields::settings()
{
    crc::TransferSettings transfer;
    std::vector<FieldError> errors;
    readPositive(fileEdit_, QStringLiteral("M: введите число бит больше 0"), transfer.fileBits, errors);
    readPositive(rateEdit_, QStringLiteral("скорость: введите число бит/с больше 0"), transfer.bitRate, errors);
    for (const crc::ValidationError& error : crc::validateTransfer(transfer)) {
        QLineEdit* edit = error.field == crc::Field::FileSize ? fileEdit_ : rateEdit_;
        addUnlessReported(errors, {edit, QString::fromStdString(error.message)});
    }
    highlightFields({fileEdit_, rateEdit_}, errors);
    QStringList messages;
    for (const FieldError& error : errors)
        messages.append(error.message);
    errorLabel_->setText(messages.join(QStringLiteral("; ")));
    if (!errors.empty())
        return std::nullopt;
    return transfer;
}

void TransferFields::copyFrom(const TransferFields& other)
{
    if (fileEdit_->text() != other.fileEdit_->text())
        fileEdit_->setText(other.fileEdit_->text());
    if (rateEdit_->text() != other.rateEdit_->text())
        rateEdit_->setText(other.rateEdit_->text());
}
