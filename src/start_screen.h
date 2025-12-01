#ifndef START_SCREEN_H
#define START_SCREEN_H

#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QWidget>
#include "client.h"
#include "game_screen.h"
#include "server.h"

class LobbyScreen : public QWidget {
  Q_OBJECT
 public:
  LobbyScreen(QWidget* parent = nullptr);
  void updatePlayerList(const QStringList& players);
  void clearPlayerList();

 signals:
  void leaveLobbyRequested();
  void startGameRequested();

 private:
  QListWidget* playerList;
  QPushButton* leaveButton;
  QPushButton* startGameBtn;
};

class StartScreen : public QWidget {
  Q_OBJECT

public:
  StartScreen(QWidget* parent = nullptr);
  ~StartScreen();

protected:
  void closeEvent(QCloseEvent* event) override;

private slots:
  void onLobbyLeave();
  void onHostClicked();
  void onJoinClicked();
  void onBackClicked();
  void onStartGameClicked(QString port);
  void onConnectClicked(QString ip, QString port);
  void onConnected();
  void onDisconnected();

  void cleanupServerClient();
  void cleanupClient();
  void cleanupServer();

private:
  void setupUI();
  void returnToMainMenu();
  void transitionToGame();
  QWidget* createMainMenu();
  QWidget* createHostScreen();
  QWidget* createJoinScreen();

  QStackedWidget* stackedWidget;
  Server* server = nullptr;
  Client* client = nullptr;
  LobbyScreen* lobbyScreen = nullptr;
  GameScreen* gameScreen = nullptr;
};

#endif
