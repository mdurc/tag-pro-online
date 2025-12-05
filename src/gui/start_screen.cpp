#include "start_screen.h"

#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include "../network/protocol.h"

LobbyScreen::LobbyScreen(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* title = new QLabel("Lobby");
    title->setObjectName("screenTitle");

    playerList = new QListWidget();
    leaveButton = new QPushButton("Leave Lobby");

    layout->addWidget(title);
    layout->addWidget(playerList);
    layout->addWidget(leaveButton);

    connect(leaveButton, &QPushButton::clicked, this, [this]() {
        emit leaveLobbyRequested();
    });
}

void LobbyScreen::updatePlayerList(const QStringList& players) {
    playerList->clear();
    for (const QString& player : players) {
        playerList->addItem(player);
    }
}

void LobbyScreen::clearPlayerList() {
    playerList->clear();
}

void LobbyScreen::setHost(bool host) {
    if (!startGameBtn) {
        startGameBtn = new QPushButton("Start Game", this);
        connect(startGameBtn, &QPushButton::clicked, this, [this]() {
            emit lobbyHostStartGameRequested();
        });
        layout()->addWidget(startGameBtn);
    }
    startGameBtn->setVisible(host);
}

// Start screen implementation:
StartScreen::StartScreen(QWidget* parent) : QWidget(parent) {
  setupUI();
}
StartScreen::~StartScreen() { cleanupServerClient(); }

void StartScreen::closeEvent(QCloseEvent *event) {
    cleanupServerClient();
    event->accept();
}
void StartScreen::onLobbyLeave() {
    if (server) {
      // maybe send a message to clients that the server is closing
    }
    cleanupServerClient();
    returnToMainMenu();
}
void StartScreen::onBackClicked() { returnToMainMenu(); }
void StartScreen::onHostClicked() { stackedWidget->setCurrentIndex(1); }
void StartScreen::onJoinClicked() { stackedWidget->setCurrentIndex(2); }
void StartScreen::onHostCreateLobby(QString port) {
    if (port.isEmpty()) {
        port = "12345";
    }
    server = new Server(port.toUShort());
    if (server->init()) {
        server->start(true);

        // let this host join as a client to their own server
        onClientJoinGame("127.0.0.1", port);
    } else {
        LOG("[StartScreen] Failed to start server");
        cleanupServer();
    }
}

void StartScreen::onClientJoinGame(QString ip, QString port) {
    client = new Client();
    if (ip.isEmpty()) ip = "127.0.0.1";
    if (port.isEmpty()) port = "12345";
    setupClientCallbacks(); // callbacks before connecting
    client->connect(port.toInt(), ip.toStdString().c_str());
}

void StartScreen::cleanupServerClient() {
    cleanupClient();
    cleanupServer();
}
void StartScreen::cleanupClient() {
    if (client) {
        client->clearCallbacks();
        gameScreen->setLocalClient(nullptr);
        delete client;
        client = nullptr;
    }
}
void StartScreen::cleanupServer() {
    if (server) {
        delete server;
        server = nullptr;
    }
}

void StartScreen::onClientMessageReceived(const std::string& message) {
    if (message.empty()) return;
    uint8_t messageType = static_cast<uint8_t>(message[0]);
    switch (messageType) {
        case Protocol::PLAYER_LIST: {
            std::vector<std::string> players = Protocol::deserializePlayerList(message);
            QStringList qPlayers;
            for (const auto& player : players) {
                qPlayers.append(QString::fromStdString(player));
            }
            lobbyScreen->updatePlayerList(qPlayers);
            break;
        }
        case Protocol::GAME_STATE: {
            // first game state update will transition to the game screen
            if (client) {
              if (stackedWidget->currentWidget() != gameScreen) {
                gameScreen->setLocalClient(client);
                stackedWidget->setCurrentWidget(gameScreen);
              } else {
                // we can change this to update the client callbacks to
                // game_screen functions but this is simple for now
                GameState state;
                Protocol::deserializeGameState(message, state);
                gameScreen->applyGameState(state);

                // TODO: implement option to leave a game on the gamescreen
              }
            }
            break;
        }
        case Protocol::MARK_CLIENT_HOST: {
            lobbyScreen->setHost(true);
            break;
        }
        default: {
            LOG("Unexpected message relayed from client to StartScreen (%d): %s",
                messageType, message.c_str());
            break;
        }
    }
}

void StartScreen::onClientConnectionChanged(bool connected) {
    if (connected) {
        stackedWidget->setCurrentWidget(lobbyScreen);
    } else {
        LOG("[StartScreen] Returning to main menu after disconnecting");
        cleanupClient();
        returnToMainMenu();
    }
}

void StartScreen::setupClientCallbacks() {
    if (client) {
        client->setMessageCallback([this](const std::string& message) {
            QMetaObject::invokeMethod(this, [this, message]() {
                onClientMessageReceived(message);
            }, Qt::QueuedConnection);
        });
        client->setConnectionCallback([this](bool connected) {
            QMetaObject::invokeMethod(this, [this, connected]() {
                onClientConnectionChanged(connected);
            }, Qt::QueuedConnection);
        });
    }
}

