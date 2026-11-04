#pragma once

#include "models/KeyboardConfig.h"
#include "ui/pages/DevicePage.h"

class Apex3Tkl;
class ToggleSwitch;
class QSpinBox;
class QLabel;

/// "Settings" tab of the Apex 3 TKL (software side behaviour + device info).
class KeyboardSettingsTab : public ConfigTab
{
    Q_OBJECT
public:
    explicit KeyboardSettingsTab(Apex3Tkl *device, QWidget *parent = nullptr);

    void loadConfig(const QJsonObject &data) override;

private:
    void emitChange();

    Apex3Tkl *m_device;
    KeyboardConfig m_cfg;
    bool m_loading = false;

    ToggleSwitch *m_followBrightness;
    ToggleSwitch *m_restore;
    ToggleSwitch *m_idleEnabled;
    QSpinBox *m_idleMinutes;
    QLabel *m_idleSupport;
};
