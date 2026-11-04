#pragma once

#include <QByteArray>
#include <QList>
#include <QRgb>
#include <QString>
#include <QVector>

#include <array>

/**
 * Pure functions building/parsing the HID reports understood by the supported
 * devices. Kept free of any I/O so they can be unit tested.
 *
 * All reports start with the HID report id byte (always 0x00 for these
 * devices) followed by the command byte and its payload.
 */
namespace Protocol {

constexpr quint16 SteelSeriesVendorId = 0x1038;

// ---------------------------------------------------------------------------
// Apex 3 TKL (8 zone RGB keyboard)
// ---------------------------------------------------------------------------
namespace Apex3Tkl {

constexpr quint16 ProductId = 0x1622;
constexpr int InterfaceNumber = 1;
constexpr quint16 UsagePage = 0xFFC0;
constexpr int PacketSize = 65; // report id + 64 bytes
constexpr int ZoneCount = 8;
constexpr int BrightnessMax = 0x10;
constexpr int CommandDelayMs = 0;

constexpr quint8 CmdSetColors = 0x21;
constexpr quint8 CmdRainbowWave = 0x22;
constexpr quint8 CmdSetBrightness = 0x23;
constexpr quint8 CmdQueryBrightness = 0xA3;

using ZoneColors = std::array<QRgb, ZoneCount>;

QByteArray setColors(const ZoneColors &colors, quint8 zoneMask = 0xFF);
QByteArray setRainbowWave();
QByteArray setBrightness(int level); // 0..16
QByteArray queryBrightness();
/// Returns brightness 0..16 or -1 when the response is not valid.
int parseBrightness(const QByteArray &response);

/// Maps a 0..100 percentage to the 0..16 hardware scale (and back).
int percentToLevel(int percent);
int levelToPercent(int level);

} // namespace Apex3Tkl

// ---------------------------------------------------------------------------
// Rival 3 Wireless Gen 2 (2.4 GHz receiver)
// ---------------------------------------------------------------------------

/// What a mouse button does. Shared between the protocol and the UI model.
struct ButtonAction {
    enum Type {
        MouseButton = 0, ///< code = 1..5 (left, right, middle, back, forward)
        Disabled,
        CpiSwitch,
        ScrollUp,
        ScrollDown,
        Keyboard,  ///< code = USB HID keyboard usage id
        Multimedia ///< code = consumer page usage id (low byte)
    };
    Type type = MouseButton;
    int code = 1;

    bool operator==(const ButtonAction &o) const { return type == o.type && code == o.code; }
    bool operator!=(const ButtonAction &o) const { return !(*this == o); }
};

namespace Rival3WirelessGen2 {

constexpr quint16 ProductId = 0x1872;
constexpr int InterfaceNumber = 3;
constexpr int CommandDelayMs = 50;
constexpr int ButtonCount = 6;
constexpr int MaxPresets = 5;
constexpr int DpiMin = 100;
constexpr int DpiMax = 18000;
constexpr int DpiStep = 100;

constexpr quint8 CmdSensitivity = 0x2C;
constexpr quint8 CmdPollingRate = 0x17;
constexpr quint8 CmdButtons = 0x19;
constexpr quint8 CmdBattery = 0xAA;
constexpr quint8 CmdSave = 0x09;

constexpr quint8 ButtonFieldLength = 5;
constexpr quint8 ButtonDisable = 0x00;
constexpr quint8 ButtonKeyboard = 0x51;
constexpr quint8 ButtonMultimedia = 0x61;
constexpr quint8 ButtonDpiSwitch = 0x30;
constexpr quint8 ButtonScrollUp = 0x31;
constexpr quint8 ButtonScrollDown = 0x32;

struct DpiPreset {
    int x = 800;
    int y = 800;
};

struct BatteryStatus {
    bool valid = false;
    int level = -1; // percent
    bool charging = false;
};

/// Rounds to the closest value the TrueMove Air sensor table knows.
int nearestDpi(int dpi);
/// Hardware code for a (valid) DPI value.
quint8 dpiCode(int dpi);
/// All supported DPI values in ascending order.
const QVector<int> &supportedDpis();

/// `presets` must contain 1..5 entries, `selectedIndex` is 0 based.
QByteArray setSensitivity(const QList<DpiPreset> &presets, int selectedIndex);
QByteArray setPollingRate(int hz); // 125, 250, 500, 1000
QByteArray setButtons(const std::array<ButtonAction, ButtonCount> &actions);
QByteArray queryBattery();
BatteryStatus parseBattery(const QByteArray &response);
QByteArray save();

std::array<ButtonAction, ButtonCount> defaultButtons();

} // namespace Rival3WirelessGen2

// ---------------------------------------------------------------------------
// Key tables used for button remapping
// ---------------------------------------------------------------------------
struct NamedCode {
    const char *name; ///< untranslated identifier (also the display label)
    int code;
};

/// USB HID keyboard usage ids grouped for menus.
struct KeyGroup {
    const char *group;
    QVector<NamedCode> keys;
};

const QVector<KeyGroup> &keyboardKeyGroups();
const QVector<NamedCode> &multimediaKeys();
QString keyboardKeyName(int code);
QString multimediaKeyName(int code);

} // namespace Protocol
