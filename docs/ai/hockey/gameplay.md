# Hockey Gameplay

Use this guide for scoring, possession, shooting, passing, periods, restarts,
goalies, player actions, and hockey rule changes.

## Direction

ZH2 should prioritize responsive, readable hockey action over strict simulation
realism. Do not assume NHL-realistic rules or pacing unless the task explicitly
asks for them.

Preserve an arcade-friendly feel:

- quick turns
- clear shots
- readable collisions
- fast feedback
- tunable assists and timing

## Core Concepts

Treat these as first-class gameplay systems when they exist or are being
introduced:

- teams and player ownership
- puck possession or puck control state
- passing and receiving
- shooting, shot charging, slapshots, wrist shots, and deflections
- goals, scoring, restarts, and periods
- faceoffs or arcade-style restarts
- player contact and checking if enabled
- goalies and goalie behavior

## Boundaries

Gameplay logic should depend on simulation state, input commands, and rules
data. It must not depend directly on Vulkan, SDL windows, UI widgets, editor
state, networking, or local machine timing.

When changing gameplay code:

1. Identify the authoritative state being changed.
2. Check whether the behavior must be deterministic for replay or networking.
3. Use existing tuning/configuration mechanisms where practical.
4. Update focused tests where practical.
5. Avoid broad rewrites of unrelated systems.

## Tuning

Prefer tunable parameters over hardcoded realism:

- shot speed
- pass assist
- puck friction
- player acceleration
- turn speed
- stick pickup radius
- goalie reaction speed

## Completion Notes

Report:

- systems touched
- rules or behavior changed
- tuning values added or modified
- tests run
- deterministic simulation risks
