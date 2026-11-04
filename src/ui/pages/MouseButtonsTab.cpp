#include "ui/pages/MouseButtonsTab.h"

#include "core/Icons.h"
#include "core/Theme.h"
#include "devices/Protocol.h"
#include "devices/Rival3WirelessGen2.h"
#include "ui/pages/PageHelpers.h"
#include "ui/widgets/Card.h"
#include "ui/widgets/MousePreview.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

using Protocol::ButtonAction;

namespace {
// Makes the key group / multimedia names from Protocol.cpp visible to lupdate.
const char *const kTranslatableNames[] = {
    QT_TRANSLATE_NOOP("MouseButtonsTab", "Letters"),       QT_TRANSLATE_NOOP("MouseButtonsTab", "Numbers"),
    QT_TRANSLATE_NOOP("MouseButtonsTab", "Function keys"), QT_TRANSLATE_NOOP("MouseButtonsTab", "Editing"),
    QT_TRANSLATE_NOOP("MouseButtonsTab", "Navigation"),    QT_TRANSLATE_NOOP("MouseButtonsTab", "Symbols"),
    QT_TRANSLATE_NOOP("MouseButtonsTab", "Keypad"),        QT_TRANSLATE_NOOP("MouseButtonsTab", "Modifiers"),
    QT_TRANSLATE_NOOP("MouseButtonsTab", "Play / Pause"),  QT_TRANSLATE_NOOP("MouseButtonsTab", "Next track"),
    QT_TRANSLATE_NOOP("MouseButtonsTab", "Previous track"), QT_TRANSLATE_NOOP("MouseButtonsTab", "Mute"),
    QT_TRANSLATE_NOOP("MouseButtonsTab", "Volume up"),     QT_TRANSLATE_NOOP("MouseButtonsTab", "Volume down"),
};
} // namespace

QString MouseButtonsTab::buttonName(int index)
{
    switch (index) {
    case 0: return tr("Left button");
    case 1: return tr("Right button");
    case 2: return tr("Wheel click");
    case 3: return tr("Back (side)");
    case 4: return tr("Forward (side)");
    case 5: return tr("CPI button");
    }
    return QString();
}

QString MouseButtonsTab::actionName(const ButtonAction &a)
{
    switch (a.type) {
    case ButtonAction::MouseButton:
        switch (a.code) {
        case 1: return tr("Left click");
        case 2: return tr("Right click");
        case 3: return tr("Middle click");
        case 4: return tr("Back");
        case 5: return tr("Forward");
        default: return tr("Mouse button %1").arg(a.code);
        }
    case ButtonAction::Disabled: return tr("Disabled");
    case ButtonAction::CpiSwitch: return tr("Switch CPI");
    case ButtonAction::ScrollUp: return tr("Scroll up");
    case ButtonAction::ScrollDown: return tr("Scroll down");
    case ButtonAction::Keyboard: return tr("Key: %1").arg(Protocol::keyboardKeyName(a.code));
    case ButtonAction::Multimedia: return tr(Protocol::multimediaKeyName(a.code).toLatin1().constData());
    }
    return QString();
}

MouseButtonsTab::MouseButtonsTab(Rival3WirelessGen2 *device, QWidget *parent)
    : ConfigTab(parent)
    , m_device(device)
{
    auto *h = new QHBoxLayout(this);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(16);

    // Preview
    auto *previewCard = new Card(this);
    previewCard->setMinimumWidth(330);
    previewCard->setMaximumWidth(380);
    m_preview = new MousePreview(previewCard);
    m_preview->setShowCallouts(true);
    m_preview->setMinimumHeight(300);
    previewCard->addWidget(m_preview, 1);
    auto *hint = PageHelpers::secondary(tr("Click a button on the mouse or pick an action on the right."), previewCard);
    hint->setFont(Theme::font(12));
    hint->setAlignment(Qt::AlignCenter);
    previewCard->addWidget(hint);
    h->addWidget(previewCard, 0, Qt::AlignTop);

    // Assignments
    auto *card = new Card(tr("Button assignments"), tr("Changes are written to the mouse's on-board memory."), this);
    card->bodyLayout()->setSpacing(6);
    for (int i = 0; i < MouseConfig::ButtonCount; ++i) {
        auto *row = new QWidget(card);
        row->setAttribute(Qt::WA_Hover, true);
        auto *rh = new QHBoxLayout(row);
        rh->setContentsMargins(8, 6, 8, 6);
        rh->setSpacing(12);

        auto *badge = new QLabel(QString::number(i + 1), row);
        badge->setFixedSize(26, 26);
        badge->setAlignment(Qt::AlignCenter);
        badge->setFont(Theme::font(12, QFont::Bold));
        badge->setStyleSheet(QStringLiteral("background:%1; border-radius:13px; color:%2;").arg(Theme::hex(Theme::Bg4), Theme::hex(Theme::Text)));
        rh->addWidget(badge);

        auto *name = new QLabel(buttonName(i), row);
        name->setFont(Theme::font(13, QFont::DemiBold));
        rh->addWidget(name, 1);

        auto *btn = new QToolButton(row);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        btn->setIcon(Icons::icon(QStringLiteral("chevron-down"), Theme::TextSecondary, 14));
        btn->setLayoutDirection(Qt::RightToLeft); // icon on the right
        btn->setMinimumWidth(200);
        btn->setStyleSheet(QStringLiteral("QToolButton{background:%1;border:1px solid %2;border-radius:6px;padding:6px 10px;color:%3;text-align:left;}"
                                          "QToolButton:hover{border-color:#4a4a4a;background:%4;}")
                               .arg(Theme::hex(Theme::Bg3), Theme::hex(Theme::BorderStrong), Theme::hex(Theme::Text), Theme::hex(Theme::Bg4)));
        connect(btn, &QToolButton::clicked, this, [this, i]() { showMenuFor(i); });
        rh->addWidget(btn);

        m_buttons[size_t(i)] = btn;
        m_rows[size_t(i)] = row;
        card->addWidget(row);

        row->installEventFilter(this);
    }
    card->addSpacing(6);
    auto *resetRow = new QHBoxLayout;
    resetRow->addStretch(1);
    auto *reset = new QPushButton(tr("Reset to defaults"), card);
    reset->setProperty("variant", QStringLiteral("ghost"));
    reset->setCursor(Qt::PointingHandCursor);
    connect(reset, &QPushButton::clicked, this, [this]() {
        m_cfg.buttons = Protocol::Rival3WirelessGen2::defaultButtons();
        refreshButtons();
        emitChange();
    });
    resetRow->addWidget(reset);
    card->addLayout(resetRow);
    h->addWidget(card, 1, Qt::AlignTop);

    connect(m_preview, &MousePreview::buttonClicked, this, &MouseButtonsTab::showMenuFor);
    connect(m_preview, &MousePreview::buttonHovered, this, [this](int idx) {
        for (int i = 0; i < MouseConfig::ButtonCount; ++i) {
            m_rows[size_t(i)]->setStyleSheet(i == idx ? QStringLiteral("background:%1; border-radius:8px;").arg(Theme::hex(Theme::Bg3))
                                                      : QString());
        }
    });

    loadConfig(device->currentConfig().isEmpty() ? device->defaultConfig() : device->currentConfig());
}

