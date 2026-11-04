#pragma once

#include <QColor>
#include <QList>
#include <QWidget>

class QLineEdit;
class SvSquare;
class HueBar;
class SwatchGrid;

/// Color picker in the style of the GG illumination panel:
/// saturation/value square, hue bar, preset swatches and a hex field.
class ColorPicker : public QWidget
{
    Q_OBJECT
public:
    explicit ColorPicker(QWidget *parent = nullptr);

    QColor color() const { return m_color; }
    void setColor(const QColor &color, bool emitSignal = false);

    static QList<QColor> presetPalette();

signals:
    /// Emitted continuously while dragging.
    void colorChanged(const QColor &color);
    /// Emitted when the user finishes a change (release, swatch click, hex edit).
    void colorCommitted(const QColor &color);

private:
    void applyColor(const QColor &c, bool commit);

    QColor m_color{0xff, 0x52, 0x00};
    SvSquare *m_sv;
    HueBar *m_hue;
    SwatchGrid *m_swatches;
    QLineEdit *m_hex;
    QWidget *m_preview;
    qreal m_lastHue = 0.06;
    bool m_updating = false;
};
