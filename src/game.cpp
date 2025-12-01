#include "game.h"

#include <QDebug>

#define GAME_LOG(fmt, ...) \
  { qInfo().noquote() << "[GAME] " << QString().asprintf(fmt, ##__VA_ARGS__); }

const float playerRadius = 15.0f;

Game::Game(uint32_t lobbyId, QObject* parent)
    : QObject(parent), gameRunning(false) {
  currentState.lobbyId = lobbyId;
  currentState.mapId = 0;
  currentState.redScore = 0;
  currentState.blueScore = 0;

  gameTimer = new QTimer(this);
  connect(gameTimer, &QTimer::timeout, this,
          [this]() { update(1000 / updatesPerSecond); });
}

Game::~Game() { stopGame(); }

void Game::startGame() {
  if (gameRunning) return;

  gameRunning = true;
  gameTimer->start(1000 / updatesPerSecond);
  GAME_LOG("Started lobby %d", currentState.lobbyId);
}

void Game::stopGame() {
  gameRunning = false;
  gameTimer->stop();
  GAME_LOG("Stopped lobby %d", currentState.lobbyId);
}

uint32_t Game::addPlayer(const QString& name, uint8_t team) {
  static uint32_t nextPlayerId = 1;
  uint32_t playerId = nextPlayerId++;

  PlayerState newPlayer;
  newPlayer.playerId = playerId;
  newPlayer.name = name;
  newPlayer.team = team;
  newPlayer.connected = true;

  // set initial position based on team
  if (team == 0) {
    newPlayer.position = QVector2D(100, arenaSize.y() / 2);
  } else {
    newPlayer.position = QVector2D(arenaSize.x() - 100, arenaSize.y() / 2);
  }

  newPlayer.velocity = QVector2D(0, 0);
  currentState.players[playerId] = newPlayer;

  GAME_LOG("Player %s (id: %d) added to team %d", name.toLatin1().data(), playerId, team);
  return playerId;
}

void Game::removePlayer(uint32_t playerId) {
  auto it = currentState.players.find(playerId);
  if (it != currentState.players.end()) {
    GAME_LOG("Player %s removed from game", it->name.toLatin1().data());
    currentState.players.erase(it);
  }
}

void Game::updatePlayerInput(uint32_t playerId, const QVector2D& input) {
  PlayerState* player = getPlayerState(playerId);
  if (player && player->connected) {
    player->velocity = input * playerMaxSpeed;
  }
}

bool Game::changePlayerTeam(uint32_t playerId, uint8_t newTeam) {
  PlayerState* player = getPlayerState(playerId);
  if (player && player->team != newTeam) {
    player->team = newTeam;
    return true;
  }
  return false;
}

PlayerState* Game::getPlayerState(uint32_t playerId) {
  return currentState.getPlayer(playerId);
}

void Game::update(uint32_t deltaTime) {
  // update all players
  for (auto& pair : currentState.players) {
    PlayerState& player = pair;
    if (player.connected) {
      applyPhysics(player, deltaTime);
      checkBoundaries(player);
    }
  }

  resolveCollisions();
  emit gameStateUpdated(currentState);
}

void Game::applyPhysics(PlayerState& player, uint32_t deltaTime) {
  float deltaTimeSec = static_cast<float>(deltaTime) / 1000.0f;

  player.position += player.velocity * deltaTimeSec * 60.0f;
  player.velocity *= playerFriction;

  if (player.velocity.length() < 0.1f) {
    player.velocity = QVector2D(0, 0);
  }
}

void Game::checkBoundaries(PlayerState& player) {
  // X boundaries
  if (player.position.x() < playerRadius) {
    player.position.setX(playerRadius);
    player.velocity.setX(0);
  } else if (player.position.x() > arenaSize.x() - playerRadius) {
    player.position.setX(arenaSize.x() - playerRadius);
    player.velocity.setX(0);
  }

  // Y boundaries
  if (player.position.y() < playerRadius) {
    player.position.setY(playerRadius);
    player.velocity.setY(0);
  } else if (player.position.y() > arenaSize.y() - playerRadius) {
    player.position.setY(arenaSize.y() - playerRadius);
    player.velocity.setY(0);
  }
}

void Game::resolveCollisions() {
  // simple collision detection
  const float minDistance = 2 * playerRadius;

  QList<uint32_t> playerIds = currentState.players.keys();
  for (int i = 0; i < playerIds.size(); ++i) {
    PlayerState* player1 = getPlayerState(playerIds[i]);
    if (!player1) continue;

    for (int j = i + 1; j < playerIds.size(); ++j) {
      PlayerState* player2 = getPlayerState(playerIds[j]);
      if (!player2) continue;

      QVector2D diff = player1->position - player2->position;
      float distance = diff.length();

      if (distance < minDistance && distance > 0) {
        QVector2D collisionNormal = diff.normalized();
        float overlap = minDistance - distance;

        player1->position += collisionNormal * overlap * 0.5f;
        player2->position -= collisionNormal * overlap * 0.5f;
      }
    }
  }
}
