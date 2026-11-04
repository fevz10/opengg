#pragma once

#include "models/MouseConfig.h"
#include "ui/pages/DevicePage.h"

#include <QTimer>
#include <array>

class Rival3WirelessGen2;
class ToggleSwitch;
class ValueSlider;
class QRadioButton;
class QLabel;
class QWidget;
class QButtonGroup;

/// "Sensitivity (CPI)" tab of the Rival 3 Wireless Gen 2.
class MouseSensitivityTab : public ConfigTab
{
    Q_OBJECT
public:
    explicit MouseSensitivityTab(Rival3WirelessGen2 *device, QWidget *parent = nullptr);

    void loadConfig(const QJsonObject &data) override;

private:
    struct Row {
        QWidget *widget = nullptr;
        ToggleSwitch *enabled = nullptr;
        QRadioButton *active = nullptr;
        ValueSlider *x = nullptr;
        ValueSlider *y = nullptr;
        QWidget *yRow = nullptr;
        QLabel *xLabel = nullptr;
    };

    void refreshRows();
    void updateSummary();
    void scheduleEmit();
    void emitChange();

    Rival3WirelessGen2 *m_device;
    MouseConfig m_cfg;
    bool m_loading = false;

    ToggleSwitch *m_separateXY;
    QButtonGroup *m_activeGroup;
    std::array<Row, MouseConfig::PresetCount> m_rows{};
    QLabel *m_summary;
    QTimer m_debounce;
};
