#pragma once

#include <QList>
#include <QWidget>

class QLabel;

/// Transient notification bubble shown at the bottom right of the window.
class Toast : public QWidget
{
    Q_OBJECT
public:
    enum Type { Info, Success, Warning, Error };

    Toast(const QString &message, Type type, QWidget *parent);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Type m_type;
    QLabel *m_label;
};

/// Owns the toasts of a window and stacks them.
class ToastHost : public QObject
{
    Q_OBJECT
public:
    explicit ToastHost(QWidget *host);

    void show(const QString &message, Toast::Type type = Toast::Info, int durationMs = 3200);
    void relayout();

private:
    QWidget *m_host;
    QList<Toast *> m_toasts;
};
