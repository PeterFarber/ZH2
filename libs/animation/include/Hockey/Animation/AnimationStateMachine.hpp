#pragma once

#include "Hockey/Animation/AnimationComponents.hpp"
#include "Hockey/Animation/AnimationGraph.hpp"

#include <string>

namespace Hockey {

struct AnimationStateTransitionResult {
    bool transitioned = false;
    std::string previousState;
    std::string currentState;
};

class AnimationStateMachine {
public:
    static const AnimationGraphState* ResolveCurrentState(const AnimationGraph& graph, AnimatorComponent& animator);
    static AnimationStateTransitionResult EvaluateTransitions(const AnimationGraph& graph,
                                                              const AnimationParameterComponent& parameters,
                                                              AnimatorComponent& animator);
};

} // namespace Hockey
