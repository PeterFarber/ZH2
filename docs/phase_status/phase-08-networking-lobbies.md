# Phase 8 - Networking, Dedicated Server, Lobbies

Status: Not implemented as a phase yet. Prerequisites exist from earlier phases.

Source material:

- [x] Read `docs/phase-plans/phase-08-networking-lobbies.md`.
- [x] Read `docs/phase-rules/120-phase-8-networking-lobbies.mdc`.
- [x] Checked `docs/phase-rules/120-phase-8-networking-lobbies.mdc`.
- [x] Checked `docs/project-structure.md`.

## What This Phase Implements

- [ ] `hockey_networking` target.
- [ ] GameNetworkingSockets initialization/shutdown.
- [ ] Transport abstraction with client/server transport.
- [ ] Protocol versioning and binary message serialization.
- [ ] Handshake and connection lifecycle.
- [ ] Reliable and unreliable messages.
- [ ] Sequence, acknowledgement, loss, ping, rate, timeout, and stats tracking.
- [ ] Lobby state, ready state, team/role/slot selection, and match start flow.
- [ ] Client input streaming and authoritative server input handling.
- [ ] Network snapshots, interpolation, prediction, reconciliation, and replicated match/player/puck state.
- [ ] Dedicated server and game client online integration.
- [ ] `HockeyNetworkTests`.

## Finished Prerequisites From Earlier Phases

- [x] Headless dedicated server app exists.
- [x] Dedicated server runs authoritative local physics.
- [x] Dedicated server runs authoritative local gameplay.
- [x] Server fixed tick loop exists.
- [x] Client local input mapping exists.
- [x] Gameplay input frame exists.
- [x] Gameplay snapshots exist for match/player/puck state.
- [x] Gameplay events exist locally.
- [x] Scene validation exists.
- [x] CMake dependency guards protect server headless boundaries.
- [x] `SceneMode::ClientPrediction` exists as a future mode placeholder.

## Not Started - Target And Dependencies

- [ ] Create `libs/networking/`.
- [ ] Add `hockey_networking` CMake target.
- [ ] Add `apps/network_tests/` or equivalent `HockeyNetworkTests`.
- [ ] Add GameNetworkingSockets dependency through vcpkg.
- [ ] Keep dependency direction `networking -> core, ecs`.
- [ ] Link game client to networking.
- [ ] Link dedicated server to networking.
- [ ] Keep networking free of renderer/editor/Vulkan/ImGui dependencies.

## Not Started - Settings, Init, Protocol

- [ ] Add networking config/settings.
- [ ] Add GameNetworkingSockets global init/shutdown.
- [ ] Add network logging categories.
- [ ] Add protocol version constants.
- [ ] Add message type enum.
- [ ] Add binary serializer/deserializer.
- [ ] Add version rejection behavior.
- [ ] Add connection token or connection validation strategy.
- [ ] Add disconnect reason data.

## Not Started - Reliability And Transport

- [ ] Add reliable message path for handshake/lobby/match events.
- [ ] Add unreliable message path for input and snapshots.
- [ ] Add sequence numbers.
- [ ] Add acknowledgements.
- [ ] Add packet loss tracking.
- [ ] Add ping/round-trip-time tracking.
- [ ] Add bandwidth statistics.
- [ ] Add rate limiting.
- [ ] Add disconnect/timeout handling.
- [ ] Add local client/server transport tests.

## Not Started - Lobby And Match Flow

- [ ] Add lobby create flow.
- [ ] Add lobby join flow.
- [ ] Add lobby leave flow.
- [ ] Add lobby player list.
- [ ] Add ready state.
- [ ] Add team selection.
- [ ] Add role selection.
- [ ] Add slot assignment.
- [ ] Add match loading/start flow.
- [ ] Add return-to-lobby flow.
- [ ] Add lobby tests.

## Not Started - Replication And Prediction

- [ ] Add network identity component/data.
- [ ] Add owner component/data.
- [ ] Add replicated transform component/data.
- [ ] Add client input streaming.
- [ ] Add authoritative server input handling.
- [ ] Add server snapshot generation over the network.
- [ ] Add player state snapshot replication.
- [ ] Add puck state snapshot replication.
- [ ] Add match/score/period state replication.
- [ ] Add reliable gameplay event replication for goals, period changes, match end, and disconnects.
- [ ] Add interpolation buffer for remote players/puck.
- [ ] Add local prediction.
- [ ] Add input history.
- [ ] Add reconciliation/correction smoothing.

## Not Started - App Integration

- [ ] Add dedicated server listen/start networking path.
- [ ] Add dedicated server client connection handling.
- [ ] Add dedicated server network stats logging.
- [ ] Add game client connect flow.
- [ ] Add game client send-input loop.
- [ ] Add game client receive snapshot loop.
- [ ] Add game client disconnect handling.
- [ ] Add user-facing or debug network stats output.
- [ ] Add local multiplayer scripts or commands for verification.

## Left To Do

- [ ] Complete all target/dependency work.
- [ ] Complete protocol/transport work.
- [ ] Complete lobby flow.
- [ ] Complete input/snapshot replication.
- [ ] Complete interpolation, prediction, and reconciliation.
- [ ] Complete server/client app integration.
- [ ] Add `HockeyNetworkTests` covering init/shutdown, connect, messages, serialization, version rejection, lobby, inputs, snapshots, interpolation, prediction/reconciliation, disconnects, and stats.
- [ ] Verify Windows and Linux builds.
- [ ] Verify server remains headless.
