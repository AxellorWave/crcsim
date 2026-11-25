#include "main_window.h"

#include "analysis_panel.h"
#include "batch_panel.h"
#include "channel_panel.h"
#include "coding_panel.h"
#include "control_panel.h"
#include "model_diagram.h"
#include "results_table.h"
#include "run_report.h"
#include "selection_panel.h"
#include "theme.h"
#include "transfer_fields.h"
#include "user_guide_view.h"
#include "walkthrough_panel.h"

#include "results.h"

#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>

#include <exception>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , codingPanel_(new CodingPanel)
    , channelPanel_(new ChannelPanel)
    , controlPanel_(new ControlPanel)
    , errorLabel_(new QLabel)
    , diagram_(new ModelDiagram)
    , startButton_(new QPushButton(QStringLiteral("Старт")))
    , stopButton_(new QPushButton(QStringLiteral("Стоп")))
    , progressBar_(new QProgressBar)
    , countersLabel_(new QLabel)
    , statusLabel_(new QLabel)
    , resultsTable_(new ResultsTable)
    , clearButton_(new QPushButton(QStringLiteral("Очистить")))
    , saveButton_(new QPushButton(QStringLiteral("Сохранить в CSV")))
    , analysisPanel_(new AnalysisPanel)
    , tabs_(new QTabWidget)
    , walkthroughPanel_(new WalkthroughPanel)
    , runner_(this)
{
    setWindowTitle(QStringLiteral("Исследование циклических кодов в режиме обнаружения ошибок"));
    startButton_->setObjectName(QStringLiteral("startButton"));
    stopButton_->setObjectName(QStringLiteral("stopButton"));

    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(createParameterColumn());
    splitter->addWidget(createRunColumn());
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setChildrenCollapsible(false);
    splitter->setSizes({440, 880});
    setCentralWidget(splitter);
    resize(1320, 780);

    connectActions();
    countersLabel_->setText(countersText({}));
    statusLabel_->setText(QStringLiteral("Задайте параметры и нажмите «Старт»."));
    setRunning(false);
    revalidate();
}

QWidget* MainWindow::createParameterColumn()
{
    errorLabel_->setObjectName(QStringLiteral("errorLabel"));
    errorLabel_->setWordWrap(true);
    keepTextColor(errorLabel_, &Theme::dangerText);

    auto* column = new QWidget;
    auto* layout = new QVBoxLayout(column);
    layout->addWidget(codingPanel_);
    layout->addWidget(channelPanel_);
    layout->addWidget(controlPanel_);
    layout->addWidget(errorLabel_);
    layout->addStretch();
    column->setMinimumWidth(400);
    return column;
}

QWidget* MainWindow::createRunColumn()
{
    tabs_->setObjectName(QStringLiteral("resultTabs"));
    tabs_->addTab(createSimulationTab(), QStringLiteral("Моделирование"));
    tabs_->addTab(analysisPanel_, QStringLiteral("Анализ кода"));
    tabs_->addTab(walkthroughPanel_, QStringLiteral("Разбор блока"));
    batchPanel_ = new BatchPanel;
    selectionPanel_ = new SelectionPanel;
    tabs_->addTab(batchPanel_, QStringLiteral("Пакет"));
    tabs_->addTab(selectionPanel_, QStringLiteral("Подбор кода"));
    tabs_->addTab(createUserGuideView(), QStringLiteral("Руководство"));
    return tabs_;
}

QWidget* MainWindow::createSimulationTab()
{
    countersLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    statusLabel_->setObjectName(QStringLiteral("statusLabel"));
    statusLabel_->setWordWrap(true);
    statusLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto* column = new QWidget;
    auto* layout = new QVBoxLayout(column);
    layout->addWidget(diagram_);
    layout->addWidget(createRunControls());
    layout->addWidget(countersLabel_);
    layout->addWidget(statusLabel_);
    transferFields_ = new TransferFields;
    layout->addWidget(transferFields_);
    layout->addWidget(resultsTable_, 1);
    layout->addWidget(createTableControls());
    return column;
}

QWidget* MainWindow::createRunControls()
{
    progressBar_->setRange(0, 1000);
    progressBar_->setValue(0);
    progressBar_->setFormat(QStringLiteral("%p %"));
    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(startButton_);
    layout->addWidget(stopButton_);
    layout->addWidget(progressBar_, 1);
    return row;
}

QWidget* MainWindow::createTableControls()
{
    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addStretch();
    layout->addWidget(clearButton_);
    layout->addWidget(saveButton_);
    return row;
}

void MainWindow::connectActions()
{
    codingPanel_->setOnChanged([this] { revalidate(); });
    channelPanel_->setOnChanged([this] { revalidate(); });
    controlPanel_->setOnChanged([this] { revalidate(); });
    connect(startButton_, &QPushButton::clicked, this, [this] { startSimulation(); });
    connect(stopButton_, &QPushButton::clicked, this, [this] {
        runner_.cancel();
        stopButton_->setEnabled(false);
        statusLabel_->setText(QStringLiteral("Остановка…"));
    });
    connect(clearButton_, &QPushButton::clicked, this, [this] { clearResults(); });
    connect(saveButton_, &QPushButton::clicked, this, [this] { saveResults(); });
    analysisPanel_->setOnSpectrum([this](const crc::CyclicCode& code, const crc::WeightSpectrum& spectrum) {
        resultsTable_->rememberSpectrum(code, spectrum);
    });
    connectBatchAndSelection();
}

