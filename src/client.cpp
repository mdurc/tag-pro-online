#include "client.h"
#include "network.h"

Client::Client() {
    createSocket();
}

Client::~Client() {
    disconnect();
}

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

    LOG("[Client] Connected successfully.");

    isRunning = true;
    receivingThread = std::thread(&Client::receiveLoop, this);
    emit connectedSuccessfully();
}

void Client::disconnect() {
    LOG("[Client] Disconnecting from server.");
    isRunning = false;
    if (clientSocket != INVALID_SOCKET) {
        closeSocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }

    if (receivingThread.joinable()) {
        receivingThread.join();
    }

    cleanupSockets();
    // disconnectedFromServer is emitted via constructor/by caller
    // avoids infinite loop when startscreen calls client->disconnect
}

void Client::receiveLoop() {
    char buffer[1024];
    while (isRunning) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            receiveBuffer += buffer;
            // LOG("[Client] Received %d bytes, buffer size: %zu", bytesReceived, receiveBuffer.size());

            std::string completeMessage;
            while (receiveMessage(completeMessage)) {
                // LOG("[Client] Processing message: %s", completeMessage.c_str());
                if (completeMessage.find("PLAYER_LIST:") == 0) {
                    QString playerListStr = QString::fromStdString(completeMessage.substr(12));
                    QStringList newPlayers = playerListStr.split(',', Qt::SkipEmptyParts);
                    emit playerListUpdated(newPlayers);
                }
                else if (completeMessage.find("GAME_STATE:") == 0) {
                    emit gameMessageReceived(QString::fromStdString(completeMessage));
                }
                // else if (completeMessage.find("PLAYER_ID:") == 0) {
                //     uint32_t playerId = std::stoi(completeMessage.substr(10));
                //     // TODO is this needed?
                // }
            }
        } else {
            LOG("[Client] Server disconnected");
            isRunning = false;
            emit disconnectedFromServer();
            break;
        }
    }
    LOG("[Client] Receive loop finished.");
}

void Client::requestPlayerList() {
    sendMessage("REQUEST_PLAYER_LIST");
}

void Client::sendMessage(const char * msg) {
    if (!isRunning || clientSocket == INVALID_SOCKET) {
        LOG("[Client] Cannot send message - not connected");
        return;
    }

    std::string framedMsg = std::to_string(strlen(msg)) + ":" + msg;
    int totalSent = 0;
    int msgLength = framedMsg.length();

    while (totalSent < msgLength && isRunning) {
        int bytesSent = send(clientSocket, framedMsg.c_str() + totalSent, msgLength - totalSent, 0);
        if (bytesSent <= 0) {
            LOG("[Client] Failed to send message: %s", msg);
            isRunning = false;
            emit disconnectedFromServer();
            break;
        }
        totalSent += bytesSent;
    }

    if (totalSent == msgLength) {
        // LOG("[Client] Successfully sent %d bytes: %s", totalSent, msg);
    }
}

bool Client::receiveMessage(std::string& message) {
    // look for length prefix (format: "length:message")
    size_t colonPos = receiveBuffer.find(':');
    if (colonPos == std::string::npos) {
        return false;
    }

    // extract length
    std::string lengthStr = receiveBuffer.substr(0, colonPos);
    try {
        int messageLength = std::stoi(lengthStr);
        int totalMessageLength = colonPos + 1 + messageLength;

        if (receiveBuffer.length() >= totalMessageLength) {
            message = receiveBuffer.substr(colonPos + 1, messageLength);
            receiveBuffer.erase(0, totalMessageLength);
            return true;
        }
    } catch (const std::exception& e) {
        LOG("[Client] Invalid message length format: %s", lengthStr.c_str());
        receiveBuffer.clear();
    }

    return false; // no complete msg yet
}
