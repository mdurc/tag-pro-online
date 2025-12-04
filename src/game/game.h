#ifndef GAME_H
#define GAME_H

#include <mutex>
#include <queue>
#include "game_state.h"

// for queue of events
struct PlayerInput {
    uint32_t playerId;
    float inputX;
    float inputY;
};

class Game {
public:
    Game(uint32_t lobbyId);

    void start();
    void stop();

    // player management
    uint32_t addPlayer(const std::string& name, uint8_t team);
    bool removePlayer(uint32_t playerId);
    void queuePlayerInput(uint32_t playerId, float inputX, float inputY);
    bool setPlayerTeam(uint32_t playerId, uint8_t team); // unused

    // getters
    GameState getGameState() const { return currentState; }
    PlayerState* getPlayerState(uint32_t playerId);

    void update(uint32_t deltaTimeMs);

    constexpr static float playerRadius = 15.0f;
    constexpr static float playerAcceleration = 60.0f;
    constexpr static float playerMaxSpeed = 1000.0f;
    constexpr static float playerFriction = 0.98f;
    constexpr static float playerRestitution = 0.2f;
    constexpr static float wallRestitution = 0.15f;

    constexpr static float arenaWidth = 800.0f;
    constexpr static float arenaHeight = 600.0f;

    constexpr static float redFlagX = 100.0f;
    constexpr static float redFlagY = arenaHeight / 2.0f;
    constexpr static float blueFlagX = arenaWidth - 100.0f;
    constexpr static float blueFlagY = arenaHeight / 2.0f;
private:
    float getTeamSpawnXLocation(uint8_t team);
    bool tryFlagGrab(PlayerState& player);
    void applyPhysics(PlayerState& player, float deltaTimeSec);
    void checkBoundaries(PlayerState& player);
    void pop(PlayerState& player);
    bool checkCollision(float x1, float y1, float x2, float y2);
    void resolveCollisions();
    void updatePlayerVelocity(PlayerState& player, float inputX, float inputY, float deltaTimeSec);

    mutable std::mutex stateMutex;
    GameState currentState;

    std::queue<PlayerInput> inputQueue;
    mutable std::mutex inputQueueMutex;
};

#endif // GAME_H
