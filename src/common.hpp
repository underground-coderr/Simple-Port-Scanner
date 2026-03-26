#pragma once

#include <string>
#include <stdint.h>
#include <unordered_map>

using namespace std;

enum class PortStatus {
    OPEN,
    CLOSED,
    FILTERED,
    UNKNOWN    // renamed from ERROR — windows.h defines ERROR as a macro
};

struct ScanResult {
    uint16_t   port    = 0;
    PortStatus status  = PortStatus::UNKNOWN;
    string     service = "";
    string     banner  = "";
};

struct ScanConfig {
    string   host;
    uint16_t port_start         = 1;
    uint16_t port_end           = 1024;
    int      threads            = 100;
    int      connect_timeout_ms = 2000;
    int      banner_timeout_ms  = 2000;
    bool     grab_banners       = true;
    bool     show_closed        = false;
    bool     show_filtered      = false;
};

inline string service_name(uint16_t port) {
    static const unordered_map<uint16_t, string> table = {
        {21,   "ftp"},
        {22,   "ssh"},
        {23,   "telnet"},
        {25,   "smtp"},
        {53,   "dns"},
        {67,   "dhcp"},
        {69,   "tftp"},
        {80,   "http"},
        {110,  "pop3"},
        {111,  "rpcbind"},
        {119,  "nntp"},
        {123,  "ntp"},
        {135,  "msrpc"},
        {139,  "netbios"},
        {143,  "imap"},
        {161,  "snmp"},
        {179,  "bgp"},
        {443,  "https"},
        {445,  "smb"},
        {465,  "smtps"},
        {587,  "smtp-sub"},
        {993,  "imaps"},
        {995,  "pop3s"},
        {1080, "socks5"},
        {1433, "mssql"},
        {1521, "oracle"},
        {1723, "pptp"},
        {2049, "nfs"},
        {2375, "docker"},
        {3000, "http-alt"},
        {3306, "mysql"},
        {3389, "rdp"},
        {5000, "http-alt"},
        {5432, "postgres"},
        {5900, "vnc"},
        {6379, "redis"},
        {6667, "irc"},
        {8080, "http-alt"},
        {8443, "https-alt"},
        {8888, "http-alt"},
        {9000, "http-alt"},
        {9200, "elasticsearch"},
        {9300, "elasticsearch"},
        {9092, "kafka"},
        {11211,"memcached"},
        {27017,"mongodb"},
        {27018,"mongodb"},
    };

    auto it = table.find(port);
    return it != table.end() ? it->second : "";
}