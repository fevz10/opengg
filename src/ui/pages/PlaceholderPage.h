#pragma once

#include <QWidget>

/// Page for GG features that do not exist on Linux (Sonar, Moments).
class PlaceholderPage : public QWidget
{
    Q_OBJECT
public:
    struct Alternative {
        QString name;
        QString url;
    };

    PlaceholderPage(const QString &iconName, const QString &title, const QString &description,
                    const QList<Alternative> &alternatives, QWidget *parent = nullptr);
};
