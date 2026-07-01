#include "Hockey/Gameplay/Tuning/TuningSerializer.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

#include <yaml-cpp/yaml.h>

#include "Hockey/Core/FileSystem.hpp"

namespace Hockey {
namespace {
float ReadFloat(const YAML::Node& node, const char* key, float fallback) {
    if (const YAML::Node value = node[key]) {
        return value.as<float>(fallback);
    }
    return fallback;
}

uint32_t ReadUInt(const YAML::Node& node, const char* key, uint32_t fallback) {
    if (const YAML::Node value = node[key]) {
        return value.as<uint32_t>(fallback);
    }
    return fallback;
}

glm::vec3 ReadVec3(const YAML::Node& node, const char* key, glm::vec3 fallback) {
    const YAML::Node value = node[key];
    if (!value || !value.IsSequence() || value.size() != 3) {
        return fallback;
    }
    return {value[0].as<float>(fallback.x), value[1].as<float>(fallback.y), value[2].as<float>(fallback.z)};
}

std::filesystem::path ReadPath(const YAML::Node& node, const char* key, const std::filesystem::path& fallback) {
    if (const YAML::Node value = node[key]) {
        return value.as<std::string>(fallback.generic_string());
    }
    return fallback;
}

void EmitVec3(YAML::Emitter& out, const char* key, const glm::vec3& value) {
    out << YAML::Key << key << YAML::Value << YAML::Flow << YAML::BeginSeq << value.x << value.y << value.z << YAML::EndSeq;
}

void EmitStickTuning(YAML::Emitter& out, const char* key, const StickTuning& tuning) {
    out << YAML::Key << key << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "PrefabPath" << YAML::Value << tuning.prefabPath.generic_string();
    out << YAML::Key << "Reach" << YAML::Value << tuning.reach;
    out << YAML::Key << "Width" << YAML::Value << tuning.width;
    EmitVec3(out, "LocalOffset", tuning.localOffset);
    out << YAML::EndMap;
}

void ReadStickTuning(const YAML::Node& node, StickTuning& tuning) {
    tuning.prefabPath = ReadPath(node, "PrefabPath", tuning.prefabPath);
    tuning.reach = ReadFloat(node, "Reach", tuning.reach);
    tuning.width = ReadFloat(node, "Width", tuning.width);
    tuning.localOffset = ReadVec3(node, "LocalOffset", tuning.localOffset);
}
}

std::string TuningSerializer::Serialize(const GameplayTuning& tuning) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Skater" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "MaxSpeed" << YAML::Value << tuning.skater.maxSpeed;
    out << YAML::Key << "Acceleration" << YAML::Value << tuning.skater.acceleration;
    out << YAML::Key << "Deceleration" << YAML::Value << tuning.skater.deceleration;
    out << YAML::Key << "TurnSpeedDegrees" << YAML::Value << tuning.skater.turnSpeedDegrees;
    out << YAML::Key << "BoostImpulse" << YAML::Value << tuning.skater.boostImpulse;
    out << YAML::Key << "BoostCooldownSeconds" << YAML::Value << tuning.skater.boostCooldownSeconds;
    out << YAML::Key << "SlideStopDamping" << YAML::Value << tuning.skater.slideStopDamping;
    out << YAML::Key << "DoubleStopWindowSeconds" << YAML::Value << tuning.skater.doubleStopWindowSeconds;
    out << YAML::Key << "PuckControlRadius" << YAML::Value << tuning.skater.puckControlRadius;
    out << YAML::Key << "StealRadius" << YAML::Value << tuning.skater.stealRadius;
    out << YAML::Key << "StealCooldownSeconds" << YAML::Value << tuning.skater.stealCooldownSeconds;
    out << YAML::EndMap;

