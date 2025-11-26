#include "server.h"

#include <QDebug>
#include <mutex>

std::mutex consoleMutex;

void Server::handleClient(SOCKET clientSocket, std::atomic<bool>* finishedFlag) {
    char buffer[1024];

    // Loop to keep receiving data until the client disconnects
    while (true) {
        memset(buffer, 0, sizeof(buffer)); // Clear buffer

        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            // If 0, client disconnected. If -1, error.
            std::lock_guard<std::mutex> lock(consoleMutex);
            qInfo() << "[Server] Client disconnected.\n";
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
    }

    // Clean up this specific client socket
    closeSocket(clientSocket);

    *finishedFlag = true;
}

void Server::run() {
    running = true;
    while (running) {

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
                    qDebug() << "Client disconnected. Cleaned up thread.";
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
            std::lock_guard<std::mutex> lock(consoleMutex);
            qDebug() << "Accept failed.";
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
    }
}

void Server::init() {
    if (!initSockets()) {
        qErrnoWarning("Socket initialization failed.\n", 200);
        return;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        qErrnoWarning("Error creating socket.\n", 200);
        cleanupSockets();
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        qErrnoWarning("Bind failed. Port might be in use.\n", 200);
        closeSocket(serverSocket);
        cleanupSockets();
        return;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        qErrnoWarning("Listen failed.\n", 200);
        closeSocket(serverSocket);
        cleanupSockets();
        return;
    }

    qInfo() << "Listening on port " << port << "...\n";
}

Server::Server(unsigned int port) {
    this->port = port;
    init();
}

Server::~Server() {
    running = false;
    closeSocket(serverSocket);
    cleanupSockets();

    for (auto &t : clientThreads) {
        if ((*t).joinable()) {
            (*t).join();
        }
    }
}
