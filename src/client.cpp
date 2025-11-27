#include "client.h"

Client::Client() {

}

void Client::createSocket() {
    if (!initSockets()) {
        qDebug() << "Socket initialization failed.";
        return;
    }

    this->clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        qDebug() << "Error creating socket.";
        cleanupSockets();
        return;
    }
}

void Client::connect(int port, char * ip) {
    // 3. Connect to Server
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    inet_pton(AF_INET, ip, &serverAddr.sin_addr);

    if (::connect(this->clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        qDebug() << "Connection failed.";
        closeSocket(this->clientSocket);
        cleanupSockets();
        return;
    }

    qInfo() << "Connected successfully.";
}
