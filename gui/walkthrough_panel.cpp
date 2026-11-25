#include "walkthrough_panel.h"

#include "bit_cells_edit.h"
#include "media_icons.h"
#include "stage_strip.h"
#include "walkthrough_canvas.h"
#include "walkthrough_style.h"

#include "decoder.h"
#include "encoder.h"

#include <QCheckBox>
#include <QFontDatabase>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>
#include <utility>

namespace {

const int maxVisualLength = 31;
const int compactEditorLength = 22;
const int defaultSpeed = 4;
const int maxSpeed = 10;
const double speedUnit = 4.0;
const int holdMs = 700;
const int sceneHoldMs = 1100;
const int minFlashStepMs = 650;
const int scrollMs = 380;

class CanvasScrollArea : public QScrollArea {
public:
    explicit CanvasScrollArea(std::function<void()> onResized)
        : onResized_(std::move(onResized))
    {
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QScrollArea::resizeEvent(event);
        onResized_();
    }

private:
    std::function<void()> onResized_;
};

QPushButton* playbackButton(MediaGlyph glyph, const QString& text, const QString& toolTip)
{
    auto* button = new QPushButton(mediaIcon(glyph), text);
    button->setToolTip(toolTip);
    return button;
}

QString speedText(double factor)
{
    return QString::number(factor, 'g', 3).replace(QLatin1Char('.'), QLatin1Char(',')) + QStringLiteral("×");
}

int stageIndex(WalkStage stage)
{
    return static_cast<int>(stage);
}

}

WalkthroughPanel::WalkthroughPanel(QWidget* parent)
    : QWidget(parent)
    , codeLabel_(new QLabel)
    , textToggle_(new QCheckBox(QStringLiteral("Текстом")))
    , dataEdit_(new BitCellsEdit)
    , errorEdit_(new BitCellsEdit)
    , errorCaption_(new QLabel(QStringLiteral("Ошибки E:")))
    , inputs_(nullptr)
    , randomDataButton_(new QPushButton(QStringLiteral("Случайные данные")))
    , noErrorsButton_(new QPushButton(QStringLiteral("Без ошибок")))
    , channelErrorsButton_(new QPushButton(QStringLiteral("Ошибка из канала")))
    , falseAcceptButton_(new QPushButton(QStringLiteral("Ложный приём")))
    , noteLabel_(new QLabel)
    , limitLabel_(new QLabel(QStringLiteral("Наглядный разбор доступен для n ≤ 31 — ниже показан текстовый разбор")))
    , pages_(new QStackedWidget)
    , visualPage_(nullptr)
    , textView_(new QPlainTextEdit)
    , messageLabel_(new QLabel)
    , strip_(new StageStrip)
    , captionLabel_(new QLabel)
    , stepLabel_(new QLabel)
    , scrollArea_(nullptr)
    , canvas_(new WalkthroughCanvas)
    , restartButton_(playbackButton(MediaGlyph::Restart, QStringLiteral("Сначала"), QStringLiteral("Вернуться к первому шагу")))
    , backButton_(playbackButton(MediaGlyph::Back, QStringLiteral("Назад"), QStringLiteral("Предыдущий шаг")))
    , playButton_(playbackButton(MediaGlyph::Play, QStringLiteral("Воспроизвести"), QStringLiteral("Проиграть разбор до конца")))
    , pauseButton_(playbackButton(MediaGlyph::Pause, QStringLiteral("Пауза"), QStringLiteral("Остановить анимацию")))
    , stepButton_(playbackButton(MediaGlyph::Step, QStringLiteral("Шаг"), QStringLiteral("Показать следующий шаг")))
    , speedSlider_(new QSlider(Qt::Horizontal))
    , speedLabel_(new QLabel)
    , random_(crc::makeRandomSeed())
{
    setObjectName(QStringLiteral("walkthroughPanel"));
    inputs_ = createInputs();
    visualPage_ = createVisualPage();

    textView_->setReadOnly(true);
    textView_->setLineWrapMode(QPlainTextEdit::NoWrap);
    QFont textFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    textFont.setPointSizeF(13.0);
    textView_->setFont(textFont);
    messageLabel_->setWordWrap(true);
    messageLabel_->setAlignment(Qt::AlignCenter);
    messageLabel_->setStyleSheet(QStringLiteral("color: #4A5568; font-size: 14pt;"));
    pages_->addWidget(visualPage_);
    pages_->addWidget(textView_);
    pages_->addWidget(messageLabel_);

    limitLabel_->setStyleSheet(QStringLiteral("color: #b45309;"));
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(inputs_);
    layout->addWidget(limitLabel_);
    layout->addWidget(pages_, 1);

    animation_.setStartValue(0.0);
    animation_.setEndValue(1.0);
    holdTimer_.setSingleShot(true);
    scrollAnimation_.setPropertyName("value");
    scrollAnimation_.setDuration(scrollMs);
    scrollAnimation_.setEasingCurve(QEasingCurve::InOutCubic);
    connectActions();
    setCode(std::nullopt, QStringLiteral("не заданы параметры кода"));
}

