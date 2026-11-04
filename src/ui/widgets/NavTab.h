#pragma once

#include <QAbstractButton>

/// Text tab with the orange underline used in the GG navigation bars.
class NavTab : public QAbstractButton
{
    Q_OBJECT
public:
    explicit NavTab(const QString &text, QWidget *parent = nullptr);

    void setCompact(bool compact);
    void setBadgeText(const QString &text) { m_badge = text; updateGeometry(); update(); }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QFont tabFont() const;

    bool m_hover = false;
    bool m_compact = false;
    QString m_badge;
};
