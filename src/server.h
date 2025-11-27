#ifndef SERVER_H
#define SERVER_H

#include <thread>
#include <atomic>
#include <vector>
#include <queue>
#include <mutex>
#include "game.h"
#include "network.h"

extern std::mutex consoleMutex;

struct ClientInfo {
    std::thread thread;
    std::atomic<bool> finished{false}; // Flag to track status
    SOCKET socket;

    // Helper to make moving this struct into a vector easier
    ClientInfo(SOCKET s) : socket(s) {}
    bool joinable() { return thread.joinable(); }
    void join() { thread.join(); }

    // Disable copying (threads can't be copied), allow moving
    ClientInfo(const ClientInfo&) = delete;
};

struct GameEvent {
    uint32_t playerId;
    float x;
    float y;
};

class Server
{
public:
    Server(unsigned int port = 12345);
    ~Server();

    void listenForClients();
private:
    bool init();
    void handleClient(SOCKET clientSocket, std::atomic<bool>* finished_flag);

    void handleEventQueue();
    void notifyAll(char* msg);

    // Game stuffs
    void update();
    Game game;

    std::mutex queueMutex;
    std::queue<GameEvent> eventQueue;

    // Network Variables
    unsigned int port;
    SOCKET serverSocket = -1;
    std::thread netThread;
    std::atomic<bool> isRunning; // Flag to stop thread safely

    std::vector<std::unique_ptr<ClientInfo>> clientThreads;
};

#endif // SERVER_H
