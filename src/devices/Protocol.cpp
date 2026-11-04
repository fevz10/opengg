#include "devices/Protocol.h"

#include <QHash>

#include <algorithm>
#include <cstdlib>

namespace Protocol {

// ===========================================================================
// Apex 3 TKL
// ===========================================================================
namespace Apex3Tkl {

QByteArray setColors(const ZoneColors &colors, quint8 zoneMask)
{
    QByteArray pkt(PacketSize, '\0');
    pkt[0] = 0x00; // report id
    pkt[1] = char(CmdSetColors);
    pkt[2] = char(zoneMask);
    for (int i = 0; i < ZoneCount; ++i) {
        const int o = 3 + i * 3;
        pkt[o + 0] = char(qRed(colors[size_t(i)]));
        pkt[o + 1] = char(qGreen(colors[size_t(i)]));
        pkt[o + 2] = char(qBlue(colors[size_t(i)]));
    }
    return pkt;
}

QByteArray setRainbowWave()
{
    QByteArray pkt(PacketSize, '\0');
    pkt[1] = char(CmdRainbowWave);
    pkt[2] = char(0xFF);
    return pkt;
}

QByteArray setBrightness(int level)
{
    level = std::clamp(level, 0, BrightnessMax);
    QByteArray pkt(PacketSize, '\0');
    pkt[1] = char(CmdSetBrightness);
    pkt[2] = char(level);
    return pkt;
}

QByteArray queryBrightness()
{
    QByteArray pkt(PacketSize, '\0');
    pkt[1] = char(CmdQueryBrightness);
    return pkt;
}

int parseBrightness(const QByteArray &response)
{
    if (response.size() < 2 || quint8(response.at(0)) != CmdQueryBrightness)
        return -1;
    const int v = quint8(response.at(1));
    return v > BrightnessMax ? -1 : v;
}

int percentToLevel(int percent)
{
    percent = std::clamp(percent, 0, 100);
    return int((percent * BrightnessMax + 50) / 100);
}

int levelToPercent(int level)
{
    level = std::clamp(level, 0, BrightnessMax);
    return int((level * 100 + BrightnessMax / 2) / BrightnessMax);
}

} // namespace Apex3Tkl

// ===========================================================================
// Rival 3 Wireless Gen 2
// ===========================================================================
namespace Rival3WirelessGen2 {

namespace {

struct DpiEntry {
    int dpi;
    quint8 code;
};

// TrueMove Air sensor table (100 .. 18000 CPI).
const DpiEntry kDpiTable[] = {
    {100, 0x00}, {200, 0x02}, {300, 0x03}, {400, 0x04}, {500, 0x05}, {600, 0x06},
    {700, 0x07}, {800, 0x09}, {900, 0x0A}, {1000, 0x0B}, {1100, 0x0C}, {1200, 0x0D},
    {1300, 0x0E}, {1400, 0x10}, {1500, 0x11}, {1600, 0x12}, {1700, 0x13}, {1800, 0x14},
    {1900, 0x16}, {2000, 0x17}, {2100, 0x18}, {2200, 0x19}, {2300, 0x1A}, {2400, 0x1B},
    {2500, 0x1D}, {2600, 0x1E}, {2700, 0x1F}, {2800, 0x20}, {2900, 0x21}, {3000, 0x23},
    {3100, 0x25}, {3200, 0x26}, {3300, 0x27}, {3400, 0x28}, {3500, 0x29}, {3600, 0x2A},
    {3700, 0x2C}, {3800, 0x2D}, {3900, 0x2E}, {4000, 0x2F}, {4100, 0x30}, {4200, 0x32},
    {4300, 0x33}, {4400, 0x34}, {4500, 0x35}, {4600, 0x36}, {4700, 0x38}, {4800, 0x39},
    {4900, 0x3A}, {5000, 0x3B}, {5100, 0x3C}, {5200, 0x3E}, {5300, 0x3F}, {5400, 0x40},
    {5500, 0x41}, {5600, 0x42}, {5700, 0x44}, {5800, 0x45}, {5900, 0x46}, {6000, 0x47},
    {6100, 0x48}, {6200, 0x4A}, {6300, 0x4B}, {6400, 0x4C}, {6500, 0x4D}, {6600, 0x4E},
    {6700, 0x50}, {6800, 0x51}, {6900, 0x52}, {7000, 0x53}, {7100, 0x54}, {7200, 0x56},
    {7300, 0x57}, {7400, 0x58}, {7500, 0x59}, {7600, 0x5A}, {7700, 0x5C}, {7800, 0x5D},
    {7900, 0x5E}, {8000, 0x5F}, {8100, 0x60}, {8200, 0x62}, {8300, 0x63}, {8400, 0x64},
    {8500, 0x65}, {8600, 0x66}, {8700, 0x68}, {8800, 0x69}, {8900, 0x6A}, {9000, 0x6B},
    {9100, 0x6C}, {9200, 0x6E}, {9300, 0x6F}, {9400, 0x70}, {9500, 0x71}, {9600, 0x72},
    {9700, 0x74}, {9800, 0x75}, {9900, 0x76}, {10000, 0x77}, {10100, 0x78}, {10200, 0x7A},
    {10300, 0x7B}, {10400, 0x7C}, {10500, 0x7D}, {10600, 0x7E}, {10700, 0x80}, {10800, 0x81},
    {10900, 0x82}, {11000, 0x83}, {11100, 0x84}, {11200, 0x86}, {11300, 0x87}, {11400, 0x88},
    {11500, 0x89}, {11600, 0x8A}, {11700, 0x8C}, {11800, 0x8D}, {11900, 0x8E}, {12000, 0x8F},
    {12100, 0x90}, {12200, 0x92}, {12300, 0x93}, {12400, 0x94}, {12500, 0x95}, {12600, 0x96},
    {12700, 0x98}, {12800, 0x99}, {12900, 0x9A}, {13000, 0x9B}, {13100, 0x9C}, {13200, 0x9E},
    {13300, 0x9F}, {13400, 0xA0}, {13500, 0xA1}, {13600, 0xA2}, {13700, 0xA4}, {13800, 0xA5},
    {13900, 0xA6}, {14000, 0xA7}, {14100, 0xA8}, {14200, 0xAA}, {14300, 0xAB}, {14400, 0xAC},
    {14500, 0xAD}, {14600, 0xAE}, {14700, 0xB0}, {14800, 0xB1}, {14900, 0xB2}, {15000, 0xB3},
    {15100, 0xB4}, {15200, 0xB5}, {15300, 0xB6}, {15400, 0xB7}, {15500, 0xB8}, {15600, 0xB9},
    {15700, 0xBA}, {15800, 0xBB}, {15900, 0xBC}, {16000, 0xBD}, {16100, 0xBF}, {16200, 0xC0},
    {16300, 0xC2}, {16400, 0xC3}, {16500, 0xC4}, {16600, 0xC5}, {16700, 0xC6}, {16800, 0xC7},
    {16900, 0xC9}, {17000, 0xCA}, {17100, 0xCB}, {17200, 0xCC}, {17300, 0xCD}, {17400, 0xCF},
    {17500, 0xD0}, {17600, 0xD1}, {17700, 0xD2}, {17800, 0xD3}, {17900, 0xD5}, {18000, 0xD6},
};

constexpr int kDpiTableSize = int(sizeof(kDpiTable) / sizeof(kDpiTable[0]));

} // namespace

const QVector<int> &supportedDpis()
{
    static const QVector<int> list = [] {
        QVector<int> v;
        v.reserve(kDpiTableSize);
        for (const DpiEntry &e : kDpiTable)
            v.append(e.dpi);
        return v;
    }();
    return list;
}

int nearestDpi(int dpi)
{
    dpi = std::clamp(dpi, DpiMin, DpiMax);
    int best = kDpiTable[0].dpi;
    int bestDist = std::abs(dpi - best);
    for (const DpiEntry &e : kDpiTable) {
        const int d = std::abs(dpi - e.dpi);
        if (d < bestDist) {
            best = e.dpi;
            bestDist = d;
        }
    }
    return best;
}

quint8 dpiCode(int dpi)
{
    const int n = nearestDpi(dpi);
    for (const DpiEntry &e : kDpiTable)
        if (e.dpi == n)
            return e.code;
    return kDpiTable[0].code;
}

QByteArray setSensitivity(const QList<DpiPreset> &presets, int selectedIndex)
{
    const int count = std::clamp(int(presets.size()), 1, MaxPresets);
    selectedIndex = std::clamp(selectedIndex, 0, count - 1);

    // Layout: 00 2C count selected x1 x2 x3 x4 x5 y1 y2 y3 y4 y5   ("xxyy" mapping)
    QByteArray pkt;
    pkt.append(char(0x00));
    pkt.append(char(CmdSensitivity));
    pkt.append(char(count));
    pkt.append(char(selectedIndex + 1)); // first preset is 1
    for (int i = 0; i < MaxPresets; ++i)
        pkt.append(i < count ? char(dpiCode(presets.at(i).x)) : char(0));
    for (int i = 0; i < MaxPresets; ++i)
        pkt.append(i < count ? char(dpiCode(presets.at(i).y)) : char(0));
    return pkt;
}

QByteArray setPollingRate(int hz)
{
    quint8 code = 0x00;
    switch (hz) {
    case 125: code = 0x03; break;
    case 250: code = 0x02; break;
    case 500: code = 0x01; break;
    default: code = 0x00; break; // 1000 Hz
    }
    QByteArray pkt;
    pkt.append(char(0x00));
    pkt.append(char(CmdPollingRate));
    pkt.append(char(code));
    return pkt;
}

QByteArray setButtons(const std::array<ButtonAction, ButtonCount> &actions)
{
    QByteArray pkt(2 + ButtonCount * ButtonFieldLength, '\0');
    pkt[0] = 0x00;
    pkt[1] = char(CmdButtons);
    for (int i = 0; i < ButtonCount; ++i) {
        const int o = 2 + i * ButtonFieldLength;
        const ButtonAction &a = actions[size_t(i)];
        switch (a.type) {
        case ButtonAction::MouseButton:
            pkt[o] = char(std::clamp(a.code, 1, 6));
            break;
        case ButtonAction::Disabled:
            pkt[o] = char(ButtonDisable);
            break;
        case ButtonAction::CpiSwitch:
            pkt[o] = char(ButtonDpiSwitch);
            break;
        case ButtonAction::ScrollUp:
            pkt[o] = char(ButtonScrollUp);
            break;
        case ButtonAction::ScrollDown:
            pkt[o] = char(ButtonScrollDown);
            break;
        case ButtonAction::Keyboard:
            pkt[o] = char(ButtonKeyboard);
            pkt[o + 1] = char(a.code & 0xFF);
            break;
        case ButtonAction::Multimedia:
            pkt[o] = char(ButtonMultimedia);
            pkt[o + 1] = char(a.code & 0xFF);
            break;
        }
    }
    return pkt;
}

QByteArray queryBattery()
{
    QByteArray pkt;
    pkt.append(char(0x00));
    pkt.append(char(CmdBattery));
    pkt.append(char(0x01));
    return pkt;
}

BatteryStatus parseBattery(const QByteArray &response)
{
    BatteryStatus s;
    if (response.size() < 3)
        return s;
    const int level = quint8(response.at(0));
    if (level < 0 || level > 100)
        return s;
    s.valid = true;
    s.level = level;
    s.charging = quint8(response.at(2)) != 0;
    return s;
}

QByteArray save()
{
    QByteArray pkt;
    pkt.append(char(0x00));
    pkt.append(char(CmdSave));
    return pkt;
}

std::array<ButtonAction, ButtonCount> defaultButtons()
{
    return {{{ButtonAction::MouseButton, 1}, {ButtonAction::MouseButton, 2}, {ButtonAction::MouseButton, 3},
             {ButtonAction::MouseButton, 4}, {ButtonAction::MouseButton, 5}, {ButtonAction::CpiSwitch, 0}}};
}

} // namespace Rival3WirelessGen2

// ===========================================================================
// Key tables
// ===========================================================================

const QVector<KeyGroup> &keyboardKeyGroups()
{
    static const QVector<KeyGroup> groups = {
        {"Letters",
         {{"A", 0x04}, {"B", 0x05}, {"C", 0x06}, {"D", 0x07}, {"E", 0x08}, {"F", 0x09}, {"G", 0x0A},
          {"H", 0x0B}, {"I", 0x0C}, {"J", 0x0D}, {"K", 0x0E}, {"L", 0x0F}, {"M", 0x10}, {"N", 0x11},
          {"O", 0x12}, {"P", 0x13}, {"Q", 0x14}, {"R", 0x15}, {"S", 0x16}, {"T", 0x17}, {"U", 0x18},
          {"V", 0x19}, {"W", 0x1A}, {"X", 0x1B}, {"Y", 0x1C}, {"Z", 0x1D}}},
        {"Numbers",
         {{"1", 0x1E}, {"2", 0x1F}, {"3", 0x20}, {"4", 0x21}, {"5", 0x22}, {"6", 0x23}, {"7", 0x24},
          {"8", 0x25}, {"9", 0x26}, {"0", 0x27}}},
        {"Function keys",
         {{"F1", 0x3A}, {"F2", 0x3B}, {"F3", 0x3C}, {"F4", 0x3D}, {"F5", 0x3E}, {"F6", 0x3F},
          {"F7", 0x40}, {"F8", 0x41}, {"F9", 0x42}, {"F10", 0x43}, {"F11", 0x44}, {"F12", 0x45}}},
        {"Editing",
         {{"Enter", 0x28}, {"Escape", 0x29}, {"Backspace", 0x2A}, {"Tab", 0x2B}, {"Space", 0x2C},
          {"Delete", 0x4C}, {"Insert", 0x49}, {"Caps Lock", 0x39}, {"Print Screen", 0x46},
          {"Scroll Lock", 0x47}, {"Pause", 0x48}, {"Menu", 0x65}}},
        {"Navigation",
         {{"Home", 0x4A}, {"End", 0x4D}, {"Page Up", 0x4B}, {"Page Down", 0x4E},
          {"Arrow Up", 0x52}, {"Arrow Down", 0x51}, {"Arrow Left", 0x50}, {"Arrow Right", 0x4F}}},
        {"Symbols",
         {{"-", 0x2D}, {"=", 0x2E}, {"[", 0x2F}, {"]", 0x30}, {"\\", 0x31}, {";", 0x33}, {"'", 0x34},
          {"`", 0x35}, {",", 0x36}, {".", 0x37}, {"/", 0x38}}},
        {"Keypad",
         {{"Num Lock", 0x53}, {"Keypad /", 0x54}, {"Keypad *", 0x55}, {"Keypad -", 0x56}, {"Keypad +", 0x57},
          {"Keypad Enter", 0x58}, {"Keypad 1", 0x59}, {"Keypad 2", 0x5A}, {"Keypad 3", 0x5B}, {"Keypad 4", 0x5C},
          {"Keypad 5", 0x5D}, {"Keypad 6", 0x5E}, {"Keypad 7", 0x5F}, {"Keypad 8", 0x60}, {"Keypad 9", 0x61},
          {"Keypad 0", 0x62}, {"Keypad .", 0x63}}},
        {"Modifiers",
         {{"Left Ctrl", 0xE0}, {"Left Shift", 0xE1}, {"Left Alt", 0xE2}, {"Left Super", 0xE3},
          {"Right Ctrl", 0xE4}, {"Right Shift", 0xE5}, {"Right Alt", 0xE6}, {"Right Super", 0xE7}}},
    };
    return groups;
}

const QVector<NamedCode> &multimediaKeys()
{
    static const QVector<NamedCode> keys = {
        {"Play / Pause", 0xCD}, {"Next track", 0xB5}, {"Previous track", 0xB6},
        {"Mute", 0xE2}, {"Volume up", 0xE9}, {"Volume down", 0xEA},
    };
    return keys;
}

QString keyboardKeyName(int code)
{
    for (const KeyGroup &g : keyboardKeyGroups())
        for (const NamedCode &k : g.keys)
            if (k.code == code)
                return QString::fromLatin1(k.name);
    return QStringLiteral("0x%1").arg(code, 2, 16, QLatin1Char('0')).toUpper();
}

QString multimediaKeyName(int code)
{
    for (const NamedCode &k : multimediaKeys())
        if (k.code == code)
            return QString::fromLatin1(k.name);
    return QStringLiteral("0x%1").arg(code, 2, 16, QLatin1Char('0')).toUpper();
}

} // namespace Protocol
