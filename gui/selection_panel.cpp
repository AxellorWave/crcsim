#include "selection_panel.h"

#include "field_error.h"
#include "input_parsing.h"
#include "selection_table.h"
#include "transfer_fields.h"

#include "plan.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

const int permilleScale = 1000;
const int allLengthsIndex = -1;
const int editWidth = 150;

QString channelDescription(const crc::ChannelParams& channel)
{
    const QString name = channel.kind == crc::ChannelKind::Bsc ? QStringLiteral("ДСК") : QStringLiteral("канал Гилберта");
    return name + QStringLiteral(", ") + QString::fromStdString(crc::channelParametersText(channel));
}

int candidatePermille(const crc::Counters& counters, std::uint64_t blocks)
{
    return blocks == 0 ? 0 : static_cast<int>(counters.blocks * permilleScale / blocks);
}

}

SelectionPanel::SelectionPanel(QWidget* parent)
    : QWidget(parent)
    , lengthCombo_(new QComboBox)
    , channelLabel_(new QLabel)
    , transferFields_(new TransferFields)
    , errorLabel_(new QLabel)
    , selectButton_(new QPushButton(QStringLiteral("Подобрать")))
    , stopButton_(new QPushButton(QStringLiteral("Стоп")))
    , progressBar_(new QProgressBar)
    , statusLabel_(new QLabel)
    , table_(new SelectionTable)
    , conclusionLabel_(new QLabel)
    , runner_(this)
{
    blocksEdit_ = createEdit(QStringLiteral("selectionBlocksEdit"), formatCount(crc::defaultSelectionBlocks),
        QStringLiteral("Число блоков N для каждого кода; для Pлож ≤ 10⁻⁶ нужно не меньше 3 000 000"));
    seedEdit_ = createEdit(QStringLiteral("selectionSeedEdit"), QString::number(crc::defaultSelectionSeed),
        QStringLiteral("Одинаковый seed для всех кодов; пусто или 0 — случайный"));
    targetEdit_ = createEdit(QStringLiteral("selectionTargetEdit"), QStringLiteral("1e-6"),
        QStringLiteral("Допустимая вероятность ложного приёма, например 1e-6 или 0,000001"));
    lengthCombo_->setObjectName(QStringLiteral("selectionLengthCombo"));
    for (const int length : crc::knownCodeLengths())
        lengthCombo_->addItem(QString::number(length), length);
    lengthCombo_->addItem(QStringLiteral("все"), allLengthsIndex);
    lengthCombo_->setCurrentIndex(lengthCombo_->findData(15));
    lengthCombo_->setFixedWidth(editWidth);
    channelLabel_->setWordWrap(true);
    errorLabel_->setWordWrap(true);
    errorLabel_->setStyleSheet(QStringLiteral("color: #b01010;"));
    errorLabel_->hide();
    statusLabel_->setWordWrap(true);
    conclusionLabel_->setObjectName(QStringLiteral("selectionConclusion"));
    conclusionLabel_->setWordWrap(true);
    conclusionLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    conclusionLabel_->setStyleSheet(QStringLiteral("font-weight: bold; padding: 4px 0;"));

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(createSettings());
    layout->addWidget(createRunControls());
    layout->addWidget(statusLabel_);
    layout->addWidget(table_, 1);
    layout->addWidget(conclusionLabel_);

    connectActions();
    statusLabel_->setText(QStringLiteral("Каждый известный код выбранной длины моделируется на одном и том же канале."));
    progressBar_->setRange(0, permilleScale);
    progressBar_->setValue(0);
    updateButtons();
}

QLineEdit* SelectionPanel::createEdit(const QString& objectName, const QString& text, const QString& toolTip)
{
    auto* edit = new QLineEdit(text);
    edit->setObjectName(objectName);
    edit->setToolTip(toolTip);
    edit->setFixedWidth(editWidth);
    return edit;
}

