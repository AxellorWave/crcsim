#pragma once

#include "walkthrough_script.h"

#include "params.h"
#include "random.h"
#include "walkthrough.h"

#include <QPropertyAnimation>
#include <QString>
#include <QTimer>
#include <QVariantAnimation>
#include <QWidget>

#include <optional>
#include <vector>

class BitCellsEdit;
class QCheckBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QScrollArea;
class QSlider;
class QStackedWidget;
class StageStrip;
class WalkthroughCanvas;

class WalkthroughPanel : public QWidget {
public:
    explicit WalkthroughPanel(QWidget* parent = nullptr);

    void setCode(const std::optional<crc::CyclicCode>& code, const QString& problem);
    void setChannel(const std::optional<crc::ChannelParams>& channel, const QString& problem);
    void setBlock(crc::Word data, crc::Word errors);
    void showStep(int step, double progress);
    int stepCount() const;

    void play();
    void pause();
    void stepForward();
    void stepBack();
    void restart();

private:
    QWidget* createInputs();
    QWidget* createVisualPage();
    QWidget* createPlaybackControls();
    QScrollArea* createCanvasArea();

    void connectActions();
    void applyThemeStyles();
    void rebuild();
    void showPosition();
    void fitCanvas();
    void scrollToFocus();
    void updateModes();
    void updateNote();
    void updateButtons();
    void animateCurrentStep();
    void finishCurrentStep();
    void startNextStep();
    int scaledDuration(int milliseconds) const;
    double speedFactor() const;
    bool atEnd() const;
    bool visualAllowed() const;

    void randomizeData();
    void clearErrors();
    void errorsFromChannel();
    void falseAcceptance();

    QLabel* codeLabel_;
    QCheckBox* textToggle_;
    BitCellsEdit* dataEdit_;
    BitCellsEdit* errorEdit_;
    QLabel* errorCaption_;
    QWidget* inputs_;
    QPushButton* randomDataButton_;
    QPushButton* noErrorsButton_;
    QPushButton* channelErrorsButton_;
    QPushButton* falseAcceptButton_;
    QLabel* noteLabel_;
    QLabel* limitLabel_;
    QStackedWidget* pages_;
    QWidget* visualPage_;
    QPlainTextEdit* textView_;
    QLabel* messageLabel_;
    StageStrip* strip_;
    QLabel* captionLabel_;
    QLabel* stepLabel_;
    QScrollArea* scrollArea_;
    WalkthroughCanvas* canvas_;
    QPushButton* restartButton_;
    QPushButton* backButton_;
    QPushButton* playButton_;
    QPushButton* pauseButton_;
    QPushButton* stepButton_;
    QSlider* speedSlider_;
    QLabel* speedLabel_;

    QVariantAnimation animation_;
    QTimer holdTimer_;
    QPropertyAnimation scrollAnimation_;
    crc::Random random_;

    std::optional<crc::CyclicCode> code_;
    std::optional<crc::ChannelParams> channel_;
    QString channelProblem_;
    QString channelNote_;
    crc::Word data_ = 0;
    crc::Word errors_ = 0;
    crc::Walkthrough trace_;
    std::vector<WalkStep> steps_;
    int current_ = 0;
    double progress_ = 0.0;
    int focusedStep_ = -1;
    bool playing_ = false;
};
