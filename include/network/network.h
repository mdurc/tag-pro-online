#ifndef NETWORK_H
#define NETWORK_H

#pragma once

#include <QDebug>
#include <mutex>

extern std::mutex consoleMutex;
#define LOG(fmt, ...) \
    { std::lock_guard<std::mutex> lock(consoleMutex); \
      qInfo().noquote() << QString().asprintf(fmt, ##__VA_ARGS__); }

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")

    inline void shutdownSocket(SOCKET sock) { shutdown(sock, SD_BOTH); }
    inline void closeSocket(SOCKET sock) { closesocket(sock); }
    inline bool initSockets() {
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
    }
    inline void cleanupSockets() { WSACleanup(); }
#else // _WIN32
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <sys/select.h>
    #include <arpa/inet.h>
    #include <unistd.h>

    using SOCKET = int;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1

    inline void shutdownSocket(SOCKET sock) { shutdown(sock, SHUT_RDWR); }
    inline void closeSocket(SOCKET sock) { close(sock); }
    inline bool initSockets() { return true; }
    inline void cleanupSockets() {}
#endif // _WIN32
#endif // NETWORK_H
