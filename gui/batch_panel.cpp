#include "batch_panel.h"

#include "plan_table.h"
#include "run_report.h"

#include "results.h"

#include <QCheckBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

#include <exception>

namespace {

const int permilleScale = 1000;

QString experimentCount(std::size_t count)
{
    const std::size_t lastTwo = count % 100;
    const std::size_t last = count % 10;
    QString word = QStringLiteral("опытов");
    if (last == 1 && lastTwo != 11)
        word = QStringLiteral("опыт");
    else if (last >= 2 && last <= 4 && (lastTwo < 12 || lastTwo > 14))
        word = QStringLiteral("опыта");
    return QString::number(static_cast<qulonglong>(count)) + QLatin1Char(' ') + word;
}

QString experimentTitle(const crc::PlannedExperiment& experiment)
{
    if (!experiment.label.empty())
        return QString::fromStdString(experiment.label);
    const crc::SimulationParams& params = experiment.params;
    const QString channel = params.channel.kind == crc::ChannelKind::Bsc ? QStringLiteral("ДСК") : QStringLiteral("Гилберт");
    return QString::fromStdString(crc::codeName(crc::makeCode(params.coding))) + QStringLiteral(", ") + channel
        + QLatin1Char(' ') + QString::fromStdString(crc::channelParametersText(params.channel));
}

std::string localPath(const QString& path)
{
    return QFile::encodeName(path).toStdString();
}

}

BatchPanel::BatchPanel(QWidget* parent)
    : QWidget(parent)
    , openButton_(new QPushButton(QStringLiteral("Открыть план…")))
    , methodichkaButton_(new QPushButton(QStringLiteral("План из методички")))
    , gilbertCheck_(new QCheckBox(QStringLiteral("с каналом Гилберта")))
    , savePlanButton_(new QPushButton(QStringLiteral("Сохранить план…")))
    , planLabel_(new QLabel)
    , table_(new PlanTable)
    , runButton_(new QPushButton(QStringLiteral("Запустить все")))
    , stopButton_(new QPushButton(QStringLiteral("Стоп")))
    , progressBar_(new QProgressBar)
    , statusLabel_(new QLabel)
    , saveResultsButton_(new QPushButton(QStringLiteral("Сохранить результаты в CSV")))
    , runner_(this)
{
    gilbertCheck_->setChecked(true);
    gilbertCheck_->setToolTip(QStringLiteral(
        "Добавить опыты на канале Гилберта: P_ХП = 0,003; 0,001; 0,0005, P_ПХ = 0,1, p_П = 0,5, p_Х = 0"));
    methodichkaButton_->setToolTip(
        QStringLiteral("Все 17 кодов на ДСК p = 0,05; 0,01; 0,005, N = 3 000 000, seed = 42"));
    planLabel_->setText(QStringLiteral("План не загружен. Откройте файл CSV или JSON либо возьмите план из методички."));
    planLabel_->setWordWrap(true);
    statusLabel_->setWordWrap(true);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(createPlanControls());
    layout->addWidget(planLabel_);
    layout->addWidget(table_, 1);
    layout->addWidget(createRunControls());
    layout->addWidget(statusLabel_);
    layout->addWidget(createResultControls());

    connectActions();
    showOverallProgress(0);
    updateButtons();
}

QWidget* BatchPanel::createPlanControls()
{
    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(openButton_);
    layout->addWidget(methodichkaButton_);
    layout->addWidget(gilbertCheck_);
    layout->addStretch();
    layout->addWidget(savePlanButton_);
    return row;
}

QWidget* BatchPanel::createRunControls()
{
    progressBar_->setRange(0, permilleScale);
    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(runButton_);
    layout->addWidget(stopButton_);
    layout->addWidget(progressBar_, 1);
    return row;
}

QWidget* BatchPanel::createResultControls()
{
    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new QLabel(QStringLiteral("Результаты опытов добавляются в таблицу на вкладке «Моделирование».")));
    layout->addStretch();
    layout->addWidget(saveResultsButton_);
    return row;
}

void BatchPanel::connectActions()
{
    connect(openButton_, &QPushButton::clicked, this, [this] { openPlan(); });
    connect(methodichkaButton_, &QPushButton::clicked, this, [this] { loadMethodichkaPlan(); });
    connect(savePlanButton_, &QPushButton::clicked, this, [this] { savePlan(); });
    connect(runButton_, &QPushButton::clicked, this, [this] { startAll(); });
    connect(stopButton_, &QPushButton::clicked, this, [this] { stop(); });
    connect(saveResultsButton_, &QPushButton::clicked, this, [this] { saveResults(); });
}

void BatchPanel::setOnResult(ResultHandler handler)
{
    onResult_ = std::move(handler);
}

bool BatchPanel::running() const
{
    return runner_.running();
}

void BatchPanel::updateButtons()
{
    const bool busy = runner_.running();
    openButton_->setEnabled(!busy);
    methodichkaButton_->setEnabled(!busy);
    gilbertCheck_->setEnabled(!busy);
    savePlanButton_->setEnabled(!busy && !plan_.empty());
    runButton_->setEnabled(!busy && !plan_.empty());
    stopButton_->setEnabled(busy && !stopRequested_);
    saveResultsButton_->setEnabled(!results_.empty());
}

void BatchPanel::setPlan(const crc::ExperimentPlan& plan, const QString& source)
{
    plan_ = plan;
    results_.clear();
    table_->showPlan(plan_);
    planLabel_->setText(QStringLiteral("%1 — %2").arg(source, experimentCount(plan_.size())));
    statusLabel_->setText(QStringLiteral("Нажмите «Запустить все», чтобы выполнить опыты по порядку."));
    showOverallProgress(0);
    updateButtons();
}

