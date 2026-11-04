#include "ui/widgets/TopBar.h"

#include "core/Icons.h"
#include "core/Theme.h"
#include "ui/widgets/IconButton.h"
#include "ui/widgets/NavTab.h"

#include <QButtonGroup>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>

namespace {

/// The logo at the left of the bar: the OpenGG mark (the application icon
/// SVG) followed by the two-tone "OpenGG" word mark.
class LogoMark : public QWidget
{
public:
    explicit LogoMark(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(184, 56);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        // Mark
        constexpr int markSize = 30;
        const QPixmap mark = Icons::appIcon(markSize, devicePixelRatioF());
        p.drawPixmap(QPointF(0, (height() - markSize) / 2.0), mark);

        // Word mark: "Open" in the text color, "GG" in the accent color.
        const QFont wordFont = Theme::font(15, QFont::Bold);
        const QFontMetrics fm(wordFont);
        const QString open = QStringLiteral("Open");
        const QString gg = QStringLiteral("GG");
        const int openW = fm.horizontalAdvance(open);
        const int ggW = fm.horizontalAdvance(gg);
        const qreal x = markSize + 9;
        p.setFont(wordFont);
        p.setPen(Theme::Text);
        p.drawText(QRectF(x, 0, openW + 2, height()), Qt::AlignVCenter | Qt::AlignLeft, open);
        p.setPen(Theme::Accent);
        p.drawText(QRectF(x + openW, 0, ggW + 2, height()), Qt::AlignVCenter | Qt::AlignLeft, gg);

        p.setPen(Theme::TextMuted);
        p.setFont(Theme::font(10, QFont::DemiBold));
        p.drawText(QRectF(x + openW + ggW + 7, 1, 80, height()), Qt::AlignVCenter | Qt::AlignLeft, QStringLiteral("for Linux"));
    }
};

class Avatar : public QWidget
{
public:
    explicit Avatar(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(32, 32);
    }
    void setInitial(const QString &s)
    {
        m_initial = s;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0x3a, 0x3a, 0x3a));
        p.drawEllipse(rect().adjusted(1, 1, -1, -1));
        p.setPen(QPen(Theme::Accent, 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(rect().adjusted(1, 1, -1, -1));
        p.setPen(Theme::Text);
        p.setFont(Theme::font(13, QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, m_initial);
    }

private:
    QString m_initial = QStringLiteral("U");
};

} // namespace

TopBar::TopBar(QWidget *parent)
    : QWidget(parent)
    , m_group(new QButtonGroup(this))
{
    setFixedHeight(56);
    setAutoFillBackground(false);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 0, 16, 0);
    layout->setSpacing(4);

    layout->addWidget(new LogoMark(this));
    layout->addSpacing(16);

    m_group->setExclusive(true);
    connect(m_group, &QButtonGroup::idClicked, this, [this](int id) { emit tabSelected(id); });

    // tabs are inserted before the stretch (index kept in m_tabs)
    layout->addStretch(1);

    m_bell = new IconButton(QStringLiteral("bell"), this);
    m_bell->setToolTip(tr("Notifications"));
    connect(m_bell, &QAbstractButton::clicked, this, &TopBar::notificationsClicked);
    layout->addWidget(m_bell);

    m_settings = new IconButton(QStringLiteral("settings"), this);
    m_settings->setToolTip(tr("Settings"));
    m_settings->setCheckable(true);
    connect(m_settings, &QAbstractButton::clicked, this, &TopBar::settingsClicked);
    layout->addWidget(m_settings);

    layout->addSpacing(8);
    m_avatar = new Avatar(this);
    layout->addWidget(m_avatar);
}

int TopBar::addTab(const QString &text)
{
    auto *tab = new NavTab(text, this);
    const int index = m_tabs.size();
    m_tabs.append(tab);
    m_group->addButton(tab, index);
    auto *layout = static_cast<QHBoxLayout *>(this->layout());
    // insert after logo (0) + spacing (1) + previous tabs
    layout->insertWidget(2 + index, tab);
    if (index == 0)
        tab->setChecked(true);
    return index;
}

void TopBar::setCurrentTab(int index)
{
    if (index < 0 || index >= m_tabs.size())
        return;
    m_tabs.at(index)->setChecked(true);
}

int TopBar::currentTab() const
{
    return m_group->checkedId();
}

void TopBar::setTabBadge(int index, const QString &badge)
{
    if (index >= 0 && index < m_tabs.size())
        m_tabs.at(index)->setBadgeText(badge);
}

void TopBar::setNotificationBadge(bool on)
{
    m_bell->setBadge(on);
}

void TopBar::setSettingsActive(bool active)
{
    m_settings->setChecked(active);
    if (active) {
        // Settings is not one of the tabs: clear the tab highlight.
        m_group->setExclusive(false);
        for (NavTab *t : m_tabs)
            t->setChecked(false);
        m_group->setExclusive(true);
    }
}

void TopBar::setUserName(const QString &name)
{
    m_userInitial = name.isEmpty() ? QStringLiteral("U") : name.left(1).toUpper();
    static_cast<Avatar *>(m_avatar)->setInitial(m_userInitial);
    m_avatar->setToolTip(name);
}

void TopBar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Theme::Bg0);
    p.setPen(Theme::Border);
    p.drawLine(0, height() - 1, width(), height() - 1);
}
