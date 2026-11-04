#pragma once

#include <QAbstractButton>
#include <QVariantAnimation>

/// iOS/GG style on-off switch.
class ToggleSwitch : public QAbstractButton
{
    Q_OBJECT
public:
    explicit ToggleSwitch(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent *event) override;
    void checkStateSet() override;
    void nextCheckState() override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void animateTo(qreal target);

    QVariantAnimation m_anim;
    qreal m_pos = 0.0; // 0 = off, 1 = on
    bool m_hover = false;
};
