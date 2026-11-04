#include "ui/widgets/MousePreview.h"

#include "core/Theme.h"

#include <QMouseEvent>
#include <QPainter>

namespace {
// Design space: the mouse body occupies x 0..200, y 0..300; call-outs extend
// to the left and right (x -60..260).
constexpr qreal kDesignLeft = -62;
constexpr qreal kDesignWidth = 324;
constexpr qreal kDesignLeftNoCallout = -6;
constexpr qreal kDesignWidthNoCallout = 212;
constexpr qreal kDesignHeight = 304;

struct Callout {
    QPointF badge;  // badge center
    QPointF anchor; // point on the mouse
};

const Callout kCallouts[6] = {
    {{-38, 56}, {62, 62}},    // 1 left click
    {{238, 56}, {138, 62}},   // 2 right click
    {{238, 104}, {110, 72}},  // 3 wheel click
    {{-38, 132}, {12, 128}},  // 4 back
    {{-38, 178}, {12, 172}},  // 5 forward
    {{238, 152}, {108, 104}}, // 6 CPI
};
} // namespace

MousePreview::MousePreview(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sp.setHeightForWidth(true);
    setSizePolicy(sp);
}

void MousePreview::setShowCallouts(bool show)
{
    m_callouts = show;
    setCursor(show ? Qt::PointingHandCursor : Qt::ArrowCursor);
    updateGeometry();
    update();
}

void MousePreview::setHighlightedButton(int index)
{
    if (m_highlight == index)
        return;
    m_highlight = index;
    update();
}

QSize MousePreview::sizeHint() const
{
    return QSize(m_callouts ? 320 : 200, heightForWidth(m_callouts ? 320 : 200));
}

int MousePreview::heightForWidth(int w) const
{
    const qreal dw = m_callouts ? kDesignWidth : kDesignWidthNoCallout;
    return int(w * kDesignHeight / dw);
}

QTransform MousePreview::designTransform() const
{
    const qreal dw = m_callouts ? kDesignWidth : kDesignWidthNoCallout;
    const qreal dl = m_callouts ? kDesignLeft : kDesignLeftNoCallout;
    const qreal scale = qMin(width() / dw, height() / kDesignHeight);
    const qreal ox = (width() - dw * scale) / 2.0 - dl * scale;
    const qreal oy = (height() - kDesignHeight * scale) / 2.0;
    QTransform t;
    t.translate(ox, oy);
    t.scale(scale, scale);
    return t;
}

QPainterPath MousePreview::bodyPath() const
{
    QPainterPath path;
    path.moveTo(100, 8);
    path.cubicTo(158, 8, 192, 62, 192, 122);
    path.cubicTo(192, 196, 178, 294, 100, 294);
    path.cubicTo(22, 294, 8, 196, 8, 122);
    path.cubicTo(8, 62, 42, 8, 100, 8);
    path.closeSubpath();
    return path;
}

QPainterPath MousePreview::buttonRegion(int index) const
{
    QPainterPath body = bodyPath();
    QPainterPath r;
    switch (index) {
    case 0: { // left button: body ∩ left half above the seam
        QPainterPath half;
        half.addRect(QRectF(0, 0, 100, 118));
        r = body.intersected(half);
        QPainterPath wheel;
        wheel.addRoundedRect(QRectF(88, 36, 24, 50), 10, 10);
        r = r.subtracted(wheel);
        break;
    }
    case 1: {
        QPainterPath half;
        half.addRect(QRectF(100, 0, 100, 118));
        r = body.intersected(half);
        QPainterPath wheel;
        wheel.addRoundedRect(QRectF(88, 36, 24, 50), 10, 10);
        r = r.subtracted(wheel);
        break;
    }
    case 2:
        r.addRoundedRect(QRectF(89, 38, 22, 46), 9, 9);
        break;
    case 3:
        r.addRoundedRect(QRectF(2, 112, 14, 40), 5, 5);
        break;
    case 4:
        r.addRoundedRect(QRectF(2, 156, 14, 40), 5, 5);
        break;
    case 5:
        r.addRoundedRect(QRectF(91, 96, 18, 13), 4, 4);
        break;
    default:
        break;
    }
    return r;
}

int MousePreview::buttonAt(const QPointF &widgetPos) const
{
    const QTransform t = designTransform();
    bool ok = false;
    const QTransform inv = t.inverted(&ok);
    if (!ok)
        return -1;
    const QPointF d = inv.map(widgetPos);
    if (m_callouts) {
        for (int i = 0; i < 6; ++i)
            if (QLineF(d, kCallouts[i].badge).length() <= 16)
                return i;
    }
    // smaller regions first so they win over the big buttons
    static const int order[] = {2, 5, 3, 4, 0, 1};
    for (int i : order)
        if (buttonRegion(i).contains(d))
            return i;
    return -1;
}

void MousePreview::mousePressEvent(QMouseEvent *event)
{
    if (!m_callouts || event->button() != Qt::LeftButton)
        return;
    const int b = buttonAt(event->position());
    if (b >= 0)
        emit buttonClicked(b);
}

void MousePreview::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_callouts)
        return;
    const int b = buttonAt(event->position());
    if (b != m_hover) {
        m_hover = b;
        update();
        emit buttonHovered(b);
    }
}

