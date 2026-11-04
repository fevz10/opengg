#include "devices/Protocol.h"
#include "models/KeyboardConfig.h"
#include "models/MouseConfig.h"

#include <QtTest>

using namespace Protocol;

class ProtocolTests : public QObject
{
    Q_OBJECT

private slots:
    // ---- Apex 3 TKL ------------------------------------------------------
    void apexColorsPacket()
    {
        Apex3Tkl::ZoneColors colors{};
        for (int i = 0; i < 8; ++i)
            colors[size_t(i)] = qRgb(i * 10, 100 + i, 255 - i);
        const QByteArray pkt = Apex3Tkl::setColors(colors);
        QCOMPARE(pkt.size(), Apex3Tkl::PacketSize);
        QCOMPARE(quint8(pkt[0]), quint8(0x00));
        QCOMPARE(quint8(pkt[1]), quint8(0x21));
        QCOMPARE(quint8(pkt[2]), quint8(0xFF));
        for (int i = 0; i < 8; ++i) {
            QCOMPARE(quint8(pkt[3 + i * 3]), quint8(i * 10));
            QCOMPARE(quint8(pkt[4 + i * 3]), quint8(100 + i));
            QCOMPARE(quint8(pkt[5 + i * 3]), quint8(255 - i));
        }
        // trailing bytes stay zero
        for (int i = 27; i < pkt.size(); ++i)
            QCOMPARE(quint8(pkt[i]), quint8(0));
    }

    void apexBrightnessAndRainbow()
    {
        QByteArray b = Apex3Tkl::setBrightness(0x10);
        QCOMPARE(b.size(), Apex3Tkl::PacketSize);
        QCOMPARE(quint8(b[1]), quint8(0x23));
        QCOMPARE(quint8(b[2]), quint8(0x10));
        b = Apex3Tkl::setBrightness(99);
        QCOMPARE(quint8(b[2]), quint8(0x10)); // clamped
        const QByteArray r = Apex3Tkl::setRainbowWave();
        QCOMPARE(quint8(r[1]), quint8(0x22));
        QCOMPARE(quint8(r[2]), quint8(0xFF));
        const QByteArray q = Apex3Tkl::queryBrightness();
        QCOMPARE(quint8(q[1]), quint8(0xA3));
    }

    void apexBrightnessParse()
    {
        QByteArray resp(2, '\0');
        resp[0] = char(0xA3);
        resp[1] = char(0x0B);
        QCOMPARE(Apex3Tkl::parseBrightness(resp), 11);
        resp[0] = char(0x90);
        QCOMPARE(Apex3Tkl::parseBrightness(resp), -1);
        QCOMPARE(Apex3Tkl::parseBrightness(QByteArray()), -1);
    }

    void apexPercentRoundTrip()
    {
        for (int level = 0; level <= Apex3Tkl::BrightnessMax; ++level)
            QCOMPARE(Apex3Tkl::percentToLevel(Apex3Tkl::levelToPercent(level)), level);
        QCOMPARE(Apex3Tkl::percentToLevel(100), 16);
        QCOMPARE(Apex3Tkl::percentToLevel(0), 0);
    }

    // ---- Rival 3 Wireless Gen 2 ----------------------------------------------
    void rivalDpiTable()
    {
        QCOMPARE(Rival3WirelessGen2::dpiCode(100), quint8(0x00));
        QCOMPARE(Rival3WirelessGen2::dpiCode(800), quint8(0x09));
        QCOMPARE(Rival3WirelessGen2::dpiCode(1600), quint8(0x12));
        QCOMPARE(Rival3WirelessGen2::dpiCode(18000), quint8(0xD6));
        QCOMPARE(Rival3WirelessGen2::nearestDpi(1649), 1600);
        QCOMPARE(Rival3WirelessGen2::nearestDpi(1651), 1700);
        QCOMPARE(Rival3WirelessGen2::nearestDpi(0), 100);
        QCOMPARE(Rival3WirelessGen2::nearestDpi(99999), 18000);
        QCOMPARE(Rival3WirelessGen2::supportedDpis().size(), 180);
    }

    void rivalSensitivityPacket()
    {
        // Same layout as rivalcfg: 00 2C count selected x1..x5 y1..y5
        const QByteArray pkt = Rival3WirelessGen2::setSensitivity({{800, 800}, {1600, 1600}}, 1);
        const QByteArray expected = QByteArray::fromHex("002C0202" "0912000000" "0912000000");
        QCOMPARE(pkt.toHex(), expected.toHex());

        const QByteArray xy = Rival3WirelessGen2::setSensitivity({{800, 1000}}, 0);
        QCOMPARE(xy.toHex(), QByteArray::fromHex("002C0101" "0900000000" "0B00000000").toHex());

        // five presets, last one selected
        const QByteArray five = Rival3WirelessGen2::setSensitivity({{400, 400}, {800, 800}, {1600, 1600}, {3200, 3200}, {6400, 6400}}, 4);
        QCOMPARE(five.size(), 14);
        QCOMPARE(quint8(five[2]), quint8(5));
        QCOMPARE(quint8(five[3]), quint8(5));
        QCOMPARE(quint8(five[8]), quint8(0x4C)); // 6400 x
        QCOMPARE(quint8(five[13]), quint8(0x4C)); // 6400 y
    }

