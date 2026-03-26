#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstdlib>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "common.hpp"
#include "scanner.hpp"

using namespace std;

void print_usage(const char* prog) {
    cout << "\nusage: " << prog << " [options] <host>\n\n"
         << "options:\n"
         << "  -p <start>-<end>   port range        (default: 1-1024)\n"
         << "  -t <n>             threads            (default: 100)\n"
         << "  -T <ms>            connect timeout ms (default: 2000)\n"
         << "  -b                 grab banners       (default: on)\n"
         << "  -B                 skip banner grab\n"
         << "  -c                 show closed ports\n"
         << "  -f                 show filtered ports\n"
         << "  -h                 show this help\n\n"
         << "examples:\n"
         << "  " << prog << " scanme.nmap.org\n"
         << "  " << prog << " -p 1-65535 -t 200 192.168.1.1\n"
         << "  " << prog << " -p 80-443 -B scanme.nmap.org\n\n";
}

bool parse_range(const string& s, uint16_t& start, uint16_t& end) {
    auto dash = s.find('-');
    if (dash == string::npos) return false;
    try {
        start = (uint16_t)stoi(s.substr(0, dash));
        end   = (uint16_t)stoi(s.substr(dash + 1));
        return start <= end;
    } catch (...) {
        return false;
    }
}

string status_label(PortStatus s) {
    switch (s) {
        case PortStatus::OPEN:     return "open";
        case PortStatus::CLOSED:   return "closed";
        case PortStatus::FILTERED: return "filtered";
        default:                   return "error";
    }
}

void print_results(const vector<ScanResult>& results,
                   const ScanConfig&         cfg) {

    cout << left
         << setw(8)  << "port"
         << setw(12) << "status"
         << setw(16) << "service"
         << "banner\n";
    cout << string(72, '-') << "\n";

    int open_count = 0;

    for (auto& r : results) {

        if (r.status == PortStatus::CLOSED   && !cfg.show_closed)   continue;
        if (r.status == PortStatus::FILTERED  && !cfg.show_filtered) continue;

        if (r.status == PortStatus::OPEN) open_count++;

        cout << left
             << setw(8)  << r.port
             << setw(12) << status_label(r.status)
             << setw(16) << (r.service.empty() ? "-" : r.service);

        if (!r.banner.empty()) {
            auto newline = r.banner.find('\n');
            cout << (newline != string::npos
                        ? r.banner.substr(0, newline)
                        : r.banner);
        }

        cout << "\n";
    }

    cout << string(72, '-') << "\n";
    cout << open_count << " open port(s) found\n\n";
}

int main(int argc, char* argv[]) {

    // windows requires this before any socket call
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cerr << "error: failed to initialize winsock\n";
        return 1;
    }

    if (argc < 2) {
        print_usage(argv[0]);
        WSACleanup();
        return 1;
    }

    ScanConfig cfg;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

        if (arg == "-h") {
            print_usage(argv[0]);
            WSACleanup();
            return 0;

        } else if (arg == "-p" && i + 1 < argc) {
            if (!parse_range(argv[++i], cfg.port_start, cfg.port_end)) {
                cerr << "error: bad port range '" << argv[i] << "'\n";
                WSACleanup();
                return 1;
            }

        } else if (arg == "-t" && i + 1 < argc) {
            cfg.threads = atoi(argv[++i]);
            if (cfg.threads < 1 || cfg.threads > 1000) {
                cerr << "error: threads must be 1-1000\n";
                WSACleanup();
                return 1;
            }

        } else if (arg == "-T" && i + 1 < argc) {
            cfg.connect_timeout_ms = atoi(argv[++i]);

        } else if (arg == "-b") {
            cfg.grab_banners = true;

        } else if (arg == "-B") {
            cfg.grab_banners = false;

        } else if (arg == "-c") {
            cfg.show_closed = true;

        } else if (arg == "-f") {
            cfg.show_filtered = true;

        } else if (arg[0] != '-') {
            cfg.host = arg;

        } else {
            cerr << "error: unknown option '" << arg << "'\n";
            print_usage(argv[0]);
            WSACleanup();
            return 1;
        }
    }

    if (cfg.host.empty()) {
        cerr << "error: no host specified\n";
        print_usage(argv[0]);
        WSACleanup();
        return 1;
    }

    PortScanner scanner(cfg);
    auto results = scanner.run();
    print_results(results, cfg);

    WSACleanup();
    return 0;
}