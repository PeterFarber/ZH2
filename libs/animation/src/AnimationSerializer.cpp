#include "Hockey/Animation/AnimationSerializer.hpp"

#include "Hockey/Animation/AnimationComponents.hpp"
#include "Hockey/ECS/ComponentSerializer.hpp"
#include "Hockey/ECS/Entity.hpp"

#include <cstdint>

namespace Hockey {

namespace {

void SerializeAnimationComponents(YAML::Emitter& out, Entity entity) {
    if (entity.HasComponent<AnimatorComponent>()) {
        const AnimatorComponent& animator = entity.GetComponent<AnimatorComponent>();
        out << YAML::Key << "AnimatorComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "AnimationGraphAsset" << YAML::Value << animator.animationGraphAsset;
        out << YAML::Key << "CurrentState" << YAML::Value << animator.currentState;
        out << YAML::Key << "PlaybackTimeSeconds" << YAML::Value << animator.playbackTimeSeconds;
        out << YAML::Key << "PlaybackSpeed" << YAML::Value << animator.playbackSpeed;
        out << YAML::Key << "Playing" << YAML::Value << animator.playing;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<AnimationParameterComponent>()) {
        const AnimationParameterComponent& parameters = entity.GetComponent<AnimationParameterComponent>();
        out << YAML::Key << "AnimationParameterComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Speed" << YAML::Value << parameters.speed;
        out << YAML::Key << "ShotChargeRatio" << YAML::Value << parameters.shotChargeRatio;
        out << YAML::Key << "HasPuck" << YAML::Value << parameters.hasPuck;
        out << YAML::Key << "Shooting" << YAML::Value << parameters.shooting;
        out << YAML::Key << "Stealing" << YAML::Value << parameters.stealing;
        out << YAML::Key << "GoalieAction" << YAML::Value << parameters.goalieAction;
        out << YAML::Key << "Celebrating" << YAML::Value << parameters.celebrating;
        out << YAML::EndMap;
    }
}

void DeserializeAnimationComponents(Entity entity, const YAML::Node& node) {
    if (const YAML::Node animatorNode = node["AnimatorComponent"]) {
        AnimatorComponent animator;
        if (animatorNode["AnimationGraphAsset"]) {
            animator.animationGraphAsset = animatorNode["AnimationGraphAsset"].as<std::uint64_t>();
        }
        if (animatorNode["CurrentState"]) {
            animator.currentState = animatorNode["CurrentState"].as<std::string>();
        }
        if (animatorNode["PlaybackTimeSeconds"]) {
            animator.playbackTimeSeconds = animatorNode["PlaybackTimeSeconds"].as<float>();
        }
        if (animatorNode["PlaybackSpeed"]) {
            animator.playbackSpeed = animatorNode["PlaybackSpeed"].as<float>();
        }
        if (animatorNode["Playing"]) {
            animator.playing = animatorNode["Playing"].as<bool>();
        }
        entity.AddComponent<AnimatorComponent>(animator);
    }

    if (const YAML::Node parametersNode = node["AnimationParameterComponent"]) {
        AnimationParameterComponent parameters;
        if (parametersNode["Speed"]) {
            parameters.speed = parametersNode["Speed"].as<float>();
        }
        if (parametersNode["ShotChargeRatio"]) {
            parameters.shotChargeRatio = parametersNode["ShotChargeRatio"].as<float>();
        }
        if (parametersNode["HasPuck"]) {
            parameters.hasPuck = parametersNode["HasPuck"].as<bool>();
        }
        if (parametersNode["Shooting"]) {
            parameters.shooting = parametersNode["Shooting"].as<bool>();
        }
        if (parametersNode["Stealing"]) {
            parameters.stealing = parametersNode["Stealing"].as<bool>();
        }
        if (parametersNode["GoalieAction"]) {
            parameters.goalieAction = parametersNode["GoalieAction"].as<bool>();
        }
        if (parametersNode["Celebrating"]) {
            parameters.celebrating = parametersNode["Celebrating"].as<bool>();
        }
        entity.AddComponent<AnimationParameterComponent>(parameters);
    }
}

} // namespace

void RegisterAnimationComponentSerialization() {
    ComponentSerializer::RegisterExternal(SerializeAnimationComponents, DeserializeAnimationComponents);
}

} // namespace Hockey
