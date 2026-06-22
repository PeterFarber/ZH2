# Hockey Gameplay

Use this guide for scoring, possession, shooting, passing, periods, restarts,
goalies, player actions, and hockey rule changes. `GAMEPLAY.md` is the
player-facing action design source; keep this guide, Phase 7 plan/rules, and
Phase 7 status synchronized when the gameplay design changes.

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
- pregame countdown
- waypoint movement and waypoint clearing
- contextual steal-or-shoot input
- skater boost impulses
- goalie boost charges
- goalie shield
- role-specific collider and trigger behavior

## Current Action Design

Follow `GAMEPLAY.md` unless a task explicitly changes it:

- 4v4 match: 3 skaters plus 1 goalie per team.
- Match length: 3 periods at 180 seconds each.
- Game start: 10-second pregame countdown, with countdown tick events every
  second and beep events for the final 4 seconds.
- Input boundary: gameplay receives semantic input actions, not SDL, mouse, key,
  or gamepad state.
- Left click is contextual: steal/poke check without puck, shot charge/release
  with puck.
- Right click sets a waypoint on the ice plane.
- S brakes, clears the waypoint, and double-tap fully stops.
- Z is an impulse boost, not a held speed multiplier.
- Skater X performs slide stop / quick turn.
- Goalie Z uses two boost charges with 4-second per-charge recovery.
- Goalie X activates a temporary shield that reflects pucks and bounces players.
- Goal triggers score only puck entry; non-puck bodies do not score.
- Solid skater-only and goalie-only collider authoring must be explicit and
  covered by tests.

Gameplay emits events for presentation-facing moments such as countdown ticks,
countdown beeps, shots, goals, steals, boosts, and shields. Gameplay does not
play audio or spawn final VFX directly.

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
- period length
- countdown timing
- boost impulse and cooldown
- goalie boost charges and recharge time
- goalie shield radius, duration, cooldown, and reflection impulse
- steal radius and cooldown
- waypoint stop/double-stop timing

## Completion Notes

Report:

- systems touched
- rules or behavior changed
- tuning values added or modified
- tests run
- deterministic simulation risks
