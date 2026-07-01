#include "Hockey/Gameplay/GameplayInput.hpp"

#include <glm/geometric.hpp>

namespace Hockey {
namespace {

void ClearTransientInputs(GameplayInputFrame& input) {
    input.setMoveTarget = false;
    input.clearMoveTarget = false;
    input.stealPressed = false;
    input.boostPressed = false;
    input.brakePressed = false;
    input.quickTurnPressed = false;
    input.shootPressed = false;
    input.shootReleased = false;
    input.switchPlayerPressed = false;
    input.goalieShieldPressed = false;
    input.pausePressed = false;
}

void MergeTransientInputs(GameplayInputFrame& pending, const GameplayInputFrame& input) {
    if (input.clearMoveTarget || input.brakePressed) {
        pending.clearMoveTarget = true;
        if (!input.setMoveTarget) {
            pending.setMoveTarget = false;
        }
    }
    if (input.setMoveTarget) {
        pending.moveTarget = input.moveTarget;
        pending.setMoveTarget = true;
    }

    pending.stealPressed = pending.stealPressed || input.stealPressed;
    pending.boostPressed = pending.boostPressed || input.boostPressed;
    pending.brakePressed = pending.brakePressed || input.brakePressed;
    pending.quickTurnPressed = pending.quickTurnPressed || input.quickTurnPressed;
    pending.shootPressed = pending.shootPressed || input.shootPressed;
    pending.shootReleased = pending.shootReleased || input.shootReleased;
    pending.switchPlayerPressed = pending.switchPlayerPressed || input.switchPlayerPressed;
    pending.goalieShieldPressed = pending.goalieShieldPressed || input.goalieShieldPressed;
    pending.pausePressed = pending.pausePressed || input.pausePressed;
}

bool HasAim(const GameplayInputFrame& input) {
    return glm::dot(input.aim, input.aim) > 0.0001f;
}

bool HasShotInput(const GameplayInputFrame& input) {
    return input.shootPressed || input.shootHeld || input.shootReleased;
}

} // namespace

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

void GameplayInputAccumulator::Accumulate(const GameplayInputFrame& input) {
    if (!m_HasPending || m_Pending.playerIndex != input.playerIndex) {
        m_Pending = input;
        m_HasPending = true;
        return;
    }

    m_Pending.inputSequence = input.inputSequence;
    m_Pending.simulationTick = input.simulationTick;
    m_Pending.move = input.move;
    // A one-frame release edge can wait for the next fixed tick while later
    // render samples arrive. Do not let a neutral post-release sample erase the
    // mouse aim that belongs to the pending shot release.
    if (HasAim(input) || (!HasShotInput(m_Pending) && !HasShotInput(input))) {
        m_Pending.aim = input.aim;
    }
    m_Pending.brakeHeld = input.brakeHeld;
    m_Pending.shootHeld = input.shootHeld;

    MergeTransientInputs(m_Pending, input);
}

GameplayInputFrame GameplayInputAccumulator::Consume(uint64_t simulationTick) {
    if (!m_HasPending) {
        GameplayInputFrame input;
        input.simulationTick = simulationTick;
        return input;
    }

    GameplayInputFrame input = m_Pending;
    input.simulationTick = simulationTick;
    ClearTransientInputs(m_Pending);
    m_Pending.simulationTick = simulationTick;
    return input;
}

void GameplayInputAccumulator::Reset() {
    m_Pending = GameplayInputFrame{};
    m_HasPending = false;
}

bool GameplayInputAccumulator::HasPendingInput() const {
    return m_HasPending;
}

}
