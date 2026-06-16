#include "Hockey/Physics/PhysicsLayer.hpp"

#include <array>

namespace Hockey {

namespace {

using Matrix = std::array<std::array<bool, kPhysicsLayerCount>, kPhysicsLayerCount>;

Matrix MakeDefaultMatrix() {
    Matrix m{};
    for (auto& row : m) {
        row.fill(false);
    }

    auto enable = [&m](PhysicsLayer a, PhysicsLayer b) {
        const auto ia = static_cast<std::size_t>(a);
        const auto ib = static_cast<std::size_t>(b);
        m[ia][ib] = true;
        m[ib][ia] = true;
    };

    // Static world geometry stops the dynamic actors.
    enable(PhysicsLayer::Static, PhysicsLayer::Player);
    enable(PhysicsLayer::Static, PhysicsLayer::Goalie);
    enable(PhysicsLayer::Static, PhysicsLayer::Puck);
    enable(PhysicsLayer::Static, PhysicsLayer::Stick);

    // Players / goalies interact with each other and the world.
    enable(PhysicsLayer::Player, PhysicsLayer::Player);
    enable(PhysicsLayer::Player, PhysicsLayer::Goalie);
    enable(PhysicsLayer::Player, PhysicsLayer::Puck);
    enable(PhysicsLayer::Player, PhysicsLayer::Rink);
    enable(PhysicsLayer::Player, PhysicsLayer::Goal);
    enable(PhysicsLayer::Player, PhysicsLayer::Trigger);

    enable(PhysicsLayer::Goalie, PhysicsLayer::Goalie);
    enable(PhysicsLayer::Goalie, PhysicsLayer::Puck);
    enable(PhysicsLayer::Goalie, PhysicsLayer::Rink);
    enable(PhysicsLayer::Goalie, PhysicsLayer::Goal);
    enable(PhysicsLayer::Goalie, PhysicsLayer::Trigger);

    // Puck interacts with everything that can touch it.
    enable(PhysicsLayer::Puck, PhysicsLayer::Puck);
    enable(PhysicsLayer::Puck, PhysicsLayer::Stick);
    enable(PhysicsLayer::Puck, PhysicsLayer::Rink);
    enable(PhysicsLayer::Puck, PhysicsLayer::Goal);
    enable(PhysicsLayer::Puck, PhysicsLayer::Trigger);

    // Sticks can hit pucks and (optionally) bodies.
    enable(PhysicsLayer::Stick, PhysicsLayer::Player);
    enable(PhysicsLayer::Stick, PhysicsLayer::Goalie);
    enable(PhysicsLayer::Stick, PhysicsLayer::Rink);

    // Sensor volumes are non-physical detection zones (e.g. crease, offside
    // lines). Like triggers they overlap the moving actors but never the static
    // world, so they report enter/exit without affecting the simulation.
    enable(PhysicsLayer::Sensor, PhysicsLayer::Player);
    enable(PhysicsLayer::Sensor, PhysicsLayer::Goalie);
    enable(PhysicsLayer::Sensor, PhysicsLayer::Puck);
    enable(PhysicsLayer::Sensor, PhysicsLayer::Stick);

    return m;
}

Matrix& State() {
    static Matrix matrix = MakeDefaultMatrix();
    return matrix;
}

} // namespace

const char* PhysicsLayerToString(PhysicsLayer layer) {
    switch (layer) {
    case PhysicsLayer::Static:
        return "Static";
    case PhysicsLayer::Player:
        return "Player";
    case PhysicsLayer::Goalie:
        return "Goalie";
    case PhysicsLayer::Puck:
        return "Puck";
    case PhysicsLayer::Stick:
        return "Stick";
    case PhysicsLayer::Rink:
        return "Rink";
    case PhysicsLayer::Goal:
        return "Goal";
    case PhysicsLayer::Trigger:
        return "Trigger";
    case PhysicsLayer::Sensor:
        return "Sensor";
    case PhysicsLayer::Editor:
        return "Editor";
    }
    return "Static";
}

bool PhysicsLayerFromString(std::string_view text, PhysicsLayer& out) {
    struct Entry {
        const char* name;
        PhysicsLayer layer;
    };
    static constexpr Entry kEntries[] = {
        {"Static", PhysicsLayer::Static}, {"Player", PhysicsLayer::Player},   {"Goalie", PhysicsLayer::Goalie},
        {"Puck", PhysicsLayer::Puck},     {"Stick", PhysicsLayer::Stick},     {"Rink", PhysicsLayer::Rink},
        {"Goal", PhysicsLayer::Goal},     {"Trigger", PhysicsLayer::Trigger}, {"Sensor", PhysicsLayer::Sensor},
        {"Editor", PhysicsLayer::Editor},
    };
    for (const auto& entry : kEntries) {
        if (text == entry.name) {
            out = entry.layer;
            return true;
        }
    }
    return false;
}

bool IsValidPhysicsLayer(std::uint16_t value) {
    return value < kPhysicsLayerCount;
}

bool CollisionFilter::ShouldCollide(PhysicsLayer a, PhysicsLayer b) {
    const auto ia = static_cast<std::size_t>(a);
    const auto ib = static_cast<std::size_t>(b);
    if (ia >= kPhysicsLayerCount || ib >= kPhysicsLayerCount) {
        return false;
    }
    return State()[ia][ib];
}

void CollisionFilter::SetShouldCollide(PhysicsLayer a, PhysicsLayer b, bool shouldCollide) {
    const auto ia = static_cast<std::size_t>(a);
    const auto ib = static_cast<std::size_t>(b);
    if (ia >= kPhysicsLayerCount || ib >= kPhysicsLayerCount) {
        return;
    }
    State()[ia][ib] = shouldCollide;
    State()[ib][ia] = shouldCollide;
}

void CollisionFilter::ResetDefaults() {
    State() = MakeDefaultMatrix();
}

} // namespace Hockey
