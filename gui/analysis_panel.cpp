#include "analysis_panel.h"

#include "analysis_plot.h"
#include "help_button.h"
#include "input_parsing.h"
#include "probability_format.h"

#include "results.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

const double requiredUndetectedProbability = 1e-6;
const int summaryColumnWidth = 320;

bool sameCode(const crc::CyclicCode& left, const crc::CyclicCode& right)
{
    return left.n == right.n && left.generator == right.generator;
}

QString codeTitle(const crc::CyclicCode& code)
{
    return QStringLiteral("(%1, %2)").arg(code.n).arg(code.dataBitCount());
}

QTableWidgetItem* rightAligned(const QString& text)
{
    auto* item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}

std::optional<double> validBitErrorProbability(const QString& text)
{
    const std::optional<double> value = parseDecimal(text);
    if (!value || !(*value > 0.0 && *value <= 0.5))
        return std::nullopt;
    return value;
}

}

AnalysisPanel::AnalysisPanel(QWidget* parent)
    : QWidget(parent)
    , calculateButton_(new QPushButton(QStringLiteral("Рассчитать")))
    , busyIndicator_(new QProgressBar)
    , statusLabel_(new QLabel)
    , summaryLabel_(new QLabel)
    , spectrumTable_(new QTableWidget(0, 2))
    , plot_(new AnalysisPlot)
    , probabilityEdit_(new QLineEdit(QStringLiteral("0,005")))
    , probabilityLabel_(new QLabel)
    , runner_(this)
{
    calculateButton_->setObjectName(QStringLiteral("calculateButton"));
    probabilityEdit_->setObjectName(QStringLiteral("analysisProbabilityEdit"));

    auto* body = new QHBoxLayout;
    body->addWidget(createSummaryColumn());
    body->addWidget(createPlotColumn(), 1);
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(createControls());
    layout->addLayout(body, 1);

    connect(calculateButton_, &QPushButton::clicked, this, [this] { startAnalysis(); });
    connect(probabilityEdit_, &QLineEdit::textChanged, this, [this] { updateProbability(); });
    plot_->setMessage(QStringLiteral("Нажмите «Рассчитать», чтобы построить зависимость Pлож(p) для выбранного кода"));
    showSummary();
    updateProbability();
    updateControls();
}

QWidget* AnalysisPanel::createControls()
{
    busyIndicator_->setRange(0, 0);
    busyIndicator_->setFixedWidth(160);
    busyIndicator_->setTextVisible(false);
    QSizePolicy policy = busyIndicator_->sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    busyIndicator_->setSizePolicy(policy);
    statusLabel_->setWordWrap(true);
    statusLabel_->setObjectName(QStringLiteral("analysisStatusLabel"));

    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(calculateButton_);
    layout->addWidget(statusLabel_, 1);
    layout->addWidget(busyIndicator_);
    return row;
}

QWidget* AnalysisPanel::createSummaryColumn()
{
    summaryLabel_->setWordWrap(true);
    summaryLabel_->setTextFormat(Qt::RichText);
    summaryLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    summaryLabel_->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    spectrumTable_->setHorizontalHeaderLabels({QStringLiteral("w"), QStringLiteral("A_w")});
    spectrumTable_->setToolTip(QStringLiteral("Весовой спектр: A_w — число кодовых слов веса w (нулевые не показаны)"));
    spectrumTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    spectrumTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    spectrumTable_->setAlternatingRowColors(true);
    spectrumTable_->verticalHeader()->setVisible(false);
    spectrumTable_->verticalHeader()->setDefaultSectionSize(fontMetrics().height() + 6);
    spectrumTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    spectrumTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    auto* column = new QWidget;
    column->setFixedWidth(summaryColumnWidth);
    auto* layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(summaryLabel_);
    layout->addWidget(new QLabel(QStringLiteral("<b>Весовой спектр</b>")));
    layout->addWidget(spectrumTable_, 1);
    return column;
}

QWidget* AnalysisPanel::createPlotColumn()
{
    probabilityEdit_->setMaximumWidth(110);
    probabilityEdit_->setToolTip(parameterHelpToolTip(HelpTopic::AnalysisProbability));
    probabilityLabel_->setTextFormat(Qt::RichText);
    probabilityLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto* probabilityRow = new QHBoxLayout;
    probabilityRow->addWidget(new QLabel(QStringLiteral("p =")));
    probabilityRow->addWidget(probabilityEdit_);
    probabilityRow->addWidget(createHelpButton(HelpTopic::AnalysisProbability));
    probabilityRow->addWidget(probabilityLabel_, 1);

    auto* column = new QWidget;
    auto* layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(plot_, 1);
    layout->addLayout(probabilityRow);
    return column;
}

void AnalysisPanel::setCode(const std::optional<crc::CyclicCode>& code)
{
    currentCode_ = code;
    updateControls();
}

void AnalysisPanel::setResults(const std::vector<crc::SimulationResult>& results)
{
    results_ = results;
    updatePoints();
}

void AnalysisPanel::setOnSpectrum(SpectrumHandler handler)
{
    onSpectrum_ = std::move(handler);
}

void AnalysisPanel::cancel()
{
    runner_.cancel();
}

void AnalysisPanel::startAnalysis()
{
    if (!currentCode_ || runner_.running())
        return;
    runner_.start(*currentCode_, [this](const crc::CodeAnalysis& analysis) { finishAnalysis(analysis); });
    updateControls();
}

