#include "ui/widgets/ValueSlider.h"

#include "core/Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>

ValueSlider::ValueSlider(int minimum, int maximum, int step, QWidget *parent)
    : QWidget(parent)
    , m_slider(new QSlider(Qt::Horizontal, this))
    , m_label(new QLabel(this))
    , m_spin(new QSpinBox(this))
    , m_step(qMax(1, step))
{
    auto *h = new QHBoxLayout(this);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(14);

    m_slider->setRange(minimum, maximum);
    m_slider->setSingleStep(m_step);
    m_slider->setPageStep(m_step * 10);
    m_slider->setCursor(Qt::PointingHandCursor);
    m_slider->setMinimumHeight(22);
    h->addWidget(m_slider, 1);

    m_label->setFont(Theme::font(13, QFont::DemiBold));
    m_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_label->setMinimumWidth(52);
    h->addWidget(m_label);

    m_spin->setRange(minimum, maximum);
    m_spin->setSingleStep(m_step);
    m_spin->setAlignment(Qt::AlignRight);
    m_spin->setFixedWidth(84);
    m_spin->setVisible(false);
    m_spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    h->addWidget(m_spin);

    connect(m_slider, &QSlider::valueChanged, this, [this](int v) {
        if (m_updating)
            return;
        // snap to step
        const int snapped = m_slider->minimum() + ((v - m_slider->minimum() + m_step / 2) / m_step) * m_step;
        if (snapped != v) {
            m_updating = true;
            m_slider->setValue(qBound(m_slider->minimum(), snapped, m_slider->maximum()));
            m_updating = false;
        }
        m_updating = true;
        m_spin->setValue(m_slider->value());
        m_updating = false;
        updateLabel();
        emit valueChanged(m_slider->value());
    });
    connect(m_spin, &QSpinBox::editingFinished, this, [this]() {
        if (m_updating)
            return;
        if (m_spin->value() != m_slider->value())
            m_slider->setValue(m_spin->value());
    });
    updateLabel();
}

int ValueSlider::value() const
{
    return m_slider->value();
}

void ValueSlider::setValue(int value)
{
    if (value == m_slider->value()) {
        updateLabel();
        return;
    }
    m_updating = true;
    m_slider->setValue(value);
    m_spin->setValue(m_slider->value());
    m_updating = false;
    updateLabel();
}

void ValueSlider::setRange(int minimum, int maximum)
{
    m_slider->setRange(minimum, maximum);
    m_spin->setRange(minimum, maximum);
    updateLabel();
}

void ValueSlider::setSuffix(const QString &suffix)
{
    m_suffix = suffix;
    m_spin->setSuffix(suffix);
    updateLabel();
}

void ValueSlider::setEditable(bool editable)
{
    m_spin->setVisible(editable);
    m_label->setVisible(!editable);
}

void ValueSlider::setLabelWidth(int px)
{
    m_label->setMinimumWidth(px);
    m_spin->setFixedWidth(px + 32);
}

void ValueSlider::updateLabel()
{
    m_label->setText(QString::number(m_slider->value()) + m_suffix);
}