bool BatchPanel::loadPlanFile(const QString& path)
{
    try {
        setPlan(crc::loadPlan(localPath(path)), QStringLiteral("План: ") + QFileInfo(path).fileName());
        return true;
    } catch (const std::exception& error) {
        QMessageBox::critical(this, QStringLiteral("Ошибка в плане"),
            QStringLiteral("Не удалось загрузить план опытов:\n") + QString::fromUtf8(error.what()));
        return false;
    }
}

void BatchPanel::loadMethodichkaPlan()
{
    const bool withGilbert = gilbertCheck_->isChecked();
    setPlan(crc::methodichkaPlan(withGilbert),
        withGilbert ? QStringLiteral("План из методички (ДСК и канал Гилберта)") : QStringLiteral("План из методички (ДСК)"));
}

void BatchPanel::openPlan()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Открыть план опытов"), QString(), QStringLiteral("План опытов (*.csv *.json)"));
    if (!path.isEmpty())
        loadPlanFile(path);
}

void BatchPanel::savePlan()
{
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Сохранить план опытов"),
        QStringLiteral("plan.csv"), QStringLiteral("План опытов CSV (*.csv);;План опытов JSON (*.json)"));
    if (path.isEmpty())
        return;
    try {
        crc::savePlan(localPath(path), plan_);
        statusLabel_->setText(QStringLiteral("План сохранён в файл ") + path);
    } catch (const std::exception& error) {
        QMessageBox::critical(this, QStringLiteral("Ошибка сохранения"),
            QStringLiteral("Не удалось сохранить план:\n") + QString::fromUtf8(error.what()));
    }
}

void BatchPanel::saveResults()
{
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Сохранить результаты"), QStringLiteral("batch_results.csv"), QStringLiteral("Файлы CSV (*.csv)"));
    if (path.isEmpty())
        return;
    try {
        crc::writeCsv(localPath(path), results_);
        statusLabel_->setText(QStringLiteral("Результаты сохранены в файл ") + path);
    } catch (const std::exception& error) {
        QMessageBox::critical(this, QStringLiteral("Ошибка сохранения"),
            QStringLiteral("Не удалось сохранить результаты:\n") + QString::fromUtf8(error.what()));
    }
}

void BatchPanel::startAll()
{
    if (plan_.empty() || runner_.running())
        return;
    results_.clear();
    stopRequested_ = false;
    for (std::size_t index = 0; index < plan_.size(); ++index) {
        table_->setStatus(static_cast<int>(index), ExperimentStatus::Waiting);
        table_->setSeed(static_cast<int>(index), plan_[index].params.control.seed);
    }
    startExperiment(0);
}

void BatchPanel::stop()
{
    if (!runner_.running())
        return;
    stopRequested_ = true;
    runner_.cancel();
    statusLabel_->setText(QStringLiteral("Остановка…"));
    updateButtons();
}

void BatchPanel::startExperiment(std::size_t index)
{
    current_ = index;
    table_->setStatus(static_cast<int>(index), ExperimentStatus::Running);
    showOverallProgress(0);
    statusLabel_->setText(QStringLiteral("Выполняется опыт %1 из %2: %3")
                              .arg(index + 1)
                              .arg(plan_.size())
                              .arg(experimentTitle(plan_[index])));
    runner_.start(
        plan_[index].params, [this](const crc::Counters& counters) { showProgress(counters); },
        [this](const crc::SimulationResult& result) { finishExperiment(result); },
        [this](const QString& message) { failExperiment(message); });
    updateButtons();
}

void BatchPanel::showOverallProgress(int currentPermille)
{
    const std::size_t count = plan_.size();
    if (count == 0) {
        progressBar_->setValue(0);
        progressBar_->setFormat(QStringLiteral("нет плана"));
        return;
    }
    const double done = (static_cast<double>(current_) * permilleScale + currentPermille) / static_cast<double>(count);
    progressBar_->setValue(static_cast<int>(done));
    progressBar_->setFormat(QStringLiteral("опыт %1 из %2 · текущий %3 % · всего %p %")
                                .arg(current_ + 1)
                                .arg(count)
                                .arg(currentPermille / 10));
}

void BatchPanel::showProgress(const crc::Counters& counters)
{
    const int permille = progressPermille(counters, plan_[current_].params.control);
    table_->setStatus(static_cast<int>(current_), ExperimentStatus::Running, permille / 10);
    showOverallProgress(permille);
}

void BatchPanel::finishExperiment(const crc::SimulationResult& result)
{
    const int row = static_cast<int>(current_);
    if (result.cancelled) {
        table_->setStatus(row, ExperimentStatus::Cancelled);
        finishBatch(QStringLiteral("Пакет остановлен: выполнено %1 из %2.").arg(results_.size()).arg(plan_.size()));
        return;
    }
    table_->setStatus(row, ExperimentStatus::Done);
    table_->setSeed(row, result.seed);
    results_.push_back(result);
    if (onResult_)
        onResult_(result);
    if (current_ + 1 < plan_.size() && !stopRequested_) {
        startExperiment(current_ + 1);
        return;
    }
    showOverallProgress(permilleScale);
    finishBatch(QStringLiteral("Пакет выполнен: %1, результаты добавлены в таблицу на вкладке «Моделирование».")
                    .arg(experimentCount(results_.size())));
}

void BatchPanel::failExperiment(const QString& message)
{
    table_->setStatus(static_cast<int>(current_), ExperimentStatus::Cancelled);
    finishBatch(QStringLiteral("Ошибка в опыте %1: %2").arg(current_ + 1).arg(message));
}

void BatchPanel::finishBatch(const QString& status)
{
    statusLabel_->setText(status);
    stopRequested_ = false;
    updateButtons();
}
