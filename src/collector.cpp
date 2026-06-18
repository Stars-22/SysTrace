// collector.cpp - System resource data collection implementation
#include "collector.h"
#include "persistence.h"

// Windows headers must be included in a specific order to avoid conflicts
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <iphlpapi.h>
#include <chrono>
#include <cmath>
#include <set>

// Link libraries
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "iphlpapi.lib")

Collector::Collector(RingBuffer& buffer, CollectorConfig config, Persistence* persist)
    : buffer_(buffer), config_(std::move(config)), persistence_(persist) {
    // PDH query will be initialized lazily in query_disk_io()
}

Collector::~Collector() {
    stop();
    if (pdh_query_ != nullptr) {
        PdhCloseQuery(pdh_query_);
        pdh_query_ = nullptr;
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

// query_disk_io() - Query disk read/write bytes per second using PDH (Performance Data Helper)
// PDH is the official Windows API for querying performance counters.
// We use the "\PhysicalDisk(_Total)\Disk Read Bytes/sec" and "\Disk Write Bytes/sec" counters.
bool Collector::query_disk_io(double& read_bps, double& write_bps) {
    if (!disk_io_available_) {
        read_bps = -1.0;
        write_bps = -1.0;
        return false;
    }

    if (pdh_query_ == nullptr) {
        PDH_STATUS status = PdhOpenQuery(nullptr, 0, &pdh_query_);
        if (status != ERROR_SUCCESS) {
            disk_io_available_ = false;
            read_bps = -1.0;
            write_bps = -1.0;
            return false;
        }

        status = PdhAddEnglishCounterA(pdh_query_, "\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &pdh_counter_read_);
        if (status != ERROR_SUCCESS) {
            PdhCloseQuery(pdh_query_);
            pdh_query_ = nullptr;
            disk_io_available_ = false;
            read_bps = -1.0;
            write_bps = -1.0;
            return false;
        }

        status = PdhAddEnglishCounterA(pdh_query_, "\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", 0, &pdh_counter_write_);
        if (status != ERROR_SUCCESS) {
            PdhCloseQuery(pdh_query_);
            pdh_query_ = nullptr;
            disk_io_available_ = false;
            read_bps = -1.0;
            write_bps = -1.0;
            return false;
        }

        // First call to PdhCollectQueryData establishes the baseline
        PdhCollectQueryData(pdh_query_);
        first_disk_sample_ = true;
        read_bps = -1.0;
        write_bps = -1.0;
        return false;
    }

    PDH_STATUS status = PdhCollectQueryData(pdh_query_);
    if (status != ERROR_SUCCESS) {
        read_bps = -1.0;
        write_bps = -1.0;
        return false;
    }

    if (first_disk_sample_) {
        first_disk_sample_ = false;
        read_bps = -1.0;
        write_bps = -1.0;
        return false;
    }

    PDH_FMT_COUNTERVALUE val_read, val_write;
    status = PdhGetFormattedCounterValue(pdh_counter_read_, PDH_FMT_DOUBLE, nullptr, &val_read);
    if (status != ERROR_SUCCESS) {
        read_bps = -1.0;
    } else {
        read_bps = val_read.doubleValue;
    }

    status = PdhGetFormattedCounterValue(pdh_counter_write_, PDH_FMT_DOUBLE, nullptr, &val_write);
    if (status != ERROR_SUCCESS) {
        write_bps = -1.0;
    } else {
        write_bps = val_write.doubleValue;
    }

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

// query_network() - Query system-wide network upload/download bytes per second using GetIfTable2
// Sums octets across all operational non-loopback interfaces and computes delta between samples.
bool Collector::query_network(double& up_bps, double& down_bps) {
    up_bps = -1.0;
    down_bps = -1.0;

    PMIB_IF_TABLE2 if_table = nullptr;
    DWORD err = GetIfTable2(&if_table);
    if (err != NO_ERROR) {
        return false;
    }

    uint64_t total_sent = 0;
    uint64_t total_received = 0;

    for (ULONG i = 0; i < if_table->NumEntries; ++i) {
        auto& row = if_table->Table[i];
        if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK) {
            continue;
        }
        if (row.OperStatus != IfOperStatusUp) {
            continue;
        }
        total_sent += row.OutOctets;
        total_received += row.InOctets;
    }

    FreeMibTable(if_table);

    if (first_net_sample_) {
        prev_net_.bytes_sent = total_sent;
        prev_net_.bytes_received = total_received;
        first_net_sample_ = false;
        return false;
    }

    double interval_sec = config_.interval_ms / 1000.0;
    uint64_t delta_sent = total_sent - prev_net_.bytes_sent;
    uint64_t delta_received = total_received - prev_net_.bytes_received;
    prev_net_.bytes_sent = total_sent;
    prev_net_.bytes_received = total_received;

    up_bps = static_cast<double>(delta_sent) / interval_sec;
    down_bps = static_cast<double>(delta_received) / interval_sec;
    return true;
}

// collect_once() - Collect one cycle of system/process metrics and push to ring buffer
void Collector::collect_once() {
    try {
        time_t timestamp = std::time(nullptr);

        bool was_first = first_sample_;

        // ===== System CPU =====
        FILETIME idle_ft, kernel_ft, user_ft;
        double system_cpu = -1.0;
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
        double memory_pct = -1.0;
        if (!was_first) {
            MEMORYSTATUSEX mem_info;
            mem_info.dwLength = sizeof(mem_info);
            if (GlobalMemoryStatusEx(&mem_info)) {
                memory_pct = static_cast<double>(mem_info.dwMemoryLoad);
            }
        }

        // ===== Disk IO =====
        double disk_read_bps = -1.0, disk_write_bps = -1.0;
        query_disk_io(disk_read_bps, disk_write_bps);

        // ===== Network =====
        double net_up_bps = -1.0, net_down_bps = -1.0;
        query_network(net_up_bps, net_down_bps);

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
                                proc_contexts_[pid] = {pid, info.name, curr_kernel, curr_user, 0, 0, false};
                            }
                        }

                        // Process Disk IO
                        IO_COUNTERS io_counters;
                        if (GetProcessIoCounters(hProcess, &io_counters)) {
                            uint64_t curr_read  = io_counters.ReadTransferCount;
                            uint64_t curr_write = io_counters.WriteTransferCount;
                            auto io_it = proc_contexts_.find(pid);
                            if (io_it != proc_contexts_.end() && io_it->second.has_io_baseline) {
                                double interval_sec = config_.interval_ms / 1000.0;
                                uint64_t delta_read  = curr_read  - io_it->second.prev_read_bytes;
                                uint64_t delta_write = curr_write - io_it->second.prev_write_bytes;
                                info.io_read_bps  = static_cast<double>(delta_read)  / interval_sec;
                                info.io_write_bps = static_cast<double>(delta_write) / interval_sec;
                                io_it->second.prev_read_bytes  = curr_read;
                                io_it->second.prev_write_bytes = curr_write;
                            } else if (io_it != proc_contexts_.end()) {
                                io_it->second.prev_read_bytes  = curr_read;
                                io_it->second.prev_write_bytes = curr_write;
                                io_it->second.has_io_baseline   = true;
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

                    if (!was_first) {
                        processes.push_back(std::move(info));
                    }
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
        snap.net_up_bps   = net_up_bps;
        snap.net_down_bps = net_down_bps;
        snap.processes      = std::move(processes);

        if (persistence_) {
            HeatmapEntry he;
            he.timestamp      = snap.timestamp;
            he.cpu_pct        = snap.cpu_pct;
            he.mem_pct        = snap.mem_pct;
            he.disk_read_bps  = snap.disk_read_bps;
            he.disk_write_bps = snap.disk_write_bps;
            he.net_up_bps     = snap.net_up_bps;
            he.net_down_bps   = snap.net_down_bps;
            persistence_->write_heatmap(he);
            persistence_->write_snapshot(snap);
            persistence_->flush_if_needed(timestamp);
        }

        buffer_.push(std::move(snap));
    } catch (const std::exception& e) {
        // Swallow exception, try again next cycle
    } catch (...) {
        // Swallow unknown exception
    }
}