#ifndef GAME_H
#define GAME_H

#include <QObject>
#include <QTimer>
#include <QVector2D>
#include <QMap>

struct PlayerState { // network side representation
    uint32_t playerId;
    QString name;
    QVector2D position, velocity, acceleration;
    uint8_t team; // 0 = red, 1 = blue
    bool connected;
};

struct GameState {
    uint32_t lobbyId;
    uint8_t mapId, redScore, blueScore;
    QMap<uint32_t, PlayerState> players;
    PlayerState* getPlayer(uint32_t playerId) {
        auto it = players.find(playerId);
        return (it != players.end()) ? &it.value() : nullptr;
    }
};

class Game : public QObject {
    Q_OBJECT
public:
    explicit Game(uint32_t lobbyId, QObject* parent = nullptr);
    ~Game();

    void startGame();
    void stopGame();
    void update(uint32_t deltaTime);

    // player management
    uint32_t addPlayer(const QString& name, uint8_t team);
    void removePlayer(uint32_t playerId);
    void updatePlayerInput(uint32_t playerId, const QVector2D& input);
    bool changePlayerTeam(uint32_t playerId, uint8_t newTeam);

    // getters
    GameState getGameState() const { return currentState; }
    PlayerState* getPlayerState(uint32_t playerId);
    bool isRunning() const { return gameRunning; }

signals:
    void gameStateUpdated(const GameState& state);

private:
    void applyPhysics(PlayerState& player, uint32_t deltaTime);
    void checkBoundaries(PlayerState& player);
    void resolveCollisions();

    GameState currentState;
    QTimer* gameTimer;
    bool gameRunning;
    const uint32_t updatesPerSecond = 60;

    const float playerRadius = 15.0f;
    const float playerAcceleration = 0.5f;
    const float playerMaxSpeed = 3.0f;
    const float playerFriction = 0.99f;
    const float playerRestitution = 0.2f;
    const QVector2D arenaSize = QVector2D(800, 600);
};

#endif // GAME_H