void StartScreen::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    stackedWidget = new QStackedWidget(this);

    stackedWidget->addWidget(createMainMenu());
    stackedWidget->addWidget(createHostScreen());
    stackedWidget->addWidget(createJoinScreen());
    stackedWidget->addWidget(lobbyScreen = new LobbyScreen());
    stackedWidget->addWidget(gameScreen = new GameScreen(this));

    mainLayout->addWidget(stackedWidget);

    connect(lobbyScreen, &LobbyScreen::leaveLobbyRequested, this, &StartScreen::onLobbyLeave);
    connect(lobbyScreen, &LobbyScreen::lobbyHostStartGameRequested, this, &StartScreen::onLobbyHostStartGame);

    QFile file("src/style.qss");
    assert(file.open(QFile::ReadOnly | QFile::Text));
    setStyleSheet(file.readAll());
}

void StartScreen::returnToMainMenu() {
    if (lobbyScreen) {
      lobbyScreen->clearPlayerList();
      lobbyScreen->setHost(false);
    }
    stackedWidget->setCurrentIndex(0);
}

void StartScreen::onLobbyHostStartGame() {
    if (server) {
        server->start_game();
    } else {
      std::string message = Protocol::serializeRequestStartGame();
      std::string framed = Protocol::frameMessage(message);
      client->sendMessage(message);
    }
}

QWidget* StartScreen::createMainMenu() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(20);

    QLabel* title = new QLabel("TAGPRO");
    title->setObjectName("title");

    QLabel* subtitle = new QLabel("Capture the Flag");
    subtitle->setObjectName("subtitle");

    QPushButton* hostBtn = new QPushButton("Host Game");
    QPushButton* joinBtn = new QPushButton("Join Game");
    QPushButton* exitBtn = new QPushButton("Exit");
    exitBtn->setObjectName("exitButton");

    layout->addWidget(title, 0, Qt::AlignCenter);
    layout->addWidget(subtitle, 0, Qt::AlignCenter);
    layout->addSpacing(30);
    layout->addWidget(hostBtn);
    layout->addWidget(joinBtn);
    layout->addWidget(exitBtn);

    connect(hostBtn, &QPushButton::clicked, this, &StartScreen::onHostClicked);
    connect(joinBtn, &QPushButton::clicked, this, &StartScreen::onJoinClicked);
    connect(exitBtn, &QPushButton::clicked, qApp, &QApplication::quit);

    return widget;
}

QWidget* StartScreen::createHostScreen() {
  QWidget* widget = new QWidget();
  QVBoxLayout* layout = new QVBoxLayout(widget);

  QLabel* title = new QLabel("Host Game");
  title->setObjectName("screenTitle");

  QLineEdit* portInput = new QLineEdit();
  portInput->setPlaceholderText("Port");

  QComboBox* mapCombo = new QComboBox();
  mapCombo->addItems({"Classic"});

  QComboBox* maxPlayers = new QComboBox();
  maxPlayers->addItems({"4"});
  maxPlayers->setCurrentText("4");

  QHBoxLayout* buttons = new QHBoxLayout();
  QPushButton* backBtn = new QPushButton("Back");
  backBtn->setObjectName("backButton");
  QPushButton* startBtn = new QPushButton("Start Game");
  buttons->addWidget(backBtn);
  buttons->addStretch();
  buttons->addWidget(startBtn);

  layout->addWidget(title);
  layout->addSpacing(20);
  layout->addWidget(portInput);
  layout->addWidget(mapCombo);
  layout->addWidget(maxPlayers);
  layout->addStretch();
  layout->addLayout(buttons);

  connect(backBtn, &QPushButton::clicked, this, &StartScreen::onBackClicked);
  connect(startBtn, &QPushButton::clicked, this, [this, portInput]() {
    onHostCreateLobby(portInput->text());
  });

  return widget;
}

QWidget* StartScreen::createJoinScreen() {
  QWidget* widget = new QWidget();
  QVBoxLayout* layout = new QVBoxLayout(widget);

  QLabel* title = new QLabel("Join Game");
  title->setObjectName("screenTitle");

  QLineEdit* serverInput = new QLineEdit();
  serverInput->setPlaceholderText("Server ID or IP");

  QLineEdit* portInput = new QLineEdit();
  portInput->setPlaceholderText("Port");

  QHBoxLayout* buttons = new QHBoxLayout();
  QPushButton* backBtn = new QPushButton("Back");
  backBtn->setObjectName("backButton");
  QPushButton* connectBtn = new QPushButton("Connect");
  buttons->addWidget(backBtn);
  buttons->addStretch();
  buttons->addWidget(connectBtn);

  layout->addWidget(title);
  layout->addSpacing(20);
  layout->addWidget(serverInput);
  layout->addWidget(portInput);
  layout->addStretch();
  layout->addLayout(buttons);

  connect(backBtn, &QPushButton::clicked, this, &StartScreen::onBackClicked);
  connect(connectBtn, &QPushButton::clicked, this, [this, serverInput, portInput]() {
      onClientJoinGame(serverInput->text(), portInput->text());
  });

  return widget;
}
