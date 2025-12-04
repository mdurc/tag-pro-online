#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QTimer>
#include "../game/game.h"
#include "../network/client.h"
#include "../game/game_state.h"

class InputHandler {
public:
    void keyPressed(int key);
    void keyReleased(int key);
    QVector2D getInputVector() const;
private:
    QSet<int> keysPressed;
};

class GameScreen : public QWidget
{
    Q_OBJECT
public:
    GameScreen(QWidget* parent = nullptr);
    ~GameScreen();

    void setLocalClient(Client* client) { localClient = client; }
    void applyGameState(const GameState& state);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void sendPlayerInput();

private:
    void setupScene();
    void updateRedFlag(uint32_t redFlag);
    void updateBlueFlag(uint32_t blueFlag);
    void updatePlayerGraphics(uint32_t playerId, const PlayerState& state);
    void removePlayerGraphics(uint32_t playerId);
    QColor getTeamColor(uint8_t team);

    Client* localClient = nullptr;
    QGraphicsScene* scene = nullptr;
    QGraphicsView* view = nullptr;
    QTimer* inputTimer = nullptr;

    InputHandler inputs;
    QMap<uint32_t, QGraphicsEllipseItem*> playerGraphics;
    QMap<uint32_t, QGraphicsTextItem*> playerNames;
    QGraphicsPolygonItem* redFlag = nullptr, *blueFlag = nullptr;
};

#endif
