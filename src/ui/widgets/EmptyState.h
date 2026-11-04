#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

/// Centered icon + title + description (+ optional action), used when a page
/// has nothing to show (no devices, feature unavailable, ...).
class EmptyState : public QWidget
{
    Q_OBJECT
public:
    EmptyState(const QString &iconName, const QString &title, const QString &description, QWidget *parent = nullptr);

    QPushButton *addAction(const QString &text, bool primary = false);
    void setDescription(const QString &text);

private:
    QLabel *m_description;
    class QHBoxLayout *m_actions;
};
