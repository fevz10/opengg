#include "ui/widgets/IconButton.h"

#include "core/Icons.h"
#include "core/Theme.h"

#include <QPainter>

IconButton::IconButton(const QString &iconName, QWidget *parent)
    : QAbstractButton(parent)
    , m_icon(iconName)
    , m_normal(Theme::TextSecondary)
    , m_hoverColor(Theme::Text)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
}

void IconButton::setIconName(const QString &name)
{
    m_icon = name;
    update();
}

void IconButton::setColors(const QColor &normal, const QColor &hover)
{
    m_normal = normal;
    m_hoverColor = hover;
    update();
}

void IconButton::enterEvent(QEnterEvent *event)
{
    m_hover = true;
    update();
    QAbstractButton::enterEvent(event);
}

void IconButton::leaveEvent(QEvent *event)
{
    m_hover = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void IconButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF r = rect();

    const bool active = isChecked() && m_accentChecked;
    if (m_hover || isDown() || active) {
        p.setPen(Qt::NoPen);
        p.setBrush(isDown() ? Theme::Bg4 : (active ? Theme::AccentSoft : Theme::Bg3));
        if (m_round)
            p.drawEllipse(r.adjusted(1, 1, -1, -1));
        else
            p.drawRoundedRect(r.adjusted(1, 1, -1, -1), 8, 8);
    }

    QColor color = m_hover ? m_hoverColor : m_normal;
    if (active)
        color = Theme::Accent;
    if (!isEnabled())
        color = Theme::TextMuted;

    const QPixmap pm = Icons::pixmap(m_icon, color, m_iconSize, devicePixelRatioF());
    const QRectF target((width() - m_iconSize) / 2.0, (height() - m_iconSize) / 2.0, m_iconSize, m_iconSize);
    p.drawPixmap(target.toRect(), pm);

    if (m_badge) {
        p.setPen(QPen(Theme::Bg0, 2));
        p.setBrush(Theme::Accent);
        p.drawEllipse(QPointF(target.right() - 1, target.top() + 1), 4.5, 4.5);
    }
}
