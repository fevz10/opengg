#pragma once

#include <QAbstractButton>

class Device;
class KeyboardPreview;
class MousePreview;
class QLabel;

/// Clickable "gear" tile on the Engine page.
class DeviceCard : public QAbstractButton
{
    Q_OBJECT
public:
    explicit DeviceCard(Device *device, QWidget *parent = nullptr);

    Device *device() const { return m_device; }
    QSize sizeHint() const override { return QSize(300, 236); }
    QSize minimumSizeHint() const override { return sizeHint(); }

    /// Status text + color used both here and on the device page.
    static QString statusText(const Device *device);
    static QColor statusColor(const Device *device);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void refresh();

    Device *m_device;
    KeyboardPreview *m_keyboard = nullptr;
    MousePreview *m_mouse = nullptr;
    QLabel *m_name;
    QLabel *m_status;
    QLabel *m_battery;
    bool m_hover = false;
};
