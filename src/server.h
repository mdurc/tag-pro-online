#ifndef SERVER_H
#define SERVER_H

#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include "game.h"
#include "network.h"

extern std::mutex consoleMutex;

struct ClientInfo {
    std::thread thread;
    std::atomic<bool> finished{false}; // Flag to track status
    SOCKET socket;
    uint32_t playerId;

    // Helper to make moving this struct into a vector easier
    ClientInfo(SOCKET s, uint32_t id) : socket(s), playerId(id) {}
    bool joinable() { return thread.joinable(); }
    void join() { if (thread.joinable()) thread.join(); }

    // Disable copying (threads can't be copied), allow moving
    ClientInfo(const ClientInfo&) = delete;
};

class Server
{
public:
    Server(unsigned int port = 12345);
    ~Server();

    bool init();
    void run(bool inBackground = true);
    void stop();
private:
    void listenForClients();
    void cleanupClientThreads();
    void cleanupFinishedClients();

    void handleClient(SOCKET clientSocket, uint32_t playerId, std::atomic<bool>* finished_flag);

    void broadcastPlayerList(SOCKET client = -1);
    void notifyAll(const char* msg);
    void notifyAllOthers(const char* msg, SOCKET client);
    bool sendMessage(const char* msg, SOCKET client);

    uint32_t nextPlayerId = 1;

    // Network Variables
    unsigned int port;
    SOCKET serverSocket = INVALID_SOCKET;
    std::thread lobbyThread;
    std::atomic<bool> isRunning{false}; // Flag to stop thread safely

    std::mutex clientsMutex;
    std::vector<std::unique_ptr<ClientInfo>> clientThreads;
};

#endif // SERVER_H
