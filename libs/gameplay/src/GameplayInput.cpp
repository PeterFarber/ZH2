#include "Hockey/Gameplay/GameplayInput.hpp"

#include <glm/geometric.hpp>

namespace Hockey {

bool TryBuildAimFromWorldTarget(glm::vec3 sourcePosition, glm::vec3 targetPosition, glm::vec2& outAim) {
    const glm::vec2 delta{targetPosition.x - sourcePosition.x, targetPosition.z - sourcePosition.z};
    if (glm::dot(delta, delta) <= 0.0001f) {
        return false;
    }
    outAim = glm::normalize(delta);
    return true;
}

void GameplayInputBuffer::PushInput(const GameplayInputFrame& input) {
    auto it = m_LatestByPlayer.find(input.playerIndex);
    if (it == m_LatestByPlayer.end() || input.inputSequence >= it->second.inputSequence) {
        m_LatestByPlayer[input.playerIndex] = input;
    }
}

bool GameplayInputBuffer::GetInput(uint32_t playerIndex, GameplayInputFrame& outInput) const {
    const auto it = m_LatestByPlayer.find(playerIndex);
    if (it == m_LatestByPlayer.end()) {
        return false;
    }
    outInput = it->second;
    return true;
}

void GameplayInputBuffer::ClearForTick(uint64_t tick) {
    for (auto it = m_LatestByPlayer.begin(); it != m_LatestByPlayer.end();) {
        if (it->second.simulationTick <= tick) {
            it = m_LatestByPlayer.erase(it);
        } else {
            ++it;
        }
    }
}

void GameplayInputBuffer::Reset() {
    m_LatestByPlayer.clear();
}

std::size_t GameplayInputBuffer::Size() const {
    return m_LatestByPlayer.size();
}

}
