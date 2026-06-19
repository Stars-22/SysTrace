// http_server.cpp - HTTP server route handlers and lifecycle management
#include "http_server.h"
#include "persistence.h"
#include "json_serializer.h"
#include "embed_index.h"
#include "embed_logo.h"
#include <cstdio>
#include <cstdlib>

SysTraceServer::SysTraceServer(ServerConfig config)
    : config_(std::move(config)),
      buffer_(std::make_unique<RingBuffer>(config_.heatmap_capacity, config_.snapshot_capacity)),
      start_time_(std::chrono::steady_clock::now()),
      server_(std::make_unique<httplib::Server>()) {
    CollectorConfig coll_cfg;
    coll_cfg.interval_ms = config_.interval_ms;
    coll_cfg.max_processes = config_.max_processes;

    if (config_.persist_enabled) {
        persistence_ = std::make_unique<Persistence>(
            config_.data_dir, config_.interval_ms,
            config_.heatmap_capacity, config_.snapshot_capacity);
    }

    collector_ = std::make_unique<Collector>(*buffer_, coll_cfg, persistence_.get());
}

SysTraceServer::~SysTraceServer() {
    stop();
}

bool SysTraceServer::start() {
    setup_routes();

    server_->set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });

    if (persistence_) {
        if (persistence_->open_files()) {
            fprintf(stderr, "[SysTrace] Recovering data from disk...\n");
            persistence_->recover(*buffer_);
            persistence_->trim();
        } else {
            fprintf(stderr, "[SysTrace] Warning: failed to open persistence files, continuing without persistence\n");
            persistence_.reset();
        }
    }

    collector_->start();

    if (!server_->bind_to_port("0.0.0.0", config_.port)) {
        fprintf(stderr, "[SysTrace] Failed to bind port %d\n", config_.port);

        if (config_.port == 26616) {
            for (int p = config_.port + 1; p < config_.port + 10; ++p) {
                if (server_->bind_to_port("0.0.0.0", p)) {
                    actual_port_ = p;
                    fprintf(stderr, "[SysTrace] Bound to alternative port %d\n", p);
                    break;
                }
            }
            if (actual_port_ == 0) {
                fprintf(stderr, "[SysTrace] No available port found\n");
                return false;
            }
        } else {
            return false;
        }
    } else {
        actual_port_ = config_.port;
    }

    fprintf(stderr, "[SysTrace] Server starting on http://localhost:%d\n", actual_port_);
    server_->listen_after_bind();
    return true;
}

void SysTraceServer::stop() {
    if (collector_) collector_->stop();
    if (server_) server_->stop();
}

