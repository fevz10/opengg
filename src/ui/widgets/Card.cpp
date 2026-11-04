#include "ui/widgets/Card.h"

#include "core/Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

Card::Card(QWidget *parent)
    : QFrame(parent)
{
    build();
}

Card::Card(const QString &title, const QString &subtitle, QWidget *parent)
    : QFrame(parent)
{
    build();
    setTitle(title);
    setSubtitle(subtitle);
}

void Card::build()
{
    setObjectName(QStringLiteral("Card"));
    setAttribute(Qt::WA_StyledBackground, true);

    m_outer = new QVBoxLayout(this);
    m_outer->setContentsMargins(20, 18, 20, 20);
    m_outer->setSpacing(14);

    auto *headerBox = new QVBoxLayout;
    headerBox->setSpacing(3);
    m_title = new QLabel(this);
    m_title->setFont(Theme::font(15, QFont::DemiBold));
    m_title->setVisible(false);
    m_subtitle = new QLabel(this);
    m_subtitle->setProperty("role", QStringLiteral("secondary"));
    m_subtitle->setWordWrap(true);
    m_subtitle->setVisible(false);
    headerBox->addWidget(m_title);
    headerBox->addWidget(m_subtitle);

    m_header = new QHBoxLayout;
    m_header->setSpacing(12);
    m_header->addLayout(headerBox, 1);
    m_outer->addLayout(m_header);

    m_body = new QVBoxLayout;
    m_body->setSpacing(12);
    m_outer->addLayout(m_body);
}

void Card::setTitle(const QString &title)
{
    m_title->setText(title);
    m_title->setVisible(!title.isEmpty());
}

void Card::setSubtitle(const QString &subtitle)
{
    m_subtitle->setText(subtitle);
    m_subtitle->setVisible(!subtitle.isEmpty());
}

void Card::setHeaderWidget(QWidget *w)
{
    if (m_headerWidget) {
        m_header->removeWidget(m_headerWidget);
        m_headerWidget->deleteLater();
    }
    m_headerWidget = w;
    if (w)
        m_header->addWidget(w, 0, Qt::AlignTop);
}

void Card::addWidget(QWidget *w, int stretch)
{
    m_body->addWidget(w, stretch);
}

void Card::addLayout(QLayout *l, int stretch)
{
    m_body->addLayout(l, stretch);
}

void Card::addStretch(int stretch)
{
    m_body->addStretch(stretch);
}

void Card::addSpacing(int px)
{
    m_body->addSpacing(px);
}

QWidget *Card::addRow(const QString &label, QWidget *control, const QString &description)
{
    auto *row = new QWidget(this);
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(16);
    auto *textBox = new QVBoxLayout;
    textBox->setSpacing(2);
    auto *l = new QLabel(label, row);
    l->setFont(Theme::font(13, QFont::DemiBold));
    textBox->addWidget(l);
    if (!description.isEmpty()) {
        auto *d = new QLabel(description, row);
        d->setProperty("role", QStringLiteral("secondary"));
        d->setWordWrap(true);
        d->setFont(Theme::font(12));
        textBox->addWidget(d);
    }
    h->addLayout(textBox, 1);
    if (control)
        h->addWidget(control, 0, Qt::AlignVCenter);
    m_body->addWidget(row);
    return row;
}
