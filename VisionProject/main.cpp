#include "VisionProject.h"
#include <QtWidgets/QApplication>
#define NOMINMAX              // 防止 windows.h 定义 min/max 宏，跟标准库打架
#include <Windows.h>
#include <dwmapi.h>      //新增：让标题栏也变深色
#pragma comment(lib,"dwmapi.lib")
#include <cstdio> 
//用于创建控制台调试
void createDebugConsole()
{
#ifdef _WIN32
    // 分配控制台
    AllocConsole();

    // 重定向标准输出到控制台
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);

    // 设置控制台标题
    SetConsoleTitle(L"调试控制台");

    // 设置编码
    SetConsoleOutputCP(CP_UTF8);
#endif
}

//新增：界面主题（深色工业风：黑灰底 + 青蓝强调色）
static const char* kThemeQss = R"QSS(
QWidget {
    font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI";
    font-size: 10pt;
    color: #c8d1da;
}
QMainWindow, QDialog { background: #16191d; }
/* 普通 QWidget 窗口也要上底色，否则像 ImageTool 这种独立窗口顶部会留一块白 */
QWidget#centralWidget { background: #16191d; }
ImageTool { background: #16191d; }
QTabWidget { background: #16191d; }
QLabel { color: #c8d1da; }
QToolTip {
    background: #232a31;
    color: #d6dee6;
    border: 1px solid #374151;
    padding: 4px 8px;
}

/* ---------- 工具栏 ---------- */
QToolBar {
    background: #1b2026;
    border: none;
    border-bottom: 1px solid #2f3841;
    padding: 6px 10px;
    spacing: 4px;
}
QToolBar::separator {
    background: #2f3841;
    width: 1px;
    margin: 6px 8px;
}
QToolButton {
    background: transparent;
    border: 1px solid transparent;
    border-radius: 4px;
    padding: 6px 12px;
    color: #c8d1da;
}
QToolButton:hover {
    background: #232c35;
    border-color: #2f81f7;
    color: #4db8ff;
}
QToolButton:pressed { background: #1c2733; }
QToolButton:disabled { color: #5b6672; }

/* ---------- 按钮 ---------- */
QPushButton {
    background: #232a31;
    border: 1px solid #374151;
    border-radius: 4px;
    padding: 6px 16px;
    color: #d6dee6;
    min-height: 16px;
}
QPushButton:hover {
    background: #26313b;
    border-color: #2f81f7;
    color: #4db8ff;
}
QPushButton:pressed { background: #1d262e; }
QPushButton:disabled {
    color: #5b6672;
    border-color: #2a323a;
    background: #1b2026;
}

/* ---------- 输入类 ---------- */
QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QPlainTextEdit, QTextEdit {
    background: #12161a;
    border: 1px solid #333d47;
    border-radius: 4px;
    padding: 4px 8px;
    color: #d6dee6;
    selection-background-color: #2f81f7;
    selection-color: #ffffff;
}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus,
QPlainTextEdit:focus, QTextEdit:focus {
    border-color: #2f81f7;
}
QLineEdit:disabled, QComboBox:disabled {
    color: #6b7683;
    background: #171c21;
}
QComboBox::drop-down { border: none; width: 22px; }
QComboBox QAbstractItemView {
    background: #1b2127;
    border: 1px solid #333d47;
    color: #d6dee6;
    outline: none;
    selection-background-color: #1f3a55;
    selection-color: #4db8ff;
}

/* ---------- 勾选框 ---------- */
QCheckBox, QRadioButton { spacing: 6px; }
QCheckBox::indicator, QRadioButton::indicator { width: 15px; height: 15px; }
QCheckBox::indicator:unchecked, QRadioButton::indicator:unchecked {
    border: 1px solid #4a5560;
    border-radius: 3px;
    background: #12161a;
}
QCheckBox::indicator:checked, QRadioButton::indicator:checked {
    border: 1px solid #2f81f7;
    border-radius: 3px;
    background: #2f81f7;
}

/* ---------- 标签页 ---------- */
QTabWidget::pane {
    border: 1px solid #2f3841;
    border-radius: 6px;
    background: #1b2026;
    top: -1px;
}
QTabBar::tab {
    background: transparent;
    color: #8b98a5;
    padding: 8px 20px;
    margin-right: 2px;
    border: none;
    border-bottom: 2px solid transparent;
}
QTabBar::tab:selected {
    color: #4db8ff;
    border-bottom: 2px solid #2f81f7;
    font-weight: 600;
}
QTabBar::tab:hover { color: #4db8ff; }

/* ---------- 列表 ---------- */
QListWidget {
    background: #12161a;
    border: 1px solid #2f3841;
    border-radius: 6px;
    outline: none;
}
QListWidget::item { border-radius: 4px; padding: 4px; }
QListWidget::item:selected {
    background: #1f3a55;
    color: #ffffff;
    border: 1px solid #2f81f7;
}

/* ---------- 滚动条 ---------- */
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical {
    background: #3a454f;
    border-radius: 5px;
    min-height: 30px;
}
QScrollBar::handle:vertical:hover { background: #4a5a68; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
QScrollBar::handle:horizontal {
    background: #3a454f;
    border-radius: 5px;
    min-width: 30px;
}
QScrollBar::handle:horizontal:hover { background: #4a5a68; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

/* ---------- 状态栏 ---------- */
QStatusBar {
    background: #1b2026;
    border-top: 1px solid #2f3841;
    color: #8b98a5;
}
QStatusBar::item { border: none; }

/* ---------- 消息框 ---------- */
QMessageBox { background: #1b2026; }
QMessageBox QLabel { color: #d6dee6; }
)QSS";
int main(int argc, char *argv[])
{
    createDebugConsole();
    QApplication app(argc, argv);

    //新增：统一用 Fusion 风格 + 自定义样式表，避免系统主题把界面画得花里胡哨
    QApplication::setStyle("Fusion");
    app.setStyleSheet(kThemeQss);

    VisionProject window;
    
    window.show();

    //新增：Windows 10 1809+ 支持深色标题栏，和主题配套
    BOOL bDark = TRUE;
    DwmSetWindowAttribute((HWND)window.winId(), 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &bDark, sizeof(bDark));

    return app.exec();
}
