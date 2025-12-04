#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <cstdint>
#include <string>
#include <unordered_map>

#define REDTEAM 0
#define BLUETEAM 1

struct PlayerState {
  uint32_t id;
  std::string name;
  float x, y;
  float velocityX, velocityY;
  uint8_t team;
  uint32_t respawnTimer;
  bool connected;
  bool hasFlag;

  PlayerState() : id(0), x(0), y(0), velocityX(0), velocityY(0), team(0), respawnTimer(0), connected(false), hasFlag(false) {}
  PlayerState(uint32_t id, const std::string& name, uint8_t team)
      : id(id), name(name), x(0), y(0), velocityX(0), velocityY(0), team(team), respawnTimer(0), connected(true), hasFlag(false) {}
};

struct GameState {
  uint32_t lobbyId = 0;
  uint8_t mapId = 0, redScore = 0, blueScore = 0;
  std::unordered_map<uint32_t, PlayerState> players;
  PlayerState* getPlayer(uint32_t playerId) {
    auto it = players.find(playerId);
    return it != players.end() ? &it->second : nullptr;
  }
  // Will be player's id when picked up, 0 when not
  uint32_t redFlag = 0, blueFlag = 0;
};

#endif // GAME_STATE_H
