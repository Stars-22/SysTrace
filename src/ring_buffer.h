// ring_buffer.h - Two-level ring buffer with read-write lock for heatmap and snapshot storage
#pragma once

#include <deque>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

#ifndef _WINDOWS_
typedef unsigned long DWORD;
#endif

struct ProcessInfo {
    DWORD       pid     = 0;
    std::string name;
    double      cpu_pct = 0.0;
    double      mem_mb  = 0.0;
};

struct HeatmapEntry {
    time_t timestamp    = 0;
    double cpu_pct      = 0.0;
    double mem_pct      = 0.0;
    double disk_read_bps  = -1.0;
    double disk_write_bps = -1.0;
};

struct SystemSnapshot {
    time_t timestamp = 0;
    double cpu_pct   = 0.0;
    double mem_pct   = 0.0;
    double disk_read_bps  = -1.0;
    double disk_write_bps = -1.0;
    std::vector<ProcessInfo> processes;
};

class RingBuffer {
public:
    explicit RingBuffer(size_t heatmap_capacity = 86400,
                        size_t snapshot_capacity = 3600)
        : heatmap_capacity_(heatmap_capacity),
          snapshot_capacity_(snapshot_capacity) {}

    void push(SystemSnapshot snapshot) {
        std::unique_lock lock(mutex_);

        HeatmapEntry entry;
        entry.timestamp     = snapshot.timestamp;
        entry.cpu_pct       = snapshot.cpu_pct;
        entry.mem_pct       = snapshot.mem_pct;
        entry.disk_read_bps  = snapshot.disk_read_bps;
        entry.disk_write_bps = snapshot.disk_write_bps;

        if (heatmap_data_.size() >= heatmap_capacity_) {
            heatmap_data_.pop_front();
        }
        heatmap_data_.push_back(std::move(entry));

        if (snapshot_data_.size() >= snapshot_capacity_) {
            snapshot_data_.pop_front();
        }
        snapshot_data_.push_back(std::move(snapshot));
    }

    std::vector<HeatmapEntry> get_heatmap_range(time_t from, time_t to) const {
        std::shared_lock lock(mutex_);
        std::vector<HeatmapEntry> result;
        for (const auto& e : heatmap_data_) {
            if (e.timestamp >= from && e.timestamp <= to) {
                result.push_back(e);
            }
        }
        return result;
    }

    std::vector<HeatmapEntry> get_all_heatmap() const {
        std::shared_lock lock(mutex_);
        return std::vector<HeatmapEntry>(heatmap_data_.begin(), heatmap_data_.end());
    }

    const SystemSnapshot* find_nearest_snapshot(time_t t, int max_delta_seconds = 30) const {
        std::shared_lock lock(mutex_);
        if (snapshot_data_.empty()) return nullptr;

        auto it = std::lower_bound(snapshot_data_.begin(), snapshot_data_.end(), t,
            [](const SystemSnapshot& s, time_t val) {
                return s.timestamp < val;
            });

        if (it == snapshot_data_.end()) {
            it = std::prev(it);
        } else if (it != snapshot_data_.begin()) {
            auto prev_it = std::prev(it);
            if (t - prev_it->timestamp <= it->timestamp - t) {
                it = prev_it;
            }
        }

        if (std::abs(static_cast<long long>(it->timestamp - t)) <= max_delta_seconds) {
            return &(*it);
        }
        return nullptr;
    }

    time_t oldest_heatmap_time() const {
        std::shared_lock lock(mutex_);
        if (heatmap_data_.empty()) return 0;
        return heatmap_data_.front().timestamp;
    }

    time_t newest_heatmap_time() const {
        std::shared_lock lock(mutex_);
        if (heatmap_data_.empty()) return 0;
        return heatmap_data_.back().timestamp;
    }

    time_t oldest_snapshot_time() const {
        std::shared_lock lock(mutex_);
        if (snapshot_data_.empty()) return 0;
        return snapshot_data_.front().timestamp;
    }

    time_t newest_snapshot_time() const {
        std::shared_lock lock(mutex_);
        if (snapshot_data_.empty()) return 0;
        return snapshot_data_.back().timestamp;
    }

    const SystemSnapshot* earliest_snapshot() const {
        std::shared_lock lock(mutex_);
        if (snapshot_data_.empty()) return nullptr;
        return &snapshot_data_.front();
    }

    struct HeatmapAverage {
        double cpu_pct      = 0.0;
        double mem_pct      = 0.0;
        double disk_read_bps  = -1.0;
        double disk_write_bps = -1.0;
        int    count        = 0;
    };

    HeatmapAverage average_heatmap_range(time_t from, time_t to) const {
        std::shared_lock lock(mutex_);
        HeatmapAverage avg;
        double sum_cpu = 0, sum_mem = 0, sum_dr = 0, sum_dw = 0;
        int dr_count = 0, dw_count = 0;
        for (const auto& e : heatmap_data_) {
            if (e.timestamp < from) continue;
            if (e.timestamp > to) break;
            sum_cpu += e.cpu_pct;
            sum_mem += e.mem_pct;
            if (e.disk_read_bps >= 0) { sum_dr += e.disk_read_bps; dr_count++; }
            if (e.disk_write_bps >= 0) { sum_dw += e.disk_write_bps; dw_count++; }
            avg.count++;
        }
        if (avg.count > 0) {
            avg.cpu_pct = sum_cpu / avg.count;
            avg.mem_pct = sum_mem / avg.count;
            avg.disk_read_bps  = (dr_count > 0)  ? sum_dr / dr_count  : -1.0;
            avg.disk_write_bps = (dw_count > 0) ? sum_dw / dw_count : -1.0;
        }
        return avg;
    }

    size_t heatmap_size() const {
        std::shared_lock lock(mutex_);
        return heatmap_data_.size();
    }

    size_t snapshot_size() const {
        std::shared_lock lock(mutex_);
        return snapshot_data_.size();
    }

    size_t latest_process_count() const {
        std::shared_lock lock(mutex_);
        if (snapshot_data_.empty()) return 0;
        return snapshot_data_.back().processes.size();
    }

private:
    std::deque<HeatmapEntry>  heatmap_data_;
    std::deque<SystemSnapshot> snapshot_data_;
    size_t heatmap_capacity_;
    size_t snapshot_capacity_;
    mutable std::shared_mutex mutex_;
};