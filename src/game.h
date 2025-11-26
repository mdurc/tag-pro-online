#ifndef GAME_H
#define GAME_H

#include <QTimer>
#include <QObject>
#include <vector>
#include "player.h"

class Game : public QObject
{
public:
    Game(uint fps = 60);

    void init();
private:
    void update();

    std::vector<Player> redTeam;
    std::vector<Player> blueTeam;

    unsigned int interval;
    QTimer *timer = nullptr;
};

#endif // GAME_H