void AnalysisPanel::finishAnalysis(const crc::CodeAnalysis& analysis)
{
    if (analysis.cancelled) {
        updateControls();
        return;
    }
    analysis_ = analysis;
    plot_->setSpectrum(analysis.spectrum);
    plot_->setMessage(QString::fromStdString(analysis.failureReason));
    showSummary();
    fillSpectrumTable();
    updatePoints();
    updateProbability();
    updateControls();
    if (analysis.spectrum && onSpectrum_)
        onSpectrum_(analysis.code, *analysis.spectrum);
}

void AnalysisPanel::updateControls()
{
    const bool running = runner_.running();
    calculateButton_->setEnabled(currentCode_.has_value() && !running);
    busyIndicator_->setVisible(running);
    updateStatus();
}

void AnalysisPanel::updateStatus()
{
    if (runner_.running()) {
        statusLabel_->setText(QStringLiteral("Идёт перебор кодовых слов…"));
        return;
    }
    if (!currentCode_) {
        statusLabel_->setText(QStringLiteral("Параметры кода заданы неверно — исправьте их в панели «Кодирование»."));
        return;
    }
    if (!analysis_) {
        statusLabel_->setText(QStringLiteral("Точный расчёт для кода %1 по весовому спектру.").arg(codeTitle(*currentCode_)));
        return;
    }
    if (!sameCode(analysis_->code, *currentCode_)) {
        statusLabel_->setText(QStringLiteral("Показан код %1; выбран код %2 — нажмите «Рассчитать».")
                                  .arg(codeTitle(analysis_->code), codeTitle(*currentCode_)));
        return;
    }
    statusLabel_->setText(analysis_->computable() ? QStringLiteral("Расчёт выполнен.")
                                                  : QString::fromStdString(analysis_->failureReason));
}

void AnalysisPanel::showSummary()
{
    if (!analysis_) {
        summaryLabel_->setText(QStringLiteral("Код ещё не рассчитан."));
        return;
    }
    const crc::CyclicCode& code = analysis_->code;
    const std::optional<crc::WeightSpectrum>& spectrum = analysis_->spectrum;
    QString text = QStringLiteral("<b>Код %1</b><br>g(x) = %2<br>n = %3, k = %4, h = %5, d = %6")
                       .arg(codeTitle(code), QString::fromStdString(crc::toBinary(code.generator)))
                       .arg(code.n)
                       .arg(code.dataBitCount())
                       .arg(code.checkBitCount())
                       .arg(spectrum ? QString::number(crc::minimumDistance(*spectrum)) : QStringLiteral("—"));
    text += QStringLiteral("<p>Гарантированно обнаруживаются:<br>");
    if (spectrum)
        text += QStringLiteral("• ") + QString::fromStdString(crc::guaranteedErrorsText(*spectrum)) + QStringLiteral(";<br>");
    text += QStringLiteral("• ") + QString::fromStdString(crc::guaranteedBurstsText(code)) + QStringLiteral(".</p>");
    summaryLabel_->setText(text);
}

void AnalysisPanel::fillSpectrumTable()
{
    spectrumTable_->setRowCount(0);
    if (!analysis_ || !analysis_->spectrum)
        return;
    const crc::WeightSpectrum& spectrum = *analysis_->spectrum;
    for (std::size_t weight = 0; weight < spectrum.size(); ++weight) {
        if (spectrum[weight] == 0)
            continue;
        const int row = spectrumTable_->rowCount();
        spectrumTable_->insertRow(row);
        spectrumTable_->setItem(row, 0, rightAligned(QString::number(weight)));
        spectrumTable_->setItem(row, 1, rightAligned(formatCount(spectrum[weight])));
    }
}

void AnalysisPanel::updatePoints()
{
    std::vector<ExperimentPoint> points;
    if (analysis_) {
        for (const crc::SimulationResult& result : results_) {
            if (result.channelCsvName != "BSC" || !sameCode(result.code, analysis_->code) || result.counters.blocks == 0)
                continue;
            const crc::ProbabilityEstimate estimate = crc::summarize(result).undetected;
            const double value = estimate.observed() ? estimate.value() : 3.0 / static_cast<double>(estimate.trials);
            points.push_back({result.meanBitErrorProbability, value, estimate.observed()});
        }
    }
    plot_->setPoints(points);
}

void AnalysisPanel::updateProbability()
{
    const std::optional<double> p = validBitErrorProbability(probabilityEdit_->text());
    plot_->setMarkedProbability(p);
    if (!p) {
        probabilityLabel_->setStyleSheet(QStringLiteral("color: #b01010;"));
        probabilityLabel_->setText(QStringLiteral("p должна быть в интервале (0; 0,5]"));
        return;
    }
    probabilityLabel_->setStyleSheet(QString());
    if (!analysis_ || !analysis_->spectrum) {
        probabilityLabel_->setText(analysis_ ? QStringLiteral("Pлож = — (весовой спектр не вычислен)")
                                             : QStringLiteral("Pлож = — (код не рассчитан)"));
        return;
    }
    const double probability = crc::undetectedProbability(*analysis_->spectrum, *p);
    const bool meets = probability <= requiredUndetectedProbability;
    probabilityLabel_->setText(QStringLiteral("точная Pлож = <b>%1</b> — требование Pлож ≤ 10⁻⁶ %2")
                                   .arg(formatSmallProbability(probability),
                                       meets ? QStringLiteral("выполняется") : QStringLiteral("не выполняется")));
}
