// collector.cpp - System resource data collection implementation
#include "collector.h"
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <chrono>
#include <cmath>
#include <set>

Collector::Collector(RingBuffer& buffer, CollectorConfig config)
    : buffer_(buffer), config_(std::move(config)) {
    init_disk_io();
}

Collector::~Collector() {
    stop();
    if (nt_dll_) {
        FreeLibrary(static_cast<HMODULE>(nt_dll_));
    }
}

void Collector::start() {
    if (running_.load()) return;
    running_.store(true);
    thread_ = std::thread(&Collector::run, this);
}

void Collector::stop() {
    running_.store(false);
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Collector::run() {
    auto next_wake = std::chrono::steady_clock::now();
    const auto interval = std::chrono::milliseconds(config_.interval_ms);

    while (running_.load()) {
        next_wake += interval;
        collect_once();
        std::this_thread::sleep_until(next_wake);
    }
}

uint64_t Collector::filetime_to_uint64(const FILETIME& ft) {
    return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}

// init_disk_io() - Dynamically load ntdll.dll for disk IO statistics via NtQuerySystemInformation
void Collector::init_disk_io() {
    nt_dll_ = static_cast<void*>(LoadLibraryA("ntdll.dll"));
    if (!nt_dll_) {
        disk_io_available_ = false;
        return;
    }
    NtQuerySystemInformation_fn_ = reinterpret_cast<NtQuerySystemInformation_t>(
        GetProcAddress(static_cast<HMODULE>(nt_dll_), "NtQuerySystemInformation"));
    if (!NtQuerySystemInformation_fn_) {
        disk_io_available_ = false;
    }
}

// query_disk_io() - Query disk read/write bytes using NtQuerySystemInformation (class 2)
bool Collector::query_disk_io(double& read_bps, double& write_bps) {
    if (!disk_io_available_ || !NtQuerySystemInformation_fn_) {
        read_bps = -1.0;
        write_bps = -1.0;
        return false;
    }

    // SystemPerformanceInformation = class 2
    struct SYSTEM_PERFORMANCE_INFORMATION {
        uint64_t idle_time;
        uint64_t read_operation_count;
        uint64_t write_operation_count;
        uint64_t other_operation_count;
        uint64_t read_transfer_count;
        uint64_t write_transfer_count;
        uint64_t other_transfer_count;
    };

    SYSTEM_PERFORMANCE_INFORMATION perf = {};
    ULONG ret_len = 0;
    long status = NtQuerySystemInformation_fn_(2, &perf, sizeof(perf), &ret_len);
    if (status != 0) {
        read_bps = -1.0;
        write_bps = -1.0;
        return false;
    }

    DiskIOSample curr;
    curr.read_bytes  = perf.read_transfer_count;
    curr.write_bytes = perf.write_transfer_count;

    if (first_disk_sample_) {
        prev_disk_io_ = curr;
        first_disk_sample_ = false;
        read_bps = -1.0;
        write_bps = -1.0;
        return false;
    }

    double interval_sec = static_cast<double>(config_.interval_ms) / 1000.0;
    read_bps  = static_cast<double>(curr.read_bytes - prev_disk_io_.read_bytes) / interval_sec;
    write_bps = static_cast<double>(curr.write_bytes - prev_disk_io_.write_bytes) / interval_sec;

    prev_disk_io_ = curr;
    return true;
}

// get_process_name() - Retrieve process executable name from PID via QueryFullProcessImageNameA
std::string Collector::get_process_name(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess) {
        HANDLE hProcess2 = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!hProcess2) return "<unknown>(" + std::to_string(pid) + ")";
        char buf[MAX_PATH];
        DWORD buf_size = MAX_PATH;
        BOOL ok = QueryFullProcessImageNameA(hProcess2, 0, buf, &buf_size);
        CloseHandle(hProcess2);
        if (!ok) return "<unknown>(" + std::to_string(pid) + ")";
        std::string full_path(buf, buf_size);
        size_t pos = full_path.find_last_of("\\/");
        return (pos != std::string::npos) ? full_path.substr(pos + 1) : full_path;
    }

    char buf[MAX_PATH];
    DWORD buf_size = MAX_PATH;
    BOOL ok = QueryFullProcessImageNameA(hProcess, 0, buf, &buf_size);
    CloseHandle(hProcess);

    if (!ok) return "<unknown>(" + std::to_string(pid) + ")";
    std::string full_path(buf, buf_size);
    size_t pos = full_path.find_last_of("\\/");
    return (pos != std::string::npos) ? full_path.substr(pos + 1) : full_path;
}

