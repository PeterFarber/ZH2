# Hockey Physics

Use this guide for puck movement, ice friction, skating, stick contact, shot
impulses, board collisions, goal collisions, and deterministic simulation.

## Core Rule

Do not tie gameplay physics to frame rate. Use fixed simulation steps where the
architecture supports them.

## Important Systems

Physics-related work may include:

- puck movement
- ice friction
- player acceleration and turning
- stick-to-puck contact
- shot impulse and shot direction
- board and goal collisions
- puck bounce and damping
- player collision or avoidance
- replay and network determinism

## Determinism Rules

Avoid these in authoritative simulation paths unless explicitly justified:

- wall-clock time
- unseeded randomness
- pointer addresses as IDs
- unordered iteration where order affects results
- renderer transforms as source-of-truth physics state
- SDL events as persistent simulation state

Prefer:

- explicit input commands
- stable entity IDs
- fixed tick counts
- seeded random generators
- stable iteration order when collision or rule resolution depends on order

## Puck Checks

When changing puck behavior, check:

- linear velocity
- angular velocity or spin if present
- ice friction
- collision response
- shot impulse
- deflection behavior
- goal detection
- serialization and replay state

## Verification

Prefer tests over visual-only checks. Useful tests include:

- same input, seed, and tick count produces the same result
- puck slows down under friction
- board collision reflects or dampens velocity as expected
- shot impulse produces expected speed range
- goal collision does not tunnel through obvious geometry

## Completion Notes

Report:

- authoritative state changed
- fixed-timestep assumptions
- tests run
- determinism risks
- visual behavior that still needs manual tuning
