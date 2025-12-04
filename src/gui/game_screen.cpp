#include "game_screen.h"

#include <QDebug>
#include <QGraphicsTextItem>
#include <QVBoxLayout>

void InputHandler::keyPressed(int key) { keysPressed.insert(key); }
void InputHandler::keyReleased(int key) { keysPressed.remove(key); }
QVector2D InputHandler::getInputVector() const {
  QVector2D input(0, 0);
  if (keysPressed.contains(Qt::Key_Up)) input.setY(-1);
  if (keysPressed.contains(Qt::Key_Down)) input.setY(1);
  if (keysPressed.contains(Qt::Key_Left)) input.setX(-1);
  if (keysPressed.contains(Qt::Key_Right)) input.setX(1);
  input.normalize();
  return input;
}

GameScreen::GameScreen(QWidget* parent) : QWidget(parent) {
  setupScene();

  inputTimer = new QTimer(this);
  connect(inputTimer, &QTimer::timeout, this, &GameScreen::sendPlayerInput);
  inputTimer->start(16); // ~60fps

  view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view->setFocusPolicy(Qt::StrongFocus);

  view->setFrameShape(QFrame::NoFrame);
}

GameScreen::~GameScreen() { inputTimer->stop(); }

void GameScreen::setupScene() {
  QVBoxLayout* layout = new QVBoxLayout(this);
  scene = new QGraphicsScene(this);
  view = new QGraphicsView(scene);
  layout->addWidget(view);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  scene->setSceneRect(0, 0, Game::arenaWidth, Game::arenaHeight);
  scene->setBackgroundBrush(QBrush(QColor(50, 50, 50)));

  QGraphicsLineItem* centerLine =
      scene->addLine(400, 0, 400, 600, QPen(Qt::white, 2, Qt::DashLine));
  centerLine->setZValue(-1);

  // Add a debug rectangle representing the arena boundaries
  QGraphicsRectItem* bounds = scene->addRect(0, 0, Game::arenaWidth, Game::arenaHeight, QPen(Qt::red, 2));
  bounds->setZValue(0); // Draw behind players
}

void GameScreen::applyGameState(const GameState& state) {
  for (const auto& [id, playerState] : state.players) {
    updatePlayerGraphics(id, playerState);
  }

  QList<uint32_t> currentPlayers = playerGraphics.keys();
  for (uint32_t playerId : currentPlayers) {
    if (state.players.find(playerId) == state.players.end()) {
      removePlayerGraphics(playerId);
    }
  }
}

void GameScreen::updatePlayerGraphics(uint32_t playerId,
                                      const PlayerState& state) {
  if (!localClient) return;
  if (!playerGraphics.contains(playerId)) {
    QColor color = getTeamColor(state.team);

    QGraphicsEllipseItem* circle =
        scene->addEllipse(-Game::playerRadius, -Game::playerRadius, Game::playerRadius * 2, Game::playerRadius * 2, QPen(Qt::black, 2), QBrush(color));
    circle->setZValue(1);

    QGraphicsTextItem* nameTag = scene->addText(QString::fromStdString(state.name));
    nameTag->setDefaultTextColor(Qt::white);
    nameTag->setZValue(2);

    playerGraphics[playerId] = circle;
    playerNames[playerId] = nameTag;
  }

  // update positions
  QGraphicsEllipseItem* circle = playerGraphics[playerId];
  QGraphicsTextItem* nameTag = playerNames[playerId];

  circle->setPos(state.x, state.y);
  nameTag->setPos(state.x - nameTag->boundingRect().width() / 2,
                  state.y + 20);

  // highlight local player
  if (playerId == localClient->getPlayerId()) {
    circle->setPen(QPen(Qt::black, 2));
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
    case Qt::Key_Up: inputs.keyPressed(Qt::Key_Up); break;
    case Qt::Key_S:
    case Qt::Key_Down: inputs.keyPressed(Qt::Key_Down); break;
    case Qt::Key_A:
    case Qt::Key_Left: inputs.keyPressed(Qt::Key_Left); break;
    case Qt::Key_D:
    case Qt::Key_Right: inputs.keyPressed(Qt::Key_Right); break;
  }
  event->accept();
}

void GameScreen::keyReleaseEvent(QKeyEvent* event) {
  // update local player input
  switch (event->key()) {
    case Qt::Key_W:
    case Qt::Key_Up: inputs.keyReleased(Qt::Key_Up); break;
    case Qt::Key_S:
    case Qt::Key_Down: inputs.keyReleased(Qt::Key_Down); break;
    case Qt::Key_A:
    case Qt::Key_Left: inputs.keyReleased(Qt::Key_Left); break;
    case Qt::Key_D:
    case Qt::Key_Right: inputs.keyReleased(Qt::Key_Right); break;
  }
  event->accept();
}

void GameScreen::sendPlayerInput() {
  if (!localClient) return;
  QVector2D input = inputs.getInputVector();
  localClient->sendPlayerInput(input.x(), input.y());
}
