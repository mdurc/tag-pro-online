#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include "network.h"
#include "../game/game_state.h"

namespace Protocol {
    enum MessageType : uint8_t {
        PLAYER_LIST = 0x01,
        GAME_STATE = 0x02,
        PLAYER_INPUT = 0x03,
        REQUEST_PLAYER_LIST = 0x04,
        PLAYER_JOINED = 0x05, // used to assign a client with playerId from server
        PLAYER_LEFT = 0x06, // TODO
    };

    std::string serializeGameState(const GameState& state);
    bool deserializeGameState(const std::string& data, GameState& state);

    std::string serializePlayerList(const std::vector<std::string>& players);
    std::vector<std::string> deserializePlayerList(const std::string& data);

    std::string serializePlayerInput(uint32_t playerId, float inputX, float inputY);
    bool deserializePlayerInput(const std::string& data, uint32_t& playerId, float& inputX, float& inputY);

    std::string serializePlayerJoined(uint32_t playerId);
    bool deserializePlayerJoined(const std::string& data, uint32_t& playerId);

    std::string frameMessage(const std::string& data);
    bool extractMessage(std::string& buffer, std::string& message);

    bool sendRaw(const char* msg, SOCKET socket, std::atomic<bool>* running);
}

#endif // PROTOCOL_H
