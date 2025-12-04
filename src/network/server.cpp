#include "server.h"

#include <QDebug>
#include <mutex>
#include "network.h"
#include "protocol.h"

Server::Server(unsigned int port) : port(port) {
    game = std::make_unique<Game>(1);
    LOG("[Server] instance created on port %d", port);
}

Server::~Server() {
    LOG("[Server] Destructor called, stopping server");
    stop();
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

void Server::start(bool inBackground) {
    if (serverRunning) {
      LOG("[Server] Already running");
      return;
    }

    serverRunning = true;
    if (inBackground) {
        lobbyThread = std::thread(&Server::listenForClients, this);
    } else {
        listenForClients();
    }
}

void Server::start_game() {
  if (!serverRunning) {
    LOG("Start server before running game");
    return;
  }
  if (gameRunning) {
    LOG("[Server] Game already running");
    return;
  }
  gameRunning = true;
  game->start();
  gameThread = std::thread(&Server::gameLoop, this);
}

void Server::stop() {
    if (!serverRunning) return;
    broadcastServerShutdown();

    serverRunning = false;
    gameRunning = false;
    game->stop();

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto& client : clientThreads) {
            client->running = false; // signal client handler to exit
        }
    }

    if (serverSocket != INVALID_SOCKET) {
      ::shutdownSocket(serverSocket);
      closeSocket(serverSocket);
      serverSocket = INVALID_SOCKET;
    }

    if (gameThread.joinable()) gameThread.join();
    if (lobbyThread.joinable()) lobbyThread.join();

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto& client: clientThreads) {
          if (client->socket != INVALID_SOCKET) {
            ::shutdownSocket(client->socket);
            closeSocket(client->socket);
            client->socket = INVALID_SOCKET;
          }
        }
    }

    // clear all the threads
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        int i = 0;
        for (auto& client : clientThreads) {
          if (client->thread.joinable()) {
            client->thread.join();
          }
        }
        clientThreads.clear();
    }

    cleanupSockets();
    LOG("[Server] Server has stopped cleanly.")
}

void Server::listenForClients() {
    while (serverRunning) {
        cleanFinishedClientThreads();
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            if (clientThreads.size() >= 8) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }
        }

        // Check with select() before accept()
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);

        // Set timeout to 1 second
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        // Check if the socket has data waiting (readable)
        // first argument is ignored in Windows, but needed for max_fd in Linux
        int activity = select(serverSocket + 1, &readfds, nullptr, nullptr, &timeout);

        if (activity < 0) {
            LOG("[Server] Select error");
            break; // Exit loop on system error
        }

        if (activity == 0) {
            // Timeout occurred (1 second passed), loop back to check serverRunning
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
            LOG("[Server] Accept failed");
            continue; // Don't exit, just try next connection
        }

        uint32_t newPlayerId = nextPlayerId++;
        LOG("[Server] New client connected, playerId: %d", newPlayerId);

        auto newClient = std::make_unique<ClientInfo>(clientSocket, newPlayerId);
        ClientInfo* clientRaw = newClient.get();

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            std::string name = "Player" + std::to_string(newPlayerId);
            game->addPlayer(name, newPlayerId % 2);

            newClient->thread = std::thread(&Server::handleClient, this, clientRaw);
            clientThreads.push_back(std::move(newClient));
        }

        assignPlayerId(clientRaw);
        if (game->playerCount() == 1) {
          std::string message = Protocol::serializeMarkClientHost();
          std::string framed = Protocol::frameMessage(message);
          Protocol::sendRaw(framed.c_str(), clientRaw->socket);
        }
        broadcastPlayerList();
    }
    LOG("[Server] Stopped listening for Clients.");
}

