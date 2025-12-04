#include "client.h"
#include "network.h"
#include "protocol.h"

Client::Client(): clientSocket(INVALID_SOCKET) {
    createSocket();
}

Client::~Client() { disconnect(); }

void Client::createSocket() {
    if (!initSockets()) {
        LOG("[Client] Socket initialization failed.");
        return;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        LOG("[Client] Error creating socket.");
        cleanupSockets();
        return;
    }
}

void Client::connect(int port, const char * ip) {
    // Might be connected to a different server;
    // In this case, the user of this class should create a
    // new Client object.
    if (isRunning) {
        LOG("[Client] Connection failed. Is this already connected to another server?");
        return;
    }

    // Setup socket information
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serverAddr.sin_addr) <= 0) {
        LOG("[Client] Invalid IP address: %s", ip);
        return;
    }

    if (::connect(this->clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        LOG("[Client] Connection failed to %s:%d", ip, port);
        closeSocket(this->clientSocket);
        cleanupSockets();
        return;
    }

    LOG("[Client] Connected successfully to %s:%d", ip, port);

    {
      std::lock_guard<std::mutex> lock(callbackMutex);
      if (connectionCallback) {
        connectionCallback(true);
      }
    }
    isRunning = true;
    receivingThread = std::thread(&Client::receiveLoop, this);
}

void Client::disconnect() {
    LOG("[Client] Disconnecting from server.");
    isRunning = false;

    if (clientSocket != INVALID_SOCKET) {
        shutdownSocket(clientSocket);
        closeSocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }

    LOG("[Client] Waiting for receivingThread...");
    if (receivingThread.joinable()) {
        if (std::this_thread::get_id() != receivingThread.get_id()) {
            receivingThread.join();
        } else {
            receivingThread.detach(); 
        }
    }
    LOG("[Client] receivingThread has joined.");

    cleanupSockets();
    LOG("[Client] Disconnected cleanly.");
}

void Client::receiveLoop() {
    char buffer[1024];
    while (isRunning) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';

            {
                std::lock_guard<std::mutex> lock(bufferMutex);
                receiveBuffer.append(buffer);
            }
            // LOG("[Client] Received %d bytes, buffer size: %zu", bytesReceived, receiveBuffer.size());

            processIncomingData();
        } else {
            if (isRunning) {
              LOG("[Client] Server disconnected");
              {
                std::lock_guard<std::mutex> lock(callbackMutex);
                if (connectionCallback) {
                  connectionCallback(false);
                }
              }
            }
            break;
        }
    }
    LOG("[Client] Receive loop finished.");
}

void Client::processIncomingData() {
    std::lock_guard<std::mutex> lock(bufferMutex);
    std::string message;
    while (Protocol::extractMessage(receiveBuffer, message)) {
        // LOG("[Client] Processing message: %s", message.c_str());

        uint32_t assignedId;
        if (Protocol::deserializeServerShutdown(message)) {
            disconnect();
            break;
        } else if (Protocol::deserializePlayerJoined(message, assignedId)) {
            playerId = assignedId;
        } else {
          // relay to GUI callback
          std::lock_guard<std::mutex> lock(callbackMutex);
          if (messageCallback) {
              messageCallback(message);
          }
        }
    }
}

void Client::sendMessage(const std::string& message) {
    if (!isRunning || clientSocket == INVALID_SOCKET) {
        LOG("[Client] Cannot send message - not connected");
        return;
    }

    std::string framed = Protocol::frameMessage(message);
    Protocol::sendRaw(framed.c_str(), clientSocket);
}

void Client::sendPlayerInput(float x, float y) {
    std::string message = Protocol::serializePlayerInput(playerId, x, y);
    sendMessage(message);
}

void Client::setMessageCallback(MessageCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex);
    messageCallback = std::move(callback);
}

void Client::setConnectionCallback(ConnectionCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex);
    connectionCallback = std::move(callback);
}

void Client::clearCallbacks() {
    std::lock_guard<std::mutex> lock(callbackMutex);
    messageCallback = nullptr;
    connectionCallback = nullptr;
}
