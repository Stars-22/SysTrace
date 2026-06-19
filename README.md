# SysTrace

<p align="center"><img src="logo.svg" width="128" height="128" alt="SysTrace Logo"></p>

**轻量级 Windows 系统资源历史回溯器 | Lightweight Windows System Resource History Retrospector**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows-blue.svg)](https://github.com/Stars-22/SysTrace)
[![C++20](https://img.shields.io/badge/C++-20-blue.svg)](https://isocpp.org/std/the-standard)
[![Release](https://img.shields.io/badge/Release-v1.1.0-green.svg)](../../releases)

[English](#english) | [中文](#中文)

---

## English

### What is SysTrace?

SysTrace is a lightweight tool that silently records system resource usage (CPU, memory, disk IO, network, per-process stats) over the past 24 hours. Open `http://localhost:26616` in your browser to view a heatmap timeline, click any point to drill down into process-level details, and pinpoint the culprit behind occasional lag or overheating.

### Features

- **Silent background operation** — No GUI window, runs as a console app with minimal overhead
- **24-hour ring buffer** — In-memory storage with automatic expiry, constant memory footprint (~40MB)
- **Heatmap visualization** — Color-coded timeline (green → yellow → red) shows resource peaks at a glance
- **Drag-to-zoom & scroll zoom** — Zoom into any time range on the heatmap for detailed inspection
- **Keyboard navigation** — Arrow keys to step through data points, Home/End to jump to edges
- **Quick time presets** — One-click buttons for Now, 1m, 5m, 30m, 1h, 6h, plus datetime picker
- **Auto-refresh** — Configurable intervals (1s, 2s, 5s, 1m, 5m, 10m) with silent update mode (no jitter or placeholder flash)
- **Process drill-down** — Click any heatmap point to view all processes at that second, with spike highlighting
- **Aggregated data** — Automatically averages historical data when requested time precedes available records
- **Status spike badges** — CPU/Memory change indicators (▲ +12.3% / ▼ -5.2%) in process table
- **Disk I/O monitoring** — PDH-based disk read/write tracking (MB/s)
- **Network monitoring** — System-wide upload/download speed tracking via iphlpapi (MB/s)
- **Per-process IO** — Per-process IO Read/Write rates (total IO including disk + network + other)
- **6-metric heatmap** — CPU, Memory, Disk Read, Disk Write, Net Up, Net Down
- **Disk persistence** — Binary append-log storage, auto-recovers on restart (24h heatmap + 2h snapshots)
- **System tray** — Runs hidden in background with tray icon, right-click to open dashboard or exit
- **Graceful shutdown** — Ctrl+C cleanly stops the server
- **Single file exe** — No dependencies, no installer, just run it

### Quick Start

#### Option 1: Download from Releases

Go to the [Releases](../../releases) page and download the latest `SysTrace.exe`.

#### Option 2: Build from Source

**Prerequisites:**

- MinGW GCC 12+ (or MSVC 2019+)
- CMake 3.20+
- Windows 10+

```bash
git clone https://github.com/Stars-22/SysTrace.git
cd SysTrace

# Generate src/embed_index.h from web/index.html
python scripts/embed_index.py

# Build with CMake
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# Or build with g++ directly
g++ -std=c++20 -O2 -static -o SysTrace.exe main.cpp src/*.cpp -Isrc -Ithird_party/httplib -lpsapi -lkernel32 -lws2_32 -lpdh -liphlpapi
```

### Usage

```bash
# Run with default settings (port 26616, 1s interval)
SysTrace.exe

# Custom settings
SysTrace.exe --port 8080 --interval 2000

# Show all options
SysTrace.exe --help
```

Then open **http://localhost:26616** in your browser.

### Command Line Options

| Option | Default | Description |
|--------|---------|-------------|
| `--port` | 26616 | HTTP server port (1024–65535) |
| `--interval` | 1000 | Collection interval in milliseconds (500–5000) |
| `--heatmap-capacity` | 86400 | Heatmap buffer size (24h at 1s) |
| `--snapshot-capacity` | 7200 | Process snapshot buffer size (2h at 1s) |
| `--max-processes` | 500 | Max processes collected per snapshot |
| `--log-level` | warn | Log level: debug, info, warn, error |
| `--no-persist` | (off) | Disable disk persistence (memory-only mode) |
| `--data-dir` | exe dir | Directory for data files |
| `--flush-interval` | 10000 | Disk flush interval in milliseconds |
| `--foreground` | (off) | Show console window instead of hiding to system tray |

### API Endpoints

| Endpoint | Description |
|----------|-------------|
| `GET /` | Serves the built-in heatmap dashboard |
| `GET /logo.svg` | Serves the embedded SVG logo (cached 1h) |
| `GET /api/heatmap?from=&to=&step=` | Returns heatmap data (timestamp + CPU/mem/disk/net) |
| `GET /api/snapshot?time=` | Returns process-level details at a given timestamp (system: CPU/mem/disk/net; per-process: CPU/mem/IO Read/IO Write) |
| `GET /api/status` | Returns server status (uptime, buffer sizes, process count) |

### Architecture

```
┌─────────────────┐     ┌──────────────┐     ┌──────────────┐
│  Collector      │────>│  RingBuffer   │<────│  HTTP Server  │
│  (1s interval)  │     │  (deque+lock) │     │  (httplib)    │
│  Windows API    │     │  24h heatmap  │     │  :26616       │
│  + PDH disk IO  │     │  1h snapshots │     │               │
│  + iphlpapi net │     │               │     │               │
└─────────────────┘     └──────┬───────┘     └───────┬───────┘
                                │                      │
                          SystemSnapshot          JSON Response
                                │                      │
                         ┌──────▼──────────────────────▼──────┐
                         │         Browser (Canvas + JS)        │
                         │  Heatmap + Zoom + Presets + Refresh  │
                         │  Timeline ──> Process Table + Spikes │
                         └─────────────────────────────────────┘
```

### Tech Stack

| Layer | Technology | Purpose |
|-------|-----------|---------|
| Core | C++20 | High-performance system API calls |
| HTTP | cpp-httplib (header-only) | Zero-dependency HTTP server |
| System API | Windows kernel32, psapi, pdh, iphlpapi | CPU/memory/disk/network/process collection |
| Frontend | Vanilla HTML/JS + Canvas | Single-file embedded dashboard |
| Build | CMake 3.20+ | Cross-IDE build system |

### Contributing

We welcome contributions! Feel free to:

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

When modifying the frontend, edit `web/index.html`, `web/style.css`, or `web/app.js`, then run `python scripts/embed_index.py` to regenerate `src/embed_index.h`. Modifying `logo.svg` requires running `python scripts/embed_logo.py` and `python scripts/generate_icon.py`.

### License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.

### Acknowledgments

- [cpp-httplib](https://github.com/yhirose/cpp-httplib) — Yujiri Hirose's excellent header-only HTTP library

---

## 中文

### SysTrace 是什么？

SysTrace 是一个极轻量级的 Windows 系统资源监控工具。它以单文件 exe 常驻后台，默默记录过去 24 小时的系统与进程级资源占用（CPU、内存、磁盘 IO、网络）。用户只需在浏览器中打开 `http://localhost:26616`，即可查看资源热力图，并点击任意时间点查看该秒的进程快照，精准定位偶发卡顿的罪魁祸首。

### 功能特性

- **后台静默运行** — 无 GUI 窗口，控制台隐藏，极低系统开销（约 40MB 内存）
- **24 小时环形缓冲** — 内存中保存最近 24 小时数据，超时自动丢弃，内存恒定
- **热力图可视化** — 时间轴色阶映射（绿→黄→红），一眼看清高低峰
- **拖拽缩放 & 滚轮缩放** — 自由缩放至任意时间范围，鼠标滚轮以光标为中心缩放
- **键盘导航** — 方向键逐步切换数据点，Home/End 跳至首尾
- **快捷预设 & 时间选择器** — Now/1m/5m/30m/1h/6h 一键跳转，datetime 精确选择
- **自动刷新** — 可选 1s/2s/5s/1m/5m/10m 自动刷新，支持静默更新模式（无抖动、无占位闪烁）
- **进程快照下钻** — 点击热力图上的峰值点，查看该秒所有进程详情，突增高亮标记
- **智能聚合** — 查询早于记录范围的时间时，自动返回已有数据的平均值
- **状态突增标记** — 进程表中显示 CPU/内存变化徽章（▲ +12.3% / ▼ -5.2%）
- **磁盘 I/O 监控** — 基于 PDH API 的磁盘读写速率追踪
- **网络监控** — 基于 iphlpapi 的系统级上传/下载速率追踪
- **进程 IO 速率** — 每进程 IO Read/Write 速率（包含磁盘+网络+其他所有 IO 的总量）
- **6 指标热力图** — CPU、内存、磁盘读、磁盘写、上行网速、下行网速
- **磁盘持久化** — 二进制追加日志存储，重启自动恢复（24h 热力图 + 2h 进程快照）
- **系统托盘** — 后台静默运行于系统托盘，右键打开面板或退出
- **优雅退出** — Ctrl+C 干净关闭服务器
- **单文件部署** — 无依赖，无安装，双击即运行

### 快速开始

#### 方式一：从 Releases 下载

前往 [Releases](../../releases) 页面下载最新的 `SysTrace.exe`。

#### 方式二：自行编译

**前置要求：**

- MinGW GCC 12+（或 MSVC 2019+）
- CMake 3.20+
- Windows 10+

```bash
git clone https://github.com/Stars-22/SysTrace.git
cd SysTrace

# 从 web/index.html 生成 src/embed_index.h
python scripts/embed_index.py

# 使用 CMake 编译
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# 或使用 g++ 一行编译
g++ -std=c++20 -O2 -static -o SysTrace.exe main.cpp src/*.cpp -Isrc -Ithird_party/httplib -lpsapi -lkernel32 -lws2_32 -lpdh -liphlpapi
```

### 使用说明

```bash
# 默认配置运行（端口 26616，1 秒间隔）
SysTrace.exe

# 自定义设置
SysTrace.exe --port 8080 --interval 2000

# 查看所有选项
SysTrace.exe --help
```

运行后在浏览器中打开 **http://localhost:26616**。

### 命令行参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--port` | 26616 | HTTP 服务端口 (1024–65535) |
| `--interval` | 1000 | 采集间隔毫秒数 (500–5000) |
| `--heatmap-capacity` | 86400 | 热力图缓冲区大小（1 秒间隔下为 24 小时） |
| `--snapshot-capacity` | 7200 | 进程快照缓冲区大小（1 秒间隔下为 2 小时） |
| `--max-processes` | 500 | 每次采集的最大进程数 |
| `--log-level` | warn | 日志级别：debug, info, warn, error |
| `--no-persist` | (关闭) | 禁用磁盘持久化（纯内存模式） |
| `--data-dir` | exe 同目录 | 数据文件存储目录 |
| `--flush-interval` | 10000 | 磁盘 flush 间隔毫秒数 |
| `--foreground` | (关闭) | 显示控制台窗口，不隐藏到系统托盘 |

### API 接口

| 接口 | 说明 |
|------|------|
| `GET /` | 返回内置热力图面板页面 |
| `GET /logo.svg` | 返回嵌入式 SVG logo（缓存 1h） |
| `GET /api/heatmap?from=&to=&step=` | 返回热力图数据（时间戳 + CPU/内存/磁盘/网络） |
| `GET /api/snapshot?time=` | 返回指定时间戳的进程快照详情（系统级含磁盘+网络，进程级含 IO Read/IO Write） |
| `GET /api/status` | 返回服务状态（运行时间、缓冲区大小、进程数） |

### 项目架构

```
┌─────────────────┐     ┌──────────────┐     ┌──────────────┐
│  数据采集层      │────>│  环形缓冲区    │<────│  HTTP 服务层   │
│  (1 秒间隔)      │     │  (deque+锁)   │     │  (httplib)    │
│  Windows API     │     │  24h 热力图    │     │  :26616       │
│  + PDH 磁盘IO    │     │  1h 快照       │     │               │
│  + iphlpapi 网络  │     │               │     │               │
└─────────────────┘     └──────┬───────┘     └───────┬───────┘
                                │                      │
                          SystemSnapshot          JSON 响应
                                │                      │
                         ┌──────▼──────────────────────▼──────┐
                         │         浏览器 (Canvas + JS)        │
                         │  热力图 + 缩放 + 预设 + 自动刷新   │
                         │  时间轴 ──> 进程表格 + 突增标记     │
                         └─────────────────────────────────────┘
```

### 技术栈

| 层 | 技术 | 用途 |
|---|------|------|
| 核心 | C++20 | 高性能系统 API 调用 |
| HTTP | cpp-httplib (header-only) | 零依赖 HTTP 服务器 |
| 系统 API | Windows kernel32, psapi, pdh, iphlpapi | CPU/内存/磁盘/网络/进程采集 |
| 前端 | 原生 HTML/JS + Canvas | 单文件嵌入式仪表盘 |
| 构建 | CMake 3.20+ | 跨 IDE 构建系统 |

### 参与贡献

欢迎提交 Pull Request 参与开发！

1. Fork 本仓库
2. 创建功能分支 (`git checkout -b feature/amazing-feature`)
3. 提交修改
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 发起 Pull Request

修改前端时，请编辑 `web/index.html`、`web/style.css` 或 `web/app.js`，然后运行 `python scripts/embed_index.py` 重新生成 `src/embed_index.h`。修改 `logo.svg` 后需运行 `python scripts/embed_logo.py` 和 `python scripts/generate_icon.py`。

### 开源协议

本项目基于 MIT 协议开源，详见 [LICENSE](LICENSE) 文件。

### 致谢

- [cpp-httplib](https://github.com/yhirose/cpp-httplib) — Yuji Hirose 开发的优秀 header-only HTTP 库