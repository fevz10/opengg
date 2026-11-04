#include "ui/widgets/ColorPicker.h"

#include "core/Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
// Saturation / value square
// ---------------------------------------------------------------------------
class SvSquare : public QWidget
{
    Q_OBJECT
public:
    explicit SvSquare(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumSize(180, 120);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setFixedHeight(132);
        setCursor(Qt::CrossCursor);
    }

    void setHsv(qreal h, qreal s, qreal v)
    {
        m_h = h;
        m_s = s;
        m_v = v;
        update();
    }

signals:
    void picked(qreal s, qreal v, bool commit);

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF r = rect().adjusted(0, 0, -1, -1);
        QPainterPath clip;
        clip.addRoundedRect(r, 8, 8);
        p.setClipPath(clip);

        QLinearGradient sat(r.topLeft(), r.topRight());
        sat.setColorAt(0, Qt::white);
        sat.setColorAt(1, QColor::fromHsvF(m_h, 1.0, 1.0));
        p.fillRect(r, sat);
        QLinearGradient val(r.topLeft(), r.bottomLeft());
        val.setColorAt(0, QColor(0, 0, 0, 0));
        val.setColorAt(1, QColor(0, 0, 0, 255));
        p.fillRect(r, val);

        p.setClipping(false);
        const QPointF marker(r.left() + m_s * r.width(), r.top() + (1.0 - m_v) * r.height());
        p.setPen(QPen(Qt::black, 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(marker, 7, 7);
        p.setPen(QPen(Qt::white, 2));
        p.drawEllipse(marker, 5.5, 5.5);
    }
    void mousePressEvent(QMouseEvent *e) override { pick(e->position(), false); }
    void mouseMoveEvent(QMouseEvent *e) override
    {
        if (e->buttons() & Qt::LeftButton)
            pick(e->position(), false);
    }
    void mouseReleaseEvent(QMouseEvent *e) override { pick(e->position(), true); }

private:
    void pick(const QPointF &pos, bool commit)
    {
        const qreal s = qBound<qreal>(0.0, pos.x() / qMax(1.0, width() - 1.0), 1.0);
        const qreal v = qBound<qreal>(0.0, 1.0 - pos.y() / qMax(1.0, height() - 1.0), 1.0);
        m_s = s;
        m_v = v;
        update();
        emit picked(s, v, commit);
    }

    qreal m_h = 0.06, m_s = 1.0, m_v = 1.0;
};

// ---------------------------------------------------------------------------
// Hue bar
// ---------------------------------------------------------------------------
class HueBar : public QWidget
{
    Q_OBJECT
public:
    explicit HueBar(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setFixedHeight(18);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setCursor(Qt::PointingHandCursor);
    }
    void setHue(qreal h)
    {
        m_h = h;
        update();
    }

signals:
    void picked(qreal h, bool commit);

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF r(0, 4, width() - 1, 10);
        QLinearGradient g(r.topLeft(), r.topRight());
        for (int i = 0; i <= 6; ++i)
            g.setColorAt(i / 6.0, QColor::fromHsvF(i == 6 ? 0.9999 : i / 6.0, 1.0, 1.0));
        p.setPen(Qt::NoPen);
        p.setBrush(g);
        p.drawRoundedRect(r, 5, 5);
        const qreal x = r.left() + m_h * r.width();
        p.setPen(QPen(Qt::black, 1.5));
        p.setBrush(QColor::fromHsvF(qMin(m_h, 0.9999), 1.0, 1.0));
        p.drawEllipse(QPointF(x, r.center().y()), 7, 7);
        p.setPen(QPen(Qt::white, 2));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(x, r.center().y()), 5.5, 5.5);
    }
    void mousePressEvent(QMouseEvent *e) override { pick(e->position().x(), false); }
    void mouseMoveEvent(QMouseEvent *e) override
    {
        if (e->buttons() & Qt::LeftButton)
            pick(e->position().x(), false);
    }
    void mouseReleaseEvent(QMouseEvent *e) override { pick(e->position().x(), true); }

