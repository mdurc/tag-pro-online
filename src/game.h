#ifndef GAME_H
#define GAME_H

#include <QTimer>
#include <QObject>
#include <vector>
#include "player.h"

#pragma pack(push, 1)

struct GameState{
    uint32_t game_id;
    uint8_t map_id;
    uint8_t redScore;
    uint8_t blueScore;
    QVector2D redTeamPosition[4];
    QVector2D blueTeamPosition[4];
};
#pragma pack(pop)

class Game : public QObject
{
public:
    Game(uint64_t fps = 50);

    void init();

    void startGame();
    void endGame();

    GameState getGameState();

    bool isPlayerHost();
    void updatePlayerInput();
    void kickPlayer();
    bool changePlayerTeam();
    bool changePlayerName();
private:
    void update();

    std::vector<Player> redTeam;
    std::vector<Player> blueTeam;

    unsigned int interval;
    QTimer *timer = nullptr;
};

#endif // GAME_H
