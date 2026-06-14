#include "Hockey/ECS/YAML.hpp"

namespace YAML {

Emitter& operator<<(Emitter& out, const glm::vec2& value) {
    out << Flow << BeginSeq << value.x << value.y << EndSeq;
    return out;
}

Emitter& operator<<(Emitter& out, const glm::vec3& value) {
    out << Flow << BeginSeq << value.x << value.y << value.z << EndSeq;
    return out;
}

Emitter& operator<<(Emitter& out, const glm::vec4& value) {
    out << Flow << BeginSeq << value.x << value.y << value.z << value.w << EndSeq;
    return out;
}

} // namespace YAML

namespace Hockey {

bool ReadVec2(const YAML::Node& node, glm::vec2& out) {
    if (!node || !node.IsSequence() || node.size() != 2) {
        return false;
    }
    out.x = node[0].as<float>();
    out.y = node[1].as<float>();
    return true;
}

bool ReadVec3(const YAML::Node& node, glm::vec3& out) {
    if (!node || !node.IsSequence() || node.size() != 3) {
        return false;
    }
    out.x = node[0].as<float>();
    out.y = node[1].as<float>();
    out.z = node[2].as<float>();
    return true;
}

bool ReadVec4(const YAML::Node& node, glm::vec4& out) {
    if (!node || !node.IsSequence() || node.size() != 4) {
        return false;
    }
    out.x = node[0].as<float>();
    out.y = node[1].as<float>();
    out.z = node[2].as<float>();
    out.w = node[3].as<float>();
    return true;
}

std::string TeamToString(Team team) {
    switch (team) {
    case Team::Home:
        return "Home";
    case Team::Away:
        return "Away";
    case Team::None:
        return "None";
    }
    return "None";
}

Team TeamFromString(const std::string& value) {
    if (value == "Home")
        return Team::Home;
    if (value == "Away")
        return Team::Away;
    return Team::None;
}

std::string PlayerRoleToString(PlayerRole role) {
    switch (role) {
    case PlayerRole::Skater:
        return "Skater";
    case PlayerRole::Goalie:
        return "Goalie";
    }
    return "Skater";
}

PlayerRole PlayerRoleFromString(const std::string& value) {
    if (value == "Goalie")
        return PlayerRole::Goalie;
    return PlayerRole::Skater;
}

} // namespace Hockey
