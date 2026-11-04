#include "ui/widgets/SegmentedControl.h"

#include "core/Theme.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>

namespace {
constexpr int kHeight = 36;
constexpr int kPadding = 3;
}

SegmentedControl::SegmentedControl(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void SegmentedControl::setItems(const QStringList &items)
{
    m_items = items;
    m_current = qBound(0, m_current, qMax(0, int(m_items.size()) - 1));
    updateGeometry();
    update();
}

void SegmentedControl::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_items.size() || index == m_current)
        return;
    m_current = index;
    update();
    emit currentIndexChanged(index);
}

QSize SegmentedControl::sizeHint() const
{
    const QFontMetrics fm(Theme::font(13, QFont::DemiBold));
    int maxW = 0;
    for (const QString &s : m_items)
        maxW = qMax(maxW, fm.horizontalAdvance(s));
    const int segW = maxW + 28;
    return QSize(segW * qMax(1, int(m_items.size())) + kPadding * 2, kHeight);
}

qreal SegmentedControl::segmentWidth() const
{
    return m_items.isEmpty() ? 0 : (width() - kPadding * 2) / qreal(m_items.size());
}

int SegmentedControl::indexAt(const QPoint &pos) const
{
    if (m_items.isEmpty())
        return -1;
    const int i = int((pos.x() - kPadding) / segmentWidth());
    return (i >= 0 && i < m_items.size()) ? i : -1;
}

void SegmentedControl::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const int i = indexAt(event->pos());
        if (i >= 0)
            setCurrentIndex(i);
    }
}

void SegmentedControl::mouseMoveEvent(QMouseEvent *event)
{
    const int i = indexAt(event->pos());
    if (i != m_hover) {
        m_hover = i;
        update();
    }
}

void SegmentedControl::leaveEvent(QEvent *event)
{
    m_hover = -1;
    update();
    QWidget::leaveEvent(event);
}

void SegmentedControl::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF r = rect();
    p.setPen(Qt::NoPen);
    p.setBrush(Theme::Bg3);
    p.drawRoundedRect(r, 8, 8);

    const qreal segW = segmentWidth();
    p.setFont(Theme::font(13, QFont::DemiBold));
    for (int i = 0; i < m_items.size(); ++i) {
        const QRectF seg(kPadding + i * segW, kPadding, segW, height() - kPadding * 2);
        if (i == m_current) {
            p.setPen(Qt::NoPen);
            p.setBrush(Theme::Accent);
            p.drawRoundedRect(seg.adjusted(1, 0, -1, 0), 6, 6);
            p.setPen(Qt::white);
        } else {
            if (i == m_hover) {
                p.setPen(Qt::NoPen);
                p.setBrush(Theme::Bg4);
                p.drawRoundedRect(seg.adjusted(1, 0, -1, 0), 6, 6);
            }
            p.setPen(isEnabled() ? Theme::TextSecondary : Theme::TextMuted);
        }
        p.drawText(seg, Qt::AlignCenter, m_items.at(i));
    }
}
