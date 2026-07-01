#include "Hockey/Animation/AnimationGraph.hpp"

#include <algorithm>
#include <string>
#include <unordered_set>

namespace Hockey {
namespace {

bool GetBoolParameter(const AnimationParameterComponent& parameters, std::string_view name, bool& outValue) {
    if (name == "hasPuck") {
        outValue = parameters.hasPuck;
    } else if (name == "shooting") {
        outValue = parameters.shooting;
    } else if (name == "stealing") {
        outValue = parameters.stealing;
    } else if (name == "goalieAction") {
        outValue = parameters.goalieAction;
    } else if (name == "celebrating") {
        outValue = parameters.celebrating;
    } else {
        return false;
    }
    return true;
}

bool GetFloatParameter(const AnimationParameterComponent& parameters, std::string_view name, float& outValue) {
    if (name == "speed") {
        outValue = parameters.speed;
    } else if (name == "shotChargeRatio") {
        outValue = parameters.shotChargeRatio;
    } else {
        return false;
    }
    return true;
}

} // namespace

AnimationTransitionCondition AnimationTransitionCondition::BoolEquals(std::string parameterName, bool value) {
    AnimationTransitionCondition condition;
    condition.parameterName = std::move(parameterName);
    condition.op = AnimationConditionOperator::BoolEquals;
    condition.boolValue = value;
    return condition;
}

AnimationTransitionCondition AnimationTransitionCondition::FloatGreaterThan(std::string parameterName, float value) {
    AnimationTransitionCondition condition;
    condition.parameterName = std::move(parameterName);
    condition.op = AnimationConditionOperator::FloatGreaterThan;
    condition.floatValue = value;
    return condition;
}

AnimationTransitionCondition AnimationTransitionCondition::FloatGreaterOrEqual(std::string parameterName, float value) {
    AnimationTransitionCondition condition;
    condition.parameterName = std::move(parameterName);
    condition.op = AnimationConditionOperator::FloatGreaterOrEqual;
    condition.floatValue = value;
    return condition;
}

AnimationTransitionCondition AnimationTransitionCondition::FloatLessThan(std::string parameterName, float value) {
    AnimationTransitionCondition condition;
    condition.parameterName = std::move(parameterName);
    condition.op = AnimationConditionOperator::FloatLessThan;
    condition.floatValue = value;
    return condition;
}

AnimationTransitionCondition AnimationTransitionCondition::FloatLessOrEqual(std::string parameterName, float value) {
    AnimationTransitionCondition condition;
    condition.parameterName = std::move(parameterName);
    condition.op = AnimationConditionOperator::FloatLessOrEqual;
    condition.floatValue = value;
    return condition;
}

bool AnimationTransitionCondition::Evaluate(const AnimationParameterComponent& parameters) const {
    if (op == AnimationConditionOperator::BoolEquals) {
        bool value = false;
        return GetBoolParameter(parameters, parameterName, value) && value == boolValue;
    }

    float value = 0.0f;
    if (!GetFloatParameter(parameters, parameterName, value)) {
        return false;
    }

    switch (op) {
    case AnimationConditionOperator::FloatGreaterThan:
        return value > floatValue;
    case AnimationConditionOperator::FloatGreaterOrEqual:
        return value >= floatValue;
    case AnimationConditionOperator::FloatLessThan:
        return value < floatValue;
    case AnimationConditionOperator::FloatLessOrEqual:
        return value <= floatValue;
    case AnimationConditionOperator::BoolEquals:
        break;
    }

    return false;
}

bool AnimationGraphTransition::AreConditionsMet(const AnimationParameterComponent& parameters) const {
    return std::all_of(conditions.begin(), conditions.end(), [&](const AnimationTransitionCondition& condition) {
        return condition.Evaluate(parameters);
    });
}

AnimationGraphState* AnimationGraph::FindState(std::string_view stateName) {
    const auto it = std::find_if(states.begin(), states.end(), [&](const AnimationGraphState& state) {
        return state.name == stateName;
    });
    return it == states.end() ? nullptr : &(*it);
}

const AnimationGraphState* AnimationGraph::FindState(std::string_view stateName) const {
    const auto it = std::find_if(states.begin(), states.end(), [&](const AnimationGraphState& state) {
        return state.name == stateName;
    });
    return it == states.end() ? nullptr : &(*it);
}

bool AnimationGraph::HasRequiredHockeyStates() const {
    for (std::string_view stateName : RequiredHockeyStateNames()) {
        if (FindState(stateName) == nullptr) {
            return false;
        }
    }
    return true;
}

Status AnimationGraph::Validate() const {
    if (states.empty()) {
        return Status::Fail("animation graph must contain at least one state");
    }

    std::unordered_set<std::string> names;
    for (const AnimationGraphState& state : states) {
        if (state.name.empty()) {
            return Status::Fail("animation graph state name must not be empty");
        }
        if (!names.insert(state.name).second) {
            return Status::Fail("animation graph state names must be unique");
        }
        if (state.playbackSpeed <= 0.0f) {
            return Status::Fail("animation graph state playback speed must be positive");
        }
        if (state.blendTimeSeconds < 0.0f) {
            return Status::Fail("animation graph state blend time must not be negative");
        }
    }

    const std::string_view initial = initialState.empty() ? std::string_view{states.front().name} : std::string_view{initialState};
    if (FindState(initial) == nullptr) {
        return Status::Fail("animation graph initial state is missing");
    }

    for (const AnimationGraphState& state : states) {
        for (const AnimationGraphTransition& transition : state.transitions) {
            if (transition.targetState.empty()) {
                return Status::Fail("animation graph transition target must not be empty");
            }
            if (FindState(transition.targetState) == nullptr) {
                return Status::Fail("animation graph transition targets a missing state");
            }
        }
    }

    return Status::Ok();
}

std::array<std::string_view, 11> AnimationGraph::RequiredHockeyStateNames() {
    return {"Idle",
            "SkateForward",
            "SkateBackward",
            "Turn",
            "StickHandle",
            "Shoot",
            "Steal",
            "GoalieIdle",
            "GoalieMove",
            "GoalieSave",
            "Celebrate"};
}

} // namespace Hockey
