# Gameplay Design

This document describes the intended arcade hockey gameplay behavior for ZH2.
It is a design target for follow-up implementation, tests, editor tooling, and
tuning. Gameplay code must remain fixed-tick, deterministic-minded, and
independent from renderer, ImGui, SDL polling, and networking transport.

## Core Match

- Format: 4v4 hockey.
- Teams: Home and Away.
- Roster per team: 3 skaters and 1 goalie.
- Total active players: 8.
- Puck count: 1.
- Goals: 2.
- Periods: 3.
- Period length: 180 seconds each.
- Match clock runs only during active play.
- Goals stop play, increment score, reset puck/player positions, and start the
  next faceoff flow.

Current implementation note: the existing default period length is 300 seconds.
The gameplay settings, config defaults, rules YAML, and tests need to be updated
to make 180 seconds the default.

## Game Start

Before the first period starts:

- Match enters a pregame countdown phase for 10 seconds.
- Player movement and puck interaction are locked during the countdown.
- Countdown events are emitted every second.
- The last 4 seconds emit a countdown-beep gameplay event.
- At countdown end, the match transitions into faceoff setup.

Audio playback is not owned by gameplay. Gameplay should emit events such as
`CountdownTick` and `CountdownBeep`; Phase 9 audio/UI systems can decide how to
present those events.

## Input Model

Gameplay receives input as action data, not as raw keyboard, mouse, gamepad, or
SDL events. The game client maps local controls into gameplay input frames.
Future networking should send the same input-frame data model to the server.

Target local controls:

| Control | No puck | With puck |
| --- | --- | --- |
| Left click press | Steal / poke check | Begin charging shot |
| Left click hold | Continue steal intent if applicable | Continue charging shot |
| Left click release | End steal intent | Release shot |
| Right click | Set waypoint marker | Set waypoint marker |
| Z | Boost / forward impulse | Boost / forward impulse |
| X skater | Slide stop / quick turn | Slide stop / quick turn |
| X goalie | Shield | Shield |
| S | Brake / stop | Brake / stop |

The gameplay input model should represent actions semantically:

- `setMoveTarget`
- `clearMoveTarget`
- `stealPressed`
- `stealHeld`
- `stealReleased`
- `shootPressed`
- `shootHeld`
- `shootReleased`
- `boostPressed`
- `brakeHeld`
- `brakePressed`
- `quickTurnPressed`
- `goalieShieldPressed`

The client may still map these to mouse and keyboard, but gameplay systems
should not infer action meaning from button names.

## Movement

Players use vector-based movement with velocity and acceleration.

- A player moves toward the current waypoint marker when one exists.
- Direct movement input can also drive the player when no waypoint is active.
- Facing direction updates from aim input, movement direction, or velocity.
- Movement is fixed-tick and uses gameplay tuning values.
- Movement is disabled when match state says player input is locked.

Waypoint behavior:

- Right click places a waypoint on the ice plane.
- Reaching the waypoint clears it.
- Pressing S clears the active waypoint.
- Pressing S twice quickly fully stops the player.
- A quick turn also clears the active waypoint.

Skater movement:

- Skaters have higher top speed than goalies.
- Z applies a quick impulse in the current facing direction.
- X performs a slide stop / quick turn:
  - reverse facing direction quickly,
  - damp current velocity,
  - clear waypoint,
  - keep the action readable and arcade-focused.

Goalie movement:

- Goalies are slower than skaters.
- Goalies can be constrained to crease movement where the scene requires it.
- Z uses goalie boost charges instead of a simple skater impulse.
- X activates shield instead of skater quick turn.

## Skater Abilities

### Steal

Left click without puck attempts a steal / poke check.

- Steal checks the stick control volume in front of the skater.
- If the target has possession and is in range, the puck becomes loose.
- Steal has a cooldown from gameplay tuning.
- Steal should prefer puck interaction over body checking.
- Failed steal attempts still trigger cooldown to prevent spamming.

### Shot

Left click while holding puck charges and releases a shot.

- Press starts charge.
- Hold increases charge up to a tuned maximum.
- Release shoots in aim direction or facing direction if no aim exists.
- Shot releases possession.
- Shot sets puck state to `Shot`.
- The shooter has a short tuned grace window after release where the puck cannot
  be immediately re-controlled by that same player.
- Charging exposes shot power so the local player can see a power bar.
- Shot emits a `PuckShot` gameplay event.

### Boost

Z applies a short forward impulse.

- Direction is current facing direction.
- Boost is an impulse, not a held speed multiplier.
- Boost should clear or override waypoint steering for the impulse duration.
- Boost has cooldown and impulse strength tuning.
- Boost should not bypass match input locks.

### Slide Stop / Quick Turn

X performs a slide stop.

- Flip or sharply rotate facing direction.
- Damp velocity.
- Clear waypoint.
- Leave the player in control quickly.

### Stop

S is a brake and stop command.

- Holding S decelerates the player by a tuned amount.
- Pressing S clears the active waypoint.
- Double-tapping S within a short tuned window fully stops the player.

## Goalie Abilities

Goalies share puck control, stealing, shooting, waypoint movement, and stopping
rules with skaters unless stated otherwise.

### Goalie Boost

Z uses a two-charge boost system.

- Max charges: 2.
- Each boost consumes 1 charge.
- Each charge takes 4 seconds to recover.
- If both charges are used, the first charge returns after 4 seconds and the
  second returns after 8 seconds total.