private:
    void pick(qreal x, bool commit)
    {
        m_h = qBound<qreal>(0.0, x / qMax(1.0, width() - 1.0), 0.9999);
        update();
        emit picked(m_h, commit);
    }
    qreal m_h = 0.06;
};

// ---------------------------------------------------------------------------
// Preset swatches
// ---------------------------------------------------------------------------
class SwatchGrid : public QWidget
{
    Q_OBJECT
public:
    explicit SwatchGrid(const QList<QColor> &colors, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_colors(colors)
    {
        setFixedHeight(rows() * (kSize + kGap) - kGap);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setCursor(Qt::PointingHandCursor);
        setMouseTracking(true);
    }
    void setCurrent(const QColor &c)
    {
        m_current = c;
        update();
    }

signals:
    void picked(const QColor &c);

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const int cols = columns();
        for (int i = 0; i < m_colors.size(); ++i) {
            const QRectF r = swatchRect(i, cols);
            const QColor &c = m_colors.at(i);
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawRoundedRect(r, 6, 6);
            if (c.rgb() == m_current.rgb()) {
                p.setPen(QPen(Qt::white, 2));
                p.setBrush(Qt::NoBrush);
                p.drawRoundedRect(r.adjusted(1, 1, -1, -1), 5, 5);
            } else if (i == m_hover) {
                p.setPen(QPen(QColor(255, 255, 255, 120), 1.5));
                p.setBrush(Qt::NoBrush);
                p.drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), 6, 6);
            }
        }
    }
    void mousePressEvent(QMouseEvent *e) override
    {
        const int i = indexAt(e->position());
        if (i >= 0)
            emit picked(m_colors.at(i));
    }
    void mouseMoveEvent(QMouseEvent *e) override
    {
        const int i = indexAt(e->position());
        if (i != m_hover) {
            m_hover = i;
            update();
        }
    }
    void leaveEvent(QEvent *) override
    {
        m_hover = -1;
        update();
    }

private:
    static constexpr int kSize = 24;
    static constexpr int kGap = 8;
    int columns() const { return qMax(1, (width() + kGap) / (kSize + kGap)); }
    int rows() const { return 2; }
    QRectF swatchRect(int i, int cols) const
    {
        const int row = i / cols, col = i % cols;
        return QRectF(col * (kSize + kGap), row * (kSize + kGap), kSize, kSize);
    }
    int indexAt(const QPointF &pos) const
    {
        const int cols = columns();
        for (int i = 0; i < m_colors.size(); ++i)
            if (swatchRect(i, cols).contains(pos))
                return i;
        return -1;
    }

    QList<QColor> m_colors;
    QColor m_current;
    int m_hover = -1;
};

// ---------------------------------------------------------------------------
// ColorPicker
// ---------------------------------------------------------------------------

QList<QColor> ColorPicker::presetPalette()
{
    return {
        QColor(0xff, 0x52, 0x00), QColor(0xff, 0x00, 0x33), QColor(0xff, 0x00, 0x99), QColor(0xb0, 0x3c, 0xff),
        QColor(0x3d, 0x5a, 0xff), QColor(0x00, 0xa8, 0xff), QColor(0x00, 0xe5, 0xff), QColor(0x00, 0xd6, 0x8f),
        QColor(0x4c, 0xff, 0x33), QColor(0xd4, 0xff, 0x00), QColor(0xff, 0xd6, 0x00), QColor(0xff, 0x8a, 0x00),
        QColor(0xff, 0xff, 0xff), QColor(0xbf, 0xbf, 0xbf), QColor(0x80, 0x80, 0x80), QColor(0xff, 0xb8, 0x8c),
    };
}

