// tray_icon.h - Windows system tray icon with right-click menu
#pragma once

#include <windows.h>
#include <shellapi.h>
#include <string>

#define WM_TRAYICON (WM_USER + 1)
#define IDM_OPEN_DASHBOARD 1001
#define IDM_EXIT 1002

class TrayIcon {
public:
    bool init(HWND hwnd, HICON icon, const std::string& tooltip);
    void show_menu(HWND hwnd);
    void remove();
    ~TrayIcon();
private:
    NOTIFYICONDATAA nid_ = {};
    bool added_ = false;
};
