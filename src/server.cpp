#include "server.h"

#include <QDebug>
#include <mutex>
#include "network.h"

Server::Server(unsigned int port) : port(port) {
  game = std::make_unique<Game>(1);
  LOG("[Server] instance created on port %d", port);
}

Server::~Server() { stop(); }

void Server::run(bool inBackground) {
    isRunning = true;
    gameRunning = true;
    gameThread = std::thread(&Server::runGameLoop, this);
    if (inBackground) {
        lobbyThread = std::thread(&Server::listenForClients, this);
    } else {
        listenForClients();
    }
}

void Server::runGameLoop() {
    const int UPDATE_INTERVAL_MS = 1000 / 60; // 60 FPS
    auto previousTime = std::chrono::steady_clock::now();

    while (gameRunning) {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - previousTime).count();

        if (elapsedTime >= UPDATE_INTERVAL_MS) {
            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                game->update(static_cast<uint32_t>(elapsedTime));
            }
            broadcastGameState();
            previousTime = currentTime;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void Server::broadcastGameState() {
    GameState state;
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        state = game->getGameState();
    }
    std::string message = "GAME_STATE:";
    for (const auto& playerPair : state.players) {
        const PlayerState& player = playerPair;
        message += std::to_string(player.playerId) + "," +
                   std::to_string(static_cast<int>(player.position.x())) + "," +
                   std::to_string(static_cast<int>(player.position.y())) + "," +
                   std::to_string(player.team) + "|";
    }
    if (!state.players.empty()) {
        message.pop_back(); // remove last '|'
    }
    std::string framedMessage = std::to_string(message.length()) + ":" + message;
    std::lock_guard<std::mutex> lock(clientsMutex);
    notifyAll(framedMessage.c_str());
}

void Server::stop() {
    LOG("[Server] Destructor called, stopping server");
    isRunning = false;
    gameRunning = false;

    if (serverSocket != INVALID_SOCKET) {
      closeSocket(serverSocket);
      serverSocket = INVALID_SOCKET;
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto& client : clientThreads) {
            if (client->socket != INVALID_SOCKET) {
                closeSocket(client->socket);
                client->socket = INVALID_SOCKET;
            }
            client->finished = true;
        }
    }

    if (gameThread.joinable()) {
      gameThread.join();
    }

    if (lobbyThread.joinable()) {
        lobbyThread.join();
    }

    cleanupClientThreads();
    cleanupSockets();
}

void Server::listenForClients() {
    while (isRunning) {
        cleanupFinishedClients();
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
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
        LOG("[Server] New client connected, playerId: %d", newPlayerId);

        auto newClient = std::make_unique<ClientInfo>(clientSocket, newPlayerId);

        // We must capture the pointer to the 'finished' flag to pass it to the thread
        // We use a raw pointer here because the unique_ptr in the vector owns the memory
        std::atomic<bool>* finishedFlag = &newClient->finished;

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            newClient->thread = std::thread(&Server::handleClient, this, clientSocket, newPlayerId, finishedFlag);
            clientThreads.push_back(std::move(newClient));
        }


        broadcastPlayerList();
    }
}

void Server::cleanupClientThreads() {
    std::vector<std::thread> threadsToJoin;
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        LOG("[Server] Cleaning up %zu client threads", clientThreads.size());
        for (auto& client : clientThreads) {
            client->finished = true;
            if (client->thread.joinable()) {
                threadsToJoin.push_back(std::move(client->thread));
            }
        }

        clientThreads.clear();
    }
    for (auto& t : threadsToJoin) {
        LOG("[Server] Joining a client thread...");
        t.join();
    }
}

void Server::cleanupFinishedClients() {
    std::lock_guard<std::mutex> lock(clientsMutex);

    auto it = clientThreads.begin();
    while (it != clientThreads.end()) {
        if ((*it)->finished) {
            LOG("[Server] Cleaning up finished client thread for player %d", (*it)->playerId);

            if ((*it)->thread.joinable()) {
                (*it)->thread.join();
            }

            it = clientThreads.erase(it);
        } else {
            ++it;
        }
    }
}

void Server::handleClient(SOCKET clientSocket, uint32_t playerId, std::atomic<bool>* finishedFlag) {
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        std::string name = "Player" + std::to_string(playerId);
        game->addPlayer(QString::fromStdString(name), playerId % 2);
    }

    // std::string playerIdMsg = "PLAYER_ID:" + std::to_string(playerId);
    // std::string framedIdMsg = std::to_string(playerIdMsg.length()) + ":" + playerIdMsg;
    // sendMessage(framedIdMsg.c_str(), clientSocket);

    broadcastPlayerList(clientSocket);

    char buffer[1024];
    // Loop to keep receiving data until the client disconnects
    while (!finishedFlag->load() && isRunning) {
        memset(buffer, 0, sizeof(buffer)); // Clear buffer

        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer)-1, 0);

        if (bytesReceived <= 0) break; // client disconnected

        buffer[bytesReceived] = '\0';

        // LOG("[Server] Received from player %d: %s", playerId, buffer);

        if (strstr(buffer, "REQUEST_PLAYER_LIST")) {
            broadcastPlayerList(clientSocket); // send to that client
        }
        else if (strstr(buffer, "PLAYER_INPUT:")) {
            // format: "##:PLAYER_INPUT:x,y"
            char* data = buffer;
            while (isdigit(*data)) data++;
            if (*data == ':') data++;
            float x, y;
            if (sscanf(data, "PLAYER_INPUT:%f,%f", &x, &y) == 2) {
                // LOG("[Server] Received input from player %d: (%f, %f)", playerId, x, y);
                game->updatePlayerInput(playerId, QVector2D(x, y));
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        game->removePlayer(playerId);
    }

    LOG("[Server] Client handler exiting for player %d", playerId);
    closeSocket(clientSocket);
    *finishedFlag = true;
    broadcastPlayerList();
}

void Server::broadcastPlayerList(SOCKET client) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    std::string message = "PLAYER_LIST:";
    for (int i = 0; i <  clientThreads.size(); ++i) {
        if (!clientThreads[i]->finished) {
            message += "Player " + std::to_string(clientThreads[i]->playerId);
            if (i != clientThreads.size() - 1) {
                message += ",";
            }
        }
    }
    std::string framedMessage = std::to_string(message.length()) + ":" + message;
    if (client == INVALID_SOCKET) {
        notifyAll(framedMessage.c_str());
    } else {
        notifyAllOthers(framedMessage.c_str(), client);
    }
}

void Server::notifyAll(const char* msg) {
    for (auto& client : this->clientThreads) {
        if (!client->finished && client->socket != INVALID_SOCKET) {
            sendMessage(msg, client->socket);
        }
    }
}

void Server::notifyAllOthers(const char* msg, SOCKET client) {
    for (auto& other : this->clientThreads) {
        if (other->finished || other->socket == INVALID_SOCKET || other->socket == client) {
            continue;
        }
        sendMessage(msg, other->socket);
    }
}

bool Server::sendMessage(const char* msg, SOCKET client) {
    int totalSent = 0;
    int msgLength = strlen(msg);
    while (totalSent < msgLength && isRunning) {
        int bytesSent = send(client, msg + totalSent, msgLength - totalSent, 0);
        if (bytesSent <= 0) {
            LOG("[Server] Failed to send message to socket %d", client);
            return false;
        }
        totalSent += bytesSent;
    }
    if (totalSent == msgLength) {
        // LOG("[Server] Successfully sent %d bytes to socket %d", totalSent, client);
        return true;
    }
    return false;
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
