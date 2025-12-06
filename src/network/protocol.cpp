#include "network/protocol.h"

#include <cstring>
#include <sstream>
#include <vector>

namespace Protocol {
    // [xx]lobbyId|mapId|redScore|blueScore|player1;player2;...
    // each player: id,name,x,y,velocityX,velocityY,team,connected;
    std::string serializeGameState(const GameState& state) {
      std::ostringstream ss;
      ss << static_cast<char>(GAME_STATE);
      ss << state.lobbyId << '|' << static_cast<int>(state.mapId) << '|'
         << static_cast<int>(state.redScore) << '|'
         << static_cast<int>(state.blueScore) << '|';
      ss << state.redFlag << '|' << state.blueFlag << '|';
      for (const auto& pair : state.players) {
        const PlayerState& player = pair.second;
        ss << player.id << ',' << player.name << ',' << player.x << ',' << player.y
           << ',' << player.velocityX << ',' << player.velocityY << ','
           << static_cast<int>(player.team) << ',' << player.connected << ';';
      }
      return ss.str();
    }

    bool deserializeGameState(const std::string& data, GameState& state) {
      std::istringstream ss(data);
      char type;
      ss >> type;
      if (type != GAME_STATE) return false;

      char delim;
      int mapTmp, redTmp, blueTmp;
      ss >> state.lobbyId >> delim >> mapTmp >> delim >> redTmp >> delim >> blueTmp >> delim;
      state.mapId = static_cast<uint8_t>(mapTmp);
      state.redScore = static_cast<uint8_t>(redTmp);
      state.blueScore = static_cast<uint8_t>(blueTmp);
      ss >> state.redFlag >> delim >> state.blueFlag >> delim;
      std::string playerData;
      while (std::getline(ss, playerData, ';') && !playerData.empty()) {
        std::istringstream playerStream(playerData);
        PlayerState player;
        playerStream >> player.id >> delim;
        std::getline(playerStream, player.name, ',');
        playerStream >> player.x >> delim >> player.y >> delim >>
            player.velocityX >> delim >> player.velocityY >> delim >>
            player.team >> delim >> player.connected;
        player.team -= '0';
        state.players[player.id] = player;
      }
      return true;
    }

    std::string serializePlayerList(const std::vector<std::string>& players) {
      std::ostringstream ss;
      ss << static_cast<char>(PLAYER_LIST);
      for (size_t i = 0; i < players.size(); ++i) {
        if (i > 0) ss << ',';
        ss << players[i];
      }
      return ss.str();
    }

    std::vector<std::string> deserializePlayerList(const std::string& data) {
      std::vector<std::string> players;
      std::istringstream ss(data);
      char type;
      ss >> type;
      if (type != PLAYER_LIST) return players;

      std::string player;
      while (std::getline(ss, player, ',')) {
        if (!player.empty()) players.push_back(player);
      }
      return players;
    }

    // [xx]playerId,inputX,inputY
    std::string serializePlayerInput(uint32_t playerId, float inputX,
                                     float inputY) {
      std::ostringstream ss;
      ss << static_cast<char>(PLAYER_INPUT) << playerId << ',' << inputX << ','
         << inputY;
      return ss.str();
    }

    bool deserializePlayerInput(const std::string& data, uint32_t& playerId,
                                float& inputX, float& inputY) {
      std::istringstream ss(data);
      char type;
      ss >> type;
      if (type != PLAYER_INPUT) return false;

      char delim;
      ss >> playerId >> delim >> inputX >> delim >> inputY;
      return true;
    }

    std::string serializePlayerJoined(uint32_t playerId) {
      std::ostringstream ss;
      ss << static_cast<char>(PLAYER_JOINED) << playerId;
      return ss.str();
    }

    bool deserializePlayerJoined(const std::string& data, uint32_t& playerId) {
      std::istringstream ss(data);
      char type;
      ss >> type;
      if (type != PLAYER_JOINED) return false;

      char delim;
      ss >> playerId;
      return true;
    }

    std::string serializeServerShutdown() {
      std::ostringstream ss;
      ss << static_cast<char>(SERVER_SHUTDOWN);
      return ss.str();
    }

    bool deserializeServerShutdown(const std::string& data) {
      std::istringstream ss(data);
      int type;
      ss >> type;
      if (type != SERVER_SHUTDOWN) return false;

      return true;
    }

    std::string frameMessage(const std::string& data) {
      return std::to_string(data.size()) + ":" + data;
    }

    bool extractMessage(std::string& buffer, std::string& message) {
      size_t colonPos = buffer.find(':');
      if (colonPos == std::string::npos) return false;

      try {
        size_t messageLength = std::stoul(buffer.substr(0, colonPos));
        if (buffer.length() >= colonPos + 1 + messageLength) {
          message = buffer.substr(colonPos + 1, messageLength);
          buffer.erase(0, colonPos + 1 + messageLength);
          return true;
        }
      } catch (const std::exception&) {
        buffer.clear();
      }
      return false;
    }

    std::string serializeMarkClientHost() {
      std::ostringstream ss;
      ss << static_cast<char>(MARK_CLIENT_HOST) << "CLIENT_IS_HOST";
      return ss.str();
    }

    std::string serializeRequestStartGame() {
      std::ostringstream ss;
      ss << static_cast<char>(REQUEST_START_GAME) << "START_GAME_SERVER";
      return ss.str();
    }

    bool sendRaw(const char* msg, SOCKET socket) {
        if (socket == INVALID_SOCKET) return false;

        int totalSent = 0;
        int msgLength = strlen(msg);
        while (totalSent < msgLength) {
            int bytesSent = send(socket, msg + totalSent, msgLength - totalSent, 0);
            if (bytesSent <= 0) {
                LOG("Failed to send message to socket %d", socket);
                return false;
            }
            totalSent += bytesSent;
        }
        return totalSent == msgLength;
    }

} // namespace Protocol
