#include "ui/pages/KeyboardIlluminationTab.h"

#include "core/Icons.h"
#include "core/Theme.h"
#include "devices/Apex3Tkl.h"
#include "lighting/LightingEngine.h"
#include "ui/pages/PageHelpers.h"
#include "ui/widgets/Card.h"
#include "ui/widgets/ColorPicker.h"
#include "ui/widgets/IconButton.h"
#include "ui/widgets/KeyboardPreview.h"
#include "ui/widgets/ValueSlider.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
// Row of gradient stops for the Color Shift effect
// ---------------------------------------------------------------------------
class StopStrip : public QWidget
{
    Q_OBJECT
public:
    explicit StopStrip(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setFixedHeight(44);
        setCursor(Qt::PointingHandCursor);
    }
    void setStops(const QList<QColor> &stops)
    {
        m_stops = stops;
        m_selected = qBound(0, m_selected, qMax(0, int(m_stops.size()) - 1));
        update();
    }
    const QList<QColor> &stops() const { return m_stops; }
    int selected() const { return m_selected; }
    void setSelected(int i)
    {
        m_selected = qBound(0, i, qMax(0, int(m_stops.size()) - 1));
        update();
        emit selectedChanged(m_selected);
    }

signals:
    void selectedChanged(int index);

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        // gradient preview bar
        const QRectF bar(0, 0, width() - 1, 14);
        QLinearGradient g(bar.topLeft(), bar.topRight());
        const int n = m_stops.size();
        for (int i = 0; i <= n; ++i)
            g.setColorAt(n == 0 ? 0 : qreal(i) / n, m_stops.isEmpty() ? QColor(Qt::black) : m_stops.at(i % n));
        p.setPen(Qt::NoPen);
        p.setBrush(g);
        p.drawRoundedRect(bar, 7, 7);
        // swatches
        for (int i = 0; i < n; ++i) {
            const QRectF r = swatchRect(i);
            p.setPen(Qt::NoPen);
            p.setBrush(m_stops.at(i));
            p.drawRoundedRect(r, 6, 6);
            if (i == m_selected) {
                p.setPen(QPen(Qt::white, 2));
                p.setBrush(Qt::NoBrush);
                p.drawRoundedRect(r.adjusted(1, 1, -1, -1), 5, 5);
            }
        }
    }
    void mousePressEvent(QMouseEvent *e) override
    {
        for (int i = 0; i < m_stops.size(); ++i)
            if (swatchRect(i).contains(e->position())) {
                setSelected(i);
                return;
            }
    }

private:
    QRectF swatchRect(int i) const { return QRectF(i * 32, 20, 24, 24); }
    QList<QColor> m_stops;
    int m_selected = 0;
};

// ---------------------------------------------------------------------------

