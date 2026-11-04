#pragma once

#include "models/MouseConfig.h"
#include "ui/pages/DevicePage.h"

#include <array>

class Rival3WirelessGen2;
class MousePreview;
class QToolButton;
class QWidget;

/// "Buttons" tab of the Rival 3 Wireless Gen 2: button remapping.
class MouseButtonsTab : public ConfigTab
{
    Q_OBJECT
public:
    explicit MouseButtonsTab(Rival3WirelessGen2 *device, QWidget *parent = nullptr);

    void loadConfig(const QJsonObject &data) override;

    static QString actionName(const Protocol::ButtonAction &a);
    static QString buttonName(int index);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void showMenuFor(int index);
    void refreshButtons();
    void emitChange();

    Rival3WirelessGen2 *m_device;
    MouseConfig m_cfg;
    bool m_loading = false;

    MousePreview *m_preview;
    std::array<QToolButton *, MouseConfig::ButtonCount> m_buttons{};
    std::array<QWidget *, MouseConfig::ButtonCount> m_rows{};
};
