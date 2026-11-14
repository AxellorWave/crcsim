#include "channel_panel.h"

#include "input_parsing.h"

#include "results.h"

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

QString formatValue(double value)
{
    return QString::fromStdString(crc::formatNumber(value, 3));
}

QString notNumberMessage(const QString& name)
{
    return name + QStringLiteral(": введите число (например, 0,005 или 0.005)");
}

}

ChannelPanel::ChannelPanel(QWidget* parent)
    : QGroupBox(QStringLiteral("Канал связи"), parent)
    , kindCombo_(new QComboBox(this))
    , pages_(new QStackedWidget(this))
{
    kindCombo_->setObjectName(QStringLiteral("channelCombo"));
    kindCombo_->addItem(QStringLiteral("ДСК (независимые ошибки)"));
    kindCombo_->addItem(QStringLiteral("Гилберта (пачки ошибок)"));
    pages_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    pages_->addWidget(createBscPage());
    pages_->addWidget(createGilbertPage());

    auto* kindLayout = new QFormLayout;
    kindLayout->addRow(QStringLiteral("Модель:"), kindCombo_);
    auto* layout = new QVBoxLayout(this);
    layout->addLayout(kindLayout);
    layout->addWidget(pages_);

    showPage(0);
    connect(kindCombo_, &QComboBox::currentIndexChanged, this, [this](int index) {
        showPage(index);
        notifyChanged();
    });
}

void ChannelPanel::showPage(int index)
{
    for (int page = 0; page < pages_->count(); ++page) {
        const QSizePolicy::Policy policy = page == index ? QSizePolicy::Preferred : QSizePolicy::Ignored;
        pages_->widget(page)->setSizePolicy(QSizePolicy::Preferred, policy);
    }
    pages_->setCurrentIndex(index);
    pages_->updateGeometry();
}

void ChannelPanel::setOnChanged(std::function<void()> handler)
{
    onChanged_ = std::move(handler);
}

void ChannelPanel::notifyChanged()
{
    if (onChanged_)
        onChanged_();
}

QLineEdit* ChannelPanel::addProbabilityEdit(
    QFormLayout* layout, const QString& label, const QString& objectName, double value, const QString& toolTip)
{
    auto* edit = new QLineEdit(QString::fromStdString(crc::formatNumber(value, 6)).replace(QLatin1Char('.'), QLatin1Char(',')));
    edit->setObjectName(objectName);
    edit->setToolTip(toolTip);
    layout->addRow(label, edit);
    connect(edit, &QLineEdit::textChanged, this, [this] { notifyChanged(); });
    return edit;
}

QWidget* ChannelPanel::createBscPage()
{
    auto* page = new QWidget;
    auto* layout = new QFormLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    const crc::BscSettings defaults;
    bitErrorEdit_ = addProbabilityEdit(layout, QStringLiteral("Вероятность ошибки p:"), QStringLiteral("bitErrorEdit"),
        defaults.bitErrorProbability, QStringLiteral("Вероятность искажения одного бита, интервал (0; 0,5]"));
    return page;
}

QWidget* ChannelPanel::createGilbertPage()
{
    auto* page = new QWidget;
    auto* layout = new QFormLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    const crc::GilbertSettings defaults;
    goodToBadEdit_ = addProbabilityEdit(layout, QStringLiteral("P_ХП (хорошее → плохое):"),
        QStringLiteral("goodToBadEdit"), defaults.goodToBad,
        QStringLiteral("Вероятность перехода из хорошего состояния в плохое за один бит"));
    badToGoodEdit_ = addProbabilityEdit(layout, QStringLiteral("P_ПХ (плохое → хорошее):"),
        QStringLiteral("badToGoodEdit"), defaults.badToGood,
        QStringLiteral("Вероятность перехода из плохого состояния в хорошее за один бит"));
    errorInBadEdit_ = addProbabilityEdit(layout, QStringLiteral("p_П (ошибка в плохом):"),
        QStringLiteral("errorInBadEdit"), defaults.errorInBad,
        QStringLiteral("Вероятность ошибки бита в плохом состоянии"));
    errorInGoodEdit_ = addProbabilityEdit(layout, QStringLiteral("p_Х (ошибка в хорошем):"),
        QStringLiteral("errorInGoodEdit"), defaults.errorInGood,
        QStringLiteral("Вероятность ошибки бита в хорошем состоянии"));
    gilbertSummaryLabel_ = new QLabel;
    gilbertSummaryLabel_->setWordWrap(true);
    layout->addRow(gilbertSummaryLabel_);
    return page;
}

std::vector<ChannelPanel::ProbabilityInput> ChannelPanel::activeInputs()
{
    if (params_.kind == crc::ChannelKind::Bsc)
        return {{crc::Field::BitErrorProbability, QStringLiteral("p"), bitErrorEdit_, &params_.bsc.bitErrorProbability}};
    crc::GilbertSettings& gilbert = params_.gilbert;
    return {
        {crc::Field::GoodToBad, QStringLiteral("P_ХП"), goodToBadEdit_, &gilbert.goodToBad},
        {crc::Field::BadToGood, QStringLiteral("P_ПХ"), badToGoodEdit_, &gilbert.badToGood},
        {crc::Field::ErrorInBad, QStringLiteral("p_П"), errorInBadEdit_, &gilbert.errorInBad},
        {crc::Field::ErrorInGood, QStringLiteral("p_Х"), errorInGoodEdit_, &gilbert.errorInGood},
    };
}

QWidget* ChannelPanel::widgetFor(crc::Field field) const
{
    switch (field) {
    case crc::Field::GoodToBad:
        return goodToBadEdit_;
    case crc::Field::BadToGood:
        return badToGoodEdit_;
    case crc::Field::ErrorInBad:
        return errorInBadEdit_;
    case crc::Field::ErrorInGood:
        return errorInGoodEdit_;
    default:
        return bitErrorEdit_;
    }
}

std::vector<FieldError> ChannelPanel::validate()
{
    params_.kind = kindCombo_->currentIndex() == 0 ? crc::ChannelKind::Bsc : crc::ChannelKind::Gilbert;
    std::vector<FieldError> errors;
    for (const ProbabilityInput& input : activeInputs()) {
        const std::optional<double> value = parseDecimal(input.edit->text());
        if (value)
            *input.target = *value;
        else
            errors.push_back({input.edit, notNumberMessage(input.name)});
    }
    for (const crc::ValidationError& error : crc::validateChannel(params_)) {
        addUnlessReported(errors, {widgetFor(error.field), QString::fromStdString(error.message)});
    }
    highlightFields({bitErrorEdit_, goodToBadEdit_, badToGoodEdit_, errorInBadEdit_, errorInGoodEdit_}, errors);
    showGilbertSummary(errors.empty());
    return errors;
}

crc::ChannelParams ChannelPanel::params() const
{
    return params_;
}

void ChannelPanel::showGilbertSummary(bool valid)
{
    if (!valid || params_.kind != crc::ChannelKind::Gilbert) {
        gilbertSummaryLabel_->setText(QStringLiteral("Расчёт: P_П = —, p̄ = —, 1/P_ПХ = —"));
        return;
    }
    const crc::GilbertSettings& gilbert = params_.gilbert;
    gilbertSummaryLabel_->setText(
        QStringLiteral("Расчёт: доля плохого состояния P_П = %1\nсредняя вероятность ошибки p̄ = %2\n"
                       "средняя длина пачки 1/P_ПХ = %3 бит")
            .arg(formatValue(gilbert.badStateShare()), formatValue(gilbert.meanBitErrorProbability()),
                formatValue(gilbert.meanBurstLength())));
}
