#include "start_screen.h"

#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

StartScreen::StartScreen(QWidget* parent) : QWidget(parent) { setupUI(); }

void StartScreen::onBackClicked() { stackedWidget->setCurrentIndex(0); }
void StartScreen::onHostClicked() { stackedWidget->setCurrentIndex(1); }
void StartScreen::onJoinClicked() { stackedWidget->setCurrentIndex(2); }
void StartScreen::onStartGameClicked() { qDebug() << "Start as host"; }

void StartScreen::onConnectClicked(QString ip, QString port) {
    if (client) {
        delete client;
    }
    client = new Client();
    client->connect(port.toInt(), ip.toStdString().c_str());
}

void StartScreen::closeClient() {
    if (client) {
        delete client;
    }
}

void StartScreen::setupUI() {
  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  stackedWidget = new QStackedWidget(this);

  stackedWidget->addWidget(createMainMenu());
  stackedWidget->addWidget(createHostScreen());
  stackedWidget->addWidget(createJoinScreen());

  mainLayout->addWidget(stackedWidget);

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
  connect(qApp, &QApplication::aboutToQuit, this, &StartScreen::closeClient);
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
  connect(startBtn, &QPushButton::clicked, this,
          &StartScreen::onStartGameClicked);

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
      // This code runs when the button is clicked
      QString ip = serverInput->text();
      QString port = portInput->text();

      // Call your actual function with the values
      onConnectClicked(ip, port);
  });

  return widget;
}
