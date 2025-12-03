#ifndef CLIENT_H
#define CLIENT_H

#include <thread>
#include <mutex>

#include "network.h"

class Client {
public:
    using MessageCallback = std::function<void(const std::string&)>;
    using ConnectionCallback = std::function<void(bool)>;

    Client();
    ~Client();

    void connect(int port, const char* ip);
    void disconnect();

    void sendMessage(const std::string& message);
    void sendPlayerInput(float x, float y);

    void setMessageCallback(MessageCallback callback);
    void setConnectionCallback(ConnectionCallback callback);
    void clearCallbacks();

    uint32_t getPlayerId() const { return playerId; }

private:
    void createSocket();
    void receiveLoop();
    void processIncomingData();

    SOCKET clientSocket = INVALID_SOCKET;
    std::thread receivingThread;

    std::atomic<bool> isRunning{false};
    std::atomic<uint32_t> playerId{0};

    std::mutex bufferMutex;
    std::string receiveBuffer;

    std::mutex callbackMutex;
    MessageCallback messageCallback;
    ConnectionCallback connectionCallback;
};

#endif // CLIENT_H