QWidget* SelectionPanel::createSettings()
{
    auto* box = new QGroupBox(QStringLiteral("Условия подбора"));
    auto* grid = new QGridLayout(box);
    grid->addWidget(new QLabel(QStringLiteral("Длина n:")), 0, 0);
    grid->addWidget(lengthCombo_, 0, 1);
    grid->addWidget(new QLabel(QStringLiteral("Канал:")), 0, 2);
    grid->addWidget(channelLabel_, 0, 3, 1, 2);
    grid->addWidget(new QLabel(QStringLiteral("Число блоков N:")), 1, 0);
    grid->addWidget(blocksEdit_, 1, 1);
    grid->addWidget(new QLabel(QStringLiteral("Seed:")), 1, 2);
    grid->addWidget(seedEdit_, 1, 3);
    grid->addWidget(new QLabel(QStringLiteral("Требование Pлож ≤")), 2, 0);
    grid->addWidget(targetEdit_, 2, 1);
    grid->addWidget(transferFields_, 2, 2, 1, 3);
    grid->addWidget(errorLabel_, 3, 0, 1, 5);
    grid->setColumnStretch(4, 1);
    return box;
}

QWidget* SelectionPanel::createRunControls()
{
    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(selectButton_);
    layout->addWidget(stopButton_);
    layout->addWidget(progressBar_, 1);
    return row;
}

void SelectionPanel::connectActions()
{
    connect(selectButton_, &QPushButton::clicked, this, [this] { startSelection(); });
    connect(stopButton_, &QPushButton::clicked, this, [this] { stop(); });
    transferFields_->setOnChanged([this] { transferChanged(); });
}

void SelectionPanel::setChannel(const std::optional<crc::ChannelParams>& channel)
{
    channel_ = channel;
    channelLabel_->setText(channel ? channelDescription(*channel) + QStringLiteral(" — используются параметры канала из панели слева")
                                   : QStringLiteral("параметры канала в панели слева заданы неверно"));
    updateButtons();
}

TransferFields* SelectionPanel::transferFields()
{
    return transferFields_;
}

void SelectionPanel::setOnTransferChanged(std::function<void()> handler)
{
    onTransferChanged_ = std::move(handler);
}

bool SelectionPanel::running() const
{
    return runner_.running();
}

void SelectionPanel::updateButtons()
{
    const bool busy = runner_.running();
    selectButton_->setEnabled(!busy && channel_.has_value());
    stopButton_->setEnabled(busy && !stopRequested_);
    lengthCombo_->setEnabled(!busy);
    blocksEdit_->setEnabled(!busy);
    seedEdit_->setEnabled(!busy);
    targetEdit_->setEnabled(!busy);
}

QWidget* SelectionPanel::widgetFor(crc::Field field) const
{
    if (field == crc::Field::BlockLimit)
        return blocksEdit_;
    if (field == crc::Field::Target)
        return targetEdit_;
    if (field == crc::Field::FileSize || field == crc::Field::BitRate)
        return transferFields_;
    return lengthCombo_;
}

std::optional<crc::SelectionRequest> SelectionPanel::readRequest()
{
    crc::SelectionRequest request;
    std::vector<FieldError> errors;
    const int length = lengthCombo_->currentData().toInt();
    request.codeLength = length == allLengthsIndex ? std::nullopt : std::optional<int>(length);
    request.channel = channel_.value_or(crc::ChannelParams{});
    const std::optional<std::uint64_t> blocks = parseUnsigned(blocksEdit_->text());
    if (blocks)
        request.blocks = *blocks;
    else
        errors.push_back({blocksEdit_, QStringLiteral("N: введите целое положительное число")});
    const std::optional<std::uint64_t> seed = seedEdit_->text().trimmed().isEmpty() ? 0 : parseUnsigned(seedEdit_->text());
    if (seed)
        request.seed = *seed;
    else
        errors.push_back({seedEdit_, QStringLiteral("seed: введите целое неотрицательное число или оставьте поле пустым")});
    const std::optional<double> target = parseDecimal(targetEdit_->text());
    if (target)
        request.target = *target;
    else
        errors.push_back({targetEdit_, QStringLiteral("требование Pлож: введите число, например 1e-6")});
    const std::optional<crc::TransferSettings> transfer = transferFields_->settings();
    if (transfer)
        request.transfer = *transfer;
    else
        errors.push_back({transferFields_, QStringLiteral("исправьте размер файла M или скорость")});
    for (const crc::ValidationError& error : crc::validateSelection(request)) {
        addUnlessReported(errors, {widgetFor(error.field), QString::fromStdString(error.message)});
    }
    highlightFields({blocksEdit_, seedEdit_, targetEdit_}, errors);
    errorLabel_->setText(joinFieldErrors(errors));
    errorLabel_->setVisible(!errors.empty());
    if (!errors.empty() || !channel_)
        return std::nullopt;
    return request;
}

