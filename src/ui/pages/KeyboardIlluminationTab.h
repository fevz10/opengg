#pragma once

#include "models/KeyboardConfig.h"
#include "ui/pages/DevicePage.h"

class Apex3Tkl;
class KeyboardPreview;
class ColorPicker;
class ValueSlider;
class Card;
class QComboBox;
class QLabel;
class QPushButton;
class StopStrip;

/// "Illumination" tab of the Apex 3 TKL.
class KeyboardIlluminationTab : public ConfigTab
{
    Q_OBJECT
public:
    explicit KeyboardIlluminationTab(Apex3Tkl *device, QWidget *parent = nullptr);

    void loadConfig(const QJsonObject &data) override;

private:
    void emitChange();
    void updateVisibility();
    void updateZoneHint();
    void syncPickerFromSelection();

    Apex3Tkl *m_device;
    KeyboardConfig m_cfg;
    bool m_loading = false;

    KeyboardPreview *m_preview;
    QLabel *m_zoneHint;
    QPushButton *m_selectAll;
    QComboBox *m_effect;
    QLabel *m_effectDescription;
    Card *m_speedCard;
    ValueSlider *m_speed;
    ValueSlider *m_brightness;
    Card *m_colorCard;
    ColorPicker *m_picker;
    QPushButton *m_applyAll;
    Card *m_stopsCard;
    StopStrip *m_stops;
    ColorPicker *m_stopPicker;
};