void MainWindow::connectBatchAndSelection()
{
    batchPanel_->setOnResult([this](const crc::SimulationResult& result) { addBatchResult(result); });
    transferFields_->setOnChanged([this] {
        resultsTable_->setTransfer(transferFields_->settings());
        selectionPanel_->transferFields()->copyFrom(*transferFields_);
    });
    selectionPanel_->setOnTransferChanged([this] { transferFields_->copyFrom(*selectionPanel_->transferFields()); });
}

void MainWindow::addBatchResult(const crc::SimulationResult& result)
{
    resultsTable_->addResult(result);
    analysisPanel_->setResults(resultsTable_->results());
    updateTableButtons();
}

void MainWindow::revalidate()
{
    std::vector<FieldError> errors = codingPanel_->validate();
    updateAnalysisCode(errors.empty());
    const std::vector<FieldError> channelErrors = channelPanel_->validate();
    updateWalkthrough(joinFieldErrors(errors), channelErrors.empty());
    const std::vector<FieldError> controlErrors = controlPanel_->validate();
    errors.insert(errors.end(), channelErrors.begin(), channelErrors.end());
    selectionPanel_->setChannel(channelErrors.empty() ? std::optional<crc::ChannelParams>(channelPanel_->params()) : std::nullopt);
    errors.insert(errors.end(), controlErrors.begin(), controlErrors.end());
    parametersValid_ = errors.empty();
    errorLabel_->setText(joinFieldErrors(errors));
    errorLabel_->setVisible(!parametersValid_);
    startButton_->setEnabled(parametersValid_ && !runner_.running());
}

void MainWindow::updateAnalysisCode(bool codingValid)
{
    analysisPanel_->setCode(codingValid ? std::optional<crc::CyclicCode>(crc::makeCode(codingPanel_->params())) : std::nullopt);
}

void MainWindow::updateWalkthrough(const QString& codingProblem, bool channelValid)
{
    const bool codingValid = codingProblem.isEmpty();
    walkthroughPanel_->setCode(
        codingValid ? std::optional<crc::CyclicCode>(crc::makeCode(codingPanel_->params())) : std::nullopt, codingProblem);
    walkthroughPanel_->setChannel(channelValid ? std::optional<crc::ChannelParams>(channelPanel_->params()) : std::nullopt,
        QStringLiteral("исправьте параметры канала в левой панели"));
}

void MainWindow::setRunning(bool running)
{
    codingPanel_->setEnabled(!running);
    channelPanel_->setEnabled(!running);
    controlPanel_->setEnabled(!running);
    startButton_->setEnabled(!running && parametersValid_);
    stopButton_->setEnabled(running);
    clearButton_->setEnabled(!running);
    updateTableButtons();
}

void MainWindow::updateTableButtons()
{
    const bool hasResults = !resultsTable_->results().empty();
    clearButton_->setEnabled(hasResults && !runner_.running() && !batchPanel_->running());
    saveButton_->setEnabled(hasResults);
}

void MainWindow::startSimulation()
{
    revalidate();
    if (!parametersValid_ || runner_.running())
        return;
    crc::SimulationParams params;
    params.coding = codingPanel_->params();
    params.channel = channelPanel_->params();
    params.control = controlPanel_->params();
    runningControl_ = params.control;

    progressBar_->setValue(0);
    countersLabel_->setText(countersText({}));
    statusLabel_->setText(QStringLiteral("Идёт моделирование…"));
    runner_.start(
        params, [this](const crc::Counters& counters) { showProgress(counters); },
        [this](const crc::SimulationResult& result) { finishSimulation(result); },
        [this](const QString& message) { failSimulation(message); });
    setRunning(true);
}

void MainWindow::showProgress(const crc::Counters& counters)
{
    progressBar_->setValue(progressPermille(counters, runningControl_));
    countersLabel_->setText(countersText(counters));
}

void MainWindow::finishSimulation(const crc::SimulationResult& result)
{
    showProgress(result.counters);
    resultsTable_->addResult(result);
    analysisPanel_->setResults(resultsTable_->results());
    statusLabel_->setText(runSummaryText(result));
    setRunning(false);
}

void MainWindow::failSimulation(const QString& message)
{
    setRunning(false);
    statusLabel_->setText(QStringLiteral("Ошибка моделирования: ") + message);
    QMessageBox::critical(this, QStringLiteral("Ошибка"), QStringLiteral("Не удалось выполнить моделирование:\n") + message);
}

void MainWindow::saveResults()
{
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Сохранить результаты"), QStringLiteral("results.csv"), QStringLiteral("Файлы CSV (*.csv)"));
    if (path.isEmpty())
        return;
    try {
        crc::writeCsv(QFile::encodeName(path).toStdString(), resultsTable_->results());
        statusLabel_->setText(QStringLiteral("Результаты сохранены в файл ") + path);
    } catch (const std::exception& error) {
        QMessageBox::critical(this, QStringLiteral("Ошибка сохранения"),
            QStringLiteral("Не удалось сохранить результаты:\n") + QString::fromUtf8(error.what()));
    }
}

void MainWindow::clearResults()
{
    resultsTable_->clearResults();
    analysisPanel_->setResults(resultsTable_->results());
    updateTableButtons();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    runner_.cancel();
    analysisPanel_->cancel();
    batchPanel_->stop();
    selectionPanel_->stop();
    event->accept();
}
