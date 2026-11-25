#include "help_button.h"

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

const char* const buttonStyle =
    "QToolButton#helpButton { border: 1px solid #b8c4d6; border-radius: 10px; background: #f4f7fb;"
    " color: #4A5568; font-weight: bold; padding: 0; }"
    "QToolButton#helpButton:hover { border-color: #2a78d6; background: #2a78d6; color: white; }"
    "QToolButton#helpButton:pressed { border-color: #eb6834; background: #eb6834; color: white; }"
    "QToolButton#helpButton:disabled { border-color: #d6dbe3; color: #a9b1bd; background: #f7f8fa; }";

const char* const popupStyle =
    "QFrame#helpPopup { background: #ffffff; border: 1px solid #2a78d6; border-radius: 6px; }"
    "QLabel#helpPopupBody { color: #14213D; }";

QString popupHtml(HelpTopic topic)
{
    return QStringLiteral("<p style='color:#2a78d6; font-size:11pt; font-weight:600; margin:0 0 6px 0'>%1</p>%2")
        .arg(parameterHelp(topic).title, parameterHelpHtml(topic));
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
    popup->setStyleSheet(QString::fromLatin1(popupStyle));
    popup->setFixedWidth(popupWidth);
    auto* layout = new QVBoxLayout(popup);
    layout->setContentsMargins(14, 10, 14, 12);
    layout->addWidget(createPopupLabel(popupHtml(topic)));
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
    button->setStyleSheet(QString::fromLatin1(buttonStyle));
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