ColorPicker::ColorPicker(QWidget *parent)
    : QWidget(parent)
    , m_sv(new SvSquare(this))
    , m_hue(new HueBar(this))
    , m_swatches(new SwatchGrid(presetPalette(), this))
    , m_hex(new QLineEdit(this))
    , m_preview(new QWidget(this))
{
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(10);
    v->addWidget(m_sv);
    v->addWidget(m_hue);

    auto *row = new QHBoxLayout;
    row->setSpacing(10);
    m_preview->setFixedSize(34, 34);
    m_preview->setAutoFillBackground(true);
    row->addWidget(m_preview);
    m_hex->setFixedWidth(104);
    m_hex->setFont(Theme::font(13, QFont::DemiBold));
    m_hex->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("#?[0-9a-fA-F]{0,6}")), m_hex));
    m_hex->setPlaceholderText(QStringLiteral("#RRGGBB"));
    row->addWidget(m_hex);
    auto *hint = new QLabel(tr("Hex"), this);
    hint->setProperty("role", QStringLiteral("muted"));
    row->addWidget(hint);
    row->addStretch(1);
    v->addLayout(row);

    auto *presets = new QLabel(tr("Presets"), this);
    presets->setProperty("role", QStringLiteral("muted"));
    presets->setFont(Theme::font(11, QFont::DemiBold));
    v->addWidget(presets);
    v->addWidget(m_swatches);

    connect(m_sv, &SvSquare::picked, this, [this](qreal s, qreal v, bool commit) {
        if (m_updating)
            return;
        float h = 0, oldS = 0, oldV = 0;
        m_color.getHsvF(&h, &oldS, &oldV);
        if (h < 0)
            h = float(m_lastHue);
        applyColor(QColor::fromHsvF(h, s, v), commit);
    });
    connect(m_hue, &HueBar::picked, this, [this](qreal h, bool commit) {
        if (m_updating)
            return;
        float oldH = 0, s = 0, v = 0;
        m_color.getHsvF(&oldH, &s, &v);
        if (s <= 0.001f)
            s = 1.0f; // leaving grey: give the hue something to show
        if (v <= 0.001f)
            v = 1.0f;
        m_lastHue = h;
        applyColor(QColor::fromHsvF(h, s, v), commit);
    });
    connect(m_swatches, &SwatchGrid::picked, this, [this](const QColor &c) { applyColor(c, true); });
    connect(m_hex, &QLineEdit::editingFinished, this, [this]() {
        QString t = m_hex->text().trimmed();
        if (!t.startsWith(QLatin1Char('#')))
            t.prepend(QLatin1Char('#'));
        const QColor c(t);
        if (c.isValid() && t.size() == 7)
            applyColor(c, true);
        else
            setColor(m_color);
    });

    setColor(m_color);
}

void ColorPicker::applyColor(const QColor &c, bool commit)
{
    m_color = c;
    setColor(c, false);
    emit colorChanged(c);
    if (commit)
        emit colorCommitted(c);
}

void ColorPicker::setColor(const QColor &color, bool emitSignal)
{
    m_updating = true;
    m_color = color;
    float h = 0, s = 0, v = 0;
    color.getHsvF(&h, &s, &v);
    if (h < 0)
        h = float(m_lastHue);
    else
        m_lastHue = h;
    m_sv->setHsv(h, s, v);
    m_hue->setHue(h);
    m_swatches->setCurrent(color);
    if (!m_hex->hasFocus())
        m_hex->setText(color.name(QColor::HexRgb).toUpper());
    QPalette pal = m_preview->palette();
    pal.setColor(QPalette::Window, color);
    m_preview->setPalette(pal);
    m_preview->setStyleSheet(QStringLiteral("background:%1; border-radius:8px; border:1px solid %2;")
                                 .arg(color.name(QColor::HexRgb), Theme::hex(Theme::BorderStrong)));
    m_updating = false;
    if (emitSignal) {
        emit colorChanged(color);
        emit colorCommitted(color);
    }
}

#include "ColorPicker.moc"