void MousePreview::leaveEvent(QEvent *event)
{
    if (m_hover != -1) {
        m_hover = -1;
        update();
        emit buttonHovered(-1);
    }
    QWidget::leaveEvent(event);
}

void MousePreview::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setTransform(designTransform());
    const qreal scale = designTransform().m11();
    const qreal pen1 = 1.0 / scale; // 1 device pixel

    const QPainterPath body = bodyPath();

    // Shadow
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 90));
    p.save();
    p.translate(0, 6);
    p.drawPath(body);
    p.restore();

    // Body
    QLinearGradient g(QPointF(0, 0), QPointF(200, 300));
    g.setColorAt(0, QColor(0x33, 0x33, 0x33));
    g.setColorAt(1, QColor(0x1b, 0x1b, 0x1b));
    p.setBrush(g);
    p.setPen(QPen(QColor(0x4a, 0x4a, 0x4a), pen1 * 1.5));
    p.drawPath(body);

    // Button seam
    QPainterPath seam;
    seam.moveTo(10, 112);
    seam.cubicTo(50, 128, 150, 128, 190, 112);
    p.setPen(QPen(QColor(0x0f, 0x0f, 0x0f), pen1 * 2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(seam);
    p.drawLine(QPointF(100, 12), QPointF(100, 40));
    p.drawLine(QPointF(100, 84), QPointF(100, 122));

    // Side buttons
    p.setPen(QPen(QColor(0x4a, 0x4a, 0x4a), pen1));
    p.setBrush(QColor(0x2a, 0x2a, 0x2a));
    p.drawRoundedRect(QRectF(2, 112, 14, 40), 5, 5);
    p.drawRoundedRect(QRectF(2, 156, 14, 40), 5, 5);

    // Wheel
    p.setBrush(QColor(0x14, 0x14, 0x14));
    p.setPen(QPen(QColor(0x50, 0x50, 0x50), pen1));
    p.drawRoundedRect(QRectF(89, 38, 22, 46), 9, 9);
    p.setPen(QPen(QColor(0x3c, 0x3c, 0x3c), pen1 * 1.5));
    for (int i = 0; i < 4; ++i)
        p.drawLine(QPointF(93, 48 + i * 9), QPointF(107, 48 + i * 9));

    // CPI button
    p.setPen(QPen(QColor(0x50, 0x50, 0x50), pen1));
    p.setBrush(QColor(0x22, 0x22, 0x22));
    p.drawRoundedRect(QRectF(91, 96, 18, 13), 4, 4);

    // Logo spot on the palm (generic glow dot)
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xff, 0x52, 0x00, 60));
    p.drawEllipse(QPointF(100, 215), 16, 16);
    p.setBrush(Theme::Accent);
    p.drawEllipse(QPointF(100, 215), 6, 6);

    // Battery pill (when known)
    if (m_battery >= 0) {
        const QRectF pill(68, 250, 64, 16);
        p.setBrush(QColor(0, 0, 0, 120));
        p.drawRoundedRect(pill, 8, 8);
        QColor bc = m_battery <= 15 ? Theme::Danger : (m_battery <= 30 ? Theme::Warning : Theme::Success);
        p.setBrush(bc);
        p.drawRoundedRect(QRectF(pill.left() + 3, pill.top() + 3, (pill.width() - 6) * m_battery / 100.0, pill.height() - 6), 5, 5);
    }

    // Highlights
    for (int i = 0; i < 6; ++i) {
        const bool hl = i == m_highlight;
        const bool hv = i == m_hover && m_callouts;
        if (!hl && !hv)
            continue;
        QColor fill = Theme::Accent;
        fill.setAlpha(hl ? 110 : 60);
        p.setPen(QPen(Theme::Accent, pen1 * 1.5));
        p.setBrush(fill);
        p.drawPath(buttonRegion(i));
    }

    if (!m_callouts)
        return;

    // Call-outs
    QFont f = Theme::font(12, QFont::Bold);
    for (int i = 0; i < 6; ++i) {
        const Callout &c = kCallouts[i];
        const bool active = i == m_highlight || i == m_hover;
        QColor col = active ? Theme::Accent : QColor(0x8a, 0x8a, 0x8a);
        p.setPen(QPen(col, pen1 * 1.2));
        p.setBrush(Qt::NoBrush);
        // leader: horizontal then to anchor
        const QPointF elbow(c.badge.x() + (c.anchor.x() > c.badge.x() ? 22 : -22), c.badge.y());
        p.drawLine(c.badge, elbow);
        p.drawLine(elbow, c.anchor);
        p.setBrush(col);
        p.drawEllipse(c.anchor, 2.5, 2.5);

        p.setBrush(active ? Theme::Accent : Theme::Bg3);
        p.setPen(QPen(active ? Theme::Accent : QColor(0x55, 0x55, 0x55), pen1));
        p.drawEllipse(c.badge, 13, 13);
        p.setPen(active ? QColor(Qt::white) : Theme::Text);
        p.save();
        p.resetTransform();
        const QPointF bc = designTransform().map(c.badge);
        p.setFont(f);
        p.drawText(QRectF(bc.x() - 13, bc.y() - 13, 26, 26), Qt::AlignCenter, QString::number(i + 1));
        p.restore();
    }
}
