#include "ui/widgets/NavTab.h"

#include "core/Theme.h"

#include <QFontMetrics>
#include <QPainter>

NavTab::NavTab(const QString &text, QWidget *parent)
    : QAbstractButton(parent)
{
    setText(text);
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
}

void NavTab::setCompact(bool compact)
{
    m_compact = compact;
    updateGeometry();
    update();
}

QFont NavTab::tabFont() const
{
    return Theme::font(m_compact ? 13 : 14, QFont::DemiBold);
}

QSize NavTab::sizeHint() const
{
    const QFontMetrics fm(tabFont());
    int w = fm.horizontalAdvance(text()) + (m_compact ? 24 : 32);
    if (!m_badge.isEmpty())
        w += QFontMetrics(Theme::font(10, QFont::Bold)).horizontalAdvance(m_badge) + 18;
    return QSize(w, m_compact ? 40 : 56);
}

void NavTab::enterEvent(QEnterEvent *event)
{
    m_hover = true;
    update();
    QAbstractButton::enterEvent(event);
}

void NavTab::leaveEvent(QEvent *event)
{
    m_hover = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void NavTab::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setFont(tabFont());

    QColor color = Theme::TextSecondary;
    if (isChecked())
        color = Theme::Text;
    else if (m_hover)
        color = QColor(0xd8, 0xd8, 0xd8);
    if (!isEnabled())
        color = Theme::TextMuted;

    const QFontMetrics fm(tabFont());
    const int textW = fm.horizontalAdvance(text());
    int badgeW = 0;
    if (!m_badge.isEmpty())
        badgeW = QFontMetrics(Theme::font(10, QFont::Bold)).horizontalAdvance(m_badge) + 12;
    const int totalW = textW + (badgeW ? badgeW + 6 : 0);
    const int x0 = (width() - totalW) / 2;

    p.setPen(color);
    p.drawText(QRect(x0, 0, textW, height()), Qt::AlignVCenter | Qt::AlignLeft, text());

    if (badgeW) {
        const QRectF br(x0 + textW + 6, height() / 2.0 - 8, badgeW, 16);
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::Bg4);
        p.drawRoundedRect(br, 8, 8);
        p.setPen(Theme::TextSecondary);
        p.setFont(Theme::font(10, QFont::Bold));
        p.drawText(br, Qt::AlignCenter, m_badge);
    }

    if (isChecked()) {
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::Accent);
        const int lineW = totalW;
        p.drawRoundedRect(QRectF(x0, height() - 3, lineW, 3), 1.5, 1.5);
    }
}