KeyboardIlluminationTab::KeyboardIlluminationTab(Apex3Tkl *device, QWidget *parent)
    : ConfigTab(parent)
    , m_device(device)
{
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(16);

    // ---- preview --------------------------------------------------------------
    auto *previewCard = new Card(this);
    previewCard->bodyLayout()->setSpacing(10);
    m_preview = new KeyboardPreview(previewCard);
    m_preview->setInteractive(true);
    m_preview->setMaximumHeight(300);
    m_preview->selectAll();
    connect(device->engine(), &LightingEngine::previewFrame, m_preview, &KeyboardPreview::setFrame);
    m_preview->setFrame(device->engine()->lastPreviewFrame());
    previewCard->addWidget(m_preview);

    auto *hintRow = new QHBoxLayout;
    m_zoneHint = PageHelpers::secondary(QString(), previewCard);
    m_zoneHint->setFont(Theme::font(12));
    hintRow->addWidget(m_zoneHint, 1);
    m_selectAll = new QPushButton(tr("Select all zones"), previewCard);
    m_selectAll->setProperty("variant", QStringLiteral("link"));
    m_selectAll->setCursor(Qt::PointingHandCursor);
    connect(m_selectAll, &QPushButton::clicked, m_preview, &KeyboardPreview::selectAll);
    hintRow->addWidget(m_selectAll);
    previewCard->addLayout(hintRow);
    v->addWidget(previewCard);

    // ---- controls -------------------------------------------------------------
    auto *columns = new QHBoxLayout;
    columns->setSpacing(16);
    auto *leftCol = new QVBoxLayout;
    leftCol->setSpacing(16);
    auto *rightCol = new QVBoxLayout;
    rightCol->setSpacing(16);

    // Effect
    auto *effectCard = new Card(tr("Effect"), QString(), this);
    m_effect = new QComboBox(effectCard);
    m_effect->addItem(tr("Steady"), int(KeyboardConfig::Steady));
    m_effect->addItem(tr("Breathe"), int(KeyboardConfig::Breathe));
    m_effect->addItem(tr("Color Shift"), int(KeyboardConfig::ColorShift));
    m_effect->addItem(tr("Rainbow Wave"), int(KeyboardConfig::RainbowWave));
    m_effect->addItem(tr("Off"), int(KeyboardConfig::Off));
    m_effect->setCursor(Qt::PointingHandCursor);
    effectCard->addWidget(m_effect);
    m_effectDescription = PageHelpers::secondary(QString(), effectCard);
    m_effectDescription->setFont(Theme::font(12));
    effectCard->addWidget(m_effectDescription);
    leftCol->addWidget(effectCard);

    // Speed
    m_speedCard = new Card(tr("Speed"), QString(), this);
    m_speed = new ValueSlider(0, 100, 5, m_speedCard);
    m_speed->setSuffix(QStringLiteral("%"));
    m_speedCard->addWidget(m_speed);
    leftCol->addWidget(m_speedCard);

    // Brightness
    auto *brightnessCard = new Card(tr("Brightness"), tr("Also adjustable with the brightness keys on the keyboard."), this);
    auto *bRow = new QHBoxLayout;
    bRow->setSpacing(12);
    auto *sun = new QLabel(brightnessCard);
    sun->setPixmap(Icons::pixmap(QStringLiteral("sun"), Theme::TextSecondary, 18));
    bRow->addWidget(sun);
    m_brightness = new ValueSlider(0, 100, 1, brightnessCard);
    m_brightness->setSuffix(QStringLiteral("%"));
    bRow->addWidget(m_brightness, 1);
    brightnessCard->addLayout(bRow);
    leftCol->addWidget(brightnessCard);
    leftCol->addStretch(1);

    // Color (steady / breathe)
    m_colorCard = new Card(tr("Color"), tr("Applies to the selected zones."), this);
    m_picker = new ColorPicker(m_colorCard);
    m_colorCard->addWidget(m_picker);
    m_applyAll = new QPushButton(tr("Apply this color to all zones"), m_colorCard);
    m_applyAll->setCursor(Qt::PointingHandCursor);
    m_colorCard->addWidget(m_applyAll, 0);
    rightCol->addWidget(m_colorCard);

    // Gradient stops (color shift)
    m_stopsCard = new Card(tr("Colors"), tr("The effect cycles through these colors across the zones."), this);
    auto *stopsRow = new QHBoxLayout;
    stopsRow->setSpacing(8);
    m_stops = new StopStrip(m_stopsCard);
    stopsRow->addWidget(m_stops, 1);
    auto *addStop = new IconButton(QStringLiteral("plus"), m_stopsCard);
    addStop->setButtonSize(30);
    addStop->setIconSize(16);
    addStop->setToolTip(tr("Add color"));
    auto *removeStop = new IconButton(QStringLiteral("minus"), m_stopsCard);
    removeStop->setButtonSize(30);
    removeStop->setIconSize(16);
    removeStop->setToolTip(tr("Remove selected color"));
    stopsRow->addWidget(addStop, 0, Qt::AlignTop);
    stopsRow->addWidget(removeStop, 0, Qt::AlignTop);
    m_stopsCard->addLayout(stopsRow);
    m_stopPicker = new ColorPicker(m_stopsCard);
    m_stopsCard->addWidget(m_stopPicker);
    rightCol->addWidget(m_stopsCard);
    rightCol->addStretch(1);

    columns->addLayout(leftCol, 10);
    columns->addLayout(rightCol, 11);
    v->addLayout(columns);

    // ---- behaviour ------------------------------------------------------------
    connect(m_preview, &KeyboardPreview::selectionChanged, this, [this]() {
        updateZoneHint();
        syncPickerFromSelection();
    });
    connect(m_effect, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (m_loading)
            return;
        m_cfg.effect = KeyboardConfig::Effect(m_effect->currentData().toInt());
        updateVisibility();
        emitChange();
    });
    connect(m_speed, &ValueSlider::valueChanged, this, [this](int v) {
        if (m_loading)
            return;
        m_cfg.speed = v;
        emitChange();
    });
    connect(m_brightness, &ValueSlider::valueChanged, this, [this](int v) {
        if (m_loading)
            return;
        m_cfg.brightness = v;
        emitChange();
    });
    connect(m_picker, &ColorPicker::colorChanged, this, [this](const QColor &c) {
        if (m_loading)
            return;
        QSet<int> zones = m_preview->selectedZones();
        if (zones.isEmpty()) {
            m_preview->selectAll();
            zones = m_preview->selectedZones();
        }
        for (int z : zones)
            m_cfg.zoneColors[size_t(z)] = c;
        emitChange();
    });
    connect(m_applyAll, &QPushButton::clicked, this, [this]() {
        const QColor c = m_picker->color();
        m_cfg.zoneColors.fill(c);
        m_preview->selectAll();
        emitChange();
    });
    connect(m_stops, &StopStrip::selectedChanged, this, [this](int i) {
        if (i >= 0 && i < m_cfg.shiftColors.size())
            m_stopPicker->setColor(m_cfg.shiftColors.at(i));
    });
    connect(m_stopPicker, &ColorPicker::colorChanged, this, [this](const QColor &c) {
        if (m_loading)
            return;
        const int i = m_stops->selected();
        if (i < 0 || i >= m_cfg.shiftColors.size())
            return;
        m_cfg.shiftColors[i] = c;
        m_stops->setStops(m_cfg.shiftColors);
        emitChange();
    });
    connect(addStop, &QAbstractButton::clicked, this, [this]() {
        if (m_cfg.shiftColors.size() >= 8)
            return;
        m_cfg.shiftColors.append(m_stopPicker->color());
        m_stops->setStops(m_cfg.shiftColors);
        m_stops->setSelected(m_cfg.shiftColors.size() - 1);
        emitChange();
    });
    connect(removeStop, &QAbstractButton::clicked, this, [this]() {
        if (m_cfg.shiftColors.size() <= 2)
            return;
        const int i = m_stops->selected();
        m_cfg.shiftColors.removeAt(i);
        m_stops->setStops(m_cfg.shiftColors);
        m_stops->setSelected(qMin(i, int(m_cfg.shiftColors.size()) - 1));
        emitChange();
    });

    loadConfig(device->currentConfig().isEmpty() ? device->defaultConfig() : device->currentConfig());
}

