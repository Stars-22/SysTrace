// persistence.cpp - Binary append-log persistence implementation
#include "persistence.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sys/stat.h>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

constexpr uint32_t HEAT_MAGIC = 0x4D485453; // "STHM"
constexpr uint32_t SNAP_MAGIC = 0x4E535453; // "STSN"
constexpr uint32_t FILE_VERSION = 1;

#pragma pack(push, 1)
struct HeatHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t interval_ms;
    uint32_t reserved;
};

struct HeatRecord {
    int64_t  timestamp;
    double   cpu_pct;
    double   mem_pct;
    double   disk_read_bps;
    double   disk_write_bps;
    double   net_up_bps;
    double   net_down_bps;
};

struct SnapFixedHeader {
    int64_t  timestamp;
    double   cpu_pct;
    double   mem_pct;
    double   disk_read_bps;
    double   disk_write_bps;
    double   net_up_bps;
    double   net_down_bps;
    uint32_t proc_count;
};
#pragma pack(pop)

constexpr size_t HEAT_HEADER_SIZE = sizeof(HeatHeader);
constexpr size_t HEAT_RECORD_SIZE = sizeof(HeatRecord);
constexpr size_t SNAP_HEADER_SIZE = sizeof(HeatHeader);
constexpr size_t SNAP_FIXED_SIZE  = sizeof(SnapFixedHeader);

bool dir_exists(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR);
}

