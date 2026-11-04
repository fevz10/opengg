#include "ui/pages/PlaceholderPage.h"

#include "core/Icons.h"
#include "core/Theme.h"
#include "ui/widgets/EmptyState.h"

#include <QDesktopServices>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

PlaceholderPage::PlaceholderPage(const QString &iconName, const QString &title, const QString &description,
                                 const QList<Alternative> &alternatives, QWidget *parent)
    : QWidget(parent)
{
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    auto *empty = new EmptyState(iconName, title, description, this);
    for (const Alternative &alt : alternatives) {
        QPushButton *b = empty->addAction(alt.name);
        b->setIcon(Icons::icon(QStringLiteral("external-link"), Theme::TextSecondary, 14));
        const QString url = alt.url;
        connect(b, &QPushButton::clicked, this, [url]() { QDesktopServices::openUrl(QUrl(url)); });
    }
    v->addWidget(empty);
}
