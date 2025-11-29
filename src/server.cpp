#include "server.h"

#include <QDebug>
#include <mutex>
#include "network.h"

Server::Server(unsigned int port) : port(port) {
  LOG("[Server] instance created on port %d", port);
}

Server::~Server() { stop(); }

void Server::run(bool inBackground) {
    isRunning = true;
    if (inBackground) {
        lobbyThread = std::thread(&Server::listenForClients, this);
    } else {
        listenForClients();
    }
}

void Server::stop() {
    LOG("[Server] Destructor called, stopping server");
    isRunning = false;
    if (serverSocket != INVALID_SOCKET) {
      closeSocket(serverSocket);
      serverSocket = -1;
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto& client : clientThreads) {
            if (client->socket != INVALID_SOCKET) {
                closeSocket(client->socket);
                client->socket = -1;
            }
        }
    }

    if (lobbyThread.joinable()) {
        lobbyThread.join();
    }

    cleanupClientThreads();
    cleanupSockets();
}

void Server::listenForClients() {
    while (isRunning) {

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            // Garbage Collector-esque
            auto it = clientThreads.begin();
            bool removedClient = false;
            while (it != clientThreads.end()) {
                if ((*it)->finished) {
                    LOG("[Server] Cleaning up finished client thread for player %d",(*it)->playerId);
                    // 1. Join the thread to ensure proper cleanup
                    if ((*it)->thread.joinable()) {
                        (*it)->thread.join();
                    }
                    // 2. Remove from vector (this reduces the size!)
                    it = clientThreads.erase(it);
                    removedClient = true;
                } else {
                    ++it;
                }
            }
            if (removedClient) broadcastPlayerList();
            if (clientThreads.size() >= 8) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }
        }

        // Accept a new connection
        sockaddr_in clientAddr;
#ifdef _WIN32
        int clientAddrLen = sizeof(clientAddr);
#else
        socklen_t clientAddrLen = sizeof(clientAddr);
#endif

        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket == INVALID_SOCKET) {
            if (!isRunning) {
                // This is the server shutting down
                continue;
            }
            LOG("[Server] Accept failed");
            continue; // Don't exit, just try next connection
        }

        uint32_t newPlayerId = nextPlayerId++;
        LOG("[Server] New client connected, playerId: %d, socket: %d", newPlayerId, clientSocket);

        auto newClient = std::make_unique<ClientInfo>(clientSocket, newPlayerId);

        // We must capture the pointer to the 'finished' flag to pass it to the thread
        // We use a raw pointer here because the unique_ptr in the vector owns the memory
        std::atomic<bool>* finishedFlag = &newClient->finished;

        // Start the thread
        newClient->thread = std::thread(&Server::handleClient, this, clientSocket, newPlayerId, finishedFlag);

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clientThreads.push_back(std::move(newClient));
        }

        broadcastPlayerList();
    }
}

void Server::cleanupClientThreads() {
    std::lock_guard<std::mutex> lock(clientsMutex);
    LOG("[Server] Cleaning up %zu client threads", clientThreads.size());
    for (auto& client : clientThreads) {
        client->finished = true;
        client->join();
    }
    clientThreads.clear();
}

void Server::handleClient(SOCKET clientSocket, uint32_t playerId, std::atomic<bool>* finishedFlag) {
    broadcastPlayerList(clientSocket);

    char buffer[1024];
    // Loop to keep receiving data until the client disconnects
    while (true) {
        memset(buffer, 0, sizeof(buffer)); // Clear buffer

        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) break;

        LOG("[Server] Received from player %d: %s", playerId, buffer);

        if (strcmp(buffer, "REQUEST_PLAYER_LIST") == 0) {
            broadcastPlayerList(clientSocket); // send to that client
        }
    }

    LOG("[Server] client (playerId: %d) disconnected", playerId);
    closeSocket(clientSocket);
    *finishedFlag = true;
}

void Server::broadcastPlayerList(SOCKET client) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    std::string message = "PLAYER_LIST:";
    for (int i = 0; i <  clientThreads.size(); ++i) {
      message += "Player " + std::to_string(clientThreads[i]->playerId)
                            + (i != clientThreads.size()-1 ? "," : "");
    }
    std::string framedMessage = std::to_string(message.length()) + ":" + message;
    if (client == -1) notifyAll(framedMessage.c_str());
    else notifyAllOthers(framedMessage.c_str(), client);
}

void Server::notifyAll(const char* msg) {
    for (auto& client : this->clientThreads) {
        if (client->socket != INVALID_SOCKET) {
            sendMessage(msg, client->socket);
        }
    }
}

void Server::notifyAllOthers(const char* msg, SOCKET client) {
    for (auto& other : this->clientThreads) {
        if (other->socket == client || other->socket == INVALID_SOCKET) {
            continue;
        }
        sendMessage(msg, other->socket);
    }
}

void Server::sendMessage(const char* msg, SOCKET client) {
    int totalSent = 0;
    int msgLength = strlen(msg);
    while (totalSent < msgLength) {
        int bytesSent = send(client, msg + totalSent, msgLength - totalSent, 0);
        if (bytesSent <= 0) {
            LOG("[Server] Failed to send message to socket %d", client);
            break;
        }
        totalSent += bytesSent;
    }
    if (totalSent == msgLength) {
        LOG("[Server] Successfully sent %d bytes to socket %d", totalSent, client);
    }
}

bool Server::init() {
    if (!initSockets()) {
        qErrnoWarning("[Server] Socket initialization failed.\n", 200);
        return false;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        qErrnoWarning("[Server] Error creating socket.\n", 200);
        cleanupSockets();
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        qErrnoWarning("[Server] Bind failed. Port might be in use.\n", 200);
        closeSocket(serverSocket);
        cleanupSockets();
        return false;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        qErrnoWarning("[Server] Listen failed.\n", 200);
        closeSocket(serverSocket);
        cleanupSockets();
        return false;
    }

    LOG("[Server] Listening on port %d at ip 127.0.0.1", port);
    return true;
}
