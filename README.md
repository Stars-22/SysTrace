# SysTrace

**轻量级 Windows 系统资源历史回溯器 | Lightweight Windows System Resource History Retrospector**

[English](#english) | [中文](#中文)

---

## English

### What is SysTrace?

SysTrace is a lightweight tool that silently records system resource usage (CPU, memory, disk IO, per-process stats) over the past 24 hours. Open `http://localhost:26616` in your browser to view a heatmap timeline, click any point to drill down into process-level details, and pinpoint the culprit behind occasional lag or overheating.

### Features

- **Silent background operation** — No GUI window, runs as a console app with minimal overhead
- **24-hour ring buffer** — In-memory storage with automatic expiry, constant memory footprint
- **Heatmap visualization** — Color-coded timeline (green → yellow → red) shows resource peaks at a glance
- **Process drill-down** — Click any heatmap point to view all processes at that second, with spike highlighting
- **4 HTTP API endpoints** — `/`, `/api/heatmap`, `/api/snapshot`, `/api/status`
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
git clone https://github.com/your-username/SysTrace.git
cd SysTrace

# Generate src/embed_index.h from web/index.html
python scripts/embed_index.py

# Build with CMake
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# Or build with g++ directly
g++ -std=c++20 -O2 -static -o SysTrace.exe main.cpp src/*.cpp -Isrc -Ithird_party/httplib -lpsapi -lkernel32 -lws2_32
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
| `--snapshot-capacity` | 3600 | Process snapshot buffer size (1h at 1s) |
| `--max-processes` | 500 | Max processes collected per snapshot |
| `--log-level` | warn | Log level: debug, info, warn, error |

### API Endpoints

| Endpoint | Description |
|----------|-------------|
| `GET /` | Serves the built-in heatmap dashboard |
| `GET /api/heatmap?from=&to=&step=` | Returns heatmap data (timestamp + CPU/mem/disk) |
| `GET /api/snapshot?time=` | Returns process-level details at a given timestamp |
| `GET /api/status` | Returns server status (uptime, buffer sizes, process count) |

### Architecture

```
┌─────────────────┐     ┌──────────────┐     ┌──────────────┐
│  Collector      │────▶│  RingBuffer   │◀────│  HTTP Server  │
│  (1s interval)  │     │  (deque+lock) │     │  (httplib)    │
│  Windows API    │     │  24h heatmap  │     │  :26616       │
│                 │     │  1h snapshots │     │               │
└─────────────────┘     └──────┬───────┘     └───────┬───────┘
                               │                      │
                         SystemSnapshot          JSON Response
                               │                      │
                        ┌──────▼──────────────────────▼──────┐
                        │         Browser (Canvas + JS)        │
                        │   Heatmap Timeline → Process Table   │
                        └─────────────────────────────────────┘
```

### Tech Stack

| Layer | Technology | Purpose |
|-------|-----------|---------|
| Core | C++20 | High-performance system API calls |
| HTTP | cpp-httplib (header-only) | Zero-dependency HTTP server |
| System API | Windows kernel32, psapi, ntdll | CPU/memory/process collection |
| Frontend | Vanilla HTML/JS + Canvas | Single-file dashboard |
| Build | CMake 3.20+ | Cross-IDE build system |

### Contributing

We welcome contributions! Feel free to:

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

When modifying the frontend, edit `web/index.html` and run `python scripts/embed_index.py` to regenerate `src/embed_index.h`.

### License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.

### Acknowledgments

- [cpp-httplib](https://github.com/yhirose/cpp-httplib) — Yujiri Hirose's excellent header-only HTTP library

---

## 中文

### SysTrace 是什么？

SysTrace 是一个极轻量级的 Windows 系统资源监控工具。它以单文件 exe 常驻后台，默默记录过去 24 小时的系统与进程级资源占用。用户只需在浏览器中打开 `http://localhost:26616`，即可查看资源热力图，并点击任意时间点查看该秒的进程快照，精准定位偶发卡顿的罪魁祸首。

### 功能特性

- **后台静默运行** — 无 GUI 窗口，控制台隐藏，极低系统开销
- **24 小时环形缓冲** — 内存中保存最近 24 小时数据，超时自动丢弃，内存恒定
- **热力图可视化** — 时间轴色阶映射（绿→黄→红），一眼看清高低峰
- **进程快照下钻** — 点击热力图上的峰值点，查看该秒所有进程详情，高亮突增进程
- **4 个 HTTP 接口** — `/`、`/api/heatmap`、`/api/snapshot`、`/api/status`
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
git clone https://github.com/your-username/SysTrace.git
cd SysTrace

# 从 web/index.html 生成 src/embed_index.h
python scripts/embed_index.py

# 使用 CMake 编译
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# 或使用 g++ 一行编译
g++ -std=c++20 -O2 -static -o SysTrace.exe main.cpp src/*.cpp -Isrc -Ithird_party/httplib -lpsapi -lkernel32 -lws2_32
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
| `--snapshot-capacity` | 3600 | 进程快照缓冲区大小（1 秒间隔下为 1 小时） |
| `--max-processes` | 500 | 每次采集的最大进程数 |
| `--log-level` | warn | 日志级别：debug, info, warn, error |

### API 接口

| 接口 | 说明 |
|------|------|
| `GET /` | 返回内置热力图面板页面 |
| `GET /api/heatmap?from=&to=&step=` | 返回热力图数据（时间戳 + CPU/内存/磁盘） |
| `GET /api/snapshot?time=` | 返回指定时间戳的进程快照详情 |
| `GET /api/status` | 返回服务状态（运行时间、缓冲区大小、进程数） |

### 项目架构

```
┌─────────────────┐     ┌──────────────┐     ┌──────────────┐
│  数据采集层      │────▶│  环形缓冲区    │◀────│  HTTP 服务层   │
│  (1 秒间隔)      │     │  (deque+锁)   │     │  (httplib)    │
│  Windows API     │     │  24h 热力图    │     │  :26616       │
│                 │     │  1h 快照       │     │               │
└─────────────────┘     └──────┬───────┘     └───────┬───────┘
                               │                      │
                         SystemSnapshot          JSON 响应
                               │                      │
                        ┌──────▼──────────────────────▼──────┐
                        │         浏览器 (Canvas + JS)        │
                        │     热力图时间轴 → 进程详情表格      │
                        └─────────────────────────────────────┘
```

### 技术栈

| 层 | 技术 | 用途 |
|---|------|------|
| 核心 | C++20 | 高性能系统 API 调用 |
| HTTP | cpp-httplib (header-only) | 零依赖 HTTP 服务器 |
| 系统 API | Windows kernel32, psapi, ntdll | CPU/内存/进程采集 |
| 前端 | 原生 HTML/JS + Canvas | 单文件仪表盘 |
| 构建 | CMake 3.20+ | 跨 IDE 构建系统 |

### 参与贡献

欢迎提交 Pull Request 参与开发！

1. Fork 本仓库
2. 创建功能分支 (`git checkout -b feature/amazing-feature`)
3. 提交修改
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 发起 Pull Request

修改前端时，请编辑 `web/index.html`，然后运行 `python scripts/embed_index.py` 重新生成 `src/embed_index.h`。

### 开源协议

本项目基于 MIT 协议开源，详见 [LICENSE](LICENSE) 文件。

### 致谢

- [cpp-httplib](https://github.com/yhirose/cpp-httplib) — Yuji Hirose 开发的优秀 header-only HTTP 库