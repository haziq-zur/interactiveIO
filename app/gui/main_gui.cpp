#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application metadata
    app.setApplicationName("Interactive Instrument Communication Tool");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("InteractiveIO");
    
    // Set Fusion style for better dark mode support
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Apply dark mode color palette and stylesheet
    QString darkStyleSheet = R"(
        /* Main Window and Widgets */
        QMainWindow, QDialog {
            background-color: #1e1e1e;
            color: #e0e0e0;
        }
        
        QWidget {
            background-color: #1e1e1e;
            color: #e0e0e0;
            font-family: "Segoe UI", Arial, sans-serif;
            font-size: 10pt;
        }
        
        /* Group Boxes */
        QGroupBox {
            background-color: #252525;
            border: 2px solid #3a3a3a;
            border-radius: 8px;
            margin-top: 12px;
            padding: 15px;
            font-weight: bold;
            color: #40a0ff;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 5px 10px;
            background-color: #252525;
            color: #40a0ff;
        }
        
        /* Buttons */
        QPushButton {
            background-color: #2d2d30;
            color: #e0e0e0;
            border: 1px solid #3f3f46;
            border-radius: 6px;
            padding: 8px 16px;
            font-weight: 500;
            min-width: 80px;
        }
        
        QPushButton:hover {
            background-color: #3e3e42;
            border: 1px solid #007acc;
        }
        
        QPushButton:pressed {
            background-color: #007acc;
            border: 1px solid #005a9e;
        }
        
        QPushButton:disabled {
            background-color: #2d2d30;
            color: #656565;
            border: 1px solid #3f3f46;
        }
        
        QPushButton#connectButton {
            background-color: #0e639c;
            color: #ffffff;
            font-weight: bold;
        }
        
        QPushButton#connectButton:hover {
            background-color: #1177bb;
        }
        
        QPushButton#disconnectButton {
            background-color: #c42b1c;
            color: #ffffff;
            font-weight: bold;
        }
        
        QPushButton#disconnectButton:hover {
            background-color: #e04030;
        }
        
        QPushButton#sendButton {
            background-color: #0e639c;
            color: #ffffff;
            font-weight: bold;
        }
        
        QPushButton#sendButton:hover {
            background-color: #1177bb;
        }
        
        /* ComboBox */
        QComboBox {
            background-color: #2d2d30;
            color: #e0e0e0;
            border: 1px solid #3f3f46;
            border-radius: 5px;
            padding: 6px 10px;
            min-width: 120px;
        }
        
        QComboBox:hover {
            border: 1px solid #007acc;
        }
        
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 30px;
            border-left: 1px solid #3f3f46;
            border-top-right-radius: 5px;
            border-bottom-right-radius: 5px;
            background-color: #2d2d30;
        }
        
        QComboBox::drop-down:hover {
            background-color: #3e3e42;
        }
        
        QComboBox::down-arrow {
            image: none;
            width: 0;
            height: 0;
            border-left: 6px solid transparent;
            border-right: 6px solid transparent;
            border-top: 8px solid #e0e0e0;
            margin-top: 2px;
        }
        
        QComboBox::down-arrow:hover {
            border-top: 8px solid #40a0ff;
        }
        
        QComboBox QAbstractItemView {
            background-color: #2d2d30;
            color: #e0e0e0;
            border: 1px solid #007acc;
            selection-background-color: #0e639c;
            selection-color: #ffffff;
            outline: none;
        }
        
        /* Line Edit */
        QLineEdit {
            background-color: #2d2d30;
            color: #e0e0e0;
            border: 1px solid #3f3f46;
            border-radius: 5px;
            padding: 8px 12px;
            selection-background-color: #0e639c;
        }
        
        QLineEdit:focus {
            border: 1px solid #007acc;
            background-color: #1e1e1e;
        }
        
        QLineEdit:disabled {
            background-color: #2d2d30;
            color: #656565;
        }
        
        /* Text Edit */
        QTextEdit {
            background-color: #1e1e1e;
            color: #e0e0e0;
            border: 1px solid #3f3f46;
            border-radius: 5px;
            padding: 8px;
            selection-background-color: #0e639c;
            font-family: "Consolas", "Courier New", monospace;
            font-size: 9.5pt;
        }
        
        QTextEdit:focus {
            border: 1px solid #007acc;
        }
        
        /* Spin Box */
        QSpinBox {
            background-color: #2d2d30;
            color: #e0e0e0;
            border: 1px solid #3f3f46;
            border-radius: 5px;
            padding: 6px 10px;
        }
        
        QSpinBox:focus {
            border: 1px solid #007acc;
        }
        
        QSpinBox::up-button, QSpinBox::down-button {
            background-color: #3f3f46;
            border: none;
            border-radius: 3px;
            width: 16px;
        }
        
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background-color: #007acc;
        }
        
        /* Labels */
        QLabel {
            color: #e0e0e0;
            background-color: transparent;
        }
        
        /* Status Bar */
        QStatusBar {
            background-color: #007acc;
            color: #ffffff;
            border-top: 1px solid #005a9e;
            padding: 3px;
        }
        
        /* Menu Bar */
        QMenuBar {
            background-color: #2d2d30;
            color: #e0e0e0;
            border-bottom: 1px solid #3f3f46;
            padding: 3px;
        }
        
        QMenuBar::item {
            background-color: transparent;
            padding: 5px 10px;
            border-radius: 4px;
        }
        
        QMenuBar::item:selected {
            background-color: #3e3e42;
        }
        
        QMenuBar::item:pressed {
            background-color: #007acc;
        }
        
        /* Menu */
        QMenu {
            background-color: #2d2d30;
            color: #e0e0e0;
            border: 1px solid #3f3f46;
            padding: 5px;
        }
        
        QMenu::item {
            padding: 6px 30px 6px 20px;
            border-radius: 4px;
        }
        
        QMenu::item:selected {
            background-color: #0e639c;
        }
        
        /* Scrollbar */
        QScrollBar:vertical {
            background-color: #1e1e1e;
            width: 14px;
            border-radius: 7px;
        }
        
        QScrollBar::handle:vertical {
            background-color: #3f3f46;
            border-radius: 7px;
            min-height: 30px;
        }
        
        QScrollBar::handle:vertical:hover {
            background-color: #4f4f56;
        }
        
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        
        QScrollBar:horizontal {
            background-color: #1e1e1e;
            height: 14px;
            border-radius: 7px;
        }
        
        QScrollBar::handle:horizontal {
            background-color: #3f3f46;
            border-radius: 7px;
            min-width: 30px;
        }
        
        QScrollBar::handle:horizontal:hover {
            background-color: #4f4f56;
        }
        
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
        
        /* Dialog Buttons */
        QDialogButtonBox QPushButton {
            min-width: 80px;
        }
        
        /* Tool Tip */
        QToolTip {
            background-color: #2d2d30;
            color: #e0e0e0;
            border: 1px solid #007acc;
            padding: 5px;
            border-radius: 4px;
        }
    )";
    
    app.setStyleSheet(darkStyleSheet);
    
    // Create and show main window
    MainWindow window;
    window.show();
    
    return app.exec();
}
