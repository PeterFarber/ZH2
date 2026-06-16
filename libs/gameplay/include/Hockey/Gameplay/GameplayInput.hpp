#pragma once

#include <cstdint>
#include <unordered_map>

#include <glm/glm.hpp>

namespace Hockey {

struct GameplayInputFrame {
    uint32_t playerIndex = 0;
    uint64_t inputSequence = 0;
    uint64_t simulationTick = 0;

    glm::vec2 move{0.0f};
    glm::vec2 aim{0.0f};

    bool sprint = false;
    bool shootPressed = false;
    bool shootHeld = false;
    bool shootReleased = false;

    bool passPressed = false;
    bool passHeld = false;
    bool passReleased = false;

    bool checkPressed = false;
    bool pokeCheckPressed = false;
    bool switchPlayerPressed = false;

    bool goalieActionPressed = false;
    bool pausePressed = false;
};

class GameplayInputBuffer {
public:
    void PushInput(const GameplayInputFrame& input);
    bool GetInput(uint32_t playerIndex, GameplayInputFrame& outInput) const;
    void ClearForTick(uint64_t tick);
    void Reset();

    std::size_t Size() const;

private:
    std::unordered_map<uint32_t, GameplayInputFrame> m_LatestByPlayer;
};

}
