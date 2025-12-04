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
private:
    void applyPhysics(PlayerState& player, float deltaTimeSec);
    void checkBoundaries(PlayerState& player);
    void resolveCollisions();
    void updatePlayerVelocity(PlayerState& player, float inputX, float inputY, float deltaTimeSec);

    mutable std::mutex stateMutex;
    GameState currentState;

    std::queue<PlayerInput> inputQueue;
    mutable std::mutex inputQueueMutex;

    const float playerRadius = 15.0f;
    const float playerAcceleration = 100.0f;
    const float playerMaxSpeed = 1000.0f;
    const float playerFriction = 0.99f;
    const float playerRestitution = 0.2f;

    const float arenaWidth = 800.0f;
    const float arenaHeight = 600.0f;
};

#endif // GAME_H
