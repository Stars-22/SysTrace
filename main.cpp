// main.cpp - Entry point: parse CLI args, configure and start SysTraceServer
#include "http_server.h"
#include "tray_icon.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <shellapi.h>

static SysTraceServer* g_server = nullptr;
static TrayIcon* g_tray = nullptr;
static HWND g_hwnd = nullptr;
static bool g_should_exit = false;
static int g_actual_port = 26616;

static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT || ctrl_type == CTRL_CLOSE_EVENT) {
        fprintf(stderr, "\n[SysTrace] Shutting down...\n");
        g_should_exit = true;
        if (g_server) g_server->stop();
        if (g_hwnd) PostMessage(g_hwnd, WM_QUIT, 0, 0);
        return TRUE;
    }
    return FALSE;
}

static std::string exe_dir_path() {
    char buf[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (len == 0) return ".";
    std::string full(buf, len);
    size_t pos = full.find_last_of("\\/");
    return (pos != std::string::npos) ? full.substr(0, pos + 1) : ".\\";
}

static HICON load_icon_from_dir() {
    std::string icon_path = exe_dir_path() + "logo.ico";
    return (HICON)LoadImageA(nullptr, icon_path.c_str(), IMAGE_ICON,
                              0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
}

static HICON load_icon_from_exe() {
    char exe_path[MAX_PATH];
    GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
    HICON hIcon = ExtractIconA(nullptr, exe_path, 0);
    if ((uintptr_t)hIcon <= 1) return nullptr;
    return hIcon;
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_TRAYICON) {
        if (lp == WM_RBUTTONUP) {
            if (g_tray) g_tray->show_menu(hwnd);
            return 0;
        }
        if (lp == WM_LBUTTONUP || lp == WM_LBUTTONDBLCLK) {
            char url[64];
            snprintf(url, sizeof(url), "http://localhost:%d", g_actual_port);
            ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
            return 0;
        }
    }
    if (msg == WM_COMMAND) {
        if (LOWORD(wp) == IDM_OPEN_DASHBOARD) {
            char url[64];
            snprintf(url, sizeof(url), "http://localhost:%d", g_actual_port);
            ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
            return 0;
        }
        if (LOWORD(wp) == IDM_EXIT) {
            g_should_exit = true;
            if (g_server) g_server->stop();
            PostMessage(hwnd, WM_QUIT, 0, 0);
            return 0;
        }
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static HWND create_hidden_window() {
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "SysTrayHiddenWnd";
    RegisterClassExA(&wc);

    HWND hwnd = CreateWindowExA(0, "SysTrayHiddenWnd", "SysTrace",
                                 0, 0, 0, 0, 0, nullptr, nullptr,
                                 GetModuleHandleA(nullptr), nullptr);
    return hwnd;
}

struct Config {
    int port = 26616;
    int interval_ms = 1000;
    size_t heatmap_capacity = 86400;
    size_t snapshot_capacity = 7200;
    size_t max_processes = 500;
    std::string log_level = "warn";
    bool persist = true;
    std::string data_dir;
    int flush_interval_ms = 10000;
    bool foreground = false;
};

static void print_help() {
    printf("SysTrace v1.1.0-dev - Lightweight System Resource History Retrospector\n\n");
    printf("Usage: SysTrace [OPTIONS]\n\n");
    printf("Options:\n");
    printf("  --port <int>              HTTP server port (default: 26616)\n");
    printf("  --interval <int>          Collection interval in ms (default: 1000)\n");
    printf("  --heatmap-capacity <int>  Heatmap buffer capacity (default: 86400)\n");
    printf("  --snapshot-capacity <int> Snapshot buffer capacity (default: 7200)\n");
    printf("  --max-processes <int>     Max processes per snapshot (default: 500)\n");
    printf("  --log-level <string>      Log level: debug|info|warn|error (default: warn)\n");
    printf("  --no-persist              Disable disk persistence (memory-only mode)\n");
    printf("  --data-dir <path>         Data file directory (default: exe directory)\n");
    printf("  --flush-interval <int>    Disk flush interval in ms (default: 10000)\n");
    printf("  --foreground              Show console window (default: hidden to tray)\n");
    printf("  --help                    Show this help\n");
    printf("  --version                 Show version\n\n");
    printf("Open http://localhost:<port> in your browser to view the heatmap.\n");
}

// parse_args() - Parse command-line arguments into Config struct, validate ranges
static Config parse_args(int argc, char* argv[]) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            cfg.port = std::stoi(argv[++i]);
        } else if (arg == "--interval" && i + 1 < argc) {
            cfg.interval_ms = std::stoi(argv[++i]);
        } else if (arg == "--heatmap-capacity" && i + 1 < argc) {
            cfg.heatmap_capacity = std::stoul(argv[++i]);
        } else if (arg == "--snapshot-capacity" && i + 1 < argc) {
            cfg.snapshot_capacity = std::stoul(argv[++i]);
        } else if (arg == "--max-processes" && i + 1 < argc) {
            cfg.max_processes = std::stoul(argv[++i]);
        } else if (arg == "--log-level" && i + 1 < argc) {
            cfg.log_level = argv[++i];
        } else if (arg == "--no-persist") {
            cfg.persist = false;
        } else if (arg == "--data-dir" && i + 1 < argc) {
            cfg.data_dir = argv[++i];
        } else if (arg == "--flush-interval" && i + 1 < argc) {
            cfg.flush_interval_ms = std::stoi(argv[++i]);
        } else if (arg == "--foreground") {
            cfg.foreground = true;
        } else if (arg == "--help") {
            print_help();
            exit(0);
        } else if (arg == "--version") {
            printf("SysTrace v1.1.0-dev\n");
            exit(0);
        }
    }

    if (cfg.port < 1024 || cfg.port > 65535) {
        fprintf(stderr, "Error: --port must be between 1024 and 65535\n");
        exit(1);
    }
    if (cfg.interval_ms < 500 || cfg.interval_ms > 5000) {
        fprintf(stderr, "Error: --interval must be between 500 and 5000\n");
        exit(1);
    }
    if (cfg.snapshot_capacity < 600 || cfg.snapshot_capacity > 86400) {
        fprintf(stderr, "Error: --snapshot-capacity must be between 600 and 86400\n");
        exit(1);
    }

    return cfg;
}

