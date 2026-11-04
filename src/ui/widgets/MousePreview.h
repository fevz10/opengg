#pragma once

#include <QPainterPath>
#include <QWidget>

/**
 * Stylised top view of the mouse with numbered call-outs for its six
 * remappable buttons (1 left, 2 right, 3 wheel, 4 back, 5 forward, 6 CPI).
 */
class MousePreview : public QWidget
{
    Q_OBJECT
public:
    explicit MousePreview(QWidget *parent = nullptr);

    void setShowCallouts(bool show);
    void setHighlightedButton(int index); // 0..5 or -1
    int highlightedButton() const { return m_highlight; }
    void setBatteryLevel(int level) { m_battery = level; update(); }

    QSize sizeHint() const override;
    int heightForWidth(int w) const override;
    bool hasHeightForWidth() const override { return true; }

signals:
    void buttonClicked(int index);
    void buttonHovered(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QTransform designTransform() const;
    QPainterPath bodyPath() const;
    QPainterPath buttonRegion(int index) const;
    int buttonAt(const QPointF &widgetPos) const;

    bool m_callouts = true;
    int m_highlight = -1;
    int m_hover = -1;
    int m_battery = -1;
};