void SelectionPanel::startSelection()
{
    if (runner_.running())
        return;
    const std::optional<crc::SelectionRequest> request = readRequest();
    if (!request)
        return;
    request_ = *request;
    candidates_ = crc::candidateCodes(request_.codeLength);
    results_.clear();
    stopRequested_ = false;
    conclusionLabel_->clear();
    refreshRows();
    startCandidate(0);
}

void SelectionPanel::stop()
{
    if (!runner_.running())
        return;
    stopRequested_ = true;
    runner_.cancel();
    statusLabel_->setText(QStringLiteral("Остановка…"));
    updateButtons();
}

void SelectionPanel::startCandidate(std::size_t index)
{
    current_ = index;
    statusLabel_->setText(QStringLiteral("Моделируется код %1 (%2 из %3)…")
                              .arg(QString::fromStdString(crc::codeName(candidates_[index])))
                              .arg(index + 1)
                              .arg(candidates_.size()));
    showProgress({});
    runner_.start(
        crc::candidateParams(candidates_[index], request_), [this](const crc::Counters& counters) { showProgress(counters); },
        [this](const crc::SimulationResult& result) { finishCandidate(result); },
        [this](const QString& message) { failCandidate(message); });
    updateButtons();
}

void SelectionPanel::showProgress(const crc::Counters& counters)
{
    const int permille = candidatePermille(counters, request_.blocks);
    const double done = (static_cast<double>(current_) * permilleScale + permille) / static_cast<double>(candidates_.size());
    progressBar_->setValue(static_cast<int>(done));
    progressBar_->setFormat(QStringLiteral("код %1 из %2 · всего %p %").arg(current_ + 1).arg(candidates_.size()));
}

void SelectionPanel::finishCandidate(const crc::SimulationResult& result)
{
    if (result.cancelled) {
        finishSelection(QStringLiteral("Подбор прерван: проверено кодов %1 из %2.").arg(results_.size()).arg(candidates_.size()));
        return;
    }
    results_.push_back(result);
    refreshRows();
    if (current_ + 1 < candidates_.size() && !stopRequested_) {
        startCandidate(current_ + 1);
        return;
    }
    progressBar_->setValue(permilleScale);
    finishSelection(QStringLiteral("Подбор завершён: проверено кодов %1.").arg(results_.size()));
}

void SelectionPanel::failCandidate(const QString& message)
{
    finishSelection(QStringLiteral("Ошибка моделирования: ") + message);
}

void SelectionPanel::finishSelection(const QString& status)
{
    statusLabel_->setText(status);
    stopRequested_ = false;
    refreshRows();
    updateButtons();
}

void SelectionPanel::refreshRows()
{
    std::vector<crc::SelectionRow> rows;
    for (const crc::SimulationResult& result : results_)
        rows.push_back(crc::evaluateCandidate(result, request_));
    const crc::BestCodes best = crc::chooseBest(rows);
    table_->showRows(rows, best);
    const bool complete = !rows.empty() && rows.size() == candidates_.size();
    conclusionLabel_->setText(complete ? QString::fromStdString(crc::selectionConclusion(request_, rows, best)) : QString());
}

void SelectionPanel::transferChanged()
{
    const std::optional<crc::TransferSettings> transfer = transferFields_->settings();
    if (transfer) {
        request_.transfer = *transfer;
        refreshRows();
    }
    if (onTransferChanged_)
        onTransferChanged_();
}
