#include "core/Theme.h"

#include <QApplication>
#include <QFontDatabase>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>
#include <QWidget>

namespace Theme {

QString hex(const QColor &c)
{
    return c.name(QColor::HexRgb);
}

QString rgba(const QColor &c)
{
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alphaF(), 0, 'f', 3);
}

static QString preferredFamily()
{
    static QString family;
    if (!family.isEmpty())
        return family;
    const QStringList candidates = {
        QStringLiteral("Inter"), QStringLiteral("SF Pro Display"), QStringLiteral("Segoe UI"),
        QStringLiteral("Noto Sans"), QStringLiteral("Ubuntu"), QStringLiteral("Cantarell"),
        QStringLiteral("DejaVu Sans"), QStringLiteral("Liberation Sans")};
    const QStringList available = QFontDatabase::families();
    for (const QString &c : candidates) {
        if (available.contains(c)) {
            family = c;
            return family;
        }
    }
    family = QApplication::font().family();
    return family;
}

QFont font(int pixelSize, QFont::Weight weight)
{
    QFont f(preferredFamily());
    f.setPixelSize(pixelSize);
    f.setWeight(weight);
    f.setStyleStrategy(QFont::PreferAntialias);
    return f;
}

QFont titleFont(int pixelSize)
{
    return font(pixelSize, QFont::DemiBold);
}

QFont sectionFont()
{
    QFont f = font(11, QFont::DemiBold);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
    f.setCapitalization(QFont::AllUppercase);
    return f;
}

