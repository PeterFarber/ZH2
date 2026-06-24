#include "Hockey/Gameplay/GameplayComponents.hpp"

#include <cstddef>
#include <string>
#include <utility>

#include "Hockey/ECS/ComponentMetadata.hpp"
#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/ComponentSerializer.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/YAML.hpp"
#include "Hockey/Gameplay/Validation/GameplayValidation.hpp"

namespace Hockey {
namespace {

FieldMetadata MakeField(std::string name, FieldType type, std::size_t offset, std::string displayName = {}) {
    FieldMetadata field;
    field.name = std::move(name);
    field.displayName = displayName.empty() ? field.name : std::move(displayName);
    field.type = type;
    field.offset = offset;
    return field;
}

FieldMetadata MakeTeamField(std::string name, std::size_t offset) {
    FieldMetadata field = MakeField(std::move(name), FieldType::Enum, offset);
    field.enumNames = {"None", "Home", "Away"};
    field.enumValues = {static_cast<int>(GameplayTeam::None), static_cast<int>(GameplayTeam::Home),
                        static_cast<int>(GameplayTeam::Away)};
    return field;
}

FieldMetadata MakeRoleField(std::string name, std::size_t offset) {
    FieldMetadata field = MakeField(std::move(name), FieldType::Enum, offset);
    field.enumNames = {"Skater", "Goalie"};
    field.enumValues = {static_cast<int>(GameplayRole::Skater), static_cast<int>(GameplayRole::Goalie)};
    return field;
}

FieldMetadata MakeSlotField(std::string name, std::size_t offset) {
    FieldMetadata field = MakeField(std::move(name), FieldType::Enum, offset);
    field.enumNames = {"HomeSkater0", "HomeSkater1", "HomeSkater2", "HomeGoalie", "AwaySkater0",
                       "AwaySkater1", "AwaySkater2", "AwayGoalie", "None"};
    field.enumValues = {static_cast<int>(PlayerSlot::HomeSkater0), static_cast<int>(PlayerSlot::HomeSkater1),
                        static_cast<int>(PlayerSlot::HomeSkater2), static_cast<int>(PlayerSlot::HomeGoalie),
                        static_cast<int>(PlayerSlot::AwaySkater0), static_cast<int>(PlayerSlot::AwaySkater1),
                        static_cast<int>(PlayerSlot::AwaySkater2), static_cast<int>(PlayerSlot::AwayGoalie),
                        static_cast<int>(PlayerSlot::None)};
    return field;
}

FieldMetadata MakePhaseField(std::size_t offset) {
    FieldMetadata field = MakeField("Phase", FieldType::Enum, offset);
    field.enumNames = {"NotStarted", "Warmup", "FaceoffSetup", "Faceoff", "Playing",
                       "GoalScored", "PeriodEnded", "MatchEnded", "Paused"};
    field.enumValues = {static_cast<int>(MatchPhase::NotStarted),  static_cast<int>(MatchPhase::Warmup),
                        static_cast<int>(MatchPhase::FaceoffSetup), static_cast<int>(MatchPhase::Faceoff),
                        static_cast<int>(MatchPhase::Playing),     static_cast<int>(MatchPhase::GoalScored),
                        static_cast<int>(MatchPhase::PeriodEnded), static_cast<int>(MatchPhase::MatchEnded),
                        static_cast<int>(MatchPhase::Paused)};
    return field;
}

FieldMetadata MakePuckStateField(std::size_t offset) {
    FieldMetadata field = MakeField("State", FieldType::Enum, offset);
    field.enumNames = {"Loose", "Possessed", "Shot", "Frozen", "Resetting"};
    field.enumValues = {static_cast<int>(PuckState::Loose), static_cast<int>(PuckState::Possessed),
                        static_cast<int>(PuckState::Shot),  static_cast<int>(PuckState::Frozen),
                        static_cast<int>(PuckState::Resetting)};
    return field;
}

void EmitUUID(YAML::Emitter& out, const char* key, UUID id) {
    out << YAML::Key << key << YAML::Value << id.Value();
}

UUID ReadUUID(const YAML::Node& node, const char* key) {
    if (const YAML::Node value = node[key]) {
        return UUID(value.as<std::uint64_t>());
    }
    return UUID{};
}

void EmitUUIDList(YAML::Emitter& out, const char* key, const std::vector<UUID>& ids) {
    out << YAML::Key << key << YAML::Value << YAML::BeginSeq;
    for (UUID id : ids) {
        out << id.Value();
    }
    out << YAML::EndSeq;
}

void ReadUUIDList(const YAML::Node& node, const char* key, std::vector<UUID>& outIds) {
    outIds.clear();
    const YAML::Node list = node[key];
    if (!list || !list.IsSequence()) {
        return;
    }
    for (const YAML::Node value : list) {
        outIds.emplace_back(value.as<std::uint64_t>());
    }
}

void SerializeGameplay(YAML::Emitter& out, Entity entity) {
    if (entity.HasComponent<MatchStateComponent>()) {
        const auto& c = entity.GetComponent<MatchStateComponent>();
        out << YAML::Key << "MatchStateComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Phase" << YAML::Value << MatchPhaseToString(c.phase);
        out << YAML::Key << "Period" << YAML::Value << c.period;
        out << YAML::Key << "PeriodCount" << YAML::Value << c.periodCount;
        out << YAML::Key << "PeriodTimeRemaining" << YAML::Value << c.periodTimeRemaining;
        out << YAML::Key << "PhaseTimer" << YAML::Value << c.phaseTimer;
        out << YAML::Key << "HomeScore" << YAML::Value << c.homeScore;
        out << YAML::Key << "AwayScore" << YAML::Value << c.awayScore;
        out << YAML::Key << "ClockRunning" << YAML::Value << c.clockRunning;
        out << YAML::Key << "MatchInitialized" << YAML::Value << c.matchInitialized;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<TeamStateComponent>()) {
        const auto& c = entity.GetComponent<TeamStateComponent>();
        out << YAML::Key << "TeamStateComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Team" << YAML::Value << GameplayTeamToString(c.team);
        out << YAML::Key << "Score" << YAML::Value << c.score;
        EmitUUIDList(out, "Players", c.players);
        EmitUUID(out, "Goalie", c.goalie);
        out << YAML::EndMap;
    }
    if (entity.HasComponent<ScoreComponent>()) {
        const auto& c = entity.GetComponent<ScoreComponent>();
        out << YAML::Key << "ScoreComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "HomeScore" << YAML::Value << c.homeScore;
        out << YAML::Key << "AwayScore" << YAML::Value << c.awayScore;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<PlayerComponent>()) {
        const auto& c = entity.GetComponent<PlayerComponent>();
        out << YAML::Key << "PlayerComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "PlayerIndex" << YAML::Value << c.playerIndex;
        out << YAML::Key << "Slot" << YAML::Value << PlayerSlotToString(c.slot);
        out << YAML::Key << "Team" << YAML::Value << GameplayTeamToString(c.team);
        out << YAML::Key << "Role" << YAML::Value << GameplayRoleToString(c.role);
        out << YAML::Key << "ControlledByLocalInput" << YAML::Value << c.controlledByLocalInput;
        out << YAML::Key << "ControlledByAI" << YAML::Value << c.controlledByAI;
        out << YAML::Key << "ActiveInMatch" << YAML::Value << c.activeInMatch;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<SkaterComponent>()) {
        const auto& c = entity.GetComponent<SkaterComponent>();
        out << YAML::Key << "SkaterComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "MaxSpeed" << YAML::Value << c.maxSpeed;
        out << YAML::Key << "Acceleration" << YAML::Value << c.acceleration;
        out << YAML::Key << "Deceleration" << YAML::Value << c.deceleration;
        out << YAML::Key << "TurnSpeedDegrees" << YAML::Value << c.turnSpeedDegrees;
        out << YAML::Key << "HasPuck" << YAML::Value << c.hasPuck;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<GoalieComponent>()) {
        const auto& c = entity.GetComponent<GoalieComponent>();
        out << YAML::Key << "GoalieComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "MaxSpeed" << YAML::Value << c.maxSpeed;
        out << YAML::Key << "Acceleration" << YAML::Value << c.acceleration;
        out << YAML::Key << "SaveReachRadius" << YAML::Value << c.saveReachRadius;
        out << YAML::Key << "LockToCrease" << YAML::Value << c.lockToCrease;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<PlayerRuntimeComponent>()) {
        const auto& c = entity.GetComponent<PlayerRuntimeComponent>();
        out << YAML::Key << "PlayerRuntimeComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Velocity" << YAML::Value << c.velocity;
        out << YAML::Key << "FacingDirection" << YAML::Value << c.facingDirection;
        out << YAML::Key << "MoveTarget" << YAML::Value << c.moveTarget;
        out << YAML::Key << "BoostCooldown" << YAML::Value << c.boostCooldown;
        out << YAML::Key << "GoalieBoostCharges" << YAML::Value << c.goalieBoostCharges;
        out << YAML::Key << "GoalieBoostRechargeTimer" << YAML::Value << c.goalieBoostRechargeTimer;
        out << YAML::Key << "ShieldTimer" << YAML::Value << c.shieldTimer;
        out << YAML::Key << "ShieldCooldown" << YAML::Value << c.shieldCooldown;
        out << YAML::Key << "LastBrakePressedTime" << YAML::Value << c.lastBrakePressedTime;
        out << YAML::Key << "StealCooldown" << YAML::Value << c.stealCooldown;
        out << YAML::Key << "HasMoveTarget" << YAML::Value << c.hasMoveTarget;
        out << YAML::Key << "ShieldActive" << YAML::Value << c.shieldActive;
        out << YAML::Key << "InputEnabled" << YAML::Value << c.inputEnabled;
        out << YAML::Key << "MovementEnabled" << YAML::Value << c.movementEnabled;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<PuckGameplayComponent>()) {
        const auto& c = entity.GetComponent<PuckGameplayComponent>();
        out << YAML::Key << "PuckGameplayComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "State" << YAML::Value << PuckStateToString(c.state);
        EmitUUID(out, "PossessingPlayer", c.possessingPlayer);
        EmitUUID(out, "LastTouchedPlayer", c.lastTouchedPlayer);
        EmitUUID(out, "ShotIgnorePlayer", c.shotIgnorePlayer);
        out << YAML::Key << "LastTouchedTeam" << YAML::Value << GameplayTeamToString(c.lastTouchedTeam);
        out << YAML::Key << "TimeSinceLastTouch" << YAML::Value << c.timeSinceLastTouch;
        out << YAML::Key << "ShotIgnoreTimer" << YAML::Value << c.shotIgnoreTimer;
        out << YAML::Key << "InPlay" << YAML::Value << c.inPlay;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<PuckRuntimeComponent>()) {
        const auto& c = entity.GetComponent<PuckRuntimeComponent>();
        out << YAML::Key << "PuckRuntimeComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Velocity" << YAML::Value << c.velocity;
        out << YAML::Key << "TargetPosition" << YAML::Value << c.targetPosition;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<PossessionComponent>()) {
        const auto& c = entity.GetComponent<PossessionComponent>();
        out << YAML::Key << "PossessionComponent" << YAML::Value << YAML::BeginMap;
        EmitUUID(out, "PossessingPlayer", c.possessingPlayer);
        out << YAML::Key << "Team" << YAML::Value << GameplayTeamToString(c.team);
        out << YAML::EndMap;
    }
    if (entity.HasComponent<StickComponent>()) {
        const auto& c = entity.GetComponent<StickComponent>();
        out << YAML::Key << "StickComponent" << YAML::Value << YAML::BeginMap;
        EmitUUID(out, "OwnerPlayer", c.ownerPlayer);
        out << YAML::Key << "Reach" << YAML::Value << c.reach;
        out << YAML::Key << "Width" << YAML::Value << c.width;
        out << YAML::Key << "LocalOffset" << YAML::Value << c.localOffset;
        out << YAML::Key << "CanControlPuck" << YAML::Value << c.canControlPuck;
        out << YAML::Key << "CanShoot" << YAML::Value << c.canShoot;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<ShotComponent>()) {
        const auto& c = entity.GetComponent<ShotComponent>();
        out << YAML::Key << "ShotComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Charge" << YAML::Value << c.charge;
        out << YAML::Key << "Charging" << YAML::Value << c.charging;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<GoalGameplayComponent>()) {
        const auto& c = entity.GetComponent<GoalGameplayComponent>();
        out << YAML::Key << "GoalGameplayComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "ScoringTeam" << YAML::Value << GameplayTeamToString(c.scoringTeam);
        out << YAML::Key << "DefendingTeam" << YAML::Value << GameplayTeamToString(c.defendingTeam);
        out << YAML::Key << "Enabled" << YAML::Value << c.enabled;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<OutOfPlayComponent>()) {
        const auto& c = entity.GetComponent<OutOfPlayComponent>();
        out << YAML::Key << "OutOfPlayComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "ResetPosition" << YAML::Value << c.resetPosition;
        out << YAML::Key << "MinY" << YAML::Value << c.minY;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<FaceoffComponent>()) {
        const auto& c = entity.GetComponent<FaceoffComponent>();
        out << YAML::Key << "FaceoffComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "CauseTeam" << YAML::Value << GameplayTeamToString(c.causeTeam);
        out << YAML::Key << "SpawnSequence" << YAML::Value << c.spawnSequence;
        out << YAML::Key << "Timer" << YAML::Value << c.timer;
        out << YAML::Key << "Locked" << YAML::Value << c.locked;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<RespawnComponent>()) {
        const auto& c = entity.GetComponent<RespawnComponent>();
        out << YAML::Key << "RespawnComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Pending" << YAML::Value << c.pending;
        out << YAML::Key << "Timer" << YAML::Value << c.timer;
        out << YAML::Key << "TargetPosition" << YAML::Value << c.targetPosition;
        out << YAML::EndMap;
    }
}

void DeserializeGameplay(Entity entity, const YAML::Node& node) {
    if (const auto n = node["MatchStateComponent"]) {
        MatchStateComponent c;
        if (n["Phase"]) {
            MatchPhaseFromString(n["Phase"].as<std::string>().c_str(), c.phase);
        }
        if (n["Period"]) c.period = n["Period"].as<uint32_t>();
        if (n["PeriodCount"]) c.periodCount = n["PeriodCount"].as<uint32_t>();
        if (n["PeriodTimeRemaining"]) c.periodTimeRemaining = n["PeriodTimeRemaining"].as<float>();
        if (n["PhaseTimer"]) c.phaseTimer = n["PhaseTimer"].as<float>();
        if (n["HomeScore"]) c.homeScore = n["HomeScore"].as<uint32_t>();
        if (n["AwayScore"]) c.awayScore = n["AwayScore"].as<uint32_t>();
        if (n["ClockRunning"]) c.clockRunning = n["ClockRunning"].as<bool>();
        if (n["MatchInitialized"]) c.matchInitialized = n["MatchInitialized"].as<bool>();
        entity.AddOrReplaceComponent<MatchStateComponent>(c);
    }
    if (const auto n = node["TeamStateComponent"]) {
        TeamStateComponent c;
        if (n["Team"]) GameplayTeamFromString(n["Team"].as<std::string>().c_str(), c.team);
        if (n["Score"]) c.score = n["Score"].as<uint32_t>();
        ReadUUIDList(n, "Players", c.players);
        c.goalie = ReadUUID(n, "Goalie");
        entity.AddOrReplaceComponent<TeamStateComponent>(c);
    }
    if (const auto n = node["ScoreComponent"]) {
        ScoreComponent c;
        if (n["HomeScore"]) c.homeScore = n["HomeScore"].as<uint32_t>();
        if (n["AwayScore"]) c.awayScore = n["AwayScore"].as<uint32_t>();
        entity.AddOrReplaceComponent<ScoreComponent>(c);
    }
    if (const auto n = node["PlayerComponent"]) {
        PlayerComponent c;
        if (n["PlayerIndex"]) c.playerIndex = n["PlayerIndex"].as<uint32_t>();
        if (n["Slot"]) PlayerSlotFromString(n["Slot"].as<std::string>().c_str(), c.slot);
        if (n["Team"]) GameplayTeamFromString(n["Team"].as<std::string>().c_str(), c.team);
        if (n["Role"]) GameplayRoleFromString(n["Role"].as<std::string>().c_str(), c.role);
        if (n["ControlledByLocalInput"]) c.controlledByLocalInput = n["ControlledByLocalInput"].as<bool>();
        if (n["ControlledByAI"]) c.controlledByAI = n["ControlledByAI"].as<bool>();
        if (n["ActiveInMatch"]) c.activeInMatch = n["ActiveInMatch"].as<bool>();
        entity.AddOrReplaceComponent<PlayerComponent>(c);
    }
    if (const auto n = node["SkaterComponent"]) {
        SkaterComponent c;
        if (n["MaxSpeed"]) c.maxSpeed = n["MaxSpeed"].as<float>();
        if (n["Acceleration"]) c.acceleration = n["Acceleration"].as<float>();
        if (n["Deceleration"]) c.deceleration = n["Deceleration"].as<float>();
        if (n["TurnSpeedDegrees"]) c.turnSpeedDegrees = n["TurnSpeedDegrees"].as<float>();
        if (n["HasPuck"]) c.hasPuck = n["HasPuck"].as<bool>();
        entity.AddOrReplaceComponent<SkaterComponent>(c);
    }
    if (const auto n = node["GoalieComponent"]) {
        GoalieComponent c;
        if (n["MaxSpeed"]) c.maxSpeed = n["MaxSpeed"].as<float>();
        if (n["Acceleration"]) c.acceleration = n["Acceleration"].as<float>();
        if (n["SaveReachRadius"]) c.saveReachRadius = n["SaveReachRadius"].as<float>();
        if (n["LockToCrease"]) c.lockToCrease = n["LockToCrease"].as<bool>();
        entity.AddOrReplaceComponent<GoalieComponent>(c);
    }
    if (const auto n = node["PlayerRuntimeComponent"]) {
        PlayerRuntimeComponent c;
        ReadVec3(n["Velocity"], c.velocity);
        ReadVec3(n["FacingDirection"], c.facingDirection);
        ReadVec3(n["MoveTarget"], c.moveTarget);
        if (n["BoostCooldown"]) c.boostCooldown = n["BoostCooldown"].as<float>();
        if (n["GoalieBoostCharges"]) c.goalieBoostCharges = n["GoalieBoostCharges"].as<uint32_t>();
        if (n["GoalieBoostRechargeTimer"]) c.goalieBoostRechargeTimer = n["GoalieBoostRechargeTimer"].as<float>();
        if (n["ShieldTimer"]) c.shieldTimer = n["ShieldTimer"].as<float>();
        if (n["ShieldCooldown"]) c.shieldCooldown = n["ShieldCooldown"].as<float>();
        if (n["LastBrakePressedTime"]) c.lastBrakePressedTime = n["LastBrakePressedTime"].as<float>();
        if (n["StealCooldown"]) c.stealCooldown = n["StealCooldown"].as<float>();
        if (n["HasMoveTarget"]) c.hasMoveTarget = n["HasMoveTarget"].as<bool>();
        if (n["ShieldActive"]) c.shieldActive = n["ShieldActive"].as<bool>();
        if (n["InputEnabled"]) c.inputEnabled = n["InputEnabled"].as<bool>();
        if (n["MovementEnabled"]) c.movementEnabled = n["MovementEnabled"].as<bool>();
        entity.AddOrReplaceComponent<PlayerRuntimeComponent>(c);
    }
    if (const auto n = node["PuckGameplayComponent"]) {
        PuckGameplayComponent c;
        if (n["State"]) PuckStateFromString(n["State"].as<std::string>().c_str(), c.state);
        c.possessingPlayer = ReadUUID(n, "PossessingPlayer");
        c.lastTouchedPlayer = ReadUUID(n, "LastTouchedPlayer");
        c.shotIgnorePlayer = ReadUUID(n, "ShotIgnorePlayer");
        if (n["LastTouchedTeam"]) GameplayTeamFromString(n["LastTouchedTeam"].as<std::string>().c_str(), c.lastTouchedTeam);
        if (n["TimeSinceLastTouch"]) c.timeSinceLastTouch = n["TimeSinceLastTouch"].as<float>();
        if (n["ShotIgnoreTimer"]) c.shotIgnoreTimer = n["ShotIgnoreTimer"].as<float>();
        if (n["InPlay"]) c.inPlay = n["InPlay"].as<bool>();
        entity.AddOrReplaceComponent<PuckGameplayComponent>(c);
    }
    if (const auto n = node["PuckRuntimeComponent"]) {
        PuckRuntimeComponent c;
        ReadVec3(n["Velocity"], c.velocity);
        ReadVec3(n["TargetPosition"], c.targetPosition);
        entity.AddOrReplaceComponent<PuckRuntimeComponent>(c);
    }
    if (const auto n = node["PossessionComponent"]) {
        PossessionComponent c;
        c.possessingPlayer = ReadUUID(n, "PossessingPlayer");
        if (n["Team"]) GameplayTeamFromString(n["Team"].as<std::string>().c_str(), c.team);
        entity.AddOrReplaceComponent<PossessionComponent>(c);
    }
    if (const auto n = node["StickComponent"]) {
        StickComponent c;
        c.ownerPlayer = ReadUUID(n, "OwnerPlayer");
        if (n["Reach"]) c.reach = n["Reach"].as<float>();
        if (n["Width"]) c.width = n["Width"].as<float>();
        ReadVec3(n["LocalOffset"], c.localOffset);
        if (n["CanControlPuck"]) c.canControlPuck = n["CanControlPuck"].as<bool>();
        if (n["CanShoot"]) c.canShoot = n["CanShoot"].as<bool>();
        entity.AddOrReplaceComponent<StickComponent>(c);
    }
    if (const auto n = node["ShotComponent"]) {
        ShotComponent c;
        if (n["Charge"]) c.charge = n["Charge"].as<float>();
        if (n["Charging"]) c.charging = n["Charging"].as<bool>();
        entity.AddOrReplaceComponent<ShotComponent>(c);
    }
    if (const auto n = node["GoalGameplayComponent"]) {
        GoalGameplayComponent c;
        if (n["ScoringTeam"]) GameplayTeamFromString(n["ScoringTeam"].as<std::string>().c_str(), c.scoringTeam);
        if (n["DefendingTeam"]) GameplayTeamFromString(n["DefendingTeam"].as<std::string>().c_str(), c.defendingTeam);
        if (n["Enabled"]) c.enabled = n["Enabled"].as<bool>();
        entity.AddOrReplaceComponent<GoalGameplayComponent>(c);
    }
    if (const auto n = node["OutOfPlayComponent"]) {
        OutOfPlayComponent c;
        ReadVec3(n["ResetPosition"], c.resetPosition);
        if (n["MinY"]) c.minY = n["MinY"].as<float>();
        entity.AddOrReplaceComponent<OutOfPlayComponent>(c);
    }
    if (const auto n = node["FaceoffComponent"]) {
        FaceoffComponent c;
        if (n["CauseTeam"]) GameplayTeamFromString(n["CauseTeam"].as<std::string>().c_str(), c.causeTeam);
        if (n["SpawnSequence"]) c.spawnSequence = n["SpawnSequence"].as<uint32_t>();
        if (n["Timer"]) c.timer = n["Timer"].as<float>();
        if (n["Locked"]) c.locked = n["Locked"].as<bool>();
        entity.AddOrReplaceComponent<FaceoffComponent>(c);
    }
    if (const auto n = node["RespawnComponent"]) {
        RespawnComponent c;
        if (n["Pending"]) c.pending = n["Pending"].as<bool>();
        if (n["Timer"]) c.timer = n["Timer"].as<float>();
        ReadVec3(n["TargetPosition"], c.targetPosition);
        entity.AddOrReplaceComponent<RespawnComponent>(c);
    }
}

void RegisterMetadata() {
    ComponentRegistry& registry = ComponentRegistry::Get();
    if (registry.FindByName("PlayerComponent") != nullptr) {
        return;
    }

    {
        ComponentMetadata md;
        md.name = "PlayerComponent";
        md.displayName = "Player";
        md.category = "Gameplay";
        md.fields.push_back(MakeField("PlayerIndex", FieldType::Int, offsetof(PlayerComponent, playerIndex)));
        md.fields.push_back(MakeSlotField("Slot", offsetof(PlayerComponent, slot)));
        md.fields.push_back(MakeTeamField("Team", offsetof(PlayerComponent, team)));
        md.fields.push_back(MakeRoleField("Role", offsetof(PlayerComponent, role)));
        md.fields.push_back(MakeField("ControlledByLocalInput", FieldType::Bool, offsetof(PlayerComponent, controlledByLocalInput)));
        md.fields.push_back(MakeField("ControlledByAI", FieldType::Bool, offsetof(PlayerComponent, controlledByAI)));
        md.fields.push_back(MakeField("ActiveInMatch", FieldType::Bool, offsetof(PlayerComponent, activeInMatch)));
        registry.RegisterComponent<PlayerComponent>(std::move(md));
    }
    {
        ComponentMetadata md;
        md.name = "SkaterComponent";
        md.displayName = "Skater";
        md.category = "Gameplay";
        md.fields.push_back(MakeField("MaxSpeed", FieldType::Float, offsetof(SkaterComponent, maxSpeed)));
        md.fields.push_back(MakeField("Acceleration", FieldType::Float, offsetof(SkaterComponent, acceleration)));
        md.fields.push_back(MakeField("Deceleration", FieldType::Float, offsetof(SkaterComponent, deceleration)));
        md.fields.push_back(MakeField("TurnSpeedDegrees", FieldType::Float, offsetof(SkaterComponent, turnSpeedDegrees)));
        md.fields.push_back(MakeField("HasPuck", FieldType::Bool, offsetof(SkaterComponent, hasPuck)));
        registry.RegisterComponent<SkaterComponent>(std::move(md));
    }
    {
        ComponentMetadata md;
        md.name = "GoalieComponent";
        md.displayName = "Goalie";
        md.category = "Gameplay";
        md.fields.push_back(MakeField("MaxSpeed", FieldType::Float, offsetof(GoalieComponent, maxSpeed)));
        md.fields.push_back(MakeField("Acceleration", FieldType::Float, offsetof(GoalieComponent, acceleration)));
        md.fields.push_back(MakeField("SaveReachRadius", FieldType::Float, offsetof(GoalieComponent, saveReachRadius)));
        md.fields.push_back(MakeField("LockToCrease", FieldType::Bool, offsetof(GoalieComponent, lockToCrease)));
        registry.RegisterComponent<GoalieComponent>(std::move(md));
    }
    {
        ComponentMetadata md;
        md.name = "MatchStateComponent";
        md.displayName = "Match State";
        md.category = "Gameplay";
        md.fields.push_back(MakePhaseField(offsetof(MatchStateComponent, phase)));
        md.fields.push_back(MakeField("Period", FieldType::Int, offsetof(MatchStateComponent, period)));
        md.fields.push_back(MakeField("PeriodCount", FieldType::Int, offsetof(MatchStateComponent, periodCount)));
        md.fields.push_back(MakeField("PeriodTimeRemaining", FieldType::Float, offsetof(MatchStateComponent, periodTimeRemaining)));
        md.fields.push_back(MakeField("ClockRunning", FieldType::Bool, offsetof(MatchStateComponent, clockRunning)));
        registry.RegisterComponent<MatchStateComponent>(std::move(md));
    }
    {
        ComponentMetadata md;
        md.name = "TeamStateComponent";
        md.displayName = "Team State";
        md.category = "Gameplay";
        md.fields.push_back(MakeTeamField("Team", offsetof(TeamStateComponent, team)));
        md.fields.push_back(MakeField("Score", FieldType::Int, offsetof(TeamStateComponent, score)));
        registry.RegisterComponent<TeamStateComponent>(std::move(md));
    }
    {
        ComponentMetadata md;
        md.name = "PuckGameplayComponent";
        md.displayName = "Puck Gameplay";
        md.category = "Gameplay";
        md.fields.push_back(MakePuckStateField(offsetof(PuckGameplayComponent, state)));
        md.fields.push_back(MakeTeamField("LastTouchedTeam", offsetof(PuckGameplayComponent, lastTouchedTeam)));
        md.fields.push_back(MakeField("TimeSinceLastTouch", FieldType::Float, offsetof(PuckGameplayComponent, timeSinceLastTouch)));
        md.fields.push_back(MakeField("ShotIgnoreTimer", FieldType::Float, offsetof(PuckGameplayComponent, shotIgnoreTimer)));
        md.fields.push_back(MakeField("InPlay", FieldType::Bool, offsetof(PuckGameplayComponent, inPlay)));
        registry.RegisterComponent<PuckGameplayComponent>(std::move(md));
    }
    {
        ComponentMetadata md;
        md.name = "StickComponent";
        md.displayName = "Stick";
        md.category = "Gameplay";
        md.fields.push_back(MakeField("OwnerPlayer", FieldType::UUID, offsetof(StickComponent, ownerPlayer)));
        md.fields.push_back(MakeField("Reach", FieldType::Float, offsetof(StickComponent, reach)));
        md.fields.push_back(MakeField("Width", FieldType::Float, offsetof(StickComponent, width)));
        md.fields.push_back(MakeField("LocalOffset", FieldType::Vec3, offsetof(StickComponent, localOffset)));
        registry.RegisterComponent<StickComponent>(std::move(md));
    }
    {
        ComponentMetadata md;
        md.name = "GoalGameplayComponent";
        md.displayName = "Goal Gameplay";
        md.category = "Gameplay";
        md.fields.push_back(MakeTeamField("ScoringTeam", offsetof(GoalGameplayComponent, scoringTeam)));
        md.fields.push_back(MakeTeamField("DefendingTeam", offsetof(GoalGameplayComponent, defendingTeam)));
        md.fields.push_back(MakeField("Enabled", FieldType::Bool, offsetof(GoalGameplayComponent, enabled)));
        registry.RegisterComponent<GoalGameplayComponent>(std::move(md));
    }

    registry.RegisterComponent<PlayerRuntimeComponent>({"PlayerRuntimeComponent", "Player Runtime", "Gameplay"});
    registry.RegisterComponent<PuckRuntimeComponent>({"PuckRuntimeComponent", "Puck Runtime", "Gameplay"});
    registry.RegisterComponent<PossessionComponent>({"PossessionComponent", "Possession", "Gameplay"});
    registry.RegisterComponent<ShotComponent>({"ShotComponent", "Shot", "Gameplay"});
    registry.RegisterComponent<ScoreComponent>({"ScoreComponent", "Score", "Gameplay"});
    registry.RegisterComponent<OutOfPlayComponent>({"OutOfPlayComponent", "Out Of Play", "Gameplay"});
    {
        ComponentMetadata md;
        md.name = "FaceoffComponent";
        md.displayName = "Faceoff";
        md.category = "Gameplay";
        md.fields.push_back(MakeTeamField("CauseTeam", offsetof(FaceoffComponent, causeTeam)));
        md.fields.push_back(MakeField("SpawnSequence", FieldType::Int, offsetof(FaceoffComponent, spawnSequence)));
        md.fields.push_back(MakeField("Timer", FieldType::Float, offsetof(FaceoffComponent, timer)));
        md.fields.push_back(MakeField("Locked", FieldType::Bool, offsetof(FaceoffComponent, locked)));
        registry.RegisterComponent<FaceoffComponent>(std::move(md));
    }
    registry.RegisterComponent<RespawnComponent>({"RespawnComponent", "Respawn", "Gameplay"});
}

} // namespace

const char* PuckStateToString(PuckState state) {
    switch (state) {
    case PuckState::Loose: return "Loose";
    case PuckState::Possessed: return "Possessed";
    case PuckState::Shot: return "Shot";
    case PuckState::Frozen: return "Frozen";
    case PuckState::Resetting: return "Resetting";
    }
    return "Loose";
}

bool PuckStateFromString(const char* text, PuckState& outState) {
    constexpr PuckState kStates[] = {PuckState::Loose, PuckState::Possessed, PuckState::Shot,
                                     PuckState::Frozen, PuckState::Resetting};
    for (PuckState state : kStates) {
        if (std::string(text) == PuckStateToString(state)) {
            outState = state;
            return true;
        }
    }
    return false;
}

void RegisterGameplayComponents() {
    RegisterMetadata();

    static bool s_SerializationRegistered = false;
    if (!s_SerializationRegistered) {
        ComponentSerializer::RegisterExternal(SerializeGameplay, DeserializeGameplay);
        s_SerializationRegistered = true;
    }

    RegisterGameplayValidation();
}

} // namespace Hockey