// setup_routes() - Register all HTTP GET handlers: /, /api/heatmap, /api/snapshot, /api/status
void SysTraceServer::setup_routes() {
    // GET / - serve index.html
    server_->Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(INDEX_HTML, "text/html; charset=utf-8");
    });

    // GET /logo.svg - serve the embedded SVG logo
    server_->Get("/logo.svg", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Content-Type", "image/svg+xml; charset=utf-8");
        res.set_header("Cache-Control", "public, max-age=3600");
        res.set_content(LOGO_SVG, "image/svg+xml; charset=utf-8");
    });

    // GET /api/heatmap
    server_->Get("/api/heatmap", [this](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Content-Type", "application/json; charset=utf-8");

        time_t from = 0, to = 0;
        int step = 1;

        if (req.has_param("from")) {
            try { from = std::stoll(req.get_param_value("from")); }
            catch (...) {
                res.status = 400;
                res.set_content(json::serialize_error("invalid_param", "Parameter 'from' must be an integer timestamp"), "application/json");
                return;
            }
        }
        if (req.has_param("to")) {
            try { to = std::stoll(req.get_param_value("to")); }
            catch (...) {
                res.status = 400;
                res.set_content(json::serialize_error("invalid_param", "Parameter 'to' must be an integer timestamp"), "application/json");
                return;
            }
        }
        if (req.has_param("step")) {
            try { step = std::stoi(req.get_param_value("step")); }
            catch (...) {
                res.status = 400;
                res.set_content(json::serialize_error("invalid_param", "Parameter 'step' must be an integer"), "application/json");
                return;
            }
        }

        if (from > 0 && to > 0 && from > to) {
            res.status = 400;
            res.set_content(json::serialize_error("invalid_range", "Parameter 'from' must be less than 'to'"), "application/json");
            return;
        }

        time_t oldest = buffer_->oldest_heatmap_time();
        time_t newest = buffer_->newest_heatmap_time();

        if (oldest == 0) {
            res.set_content(json::serialize_heatmap({}, 0, 0), "application/json");
            return;
        }

        if (from == 0) from = oldest;
        if (to == 0) to = newest;

        if (from < oldest) from = oldest;
        if (to > newest) to = newest;

        auto data = buffer_->get_heatmap_range(from, to);

        if (step > 1 && data.size() > static_cast<size_t>(step)) {
            std::vector<HeatmapEntry> sampled;
            size_t i = 0;
            while (i < data.size()) {
                double max_cpu = -1.0, max_mem = -1.0;
                double max_dr = -1.0, max_dw = -1.0, max_nu = -1.0, max_nd = -1.0;
                for (size_t j = 0; j < static_cast<size_t>(step) && i + j < data.size(); ++j) {
                    const auto& d = data[i + j];
                    if (d.cpu_pct > max_cpu) max_cpu = d.cpu_pct;
                    if (d.mem_pct > max_mem) max_mem = d.mem_pct;
                    if (d.disk_read_bps > max_dr) max_dr = d.disk_read_bps;
                    if (d.disk_write_bps > max_dw) max_dw = d.disk_write_bps;
                    if (d.net_up_bps > max_nu) max_nu = d.net_up_bps;
                    if (d.net_down_bps > max_nd) max_nd = d.net_down_bps;
                }
                HeatmapEntry entry;
                entry.timestamp = data[i].timestamp;
                entry.cpu_pct = (max_cpu >= 0) ? max_cpu : -1.0;
                entry.mem_pct = (max_mem >= 0) ? max_mem : -1.0;
                entry.disk_read_bps  = (max_dr >= 0) ? max_dr : -1.0;
                entry.disk_write_bps = (max_dw >= 0) ? max_dw : -1.0;
                entry.net_up_bps   = (max_nu >= 0) ? max_nu : -1.0;
                entry.net_down_bps = (max_nd >= 0) ? max_nd : -1.0;
                sampled.push_back(entry);
                i += step;
            }
            res.set_content(json::serialize_heatmap(sampled, from, to), "application/json");
        } else {
            res.set_content(json::serialize_heatmap(data, from, to), "application/json");
        }
    });

    // GET /api/snapshot
    server_->Get("/api/snapshot", [this](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Content-Type", "application/json; charset=utf-8");

        if (!req.has_param("time")) {
            res.status = 400;
            res.set_content(json::serialize_error("missing_param", "Missing required parameter 'time'"), "application/json");
            return;
        }

        time_t t = 0;
        try { t = std::stoll(req.get_param_value("time")); }
        catch (...) {
            res.status = 400;
            res.set_content(json::serialize_error("invalid_param", "Parameter 'time' must be an integer timestamp"), "application/json");
            return;
        }

        if (buffer_->snapshot_size() == 0) {
            res.status = 404;
            res.set_content(json::serialize_error("no_data", "No snapshot data available yet"), "application/json");
            return;
        }

        const SystemSnapshot* snap = buffer_->find_nearest_snapshot(t);
        if (!snap) {
            time_t oldest = buffer_->oldest_snapshot_time();
            time_t newest = buffer_->newest_snapshot_time();

            if (t < oldest && oldest > 0) {
                const SystemSnapshot* earliest = buffer_->earliest_snapshot();
                if (earliest) {
                    auto avg = buffer_->average_heatmap_range(oldest, newest);
                    res.set_content(json::serialize_aggregate_snapshot(
                        avg.cpu_pct, avg.mem_pct, avg.disk_read_bps, avg.disk_write_bps,
                        avg.net_up_bps, avg.net_down_bps,
                        *earliest, t, oldest), "application/json");
                    return;
                }
            }

            if (t < oldest || t > newest) {
                res.status = 404;
                res.set_content(json::serialize_error("snapshot_expired",
                    "Process snapshots are only retained for the last " +
                    std::to_string(config_.snapshot_capacity) + " seconds"), "application/json");
            } else {
                res.status = 404;
                res.set_content(json::serialize_error("no_nearby_snapshot",
                    "No snapshot found within 120 seconds of the requested time"), "application/json");
            }
            return;
        }

        res.set_content(json::serialize_snapshot(*snap, t), "application/json");
    });

    // GET /api/status
    server_->Get("/api/status", [this](const httplib::Request&, httplib::Response& res) {
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time_).count();

        res.set_header("Content-Type", "application/json; charset=utf-8");
        res.set_content(json::serialize_status(
            "1.1.0-dev",
            uptime,
            buffer_->snapshot_size(),
            buffer_->heatmap_size(),
            buffer_->oldest_snapshot_time(),
            buffer_->newest_snapshot_time(),
            buffer_->latest_process_count(),
            config_.interval_ms
        ), "application/json");
    });
}