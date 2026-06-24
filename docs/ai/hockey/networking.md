# Hockey Networking

Use this guide for multiplayer, input commands, snapshots, prediction,
reconciliation, authoritative server work, network tests, and replay-adjacent
serialization.

## Direction

Assume multiplayer needs an authoritative simulation model. Avoid designs that
make renderer state, local UI state, wall-clock timing, or editor-only data
authoritative.

## Core Concepts

Networking work may include:

- input commands
- authoritative server simulation
- client prediction
- reconciliation
- snapshots
- delta compression later
- replay/state serialization
- deterministic simulation tests
- network integration tests

## Boundaries

Network state should serialize simulation/gameplay data, not renderer objects,
SDL events, or editor-only structures.

Input should be represented as explicit commands when possible:

- move direction
- aim direction
- steal pressed/clicked
- shoot press/hold/release
- boost, brake, quick turn, goalie shield, switch player, or pause buttons

The target gameplay model has no pass, body-check, or poke-check actions. Do not
add those to network input unless `_save/docs/GAMEPLAY.md` is explicitly changed
again.

`libs/networking` does not exist yet. Add it only when Phase 8/networking work is
explicitly requested.

## Testing Rules

Network tests must never hang indefinitely. Add timeouts around:

- connect
- accept
- recv/send
- condition-variable waits
- thread joins
- child process waits

Prefer loopback addresses for local tests:

```text
127.0.0.1
localhost
```

Avoid binding tests to LAN interfaces unless the task explicitly requires it.

## Completion Notes

Report:

- protocol or message changes
- serialization changes
- tests run
- timeout behavior
- prediction/reconciliation implications
- determinism risks