void Server::cleanFinishedClientThreads() {
    std::vector<std::unique_ptr<ClientInfo>> finishedClients;
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        auto it = clientThreads.begin();
        while (it != clientThreads.end()) {
            auto& client = *it;
            if (!client->running) {
                LOG("[Server] Cleaning up finished client thread for player %d", client->playerId);
                finishedClients.push_back(std::move(*it));
                it = clientThreads.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (auto& client : finishedClients) {
      if (client->thread.joinable()) client->thread.join();
    }
}

void Server::stopClient(ClientInfo* client) {
    if (!client) return;
    game->removePlayer(client->playerId);
    if (client->socket != INVALID_SOCKET) {
      closeSocket(client->socket);
      client->socket = INVALID_SOCKET;
    }
    client->running = false;
    if (serverRunning) broadcastPlayerList();
}

void Server::handleClient(ClientInfo* client) {
    char buffer[1024];
    // Loop to keep receiving data until the client disconnects
    while (client->running && serverRunning) {
        memset(buffer, 0, sizeof(buffer)); // Clear buffer

        int bytesReceived = recv(client->socket, buffer, sizeof(buffer)-1, 0);

        if (bytesReceived <= 0) break; // client disconnected

        buffer[bytesReceived] = '\0';
        client->receiveBuffer.append(buffer);

        // LOG("[Server] Received from player %d: %s", playerId, buffer);

        std::string message;
        while (Protocol::extractMessage(client->receiveBuffer, message)) {
            processClientMessage(message);
        }
    }

    LOG("[Server] Client handler exiting for player %d", client->playerId);
    stopClient(client);
}

void Server::processClientMessage(const std::string& message) {
    if (message.empty()) return;
    uint8_t messageType = static_cast<uint8_t>(message[0]);
    switch (messageType) {
        case Protocol::REQUEST_PLAYER_LIST:
            broadcastPlayerList();
            break;
        case Protocol::PLAYER_INPUT: {
            uint32_t playerId;
            float inputX, inputY;
            if (Protocol::deserializePlayerInput(message, playerId, inputX, inputY)) {
                game->queuePlayerInput(playerId, inputX, inputY);
            }
            break;
        case Protocol::REQUEST_START_GAME: {
              start_game();
              break;
            }
        default:
            LOG("[Server] Unknown message from client: %s", message.c_str());
            break;
        }
    }
}

void Server::gameLoop() {
    const int UPDATE_INTERVAL_MS = 1000 / 60; // 60 FPS
    auto previousTime = std::chrono::steady_clock::now();

    while (gameRunning && serverRunning) {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - previousTime).count();

        if (elapsedTime >= UPDATE_INTERVAL_MS) {
            game->update(static_cast<uint32_t>(elapsedTime));
            broadcastGameState();
            previousTime = currentTime;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    LOG("[Server] Game Loop ended");
}

void Server::broadcastServerShutdown() {
    if (!serverRunning) return;
    std::string message = Protocol::serializeServerShutdown();
    std::string framed = Protocol::frameMessage(message);
    notifyAll(framed.c_str());
}

void Server::broadcastGameState() {
    if (!serverRunning) return;
    GameState state = game->getGameState();
    std::string message = Protocol::serializeGameState(state);
    std::string framed = Protocol::frameMessage(message);
    notifyAll(framed.c_str());
}

void Server::broadcastPlayerList() {
    if (!serverRunning) return;
    std::vector<std::string> playerNames;
    for (auto& [id, player]: game->getGameState().players) {
      playerNames.push_back(player.name);
    }
    std::string message = Protocol::serializePlayerList(playerNames);
    std::string framed = Protocol::frameMessage(message);
    notifyAll(framed.c_str());
}

void Server::assignPlayerId(ClientInfo* client) {
    if (client->running && client->socket != INVALID_SOCKET) {
      std::string msg = Protocol::serializePlayerJoined(client->playerId);
      std::string framed = Protocol::frameMessage(msg);
      Protocol::sendRaw(framed.c_str(), client->socket);
    }
}

void Server::notifyAll(const char* msg, SOCKET avoid) {
    if (!serverRunning) return;
    std::vector<std::pair<SOCKET, std::atomic<bool>*>> sockets;
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto& client : clientThreads) {
            if (client->running && client->socket != INVALID_SOCKET && client->socket != avoid) {
                sockets.push_back({client->socket, &client->running});
            }
        }
    }
    for (auto& [sock, running] : sockets) {
        Protocol::sendRaw(msg, sock);
    }
}

void Server::notifyAllOthers(const char* msg, SOCKET socket) {
    notifyAll(msg, socket);
}
