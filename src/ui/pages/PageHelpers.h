#pragma once

#include "core/Theme.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

namespace PageHelpers {

/// Wraps `content` in a vertical scroll area with page margins.
inline QScrollArea *makeScrollArea(QWidget *content, QWidget *parent = nullptr)
{
    auto *scroll = new QScrollArea(parent);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidget(content);
    return scroll;
}

/// Content widget with standard page margins and a vertical layout.
inline QWidget *makeContent(QVBoxLayout **layoutOut, int maxWidth = 1240)
{
    auto *content = new QWidget;
    auto *outer = new QHBoxLayout(content);
    outer->setContentsMargins(36, 28, 36, 36);
    auto *inner = new QWidget(content);
    inner->setMaximumWidth(maxWidth);
    inner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *v = new QVBoxLayout(inner);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(18);
    outer->addWidget(inner, 1);
    outer->addStretch(0);
    if (layoutOut)
        *layoutOut = v;
    return content;
}

inline QLabel *title(const QString &text, QWidget *parent = nullptr)
{
    auto *l = new QLabel(text, parent);
    l->setFont(Theme::titleFont(24));
    return l;
}

inline QLabel *secondary(const QString &text, QWidget *parent = nullptr)
{
    auto *l = new QLabel(text, parent);
    l->setProperty("role", QStringLiteral("secondary"));
    l->setWordWrap(true);
    return l;
}

inline QFrame *divider(QWidget *parent = nullptr)
{
    auto *f = new QFrame(parent);
    f->setObjectName(QStringLiteral("Divider"));
    f->setFixedHeight(1);
    return f;
}

} // namespace PageHelpers
