#include "user_guide_view.h"

#include "user_guide.h"

#include <QEvent>
#include <QFont>
#include <QPalette>
#include <QScrollBar>
#include <QTextBrowser>

namespace {

const int documentMargin = 40;
const qreal bodyPointSize = 13.0;

class UserGuideBrowser : public QTextBrowser {
public:
    UserGuideBrowser()
    {
        setObjectName(QStringLiteral("userGuide"));
        setOpenExternalLinks(false);
        setFrameShape(QFrame::NoFrame);
        QFont serif;
        serif.setFamilies({QStringLiteral("Times New Roman"), QStringLiteral("Liberation Serif"),
            QStringLiteral("DejaVu Serif")});
        serif.setStyleHint(QFont::Serif);
        serif.setPointSizeF(bodyPointSize);
        setFont(serif);
        document()->setDefaultFont(serif);
        document()->setDocumentMargin(documentMargin);
        render();
    }

protected:
    void changeEvent(QEvent* event) override
    {
        QTextBrowser::changeEvent(event);
        const QEvent::Type type = event->type();
        if (type == QEvent::PaletteChange || type == QEvent::ApplicationPaletteChange || type == QEvent::StyleChange)
            render();
    }

private:
    void render()
    {
        const QPalette& colors = palette();
        const UserGuideColors guide{colors.color(QPalette::Text).name(), colors.color(QPalette::Base).name(),
            colors.color(QPalette::Mid).name()};
        const QString styleSheet = userGuideStyleSheet(guide);
        if (styleSheet == renderedStyleSheet_)
            return;
        renderedStyleSheet_ = styleSheet;
        const int position = verticalScrollBar()->value();
        document()->setDefaultStyleSheet(styleSheet);
        setHtml(userGuideHtml());
        verticalScrollBar()->setValue(position);
    }

    QString renderedStyleSheet_;
};

}

QWidget* createUserGuideView()
{
    return new UserGuideBrowser;
}
