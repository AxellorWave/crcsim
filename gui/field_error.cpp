#include "field_error.h"

#include "theme.h"

#include <QStringList>

#include <algorithm>

namespace {

const char* const invalidProperty = "invalidField";
const char* const themedProperty = "invalidFieldThemed";

QString invalidFieldStyle(const Theme& theme)
{
    return QStringLiteral("border: 2px solid %1; border-radius: 3px; background: %2; color: %3;")
        .arg(cssColor(theme.danger), cssColor(theme.dangerSoft), cssColor(theme.text));
}

void applyFieldStyle(QWidget* field)
{
    const bool invalid = field->property(invalidProperty).toBool();
    field->setStyleSheet(invalid ? invalidFieldStyle(currentTheme()) : QString());
}

void markField(QWidget* field, bool invalid)
{
    field->setProperty(invalidProperty, invalid);
    if (field->property(themedProperty).toBool()) {
        applyFieldStyle(field);
        return;
    }
    field->setProperty(themedProperty, true);
    keepThemed(field, [field] { applyFieldStyle(field); });
}

bool hasError(const QWidget* field, const std::vector<FieldError>& errors)
{
    return std::any_of(errors.begin(), errors.end(), [field](const FieldError& error) { return error.widget == field; });
}

}

void addUnlessReported(std::vector<FieldError>& errors, const FieldError& error)
{
    if (!hasError(error.widget, errors))
        errors.push_back(error);
}

void highlightFields(const std::vector<QWidget*>& fields, const std::vector<FieldError>& errors)
{
    for (QWidget* field : fields)
        markField(field, hasError(field, errors));
}

QString joinFieldErrors(const std::vector<FieldError>& errors)
{
    QStringList lines;
    for (const FieldError& error : errors)
        lines.append(QStringLiteral("• ") + error.message);
    return lines.join(QLatin1Char('\n'));
}
