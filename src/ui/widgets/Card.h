#pragma once

#include <QFrame>

class QLabel;
class QVBoxLayout;
class QHBoxLayout;

/// Rounded dark panel; optional title/subtitle header and trailing header widget.
class Card : public QFrame
{
    Q_OBJECT
public:
    explicit Card(QWidget *parent = nullptr);
    Card(const QString &title, const QString &subtitle, QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setSubtitle(const QString &subtitle);
    /// Widget shown at the right of the title row (e.g. a toggle).
    void setHeaderWidget(QWidget *w);
    /// Layout for the card body (below the header).
    QVBoxLayout *bodyLayout() const { return m_body; }
    void addWidget(QWidget *w, int stretch = 0);
    void addLayout(QLayout *l, int stretch = 0);
    void addStretch(int stretch = 1);
    void addSpacing(int px);

    /// Adds a labelled row: text on the left, widget on the right.
    QWidget *addRow(const QString &label, QWidget *control, const QString &description = QString());

private:
    void build();

    QVBoxLayout *m_outer = nullptr;
    QHBoxLayout *m_header = nullptr;
    QVBoxLayout *m_body = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_subtitle = nullptr;
    QWidget *m_headerWidget = nullptr;
};
