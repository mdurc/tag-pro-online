#ifndef START_SCREEN_H
#define START_SCREEN_H

#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QWidget>
#include "client.h"
#include "server.h"

class LobbyScreen : public QWidget {
  Q_OBJECT
 public:
  LobbyScreen(QWidget* parent = nullptr);
  void updatePlayerList(const QStringList& players);

 signals:
  void leaveLobbyRequested();

 private:
  QListWidget* playerList;
  QPushButton* leaveButton;
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

private:
  void setupUI();
  QWidget* createMainMenu();
  QWidget* createHostScreen();
  QWidget* createJoinScreen();

  QStackedWidget* stackedWidget;
  Server* server = nullptr;
  Client* client = nullptr;
  LobbyScreen* lobbyScreen;
};

#endif
