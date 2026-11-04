#include "core/Autostart.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

namespace Autostart {

QString entryPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/autostart");
    return dir + QStringLiteral("/") + QStringLiteral(SSGG_APP_ID) + QStringLiteral(".desktop");
}

static QString legacyEntryPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/autostart");
    return dir + QStringLiteral("/steelseries-gg.desktop");
}

bool migrateLegacyEntry(bool startMinimized)
{
    if (!QFile::exists(legacyEntryPath()))
        return false;
    const bool ok = setEnabled(true, startMinimized);
    QFile::remove(legacyEntryPath());
    return ok;
}

bool isEnabled()
{
    QFile f(entryPath());
    if (!f.exists() || !f.open(QIODevice::ReadOnly))
        return false;
    const QString content = QString::fromUtf8(f.readAll());
    return !content.contains(QStringLiteral("Hidden=true"), Qt::CaseInsensitive);
}

bool setEnabled(bool enabled, bool startMinimized)
{
    const QString path = entryPath();
    if (!enabled) {
        if (QFile::exists(path))
            return QFile::remove(path);
        return true;
    }

    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    // Prefer the installed binary name so the entry keeps working after
    // upgrades; fall back to the running executable when not installed.
    QString exec = QStringLiteral(SSGG_APP_ID);
    if (QStandardPaths::findExecutable(exec).isEmpty())
        exec = QCoreApplication::applicationFilePath();
    if (startMinimized)
        exec += QStringLiteral(" --minimized");

    QTextStream out(&f);
    out << "[Desktop Entry]\n"
        << "Type=Application\n"
        << "Name=OpenGG\n"
        << "Comment=Restores device configuration at login\n"
        << "Exec=" << exec << "\n"
        << "Icon=" << SSGG_APP_ID << "\n"
        << "Terminal=false\n"
        << "X-GNOME-Autostart-enabled=true\n"
        << "X-GNOME-Autostart-Delay=3\n"
        << "X-KDE-autostart-after=panel\n";
    return true;
}

} // namespace Autostart