QWidget* WalkthroughPanel::createInputs()
{
    codeLabel_->setTextFormat(Qt::RichText);
    codeLabel_->setStyleSheet(QStringLiteral("color: #14213D; font-size: 13pt;"));
    noteLabel_->setWordWrap(true);
    noteLabel_->setStyleSheet(QStringLiteral("color: #4A5568;"));
    dataEdit_->setObjectName(QStringLiteral("walkthroughDataEdit"));
    errorEdit_->setObjectName(QStringLiteral("walkthroughErrorEdit"));
    dataEdit_->setToolTip(QStringLiteral("Информационные биты D: щёлкните по клетке, чтобы изменить бит"));
    errorEdit_->setToolTip(QStringLiteral("Вектор ошибок E: щёлкните по клетке, чтобы исказить или восстановить бит"));
    dataEdit_->setLookProvider([](int, bool) { return walkstyle::dataLook(); });
    errorEdit_->setLookProvider([](int, bool bit) { return bit ? walkstyle::errorLook() : walkstyle::plainLook(); });
    channelErrorsButton_->setToolTip(QStringLiteral("Сгенерировать E по текущей модели канала из левой панели"));
    falseAcceptButton_->setToolTip(QStringLiteral("Взять в качестве E ненулевое кодовое слово — такую ошибку код не обнаруживает"));

    auto* top = new QHBoxLayout;
    top->addWidget(codeLabel_, 1);
    top->addWidget(textToggle_);

    auto* cells = new QGridLayout;
    cells->addWidget(new QLabel(QStringLiteral("Данные D:")), 0, 0);
    cells->addWidget(dataEdit_, 0, 1, Qt::AlignLeft);
    cells->addWidget(randomDataButton_, 0, 2, Qt::AlignLeft);
    cells->addWidget(errorCaption_, 1, 0);
    cells->addWidget(errorEdit_, 1, 1, 1, 2, Qt::AlignLeft);
    cells->setColumnStretch(3, 1);

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(noErrorsButton_);
    buttons->addWidget(channelErrorsButton_);
    buttons->addWidget(falseAcceptButton_);
    buttons->addSpacing(8);
    buttons->addWidget(noteLabel_, 1);

    auto* box = new QWidget;
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(top);
    layout->addLayout(cells);
    layout->addLayout(buttons);
    return box;
}

QWidget* WalkthroughPanel::createVisualPage()
{
    captionLabel_->setObjectName(QStringLiteral("walkthroughCaption"));
    captionLabel_->setWordWrap(true);
    captionLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    captionLabel_->setStyleSheet(QStringLiteral("QLabel { background: #f3f7fc; border-left: 4px solid #2a78d6; "
                                                "border-radius: 4px; padding: 8px 12px; color: #14213D; font-size: 14pt; }"));
    stepLabel_->setStyleSheet(QStringLiteral("color: #4A5568;"));

    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->addWidget(strip_);
    layout->addWidget(captionLabel_);
    layout->addWidget(createCanvasArea(), 1);
    layout->addWidget(createPlaybackControls());
    return page;
}

QScrollArea* WalkthroughPanel::createCanvasArea()
{
    scrollArea_ = new CanvasScrollArea([this] { fitCanvas(); });
    scrollArea_->setWidgetResizable(false);
    scrollArea_->setWidget(canvas_);
    scrollArea_->setFrameShape(QFrame::StyledPanel);
    scrollArea_->setStyleSheet(QStringLiteral("QScrollArea { border: 1px solid #e4e3df; border-radius: 6px; background: white; }"));
    scrollArea_->viewport()->setStyleSheet(QStringLiteral("background: white;"));
    scrollAnimation_.setTargetObject(scrollArea_->verticalScrollBar());
    return scrollArea_;
}

