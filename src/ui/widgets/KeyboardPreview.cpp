#include "ui/widgets/KeyboardPreview.h"

#include "core/Theme.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

namespace {
constexpr qreal kBoardUnitsW = 18.25;
constexpr qreal kBoardUnitsH = 6.25;
constexpr qreal kMarginUnits = 0.45;
constexpr qreal kKeyGap = 0.12; // in units
} // namespace

const QVector<KeyboardPreview::Key> &KeyboardPreview::layout()
{
    static const QVector<Key> keys = [] {
        QVector<Key> k;
        auto row = [&k](qreal y, std::initializer_list<std::pair<qreal, qreal>> xw) {
            for (const auto &p : xw)
                k.append({p.first, y, p.second, 1.0});
        };
        // Function row
        row(0.0, {{0, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6.5, 1}, {7.5, 1}, {8.5, 1}, {9.5, 1},
                  {11, 1}, {12, 1}, {13, 1}, {14, 1}, {15.25, 1}, {16.25, 1}, {17.25, 1}});
        // Number row
        {
            std::initializer_list<std::pair<qreal, qreal>> r = {{0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1},
                                                                {8, 1}, {9, 1}, {10, 1}, {11, 1}, {12, 1}, {13, 2},
                                                                {15.25, 1}, {16.25, 1}, {17.25, 1}};
            row(1.25, r);
        }
        // QWERTY row
        row(2.25, {{0, 1.5}, {1.5, 1}, {2.5, 1}, {3.5, 1}, {4.5, 1}, {5.5, 1}, {6.5, 1}, {7.5, 1}, {8.5, 1}, {9.5, 1},
                   {10.5, 1}, {11.5, 1}, {12.5, 1}, {13.5, 1.5}, {15.25, 1}, {16.25, 1}, {17.25, 1}});
        // Home row
        row(3.25, {{0, 1.75}, {1.75, 1}, {2.75, 1}, {3.75, 1}, {4.75, 1}, {5.75, 1}, {6.75, 1}, {7.75, 1}, {8.75, 1},
                   {9.75, 1}, {10.75, 1}, {11.75, 1}, {12.75, 2.25}});
        // Shift row
        row(4.25, {{0, 2.25}, {2.25, 1}, {3.25, 1}, {4.25, 1}, {5.25, 1}, {6.25, 1}, {7.25, 1}, {8.25, 1}, {9.25, 1},
                   {10.25, 1}, {11.25, 1}, {12.25, 2.75}, {16.25, 1}});
        // Bottom row
        row(5.25, {{0, 1.25}, {1.25, 1.25}, {2.5, 1.25}, {3.75, 6.25}, {10, 1.25}, {11.25, 1.25}, {12.5, 1.25},
                   {13.75, 1.25}, {15.25, 1}, {16.25, 1}, {17.25, 1}});
        return k;
    }();
    return keys;
}

KeyboardPreview::KeyboardPreview(QWidget *parent)
    : QWidget(parent)
{
    m_colors.fill(0xff000000);
    setMouseTracking(true);
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sp.setHeightForWidth(true);
    setSizePolicy(sp);
}

QSize KeyboardPreview::sizeHint() const
{
    return QSize(560, heightForWidth(560));
}

int KeyboardPreview::heightForWidth(int w) const
{
    const qreal u = w / (kBoardUnitsW + 2 * kMarginUnits);
    int h = int(u * (kBoardUnitsH + 2 * kMarginUnits));
    if (m_interactive)
        h += 26; // zone labels
    return h;
}

qreal KeyboardPreview::unit() const
{
    const qreal availH = height() - (m_interactive ? 26 : 0);
    const qreal uw = width() / (kBoardUnitsW + 2 * kMarginUnits);
    const qreal uh = availH / (kBoardUnitsH + 2 * kMarginUnits);
    return qMin(uw, uh);
}

QRectF KeyboardPreview::boardRect() const
{
    const qreal u = unit();
    const qreal w = u * (kBoardUnitsW + 2 * kMarginUnits);
    const qreal h = u * (kBoardUnitsH + 2 * kMarginUnits);
    const qreal availH = height() - (m_interactive ? 26 : 0);
    return QRectF((width() - w) / 2.0, (availH - h) / 2.0, w, h);
}

int KeyboardPreview::zoneOfKey(const Key &k)
{
    const qreal cx = k.x + k.w / 2.0;
    int z = int(cx / (kBoardUnitsW / Protocol::Apex3Tkl::ZoneCount));
    return qBound(0, z, Protocol::Apex3Tkl::ZoneCount - 1);
}

int KeyboardPreview::zoneAt(const QPointF &pos) const
{
    const QRectF b = boardRect();
    const qreal u = unit();
    const QRectF keyArea(b.left() + kMarginUnits * u, b.top() + kMarginUnits * u, kBoardUnitsW * u, kBoardUnitsH * u);
    if (pos.x() < keyArea.left() || pos.x() > keyArea.right())
        return -1;
    if (pos.y() < keyArea.top() || pos.y() > keyArea.bottom() + (m_interactive ? 26 : 0))
        return -1;
    const int z = int((pos.x() - keyArea.left()) / (keyArea.width() / Protocol::Apex3Tkl::ZoneCount));
    return qBound(0, z, Protocol::Apex3Tkl::ZoneCount - 1);
}

void KeyboardPreview::setFrame(const ZoneColors &colors)
{
    if (m_colors == colors)
        return;
    m_colors = colors;
    update();
}

