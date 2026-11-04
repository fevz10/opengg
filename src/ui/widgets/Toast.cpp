#include "ui/widgets/Toast.h"

#include "core/Icons.h"
#include "core/Theme.h"

#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPropertyAnimation>
#include <QTimer>

Toast::Toast(const QString &message, Type type, QWidget *parent)
    : QWidget(parent)
    , m_type(type)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    auto *h = new QHBoxLayout(this);
    h->setContentsMargins(14, 10, 16, 10);
    h->setSpacing(10);

    QString iconName = QStringLiteral("info");
    QColor color = Theme::Info;
    switch (type) {
    case Success: iconName = QStringLiteral("check-circle"); color = Theme::Success; break;
    case Warning: iconName = QStringLiteral("alert-triangle"); color = Theme::Warning; break;
    case Error: iconName = QStringLiteral("alert-triangle"); color = Theme::Danger; break;
    case Info: break;
    }
    auto *icon = new QLabel(this);
    icon->setPixmap(Icons::pixmap(iconName, color, 18));
    icon->setFixedSize(18, 18);
    h->addWidget(icon);

    m_label = new QLabel(message, this);
    m_label->setFont(Theme::font(13, QFont::DemiBold));
    m_label->setWordWrap(true);
    m_label->setMaximumWidth(360);
    h->addWidget(m_label);
}

QSize Toast::sizeHint() const
{
    return QWidget::sizeHint();
}

void Toast::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    p.setPen(QPen(Theme::BorderStrong, 1));
    p.setBrush(QColor(0x25, 0x25, 0x25));
    p.drawRoundedRect(r, 10, 10);
    QColor accent = Theme::Info;
    switch (m_type) {
    case Success: accent = Theme::Success; break;
    case Warning: accent = Theme::Warning; break;
    case Error: accent = Theme::Danger; break;
    case Info: break;
    }
    p.setPen(Qt::NoPen);
    p.setBrush(accent);
    p.drawRoundedRect(QRectF(r.left() + 1, r.top() + 8, 3, r.height() - 16), 1.5, 1.5);
}

// ---------------------------------------------------------------------------

ToastHost::ToastHost(QWidget *host)
    : QObject(host)
    , m_host(host)
{
}

void ToastHost::show(const QString &message, Toast::Type type, int durationMs)
{
    auto *toast = new Toast(message, type, m_host);
    toast->adjustSize();
    m_toasts.append(toast);
    relayout();
    toast->show();
    toast->raise();

    auto *effect = new QGraphicsOpacityEffect(toast);
    toast->setGraphicsEffect(effect);
    auto *fadeIn = new QPropertyAnimation(effect, "opacity", toast);
    fadeIn->setDuration(160);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

    QTimer::singleShot(durationMs, toast, [this, toast, effect]() {
        auto *fadeOut = new QPropertyAnimation(effect, "opacity", toast);
        fadeOut->setDuration(260);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        connect(fadeOut, &QPropertyAnimation::finished, toast, [this, toast]() {
            m_toasts.removeAll(toast);
            toast->deleteLater();
            relayout();
        });
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

void ToastHost::relayout()
{
    int y = m_host->height() - 24;
    for (int i = m_toasts.size() - 1; i >= 0; --i) {
        Toast *t = m_toasts.at(i);
        const QSize s = t->sizeHint();
        y -= s.height();
        t->setGeometry(m_host->width() - s.width() - 24, y, s.width(), s.height());
        y -= 10;
    }
}
