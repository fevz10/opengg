#include "core/Icons.h"

#include <QFile>
#include <QGuiApplication>
#include <QHash>
#include <QPainter>
#include <QScreen>
#include <QSvgRenderer>

namespace Icons {

namespace {

QString iconPath(const QString &name)
{
    return QStringLiteral(":/icons/%1.svg").arg(name);
}

QByteArray loadSvg(const QString &name)
{
    static QHash<QString, QByteArray> cache;
    auto it = cache.find(name);
    if (it != cache.end())
        return it.value();
    QFile f(iconPath(name));
    QByteArray data;
    if (f.open(QIODevice::ReadOnly))
        data = f.readAll();
    cache.insert(name, data);
    return data;
}

} // namespace

bool exists(const QString &name)
{
    return QFile::exists(iconPath(name));
}

QPixmap pixmap(const QString &name, const QColor &color, int size, qreal devicePixelRatio)
{
    if (devicePixelRatio <= 0.0) {
        devicePixelRatio = 1.0;
        if (const QScreen *screen = QGuiApplication::primaryScreen())
            devicePixelRatio = screen->devicePixelRatio();
    }

    const QString key = QStringLiteral("%1|%2|%3|%4").arg(name, color.name(QColor::HexArgb)).arg(size).arg(devicePixelRatio);
    static QHash<QString, QPixmap> cache;
    auto it = cache.find(key);
    if (it != cache.end())
        return it.value();

    QByteArray svg = loadSvg(name);
    QPixmap pm(int(size * devicePixelRatio), int(size * devicePixelRatio));
    pm.fill(Qt::transparent);
    if (!svg.isEmpty()) {
        svg.replace(QByteArrayLiteral("#000000"), color.name(QColor::HexRgb).toLatin1());
        QSvgRenderer renderer(svg);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        if (color.alpha() < 255)
            p.setOpacity(color.alphaF());
        renderer.render(&p, QRectF(0, 0, pm.width(), pm.height()));
    }
    pm.setDevicePixelRatio(devicePixelRatio);
    cache.insert(key, pm);
    return pm;
}

QIcon icon(const QString &name, const QColor &color, int size)
{
    QIcon ic;
    ic.addPixmap(pixmap(name, color, size, 1.0));
    ic.addPixmap(pixmap(name, color, size, 2.0));
    return ic;
}

QPixmap appIcon(int size, qreal devicePixelRatio)
{
    if (devicePixelRatio <= 0.0) {
        devicePixelRatio = 1.0;
        if (const QScreen *screen = QGuiApplication::primaryScreen())
            devicePixelRatio = screen->devicePixelRatio();
    }

    const QString key = QStringLiteral("%1|%2").arg(size).arg(devicePixelRatio);
    static QHash<QString, QPixmap> cache;
    auto it = cache.find(key);
    if (it != cache.end())
        return it.value();

    QPixmap pm(int(size * devicePixelRatio), int(size * devicePixelRatio));
    pm.fill(Qt::transparent);
    QSvgRenderer renderer(QStringLiteral(":/app/opengg.svg"));
    if (renderer.isValid()) {
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        renderer.render(&p, QRectF(0, 0, pm.width(), pm.height()));
    }
    pm.setDevicePixelRatio(devicePixelRatio);
    cache.insert(key, pm);
    return pm;
}

QIcon appIconSet()
{
    QIcon ic;
    for (int s : {16, 22, 24, 32, 48, 64, 128, 256})
        ic.addPixmap(appIcon(s));
    return ic;
}

} // namespace Icons
