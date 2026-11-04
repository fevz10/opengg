#pragma once

#include <QLabel>

/// Small uppercase, letter spaced section heading ("GEAR", "APPS", ...).
class SectionLabel : public QLabel
{
    Q_OBJECT
public:
    explicit SectionLabel(const QString &text, QWidget *parent = nullptr);
};
