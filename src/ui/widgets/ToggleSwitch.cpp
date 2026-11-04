#include "ui/widgets/ToggleSwitch.h"

#include "core/Theme.h"

#include <QPainter>

ToggleSwitch::ToggleSwitch(QWidget *parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::TabFocus);
    m_anim.setDuration(140);
    m_anim.setEasingCurve(QEasingCurve::OutCubic);
    connect(&m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_pos = v.toReal();
        update();
    });
}

QSize ToggleSwitch::sizeHint() const
{
    return QSize(44, 24);
}

void ToggleSwitch::animateTo(qreal target)
{
    m_anim.stop();
    m_anim.setStartValue(m_pos);
    m_anim.setEndValue(target);
    m_anim.start();
}

void ToggleSwitch::checkStateSet()
{
    QAbstractButton::checkStateSet();
    if (isVisible())
        animateTo(isChecked() ? 1.0 : 0.0);
    else {
        m_pos = isChecked() ? 1.0 : 0.0;
        update();
    }
}

void ToggleSwitch::nextCheckState()
{
    QAbstractButton::nextCheckState();
    animateTo(isChecked() ? 1.0 : 0.0);
}

void ToggleSwitch::enterEvent(QEnterEvent *event)
{
    m_hover = true;
    update();
    QAbstractButton::enterEvent(event);
}

void ToggleSwitch::leaveEvent(QEvent *event)
{
    m_hover = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void ToggleSwitch::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF track(0.5, (height() - 22) / 2.0 + 0.5, 43, 22);
    QColor off = Theme::Bg4;
    QColor on = Theme::Accent;
    if (!isEnabled()) {
        off = Theme::Bg3;
        on = QColor(0x8a, 0x3c, 0x14);
    } else if (m_hover) {
        off = Theme::BorderStrong;
        on = Theme::AccentHover;
    }
    // blend track color
    const QColor trackColor(int(off.red() + (on.red() - off.red()) * m_pos), int(off.green() + (on.green() - off.green()) * m_pos),
                            int(off.blue() + (on.blue() - off.blue()) * m_pos));
    p.setPen(Qt::NoPen);
    p.setBrush(trackColor);
    p.drawRoundedRect(track, 11, 11);

    const qreal knobD = 16;
    const qreal x = track.left() + 3 + (track.width() - 6 - knobD) * m_pos;
    const QRectF knob(x, track.top() + 3, knobD, knobD);
    p.setBrush(isEnabled() ? QColor(Qt::white) : Theme::TextMuted);
    p.drawEllipse(knob);

    if (hasFocus()) {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(Theme::AccentSoft, 2));
        p.drawRoundedRect(track.adjusted(-1.5, -1.5, 1.5, 1.5), 12.5, 12.5);
    }
}
