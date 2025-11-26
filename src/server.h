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

    // Helper to make moving this struct into a vector easier
    ClientInfo(SOCKET s) : socket(s) {}
    bool joinable() { return thread.joinable(); }
    void join() { thread.join(); }

    // Disable copying (threads can't be copied), allow moving
    ClientInfo(const ClientInfo&) = delete;
};

class Server
{
public:
    Server(unsigned int port = 12345);
    ~Server();

    void run();
private:
    void init();
    void handleClient(SOCKET clientSocket, std::atomic<bool>* finished_flag);

    Game game;

    // Network Variables
    unsigned int port;
    SOCKET serverSocket = -1;
    SOCKET clientSocket = -1;
    std::thread netThread;
    std::atomic<bool> running; // Flag to stop thread safely

    std::vector<std::unique_ptr<ClientInfo>> clientThreads;
};

#endif // SERVER_H