QWidget* WalkthroughPanel::createPlaybackControls()
{
    speedSlider_->setRange(1, maxSpeed);
    speedSlider_->setValue(defaultSpeed);
    speedSlider_->setFixedWidth(120);
    speedSlider_->setToolTip(QStringLiteral("Скорость анимации"));
    speedLabel_->setMinimumWidth(44);
    playButton_->setObjectName(QStringLiteral("walkthroughPlay"));

    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(restartButton_);
    layout->addWidget(backButton_);
    layout->addWidget(playButton_);
    layout->addWidget(pauseButton_);
    layout->addWidget(stepButton_);
    layout->addStretch();
    layout->addWidget(new QLabel(QStringLiteral("Скорость:")));
    layout->addWidget(speedSlider_);
    layout->addWidget(speedLabel_);
    layout->addSpacing(12);
    layout->addWidget(stepLabel_);
    return row;
}

void WalkthroughPanel::connectActions()
{
    connect(&animation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        progress_ = value.toDouble();
        canvas_->setPosition(current_, progress_);
    });
    connect(&animation_, &QAbstractAnimation::finished, this, [this] { finishCurrentStep(); });
    connect(&holdTimer_, &QTimer::timeout, this, [this] { startNextStep(); });
    connect(playButton_, &QPushButton::clicked, this, [this] { play(); });
    connect(pauseButton_, &QPushButton::clicked, this, [this] { pause(); });
    connect(stepButton_, &QPushButton::clicked, this, [this] { stepForward(); });
    connect(backButton_, &QPushButton::clicked, this, [this] { stepBack(); });
    connect(restartButton_, &QPushButton::clicked, this, [this] { restart(); });
    connect(speedSlider_, &QSlider::valueChanged, this, [this] { speedLabel_->setText(speedText(speedFactor())); });
    connect(textToggle_, &QCheckBox::toggled, this, [this] { updateModes(); });
    connect(randomDataButton_, &QPushButton::clicked, this, [this] { randomizeData(); });
    connect(noErrorsButton_, &QPushButton::clicked, this, [this] { clearErrors(); });
    connect(channelErrorsButton_, &QPushButton::clicked, this, [this] { errorsFromChannel(); });
    connect(falseAcceptButton_, &QPushButton::clicked, this, [this] { falseAcceptance(); });
    dataEdit_->setOnToggled([this](crc::Word bits) {
        channelNote_.clear();
        setBlock(bits, errors_);
    });
    errorEdit_->setOnToggled([this](crc::Word bits) {
        channelNote_.clear();
        setBlock(data_, bits);
    });
    speedLabel_->setText(speedText(speedFactor()));
}

void WalkthroughPanel::setCode(const std::optional<crc::CyclicCode>& code, const QString& problem)
{
    if (!code) {
        pause();
        code_.reset();
        messageLabel_->setText(QStringLiteral("Разбор блока недоступен: ") + problem);
        codeLabel_->setText(QStringLiteral("Код не задан"));
        updateModes();
        return;
    }
    const bool sameCode = code_ && code_->n == code->n && code_->generator == code->generator;
    if (sameCode)
        return;
    code_ = code;
    channelNote_.clear();
    data_ = random_.nextWord();
    errors_ = 0;
    rebuild();
}

void WalkthroughPanel::setChannel(const std::optional<crc::ChannelParams>& channel, const QString& problem)
{
    channel_ = channel;
    channelProblem_ = problem;
    updateButtons();
}

void WalkthroughPanel::setBlock(crc::Word data, crc::Word errors)
{
    data_ = data;
    errors_ = errors;
    if (code_)
        rebuild();
}

int WalkthroughPanel::stepCount() const
{
    return static_cast<int>(steps_.size());
}

void WalkthroughPanel::rebuild()
{
    pause();
    const crc::CyclicCode& code = *code_;
    trace_ = crc::walkthrough(code, data_, errors_);
    data_ = trace_.data;
    errors_ = trace_.errors;
    steps_ = buildScript(trace_);
    canvas_->setScript(trace_, steps_);
    textView_->setPlainText(QString::fromStdString(crc::walkthroughText(trace_)));
    const double pitch = code.n <= compactEditorLength ? 30.0 : 25.0;
    dataEdit_->setCellPitch(pitch);
    errorEdit_->setCellPitch(pitch);
    dataEdit_->setBits(data_, code.dataBitCount());
    errorEdit_->setBits(errors_, code.n);
    codeLabel_->setText(QStringLiteral("Код <b>(%1, %2)</b>: g(x) = %3 = %4")
                            .arg(code.n)
                            .arg(code.dataBitCount())
                            .arg(polynomialText(code.generator), bitText(code.generator, code.checkBitCount() + 1)));
    updateModes();
    updateNote();
    focusedStep_ = -1;
    showStep(stepCount() - 1, 1.0);
}

