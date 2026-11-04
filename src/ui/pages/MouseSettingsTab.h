#pragma once

#include "models/MouseConfig.h"
#include "ui/AppContext.h"
#include "ui/pages/DevicePage.h"

class Rival3WirelessGen2;
class SegmentedControl;
class ToggleSwitch;
class QLabel;
class QProgressBar;

/// "Settings" tab of the Rival 3 Wireless Gen 2.
class MouseSettingsTab : public ConfigTab
{
    Q_OBJECT
public:
    MouseSettingsTab(Rival3WirelessGen2 *device, const AppContext &ctx, QWidget *parent = nullptr);

    void loadConfig(const QJsonObject &data) override;

private:
    void refreshBattery();
    void emitChange();

    Rival3WirelessGen2 *m_device;
    AppContext m_ctx;
    MouseConfig m_cfg;
    bool m_loading = false;

    SegmentedControl *m_polling;
    QLabel *m_batteryValue;
    QLabel *m_batteryState;
    QProgressBar *m_batteryBar;
    ToggleSwitch *m_lowBattery;
};