- Boost direction is current facing direction.
- Boost should be strong enough for crease saves but not let goalies dominate
  the full rink.

### Goalie Shield

X activates a temporary shield.

- The shield is a sphere centered on the goalie.
- The shield reflects pucks that hit it.
- The shield bounces players that collide with it.
- Shield has tuned radius, duration, cooldown, and reflect impulse.
- Shield should not be active during faceoff/player-lock states.
- Shield should emit gameplay events so Phase 9 can add VFX/audio.

## Puck Interaction

Puck states:

- Loose.
- Possessed.
- Shot.
- Passed.
- Frozen.
- Resetting.

Possession rules:

- Only one player can possess the puck at a time.
- A player must be active, input-enabled, and in stick control range.
- Puck follows the possessing player's stick offset.
- Steal, shot, pass, goal, reset, and out-of-play can release possession.

Shot rules:

- Shot power is based on charge ratio.
- Shot direction uses aim first, then facing direction.
- Shot velocity clamps to puck tuning.
- The puck stays on the ice/floor height during possession and free movement.
- A freshly shot puck ignores the shooter for a short tuned grace window so it
  does not immediately collide back into or reattach to that player.

Pass rules:

- Passing remains a separate supported action.
- Pass assist should prefer teammates in the aim cone.
- Pass should ignore opponents as assist targets.

## Colliders And Triggers

Physics and gameplay need authoring support for role-specific interaction.

Required collision authoring:

- Static/rink colliders.
- Skater-only colliders.
- Goalie-only colliders.
- Player-and-goalie colliders.
- Puck colliders.
- Stick or steal volumes.
- Sensor/trigger volumes.

Goal trigger behavior:

- Goal areas are triggers.
- Only puck entry can score a goal.
- Non-puck bodies entering a goal trigger do not score.
- Repeated puck trigger overlap while already in `GoalScored` or reset state is
  ignored.
- The scoring team is the opposite of the defending team.

Trigger filtering:

- Triggers can opt into detecting skaters.
- Triggers can opt into detecting goalies.
- Triggers can opt into detecting puck.
- Trigger events should feed gameplay systems without changing physics response
  unless explicitly configured.

Current implementation note: physics already has separate `Player`, `Goalie`,
`Puck`, `Goal`, `Trigger`, and `Sensor` layers, and trigger components can
filter players, goalies, and puck. Solid skater-only and goalie-only collider
authoring still needs a clearer editor-facing workflow and tests.

## Tuning Targets

Gameplay values should live in settings or tuning data, not hardcoded inside
systems.

Match tuning:

- `PeriodCount = 3`
- `PeriodLengthSeconds = 180`
- `PregameCountdownSeconds = 10`
- `CountdownBeepStartSeconds = 4`

Skater tuning:

- max speed
- acceleration
- deceleration
- boost impulse
- boost cooldown
- slide-stop damping
- double-stop window
- steal radius
- steal cooldown

Goalie tuning:

- max speed
- acceleration
- crease movement constraints
- boost charges
- boost recharge seconds
- boost impulse
- shield radius
- shield duration
- shield cooldown
- shield reflect impulse

Puck tuning:

- possession offset
- loose puck drag
- floor height
- max speed
- shot min/max power
- charge duration
- shot self-collision grace seconds
- pass power
- out-of-play threshold

## Implementation Gaps To Close

The existing gameplay baseline already covers 4v4 setup, local input frames,
waypoints, movement, possession, shooting, passing, checking hooks, goals,
periods, reset, validation, snapshots, client local play, editor preview, and
headless server simulation.

The new design requires these follow-up changes:

- Change default period length from 300 seconds to 180 seconds.
- Add pregame countdown phase/timer/events.
- Add countdown beep gameplay events for the last 4 seconds.
- Remap left click to contextual steal-or-shot behavior in client input.
- Represent steal as explicit gameplay input/action data.
- Convert skater Z from held speed multiplier to quick impulse.
- Add skater boost cooldown/tuning.
- Make S clear waypoint.
- Add S double-tap full-stop behavior.
- Add goalie two-charge boost with 4-second per-charge recovery.
- Add goalie shield component/runtime state/system.
- Add shield reflection for puck and bounce response for players.
- Add a short post-shot self-collision grace window for the shooter.
- Keep puck simulation clamped to the ice/floor height.
- Add local shot power bar feedback while charging.
- Add gameplay events for boost, shield, countdown, and steal where useful.
- Add editor/tuning exposure for new gameplay values.
- Add tests for every new rule before implementation.

## Required Tests

Add or update gameplay tests for:

- 3 periods of 180 seconds.
- Pregame countdown locks movement.
- Countdown emits tick and beep events.
- Left click without possession attempts steal.
- Left click with possession charges/releases shot.
- Shot release cannot immediately reattach to the shooter during the grace window.
- Puck controller keeps puck height on the ice/floor.
- Gameplay snapshots expose shot charge ratio for the local power bar.
- Steal releases opponent possession when in range.
- Skater boost applies impulse and respects cooldown.
- S clears waypoint.
- S double-tap fully stops player.
- X skater quick turn clears waypoint.
- Goalie boost starts with 2 charges.
- Goalie boost recharges one charge every 4 seconds.
- Goalie shield reflects puck.
- Goalie shield bounces players.
- Goalie shield respects duration/cooldown.
- Goal trigger scores only puck.
- Skater-only and goalie-only collider authoring behaves as expected.
