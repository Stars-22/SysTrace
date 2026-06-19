// tray_icon.cpp - Windows system tray icon implementation
#include "tray_icon.h"

bool TrayIcon::init(HWND hwnd, HICON icon, const std::string& tooltip) {
    nid_.cbSize = sizeof(NOTIFYICONDATAA);
    nid_.hWnd = hwnd;
    nid_.uID = 1;
    nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = WM_TRAYICON;
    nid_.hIcon = icon;
    if (tooltip.size() >= sizeof(nid_.szTip)) {
        memcpy(nid_.szTip, tooltip.c_str(), sizeof(nid_.szTip) - 1);
        nid_.szTip[sizeof(nid_.szTip) - 1] = '\0';
    } else {
        strcpy_s(nid_.szTip, tooltip.c_str());
    }

    added_ = Shell_NotifyIconA(NIM_ADD, &nid_);
    return added_;
}

void TrayIcon::show_menu(HWND hwnd) {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    AppendMenuA(hMenu, MF_STRING, IDM_OPEN_DASHBOARD, "Open Dashboard");
    AppendMenuA(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(hMenu, MF_STRING, IDM_EXIT, "Exit");

    POINT pt;
    GetCursorPos(&pt);

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, nullptr);
    PostMessage(hwnd, WM_NULL, 0, 0);

    DestroyMenu(hMenu);
}

void TrayIcon::remove() {
    if (added_) {
        Shell_NotifyIconA(NIM_DELETE, &nid_);
        added_ = false;
    }
}

TrayIcon::~TrayIcon() {
    remove();
}