bool WalkthroughPanel::visualAllowed() const
{
    return code_ && code_->n <= maxVisualLength;
}

void WalkthroughPanel::updateModes()
{
    const bool valid = code_.has_value();
    const bool visual = visualAllowed();
    inputs_->setEnabled(valid);
    dataEdit_->setVisible(visual);
    errorEdit_->setVisible(visual);
    errorCaption_->setVisible(visual);
    limitLabel_->setVisible(valid && !visual);
    textToggle_->setEnabled(visual);
    if (!valid)
        pages_->setCurrentWidget(messageLabel_);
    else if (!visual || textToggle_->isChecked())
        pages_->setCurrentWidget(textView_);
    else
        pages_->setCurrentWidget(visualPage_);
    if (pages_->currentWidget() != visualPage_)
        pause();
    updateButtons();
}

void WalkthroughPanel::updateNote()
{
    const bool codewordError = errors_ != 0 && crc::syndrome(*code_, errors_) == 0;
    if (codewordError) {
        noteLabel_->setText(QStringLiteral("E — ненулевое кодовое слово: оно делится на g(x) без остатка, поэтому "
                                           "Y = F ⊕ E тоже кодовое слово, синдром S = 0 и искажение проходит незамеченным."));
    } else if (!channelNote_.isEmpty()) {
        noteLabel_->setText(channelNote_);
    } else if (visualAllowed()) {
        noteLabel_->setText(QStringLiteral("Щёлкайте по клеткам, чтобы менять биты данных и вектора ошибок."));
    } else {
        noteLabel_->clear();
    }
}

void WalkthroughPanel::updateButtons()
{
    const bool ready = code_ && !steps_.empty();
    channelErrorsButton_->setEnabled(ready && channel_.has_value());
    channelErrorsButton_->setToolTip(channel_ ? QStringLiteral("Сгенерировать E по текущей модели канала из левой панели")
                                              : QStringLiteral("Модель канала задана неверно: ") + channelProblem_);
    playButton_->setEnabled(ready && !playing_);
    pauseButton_->setEnabled(ready && playing_);
    stepButton_->setEnabled(ready && !atEnd());
    backButton_->setEnabled(ready && (current_ > 0 || progress_ > 0.0));
    restartButton_->setEnabled(ready);
}

void WalkthroughPanel::showStep(int step, double progress)
{
    if (steps_.empty())
        return;
    animation_.stop();
    current_ = std::clamp(step, 0, stepCount() - 1);
    progress_ = walkstyle::clamp01(progress);
    showPosition();
}

void WalkthroughPanel::showPosition()
{
    const WalkStep& step = steps_[static_cast<std::size_t>(current_)];
    canvas_->setPosition(current_, progress_);
    fitCanvas();
    captionLabel_->setText(step.caption);
    stepLabel_->setText(QStringLiteral("Шаг %1 из %2").arg(current_ + 1).arg(stepCount()));
    strip_->setStage(stageIndex(step.stage), isVisible());
    if (focusedStep_ != current_) {
        focusedStep_ = current_;
        scrollToFocus();
    }
    updateButtons();
}

void WalkthroughPanel::fitCanvas()
{
    const int viewportWidth = scrollArea_->viewport()->width();
    const int viewportHeight = scrollArea_->viewport()->height();
    const int width = std::max(viewportWidth, canvas_->minimumContentWidth());
    const int height = std::max(viewportHeight, canvas_->contentHeight(width));
    if (canvas_->size() != QSize(width, height))
        canvas_->resize(width, height);
}

