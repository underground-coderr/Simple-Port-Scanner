#pragma once

#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "common.hpp"

using namespace std;

inline string resolve_host(const string& host) {
    addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0)
        return "";

    char ip[INET_ADDRSTRLEN];
    auto* addr = (sockaddr_in*)res->ai_addr;
    inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
    freeaddrinfo(res);
    return string(ip);
}

inline PortStatus probe_port(const string& ip, uint16_t port, int timeout_ms) {

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
        return PortStatus::UNKNOWN;

    // set non-blocking — windows way
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    int rc = connect(sock, (sockaddr*)&addr, sizeof(addr));

    if (rc == SOCKET_ERROR) {
        int err = WSAGetLastError();
        // WSAEWOULDBLOCK is the windows equivalent of EINPROGRESS
        if (err != WSAEWOULDBLOCK) {
            closesocket(sock);
            return PortStatus::CLOSED;
        }
    } else {
        // connected instantly
        closesocket(sock);
        return PortStatus::OPEN;
    }

    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(sock, &write_fds);

    timeval tv{};
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    // first arg to select() is ignored on windows, just pass 0
    int ready = select(0, nullptr, &write_fds, nullptr, &tv);

    if (ready == 0) {
        closesocket(sock);
        return PortStatus::FILTERED;
    }

    if (ready == SOCKET_ERROR) {
        closesocket(sock);
        return PortStatus::UNKNOWN;
    }

    int err = 0;
    int len  = sizeof(err);
    getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
    closesocket(sock);

    if (err == 0)                    return PortStatus::OPEN;
    if (err == WSAECONNREFUSED)      return PortStatus::CLOSED;
    return PortStatus::FILTERED;
}