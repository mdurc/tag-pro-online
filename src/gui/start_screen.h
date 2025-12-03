#ifndef START_SCREEN_H
#define START_SCREEN_H

#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QLabel>
#include <QWidget>
#include "../network/client.h"
#include "../network/server.h"
#include "game_screen.h"

class LobbyScreen : public QWidget {
  Q_OBJECT
 public:
  LobbyScreen(QWidget* parent = nullptr);
  void updatePlayerList(const QStringList& players);
  void clearPlayerList();
  void setHost(bool h);

 signals:
  void leaveLobbyRequested();
  void lobbyHostStartGameRequested();

 private:
  QListWidget* playerList;
  QPushButton* leaveButton;
  QPushButton* startGameBtn = nullptr;
};

class StartScreen : public QWidget {
  Q_OBJECT

public:
  StartScreen(QWidget* parent = nullptr);
  ~StartScreen();

protected:
  void closeEvent(QCloseEvent* event) override;

private slots:
  // from Lobby:
  void onLobbyLeave();
  void onLobbyHostStartGame();

  void onHostClicked();
  void onJoinClicked();
  void onBackClicked();

  void onHostCreateLobby(QString port);
  void onClientJoinGame(QString ip, QString port);

private:
  void cleanupServerClient();
  void cleanupClient();
  void cleanupServer();

  void onClientMessageReceived(const std::string& message);
  void onClientConnectionChanged(bool connected);
  void setupClientCallbacks();

  void setupUI();
  void returnToMainMenu();
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