bool make_dir(const std::string& path) {
#ifdef _WIN32
    return CreateDirectoryA(path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

std::string exe_dir() {
#ifdef _WIN32
    char buf[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (len == 0) return ".";
    std::string full(buf, len);
    size_t pos = full.find_last_of("\\/");
    return (pos != std::string::npos) ? full.substr(0, pos) : ".";
#else
    return ".";
#endif
}

} // namespace

std::string Persistence::heat_path() const {
    return dir_ + "\\systrace_heatmap.bin";
}

std::string Persistence::snap_path() const {
    return dir_ + "\\systrace_snapshots.bin";
}

Persistence::Persistence(const std::string& data_dir,
                         int interval_ms,
                         size_t heatmap_capacity,
                         size_t snapshot_capacity)
    : interval_ms_(interval_ms),
      heatmap_cap_(heatmap_capacity),
      snapshot_cap_(snapshot_capacity) {
    if (data_dir.empty()) {
        dir_ = exe_dir();
    } else {
        dir_ = data_dir;
    }
}

Persistence::~Persistence() {
    close_files();
}

bool Persistence::open_files() {
    if (!dir_exists(dir_)) {
        make_dir(dir_);
    }

    heat_file_ = fopen(heat_path().c_str(), "r+b");
    if (!heat_file_) {
        heat_file_ = fopen(heat_path().c_str(), "w+b");
        if (heat_file_) {
            HeatHeader hdr{HEAT_MAGIC, FILE_VERSION, static_cast<uint32_t>(interval_ms_), 0};
            fwrite(&hdr, 1, HEAT_HEADER_SIZE, heat_file_);
            fflush(heat_file_);
        }
    }
    if (!heat_file_) return false;

    snap_file_ = fopen(snap_path().c_str(), "r+b");
    if (!snap_file_) {
        snap_file_ = fopen(snap_path().c_str(), "w+b");
        if (snap_file_) {
            HeatHeader hdr{SNAP_MAGIC, FILE_VERSION, static_cast<uint32_t>(interval_ms_), 0};
            fwrite(&hdr, 1, SNAP_HEADER_SIZE, snap_file_);
            fflush(snap_file_);
        }
    }
    if (!snap_file_) {
        fclose(heat_file_);
        heat_file_ = nullptr;
        return false;
    }

    fseek(heat_file_, 0, SEEK_END);
    fseek(snap_file_, 0, SEEK_END);
    return true;
}

void Persistence::close_files() {
    if (heat_file_) { fflush(heat_file_); fclose(heat_file_); heat_file_ = nullptr; }
    if (snap_file_) { fflush(snap_file_); fclose(snap_file_); snap_file_ = nullptr; }
}

bool Persistence::read_heatmap_record(FILE* f, HeatmapEntry& e) {
    HeatRecord rec;
    size_t n = fread(&rec, 1, HEAT_RECORD_SIZE, f);
    if (n != HEAT_RECORD_SIZE) return false;
    e.timestamp      = static_cast<time_t>(rec.timestamp);
    e.cpu_pct        = rec.cpu_pct;
    e.mem_pct        = rec.mem_pct;
    e.disk_read_bps  = rec.disk_read_bps;
    e.disk_write_bps = rec.disk_write_bps;
    e.net_up_bps     = rec.net_up_bps;
    e.net_down_bps   = rec.net_down_bps;
    return true;
}

bool Persistence::write_heatmap_record(FILE* f, const HeatmapEntry& e) {
    HeatRecord rec;
    rec.timestamp      = static_cast<int64_t>(e.timestamp);
    rec.cpu_pct        = e.cpu_pct;
    rec.mem_pct        = e.mem_pct;
    rec.disk_read_bps  = e.disk_read_bps;
    rec.disk_write_bps = e.disk_write_bps;
    rec.net_up_bps     = e.net_up_bps;
    rec.net_down_bps   = e.net_down_bps;
    return fwrite(&rec, 1, HEAT_RECORD_SIZE, f) == HEAT_RECORD_SIZE;
}

bool Persistence::read_snapshot_record(FILE* f, SystemSnapshot& s) {
    SnapFixedHeader fh;
    size_t n = fread(&fh, 1, SNAP_FIXED_SIZE, f);
    if (n != SNAP_FIXED_SIZE) return false;

    if (fh.proc_count > 2000) return false;

    s.timestamp      = static_cast<time_t>(fh.timestamp);
    s.cpu_pct        = fh.cpu_pct;
    s.mem_pct        = fh.mem_pct;
    s.disk_read_bps  = fh.disk_read_bps;
    s.disk_write_bps = fh.disk_write_bps;
    s.net_up_bps     = fh.net_up_bps;
    s.net_down_bps   = fh.net_down_bps;
    s.processes.clear();
    s.processes.reserve(fh.proc_count);

    for (uint32_t i = 0; i < fh.proc_count; ++i) {
        uint32_t pid;
        if (fread(&pid, 1, 4, f) != 4) return false;

        uint16_t name_len;
        if (fread(&name_len, 1, 2, f) != 2) return false;
        if (name_len > 256) return false;

        char name_buf[257];
        if (name_len > 0) {
            if (fread(name_buf, 1, name_len, f) != name_len) return false;
        }
        name_buf[name_len] = '\0';

        double cpu, mem, ior, iow;
        if (fread(&cpu, 1, 8, f) != 8) return false;
        if (fread(&mem, 1, 8, f) != 8) return false;
        if (fread(&ior, 1, 8, f) != 8) return false;
        if (fread(&iow, 1, 8, f) != 8) return false;

        ProcessInfo pi;
        pi.pid          = pid;
        pi.name         = std::string(name_buf, name_len);
        pi.cpu_pct      = cpu;
        pi.mem_mb       = mem;
        pi.io_read_bps  = ior;
        pi.io_write_bps = iow;
        s.processes.push_back(std::move(pi));
    }
    return true;
}

bool Persistence::write_snapshot_record(FILE* f, const SystemSnapshot& s) {
    SnapFixedHeader fh;
    fh.timestamp      = static_cast<int64_t>(s.timestamp);
    fh.cpu_pct        = s.cpu_pct;
    fh.mem_pct        = s.mem_pct;
    fh.disk_read_bps  = s.disk_read_bps;
    fh.disk_write_bps = s.disk_write_bps;
    fh.net_up_bps     = s.net_up_bps;
    fh.net_down_bps   = s.net_down_bps;
    fh.proc_count     = static_cast<uint32_t>(s.processes.size());

    if (fwrite(&fh, 1, SNAP_FIXED_SIZE, f) != SNAP_FIXED_SIZE) return false;

    for (const auto& p : s.processes) {
        uint32_t pid = p.pid;
        if (fwrite(&pid, 1, 4, f) != 4) return false;

        uint16_t name_len = static_cast<uint16_t>(p.name.size() > 255 ? 255 : p.name.size());
        if (fwrite(&name_len, 1, 2, f) != 2) return false;
        if (name_len > 0) {
            if (fwrite(p.name.data(), 1, name_len, f) != name_len) return false;
        }

        if (fwrite(&p.cpu_pct, 1, 8, f) != 8) return false;
        if (fwrite(&p.mem_mb, 1, 8, f) != 8) return false;
        if (fwrite(&p.io_read_bps, 1, 8, f) != 8) return false;
        if (fwrite(&p.io_write_bps, 1, 8, f) != 8) return false;
    }
    return true;
}

bool Persistence::recover(RingBuffer& buf) {
    if (!heat_file_ || !snap_file_) return false;

    // === Recover heatmap ===
    fseek(heat_file_, 0, SEEK_END);
    long heat_size = ftell(heat_file_);
    if (heat_size < static_cast<long>(HEAT_HEADER_SIZE)) {
        fprintf(stderr, "[SysTrace] Heatmap file too small, skipping recovery\n");
    } else {
        long record_bytes = heat_size - static_cast<long>(HEAT_HEADER_SIZE);
        long total_records = record_bytes / static_cast<long>(HEAT_RECORD_SIZE);

        if (total_records > 0) {
            long skip = 0;
            if (static_cast<size_t>(total_records) > heatmap_cap_) {
                skip = total_records - static_cast<long>(heatmap_cap_);
            }

            fseek(heat_file_, static_cast<long>(HEAT_HEADER_SIZE) + skip * static_cast<long>(HEAT_RECORD_SIZE), SEEK_SET);

            int recovered = 0;
            HeatmapEntry e;
            while (read_heatmap_record(heat_file_, e)) {
                SystemSnapshot snap;
                snap.timestamp      = e.timestamp;
                snap.cpu_pct        = e.cpu_pct;
                snap.mem_pct        = e.mem_pct;
                snap.disk_read_bps  = e.disk_read_bps;
                snap.disk_write_bps = e.disk_write_bps;
                snap.net_up_bps     = e.net_up_bps;
                snap.net_down_bps   = e.net_down_bps;
                buf.push(std::move(snap));
                recovered++;
            }
            fprintf(stderr, "[SysTrace] Recovered %d heatmap records\n", recovered);
        }
    }

    // === Recover snapshots ===
    fseek(snap_file_, 0, SEEK_END);
    long snap_size = ftell(snap_file_);
    if (snap_size < static_cast<long>(SNAP_HEADER_SIZE)) {
        fprintf(stderr, "[SysTrace] Snapshot file too small, skipping recovery\n");
        return true;
    }

    // First pass: collect file offsets of all records
    fseek(snap_file_, static_cast<long>(SNAP_HEADER_SIZE), SEEK_SET);
    std::vector<long> offsets;
    long pos = static_cast<long>(SNAP_HEADER_SIZE);
    SystemSnapshot s;
    while (true) {
        long curr = ftell(snap_file_);
        if (!read_snapshot_record(snap_file_, s)) break;
        offsets.push_back(curr);
    }

    if (offsets.empty()) {
        fprintf(stderr, "[SysTrace] No valid snapshot records to recover\n");
        return true;
    }

    // Determine which records to keep (last snapshot_cap_ entries)
    size_t start_idx = 0;
    if (offsets.size() > snapshot_cap_) {
        start_idx = offsets.size() - snapshot_cap_;
    }

    // We need to re-read the kept records and push them to buffer
    // But the buffer already has heatmap data pushed above.
    // The snapshots need to go into the snapshot deque in the RingBuffer.
    // Since RingBuffer.push() adds to both heatmap and snapshot deques,
    // we need a method to add snapshot-only data.
    // For simplicity, we'll re-read the kept snapshots and push them.
    // However, this would also add duplicate heatmap entries.
    //
    // Approach: We'll add a method to RingBuffer to push snapshot-only.
    // For now, let's just push the snapshots normally - the heatmap
    // duplicates will be deduplicated by timestamp (oldest pushed first,
    // and ring buffer capacity will trim excess).

    // Actually, better approach: read all kept snapshots, then clear
    // the buffer's snapshot deque and refill it.
    // But RingBuffer doesn't expose that. Let's add a push_snapshot_only method.

    // For now, let's just note how many snapshots we recovered
    // and push them directly. The heatmap data was already pushed above
    // and the snapshot push will add heatmap entries too, but since
    // we're within capacity limits, it should be fine.

    int snap_recovered = 0;
    for (size_t i = start_idx; i < offsets.size(); ++i) {
        fseek(snap_file_, offsets[i], SEEK_SET);
        if (read_snapshot_record(snap_file_, s)) {
            buf.push_snapshot_only(std::move(s));
            snap_recovered++;
        }
    }
    fprintf(stderr, "[SysTrace] Recovered %d snapshot records (total found: %zu)\n",
            snap_recovered, offsets.size());

    return true;
}

bool Persistence::trim_heatmap() {
    if (!heat_file_) return false;

    fseek(heat_file_, 0, SEEK_END);
    long file_size = ftell(heat_file_);
    if (file_size < static_cast<long>(HEAT_HEADER_SIZE)) return false;

    long record_bytes = file_size - static_cast<long>(HEAT_HEADER_SIZE);
    long total_records = record_bytes / static_cast<long>(HEAT_RECORD_SIZE);

    if (static_cast<size_t>(total_records) <= heatmap_cap_) {
        fseek(heat_file_, 0, SEEK_END);
        return true;
    }

    long keep = static_cast<long>(heatmap_cap_);
    long skip = total_records - keep;

    // Read the records we want to keep
    fseek(heat_file_, static_cast<long>(HEAT_HEADER_SIZE) + skip * static_cast<long>(HEAT_RECORD_SIZE), SEEK_SET);
    std::vector<HeatRecord> records(keep);
    size_t read = fread(records.data(), HEAT_RECORD_SIZE, keep, heat_file_);
    if (read != static_cast<size_t>(keep)) return false;

    // Rewrite file with only kept records
    std::string tmp_path = heat_path() + ".tmp";
    FILE* tmp = fopen(tmp_path.c_str(), "wb");
    if (!tmp) return false;

    HeatHeader hdr{HEAT_MAGIC, FILE_VERSION, static_cast<uint32_t>(interval_ms_), 0};
    fwrite(&hdr, 1, HEAT_HEADER_SIZE, tmp);
    fwrite(records.data(), HEAT_RECORD_SIZE, keep, tmp);
    fflush(tmp);
    fclose(tmp);

    fclose(heat_file_);
    heat_file_ = nullptr;

#ifdef _WIN32
    DeleteFileA(heat_path().c_str());
    MoveFileA(tmp_path.c_str(), heat_path().c_str());
#else
    rename(tmp_path.c_str(), heat_path().c_str());
#endif

    heat_file_ = fopen(heat_path().c_str(), "r+b");
    fseek(heat_file_, 0, SEEK_END);
    fprintf(stderr, "[SysTrace] Trimmed heatmap file: %ld -> %ld records\n", total_records, keep);
    return true;
}

bool Persistence::trim_snapshots() {
    if (!snap_file_) return false;

    fseek(snap_file_, 0, SEEK_END);
    long file_size = ftell(snap_file_);
    if (file_size < static_cast<long>(SNAP_HEADER_SIZE)) return false;

    // First pass: find all record offsets
    fseek(snap_file_, static_cast<long>(SNAP_HEADER_SIZE), SEEK_SET);
    std::vector<long> offsets;
    SystemSnapshot s;
    while (true) {
        long curr = ftell(snap_file_);
        if (!read_snapshot_record(snap_file_, s)) break;
        offsets.push_back(curr);
    }

    if (offsets.size() <= snapshot_cap_) {
        fseek(snap_file_, 0, SEEK_END);
        return true;
    }

    size_t start_idx = offsets.size() - snapshot_cap_;

    // Rewrite file with only kept records
    std::string tmp_path = snap_path() + ".tmp";
    FILE* tmp = fopen(tmp_path.c_str(), "wb");
    if (!tmp) return false;

    HeatHeader hdr{SNAP_MAGIC, FILE_VERSION, static_cast<uint32_t>(interval_ms_), 0};
    fwrite(&hdr, 1, SNAP_HEADER_SIZE, tmp);

    for (size_t i = start_idx; i < offsets.size(); ++i) {
        fseek(snap_file_, offsets[i], SEEK_SET);
        if (read_snapshot_record(snap_file_, s)) {
            write_snapshot_record(tmp, s);
        }
    }
    fflush(tmp);
    fclose(tmp);

    fclose(snap_file_);
    snap_file_ = nullptr;

#ifdef _WIN32
    DeleteFileA(snap_path().c_str());
    MoveFileA(tmp_path.c_str(), snap_path().c_str());
#else
    rename(tmp_path.c_str(), snap_path().c_str());
#endif

    snap_file_ = fopen(snap_path().c_str(), "r+b");
    fseek(snap_file_, 0, SEEK_END);
    fprintf(stderr, "[SysTrace] Trimmed snapshot file: %zu -> %zu records\n", offsets.size(), snapshot_cap_);
    return true;
}

void Persistence::trim() {
    trim_heatmap();
    trim_snapshots();
    // Ensure file positions are at end for appending
    if (heat_file_) fseek(heat_file_, 0, SEEK_END);
    if (snap_file_) fseek(snap_file_, 0, SEEK_END);
}

void Persistence::write_heatmap(const HeatmapEntry& e) {
    if (heat_file_) {
        write_heatmap_record(heat_file_, e);
    }
}

void Persistence::write_snapshot(const SystemSnapshot& s) {
    if (snap_file_) {
        write_snapshot_record(snap_file_, s);
    }
}

void Persistence::flush_if_needed(time_t now) {
    if (now - last_flush_ < flush_interval_ms_ / 1000) return;
    if (heat_file_) fflush(heat_file_);
    if (snap_file_) fflush(snap_file_);
    last_flush_ = now;
}
