#include "server.h"

#include <QDebug>
#include <mutex>

Server::Server(unsigned int port) {
    this->port = port;
}

Server::~Server() {
    {
        std::lock_guard<std::mutex> lock(consoleMutex);
        qInfo() << "[Server] Cleaning up.";
    }
    isRunning = false;
    closeSocket(serverSocket);
    cleanupSockets();

    for (auto &t : clientThreads) {
        if ((*t).joinable()) {
            (*t).join();
        }
    }
    if (lobbyThread.joinable()) {
        lobbyThread.join();
    }
}

void Server::run() {
    lobbyThread = std::thread(&Server::listenForClients, this);
}

void Server::listenForClients() {
    isRunning = true;
    while (isRunning) {

        // Garbage Collector-esque
        auto it = clientThreads.begin();
        while (it != clientThreads.end()) {
            if ((*it)->finished) {
                // 1. Join the thread to ensure proper cleanup
                if ((*it)->thread.joinable()) {
                    (*it)->thread.join();
                }
                // 2. Remove from vector (this reduces the size!)
                it = clientThreads.erase(it);

                {
                    std::lock_guard<std::mutex> lock(consoleMutex);
                    qDebug() << "[Server] Cleaned up thread of recently disconnected client.";
                }
            } else {
                ++it;
            }
        }
        if (clientThreads.size() >= 8) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
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
            std::lock_guard<std::mutex> lock(consoleMutex);
            qDebug() << "[Server] Accept failed.";
            continue; // Don't exit, just try next connection
        }

        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            qInfo() << "[Server] New client connected! Spawning thread...\n";
        }

        auto newClient = std::make_unique<ClientInfo>(clientSocket);

        // We must capture the pointer to the 'finished' flag to pass it to the thread
        // We use a raw pointer here because the unique_ptr in the vector owns the memory
        std::atomic<bool>* finishedFlag = &newClient->finished;

        // Start the thread
        newClient->thread = std::thread(&Server::handleClient, this, clientSocket, finishedFlag);

        // Add to vector
        clientThreads.push_back(std::move(newClient));

        notifyAll("New Player has joined.");
    }
}

void Server::handleClient(SOCKET clientSocket, std::atomic<bool>* finishedFlag) {
    char buffer[1024];

    // Loop to keep receiving data until the client disconnects
    while (true) {
        memset(buffer, 0, sizeof(buffer)); // Clear buffer

        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            // If 0, client disconnected. If -1, error.
            break;
        }

        // Print received message safely
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            qInfo() << "[Server] Received: " << buffer << "\n";
        }

        // Send a simple echo response
        const char* response = "Message received by server";
        send(clientSocket, response, strlen(response), 0);

        // TODO:
        // Add command to server queue
    }

    {
        std::lock_guard<std::mutex> lock(consoleMutex);
        qInfo() << "[Server] Client disconnected.";
    }

    // Clean up this specific client socket
    closeSocket(clientSocket);

    *finishedFlag = true;
}

void Server::notifyAll(char* msg) {
    for (auto& client : this->clientThreads) {
        send(client->socket, msg, strlen(msg), 0);
    }
}

void Server::notifyAllOthers(char* msg, SOCKET client) {
    for (auto& other : this->clientThreads) {
        if (other->socket == client) {
            continue;
        }
        send(other->socket, msg, strlen(msg), 0);
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

    {
        std::lock_guard<std::mutex> lock(consoleMutex);
        qInfo() << "[Server] Listening on port" << port << "...\n";
    }
    return true;
}
