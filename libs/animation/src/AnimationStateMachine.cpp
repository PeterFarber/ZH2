#include "Hockey/Animation/AnimationStateMachine.hpp"

namespace Hockey {

const AnimationGraphState* AnimationStateMachine::ResolveCurrentState(const AnimationGraph& graph, AnimatorComponent& animator) {
    if (const AnimationGraphState* state = graph.FindState(animator.currentState); state != nullptr) {
        return state;
    }

    const std::string fallbackState = graph.initialState.empty() && !graph.states.empty() ? graph.states.front().name : graph.initialState;
    animator.currentState = fallbackState;
    animator.playbackTimeSeconds = 0.0f;
    return graph.FindState(animator.currentState);
}

AnimationStateTransitionResult AnimationStateMachine::EvaluateTransitions(const AnimationGraph& graph,
                                                                          const AnimationParameterComponent& parameters,
                                                                          AnimatorComponent& animator) {
    AnimationStateTransitionResult result;
    const AnimationGraphState* state = ResolveCurrentState(graph, animator);
    if (state == nullptr) {
        result.currentState = animator.currentState;
        return result;
    }

    for (const AnimationGraphTransition& transition : state->transitions) {
        if (!transition.AreConditionsMet(parameters)) {
            continue;
        }

        result.transitioned = transition.targetState != animator.currentState;
        result.previousState = animator.currentState;
        animator.currentState = transition.targetState;
        animator.playbackTimeSeconds = 0.0f;
        result.currentState = animator.currentState;
        return result;
    }

    result.currentState = animator.currentState;
    return result;
}

} // namespace Hockey
