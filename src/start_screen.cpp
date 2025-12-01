#include "start_screen.h"

#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

LobbyScreen::LobbyScreen(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* title = new QLabel("Lobby");
    title->setObjectName("screenTitle");

    playerList = new QListWidget();
    leaveButton = new QPushButton("Leave Lobby");
    startGameBtn = new QPushButton("Start Game (Test)");

    layout->addWidget(title);
    layout->addWidget(playerList);
    layout->addWidget(leaveButton);
    layout->addWidget(startGameBtn);

    connect(leaveButton, &QPushButton::clicked, this, [this]() {
        emit leaveLobbyRequested();
    });

    connect(startGameBtn, &QPushButton::clicked, this, [this]() {
        emit startGameRequested();
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

// Start screen implementation:
StartScreen::StartScreen(QWidget* parent) : QWidget(parent) {
  setupUI();
}
StartScreen::~StartScreen() { cleanupServerClient(); }

void StartScreen::returnToMainMenu() {
    if (lobbyScreen) lobbyScreen->clearPlayerList();
    stackedWidget->setCurrentIndex(0);
}

void StartScreen::transitionToGame() {
    if (client) {
        gameScreen = new GameScreen(client, 0, this);
        // replace the existing game screen in stacked widget
        stackedWidget->addWidget(gameScreen);
        stackedWidget->setCurrentWidget(gameScreen);
    }
}

void StartScreen::closeEvent(QCloseEvent *event) {
    cleanupServerClient();
    event->accept();
}
void StartScreen::onLobbyLeave() {
    cleanupServerClient();
    returnToMainMenu();
}
void StartScreen::onBackClicked() { returnToMainMenu(); }
void StartScreen::onHostClicked() { stackedWidget->setCurrentIndex(1); }
void StartScreen::onJoinClicked() { stackedWidget->setCurrentIndex(2); }
void StartScreen::onStartGameClicked(QString port) {
    if (port.isEmpty()) {
        port = "12345";
    }
    LOG("[StartScreen] Starting as host on port %s", port.toStdString().c_str());
    cleanupServerClient(); // any prior existing server/client
    server = new Server(port.toUShort());
    if (server->init()) {
        server->run(true);
        onConnectClicked("127.0.0.1", port);
    } else {
        LOG("[StartScreen] Failed to start server");
        cleanupServer();
    }
}

void StartScreen::onConnectClicked(QString ip, QString port) {
    LOG("[StartScreen] Connecting to %s:%s", ip.toStdString().c_str(), port.toStdString().c_str());
    cleanupClient(); // any prior client
    client = new Client();
    connect(client, &Client::connectedSuccessfully, this, &StartScreen::onConnected);
    connect(client, &Client::playerListUpdated, lobbyScreen, &LobbyScreen::updatePlayerList);
    connect(client, &Client::disconnectedFromServer, this, &StartScreen::onDisconnected);
    if (ip.isEmpty()) ip = "127.0.0.1";
    if (port.isEmpty()) port = "12345";
    client->connect(port.toInt(), ip.toStdString().c_str());
}

void StartScreen::onConnected() {
    if (client) {
        client->requestPlayerList();
    }
    stackedWidget->setCurrentWidget(lobbyScreen);
}

void StartScreen::onDisconnected() {
    returnToMainMenu();
    // clean up client but NOT server
    cleanupClient();
}

void StartScreen::cleanupServerClient() {
    cleanupClient();
    cleanupServer();
}
void StartScreen::cleanupClient() {
    if (client) {
        client->disconnect();
        delete client;
        client = nullptr;
    }
}
void StartScreen::cleanupServer() {
    if (server) {
        server->stop();
        delete server;
        server = nullptr;
    }
}

void StartScreen::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    stackedWidget = new QStackedWidget(this);

    stackedWidget->addWidget(createMainMenu());
    stackedWidget->addWidget(createHostScreen());
    stackedWidget->addWidget(createJoinScreen());
    stackedWidget->addWidget(lobbyScreen = new LobbyScreen());

    mainLayout->addWidget(stackedWidget);

    connect(lobbyScreen, &LobbyScreen::leaveLobbyRequested, this, &StartScreen::onLobbyLeave);
    connect(lobbyScreen, &LobbyScreen::startGameRequested, this, &StartScreen::transitionToGame);

    QFile file("src/style.qss");
    assert(file.open(QFile::ReadOnly | QFile::Text));
    setStyleSheet(file.readAll());
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
  mapCombo->addItems({"Classic", "Ricochet"});

  QComboBox* maxPlayers = new QComboBox();
  maxPlayers->addItems({"2", "4", "6", "8"});
  maxPlayers->setCurrentText("8");

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
    onStartGameClicked(portInput->text());
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
      onConnectClicked(serverInput->text(), portInput->text());
  });

  return widget;
}