void MouseButtonsTab::showMenuFor(int index)
{
    if (index < 0 || index >= MouseConfig::ButtonCount)
        return;
    m_preview->setHighlightedButton(index);
    QToolButton *anchor = m_buttons[size_t(index)];

    QMenu menu(this);
    auto addAction = [&](QMenu *m, const QString &text, const ButtonAction &a) {
        QAction *act = m->addAction(text);
        act->setCheckable(true);
        act->setChecked(m_cfg.buttons[size_t(index)] == a);
        connect(act, &QAction::triggered, this, [this, index, a]() {
            m_cfg.buttons[size_t(index)] = a;
            refreshButtons();
            emitChange();
        });
        return act;
    };

    QMenu *mouse = menu.addMenu(tr("Mouse"));
    for (int code = 1; code <= 5; ++code)
        addAction(mouse, actionName({ButtonAction::MouseButton, code}), {ButtonAction::MouseButton, code});
    addAction(&menu, tr("Switch CPI"), {ButtonAction::CpiSwitch, 0});
    QMenu *scroll = menu.addMenu(tr("Scroll"));
    addAction(scroll, tr("Scroll up"), {ButtonAction::ScrollUp, 0});
    addAction(scroll, tr("Scroll down"), {ButtonAction::ScrollDown, 0});

    QMenu *keyboard = menu.addMenu(tr("Keyboard key"));
    for (const Protocol::KeyGroup &g : Protocol::keyboardKeyGroups()) {
        QMenu *gm = keyboard->addMenu(tr(g.group));
        for (const Protocol::NamedCode &k : g.keys)
            addAction(gm, QString::fromLatin1(k.name), {ButtonAction::Keyboard, k.code});
    }
    QMenu *media = menu.addMenu(tr("Multimedia"));
    for (const Protocol::NamedCode &k : Protocol::multimediaKeys())
        addAction(media, tr(k.name), {ButtonAction::Multimedia, k.code});

    menu.addSeparator();
    addAction(&menu, tr("Disabled"), {ButtonAction::Disabled, 0});
    menu.addSeparator();
    QAction *def = menu.addAction(tr("Reset this button"));
    connect(def, &QAction::triggered, this, [this, index]() {
        m_cfg.buttons[size_t(index)] = Protocol::Rival3WirelessGen2::defaultButtons()[size_t(index)];
        refreshButtons();
        emitChange();
    });

    menu.exec(anchor->mapToGlobal(QPoint(0, anchor->height() + 4)));
    m_preview->setHighlightedButton(-1);
}

bool MouseButtonsTab::eventFilter(QObject *watched, QEvent *event)
{
    for (int i = 0; i < MouseConfig::ButtonCount; ++i) {
        if (m_rows[size_t(i)] != watched)
            continue;
        if (event->type() == QEvent::Enter)
            m_preview->setHighlightedButton(i);
        else if (event->type() == QEvent::Leave && m_preview->highlightedButton() == i)
            m_preview->setHighlightedButton(-1);
        break;
    }
    return ConfigTab::eventFilter(watched, event);
}

void MouseButtonsTab::refreshButtons()
{
    for (int i = 0; i < MouseConfig::ButtonCount; ++i) {
        const ButtonAction &a = m_cfg.buttons[size_t(i)];
        m_buttons[size_t(i)]->setText(actionName(a));
        const bool modified = a != Protocol::Rival3WirelessGen2::defaultButtons()[size_t(i)];
        m_buttons[size_t(i)]->setToolTip(modified ? tr("Remapped (default: %1)").arg(actionName(Protocol::Rival3WirelessGen2::defaultButtons()[size_t(i)]))
                                                  : tr("Default"));
    }
}

void MouseButtonsTab::loadConfig(const QJsonObject &data)
{
    m_loading = true;
    m_cfg = MouseConfig::fromJson(data);
    refreshButtons();
    m_loading = false;
}

void MouseButtonsTab::emitChange()
{
    if (m_loading)
        return;
    emit configEdited(m_cfg.toJson());
}
