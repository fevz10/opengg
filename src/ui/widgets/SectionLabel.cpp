#include "ui/widgets/SectionLabel.h"

#include "core/Theme.h"

SectionLabel::SectionLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
    setFont(Theme::sectionFont());
    setProperty("role", QStringLiteral("muted"));
}