void KeyboardIlluminationTab::loadConfig(const QJsonObject &data)
{
    m_loading = true;
    m_cfg = KeyboardConfig::fromJson(data);
    m_effect->setCurrentIndex(m_effect->findData(int(m_cfg.effect)));
    m_speed->setValue(m_cfg.speed);
    m_brightness->setValue(m_cfg.brightness);
    m_stops->setStops(m_cfg.shiftColors);
    if (m_stops->selected() < m_cfg.shiftColors.size())
        m_stopPicker->setColor(m_cfg.shiftColors.at(m_stops->selected()));
    syncPickerFromSelection();
    updateVisibility();
    updateZoneHint();
    m_loading = false;
}

void KeyboardIlluminationTab::syncPickerFromSelection()
{
    const QSet<int> zones = m_preview->selectedZones();
    if (zones.isEmpty())
        return;
    int first = 8;
    for (int z : zones)
        first = qMin(first, z);
    const bool was = m_loading;
    m_loading = true;
    m_picker->setColor(m_cfg.zoneColors[size_t(first)]);
    m_loading = was;
}

void KeyboardIlluminationTab::updateZoneHint()
{
    const QSet<int> zones = m_preview->selectedZones();
    if (zones.size() == KeyboardConfig::ZoneCount || zones.isEmpty()) {
        m_zoneHint->setText(tr("All 8 zones selected. Click a zone to edit it alone, Ctrl+click to select several."));
    } else {
        QList<int> sorted = zones.values();
        std::sort(sorted.begin(), sorted.end());
        QStringList names;
        for (int z : sorted)
            names << QString::number(z + 1);
        m_zoneHint->setText(tr("Selected zones: %1").arg(names.join(QStringLiteral(", "))));
    }
    m_selectAll->setVisible(zones.size() != KeyboardConfig::ZoneCount);
}

void KeyboardIlluminationTab::updateVisibility()
{
    const KeyboardConfig::Effect e = m_cfg.effect;
    m_speedCard->setVisible(e == KeyboardConfig::Breathe || e == KeyboardConfig::ColorShift);
    m_colorCard->setVisible(e == KeyboardConfig::Steady || e == KeyboardConfig::Breathe);
    m_stopsCard->setVisible(e == KeyboardConfig::ColorShift);
    m_preview->setInteractive(e == KeyboardConfig::Steady || e == KeyboardConfig::Breathe);
    m_zoneHint->setVisible(m_preview->isInteractive());
    m_selectAll->setVisible(m_preview->isInteractive() && m_preview->selectedZones().size() != KeyboardConfig::ZoneCount);

    QString desc;
    switch (e) {
    case KeyboardConfig::Steady: desc = tr("A fixed color per zone."); break;
    case KeyboardConfig::Breathe: desc = tr("Zone colors fade in and out."); break;
    case KeyboardConfig::ColorShift: desc = tr("Colors flow across the keyboard, zone by zone."); break;
    case KeyboardConfig::RainbowWave: desc = tr("The keyboard's built-in rainbow animation."); break;
    case KeyboardConfig::Off: desc = tr("Illumination turned off."); break;
    }
    m_effectDescription->setText(desc);
}

void KeyboardIlluminationTab::emitChange()
{
    if (m_loading)
        return;
    emit configEdited(m_cfg.toJson());
}

#include "KeyboardIlluminationTab.moc"
