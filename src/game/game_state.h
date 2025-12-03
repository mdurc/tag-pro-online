#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <cstdint>
#include <string>
#include <unordered_map>

struct PlayerState {
  uint32_t id;
  std::string name;
  float x, y;
  float velocityX, velocityY;
  uint8_t team;
  bool connected;

  PlayerState() : id(0), x(0), y(0), velocityX(0), velocityY(0), team(0), connected(false) {}
  PlayerState(uint32_t id, const std::string& name, uint8_t team)
      : id(id), name(name), x(0), y(0), velocityX(0), velocityY(0), team(team), connected(true) {}
};

struct GameState {
  uint32_t lobbyId;
  uint8_t mapId, redScore, blueScore;
  std::unordered_map<uint32_t, PlayerState> players;
  PlayerState* getPlayer(uint32_t playerId) {
    auto it = players.find(playerId);
    return it != players.end() ? &it->second : nullptr;
  }
};

#endif // GAME_STATE_H