    void rivalPollingPacket()
    {
        QCOMPARE(Rival3WirelessGen2::setPollingRate(1000).toHex(), QByteArray::fromHex("001700").toHex());
        QCOMPARE(Rival3WirelessGen2::setPollingRate(500).toHex(), QByteArray::fromHex("001701").toHex());
        QCOMPARE(Rival3WirelessGen2::setPollingRate(250).toHex(), QByteArray::fromHex("001702").toHex());
        QCOMPARE(Rival3WirelessGen2::setPollingRate(125).toHex(), QByteArray::fromHex("001703").toHex());
    }

    void rivalButtonsPacket()
    {
        auto buttons = Rival3WirelessGen2::defaultButtons();
        QByteArray pkt = Rival3WirelessGen2::setButtons(buttons);
        QCOMPARE(pkt.size(), 2 + 6 * 5);
        QCOMPARE(quint8(pkt[1]), quint8(0x19));
        QCOMPARE(quint8(pkt[2 + 0 * 5]), quint8(0x01));
        QCOMPARE(quint8(pkt[2 + 4 * 5]), quint8(0x05));
        QCOMPARE(quint8(pkt[2 + 5 * 5]), quint8(0x30)); // DPI switch

        buttons[1] = {ButtonAction::Keyboard, 0x04};   // 'A'
        buttons[2] = {ButtonAction::Multimedia, 0xCD}; // play/pause
        buttons[3] = {ButtonAction::Disabled, 0};
        buttons[4] = {ButtonAction::ScrollUp, 0};
        pkt = Rival3WirelessGen2::setButtons(buttons);
        QCOMPARE(quint8(pkt[2 + 1 * 5]), quint8(0x51));
        QCOMPARE(quint8(pkt[2 + 1 * 5 + 1]), quint8(0x04));
        QCOMPARE(quint8(pkt[2 + 2 * 5]), quint8(0x61));
        QCOMPARE(quint8(pkt[2 + 2 * 5 + 1]), quint8(0xCD));
        QCOMPARE(quint8(pkt[2 + 3 * 5]), quint8(0x00));
        QCOMPARE(quint8(pkt[2 + 4 * 5]), quint8(0x31));
    }

    void rivalBatteryAndSave()
    {
        QCOMPARE(Rival3WirelessGen2::queryBattery().toHex(), QByteArray::fromHex("00AA01").toHex());
        QCOMPARE(Rival3WirelessGen2::save().toHex(), QByteArray::fromHex("0009").toHex());
        QByteArray resp(3, '\0');
        resp[0] = char(73);
        resp[2] = char(1);
        const auto b = Rival3WirelessGen2::parseBattery(resp);
        QVERIFY(b.valid);
        QCOMPARE(b.level, 73);
        QVERIFY(b.charging);
        resp[0] = char(200);
        QVERIFY(!Rival3WirelessGen2::parseBattery(resp).valid);
        QVERIFY(!Rival3WirelessGen2::parseBattery(QByteArray()).valid);
    }

    // ---- Models ---------------------------------------------------------------
    void keyboardConfigJsonRoundTrip()
    {
        KeyboardConfig c;
        c.effect = KeyboardConfig::ColorShift;
        c.zoneColors[3] = QColor(1, 2, 3);
        c.speed = 77;
        c.brightness = 42;
        c.idleDimEnabled = true;
        c.idleMinutes = 7;
        const KeyboardConfig back = KeyboardConfig::fromJson(c.toJson());
        QVERIFY(back == c);
    }

    void mouseConfigJsonRoundTripAndNormalize()
    {
        MouseConfig c;
        c.presets[0].enabled = false;
        c.presets[1].enabled = false;
        c.presets[2].enabled = true;
        c.presets[2].x = 1234; // not a table value
        c.activePreset = 0;    // disabled -> must move to preset 2
        c.pollingRate = 333;   // invalid -> 1000
        c.buttons[0] = {ButtonAction::Keyboard, 0x1E};
        const MouseConfig back = MouseConfig::fromJson(c.toJson());
        QCOMPARE(back.activePreset, 2);
        QCOMPARE(back.presets[2].x, 1200);
        QCOMPARE(back.pollingRate, 1000);
        QVERIFY(back.buttons[0] == ButtonAction({ButtonAction::Keyboard, 0x1E}));
        int active = -1;
        const auto enabled = back.enabledPresets(&active);
        QCOMPARE(enabled.size(), 1);
        QCOMPARE(active, 0);
    }
};

QTEST_GUILESS_MAIN(ProtocolTests)
#include "ProtocolTests.moc"
