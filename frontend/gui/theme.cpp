#include "theme.h"

#include <QSettings>

namespace theme {

namespace {

// =============================================================================
// Dark theme — slate + indigo accent (the original design system).
//
// Palette:
//   bg #0d0f14 · surface #161922 · surface-2 #1c2029 · surface-3 #232834
//   border #272d3a · text #e6e8ec · text-muted #9aa1ad · text-dim #6b7280
//   accent #6366f1 · success #34d399 · danger #f87171
// =============================================================================
const char *kDarkStyleSheet = R"(
    /* ---- Base ------------------------------------------------------------ */
    QWidget {
        background-color: #0d0f14;
        color: #e6e8ec;
        font-family: "Segoe UI", "Inter", Arial, sans-serif;
        font-size: 10pt;
    }
    QMainWindow, QDialog {
        background-color: #0d0f14;
    }
    QToolTip {
        background-color: #1c2029;
        color: #e6e8ec;
        border: 1px solid #38415280;
        padding: 6px 8px;
        border-radius: 6px;
    }

    /* ---- Header ----------------------------------------------------------- */
    QFrame#headerBar {
        background-color: transparent;
        border: none;
    }
    QLabel#appMark {
        color: #818cf8;
        font-size: 22pt;
        padding-right: 6px;
    }
    QLabel#appTitle {
        color: #f2f4f7;
        font-size: 14pt;
        font-weight: 700;
    }
    QLabel#appSubtitle {
        color: #6b7280;
        font-size: 8.5pt;
        font-weight: 500;
    }

    /* ---- Status badge (states set inline from updateUIState) -------------- */
    QLabel#statusLabel {
        font-size: 9pt;
        font-weight: 700;
        padding: 6px 14px;
        border-radius: 12px;
        background-color: rgba(248, 113, 113, 0.12);
        color: #f87171;
    }

    /* ---- Cards ------------------------------------------------------------ */
    /* Sections are plain transparent strips (no card box) for a consistent,
       uncluttered layout. */
    QFrame#consoleCard {
        background-color: transparent;
        border: none;
    }
    /* The top toolbar is a plain transparent strip (no card box) so it stays
       visually consistent with the rest of the window chrome. */
    QFrame#toolbarCard {
        background-color: transparent;
        border: none;
    }

    /* ---- Section labels --------------------------------------------------- */
    QLabel#protocolLabel, QLabel#consoleTitle {
        color: #6b7280;
        font-size: 8pt;
        font-weight: 800;
        letter-spacing: 1px;
    }

    /* ---- Buttons ---------------------------------------------------------- */
    QPushButton {
        background-color: #1c2029;
        color: #e6e8ec;
        border: 1px solid #2f3645;
        border-radius: 9px;
        padding: 9px 18px;
        font-weight: 600;
        min-width: 76px;
    }
    QPushButton:hover {
        background-color: #232834;
        border: 1px solid #3a4152;
    }
    QPushButton:pressed {
        background-color: #161922;
    }
    QPushButton:disabled {
        background-color: #161922;
        color: #4b515e;
        border: 1px solid #232834;
    }

    /* Primary (accent) buttons */
    QPushButton#connectButton, QPushButton#sendButton {
        background-color: #6366f1;
        color: #ffffff;
        border: 1px solid #6366f1;
        font-weight: 700;
    }
    QPushButton#connectButton:hover, QPushButton#sendButton:hover {
        background-color: #7c7ff5;
        border: 1px solid #7c7ff5;
    }
    QPushButton#connectButton:pressed, QPushButton#sendButton:pressed {
        background-color: #4f46e5;
        border: 1px solid #4f46e5;
    }
    QPushButton#connectButton:disabled, QPushButton#sendButton:disabled {
        background-color: #20243a;
        color: #6b6fa0;
        border: 1px solid #20243a;
    }

    /* Destructive button */
    QPushButton#disconnectButton {
        background-color: transparent;
        color: #f87171;
        border: 1px solid #43303a;
    }
    QPushButton#disconnectButton:hover {
        background-color: rgba(248, 113, 113, 0.10);
        border: 1px solid #f87171;
    }
    QPushButton#disconnectButton:disabled {
        background-color: transparent;
        color: #4b515e;
        border: 1px solid #232834;
    }

    /* Subtle button (clear) */
    QPushButton#clearButton {
        background-color: transparent;
        color: #9aa1ad;
        border: 1px solid #2f3645;
        padding: 6px 14px;
        min-width: 60px;
    }
    QPushButton#clearButton:hover {
        background-color: #1c2029;
        color: #e6e8ec;
        border: 1px solid #3a4152;
    }

    /* ---- ComboBox --------------------------------------------------------- */
    QComboBox {
        background-color: #1c2029;
        color: #e6e8ec;
        border: 1px solid #2f3645;
        border-radius: 9px;
        padding: 8px 12px;
        min-width: 140px;
    }
    QComboBox:hover {
        border: 1px solid #3a4152;
    }
    QComboBox:focus {
        border: 1px solid #6366f1;
    }
    QComboBox::drop-down {
        subcontrol-origin: padding;
        subcontrol-position: center right;
        width: 26px;
        border: none;
    }
    QComboBox::down-arrow {
        image: none;
        width: 0;
        height: 0;
        border-left: 5px solid transparent;
        border-right: 5px solid transparent;
        border-top: 6px solid #9aa1ad;
        margin-right: 10px;
    }
    QComboBox::down-arrow:hover {
        border-top: 6px solid #e6e8ec;
    }
    QComboBox QAbstractItemView {
        background-color: #1c2029;
        color: #e6e8ec;
        border: 1px solid #2f3645;
        border-radius: 8px;
        padding: 4px;
        selection-background-color: #6366f1;
        selection-color: #ffffff;
        outline: none;
    }

    /* ---- Line edit -------------------------------------------------------- */
    QLineEdit {
        background-color: #0f1218;
        color: #e6e8ec;
        border: 1px solid #2f3645;
        border-radius: 10px;
        padding: 11px 14px;
        selection-background-color: #6366f1;
        selection-color: #ffffff;
        font-size: 10.5pt;
    }
    QLineEdit:focus {
        border: 1px solid #6366f1;
        background-color: #0d0f14;
    }
    QLineEdit:disabled {
        background-color: #12141b;
        color: #4b515e;
        border: 1px solid #1f2530;
    }

    /* ---- Console output (log table) -------------------------------------- */
    QTableWidget#outputTable {
        background-color: #0a0c10;
        alternate-background-color: #0d1016;
        color: #e6e8ec;
        border: 1px solid #1f2530;
        border-radius: 10px;
        padding: 4px;
        gridline-color: #1f2530;
        selection-background-color: #232a3a;
        selection-color: #ffffff;
        font-family: "Cascadia Code", "Cascadia Mono", "Consolas", "Courier New", monospace;
        font-size: 10pt;
    }
    QTableWidget#outputTable::item {
        padding: 4px 8px;
        border: none;
    }
    QHeaderView::section {
        background-color: #12141b;
        color: #6b7280;
        border: none;
        border-bottom: 1px solid #272d3a;
        padding: 8px 10px;
        font-size: 8pt;
        font-weight: 800;
        letter-spacing: 1px;
    }
    QTableCornerButton::section {
        background-color: #12141b;
        border: none;
    }

    /* ---- Spin box (used in the connection dialog) ------------------------- */
    QSpinBox {
        background-color: #1c2029;
        color: #e6e8ec;
        border: 1px solid #2f3645;
        border-radius: 9px;
        padding: 8px 10px;
    }
    QSpinBox:focus {
        border: 1px solid #6366f1;
    }
    QSpinBox::up-button, QSpinBox::down-button {
        background-color: #232834;
        border: none;
        width: 18px;
    }
    QSpinBox::up-button { border-top-right-radius: 8px; }
    QSpinBox::down-button { border-bottom-right-radius: 8px; }
    QSpinBox::up-button:hover, QSpinBox::down-button:hover {
        background-color: #6366f1;
    }

    /* ---- Group boxes (connection dialog) ---------------------------------- */
    QGroupBox {
        background-color: transparent;
        border: none;
        margin-top: 14px;
        padding: 6px 0 0 0;
        font-weight: 600;
        color: #e6e8ec;
    }
    QGroupBox::title {
        subcontrol-origin: margin;
        subcontrol-position: top left;
        left: 0px;
        padding: 2px 0;
        color: #818cf8;
    }

    /* ---- Menu bar / menus ------------------------------------------------- */
    QMenuBar {
        background-color: #12141b;
        color: #c8ccd4;
        border-bottom: 1px solid #272d3a;
    }
    QMenuBar::item {
        background: transparent;
        padding: 6px 12px;
        border-radius: 6px;
    }
    QMenuBar::item:selected {
        background-color: #232834;
    }
    QMenu {
        background-color: #161922;
        color: #e6e8ec;
        border: 1px solid #2f3645;
        border-radius: 8px;
        padding: 6px;
    }
    QMenu::item {
        padding: 7px 28px 7px 18px;
        border-radius: 6px;
    }
    QMenu::item:selected {
        background-color: #6366f1;
        color: #ffffff;
    }

    /* ---- Status bar ------------------------------------------------------- */
    QStatusBar {
        background-color: #12141b;
        color: #9aa1ad;
        border-top: 1px solid #272d3a;
    }
    QStatusBar::item { border: none; }

    /* ---- Scrollbars ------------------------------------------------------- */
    QScrollBar:vertical {
        background: transparent;
        width: 12px;
        margin: 4px 2px 4px 0;
    }
    QScrollBar::handle:vertical {
        background-color: #2f3645;
        border-radius: 5px;
        min-height: 32px;
    }
    QScrollBar::handle:vertical:hover {
        background-color: #3f4859;
    }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
    QScrollBar:horizontal {
        background: transparent;
        height: 12px;
        margin: 0 4px 2px 4px;
    }
    QScrollBar::handle:horizontal {
        background-color: #2f3645;
        border-radius: 5px;
        min-width: 32px;
    }
    QScrollBar::handle:horizontal:hover {
        background-color: #3f4859;
    }
    QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
    QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }

    /* ---- Dialog buttons --------------------------------------------------- */
    QDialogButtonBox QPushButton { min-width: 88px; }
)";

