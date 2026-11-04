#pragma once

#include "devices/Protocol.h"

#include <QSet>
#include <QWidget>

/**
 * Stylised top view of a TKL keyboard whose keys glow in the colors of the 8
 * illumination zones. Zones can be selected by clicking on them.
 */
class KeyboardPreview : public QWidget
{
    Q_OBJECT
public:
    using ZoneColors = Protocol::Apex3Tkl::ZoneColors;

    explicit KeyboardPreview(QWidget *parent = nullptr);

    void setFrame(const ZoneColors &colors);
    const ZoneColors &frame() const { return m_colors; }

    void setInteractive(bool interactive);
    bool isInteractive() const { return m_interactive; }
    QSet<int> selectedZones() const { return m_selected; }
    void setSelectedZones(const QSet<int> &zones);
    void selectAll();
    void setDimmed(bool dimmed) { m_dimmed = dimmed; update(); }

    QSize sizeHint() const override;
    int heightForWidth(int w) const override;
    bool hasHeightForWidth() const override { return true; }

signals:
    void selectionChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    struct Key {
        qreal x, y, w, h;
    };
    static const QVector<Key> &layout();
    QRectF boardRect() const;
    qreal unit() const;
    int zoneAt(const QPointF &pos) const;
    static int zoneOfKey(const Key &k);

    ZoneColors m_colors{};
    QSet<int> m_selected;
    bool m_interactive = false;
    bool m_dimmed = false;
    int m_hoverZone = -1;
};
