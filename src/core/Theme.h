#pragma once

#include <QColor>
#include <QFont>
#include <QString>

class QApplication;
class QWidget;

/**
 * Central place for the GG inspired dark theme: colors, fonts and
 * the global Qt style sheet.
 */
namespace Theme {

// Backgrounds (darkest to lightest)
inline const QColor Bg0{0x0a, 0x0a, 0x0a};      // window / top bar
inline const QColor Bg1{0x11, 0x11, 0x11};      // page background
inline const QColor Bg2{0x1a, 0x1a, 0x1a};      // cards
inline const QColor Bg3{0x23, 0x23, 0x23};      // inputs, card hover
inline const QColor Bg4{0x2d, 0x2d, 0x2d};      // control hover / pressed

inline const QColor Border{0x27, 0x27, 0x27};
inline const QColor BorderStrong{0x3a, 0x3a, 0x3a};

inline const QColor Text{0xf3, 0xf3, 0xf3};
inline const QColor TextSecondary{0xa6, 0xa6, 0xa6};
inline const QColor TextMuted{0x6f, 0x6f, 0x6f};

inline const QColor Accent{0xff, 0x52, 0x00};
inline const QColor AccentHover{0xff, 0x6b, 0x24};
inline const QColor AccentPressed{0xdb, 0x46, 0x00};
inline const QColor AccentSoft{0xff, 0x52, 0x00, 0x33};

inline const QColor Success{0x3f, 0xd6, 0x8e};
inline const QColor Warning{0xff, 0xb0, 0x20};
inline const QColor Danger{0xff, 0x4a, 0x3d};
inline const QColor Info{0x4d, 0xa3, 0xff};

QString hex(const QColor &c);
QString rgba(const QColor &c);

QFont font(int pixelSize, QFont::Weight weight = QFont::Normal);
QFont titleFont(int pixelSize = 22);
QFont sectionFont();

/// Builds the application wide style sheet.
QString styleSheet();

/// Applies palette, style and style sheet to the application.
void apply(QApplication &app);

/// Sets the dynamic "role" property used by the style sheet and re-polishes.
void setRole(QWidget *w, const QString &role);
/// Sets the dynamic "variant" property (buttons) and re-polishes.
void setVariant(QWidget *w, const QString &variant);

} // namespace Theme