// =============================================================================
// Light theme — clean neutral grey with the same indigo accent.
//
// Palette:
//   bg #f4f5f7 · surface #ffffff · surface-2 #eceef2 · surface-3 #e1e4ea
//   border #d8dce3 · text #1c2029 · text-muted #5b6370 · text-dim #8b92a0
//   accent #6366f1 · success #059669 · danger #dc2626
// =============================================================================
const char *kLightStyleSheet = R"(
    /* ---- Base ------------------------------------------------------------ */
    QWidget {
        background-color: #f4f5f7;
        color: #1c2029;
        font-family: "Segoe UI", "Inter", Arial, sans-serif;
        font-size: 10pt;
    }
    QMainWindow, QDialog {
        background-color: #f4f5f7;
    }
    QToolTip {
        background-color: #ffffff;
        color: #1c2029;
        border: 1px solid #d8dce3;
        padding: 6px 8px;
        border-radius: 6px;
    }

    /* ---- Header ----------------------------------------------------------- */
    QFrame#headerBar {
        background-color: transparent;
        border: none;
    }
    QLabel#appMark {
        color: #6366f1;
        font-size: 22pt;
        padding-right: 6px;
    }
    QLabel#appTitle {
        color: #14171d;
        font-size: 14pt;
        font-weight: 700;
    }
    QLabel#appSubtitle {
        color: #8b92a0;
        font-size: 8.5pt;
        font-weight: 500;
    }

    /* ---- Status badge (states set inline from updateUIState) -------------- */
    QLabel#statusLabel {
        font-size: 9pt;
        font-weight: 700;
        padding: 6px 14px;
        border-radius: 12px;
        background-color: rgba(220, 38, 38, 0.10);
        color: #dc2626;
    }

    /* ---- Cards ------------------------------------------------------------ */
    /* Sections are plain transparent strips (no card box) for a consistent,
       uncluttered layout. */
    QFrame#consoleCard {
        background-color: transparent;
        border: none;
    }
    /* The top toolbar is a plain transparent strip (no card box) so it stays
       visually consistent with the rest of the window chrome. */
    QFrame#toolbarCard {
        background-color: transparent;
        border: none;
    }

    /* ---- Section labels --------------------------------------------------- */
    QLabel#protocolLabel, QLabel#consoleTitle {
        color: #8b92a0;
        font-size: 8pt;
        font-weight: 800;
        letter-spacing: 1px;
    }

    /* ---- Buttons ---------------------------------------------------------- */
    QPushButton {
        background-color: #eceef2;
        color: #1c2029;
        border: 1px solid #d8dce3;
        border-radius: 9px;
        padding: 9px 18px;
        font-weight: 600;
        min-width: 76px;
    }
    QPushButton:hover {
        background-color: #e1e4ea;
        border: 1px solid #c7cdd6;
    }
    QPushButton:pressed {
        background-color: #d8dce3;
    }
    QPushButton:disabled {
        background-color: #f0f1f4;
        color: #b3b9c2;
        border: 1px solid #e6e8ed;
    }

    /* Primary (accent) buttons */
    QPushButton#connectButton, QPushButton#sendButton {
        background-color: #6366f1;
        color: #ffffff;
        border: 1px solid #6366f1;
        font-weight: 700;
    }
    QPushButton#connectButton:hover, QPushButton#sendButton:hover {
        background-color: #7c7ff5;
        border: 1px solid #7c7ff5;
    }
    QPushButton#connectButton:pressed, QPushButton#sendButton:pressed {
        background-color: #4f46e5;
        border: 1px solid #4f46e5;
    }
    QPushButton#connectButton:disabled, QPushButton#sendButton:disabled {
        background-color: #c7c9f5;
        color: #ffffff;
        border: 1px solid #c7c9f5;
    }

    /* Destructive button */
    QPushButton#disconnectButton {
        background-color: transparent;
        color: #dc2626;
        border: 1px solid #f0c5c5;
    }
    QPushButton#disconnectButton:hover {
        background-color: rgba(220, 38, 38, 0.08);
        border: 1px solid #dc2626;
    }
    QPushButton#disconnectButton:disabled {
        background-color: transparent;
        color: #b3b9c2;
        border: 1px solid #e6e8ed;
    }

    /* Subtle button (clear) */
    QPushButton#clearButton {
        background-color: transparent;
        color: #5b6370;
        border: 1px solid #d8dce3;
        padding: 6px 14px;
        min-width: 60px;
    }
    QPushButton#clearButton:hover {
        background-color: #eceef2;
        color: #1c2029;
        border: 1px solid #c7cdd6;
    }

    /* ---- ComboBox --------------------------------------------------------- */
    QComboBox {
        background-color: #ffffff;
        color: #1c2029;
        border: 1px solid #d8dce3;
        border-radius: 9px;
        padding: 8px 12px;
        min-width: 140px;
    }
    QComboBox:hover {
        border: 1px solid #c7cdd6;
    }
    QComboBox:focus {
        border: 1px solid #6366f1;
    }
    QComboBox::drop-down {
        subcontrol-origin: padding;
        subcontrol-position: center right;
        width: 26px;
        border: none;
    }
    QComboBox::down-arrow {
        image: none;
        width: 0;
        height: 0;
        border-left: 5px solid transparent;
        border-right: 5px solid transparent;
        border-top: 6px solid #8b92a0;
        margin-right: 10px;
    }
    QComboBox::down-arrow:hover {
        border-top: 6px solid #1c2029;
    }
    QComboBox QAbstractItemView {
        background-color: #ffffff;
        color: #1c2029;
        border: 1px solid #d8dce3;
        border-radius: 8px;
        padding: 4px;
        selection-background-color: #6366f1;
        selection-color: #ffffff;
        outline: none;
    }

    /* ---- Line edit -------------------------------------------------------- */
    QLineEdit {
        background-color: #ffffff;
        color: #1c2029;
        border: 1px solid #d8dce3;
        border-radius: 10px;
        padding: 11px 14px;
        selection-background-color: #6366f1;
        selection-color: #ffffff;
        font-size: 10.5pt;
    }
    QLineEdit:focus {
        border: 1px solid #6366f1;
        background-color: #ffffff;
    }
    QLineEdit:disabled {
        background-color: #f0f1f4;
        color: #b3b9c2;
        border: 1px solid #e6e8ed;
    }

    /* ---- Console output (log table) -------------------------------------- */
    QTableWidget#outputTable {
        background-color: #ffffff;
        alternate-background-color: #f7f8fa;
        color: #1c2029;
        border: 1px solid #e1e4ea;
        border-radius: 10px;
        padding: 4px;
        gridline-color: #e1e4ea;
        selection-background-color: #e4e7fb;
        selection-color: #1c2029;
        font-family: "Cascadia Code", "Cascadia Mono", "Consolas", "Courier New", monospace;
        font-size: 10pt;
    }
    QTableWidget#outputTable::item {
        padding: 4px 8px;
        border: none;
    }
    QHeaderView::section {
        background-color: #f4f5f7;
        color: #8b92a0;
        border: none;
        border-bottom: 1px solid #e1e4ea;
        padding: 8px 10px;
        font-size: 8pt;
        font-weight: 800;
        letter-spacing: 1px;
    }
    QTableCornerButton::section {
        background-color: #f4f5f7;
        border: none;
    }

    /* ---- Spin box (used in the connection dialog) ------------------------- */
    QSpinBox {
        background-color: #ffffff;
        color: #1c2029;
        border: 1px solid #d8dce3;
        border-radius: 9px;
        padding: 8px 10px;
    }
    QSpinBox:focus {
        border: 1px solid #6366f1;
    }
    QSpinBox::up-button, QSpinBox::down-button {
        background-color: #eceef2;
        border: none;
        width: 18px;
    }
    QSpinBox::up-button { border-top-right-radius: 8px; }
    QSpinBox::down-button { border-bottom-right-radius: 8px; }
    QSpinBox::up-button:hover, QSpinBox::down-button:hover {
        background-color: #6366f1;
    }

    /* ---- Group boxes (connection dialog) ---------------------------------- */
    QGroupBox {
        background-color: transparent;
        border: none;
        margin-top: 14px;
        padding: 6px 0 0 0;
        font-weight: 600;
        color: #1c2029;
    }
    QGroupBox::title {
        subcontrol-origin: margin;
        subcontrol-position: top left;
        left: 0px;
        padding: 2px 0;
        color: #6366f1;
    }

    /* ---- Menu bar / menus ------------------------------------------------- */
    QMenuBar {
        background-color: #ffffff;
        color: #3a4150;
        border-bottom: 1px solid #e1e4ea;
    }
    QMenuBar::item {
        background: transparent;
        padding: 6px 12px;
        border-radius: 6px;
    }
    QMenuBar::item:selected {
        background-color: #eceef2;
    }
    QMenu {
        background-color: #ffffff;
        color: #1c2029;
        border: 1px solid #d8dce3;
        border-radius: 8px;
        padding: 6px;
    }
    QMenu::item {
        padding: 7px 28px 7px 18px;
        border-radius: 6px;
    }
    QMenu::item:selected {
        background-color: #6366f1;
        color: #ffffff;
    }

    /* ---- Status bar ------------------------------------------------------- */
    QStatusBar {
        background-color: #ffffff;
        color: #5b6370;
        border-top: 1px solid #e1e4ea;
    }
    QStatusBar::item { border: none; }

    /* ---- Scrollbars ------------------------------------------------------- */
    QScrollBar:vertical {
        background: transparent;
        width: 12px;
        margin: 4px 2px 4px 0;
    }
    QScrollBar::handle:vertical {
        background-color: #c7cdd6;
        border-radius: 5px;
        min-height: 32px;
    }
    QScrollBar::handle:vertical:hover {
        background-color: #b3b9c2;
    }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
    QScrollBar:horizontal {
        background: transparent;
        height: 12px;
        margin: 0 4px 2px 4px;
    }
    QScrollBar::handle:horizontal {
        background-color: #c7cdd6;
        border-radius: 5px;
        min-width: 32px;
    }
    QScrollBar::handle:horizontal:hover {
        background-color: #b3b9c2;
    }
    QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
    QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }

    /* ---- Dialog buttons --------------------------------------------------- */
    QDialogButtonBox QPushButton { min-width: 88px; }
)";

