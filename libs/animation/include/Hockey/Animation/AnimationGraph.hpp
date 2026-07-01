#pragma once

#include "Hockey/Animation/AnimationComponents.hpp"
#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Core/Result.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Hockey {

enum class AnimationConditionOperator : std::uint8_t {
    BoolEquals,
    FloatGreaterThan,
    FloatGreaterOrEqual,
    FloatLessThan,
    FloatLessOrEqual
};

struct AnimationTransitionCondition {
    std::string parameterName;
    AnimationConditionOperator op = AnimationConditionOperator::BoolEquals;
    bool boolValue = false;
    float floatValue = 0.0f;

    static AnimationTransitionCondition BoolEquals(std::string parameterName, bool value);
    static AnimationTransitionCondition FloatGreaterThan(std::string parameterName, float value);
    static AnimationTransitionCondition FloatGreaterOrEqual(std::string parameterName, float value);
    static AnimationTransitionCondition FloatLessThan(std::string parameterName, float value);
    static AnimationTransitionCondition FloatLessOrEqual(std::string parameterName, float value);

    bool Evaluate(const AnimationParameterComponent& parameters) const;
};

struct AnimationGraphTransition {
    std::string targetState;
    std::vector<AnimationTransitionCondition> conditions;

    bool AreConditionsMet(const AnimationParameterComponent& parameters) const;
};

struct AnimationGraphState {
    std::string name;
    AssetID clipAsset;
    bool looping = true;
    float playbackSpeed = 1.0f;
    float blendTimeSeconds = 0.0f;
    std::vector<AnimationGraphTransition> transitions;
};

struct AnimationGraph {
    AssetID id;
    std::string name;
    std::string initialState;
    std::vector<AnimationGraphState> states;

    AnimationGraphState* FindState(std::string_view stateName);
    const AnimationGraphState* FindState(std::string_view stateName) const;
    bool HasRequiredHockeyStates() const;
    Status Validate() const;

    static std::array<std::string_view, 11> RequiredHockeyStateNames();
};

} // namespace Hockey