// collect_once() - Collect one cycle of system/process metrics and push to ring buffer
void Collector::collect_once() {
    try {
        time_t timestamp = std::time(nullptr);

        // ===== System CPU =====
        FILETIME idle_ft, kernel_ft, user_ft;
        double system_cpu = 0.0;
        uint64_t sys_delta_total = 0;

        if (GetSystemTimes(&idle_ft, &kernel_ft, &user_ft)) {
            SystemCpuSample curr;
            curr.idle   = filetime_to_uint64(idle_ft);
            curr.kernel = filetime_to_uint64(kernel_ft);
            curr.user   = filetime_to_uint64(user_ft);

            if (!first_sample_) {
                uint64_t delta_idle   = curr.idle   - prev_sys_cpu_.idle;
                uint64_t delta_kernel = curr.kernel - prev_sys_cpu_.kernel;
                uint64_t delta_user   = curr.user   - prev_sys_cpu_.user;
                uint64_t total = delta_kernel + delta_user;

                if (total > 0) {
                    system_cpu = (1.0 - static_cast<double>(delta_idle) / static_cast<double>(total)) * 100.0;
                    system_cpu = std::clamp(system_cpu, 0.0, 100.0);
                }
                sys_delta_total = total;
            }

            prev_sys_cpu_ = curr;
            first_sample_ = false;
        }

        // ===== System Memory =====
        double memory_pct = 0.0;
        MEMORYSTATUSEX mem_info;
        mem_info.dwLength = sizeof(mem_info);
        if (GlobalMemoryStatusEx(&mem_info)) {
            memory_pct = static_cast<double>(mem_info.dwMemoryLoad);
        }

        // ===== Disk IO =====
        double disk_read_bps = -1.0, disk_write_bps = -1.0;
        query_disk_io(disk_read_bps, disk_write_bps);

        // ===== Process List =====
        std::vector<ProcessInfo> processes;
        std::set<DWORD> active_pids;

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);

            if (Process32First(snapshot, &pe32)) {
                do {
                    DWORD pid = pe32.th32ProcessID;
                    active_pids.insert(pid);

                    if (processes.size() >= config_.max_processes) continue;

                    ProcessInfo info;
                    info.pid = pid;

                    // Process name
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
                    if (!hProcess) {
                        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
                    }

                    if (hProcess) {
                        // Process name from full path
                        char name_buf[MAX_PATH];
                        DWORD name_size = MAX_PATH;
                        if (QueryFullProcessImageNameA(hProcess, 0, name_buf, &name_size)) {
                            std::string full_path(name_buf, name_size);
                            size_t pos = full_path.find_last_of("\\/");
                            info.name = (pos != std::string::npos) ? full_path.substr(pos + 1) : full_path;
                        } else {
                            info.name = pe32.szExeFile;
                        }

                        // Process CPU
                        FILETIME create_ft, exit_ft, proc_kernel_ft, proc_user_ft;
                        if (GetProcessTimes(hProcess, &create_ft, &exit_ft, &proc_kernel_ft, &proc_user_ft)) {
                            uint64_t curr_kernel = filetime_to_uint64(proc_kernel_ft);
                            uint64_t curr_user   = filetime_to_uint64(proc_user_ft);

                            auto ctx_it = proc_contexts_.find(pid);
                            if (ctx_it != proc_contexts_.end() && sys_delta_total > 0) {
                                uint64_t proc_delta = (curr_kernel - ctx_it->second.prev_kernel_time)
                                                   + (curr_user   - ctx_it->second.prev_user_time);
                                double pct = static_cast<double>(proc_delta) / static_cast<double>(sys_delta_total) * 100.0;
                                info.cpu_pct = std::clamp(pct, 0.0, 100.0);
                                ctx_it->second.prev_kernel_time = curr_kernel;
                                ctx_it->second.prev_user_time   = curr_user;
                                ctx_it->second.name = info.name;
                            } else {
                                proc_contexts_[pid] = {pid, info.name, curr_kernel, curr_user};
                            }
                        }

                        // Process Memory
                        PROCESS_MEMORY_COUNTERS_EX pmc;
                        pmc.cb = sizeof(pmc);
                        if (GetProcessMemoryInfo(hProcess, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
                            info.mem_mb = static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0);
                        }

                        CloseHandle(hProcess);
                    } else {
                        info.name = pe32.szExeFile;
                    }

                    processes.push_back(std::move(info));
                } while (Process32Next(snapshot, &pe32));
            }
            CloseHandle(snapshot);
        }

        // ===== Cleanup exited processes from context =====
        for (auto it = proc_contexts_.begin(); it != proc_contexts_.end(); ) {
            if (active_pids.find(it->first) == active_pids.end()) {
                it = proc_contexts_.erase(it);
            } else {
                ++it;
            }
        }

        // Sort by CPU desc, then by memory desc
        std::sort(processes.begin(), processes.end(),
            [](const ProcessInfo& a, const ProcessInfo& b) {
                if (a.cpu_pct != b.cpu_pct) return a.cpu_pct > b.cpu_pct;
                return a.mem_mb > b.mem_mb;
            });

        // ===== Build snapshot =====
        SystemSnapshot snap;
        snap.timestamp      = timestamp;
        snap.cpu_pct        = system_cpu;
        snap.mem_pct        = memory_pct;
        snap.disk_read_bps  = disk_read_bps;
        snap.disk_write_bps = disk_write_bps;
        snap.processes      = std::move(processes);

        buffer_.push(std::move(snap));
    } catch (const std::exception& e) {
        // Swallow exception, try again next cycle
    } catch (...) {
        // Swallow unknown exception
    }
}