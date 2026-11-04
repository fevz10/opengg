#include "ui/widgets/ConfigList.h"

#include "core/Icons.h"
#include "core/Theme.h"
#include "ui/widgets/IconButton.h"
#include "ui/widgets/SectionLabel.h"

#include <QAbstractButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QVBoxLayout>

namespace {

class ConfigRow : public QAbstractButton
{
    Q_OBJECT
public:
    ConfigRow(const ConfigStore::NamedConfig &cfg, bool active, bool deletable, QWidget *parent)
        : QAbstractButton(parent)
        , m_id(cfg.id)
        , m_active(active)
    {
        setCheckable(true);
        setChecked(active);
        setCursor(Qt::PointingHandCursor);
        setFixedHeight(44);
        setFocusPolicy(Qt::NoFocus);

        auto *h = new QHBoxLayout(this);
        h->setContentsMargins(14, 0, 6, 0);
        h->setSpacing(8);
        auto *name = new QLabel(cfg.name, this);
        name->setFont(Theme::font(13, active ? QFont::DemiBold : QFont::Normal));
        name->setAttribute(Qt::WA_TransparentForMouseEvents);
        h->addWidget(name, 1);
        if (active) {
            auto *badge = new QLabel(tr("Active"), this);
            badge->setProperty("role", QStringLiteral("badge-accent"));
            badge->setAttribute(Qt::WA_TransparentForMouseEvents);
            h->addWidget(badge);
        }
        auto *more = new IconButton(QStringLiteral("more-vertical"), this);
        more->setButtonSize(28);
        more->setIconSize(16);
        h->addWidget(more);
        connect(more, &QAbstractButton::clicked, this, [this, more, deletable]() {
            QMenu menu(this);
            QAction *rename = menu.addAction(Icons::icon(QStringLiteral("edit"), Theme::TextSecondary, 16), tr("Rename"));
            QAction *dup = menu.addAction(Icons::icon(QStringLiteral("copy"), Theme::TextSecondary, 16), tr("Duplicate"));
            menu.addSeparator();
            QAction *del = menu.addAction(Icons::icon(QStringLiteral("trash"), Theme::Danger, 16), tr("Delete"));
            del->setEnabled(deletable);
            QAction *picked = menu.exec(more->mapToGlobal(QPoint(0, more->height() + 4)));
            if (picked == rename)
                emit renameRequested(m_id);
            else if (picked == dup)
                emit duplicateRequested(m_id);
            else if (picked == del)
                emit deleteRequested(m_id);
        });
    }

signals:
    void renameRequested(const QString &id);
    void duplicateRequested(const QString &id);
    void deleteRequested(const QString &id);

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        p.setPen(Qt::NoPen);
        if (m_active)
            p.setBrush(Theme::Bg3);
        else if (underMouse())
            p.setBrush(QColor(255, 255, 255, 10));
        else
            p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r, 8, 8);
        if (m_active) {
            p.setBrush(Theme::Accent);
            p.drawRoundedRect(QRectF(r.left() + 1, r.top() + 10, 3, r.height() - 20), 1.5, 1.5);
        }
    }
    void enterEvent(QEnterEvent *) override { update(); }
    void leaveEvent(QEvent *) override { update(); }

private:
    QString m_id;
    bool m_active;
};

} // namespace

ConfigList::ConfigList(QWidget *parent)
    : QWidget(parent)
{
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(6);

    auto *header = new QHBoxLayout;
    header->setContentsMargins(4, 0, 0, 0);
    header->addWidget(new SectionLabel(tr("Configs"), this), 1);
    auto *add = new IconButton(QStringLiteral("plus"), this);
    add->setButtonSize(28);
    add->setIconSize(16);
    add->setToolTip(tr("New config"));
    connect(add, &QAbstractButton::clicked, this, &ConfigList::addRequested);
    header->addWidget(add);
    v->addLayout(header);

    m_rows = new QVBoxLayout;
    m_rows->setSpacing(2);
    v->addLayout(m_rows);
}

void ConfigList::setConfigs(const QList<ConfigStore::NamedConfig> &configs, const QString &activeId)
{
    for (QWidget *w : m_rowWidgets) {
        m_rows->removeWidget(w);
        w->deleteLater();
    }
    m_rowWidgets.clear();

    for (const ConfigStore::NamedConfig &cfg : configs) {
        auto *row = new ConfigRow(cfg, cfg.id == activeId, configs.size() > 1, this);
        connect(row, &QAbstractButton::clicked, this, [this, id = cfg.id]() { emit configSelected(id); });
        connect(row, &ConfigRow::renameRequested, this, &ConfigList::renameRequested);
        connect(row, &ConfigRow::duplicateRequested, this, &ConfigList::duplicateRequested);
        connect(row, &ConfigRow::deleteRequested, this, &ConfigList::deleteRequested);
        m_rows->addWidget(row);
        m_rowWidgets.append(row);
    }
}

#include "ConfigList.moc"