QString styleSheet()
{
    const QString bg0 = hex(Bg0), bg1 = hex(Bg1), bg2 = hex(Bg2), bg3 = hex(Bg3), bg4 = hex(Bg4);
    const QString border = hex(Border), borderStrong = hex(BorderStrong);
    const QString text = hex(Text), textSec = hex(TextSecondary), textMuted = hex(TextMuted);
    const QString accent = hex(Accent), accentHover = hex(AccentHover), accentPressed = hex(AccentPressed);
    const QString danger = hex(Danger);

    QString qss = QStringLiteral(R"QSS(
QWidget { color: %TEXT%; }
QMainWindow, QDialog { background: %BG1%; }
QToolTip {
    background: %BG3%; color: %TEXT%; border: 1px solid %BORDERSTRONG%;
    padding: 6px 8px; border-radius: 6px; font-size: 12px;
}

/* ---- Scroll areas ---- */
QScrollArea { background: transparent; border: none; }
QScrollArea > QWidget > QWidget { background: transparent; }
QAbstractScrollArea::corner { background: transparent; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: %BG4%; min-height: 32px; border-radius: 3px; }
QScrollBar::handle:vertical:hover { background: #3f3f3f; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
QScrollBar::handle:horizontal { background: %BG4%; min-width: 32px; border-radius: 3px; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }

/* ---- Cards ---- */
QFrame#Card { background: %BG2%; border: 1px solid %BORDER%; border-radius: 10px; }
QFrame#CardFlat { background: %BG2%; border: none; border-radius: 10px; }
QFrame#Panel { background: %BG0%; border: none; }
QFrame#Divider { background: %BORDER%; max-height: 1px; min-height: 1px; border: none; }
QFrame#VDivider { background: %BORDER%; max-width: 1px; min-width: 1px; border: none; }

/* ---- Labels ---- */
QLabel[role="muted"] { color: %TEXTMUTED%; }
QLabel[role="secondary"] { color: %TEXTSEC%; }
QLabel[role="accent"] { color: %ACCENT%; }
QLabel[role="title"] { font-size: 22px; font-weight: 600; }
QLabel[role="subtitle"] { font-size: 15px; font-weight: 600; }
QLabel[role="badge"] {
    background: %BG4%; color: %TEXTSEC%; border-radius: 9px; padding: 2px 8px; font-size: 11px; font-weight: 600;
}
QLabel[role="badge-accent"] {
    background: rgba(255,82,0,0.18); color: %ACCENT%; border-radius: 9px; padding: 2px 8px; font-size: 11px; font-weight: 600;
}
QLabel[role="badge-success"] {
    background: rgba(63,214,142,0.16); color: #3fd68e; border-radius: 9px; padding: 2px 8px; font-size: 11px; font-weight: 600;
}

/* ---- Buttons ---- */
QPushButton {
    background: %BG3%; color: %TEXT%; border: 1px solid %BORDERSTRONG%;
    border-radius: 6px; padding: 7px 16px; font-weight: 600;
}
QPushButton:hover { background: %BG4%; }
QPushButton:pressed { background: %BG2%; }
QPushButton:disabled { color: %TEXTMUTED%; border-color: %BORDER%; background: %BG2%; }
QPushButton[variant="primary"] { background: %ACCENT%; border-color: %ACCENT%; color: white; }
QPushButton[variant="primary"]:hover { background: %ACCENTHOVER%; border-color: %ACCENTHOVER%; }
QPushButton[variant="primary"]:pressed { background: %ACCENTPRESSED%; border-color: %ACCENTPRESSED%; }
QPushButton[variant="primary"]:disabled { background: #5a2a14; border-color: #5a2a14; color: #b08a78; }
QPushButton[variant="ghost"] { background: transparent; border-color: transparent; color: %TEXTSEC%; }
QPushButton[variant="ghost"]:hover { background: %BG3%; color: %TEXT%; }
QPushButton[variant="danger"] { background: transparent; border-color: %DANGER%; color: %DANGER%; }
QPushButton[variant="danger"]:hover { background: rgba(255,74,61,0.12); }
QPushButton[variant="link"] { background: transparent; border: none; color: %ACCENT%; padding: 2px 4px; }
QPushButton[variant="link"]:hover { color: %ACCENTHOVER%; text-decoration: underline; }

QToolButton { background: transparent; border: none; border-radius: 6px; padding: 4px; color: %TEXTSEC%; }
QToolButton:hover { background: %BG3%; color: %TEXT%; }
QToolButton::menu-indicator { image: none; }

/* ---- Inputs ---- */
QLineEdit, QSpinBox, QDoubleSpinBox {
    background: %BG3%; border: 1px solid %BORDERSTRONG%; border-radius: 6px;
    padding: 6px 10px; color: %TEXT%; selection-background-color: %ACCENT%;
}
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus { border-color: %ACCENT%; }
QLineEdit:disabled, QSpinBox:disabled { color: %TEXTMUTED%; }
QSpinBox::up-button, QSpinBox::down-button { width: 0; border: none; }

QComboBox {
    background: %BG3%; border: 1px solid %BORDERSTRONG%; border-radius: 6px;
    padding: 6px 32px 6px 10px; color: %TEXT%; min-height: 20px;
}
QComboBox:hover { border-color: #4a4a4a; }
QComboBox:focus, QComboBox:on { border-color: %ACCENT%; }
QComboBox:disabled { color: %TEXTMUTED%; }
QComboBox::drop-down { border: none; width: 28px; subcontrol-origin: padding; subcontrol-position: center right; }
QComboBox::down-arrow { image: url(:/icons/static/chevron-down.svg); width: 14px; height: 14px; }
QComboBox QAbstractItemView {
    background: %BG3%; border: 1px solid %BORDERSTRONG%; border-radius: 6px;
    padding: 4px; outline: none; selection-background-color: %BG4%;
}
QComboBox QAbstractItemView::item { min-height: 28px; padding: 4px 8px; border-radius: 4px; color: %TEXT%; }
QComboBox QAbstractItemView::item:selected { background: %BG4%; color: %TEXT%; }
QComboBox QAbstractItemView::item:hover { background: %BG4%; }

/* ---- Menus ---- */
QMenu { background: %BG3%; border: 1px solid %BORDERSTRONG%; border-radius: 8px; padding: 6px; }
QMenu::item { padding: 7px 28px 7px 12px; border-radius: 5px; color: %TEXT%; }
QMenu::item:selected { background: %BG4%; }
QMenu::item:disabled { color: %TEXTMUTED%; }
QMenu::separator { height: 1px; background: %BORDERSTRONG%; margin: 6px 4px; }
QMenu::right-arrow { image: url(:/icons/static/chevron-right.svg); width: 12px; height: 12px; }
QMenu::indicator { width: 14px; height: 14px; }

/* ---- Check boxes / radios ---- */
QCheckBox { spacing: 8px; }
QCheckBox::indicator { width: 20px; height: 20px; image: url(:/icons/static/checkbox-off.svg); }
QCheckBox::indicator:checked { image: url(:/icons/static/checkbox-on.svg); }
QCheckBox::indicator:disabled { image: url(:/icons/static/checkbox-disabled.svg); }
QRadioButton { spacing: 8px; }
QRadioButton::indicator { width: 20px; height: 20px; image: url(:/icons/static/radio-off.svg); }
QRadioButton::indicator:checked { image: url(:/icons/static/radio-on.svg); }
QRadioButton::indicator:disabled { image: url(:/icons/static/radio-disabled.svg); }
QRadioButton:disabled { color: %TEXTMUTED%; }

/* ---- Sliders ---- */
QSlider::groove:horizontal { height: 4px; background: %BG4%; border-radius: 2px; }
QSlider::sub-page:horizontal { background: %ACCENT%; border-radius: 2px; }
QSlider::add-page:horizontal { background: %BG4%; border-radius: 2px; }
QSlider::handle:horizontal {
    width: 16px; height: 16px; margin: -6px 0; border-radius: 8px;
    background: white; border: 2px solid white;
}
QSlider::handle:horizontal:hover { background: #ffe6d9; border-color: #ffe6d9; }
QSlider::handle:horizontal:disabled { background: %BG4%; border-color: %BG4%; }
QSlider::sub-page:horizontal:disabled { background: #4a3a33; }

/* ---- Progress ---- */
QProgressBar { background: %BG4%; border: none; border-radius: 3px; height: 6px; text-align: center; color: transparent; }
QProgressBar::chunk { background: %ACCENT%; border-radius: 3px; }

/* ---- Lists ---- */
QListWidget, QListView { background: transparent; border: none; outline: none; }
QListWidget::item { padding: 8px; border-radius: 6px; }
QListWidget::item:selected { background: %BG4%; color: %TEXT%; }
QListWidget::item:hover { background: %BG3%; }

QStatusBar { background: %BG0%; color: %TEXTMUTED%; }
)QSS");

    qss.replace(QStringLiteral("%BG0%"), bg0);
    qss.replace(QStringLiteral("%BG1%"), bg1);
    qss.replace(QStringLiteral("%BG2%"), bg2);
    qss.replace(QStringLiteral("%BG3%"), bg3);
    qss.replace(QStringLiteral("%BG4%"), bg4);
    qss.replace(QStringLiteral("%BORDERSTRONG%"), borderStrong);
    qss.replace(QStringLiteral("%BORDER%"), border);
    qss.replace(QStringLiteral("%TEXTSEC%"), textSec);
    qss.replace(QStringLiteral("%TEXTMUTED%"), textMuted);
    qss.replace(QStringLiteral("%TEXT%"), text);
    qss.replace(QStringLiteral("%ACCENTHOVER%"), accentHover);
    qss.replace(QStringLiteral("%ACCENTPRESSED%"), accentPressed);
    qss.replace(QStringLiteral("%ACCENT%"), accent);
    qss.replace(QStringLiteral("%DANGER%"), danger);
    return qss;
}

void apply(QApplication &app)
{
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QPalette p;
    p.setColor(QPalette::Window, Bg1);
    p.setColor(QPalette::WindowText, Text);
    p.setColor(QPalette::Base, Bg3);
    p.setColor(QPalette::AlternateBase, Bg2);
    p.setColor(QPalette::ToolTipBase, Bg3);
    p.setColor(QPalette::ToolTipText, Text);
    p.setColor(QPalette::Text, Text);
    p.setColor(QPalette::Button, Bg3);
    p.setColor(QPalette::ButtonText, Text);
    p.setColor(QPalette::BrightText, Qt::white);
    p.setColor(QPalette::Link, Accent);
    p.setColor(QPalette::LinkVisited, AccentHover);
    p.setColor(QPalette::Highlight, Accent);
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::PlaceholderText, TextMuted);
    p.setColor(QPalette::Light, Bg4);
    p.setColor(QPalette::Midlight, Bg3);
    p.setColor(QPalette::Mid, Border);
    p.setColor(QPalette::Dark, Bg0);
    p.setColor(QPalette::Shadow, Qt::black);
    p.setColor(QPalette::Disabled, QPalette::Text, TextMuted);
    p.setColor(QPalette::Disabled, QPalette::WindowText, TextMuted);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, TextMuted);
    app.setPalette(p);

    app.setFont(font(13));
    app.setStyleSheet(styleSheet());
}

static void repolish(QWidget *w)
{
    if (!w)
        return;
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

void setRole(QWidget *w, const QString &role)
{
    if (!w)
        return;
    w->setProperty("role", role);
    repolish(w);
}

void setVariant(QWidget *w, const QString &variant)
{
    if (!w)
        return;
    w->setProperty("variant", variant);
    repolish(w);
}

} // namespace Theme
