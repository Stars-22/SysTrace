// json_serializer.h - Manual JSON serialization for API responses (no external JSON library)
#pragma once

#include "ring_buffer.h"
#include <string>
#include <vector>

namespace json {

std::string escape(const std::string& s);

std::string serialize_heatmap(const std::vector<HeatmapEntry>& data,
                               time_t from, time_t to);

std::string serialize_snapshot(const SystemSnapshot& snap, time_t requested_time);

std::string serialize_aggregate_snapshot(double cpu, double mem, double dr, double dw,
                                          const SystemSnapshot& earliest_snap,
                                          time_t requested_time, time_t actual_from);

std::string serialize_status(const std::string& version,
                              long long uptime_seconds,
                              size_t snapshot_count,
                              size_t heatmap_count,
                              time_t oldest_snapshot,
                              time_t newest_snapshot,
                              size_t process_count,
                              int interval_ms);

std::string serialize_error(const std::string& code, const std::string& message);

}