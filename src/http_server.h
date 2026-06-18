// http_server.h - HTTP server wrapping cpp-httplib with API route handlers
#pragma once

#include "ring_buffer.h"
#include "collector.h"
#include <httplib.h>
#include <string>
#include <memory>

class Persistence;

struct ServerConfig {
    int port = 26616;
    int interval_ms = 1000;
    size_t heatmap_capacity = 86400;
    size_t snapshot_capacity = 7200;
    size_t max_processes = 500;
    bool persist_enabled = true;
    std::string data_dir;
    int flush_interval_ms = 10000;
};

class SysTraceServer {
public:
    explicit SysTraceServer(ServerConfig config = {});
    ~SysTraceServer();

    bool start();
    void stop();
    int actual_port() const { return actual_port_; }

private:
    void setup_routes();

    ServerConfig config_;
    std::unique_ptr<RingBuffer> buffer_;
    std::unique_ptr<Collector> collector_;
    std::unique_ptr<Persistence> persistence_;
    std::chrono::steady_clock::time_point start_time_;
    std::unique_ptr<httplib::Server> server_;
    int actual_port_ = 0;
};