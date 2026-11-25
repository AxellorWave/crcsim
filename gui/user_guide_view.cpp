#include "user_guide_view.h"

#include "user_guide.h"

#include <QTextBrowser>

namespace {

const int documentMargin = 28;

}

QWidget* createUserGuideView()
{
    auto* browser = new QTextBrowser;
    browser->setObjectName(QStringLiteral("userGuide"));
    browser->setOpenExternalLinks(false);
    browser->setStyleSheet(QStringLiteral("QTextBrowser#userGuide { background: #ffffff; border: none; }"));
    browser->document()->setDocumentMargin(documentMargin);
    browser->document()->setDefaultStyleSheet(userGuideStyleSheet());
    browser->setHtml(userGuideHtml());
    return browser;
}
