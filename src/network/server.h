#ifndef SERVER_H
#define SERVER_H

#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include "../game/game.h"
#include "network.h"

extern std::mutex consoleMutex;

struct ClientInfo {
    SOCKET socket;
    uint32_t playerId;
    std::thread thread;
    std::atomic<bool> running{true}; // Flag to track status
    std::string receiveBuffer;

    // Helper to make moving this struct into a vector easier
    ClientInfo(SOCKET s, uint32_t id) : socket(s), playerId(id) {}

    // Disable copying (threads can't be copied), allow moving
    ClientInfo(const ClientInfo&) = delete;
};

class Server
{
public:
    Server(unsigned int port = 12345);
    ~Server();

    bool init();
    void start(bool inBackground = true);
    void start_game();
    void stop();
private:
    void gameLoop();
    void listenForClients();
    void cleanFinishedClientThreads();

    void stopClient(ClientInfo* client);
    void handleClient(ClientInfo* client);

    void processClientMessage(const std::string& message);

    void broadcastServerShutdown();
    void broadcastPlayerList();
    void broadcastGameState();
    void assignPlayerId(ClientInfo* client);
    void notifyAll(const char* msg, SOCKET avoid = INVALID_SOCKET);
    void notifyAllOthers(const char* msg, SOCKET socket);

    unsigned int port;
    SOCKET serverSocket = INVALID_SOCKET;

    std::atomic<bool> serverRunning{false};
    std::atomic<bool> gameRunning{false};

    std::thread lobbyThread;
    std::thread gameThread;

    std::mutex clientsMutex;
    std::vector<std::unique_ptr<ClientInfo>> clientThreads;

    std::unique_ptr<Game> game;

    std::atomic<uint32_t> nextPlayerId{1};
};

#endif // SERVER_H
