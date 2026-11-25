#include "theme.h"

#include <QCoreApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QPalette>
#include <QPointer>
#include <QWidget>

#include <algorithm>
#include <utility>
#include <vector>

namespace {

const int darkLightnessLimit = 128;

QColor hex(const char* name)
{
    return QColor(QString::fromLatin1(name));
}

CellColors cell(const char* fill, const char* border, const char* text)
{
    return {hex(fill), hex(border), hex(text)};
}

void fillLightTexts(Theme& theme)
{
    theme.surface = hex("#ffffff");
    theme.surfaceAlt = hex("#faf9f7");
    theme.border = hex("#e4e3df");
    theme.grid = hex("#e4e3df");
    theme.text = hex("#14213d");
    theme.secondaryText = hex("#4a5568");
    theme.mutedText = hex("#6e7787");
}

void fillLightAccents(Theme& theme)
{
    theme.accent = hex("#2a78d6");
    theme.accentText = hex("#ffffff");
    theme.accentSoft = hex("#e8f0fa");
    theme.accentSoftBorder = hex("#9cbfea");
    theme.accentSoftText = hex("#e3eefb");
    theme.accentStrong = hex("#1a5fb4");
    theme.orange = hex("#eb6834");
    theme.orangeText = hex("#9a3f12");
    theme.success = hex("#2e9e5b");
    theme.successText = hex("#1e7a32");
    theme.danger = hex("#d0342c");
    theme.dangerText = hex("#b01010");
    theme.dangerSoft = hex("#fff0f0");
    theme.warningText = hex("#9a4a06");
}

void fillLightControls(Theme& theme)
{
    theme.highlightRow = hex("#e4eefb");
    theme.highlightRowText = hex("#14213d");
    theme.badgeText = hex("#1a4f9c");
    theme.caption = hex("#f3f7fc");
    theme.buttonFace = hex("#f4f7fb");
    theme.buttonBorder = hex("#b8c4d6");
    theme.buttonText = hex("#4a5568");
    theme.disabledFace = hex("#f7f8fa");
    theme.disabledBorder = hex("#d6dbe3");
    theme.disabledText = hex("#a9b1bd");
}

void fillLightCells(Theme& theme)
{
    theme.dataCell = cell("#e6f0fb", "#2a78d6", "#14213d");
    theme.checkCell = cell("#fdebe2", "#eb6834", "#14213d");
    theme.placeholderCell = cell("#ffffff", "#f4b89c", "#4a5568");
    theme.plainCell = cell("#faf9f7", "#cfcdc7", "#14213d");
    theme.divisorCell = cell("#eceff5", "#7d889c", "#4a5568");
    theme.dimCell = cell("#ffffff", "#eeedea", "#c4c7cd");
    theme.errorCell = cell("#d0342c", "#a8241d", "#ffffff");
    theme.syndromeCell = cell("#14213d", "#14213d", "#ffffff");
}

void fillDarkTexts(Theme& theme)
{
    theme.surface = hex("#1f2023");
    theme.surfaceAlt = hex("#2b2d31");
    theme.border = hex("#3c4047");
    theme.grid = hex("#3a3d43");
    theme.text = hex("#e8eaed");
    theme.secondaryText = hex("#b8bfcb");
    theme.mutedText = hex("#a6adbb");
}

void fillDarkAccents(Theme& theme)
{
    theme.accent = hex("#5aa2f0");
    theme.accentText = hex("#0f1b2d");
    theme.accentSoft = hex("#22344d");
    theme.accentSoftBorder = hex("#3d5f8c");
    theme.accentSoftText = hex("#1d3557");
    theme.accentStrong = hex("#7ab4f5");
    theme.orange = hex("#f08a4b");
    theme.orangeText = hex("#f5a372");
    theme.success = hex("#3fb27f");
    theme.successText = hex("#5cc98f");
    theme.danger = hex("#ef6b5b");
    theme.dangerText = hex("#ff8a7a");
    theme.dangerSoft = hex("#3a2224");
    theme.warningText = hex("#f0b04b");
}

void fillDarkControls(Theme& theme)
{
    theme.highlightRow = hex("#27415f");
    theme.highlightRowText = hex("#ffffff");
    theme.badgeText = hex("#9cc8ff");
    theme.caption = hex("#232a35");
    theme.buttonFace = hex("#2b2d31");
    theme.buttonBorder = hex("#565b66");
    theme.buttonText = hex("#c9ced8");
    theme.disabledFace = hex("#24262a");
    theme.disabledBorder = hex("#3a3d43");
    theme.disabledText = hex("#6f7580");
}

void fillDarkCells(Theme& theme)
{
    theme.dataCell = cell("#1e3550", "#5aa2f0", "#e8eaed");
    theme.checkCell = cell("#45291c", "#f08a4b", "#e8eaed");
    theme.placeholderCell = cell("#1f2023", "#8a5235", "#b8bfcb");
    theme.plainCell = cell("#2a2c30", "#4a4e56", "#e8eaed");
    theme.divisorCell = cell("#30343c", "#7d889c", "#c6ccd6");
    theme.dimCell = cell("#1f2023", "#34373d", "#6b717c");
    theme.errorCell = cell("#c8443a", "#ef6b5b", "#ffffff");
    theme.syndromeCell = cell("#9fb0c9", "#c3cee0", "#111418");
}

Theme makeLightTheme()
{
    Theme theme;
    fillLightTexts(theme);
    fillLightAccents(theme);
    fillLightControls(theme);
    fillLightCells(theme);
    return theme;
}

Theme makeDarkTheme()
{
    Theme theme;
    theme.dark = true;
    fillDarkTexts(theme);
    fillDarkAccents(theme);
    fillDarkControls(theme);
    fillDarkCells(theme);
    return theme;
}

const char* const dispatcherName = "themeDispatcher";

class ThemeDispatcher : public QObject {
public:
    explicit ThemeDispatcher(QObject* application)
        : QObject(application)
    {
        setObjectName(QString::fromLatin1(dispatcherName));
        application->installEventFilter(this);
    }

