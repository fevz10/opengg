#pragma once

#include <QWidget>

class QSlider;
class QLabel;
class QSpinBox;

/// Horizontal slider with a value read-out (label or editable spin box).
class ValueSlider : public QWidget
{
    Q_OBJECT
public:
    ValueSlider(int minimum, int maximum, int step = 1, QWidget *parent = nullptr);

    int value() const;
    void setValue(int value);
    void setRange(int minimum, int maximum);
    void setSuffix(const QString &suffix);
    void setEditable(bool editable);
    void setLabelWidth(int px);
    QSlider *slider() const { return m_slider; }

signals:
    void valueChanged(int value);

private:
    void updateLabel();

    QSlider *m_slider;
    QLabel *m_label;
    QSpinBox *m_spin;
    QString m_suffix;
    int m_step;
    bool m_updating = false;
};
