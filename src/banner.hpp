#pragma once

#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "common.hpp"

using namespace std;

inline string get_probe_for_service(const string& service) {
    if (service == "http" || service == "http-alt")
        return "HEAD / HTTP/1.0\r\nHost: localhost\r\n\r\n";

    // these talk first, no nudge needed
    if (service == "ftp"  ||
        service == "smtp" ||
        service == "ssh"  ||
        service == "pop3" ||
        service == "imap")
        return "";

    return "";
}

inline bool wait_for_data(SOCKET sock, int timeout_ms) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);

    timeval tv{};
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    return select(0, &read_fds, nullptr, nullptr, &tv) > 0;
}

inline string grab_banner(const string& ip,
                           uint16_t      port,
                           const string& service,
                           int           timeout_ms) {

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
        return "";

    // on windows SO_RCVTIMEO takes a DWORD in milliseconds, not a timeval
    DWORD tv = timeout_ms;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
        closesocket(sock);
        return "";
    }

    string probe = get_probe_for_service(service);
    if (!probe.empty())
        send(sock, probe.c_str(), (int)probe.size(), 0);

    if (!wait_for_data(sock, timeout_ms)) {
        closesocket(sock);
        return "";
    }

    char buf[1024] = {};
    int  n         = recv(sock, buf, sizeof(buf) - 1, 0);
    closesocket(sock);

    if (n <= 0)
        return "";

    string banner(buf, n);
    while (!banner.empty() && (banner.back() == '\n' ||
                                banner.back() == '\r' ||
                                banner.back() == '\0'))
        banner.pop_back();

    return banner;
}