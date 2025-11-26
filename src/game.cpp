#include "game.h"

Game::Game(uint fps) {
    this->interval = fps;
    timer = new QTimer(this);
}

void Game::init() {
    timer->setInterval(this->interval);
    connect(timer, &QTimer::timeout, this, &Game::update);
}

void Game::update() {
    for (Player& player : redTeam) {
        player.update(this->interval);
    }

    for (Player& player : blueTeam) {
        player.update(this->interval);
    }
}
