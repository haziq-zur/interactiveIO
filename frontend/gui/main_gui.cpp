#include "mainwindow.h"
#include <QApplication>
#include <QFont>
#include <QStyleFactory>

// =============================================================================
// Modern dark theme — a single, cohesive design system.
//
// Palette (slate + indigo accent, inspired by contemporary developer tools):
//   bg          #0d0f14   app background
//   surface     #161922   cards / panels
//   surface-2   #1c2029   inputs / controls
//   surface-3   #232834   hover
//   border      #272d3a   subtle separators
//   border-str  #38415280 focus / strong border
//   text        #e6e8ec   primary text
//   text-muted  #9aa1ad   secondary text
//   text-dim    #6b7280   tertiary / timestamps
//   accent      #6366f1   primary actions (indigo)
//   accent-hov  #7c7ff5
//   accent-prs  #4f46e5
//   success     #34d399   connected / responses
//   danger      #f87171   disconnect / errors
// =============================================================================
static const char *kModernStyleSheet = R"(
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
        background-color: #12141b;
        border: none;
        border-bottom: 1px solid #272d3a;
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
    QFrame#toolbarCard, QFrame#consoleCard {
        background-color: #161922;
        border: 1px solid #272d3a;
        border-radius: 14px;
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

    /* ---- Console output --------------------------------------------------- */
    QTextEdit#outputText {
        background-color: #0a0c10;
        color: #e6e8ec;
        border: 1px solid #1f2530;
        border-radius: 10px;
        padding: 12px;
        selection-background-color: #6366f1;
        selection-color: #ffffff;
        font-family: "Cascadia Code", "Cascadia Mono", "Consolas", "Courier New", monospace;
        font-size: 10pt;
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
        background-color: #161922;
        border: 1px solid #272d3a;
        border-radius: 12px;
        margin-top: 14px;
        padding: 14px;
        font-weight: 600;
        color: #e6e8ec;
    }
    QGroupBox::title {
        subcontrol-origin: margin;
        subcontrol-position: top left;
        left: 12px;
        padding: 2px 6px;
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

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Application metadata
    app.setApplicationName("Interactive Instrument Communication Tool");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("InteractiveIO");

    // Fusion is the most stylesheet-friendly built-in style.
    app.setStyle(QStyleFactory::create("Fusion"));

    // Base application font.
    QFont appFont("Segoe UI", 10);
    appFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(appFont);

    // Apply the modern theme globally (main window + dialogs inherit it).
    app.setStyleSheet(QString::fromUtf8(kModernStyleSheet));

    MainWindow window;
    window.show();

    return app.exec();
}
