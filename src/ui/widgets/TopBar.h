#pragma once

#include <QList>
#include <QWidget>

class NavTab;
class IconButton;
class QButtonGroup;
class QLabel;

/// Application wide navigation bar (logo, main tabs, notification/settings).
class TopBar : public QWidget
{
    Q_OBJECT
public:
    explicit TopBar(QWidget *parent = nullptr);

    int addTab(const QString &text);
    void setCurrentTab(int index);
    int currentTab() const;
    void setTabBadge(int index, const QString &badge);

    void setNotificationBadge(bool on);
    void setSettingsActive(bool active);
    void setUserName(const QString &name);

    QSize sizeHint() const override { return QSize(800, 56); }

signals:
    void tabSelected(int index);
    void notificationsClicked();
    void settingsClicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<NavTab *> m_tabs;
    QButtonGroup *m_group;
    IconButton *m_bell;
    IconButton *m_settings;
    QWidget *m_avatar;
    QString m_userInitial = QStringLiteral("U");
};