    void add(QWidget* widget, std::function<void()> apply)
    {
        entries_.push_back({widget, std::move(apply)});
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == parent() && isThemeChange(event->type()))
            applyAll();
        return QObject::eventFilter(watched, event);
    }

private:
    struct Entry {
        QPointer<QWidget> widget;
        std::function<void()> apply;
    };

    void applyAll()
    {
        entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [](const Entry& entry) { return !entry.widget; }),
            entries_.end());
        const std::vector<Entry> entries = entries_;
        for (const Entry& entry : entries)
            if (entry.widget)
                entry.apply();
    }

    std::vector<Entry> entries_;
};

ThemeDispatcher* dispatcher()
{
    QObject* application = QCoreApplication::instance();
    auto* existing = application->findChild<QObject*>(QString::fromLatin1(dispatcherName), Qt::FindDirectChildrenOnly);
    if (existing)
        return static_cast<ThemeDispatcher*>(existing);
    return new ThemeDispatcher(application);
}

}

const Theme& lightTheme()
{
    static const Theme theme = makeLightTheme();
    return theme;
}

const Theme& darkTheme()
{
    static const Theme theme = makeDarkTheme();
    return theme;
}

bool paletteIsDark()
{
    return QGuiApplication::palette().color(QPalette::Window).lightness() < darkLightnessLimit;
}

const Theme& currentTheme()
{
    return paletteIsDark() ? darkTheme() : lightTheme();
}

QString cssColor(const QColor& color)
{
    return color.name(QColor::HexRgb);
}

QString textColorStyle(const QColor& color)
{
    return QStringLiteral("color: %1;").arg(cssColor(color));
}

bool isThemeChange(int eventType)
{
    return eventType == QEvent::ApplicationPaletteChange || eventType == QEvent::ThemeChange;
}

void keepThemed(QWidget* widget, std::function<void()> apply)
{
    apply();
    dispatcher()->add(widget, std::move(apply));
}

void keepTextColor(QWidget* widget, QColor Theme::*role)
{
    keepThemed(widget, [widget, role] { widget->setStyleSheet(textColorStyle(currentTheme().*role)); });
}
