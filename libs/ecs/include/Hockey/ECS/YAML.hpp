#pragma once

#include <string>

#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

#include "Hockey/ECS/Components.hpp"

// glm emitter overloads live in namespace YAML so that argument-dependent
// lookup finds them through the YAML::Emitter parameter.
namespace YAML {

Emitter& operator<<(Emitter& out, const glm::vec2& value);
Emitter& operator<<(Emitter& out, const glm::vec3& value);
Emitter& operator<<(Emitter& out, const glm::vec4& value);

} // namespace YAML

namespace Hockey {

bool ReadVec2(const YAML::Node& node, glm::vec2& out);
bool ReadVec3(const YAML::Node& node, glm::vec3& out);
bool ReadVec4(const YAML::Node& node, glm::vec4& out);

std::string TeamToString(Team team);
Team TeamFromString(const std::string& value);

std::string PlayerRoleToString(PlayerRole role);
PlayerRole PlayerRoleFromString(const std::string& value);

} // namespace Hockey
