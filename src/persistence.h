// persistence.h - Binary append-log persistence for heatmap and snapshot data
#pragma once

#include "ring_buffer.h"
#include <string>
#include <cstdint>

class Persistence {
public:
    Persistence(const std::string& data_dir,
                int interval_ms,
                size_t heatmap_capacity,
                size_t snapshot_capacity);
    ~Persistence();

    bool open_files();
    void close_files();

    bool recover(RingBuffer& buf);
    void trim();

    void write_heatmap(const HeatmapEntry& e);
    void write_snapshot(const SystemSnapshot& s);
    void flush_if_needed(time_t now);

private:
    std::string dir_;
    int interval_ms_;
    size_t heatmap_cap_;
    size_t snapshot_cap_;

    FILE* heat_file_ = nullptr;
    FILE* snap_file_ = nullptr;

    time_t last_flush_ = 0;
    int flush_interval_ms_ = 10000;

    std::string heat_path() const;
    std::string snap_path() const;

    bool read_heatmap_record(FILE* f, HeatmapEntry& e);
    bool read_snapshot_record(FILE* f, SystemSnapshot& s);
    bool write_heatmap_record(FILE* f, const HeatmapEntry& e);
    bool write_snapshot_record(FILE* f, const SystemSnapshot& s);

    bool trim_heatmap();
    bool trim_snapshots();
};