int main(int argc, char* argv[]) {
    Config cfg = parse_args(argc, argv);

    ServerConfig srv_cfg;
    srv_cfg.port = cfg.port;
    srv_cfg.interval_ms = cfg.interval_ms;
    srv_cfg.heatmap_capacity = cfg.heatmap_capacity;
    srv_cfg.snapshot_capacity = cfg.snapshot_capacity;
    srv_cfg.max_processes = cfg.max_processes;
    srv_cfg.persist_enabled = cfg.persist;
    srv_cfg.data_dir = cfg.data_dir;
    srv_cfg.flush_interval_ms = cfg.flush_interval_ms;

    SysTraceServer server(srv_cfg);
    g_server = &server;
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

    if (cfg.foreground) {
        fprintf(stderr, "[SysTrace] Starting in foreground mode... Port=%d Interval=%dms\n", cfg.port, cfg.interval_ms);
    } else {
        HWND hConsole = GetConsoleWindow();
        if (hConsole) ShowWindow(hConsole, SW_HIDE);
        fprintf(stderr, "[SysTrace] Starting in background mode... Port=%d\n", cfg.port);
    }

    std::thread server_thread([&]() {
        if (!server.start()) {
            fprintf(stderr, "[SysTrace] Failed to start server\n");
            g_should_exit = true;
            if (g_hwnd) PostMessage(g_hwnd, WM_QUIT, 0, 0);
        }
    });

    server_thread.detach();

    while (server.actual_port() == 0 && !g_should_exit) {
        Sleep(100);
    }
    g_actual_port = server.actual_port() > 0 ? server.actual_port() : cfg.port;

    if (!g_should_exit) {
        if (!cfg.foreground) {
            g_hwnd = create_hidden_window();
            if (g_hwnd) {
                HICON hIcon = load_icon_from_dir();
                if (!hIcon) hIcon = load_icon_from_exe();
                if (!hIcon) hIcon = LoadIconA(nullptr, IDI_APPLICATION);

                TrayIcon tray;
                g_tray = &tray;
                tray.init(g_hwnd, hIcon, "SysTrace - System Monitor");

                fprintf(stderr, "[SysTrace] Running in system tray. Right-click tray icon to exit.\n");

                MSG msg;
                while (!g_should_exit && GetMessage(&msg, nullptr, 0, 0) > 0) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                tray.remove();
                g_tray = nullptr;
                DestroyWindow(g_hwnd);
                g_hwnd = nullptr;
            }
        } else {
            while (!g_should_exit) {
                Sleep(1000);
            }
        }
    }

    server.stop();
    if (g_hwnd) PostMessage(g_hwnd, WM_QUIT, 0, 0);

    g_server = nullptr;
    fprintf(stderr, "[SysTrace] Stopped.\n");
    return 0;
}