void KeyboardPreview::setInteractive(bool interactive)
{
    m_interactive = interactive;
    setCursor(interactive ? Qt::PointingHandCursor : Qt::ArrowCursor);
    updateGeometry();
    update();
}

void KeyboardPreview::setSelectedZones(const QSet<int> &zones)
{
    if (m_selected == zones)
        return;
    m_selected = zones;
    update();
    emit selectionChanged();
}

void KeyboardPreview::selectAll()
{
    QSet<int> all;
    for (int i = 0; i < Protocol::Apex3Tkl::ZoneCount; ++i)
        all.insert(i);
    setSelectedZones(all);
}

void KeyboardPreview::mousePressEvent(QMouseEvent *event)
{
    if (!m_interactive || event->button() != Qt::LeftButton)
        return;
    const int z = zoneAt(event->position());
    if (z < 0)
        return;
    QSet<int> sel = m_selected;
    if (event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)) {
        if (sel.contains(z))
            sel.remove(z);
        else
            sel.insert(z);
    } else {
        if (sel.size() == 1 && sel.contains(z)) {
            selectAll(); // clicking the only selected zone again selects everything
            return;
        }
        sel = {z};
    }
    setSelectedZones(sel);
}

void KeyboardPreview::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_interactive)
        return;
    const int z = zoneAt(event->position());
    if (z != m_hoverZone) {
        m_hoverZone = z;
        update();
    }
}

void KeyboardPreview::leaveEvent(QEvent *event)
{
    m_hoverZone = -1;
    update();
    QWidget::leaveEvent(event);
}

void KeyboardPreview::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF b = boardRect();
    const qreal u = unit();
    const QRectF keyArea(b.left() + kMarginUnits * u, b.top() + kMarginUnits * u, kBoardUnitsW * u, kBoardUnitsH * u);
    const qreal zoneW = keyArea.width() / Protocol::Apex3Tkl::ZoneCount;

    // Body
    p.setPen(QPen(QColor(0x3a, 0x3a, 0x3a), 1));
    p.setBrush(QColor(0x17, 0x17, 0x17));
    p.drawRoundedRect(b, u * 0.35, u * 0.35);

    // Zone glow behind the keys
    for (int z = 0; z < Protocol::Apex3Tkl::ZoneCount; ++z) {
        QColor c = QColor::fromRgb(m_colors[size_t(z)]);
        if (m_dimmed)
            c = c.darker(220);
        if (c.red() + c.green() + c.blue() == 0)
            continue;
        QColor glow = c;
        glow.setAlpha(42);
        p.setPen(Qt::NoPen);
        p.setBrush(glow);
        p.drawRect(QRectF(keyArea.left() + z * zoneW, keyArea.top() - u * 0.15, zoneW, keyArea.height() + u * 0.3));
    }

    // Keys
    for (const Key &k : layout()) {
        const int z = zoneOfKey(k);
        QColor c = QColor::fromRgb(m_colors[size_t(z)]);
        if (m_dimmed)
            c = c.darker(220);
        const bool off = c.red() + c.green() + c.blue() == 0;
        const QRectF kr(keyArea.left() + (k.x + kKeyGap / 2) * u, keyArea.top() + (k.y + kKeyGap / 2) * u,
                        (k.w - kKeyGap) * u, (k.h - kKeyGap) * u);
        QColor fill = off ? QColor(0x24, 0x24, 0x24) : QColor::fromHsvF(qMax(0.0, c.hueF()), c.saturationF() * 0.85, qMax(0.18, c.valueF() * 0.42));
        if (c.saturationF() < 0.05 && !off)
            fill = QColor::fromHsvF(0, 0, qMax(0.2, c.valueF() * 0.5));
        QColor edge = off ? QColor(0x33, 0x33, 0x33) : c;
        edge.setAlpha(off ? 255 : 200);
        p.setPen(QPen(edge, qMax(1.0, u * 0.06)));
        p.setBrush(fill);
        p.drawRoundedRect(kr, u * 0.12, u * 0.12);
        // key top highlight
        if (!off) {
            QColor top = c;
            top.setAlpha(70);
            p.setPen(Qt::NoPen);
            p.setBrush(top);
            p.drawRoundedRect(kr.adjusted(u * 0.14, u * 0.1, -u * 0.14, -kr.height() * 0.55), u * 0.08, u * 0.08);
        }
    }

    if (!m_interactive)
        return;

    // Zone selection / hover overlays and labels
    p.setFont(Theme::font(11, QFont::DemiBold));
    for (int z = 0; z < Protocol::Apex3Tkl::ZoneCount; ++z) {
        const QRectF zr(keyArea.left() + z * zoneW, keyArea.top() - u * 0.2, zoneW, keyArea.height() + u * 0.4);
        const bool allSelected = m_selected.size() == Protocol::Apex3Tkl::ZoneCount;
        const bool sel = !allSelected && m_selected.contains(z);
        if (sel || z == m_hoverZone) {
            QColor fill = Theme::Accent;
            fill.setAlpha(sel ? 46 : 22);
            p.setPen(sel ? QPen(Theme::Accent, 1.5) : QPen(QColor(255, 255, 255, 60), 1));
            p.setBrush(fill);
            p.drawRoundedRect(zr.adjusted(1.5, 0, -1.5, 0), 4, 4);
        }
        const QRectF lr(zr.left(), b.bottom() + 6, zr.width(), 18);
        p.setPen(sel ? Theme::Accent : Theme::TextMuted);
        p.drawText(lr, Qt::AlignCenter, QString::number(z + 1));
    }
}