    out << YAML::Key << "Goalie" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "MaxSpeed" << YAML::Value << tuning.goalie.maxSpeed;
    out << YAML::Key << "Acceleration" << YAML::Value << tuning.goalie.acceleration;
    out << YAML::Key << "CreaseMoveMultiplier" << YAML::Value << tuning.goalie.creaseMoveMultiplier;
    out << YAML::Key << "SaveReachRadius" << YAML::Value << tuning.goalie.saveReachRadius;
    out << YAML::Key << "BoostCharges" << YAML::Value << tuning.goalie.boostCharges;
    out << YAML::Key << "BoostRechargeSeconds" << YAML::Value << tuning.goalie.boostRechargeSeconds;
    out << YAML::Key << "BoostImpulse" << YAML::Value << tuning.goalie.boostImpulse;
    out << YAML::Key << "ShieldRadius" << YAML::Value << tuning.goalie.shieldRadius;
    out << YAML::Key << "ShieldDurationSeconds" << YAML::Value << tuning.goalie.shieldDurationSeconds;
    out << YAML::Key << "ShieldCooldownSeconds" << YAML::Value << tuning.goalie.shieldCooldownSeconds;
    out << YAML::Key << "ShieldReflectImpulse" << YAML::Value << tuning.goalie.shieldReflectImpulse;
    out << YAML::EndMap;

    out << YAML::Key << "Puck" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "MaxSpeed" << YAML::Value << tuning.puck.maxSpeed;
    EmitVec3(out, "PossessionOffset", tuning.puck.possessionOffset);
    out << YAML::Key << "LoosePuckDrag" << YAML::Value << tuning.puck.loosePuckDrag;
    out << YAML::Key << "FloorY" << YAML::Value << tuning.puck.floorY;
    out << YAML::Key << "OutOfPlayY" << YAML::Value << tuning.puck.outOfPlayY;
    out << YAML::EndMap;

    EmitStickTuning(out, "SkaterStick", tuning.skaterStick);
    EmitStickTuning(out, "GoalieStick", tuning.goalieStick);

    out << YAML::Key << "Shot" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "MinPower" << YAML::Value << tuning.shot.minPower;
    out << YAML::Key << "MaxPower" << YAML::Value << tuning.shot.maxPower;
    out << YAML::Key << "ChargeSeconds" << YAML::Value << tuning.shot.chargeSeconds;
    out << YAML::Key << "SelfCollisionGraceSeconds" << YAML::Value << tuning.shot.selfCollisionGraceSeconds;
    out << YAML::Key << "AccuracyDegrees" << YAML::Value << tuning.shot.accuracyDegrees;
    out << YAML::EndMap;

    out << YAML::EndMap;
    return out.c_str();
}