void WalkthroughPanel::scrollToFocus()
{
    QScrollBar* bar = scrollArea_->verticalScrollBar();
    const QRectF focus = canvas_->focusArea();
    const int viewHeight = scrollArea_->viewport()->height();
    int target = bar->value();
    if (focus.height() >= viewHeight || focus.top() < target)
        target = static_cast<int>(focus.top());
    else if (focus.bottom() > target + viewHeight)
        target = static_cast<int>(focus.bottom()) - viewHeight;
    target = std::clamp(target, bar->minimum(), bar->maximum());
    scrollAnimation_.stop();
    if (!isVisible()) {
        bar->setValue(target);
        return;
    }
    scrollAnimation_.setStartValue(bar->value());
    scrollAnimation_.setEndValue(target);
    scrollAnimation_.start();
}

double WalkthroughPanel::speedFactor() const
{
    return speedSlider_->value() / speedUnit;
}

int WalkthroughPanel::scaledDuration(int milliseconds) const
{
    return std::max(1, static_cast<int>(milliseconds / speedFactor()));
}

bool WalkthroughPanel::atEnd() const
{
    return current_ + 1 >= stepCount() && progress_ >= 1.0;
}

void WalkthroughPanel::animateCurrentStep()
{
    const WalkStep& step = steps_[static_cast<std::size_t>(current_)];
    int duration = scaledDuration(step.durationMs);
    if (step.action == WalkAction::ApplyErrors)
        duration = std::max(duration, minFlashStepMs);
    animation_.stop();
    animation_.setStartValue(progress_);
    animation_.setDuration(std::max(1, static_cast<int>(duration * (1.0 - progress_))));
    showPosition();
    animation_.start();
}

void WalkthroughPanel::finishCurrentStep()
{
    progress_ = 1.0;
    canvas_->setPosition(current_, progress_);
    if (playing_ && current_ + 1 < stepCount()) {
        const bool sceneChanges = sceneOf(steps_[static_cast<std::size_t>(current_ + 1)].stage)
            != sceneOf(steps_[static_cast<std::size_t>(current_)].stage);
        holdTimer_.start(scaledDuration(sceneChanges ? sceneHoldMs : holdMs));
    } else {
        playing_ = false;
    }
    updateButtons();
}

void WalkthroughPanel::startNextStep()
{
    if (current_ + 1 >= stepCount())
        return;
    ++current_;
    progress_ = 0.0;
    animateCurrentStep();
}

void WalkthroughPanel::play()
{
    if (steps_.empty())
        return;
    if (atEnd()) {
        current_ = 0;
        progress_ = 0.0;
    }
    playing_ = true;
    if (progress_ < 1.0)
        animateCurrentStep();
    else
        startNextStep();
    updateButtons();
}

void WalkthroughPanel::pause()
{
    playing_ = false;
    holdTimer_.stop();
    animation_.stop();
    updateButtons();
}

void WalkthroughPanel::stepForward()
{
    pause();
    if (steps_.empty() || atEnd())
        return;
    if (progress_ < 1.0)
        animateCurrentStep();
    else
        startNextStep();
}

void WalkthroughPanel::stepBack()
{
    pause();
    if (steps_.empty())
        return;
    if (current_ > 0)
        showStep(current_ - 1, 1.0);
    else
        showStep(0, 0.0);
}

void WalkthroughPanel::restart()
{
    pause();
    showStep(0, 0.0);
}

void WalkthroughPanel::randomizeData()
{
    channelNote_.clear();
    setBlock(random_.nextWord(), errors_);
}

void WalkthroughPanel::clearErrors()
{
    channelNote_.clear();
    setBlock(data_, 0);
}

void WalkthroughPanel::errorsFromChannel()
{
    if (!code_ || !channel_)
        return;
    crc::Random random(crc::makeRandomSeed());
    const std::unique_ptr<crc::Channel> channel = crc::makeChannel(*channel_);
    channel->start(random);
    const crc::Word errors = channel->errorVector(random, code_->n);
    const QString name = QString::fromStdString(channel->displayName());
    channelNote_ = errors == 0
        ? QStringLiteral("Канал %1 не исказил ни одного бита этого блока — нажмите ещё раз, чтобы сгенерировать новый вектор.").arg(name)
        : QStringLiteral("Канал %1 исказил %2 этого блока.").arg(name, bitCountText(countOnes(errors)));
    setBlock(data_, errors);
}

void WalkthroughPanel::falseAcceptance()
{
    if (!code_)
        return;
    const crc::Word dataMask = crc::lowBitsMask(code_->dataBitCount());
    crc::Word seed = 0;
    while (seed == 0)
        seed = random_.nextWord() & dataMask;
    channelNote_.clear();
    setBlock(data_, crc::encode(*code_, seed));
}
