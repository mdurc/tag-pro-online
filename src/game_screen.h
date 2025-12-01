#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QTimer>
#include "client.h"
#include "player.h"
#include "game.h"

class GameScreen : public QWidget
{
    Q_OBJECT
public:
    explicit GameScreen(Client* client, uint32_t localPlayerId, QWidget* parent = nullptr);
    ~GameScreen();

    void applyGameState(const GameState& state);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void sendPlayerInput();
    void processGameMessage(const QString& message);

private:
    void setupScene();
    void updatePlayerGraphics(uint32_t playerId, const PlayerState& state);
    void removePlayerGraphics(uint32_t playerId);
    QColor getTeamColor(uint8_t team);

    Client* client;
    QGraphicsScene* scene;
    QGraphicsView* view;
    QTimer* inputTimer;

    Player localPlayer;
    uint32_t localPlayerId;

    QMap<uint32_t, QGraphicsEllipseItem*> playerGraphics;
    QMap<uint32_t, QGraphicsTextItem*> playerNames;
};

#endif
