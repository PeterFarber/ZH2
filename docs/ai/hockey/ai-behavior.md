# Hockey AI Behavior

Use this guide for CPU skaters, goalies, positioning, steering, stealing,
shooting decisions, team tactics, and difficulty tuning.

## Goals

AI behavior should be readable, tunable, and modular. Prefer simple reliable
behavior over complex logic that is difficult to debug.

## Behavior Areas

AI work may include:

- puck chasing
- offensive positioning
- defensive positioning
- stealing decisions
- shooting decisions
- goalie positioning
- steering and avoidance
- difficulty tuning
- team tactics

## Boundaries

Do not bury high-level decisions inside low-level movement or physics code.
Prefer this separation:

- decision logic: what the player wants to do
- steering/movement: how the player moves toward that goal
- physics: authoritative movement and collision response

AI decisions should operate on simulation/game state, not renderer state.

## Tuning

Difficulty should come from tunable parameters where practical:

- reaction delay
- positioning accuracy
- steal pressure likelihood
- shot selection
- aggression
- goalie reaction speed
- error/noise amount

## Completion Notes

Report:

- behavior added or changed
- systems touched
- tuning parameters modified
- test or debug scenario used
- known edge cases
