// collector.h - System resource collector using Windows API (CPU, memory, disk IO, per-process stats)
#pragma once

#include "ring_buffer.h"
#include <windows.h>
#include <psapi.h>
#include <atomic>
#include <thread>
#include <unordered_map>

struct CollectorConfig {
    int interval_ms = 1000;
    size_t max_processes = 500;
};

class Collector {
public:
    explicit Collector(RingBuffer& buffer, CollectorConfig config = {});
    ~Collector();

    void start();
    void stop();

private:
    void run();
    void collect_once();

    static uint64_t filetime_to_uint64(const FILETIME& ft);

    RingBuffer& buffer_;
    CollectorConfig config_;
    std::atomic<bool> running_{false};
    std::thread thread_;

    struct SystemCpuSample {
        uint64_t idle   = 0;
        uint64_t kernel = 0;
        uint64_t user   = 0;
    };

    struct ProcessCpuContext {
        DWORD pid;
        std::string name;
        uint64_t prev_kernel_time = 0;
        uint64_t prev_user_time   = 0;
    };

    SystemCpuSample prev_sys_cpu_{};
    bool first_sample_ = true;

    struct DiskIOSample {
        uint64_t read_bytes  = 0;
        uint64_t write_bytes = 0;
    };

    DiskIOSample prev_disk_io_{};
    bool first_disk_sample_ = true;
    bool disk_io_available_ = true;
    void* nt_dll_ = nullptr;

    std::unordered_map<DWORD, ProcessCpuContext> proc_contexts_;
    std::unordered_map<std::string, uint8_t> name_pool_;

    using NtQuerySystemInformation_t = long(*)(unsigned long, void*, unsigned long, unsigned long*);
    NtQuerySystemInformation_t NtQuerySystemInformation_fn_ = nullptr;

    void init_disk_io();
    bool query_disk_io(double& read_bps, double& write_bps);
    std::string get_process_name(DWORD pid);
};