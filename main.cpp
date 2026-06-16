// main.cpp - Entry point: parse CLI args, configure and start SysTraceServer
#include "http_server.h"
#include <cstdio>
#include <cstdlib>
#include <string>

struct Config {
    int port = 26616;
    int interval_ms = 1000;
    size_t heatmap_capacity = 86400;
    size_t snapshot_capacity = 3600;
    size_t max_processes = 500;
    std::string log_level = "warn";
};

static void print_help() {
    printf("SysTrace v1.0.0 - Lightweight System Resource History Retrospector\n\n");
    printf("Usage: SysTrace [OPTIONS]\n\n");
    printf("Options:\n");
    printf("  --port <int>              HTTP server port (default: 26616)\n");
    printf("  --interval <int>          Collection interval in ms (default: 1000)\n");
    printf("  --heatmap-capacity <int>  Heatmap buffer capacity (default: 86400)\n");
    printf("  --snapshot-capacity <int> Snapshot buffer capacity (default: 3600)\n");
    printf("  --max-processes <int>     Max processes per snapshot (default: 500)\n");
    printf("  --log-level <string>      Log level: debug|info|warn|error (default: warn)\n");
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
        } else if (arg == "--help") {
            print_help();
            exit(0);
        } else if (arg == "--version") {
            printf("SysTrace v1.0.0\n");
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

    SysTraceServer server(srv_cfg);

    fprintf(stderr, "[SysTrace] Starting... Port=%d Interval=%dms\n", cfg.port, cfg.interval_ms);

    if (!server.start()) {
        fprintf(stderr, "[SysTrace] Failed to start server\n");
        return 1;
    }

    return 0;
}