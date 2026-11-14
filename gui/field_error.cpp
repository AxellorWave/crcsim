#include "field_error.h"

#include <QStringList>

#include <algorithm>

namespace {

const char* const invalidFieldStyle = "border: 2px solid #d02020; border-radius: 3px; background: #fff0f0;";

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
        field->setStyleSheet(hasError(field, errors) ? QString::fromLatin1(invalidFieldStyle) : QString());
}

QString joinFieldErrors(const std::vector<FieldError>& errors)
{
    QStringList lines;
    for (const FieldError& error : errors)
        lines.append(QStringLiteral("• ") + error.message);
    return lines.join(QLatin1Char('\n'));
}
