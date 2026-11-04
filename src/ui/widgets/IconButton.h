#pragma once

#include <QAbstractButton>
#include <QColor>
#include <QString>

/// Flat square button showing a recolorable SVG icon, with optional badge dot.
class IconButton : public QAbstractButton
{
    Q_OBJECT
public:
    explicit IconButton(const QString &iconName, QWidget *parent = nullptr);

    void setIconName(const QString &name);
    QString iconName() const { return m_icon; }
    void setIconSize(int px) { m_iconSize = px; update(); }
    void setButtonSize(int px) { m_buttonSize = px; updateGeometry(); update(); }
    void setBadge(bool badge) { m_badge = badge; update(); }
    void setColors(const QColor &normal, const QColor &hover);
    void setAccentWhenChecked(bool v) { m_accentChecked = v; update(); }
    void setRound(bool round) { m_round = round; update(); }

    QSize sizeHint() const override { return QSize(m_buttonSize, m_buttonSize); }
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QString m_icon;
    int m_iconSize = 20;
    int m_buttonSize = 36;
    bool m_badge = false;
    bool m_hover = false;
    bool m_accentChecked = true;
    bool m_round = false;
    QColor m_normal;
    QColor m_hoverColor;
};
