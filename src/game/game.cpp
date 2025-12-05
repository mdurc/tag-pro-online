#include "game.h"

#include <QDebug>
#include "game_state.h"

#define GAME_LOG(fmt, ...) \
  { qInfo().noquote() << "[GAME] " << QString().asprintf(fmt, ##__VA_ARGS__); }

Game::Game(uint32_t lobbyId) {
  currentState.lobbyId = lobbyId;
  currentState.mapId = 0;
  currentState.redScore = 0;
  currentState.blueScore = 0;
}
void Game::start() {
    GAME_LOG("Started lobby %d", currentState.lobbyId);
    currentState.mapId = currentState.redScore = currentState.blueScore = 0;
    currentState.redFlag = currentState.blueFlag = 0;
}

void Game::stop() {
    GAME_LOG("Stopped lobby %d", currentState.lobbyId);
}

float Game::getTeamSpawnXLocation(uint8_t team) {
    if (team == 0) {
        return 100.0f;
    } else {
        return arenaWidth - 100.0f;
    }
}

uint32_t Game::addPlayer(const std::string& name, uint8_t team) {
    std::lock_guard<std::mutex> lock(stateMutex);
    uint32_t playerId = getNextPlayerId();

    PlayerState player(playerId, name, team);

    player.x = getTeamSpawnXLocation(team);
    player.y = arenaHeight / 2.0f;

    currentState.players[playerId] = player;
    GAME_LOG("%s (id: %d) added to team %d", name.c_str(), playerId, team);
    return playerId;
}

bool Game::removePlayer(uint32_t playerId) {
    std::lock_guard<std::mutex> lock(stateMutex);
    GAME_LOG("%s removed from game", currentState.players[playerId].name.c_str());
    bool res = currentState.players.erase(playerId) > 0;
    if (res && currentState.players.empty()) {
      // restart score
      currentState.redScore = currentState.blueScore = 0;
      currentState.redFlag = currentState.blueFlag = 0;
    }
    return res;
}

void Game::pop(PlayerState& player) {
    player.hasFlag = false;
    player.velocityX = 0;
    player.velocityY = 0;
    player.x = getTeamSpawnXLocation(player.team);
    player.y = arenaHeight / 2.0f;
    if (player.team == REDTEAM) {
        currentState.blueFlag = 0;
    } else {
        currentState.redFlag = 0;
    }
}

void Game::queuePlayerInput(uint32_t playerId, float inputX, float inputY) {
    std::lock_guard<std::mutex> lock(inputQueueMutex);
    inputQueue.push({playerId, inputX, inputY});
}

// unused
bool Game::setPlayerTeam(uint32_t playerId, uint8_t team) {
    std::lock_guard<std::mutex> lock(stateMutex);
    if (auto* player = currentState.getPlayer(playerId)) {
        player->team = team;
        return true;
    }
    return false;
}

PlayerState* Game::getPlayerState(uint32_t playerId) {
    // caller locks
    return currentState.getPlayer(playerId);
}

size_t Game::getPlayerCount() const { return currentState.players.size(); }
int32_t Game::getNextPlayerId() const {
  int32_t next = 1;
  while (currentState.players.find(next) != currentState.players.end()) {
    ++next;
  }
  return next;
}

void Game::update(uint32_t deltaTimeMs) {
    float deltaTimeSec = deltaTimeMs / 1000.0f;
    for (auto& pair : currentState.players) {
        PlayerState& player = pair.second;

        if (player.respawnTimer > 0) {
            player.respawnTimer -= std::min(player.respawnTimer, deltaTimeMs);
        }
    }
    {
        std::lock_guard<std::mutex> lock(inputQueueMutex);
        while (!inputQueue.empty()) {
            const auto& input = inputQueue.front();
            if (auto* player = currentState.getPlayer(input.playerId)) {
                if (player->respawnTimer != 0) {
                    continue;
                }
                updatePlayerVelocity(*player, input.inputX, input.inputY, deltaTimeSec);
            }
            inputQueue.pop();
        }
    }

    std::lock_guard<std::mutex> lock(stateMutex);
    for (auto& [id, player] : currentState.players) {
        if (!player.connected) continue;
        applyPhysics(player, deltaTimeSec);
        checkBoundaries(player);
    }
    resolveCollisions();
}

void Game::updatePlayerVelocity(PlayerState& player, float inputX, float inputY, float deltaTimeSec) {
    float length = std::sqrt(inputX * inputX + inputY * inputY);
    if (length > 1.0f) {
      inputX /= length;
      inputY /= length;
    }

    player.velocityX += inputX * playerAcceleration * deltaTimeSec;
    player.velocityY += inputY * playerAcceleration * deltaTimeSec;

    float speed = std::sqrt(player.velocityX * player.velocityX +
                            player.velocityY * player.velocityY);
    if (speed > playerMaxSpeed) {
        player.velocityX = (player.velocityX / speed) * playerMaxSpeed;
        player.velocityY = (player.velocityY / speed) * playerMaxSpeed;
    }
}

void Game::applyPhysics(PlayerState& player, float deltaTimeSec) {
    player.velocityX *= std::pow(playerFriction, deltaTimeSec);
    player.velocityY *= std::pow(playerFriction, deltaTimeSec);

    player.x += player.velocityX * deltaTimeSec;
    player.y += player.velocityY * deltaTimeSec;

    if (std::abs(player.velocityX) < 0.01f) player.velocityX = 0;
    if (std::abs(player.velocityY) < 0.01f) player.velocityY = 0;
}

void Game::checkBoundaries(PlayerState& player) {
    // Left Wall
    if (player.x < playerRadius) {
        player.x = playerRadius;

        if (player.velocityX < 0) {
            player.velocityX = -player.velocityX * wallRestitution;
        }
    } 
    // Right Wall
    else if (player.x > arenaWidth - playerRadius) {
        player.x = arenaWidth - playerRadius;

        if (player.velocityX > 0) {
            player.velocityX = -player.velocityX * wallRestitution;
        }
    }

    // Top Wall
    if (player.y < playerRadius) {
        player.y = playerRadius;

        if (player.velocityY < 0) {
            player.velocityY = -player.velocityY * wallRestitution;
        }
    } 
    // Bottom Wall
    else if (player.y > arenaHeight - playerRadius) {
        player.y = arenaHeight - playerRadius;

        if (player.velocityY > 0) {
            player.velocityY = -player.velocityY * wallRestitution;
        }
    }
}

void Game::resolveCollisions() {

  std::vector<uint32_t> playerIds;
  for (const std::pair<uint32_t, PlayerState>& pair : currentState.players) {
    playerIds.push_back(pair.first);
  }
  for (int i = 0; i < playerIds.size(); ++i) {
    PlayerState* player1 = getPlayerState(playerIds[i]);
    if (!player1) continue;
    if (player1->respawnTimer != 0) continue;

    if (player1->team == REDTEAM && currentState.blueFlag == 0) {
        if (checkCollision(player1->x, player1->y, blueFlagX, blueFlagY)) {
            player1->hasFlag = true;
            currentState.blueFlag = player1->id;
            GAME_LOG("%s:%d has picked up the flag!", player1->name.c_str(), player1->id);
        }
    } else if (player1->team == BLUETEAM && currentState.redFlag == 0) {
        if (checkCollision(player1->x, player1->y, redFlagX, redFlagY)) {
            player1->hasFlag = true;
            currentState.redFlag = player1->id;
            GAME_LOG("%s:%d has picked up the red flag!", player1->name.c_str(), player1->id);
        }
    }

    if (player1->hasFlag) {
        if (player1->team == REDTEAM && currentState.redFlag == 0) {
            if (checkCollision(player1->x, player1->y, redFlagX, redFlagY)) {
                player1->hasFlag = false;
                currentState.blueFlag = 0;
                currentState.redScore++;
                GAME_LOG("%s:%d has scored for the red team!", player1->name.c_str(), player1->id);
            }
        } else if (player1->team == BLUETEAM && currentState.blueFlag == 0) {
            if (checkCollision(player1->x, player1->y, blueFlagX, blueFlagY)) {
                player1->hasFlag = false;
                currentState.redFlag = 0;
                currentState.blueScore++;
                GAME_LOG("%s:%d has scored for the blue team!", player1->name.c_str(), player1->id);
            }
        }
    }

    for (int j = i + 1; j < playerIds.size(); ++j) {
      PlayerState* player2 = getPlayerState(playerIds[j]);
      if (!player2) continue;
      if (player2->respawnTimer != 0) continue;

      if (checkCollision(player1->x, player1->y, player2->x, player2->y)) {
        float dx = player1->x - player2->x;
        float dy = player1->y - player2->y;
        float distance = std::sqrt(dx * dx + dy * dy);
        float nx = dx / distance;
        float ny = dy / distance;
        // Shift position to no longer be colliding;
        float overlap = playerRadius * 2 - distance;
        float separation = overlap * 0.5f;

        player1->x += nx * separation;
        player1->y += ny * separation;
        player2->x -= nx * separation;
        player2->y -= ny * separation;

        float rvx = player1->velocityX - player2->velocityX;
        float rvy = player1->velocityY - player2->velocityY;

        float velAlongNormal = rvx * nx + rvy * ny;

        if (velAlongNormal > 0.0f)
          continue;

        float jImpulse = -(1.0f + playerRestitution) * velAlongNormal;
        jImpulse /= 2.0f;

        float impulseX = nx * jImpulse;
        float impulseY = ny * jImpulse;

        player1->velocityX += impulseX;
        player1->velocityY += impulseY;

        player2->velocityX -= impulseX;
        player2->velocityY -= impulseY;
        if (player2->hasFlag && player1->team != player2->team) {
            pop(*player2);
            GAME_LOG("%s was popped", player2->name.c_str());
        }
        if (player1->hasFlag && player1->team != player2->team) {
            pop(*player1);
            GAME_LOG("%s was popped", player1->name.c_str());
        }
      }
    }
  }
}

bool Game::checkCollision(float x1, float y1, float x2, float y2) {
    float dx = x2-x1;
    float dy = y2-y1;
    float distance = std::sqrt(dx * dx + dy * dy);
    return distance < playerRadius * 2 && distance > 0;
}
