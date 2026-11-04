#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

/**
 * Loads the monochrome SVG icons shipped in the resources and recolors them.
 *
 * Icons are stored as 24x24 stroke based SVGs using the placeholder color
 * `#000000`; the placeholder is replaced with the requested color before
 * rendering, so a single file serves every state (normal, hover, accent, ...).
 */
namespace Icons {

/// Renders `:/icons/<name>.svg` at `size` logical pixels in `color`.
QPixmap pixmap(const QString &name, const QColor &color, int size, qreal devicePixelRatio = 0.0);

/// Convenience wrapper producing a QIcon with a single pixmap.
QIcon icon(const QString &name, const QColor &color, int size = 20);

/// Returns true when an icon with that name exists in the resources.
bool exists(const QString &name);

/// The application icon (the OpenGG mark) rendered at `size` logical pixels
/// (multi colored, not recolored). A `devicePixelRatio` of 0 uses the
/// primary screen's ratio.
QPixmap appIcon(int size, qreal devicePixelRatio = 1.0);
QIcon appIconSet();

} // namespace Icons
