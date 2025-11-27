#include "game.h"

Game::Game(uint64_t fps) {
    this->interval = fps / 1000;
    timer = new QTimer(this);
}

void Game::init() {
    timer->setInterval(this->interval);
    connect(timer, &QTimer::timeout, this, &Game::update);
}

void Game::startGame() {
    init();
    // TODO: Load map data and place players in correct locations
}

void Game::endGame() {
    delete timer;
}

GameState Game::getGameState() {
    return GameState{};
}

void Game::updatePlayerInput() {
    // TODO: Decide how to id players
}

void Game::kickPlayer() {
    // TODO: Decide how to id players
}

bool Game::changePlayerTeam() {
    // TODO: Decode how to id players
    return false;
}

bool Game::changePlayerName() {
    // TODO: Decide how to id players
    return false;
}


void Game::update() {
    for (Player& player : redTeam) {
        player.update(this->interval);
    }

    for (Player& player : blueTeam) {
        player.update(this->interval);
    }

    // TODO: Check collisions
}