bool TuningSerializer::Deserialize(const std::string& text, GameplayTuning& outTuning) {
    YAML::Node root;
    try {
        root = YAML::Load(text);
    } catch (const YAML::Exception&) {
        return false;
    }

    if (const YAML::Node skater = root["Skater"]) {
        outTuning.skater.maxSpeed = ReadFloat(skater, "MaxSpeed", outTuning.skater.maxSpeed);
        outTuning.skater.acceleration = ReadFloat(skater, "Acceleration", outTuning.skater.acceleration);
        outTuning.skater.deceleration = ReadFloat(skater, "Deceleration", outTuning.skater.deceleration);
        outTuning.skater.turnSpeedDegrees = ReadFloat(skater, "TurnSpeedDegrees", outTuning.skater.turnSpeedDegrees);
        outTuning.skater.boostImpulse = ReadFloat(skater, "BoostImpulse", outTuning.skater.boostImpulse);
        outTuning.skater.boostCooldownSeconds =
            ReadFloat(skater, "BoostCooldownSeconds", outTuning.skater.boostCooldownSeconds);
        outTuning.skater.slideStopDamping = ReadFloat(skater, "SlideStopDamping", outTuning.skater.slideStopDamping);
        outTuning.skater.doubleStopWindowSeconds =
            ReadFloat(skater, "DoubleStopWindowSeconds", outTuning.skater.doubleStopWindowSeconds);
        outTuning.skater.puckControlRadius = ReadFloat(skater, "PuckControlRadius", outTuning.skater.puckControlRadius);
        outTuning.skater.stealRadius = ReadFloat(skater, "StealRadius", outTuning.skater.stealRadius);
        outTuning.skater.stealCooldownSeconds =
            ReadFloat(skater, "StealCooldownSeconds", outTuning.skater.stealCooldownSeconds);
    }
    if (const YAML::Node goalie = root["Goalie"]) {
        outTuning.goalie.maxSpeed = ReadFloat(goalie, "MaxSpeed", outTuning.goalie.maxSpeed);
        outTuning.goalie.acceleration = ReadFloat(goalie, "Acceleration", outTuning.goalie.acceleration);
        outTuning.goalie.creaseMoveMultiplier = ReadFloat(goalie, "CreaseMoveMultiplier", outTuning.goalie.creaseMoveMultiplier);
        outTuning.goalie.saveReachRadius = ReadFloat(goalie, "SaveReachRadius", outTuning.goalie.saveReachRadius);
        outTuning.goalie.boostCharges = ReadUInt(goalie, "BoostCharges", outTuning.goalie.boostCharges);
        outTuning.goalie.boostRechargeSeconds =
            ReadFloat(goalie, "BoostRechargeSeconds", outTuning.goalie.boostRechargeSeconds);
        outTuning.goalie.boostImpulse = ReadFloat(goalie, "BoostImpulse", outTuning.goalie.boostImpulse);
        outTuning.goalie.shieldRadius = ReadFloat(goalie, "ShieldRadius", outTuning.goalie.shieldRadius);
        outTuning.goalie.shieldDurationSeconds =
            ReadFloat(goalie, "ShieldDurationSeconds", outTuning.goalie.shieldDurationSeconds);
        outTuning.goalie.shieldCooldownSeconds =
            ReadFloat(goalie, "ShieldCooldownSeconds", outTuning.goalie.shieldCooldownSeconds);
        outTuning.goalie.shieldReflectImpulse =
            ReadFloat(goalie, "ShieldReflectImpulse", outTuning.goalie.shieldReflectImpulse);
    }
    if (const YAML::Node puck = root["Puck"]) {
        outTuning.puck.maxSpeed = ReadFloat(puck, "MaxSpeed", outTuning.puck.maxSpeed);
        outTuning.puck.possessionOffset = ReadVec3(puck, "PossessionOffset", outTuning.puck.possessionOffset);
        outTuning.puck.loosePuckDrag = ReadFloat(puck, "LoosePuckDrag", outTuning.puck.loosePuckDrag);
        outTuning.puck.floorY = ReadFloat(puck, "FloorY", outTuning.puck.floorY);
        outTuning.puck.outOfPlayY = ReadFloat(puck, "OutOfPlayY", outTuning.puck.outOfPlayY);
    }
    if (const YAML::Node stick = root["Stick"]) {
        ReadStickTuning(stick, outTuning.skaterStick);
        ReadStickTuning(stick, outTuning.goalieStick);
    }
    if (const YAML::Node skaterStick = root["SkaterStick"]) {
        ReadStickTuning(skaterStick, outTuning.skaterStick);
    }
    if (const YAML::Node goalieStick = root["GoalieStick"]) {
        ReadStickTuning(goalieStick, outTuning.goalieStick);
    }
    if (const YAML::Node shot = root["Shot"]) {
        outTuning.shot.minPower = ReadFloat(shot, "MinPower", outTuning.shot.minPower);
        outTuning.shot.maxPower = ReadFloat(shot, "MaxPower", outTuning.shot.maxPower);
        outTuning.shot.chargeSeconds = ReadFloat(shot, "ChargeSeconds", outTuning.shot.chargeSeconds);
        outTuning.shot.selfCollisionGraceSeconds =
            ReadFloat(shot, "SelfCollisionGraceSeconds", outTuning.shot.selfCollisionGraceSeconds);
        outTuning.shot.accuracyDegrees = ReadFloat(shot, "AccuracyDegrees", outTuning.shot.accuracyDegrees);
    }
    return true;
}

Status TuningSerializer::Save(const std::filesystem::path& path, const GameplayTuning& tuning) {
    return FileSystem::WriteTextFile(path, Serialize(tuning));
}

Result<GameplayTuning> TuningSerializer::Load(const std::filesystem::path& path) {
    Result<std::string> text = FileSystem::ReadTextFile(path);
    if (!text) {
        return Result<GameplayTuning>::Fail(text.error);
    }
    GameplayTuning tuning;
    if (!Deserialize(text.value, tuning)) {
        return Result<GameplayTuning>::Fail("failed to parse gameplay tuning: " + path.string());
    }
    return Result<GameplayTuning>::Ok(tuning);
}

}
