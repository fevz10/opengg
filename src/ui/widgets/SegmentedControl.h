#pragma once

#include <QStringList>
#include <QWidget>

/// Pill shaped control with mutually exclusive segments (e.g. polling rates).
class SegmentedControl : public QWidget
{
    Q_OBJECT
public:
    explicit SegmentedControl(QWidget *parent = nullptr);

    void setItems(const QStringList &items);
    QStringList items() const { return m_items; }
    int currentIndex() const { return m_current; }
    void setCurrentIndex(int index);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

signals:
    void currentIndexChanged(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    int indexAt(const QPoint &pos) const;
    qreal segmentWidth() const;

    QStringList m_items;
    int m_current = 0;
    int m_hover = -1;
};
