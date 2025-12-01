#include "game_screen.h"

#include <QDebug>
#include <QGraphicsTextItem>
#include <QVBoxLayout>

GameScreen::GameScreen(Client* client, uint32_t localPlayerId, QWidget* parent)
    : QWidget(parent), client(client), localPlayerId(localPlayerId) {
  setupScene();


  connect(client, &Client::gameMessageReceived, this,
          &GameScreen::processGameMessage);

  inputTimer = new QTimer(this);
  connect(inputTimer, &QTimer::timeout, this, &GameScreen::sendPlayerInput);
  inputTimer->start(16); // ~60fps

  view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view->setFocusPolicy(Qt::StrongFocus);
}

GameScreen::~GameScreen() { inputTimer->stop(); }

void GameScreen::setupScene() {
  QVBoxLayout* layout = new QVBoxLayout(this);
  scene = new QGraphicsScene(this);
  view = new QGraphicsView(scene);
  layout->addWidget(view);

  scene->setSceneRect(0, 0, 800, 600);
  scene->setBackgroundBrush(QBrush(QColor(50, 50, 50)));

  QGraphicsLineItem* centerLine =
      scene->addLine(400, 0, 400, 600, QPen(Qt::white, 2, Qt::DashLine));
  centerLine->setZValue(-1);
}

void GameScreen::applyGameState(const GameState& state) {
  for (const auto& playerPair : state.players) {
    const PlayerState& playerState = playerPair;
    updatePlayerGraphics(playerState.playerId, playerState);
  }

  QList<uint32_t> currentPlayers = playerGraphics.keys();
  for (uint32_t playerId : currentPlayers) {
    if (!state.players.contains(playerId)) {
      removePlayerGraphics(playerId);
    }
  }
}

void GameScreen::updatePlayerGraphics(uint32_t playerId,
                                      const PlayerState& state) {
  if (!playerGraphics.contains(playerId)) {
    QColor color = getTeamColor(state.team);

    QGraphicsEllipseItem* circle =
        scene->addEllipse(-15, -15, 30, 30, QPen(Qt::black, 2), QBrush(color));
    circle->setZValue(1);

    QGraphicsTextItem* nameTag = scene->addText(state.name);
    nameTag->setDefaultTextColor(Qt::white);
    nameTag->setZValue(2);

    playerGraphics[playerId] = circle;
    playerNames[playerId] = nameTag;
  }

  // update positions
  QGraphicsEllipseItem* circle = playerGraphics[playerId];
  QGraphicsTextItem* nameTag = playerNames[playerId];

  circle->setPos(state.position.x(), state.position.y());
  nameTag->setPos(state.position.x() - nameTag->boundingRect().width() / 2,
                  state.position.y() + 20);

  // highlight local player
  if (playerId == localPlayerId) {
    circle->setPen(QPen(Qt::yellow, 3));
  }
}

void GameScreen::removePlayerGraphics(uint32_t playerId) {
  if (playerGraphics.contains(playerId)) {
    scene->removeItem(playerGraphics[playerId]);
    delete playerGraphics[playerId];
    playerGraphics.remove(playerId);
  }
  if (playerNames.contains(playerId)) {
    scene->removeItem(playerNames[playerId]);
    delete playerNames[playerId];
    playerNames.remove(playerId);
  }
}

QColor GameScreen::getTeamColor(uint8_t team) {
  return (team == 0) ? QColor(255, 100, 100)
                     : QColor(100, 100, 255); // Red : Blue
}

void GameScreen::keyPressEvent(QKeyEvent* event) {
  // update local player input
  switch (event->key()) {
    case Qt::Key_W:
    case Qt::Key_Up: localPlayer.keyPressed(Qt::Key_Up); break;
    case Qt::Key_S:
    case Qt::Key_Down: localPlayer.keyPressed(Qt::Key_Down); break;
    case Qt::Key_A:
    case Qt::Key_Left: localPlayer.keyPressed(Qt::Key_Left); break;
    case Qt::Key_D:
    case Qt::Key_Right: localPlayer.keyPressed(Qt::Key_Right); break;
  }
  event->accept();
}

void GameScreen::keyReleaseEvent(QKeyEvent* event) {
  // update local player input
  switch (event->key()) {
    case Qt::Key_W:
    case Qt::Key_Up: localPlayer.keyReleased(Qt::Key_Up); break;
    case Qt::Key_S:
    case Qt::Key_Down: localPlayer.keyReleased(Qt::Key_Down); break;
    case Qt::Key_A:
    case Qt::Key_Left: localPlayer.keyReleased(Qt::Key_Left); break;
    case Qt::Key_D:
    case Qt::Key_Right: localPlayer.keyReleased(Qt::Key_Right); break;
  }
  event->accept();
}

void GameScreen::sendPlayerInput() {
  if (!client) return;
  QVector2D input = localPlayer.getInputVector();

  QString message = QString("PLAYER_INPUT:%1,%2")
                        .arg(input.x(), 0, 'f', 2)
                        .arg(input.y(), 0, 'f', 2);
  client->sendMessage(message.toStdString().c_str());
}

void GameScreen::processGameMessage(const QString& message) {
  if (message.startsWith("GAME_STATE:")) {
    QString stateData = message.mid(11);
    QStringList players = stateData.split('|', Qt::SkipEmptyParts);

    GameState newState;
    newState.lobbyId = 1;

    for (const QString& playerStr : players) {
      QStringList parts = playerStr.split(',');
      if (parts.size() >= 4) {
        PlayerState player;
        player.playerId = parts[0].toUInt();
        player.position.setX(parts[1].toFloat());
        player.position.setY(parts[2].toFloat());
        player.team = parts[3].toUInt();
        player.connected = true;

        newState.players[player.playerId] = player;
      }
    }

    applyGameState(newState);
  }
}
