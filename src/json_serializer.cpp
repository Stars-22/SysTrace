// json_serializer.cpp - JSON string building for heatmap, snapshot, status and error responses
#include "json_serializer.h"
#include <cstdio>
#include <cmath>
#include <cstdint>

namespace json {

std::string escape(const std::string& s) {
    std::string result;
    result.reserve(s.size() + 16);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    result += buf;
                } else {
                    result += static_cast<char>(c);
                }
        }
    }
    return result;
}

static std::string format_double(double v) {
    if (v < 0) return "-1";
    if (v == 0.0) return "0";
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f", v);
    // Remove trailing zeros after decimal, but keep at least one decimal
    std::string s(buf);
    // Keep as-is for simplicity - one decimal place
    return s;
}

std::string serialize_heatmap(const std::vector<HeatmapEntry>& data,
                               time_t from, time_t to) {
    std::string json;
    json.reserve(data.size() * 64 + 128);

    char header[256];
    snprintf(header, sizeof(header),
        R"({"range":{"from":%lld,"to":%lld,"count":%zu},"data":[)",
        static_cast<long long>(from), static_cast<long long>(to), data.size());
    json += header;

    for (size_t i = 0; i < data.size(); ++i) {
        const auto& d = data[i];
        char entry[256];
        snprintf(entry, sizeof(entry),
            R"({"t":%lld,"cpu":%s,"mem":%s,"dr":%s,"dw":%s})",
            static_cast<long long>(d.timestamp),
            format_double(d.cpu_pct).c_str(),
            format_double(d.mem_pct).c_str(),
            format_double(d.disk_read_bps).c_str(),
            format_double(d.disk_write_bps).c_str());

        if (i > 0) json += ",";
        json += entry;
    }

    json += "]}";
    return json;
}

std::string serialize_snapshot(const SystemSnapshot& snap, time_t requested_time) {
    std::string json;
    json.reserve(4096);

    char header[512];
    snprintf(header, sizeof(header),
        R"({"timestamp":%lld,"system":{"cpu":%s,"mem":%s,"disk_read_bps":%s,"disk_write_bps":%s},)",
        static_cast<long long>(requested_time),
        format_double(snap.cpu_pct).c_str(),
        format_double(snap.mem_pct).c_str(),
        format_double(snap.disk_read_bps).c_str(),
        format_double(snap.disk_write_bps).c_str());

    json += header;

    long long delta = static_cast<long long>(snap.timestamp - requested_time);
    char meta[128];
    snprintf(meta, sizeof(meta),
        R"("meta":{"actual_time":%lld,"delta_seconds":%lld},"processes":[)",
        static_cast<long long>(snap.timestamp), delta);
    json += meta;

    for (size_t i = 0; i < snap.processes.size(); ++i) {
        const auto& p = snap.processes[i];
        char entry[512];
        snprintf(entry, sizeof(entry),
            R"({"pid":%lu,"name":"%s","cpu":%s,"mem":%s})",
            static_cast<unsigned long>(p.pid),
            escape(p.name).c_str(),
            format_double(p.cpu_pct).c_str(),
            format_double(p.mem_mb).c_str());

        if (i > 0) json += ",";
        json += entry;
    }

    json += "]}";
    return json;
}

std::string serialize_aggregate_snapshot(double cpu, double mem, double dr, double dw,
                                          const SystemSnapshot& earliest_snap,
                                          time_t requested_time, time_t actual_from) {
    std::string json;
    json.reserve(4096);

    char header[512];
    snprintf(header, sizeof(header),
        R"({"timestamp":%lld,"system":{"cpu":%s,"mem":%s,"disk_read_bps":%s,"disk_write_bps":%s},)",
        static_cast<long long>(requested_time),
        format_double(cpu).c_str(),
        format_double(mem).c_str(),
        format_double(dr).c_str(),
        format_double(dw).c_str());

    json += header;

    char meta[256];
    snprintf(meta, sizeof(meta),
        R"("meta":{"actual_time":%lld,"delta_seconds":0,"aggregated":true,"aggregated_from":%lld},"processes":[)",
        static_cast<long long>(earliest_snap.timestamp),
        static_cast<long long>(actual_from));
    json += meta;

    for (size_t i = 0; i < earliest_snap.processes.size(); ++i) {
        const auto& p = earliest_snap.processes[i];
        char entry[512];
        snprintf(entry, sizeof(entry),
            R"({"pid":%lu,"name":"%s","cpu":%s,"mem":%s})",
            static_cast<unsigned long>(p.pid),
            escape(p.name).c_str(),
            format_double(p.cpu_pct).c_str(),
            format_double(p.mem_mb).c_str());

        if (i > 0) json += ",";
        json += entry;
    }

    json += "]}";
    return json;
}

std::string serialize_status(const std::string& version,
                              long long uptime_seconds,
                              size_t snapshot_count,
                              size_t heatmap_count,
                              time_t oldest_snapshot,
                              time_t newest_snapshot,
                              size_t process_count,
                              int interval_ms) {
    char buf[512];
    snprintf(buf, sizeof(buf),
        R"({"version":"%s","uptime_seconds":%lld,"snapshot_count":%zu,"heatmap_count":%zu,)"
        R"("oldest_snapshot":%lld,"newest_snapshot":%lld,"process_count":%zu,"collection_interval_ms":%d})",
        version.c_str(),
        uptime_seconds,
        snapshot_count,
        heatmap_count,
        static_cast<long long>(oldest_snapshot),
        static_cast<long long>(newest_snapshot),
        process_count,
        interval_ms);
    return std::string(buf);
}

std::string serialize_error(const std::string& code, const std::string& message) {
    return R"({"error":")" + escape(code) + R"(","message":")" + escape(message) + R"("})";
}

}