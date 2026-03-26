#pragma once

#include <vector>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <iostream>

#include "common.hpp"
#include "thread_pool.hpp"
#include "probe.hpp"
#include "banner.hpp"

using namespace std;

class PortScanner {
public:

    PortScanner(const ScanConfig& cfg) : config(cfg) {}

    // kick off the scan and return sorted results when done
    vector<ScanResult> run() {

        // resolve hostname once up front — no point doing it per-port
        string ip = resolve_host(config.host);
        if (ip.empty()) {
            cerr << "error: could not resolve host '" << config.host << "'\n";
            return {};
        }

        cout << "scanning " << config.host << " (" << ip << ")"
             << "  ports " << config.port_start
             << "-"        << config.port_end
             << "  threads " << config.threads << "\n\n";

        int total_ports = config.port_end - config.port_start + 1;
        completed.store(0);

        ThreadPool pool(config.threads);

        // one task per port — the pool decides when each one actually runs
        for (int port = config.port_start; port <= config.port_end; port++) {
            pool.enqueue([this, ip, port] {
                scan_one(ip, static_cast<uint16_t>(port));
            });
        }

        // block here until all tasks are done
        // we know we're done when completed == total_ports
        while (completed.load() < total_ports) {
            this_thread::sleep_for(chrono::milliseconds(50));
            print_progress(completed.load(), total_ports);
        }

        // clear the progress line before printing results
        cout << "\r" << string(60, ' ') << "\r";

        // sort by port number before returning
        sort(results.begin(), results.end(), [](const ScanResult& a,
                                                const ScanResult& b) {
            return a.port < b.port;
        });

        return results;
    }

private:

    ScanConfig         config;
    vector<ScanResult> results;
    mutex              results_mutex;
    atomic<int>        completed{0};

    // this runs inside a worker thread — keep it self-contained
    void scan_one(const string& ip, uint16_t port) {
        ScanResult result;
        result.port    = port;
        result.service = service_name(port);
        result.status  = probe_port(ip, port, config.connect_timeout_ms);

        // only bother grabbing a banner if the port is actually open
        if (result.status == PortStatus::OPEN && config.grab_banners)
            result.banner = grab_banner(ip, port, result.service,
                                        config.banner_timeout_ms);

        // lock only long enough to push the result
        {
            lock_guard<mutex> lock(results_mutex);
            results.push_back(result);
        }

        completed.fetch_add(1);
    }

    void print_progress(int done, int total) {
        float pct = (float)done / total * 100.0f;
        cout << "\r  progress: " << done << "/" << total
             << "  (" << (int)pct << "%)    " << flush;
    }
};