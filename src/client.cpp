#include "client.h"

Client::Client() {
    createSocket();
}

Client::~Client() {
    {
        std::lock_guard<std::mutex> lock(consoleMutex);
        qInfo() << "[Client] Cleaning up.";
    }
    isRunning = false;

    closeSocket(this->clientSocket);
    cleanupSockets();
    if (receivingThread.joinable()) {
        receivingThread.join();
    }
}

void Client::createSocket() {
    if (!initSockets()) {
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            qWarning() << "[Client] Socket initialization failed.";
        }
        return;
    }

    this->clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            qWarning() << "[Client] Error creating socket.";
        }
        cleanupSockets();
        return;
    }
}

void Client::connect(int port, const char * ip) {
    // Might be connected to a different server;
    // In this case, the user of this class should create a
    // new Client object.
    if (isRunning) {
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            qWarning() << "[Client] Connection failed. Is this already connected to another server?";
        }
        return;
    }

    // Setup socket information
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    inet_pton(AF_INET, ip, &serverAddr.sin_addr);

    if (::connect(this->clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            qWarning() << "[Client] Connection failed.";
        }
        closeSocket(this->clientSocket);
        cleanupSockets();
        return;
    }

    {
        std::lock_guard<std::mutex> lock(consoleMutex);
        qInfo() << "[Client] Connected successfully.";
    }

    isRunning = true;
    receivingThread = std::thread(&Client::receiveLoop, this);
}

void Client::receiveLoop() {
    char buffer[1024];
    while (isRunning) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            // TODO: Handle server broadcasts
        } else {
            {
                std::lock_guard<std::mutex> lock(consoleMutex);
                qInfo() << "[Client] Server disconnected.";
            }
            isRunning = false;
            break;
        }
    }
}

void Client::requestStartGame() {
    sendmsg("Start Game");
}

void Client::sendmsg(char * msg) {
    send(this->clientSocket, msg, strlen(msg), 0);
    {
        std::lock_guard<std::mutex> lock(consoleMutex);
        qInfo() << "[Client] Sent message to server.";
    }
}
