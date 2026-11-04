#include "ui/widgets/EmptyState.h"

#include "core/Icons.h"
#include "core/Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

EmptyState::EmptyState(const QString &iconName, const QString &title, const QString &description, QWidget *parent)
    : QWidget(parent)
{
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(24, 24, 24, 24);
    v->setSpacing(10);
    v->addStretch(1);

    auto *iconBox = new QLabel(this);
    iconBox->setFixedSize(72, 72);
    iconBox->setAlignment(Qt::AlignCenter);
    iconBox->setPixmap(Icons::pixmap(iconName, Theme::Accent, 34));
    iconBox->setStyleSheet(QStringLiteral("background:%1; border-radius:36px;").arg(Theme::hex(Theme::Bg3)));
    v->addWidget(iconBox, 0, Qt::AlignHCenter);
    v->addSpacing(6);

    auto *t = new QLabel(title, this);
    t->setFont(Theme::font(20, QFont::DemiBold));
    t->setAlignment(Qt::AlignCenter);
    v->addWidget(t);

    m_description = new QLabel(description, this);
    m_description->setProperty("role", QStringLiteral("secondary"));
    m_description->setAlignment(Qt::AlignCenter);
    m_description->setWordWrap(true);
    m_description->setMaximumWidth(520);
    m_description->setOpenExternalLinks(true);
    v->addWidget(m_description, 0, Qt::AlignHCenter);

    m_actions = new QHBoxLayout;
    m_actions->setSpacing(10);
    m_actions->addStretch(1);
    m_actions->addStretch(1);
    v->addSpacing(8);
    v->addLayout(m_actions);
    v->addStretch(1);
}

QPushButton *EmptyState::addAction(const QString &text, bool primary)
{
    auto *b = new QPushButton(text, this);
    if (primary)
        b->setProperty("variant", QStringLiteral("primary"));
    b->setCursor(Qt::PointingHandCursor);
    m_actions->insertWidget(m_actions->count() - 1, b);
    return b;
}

void EmptyState::setDescription(const QString &text)
{
    m_description->setText(text);
}
