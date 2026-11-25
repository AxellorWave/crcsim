#include "help_button.h"

#include "theme.h"

#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QScreen>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

const int buttonSize = 20;
const int popupWidth = 380;
const int popupGap = 4;

QString buttonStyle(const Theme& theme)
{
    return QStringLiteral(
        "QToolButton#helpButton { border: 1px solid %1; border-radius: 10px; background: %2; color: %3;"
        " font-weight: bold; padding: 0; }"
        "QToolButton#helpButton:hover { border-color: %4; background: %4; color: %5; }"
        "QToolButton#helpButton:pressed { border-color: %6; background: %6; color: %5; }"
        "QToolButton#helpButton:disabled { border-color: %7; color: %8; background: %9; }")
        .arg(cssColor(theme.buttonBorder), cssColor(theme.buttonFace), cssColor(theme.buttonText), cssColor(theme.accent),
            cssColor(theme.accentText), cssColor(theme.orange), cssColor(theme.disabledBorder),
            cssColor(theme.disabledText), cssColor(theme.disabledFace));
}

QString popupStyle(const Theme& theme)
{
    return QStringLiteral("QFrame#helpPopup { background: %1; border: 1px solid %2; border-radius: 6px; }"
                          "QLabel#helpPopupBody { color: %3; }")
        .arg(cssColor(theme.surfaceAlt), cssColor(theme.accent), cssColor(theme.text));
}

QString popupHtml(HelpTopic topic)
{
    return QStringLiteral("<p style='color:%1; font-size:11pt; font-weight:600; margin:0 0 6px 0'>%2</p>%3")
        .arg(cssColor(currentTheme().accent), parameterHelp(topic).title, parameterHelpHtml(topic));
}

QLabel* createPopupLabel(const QString& text)
{
    auto* label = new QLabel(text);
    label->setObjectName(QStringLiteral("helpPopupBody"));
    label->setTextFormat(Qt::RichText);
    label->setWordWrap(true);
    return label;
}

QFrame* createPopup(HelpTopic topic, QWidget* anchor)
{
    auto* popup = new QFrame(anchor, Qt::Popup);
    popup->setObjectName(QStringLiteral("helpPopup"));
    popup->setAttribute(Qt::WA_DeleteOnClose);
    popup->setFixedWidth(popupWidth);
    auto* layout = new QVBoxLayout(popup);
    layout->setContentsMargins(14, 10, 14, 12);
    QLabel* label = createPopupLabel(popupHtml(topic));
    layout->addWidget(label);
    keepThemed(popup, [popup, label, topic] {
        popup->setStyleSheet(popupStyle(currentTheme()));
        label->setText(popupHtml(topic));
    });
    popup->setFixedHeight(popup->heightForWidth(popupWidth));
    return popup;
}

QPoint popupPosition(const QWidget* anchor, const QSize& size)
{
    const QPoint below = anchor->mapToGlobal(QPoint(0, anchor->height() + popupGap));
    QPoint position(below.x() - size.width() + anchor->width(), below.y());
    const QScreen* screen = anchor->screen() ? anchor->screen() : QGuiApplication::primaryScreen();
    if (!screen)
        return position;
    const QRect area = screen->availableGeometry();
    position.setX(qBound(area.left(), position.x(), area.right() - size.width()));
    if (position.y() + size.height() > area.bottom())
        position.setY(anchor->mapToGlobal(QPoint(0, 0)).y() - size.height() - popupGap);
    return position;
}

}

void showHelpPopup(HelpTopic topic, QWidget* anchor)
{
    QFrame* popup = createPopup(topic, anchor);
    popup->move(popupPosition(anchor, popup->size()));
    popup->show();
}

QToolButton* createHelpButton(HelpTopic topic, QWidget* parent)
{
    auto* button = new QToolButton(parent);
    button->setObjectName(QStringLiteral("helpButton"));
    button->setText(QStringLiteral("?"));
    button->setFixedSize(buttonSize, buttonSize);
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::TabFocus);
    keepThemed(button, [button] { button->setStyleSheet(buttonStyle(currentTheme())); });
    button->setToolTip(QStringLiteral("Справка: ") + parameterHelp(topic).title);
    QObject::connect(button, &QToolButton::clicked, button, [topic, button] { showHelpPopup(topic, button); });
    return button;
}

QWidget* withHelpButton(QWidget* field, HelpTopic topic)
{
    field->setToolTip(parameterHelpToolTip(topic));
    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(field, 1);
    layout->addWidget(createHelpButton(topic, row));
    return row;
}