const char *kSettingsGroup = "appearance";
const char *kThemeKey = "theme";

} // namespace

QString styleSheet(Mode mode)
{
    return QString::fromUtf8(mode == Mode::Light ? kLightStyleSheet : kDarkStyleSheet);
}

QString consoleColor(Mode mode, Output role)
{
    // Light-mode colours are darkened variants chosen for good contrast on the
    // white console; dark-mode colours are the original pastel palette.
    const bool light = (mode == Mode::Light);
    switch (role) {
        case Output::Accent:    return light ? "#4f46e5" : "#818cf8";
        case Output::Muted:     return light ? "#6b7280" : "#6b7280";
        case Output::MutedText: return light ? "#4b5563" : "#9aa1ad";
        case Output::Success:   return light ? "#059669" : "#34d399";
        case Output::Error:     return light ? "#dc2626" : "#f87171";
        case Output::Info:      return light ? "#2563eb" : "#60a5fa";
        case Output::Warning:   return light ? "#b45309" : "#fbbf24";
        case Output::Timestamp: return light ? "#9aa3af" : "#5b6370";
    }
    return light ? "#1c2029" : "#e6e8ec";
}


QString messageBoxStyleSheet(Mode mode)
{
    if (mode == Mode::Light) {
        return QStringLiteral(
            "QMessageBox { background-color: #ffffff; }"
            "QMessageBox QLabel { color: #1c2029; background-color: transparent; }"
            "QPushButton { background-color: #6366f1; color: white; border: none; "
            "border-radius: 9px; padding: 9px 20px; min-width: 80px; font-weight: 600; }"
            "QPushButton:hover { background-color: #7c7ff5; }");
    }
    return QStringLiteral(
        "QMessageBox { background-color: #161922; }"
        "QMessageBox QLabel { color: #e6e8ec; background-color: transparent; }"
        "QPushButton { background-color: #6366f1; color: white; border: none; "
        "border-radius: 9px; padding: 9px 20px; min-width: 80px; font-weight: 600; }"
        "QPushButton:hover { background-color: #7c7ff5; }");
}

Mode loadSavedMode()
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    const QString value = settings.value(kThemeKey, "dark").toString();
    settings.endGroup();
    return value.compare("light", Qt::CaseInsensitive) == 0 ? Mode::Light : Mode::Dark;
}

void saveMode(Mode mode)
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kThemeKey, mode == Mode::Light ? "light" : "dark");
    settings.endGroup();
}

} // namespace theme
