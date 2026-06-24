# Phase 8 Plan — Complete Networking, Dedicated Server, Lobbies, Replication, Prediction, and Reconciliation

File path recommendation:

```text
docs/phase-plans/phase-08-networking-lobbies.md
```

AI instruction:

```text
Read `docs/phase-plans/phase-08-networking-lobbies.md` and implement Phase 8 exactly. Build the complete multiplayer networking layer using GameNetworkingSockets, including transport, protocol, dedicated server hosting, local lobby flow, team/role selection, match start, input streaming, authoritative server snapshots, interpolation, client prediction, server reconciliation, disconnect handling, network statistics, and tests. Do not implement final matchmaking services, platform storefront integration, final animation/audio/UI polish, monetization, or anti-cheat service integration during this phase. Keep the dedicated server headless and keep Windows/Linux compatibility.
```

---

# 0. Phase Summary

Phase 8 builds the full real-time multiplayer layer.

This phase turns the local/headless hockey simulation into a networked authoritative multiplayer game with:

```text
GameNetworkingSockets transport
client connection flow
dedicated server connection flow
protocol versioning
reliable messages
unreliable input/snapshot messages
connection tokens
server-authoritative match simulation
client input streaming
server snapshots
snapshot interpolation
local player prediction
server reconciliation
puck replication
score/match event replication
lobby creation
lobby joining
team selection
role selection
ready state
match start
disconnect handling
timeout handling
ping/loss/bandwidth stats
network tests
local multi-client testing scripts
```

Phase 8 must produce:

```text
hockey_networking
HockeyNetworkTests
```

Phase 8 updates:

```text
HockeyGameClient
HockeyDedicatedServer
HockeyMapEditor only if local debug tools are useful
hockey_gameplay integration points
hockey_ecs networking components
data/config/client.toml
data/config/server.toml
scripts/windows
scripts/linux
```

Phase 8 must not implement:

```text
Steam backend
EOS backend
public matchmaking service
ranked matchmaking
storefront authentication
final anti-cheat service
final animation/audio/UI polish
monetization
```

Local/direct dedicated server multiplayer must work by the end of this phase.

---

# 1. Architecture Rules

## 1.1 Dependency Direction

Add:

```text
hockey_networking -> hockey_core, hockey_ecs
```

Gameplay integration:

```text
hockey_gameplay may expose pure data used by networking.
hockey_networking may include adapters for gameplay snapshots/input.
```

Allowed app links:

```text
HockeyGameClient       -> hockey_networking
HockeyDedicatedServer  -> hockey_networking
HockeyNetworkTests     -> hockey_networking
```

Optional editor link:

```text
HockeyMapEditor -> hockey_networking
```

Only for debug tools such as packet/protocol inspection. Do not make editor networking required.

Forbidden dependencies:

```text
hockey_networking -> hockey_renderer
hockey_networking -> hockey_editor
hockey_networking -> Vulkan
hockey_networking -> ImGui
hockey_networking -> audio
```

Dedicated server:

```text
HockeyDedicatedServer -> hockey_networking, hockey_gameplay, hockey_physics, hockey_ecs, hockey_core
```

Dedicated server must not link:

```text
hockey_renderer
hockey_editor
Vulkan
ImGui
audio
```

## 1.2 Networking Owns

`hockey_networking` owns:

```text
GameNetworkingSockets initialization/shutdown
network host/client classes
connection management
protocol messages
serialization/deserialization
reliable/unreliable channels
connection tokens
client/server state machines
lobby state transport
team/role/ready networking
input packet handling
snapshot packet handling
ack/sequence tracking
latency/loss/bandwidth stats
interpolation buffer
prediction history
reconciliation data path
disconnect/timeout handling
network tests
```

## 1.3 Networking Must Not Own

`hockey_networking` must not own:

```text
hockey rules themselves
physics stepping itself
renderer drawing
editor panels
SDL input polling
audio playback
animation runtime
asset importing
```

Gameplay remains the authority for match rules.

Networking carries input, state, and events.

---

# 2. Dependencies

Update `vcpkg.json`.

Add:

```json
"gam-networkingsockets"
```

If the actual port name differs, use the available GameNetworkingSockets vcpkg port and document the imported target in CMake.

Also add one serialization dependency only if approved by existing project style:

Preferred Phase 8 default:

```text
manual binary serialization in hockey_networking
```

Optional if desired:

```json
"flatbuffers"
```

Do not add protobuf/flatbuffers/msgpack unless you intentionally choose it and wire it cleanly. Manual binary serialization is acceptable and keeps the dependency surface smaller.

Do not add:

```text
Steamworks SDK
EOS SDK
platform matchmaking SDK
anti-cheat SDK
```

---

# 3. Target Project Structure

Add:

```text
libs/
  networking/
    CMakeLists.txt
    include/Hockey/Networking/
      Networking.hpp
      NetworkSettings.hpp
      NetworkTypes.hpp
      NetworkResult.hpp

      Transport/
        NetworkTransport.hpp
        GameNetworkingSocketsTransport.hpp
        NetworkAddress.hpp
        NetworkConnection.hpp
        NetworkConnectionState.hpp
        NetworkChannel.hpp
        NetworkStats.hpp

      Protocol/
        Protocol.hpp
        ProtocolVersion.hpp
        MessageType.hpp
        MessageHeader.hpp
        Message.hpp
        MessageReader.hpp
        MessageWriter.hpp
        MessageSerializer.hpp
        MessageRegistry.hpp

      Messages/
        HandshakeMessages.hpp
        LobbyMessages.hpp
        MatchMessages.hpp
        InputMessages.hpp
        SnapshotMessages.hpp
        EventMessages.hpp
        DebugMessages.hpp

      Server/
        NetworkServer.hpp
        ServerConnection.hpp
        ServerClient.hpp
        ServerLobby.hpp
        ServerLobbyManager.hpp
        ServerMatchHost.hpp
        ServerSnapshotSystem.hpp
        ServerInputSystem.hpp

      Client/
        NetworkClient.hpp
        ClientLobby.hpp
        ClientMatchConnection.hpp
        ClientInputSystem.hpp
        ClientSnapshotSystem.hpp
        SnapshotInterpolation.hpp
        ClientPrediction.hpp
        Reconciliation.hpp

      Replication/
        NetworkIdentity.hpp
        ReplicationComponents.hpp
        ReplicationRegistry.hpp
        ReplicationSystem.hpp
        Snapshot.hpp
        SnapshotDelta.hpp
        SnapshotBuffer.hpp
        StateQuantization.hpp

      Lobby/
        LobbyTypes.hpp
        LobbyState.hpp
        LobbyService.hpp
        TeamRoleSelection.hpp

      Reliability/
        SequenceBuffer.hpp
        AckTracker.hpp
        PacketLossTracker.hpp
        RateLimiter.hpp

      Testing/
        LocalNetworkHarness.hpp
        FakePacketTransport.hpp

    src/
      Networking.cpp
      NetworkSettings.cpp
      NetworkTypes.cpp
      NetworkResult.cpp

      Transport/
        NetworkTransport.cpp
        GameNetworkingSocketsTransport.cpp
        NetworkAddress.cpp
        NetworkConnection.cpp
        NetworkConnectionState.cpp
        NetworkChannel.cpp
        NetworkStats.cpp

      Protocol/
        Protocol.cpp
        ProtocolVersion.cpp
        MessageType.cpp
        MessageHeader.cpp
        Message.cpp
        MessageReader.cpp
        MessageWriter.cpp
        MessageSerializer.cpp
        MessageRegistry.cpp

      Messages/
        HandshakeMessages.cpp
        LobbyMessages.cpp
        MatchMessages.cpp
        InputMessages.cpp
        SnapshotMessages.cpp
        EventMessages.cpp
        DebugMessages.cpp

      Server/
        NetworkServer.cpp
        ServerConnection.cpp
        ServerClient.cpp
        ServerLobby.cpp
        ServerLobbyManager.cpp
        ServerMatchHost.cpp
        ServerSnapshotSystem.cpp
        ServerInputSystem.cpp

      Client/
        NetworkClient.cpp
        ClientLobby.cpp
        ClientMatchConnection.cpp
        ClientInputSystem.cpp
        ClientSnapshotSystem.cpp
        SnapshotInterpolation.cpp
        ClientPrediction.cpp
        Reconciliation.cpp

      Replication/
        NetworkIdentity.cpp
        ReplicationComponents.cpp
        ReplicationRegistry.cpp
        ReplicationSystem.cpp
        Snapshot.cpp
        SnapshotDelta.cpp
        SnapshotBuffer.cpp
        StateQuantization.cpp

      Lobby/
        LobbyTypes.cpp
        LobbyState.cpp
        LobbyService.cpp
        TeamRoleSelection.cpp

      Reliability/
        SequenceBuffer.cpp
        AckTracker.cpp
        PacketLossTracker.cpp
        RateLimiter.cpp

      Testing/
        LocalNetworkHarness.cpp
        FakePacketTransport.cpp

apps/
  network_tests/
    CMakeLists.txt
    src/
      NetworkTestsMain.cpp
      Test.hpp
      MessageSerializationTests.cpp
      SequenceBufferTests.cpp
      AckTrackerTests.cpp
      PacketLossTests.cpp
      FakeTransportTests.cpp
      ClientServerConnectionTests.cpp
      LobbyTests.cpp
      TeamRoleSelectionTests.cpp
      InputMessageTests.cpp
      SnapshotSerializationTests.cpp
      InterpolationTests.cpp
      PredictionTests.cpp
      ReconciliationTests.cpp
      DisconnectTests.cpp
      HeadlessServerNetworkTests.cpp
```

Update scripts:

```text
scripts/windows/run_server.ps1
scripts/windows/run_client.ps1
scripts/windows/run_two_clients.ps1
scripts/windows/run_local_multiplayer.ps1

scripts/linux/run_server.sh
scripts/linux/run_client.sh
scripts/linux/run_two_clients.sh
scripts/linux/run_local_multiplayer.sh
```

---

# 4. CMake Requirements

## 4.1 Root CMake

Add:

```cmake
add_subdirectory(libs/networking)
```

When tests are enabled:

```cmake
add_subdirectory(apps/network_tests)
```

Ordering:

```text
core
ecs
assets
renderer
physics
gameplay
networking
editor
apps
```

## 4.2 `libs/networking/CMakeLists.txt`

Create target:

```text
hockey_networking
```

Required links:

```text
hockey_core
hockey_ecs
GameNetworkingSockets imported target
```

Do not link:

```text
hockey_renderer
hockey_editor
hockey_physics unless explicitly needed
hockey_gameplay unless explicitly needed
Vulkan
ImGui
SDL3
```

Preferred architecture:

```text
hockey_networking has pure transport/protocol.
apps/server/client bridge networking to gameplay.
```

If gameplay snapshot/input types are needed, keep them in pure headers without pulling renderer/editor.

## 4.3 App Link Updates

Update:

```text
HockeyGameClient links hockey_networking
HockeyDedicatedServer links hockey_networking
HockeyNetworkTests links hockey_networking
```

Do not force:

```text
HockeyMapEditor links hockey_networking
```

unless debug tools are explicitly added.

---

# 5. Network Settings

## 5.1 Files

```text
NetworkSettings.hpp/cpp
```

## 5.2 Settings Struct

Implement:

```cpp
struct NetworkSettings {
    bool enabled = true;

    std::string bindAddress = "0.0.0.0";
    uint16_t serverPort = 27020;

    std::string connectAddress = "127.0.0.1";
    uint16_t connectPort = 27020;

    uint32_t maxClients = 8;
    uint32_t protocolVersion = 1;

    float connectionTimeoutSeconds = 10.0f;
    float heartbeatIntervalSeconds = 1.0f;

    uint32_t serverTickRate = 60;
    uint32_t inputSendRate = 60;
    uint32_t snapshotSendRate = 30;

    uint32_t maxInputPacketsPerSecond = 90;
    uint32_t maxReliableBytesPerSecond = 64 * 1024;
    uint32_t maxUnreliableBytesPerSecond = 256 * 1024;

    bool enablePacketLogging = false;
    bool enableNetworkStats = true;

    float interpolationDelaySeconds = 0.1f;
    bool enableClientPrediction = true;
    bool enableReconciliation = true;
};
```

## 5.3 Config Updates

`server.toml`:

```toml
[network]
enabled = true
bind_address = "0.0.0.0"
port = 27020
max_clients = 8
protocol_version = 1
connection_timeout_seconds = 10.0
heartbeat_interval_seconds = 1.0
server_tick_rate = 60
snapshot_send_rate = 30
max_input_packets_per_second = 90
enable_packet_logging = false
enable_network_stats = true
```

`client.toml`:

```toml
[network]
enabled = true
connect_address = "127.0.0.1"
connect_port = 27020
protocol_version = 1
input_send_rate = 60
interpolation_delay_seconds = 0.1
enable_client_prediction = true
enable_reconciliation = true
enable_packet_logging = false
enable_network_stats = true
```

## 5.4 Required Functions

Implement:

```cpp
NetworkSettings MakeDefaultNetworkSettings();
Status LoadNetworkSettings(Config& config, NetworkSettings& outSettings, bool serverMode);
void SaveNetworkSettings(Config& config, const NetworkSettings& settings, bool serverMode);
```

---

# 6. Networking Initialization

## 6.1 Files

```text
Networking.hpp/cpp
```

## 6.2 API

Implement:

```cpp
class Networking {
public:
    static Status Init();
    static void Shutdown();
    static bool IsInitialized();
};
```

Responsibilities:

```text
initialize GameNetworkingSockets
install debug output callback
shutdown cleanly
log library status
```

Rules:

```text
Init can be called once.
Shutdown must be safe.
No active server/client should remain after Shutdown.
```

---

# 7. Transport Layer

## 7.1 NetworkAddress

Files:

```text
Transport/NetworkAddress.hpp/cpp
```

Implement:

```cpp
struct NetworkAddress {
    std::string host = "127.0.0.1";
    uint16_t port = 27020;

    std::string ToString() const;
    static Result<NetworkAddress> Parse(const std::string& value);
};
```

## 7.2 NetworkChannel

Files:

```text
Transport/NetworkChannel.hpp/cpp
```

Implement:

```cpp
enum class NetworkChannel {
    ReliableControl,
    ReliableGameplay,
    UnreliableInput,
    UnreliableSnapshot,
    UnreliableDebug
};
```

Mapping to GameNetworkingSockets send flags:

```text
ReliableControl:
  reliable, ordered

ReliableGameplay:
  reliable, ordered

UnreliableInput:
  unreliable, no delay

UnreliableSnapshot:
  unreliable, no delay

UnreliableDebug:
  unreliable
```

## 7.3 NetworkConnectionState

Implement:

```cpp
enum class NetworkConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Handshaking,
    InLobby,
    LoadingMatch,
    InMatch,
    Disconnecting,
    Failed
};
```

## 7.4 NetworkStats

Implement:

```cpp
struct NetworkStats {
    float pingMs = 0.0f;
    float packetLossPercent = 0.0f;

    uint64_t bytesSent = 0;
    uint64_t bytesReceived = 0;

    uint32_t packetsSent = 0;
    uint32_t packetsReceived = 0;

    float outgoingKbps = 0.0f;
    float incomingKbps = 0.0f;

    uint32_t reliableQueueBytes = 0;
    uint32_t connectionQualityLocal = 0;
    uint32_t connectionQualityRemote = 0;
};
```

## 7.5 NetworkTransport Interface

Files:

```text
Transport/NetworkTransport.hpp/cpp
```

Implement interface:

```cpp
class NetworkTransport {
public:
    virtual ~NetworkTransport() = default;

    virtual Status StartServer(const NetworkAddress& bindAddress, uint32_t maxClients) = 0;
    virtual Status Connect(const NetworkAddress& remoteAddress) = 0;
    virtual void Shutdown() = 0;

    virtual void PollEvents() = 0;

    virtual Status Send(ConnectionID connection, NetworkChannel channel, const void* data, size_t size) = 0;
    virtual void Disconnect(ConnectionID connection, const std::string& reason) = 0;

    virtual NetworkStats GetStats(ConnectionID connection) const = 0;
};
```

`ConnectionID` should be a small engine type, not raw GNS handle in public API.

## 7.6 GameNetworkingSocketsTransport

Files:

```text
Transport/GameNetworkingSocketsTransport.hpp/cpp
```

Responsibilities:

```text
server listen socket
client connection
connection status callback
poll incoming messages
send messages
close connections
map GNS handles to ConnectionID
collect connection stats
```

---

# 8. Protocol and Serialization

## 8.1 Protocol Version

Files:

```text
Protocol/ProtocolVersion.hpp/cpp
```

Implement:

```cpp
constexpr uint32_t kProtocolVersion = 1;
constexpr uint32_t kProtocolMagic = 0x484B594E; // HKYN or similar
```

## 8.2 MessageType

Files:

```text
Protocol/MessageType.hpp/cpp
```

Implement:

```cpp
enum class MessageType : uint16_t {
    Invalid = 0,

    ClientHello,
    ServerHello,
    ProtocolReject,
    Heartbeat,
    DisconnectReason,

    CreateLobby,
    JoinLobby,
    LeaveLobby,
    LobbyState,
    SetReady,
    SelectTeam,
    SelectRole,
    SelectSlot,
    StartMatch,

    LoadMatch,
    MatchLoaded,
    MatchStart,
    MatchEnd,

    ClientInput,
    InputAck,
    ServerSnapshot,
    ReliableMatchEvent,

    ChatMessage,
    DebugPing,
    DebugPong
};
```

## 8.3 MessageHeader

Files:

```text
Protocol/MessageHeader.hpp/cpp
```

Implement:

```cpp
struct MessageHeader {
    uint32_t magic = kProtocolMagic;
    uint32_t protocolVersion = kProtocolVersion;
    MessageType type = MessageType::Invalid;

    uint32_t sequence = 0;
    uint32_t ack = 0;
    uint32_t ackBits = 0;

    uint32_t payloadSize = 0;
};
```

## 8.4 MessageWriter / MessageReader

Files:

```text
Protocol/MessageWriter.hpp/cpp
Protocol/MessageReader.hpp/cpp
```

Implement binary serialization helpers:

```cpp
class MessageWriter {
public:
    void WriteU8(uint8_t value);
    void WriteU16(uint16_t value);
    void WriteU32(uint32_t value);
    void WriteU64(uint64_t value);
    void WriteI32(int32_t value);
    void WriteF32(float value);
    void WriteBool(bool value);
    void WriteString(const std::string& value);
    void WriteVec2(const glm::vec2& value);
    void WriteVec3(const glm::vec3& value);
    void WriteUUID(UUID value);

    const std::vector<std::byte>& Data() const;
};
```

```cpp
class MessageReader {
public:
    explicit MessageReader(std::span<const std::byte> data);

    bool ReadU8(uint8_t& value);
    bool ReadU16(uint16_t& value);
    bool ReadU32(uint32_t& value);
    bool ReadU64(uint64_t& value);
    bool ReadI32(int32_t& value);
    bool ReadF32(float& value);
    bool ReadBool(bool& value);
    bool ReadString(std::string& value);
    bool ReadVec2(glm::vec2& value);
    bool ReadVec3(glm::vec3& value);
    bool ReadUUID(UUID& value);

    bool HasError() const;
};
```

Rules:

```text
little endian internally
bounds checked reads
string length limit
packet size limit
corrupt messages rejected safely
```

## 8.5 MessageSerializer

Implement:

```cpp
class MessageSerializer {
public:
    static Result<std::vector<std::byte>> Serialize(const Message& message);
    static Result<Message> Deserialize(std::span<const std::byte> data);
};
```

---

# 9. Reliability Helpers

## 9.1 SequenceBuffer

Files:

```text
Reliability/SequenceBuffer.hpp/cpp
```

Implement fixed-size sequence buffer:

```cpp
template<typename T, size_t Size>
class SequenceBuffer {
public:
    void Insert(uint32_t sequence, const T& value);
    T* Find(uint32_t sequence);
    void Remove(uint32_t sequence);
    bool Exists(uint32_t sequence) const;
    void Clear();
};
```

Handle wraparound.

## 9.2 AckTracker

Files:

```text
Reliability/AckTracker.hpp/cpp
```

Implement:

```cpp
class AckTracker {
public:
    uint32_t NextSequence();
    void MarkReceived(uint32_t sequence);
    uint32_t LatestReceived() const;
    uint32_t GenerateAckBits() const;
    bool IsAcked(uint32_t sequence, uint32_t ack, uint32_t ackBits) const;
};
```

## 9.3 PacketLossTracker

Files:

```text
Reliability/PacketLossTracker.hpp/cpp
```

Track:

```text
sent packets
acked packets
lost packets
moving packet loss percent
```

## 9.4 RateLimiter

Files:

```text
Reliability/RateLimiter.hpp/cpp
```

Implement per-connection rate limiting:

```text
input packets per second
reliable bytes per second
unreliable bytes per second
```

Server should drop/throttle abusive input rates.

---

# 10. Handshake Flow

## 10.1 ClientHello

Payload:

```text
protocol version
client build/version string
player display name
requested connection token if used
```

## 10.2 ServerHello

Payload:

```text
protocol version
assigned client ID
server tick rate
snapshot rate
match/lobby state
```

## 10.3 ProtocolReject

Payload:

```text
reason enum
reason string
server protocol version
```

Reject reasons:

```text
version mismatch
server full
invalid token
banned/kicked placeholder
malformed message
```

## 10.4 Flow

Client:

```text
Connect
Send ClientHello
Wait for ServerHello
Enter lobby
```

Server:

```text
Accept connection
Wait for ClientHello
Validate version/token/capacity
Send ServerHello or ProtocolReject
Create ServerClient state
Place client in lobby
```

---

# 11. Lobby System

## 11.1 Lobby Types

Files:

```text
Lobby/LobbyTypes.hpp/cpp
Lobby/LobbyState.hpp/cpp
Lobby/TeamRoleSelection.hpp/cpp
```

Implement:

```cpp
enum class LobbyPlayerState {
    Connected,
    SelectingTeam,
    SelectingRole,
    Ready,
    Loading,
    InMatch
};

struct LobbyPlayer {
    ClientID clientId;
    std::string displayName;

    GameplayTeam team = GameplayTeam::None;
    GameplayRole role = GameplayRole::Skater;
    PlayerSlot slot = PlayerSlot::None;

    bool ready = false;
    LobbyPlayerState state = LobbyPlayerState::Connected;
};

struct LobbyState {
    LobbyID lobbyId;
    std::string name;

    uint32_t maxPlayers = 8;
    std::vector<LobbyPlayer> players;

    bool matchStarting = false;
};
```

## 11.2 ServerLobby

Files:

```text
Server/ServerLobby.hpp/cpp
Server/ServerLobbyManager.hpp/cpp
```

Implement:

```text
create lobby
join lobby
leave lobby
set ready
select team
select role
select slot
validate slot uniqueness
broadcast LobbyState
start match when ready
```

Rules:

```text
max 8 players
Home: 3 skaters + 1 goalie
Away: 3 skaters + 1 goalie
slot unique
ready requires valid team/role/slot
match can start when all connected players are ready, or when dev start command is used
```

## 11.3 ClientLobby

Files:

```text
Client/ClientLobby.hpp/cpp
```

Implement client state:

```text
current lobby state
local player selection
send team/role/ready messages
receive lobby updates
```

## 11.4 Lobby Messages

Implement:

```text
CreateLobby
JoinLobby
LeaveLobby
LobbyState
SetReady
SelectTeam
SelectRole
SelectSlot
StartMatch
```

For Phase 8, direct-connect server can automatically create one default lobby.

No public matchmaking service required.

---

# 12. Match Start and Loading Flow

## 12.1 Flow

Server:

```text
lobby ready/start
assign slots to gameplay players
send LoadMatch message with scene ID/path/hash
wait for MatchLoaded from clients
initialize gameplay match
send MatchStart with server tick
enter InMatch
```

Client:

```text
receive LoadMatch
load local scene
prepare renderer/gameplay client scene
send MatchLoaded
receive MatchStart
enter InMatch
start input streaming
start snapshot processing
```

## 12.2 Messages

Implement:

```text
LoadMatch
MatchLoaded
MatchStart
MatchEnd
```

`LoadMatch` payload:

```text
scene path or scene asset ID
scene hash if available
server tick rate
snapshot rate
assigned player slot
```

`MatchStart` payload:

```text
server start tick
initial match snapshot
```

---

# 13. Input Networking

## 13.1 ClientInputMessage

Files:

```text
Messages/InputMessages.hpp/cpp
Client/ClientInputSystem.hpp/cpp
Server/ServerInputSystem.hpp/cpp
```

Payload:

```text
client ID
player slot
input sequence
simulation tick
move vector
aim vector
button bitmask
```

Button bitmask includes:

```text
boostPressed
brakePressed
quickTurnPressed
stealPressed
shootPressed
shootHeld
shootReleased
goalieShieldPressed
pausePressed
```

Do not add pass, body-check, or poke-check input bits unless
`_save/docs/GAMEPLAY.md` explicitly changes again.

## 13.2 Client Behavior

Client sends input:

```text
at inputSendRate
using unreliable no-delay channel
with sequence number
with target simulation tick
```

Client keeps input history for prediction/reconciliation.

## 13.3 Server Behavior

Server:

```text
receives input messages
validates client controls assigned slot
rate limits input
stores latest input per client/player
applies input during authoritative gameplay tick
acks input sequence in snapshots or InputAck
```

Server must reject:

```text
inputs from wrong client/slot
inputs before match start
malformed inputs
too many input packets
```

---

# 14. Snapshot Replication

## 14.1 Snapshot Data

Files:

```text
Replication/Snapshot.hpp/cpp
Messages/SnapshotMessages.hpp/cpp
```

Use Phase 7 `GameplaySnapshot` as source data, but define network snapshot format separately.

Implement:

```cpp
struct NetworkPlayerState {
    UUID entity;
    uint32_t playerIndex;
    GameplayTeam team;
    GameplayRole role;

    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 facingDirection;

    bool hasPuck = false;
};

struct NetworkPuckState {
    UUID entity;
    PuckState state;
    glm::vec3 position;
    glm::vec3 velocity;
    UUID possessingPlayer;
};

struct NetworkMatchState {
    MatchPhase phase;
    uint32_t period;
    float periodTimeRemaining;
    uint32_t homeScore;
    uint32_t awayScore;
};

struct NetworkSnapshot {
    uint64_t serverTick = 0;
    uint32_t lastProcessedInputSequence = 0;

    NetworkMatchState match;
    std::vector<NetworkPlayerState> players;
    NetworkPuckState puck;
};
```

## 14.2 ServerSnapshotMessage

Payload:

```text
server tick
last processed input sequence for recipient
match state
player states
puck state
reliable event sequence reference if needed
```

Sent:

```text
at snapshotSendRate
using unreliable no-delay channel
```

## 14.3 Delta Compression

Implement one of:

Preferred Phase 8:

```text
full snapshots first
delta snapshot infrastructure present
```

If implementing delta:

```text
baseline tick
changed entity bitset
quantized fields
fallback full snapshot if baseline missing
```

Do not overcomplicate before full snapshots are stable.

## 14.4 State Quantization

Files:

```text
Replication/StateQuantization.hpp/cpp
```

Implement helpers:

```text
quantize position
quantize velocity
quantize normalized direction
quantize time
```

Even if not used everywhere, provide path for future bandwidth optimization.

---

# 15. Network Identity and Replication Components

Files:

```text
Replication/NetworkIdentity.hpp/cpp
Replication/ReplicationComponents.hpp/cpp
```

Add ECS components:

```cpp
struct NetworkIdentityComponent {
    uint32_t networkId = 0;
    UUID stableEntityId;
    bool serverOwned = true;
};

struct NetworkOwnerComponent {
    ClientID ownerClient;
    bool hasOwner = false;
};

struct ReplicatedTransformComponent {
    bool replicatePosition = true;
    bool replicateRotation = true;
    bool replicateScale = false;

    float interpolationSmoothing = 0.1f;
};

struct ReplicationComponent {
    bool replicated = true;
    bool highPriority = false;
    float updateRate = 30.0f;
};
```

Serialize where useful.

Runtime network IDs can be assigned on match start.

Rules:

```text
server assigns network IDs
client maps network IDs to scene entities
stable UUID remains for scene identity
network ID optimized for packets
```

---

# 16. Client Snapshot Interpolation

Files:

```text
Client/SnapshotInterpolation.hpp/cpp
Replication/SnapshotBuffer.hpp/cpp
```

Implement:

```cpp
class SnapshotBuffer {
public:
    void AddSnapshot(const NetworkSnapshot& snapshot);
    bool GetSnapshotsForRenderTime(double renderServerTime, NetworkSnapshot& a, NetworkSnapshot& b, float& alpha) const;
    void Clear();
};
```

Client interpolation:

```text
buffer snapshots
render remote state slightly behind server time
interpolate positions/velocities/facing
smooth puck movement
handle missing/out-of-order snapshots
snap or smooth when error is too large
```

Settings:

```text
interpolationDelaySeconds
```

Client should not directly render raw latest snapshot for remote entities unless buffer is empty.

---

# 17. Client Prediction

Files:

```text
Client/ClientPrediction.hpp/cpp
```

Implement prediction for local controlled player.

```cpp
struct PredictedState {
    uint64_t tick = 0;
    uint32_t inputSequence = 0;
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 facingDirection;
};

class ClientPrediction {
public:
    void RecordInput(const GameplayInputFrame& input);
    void RecordPredictedState(const PredictedState& state);

    std::vector<GameplayInputFrame> GetInputsAfter(uint32_t inputSequence) const;
    void ClearBefore(uint32_t inputSequence);
    void Reset();
};
```

Behavior:

```text
client applies local input immediately
records input history
records predicted local state
waits for server snapshot with last processed input
```

No networking transport logic here.

---

# 18. Reconciliation

Files:

```text
Client/Reconciliation.hpp/cpp
```

Implement:

```cpp
struct ReconciliationSettings {
    float positionErrorThreshold = 0.15f;
    float hardSnapThreshold = 2.0f;
    float correctionSmoothing = 0.15f;
};

class Reconciliation {
public:
    void ApplyServerCorrection(
        Scene& scene,
        const NetworkSnapshot& snapshot,
        ClientPrediction& prediction,
        const ReconciliationSettings& settings
    );
};
```

Behavior:

```text
compare authoritative server state for local player
if error small, keep prediction
if error medium, smooth correction
if error large, hard snap
replay unacked inputs after correction if supported
clear acked input history
```

Puck correction:

```text
if local player has puck and server disagrees, correct puck/possession
if puck error large, snap
if puck error medium, smooth
```

---

# 19. Reliable Match Events

Files:

```text
Messages/EventMessages.hpp/cpp
```

Replicate important events reliably:

```text
GoalScored
ScoreChanged
PeriodStarted
PeriodEnded
FaceoffStarted
FaceoffEnded
MatchEnded
PlayerJoined
PlayerLeft
PlayerReadyChanged
TeamRoleChanged
```

Reliable event message payload:

```text
event sequence
event type
server tick
entity IDs
team
score
message text where needed
```

Client must process each reliable event once.

Use sequence tracking to avoid duplicates.

---

# 20. Network Server

Files:

```text
Server/NetworkServer.hpp/cpp
Server/ServerClient.hpp/cpp
Server/ServerConnection.hpp/cpp
```

Implement:

```cpp
class NetworkServer {
public:
    Status Start(const NetworkSettings& settings);
    void Shutdown();

    void Update(float deltaSeconds);

    void BroadcastReliable(const Message& message);
    void BroadcastUnreliable(const Message& message);

    void SendToClient(ClientID client, NetworkChannel channel, const Message& message);

    bool IsRunning() const;
    std::vector<ServerClient>& Clients();
};
```

Server responsibilities:

```text
listen for connections
accept/reject clients
perform handshake
maintain clients
host lobby
start match
receive input
run authoritative gameplay through DedicatedServer integration
send snapshots
send reliable events
disconnect timed-out clients
collect stats
```

---

# 21. Network Client

Files:

```text
Client/NetworkClient.hpp/cpp
Client/ClientMatchConnection.hpp/cpp
```

Implement:

```cpp
class NetworkClient {
public:
    Status Connect(const NetworkAddress& address, const NetworkSettings& settings);
    void Disconnect(const std::string& reason);
    void Update(float deltaSeconds);

    void SendReliable(const Message& message);
    void SendUnreliable(NetworkChannel channel, const Message& message);

    NetworkConnectionState State() const;
    NetworkStats Stats() const;

    ClientLobby& Lobby();
    ClientMatchConnection& Match();
};
```

Client responsibilities:

```text
connect to server
handshake
enter lobby
send lobby selections
load match
send input
receive snapshots
receive reliable events
update prediction/reconciliation/interpolation
handle disconnect
```

---

# 22. Dedicated Server Integration

Update `HockeyDedicatedServer`.

Required behavior:

```text
load network settings
initialize Networking
start NetworkServer
create/default lobby
load gameplay scene
wait for clients
process lobby messages
start match when ready or dev command says start
run fixed tick gameplay/physics
apply latest input from clients
send snapshots at configured rate
broadcast reliable match events
handle disconnects
shutdown networking cleanly
remain headless
```

Add command-line options:

```text
--bind 0.0.0.0
--port 27020
--max-clients 8
--snapshot-rate 30
--start-when-ready
--auto-start
--bot-inputs
```

`--bot-inputs` may feed simple deterministic inputs for local testing without clients.

No renderer/editor/UI dependency.

---

# 23. Game Client Integration

Update `HockeyGameClient`.

Required behavior:

```text
load network settings
initialize Networking
support local/offline gameplay mode
support connect-to-server mode
connect to server from config/command-line
complete handshake
enter lobby
send team/role/ready selection through temporary UI/dev keys/config
receive lobby state
load match
send input frames
receive snapshots
apply interpolation for remote entities
apply prediction for local player
apply reconciliation
render replicated scene
show/log network stats if enabled
handle disconnect/reconnect to menu/offline state
shutdown networking cleanly
```

Command-line options:

```text
--connect 127.0.0.1:27020
--name Peter
--team Home
--role Skater
--slot HomeSkater0
--ready
```

Temporary controls are acceptable until final UI phase.

---

# 24. Local Lobby UX Without Final UI

Since final UI is not Phase 8, implement simple temporary control paths.

Options:

```text
command-line selection
developer console commands if project has console
keyboard shortcuts
simple debug overlay text if renderer has text support
logs
```

Minimum command-line flow:

Client can start as:

```bash
HockeyGameClient --connect 127.0.0.1:27020 --name Player1 --team Home --role Skater --slot HomeSkater0 --ready
```

Server starts as:

```bash
HockeyDedicatedServer --port 27020 --start-when-ready
```

This must allow local multiplayer testing without final UI.

---

# 25. Network Stats and Debugging

Implement stats display/logging.

Server logs:

```text
clients connected
ping per client
packet loss per client
input packets received
snapshots sent
bytes sent/received
disconnect reasons
malformed packets
rate-limit violations
```

Client logs:

```text
connection state
ping
packet loss
snapshots received
input packets sent
prediction corrections
reconciliation snaps
disconnect reason
```

If renderer/editor debug overlay exists, expose:

```text
ping
packet loss
incoming/outgoing kbps
snapshot buffer size
prediction error
server tick
client tick
```

No final UI required.

---

# 26. Security and Robustness Basics

Implement basic safeguards:

```text
protocol version check
packet magic check
payload size limit
string length limit
message type validation
malformed packet rejection
connection timeout
server full rejection
input rate limiting
slot ownership validation
ready/start validation
disconnect reason messages
```

Do not implement full anti-cheat service in Phase 8.

But server must be authoritative:

```text
clients never decide goals
clients never decide score
clients never decide puck authoritative state
clients never decide other player transforms
```

---

# 27. Tests

Create:

```text
HockeyNetworkTests
```

Use simple test harness.

## 27.1 MessageSerializationTests

Test:

```text
header serialize/deserialize
ClientHello serialize/deserialize
ServerHello serialize/deserialize
LobbyState serialize/deserialize
ClientInput serialize/deserialize
ServerSnapshot serialize/deserialize
ReliableMatchEvent serialize/deserialize
malformed payload rejected
oversized string rejected
wrong magic rejected
wrong version rejected
```

## 27.2 SequenceBufferTests

Test:

```text
insert/find sequence
remove sequence
wraparound works
old entries overwritten correctly
missing sequence returns null
```

## 27.3 AckTrackerTests

Test:

```text
sequence increments
received sequence tracked
ack bits generated
acked packets detected
wraparound handled
```

## 27.4 PacketLossTests

Test:

```text
sent packet tracked
acked packet tracked
lost packet detected
loss percent calculated
moving window works
```

## 27.5 FakeTransportTests

Test:

```text
fake client/server connect
send reliable message
send unreliable message
drop packet simulation
delay packet simulation
disconnect simulation
```

## 27.6 ClientServerConnectionTests

Test with local transport or actual GNS loopback:

```text
server starts
client connects
handshake succeeds
version mismatch rejected
server full rejected
client disconnect works
server timeout works
```

## 27.7 LobbyTests

Test:

```text
default lobby created
client joins lobby
client leaves lobby
ready state changes
lobby broadcasts state
start match rejected if slots invalid
start match succeeds when ready
```

## 27.8 TeamRoleSelectionTests

Test:

```text
select home skater slot
select away goalie slot
duplicate slot rejected
invalid role rejected
ready requires valid slot
8 players fill all slots
```

## 27.9 InputMessageTests

Test:

```text
input bitmask packs/unpacks
move/aim serialize
sequence preserved
tick preserved
server accepts assigned slot input
server rejects wrong slot input
rate limiter triggers
```

## 27.10 SnapshotSerializationTests

Test:

```text
snapshot serializes
snapshot deserializes
8 players preserved
puck preserved
match state preserved
last processed input preserved
quantization error within tolerance
```

## 27.11 InterpolationTests

Test:

```text
snapshot buffer stores snapshots
out-of-order snapshots handled
interpolated position correct
missing snapshots handled
large gaps handled
```

## 27.12 PredictionTests

Test:

```text
input history records
predicted state records
acked inputs cleared
unacked inputs returned
prediction reset works
```

## 27.13 ReconciliationTests

Test:

```text
small error ignored/smoothed
medium error corrected smoothly
large error snaps
acked inputs removed
unacked inputs replay path works or is prepared
puck correction works
```

## 27.14 DisconnectTests

Test:

```text
client disconnect reason sent
server removes client
lobby updates after disconnect
in-match disconnect handled
timeout disconnect works
```

## 27.15 HeadlessServerNetworkTests

Test:

```text
server starts headless
server accepts local client
server runs match tick
server receives input
server sends snapshots
server shuts down cleanly
server does not require renderer/editor
```

---

# 28. Local Multiplayer Scripts

Add scripts.

Windows:

```text
scripts/windows/run_network_server.ps1
scripts/windows/run_client_player1.ps1
scripts/windows/run_client_player2.ps1
scripts/windows/run_local_multiplayer.ps1
```

Linux:

```text
scripts/linux/run_network_server.sh
scripts/linux/run_client_player1.sh
scripts/linux/run_client_player2.sh
scripts/linux/run_local_multiplayer.sh
```

`run_local_multiplayer` should:

```text
start server
start at least two clients
pass different names/team/slot arguments
```

If launching multiple terminals is platform-specific, document command lines instead.

---

# 29. Implementation Order for AI Agents

Implement in this exact order.

## Step 1 — Dependency and Target Setup

```text
1. Add GameNetworkingSockets dependency to vcpkg.json.
2. Create libs/networking folder.
3. Create libs/networking/CMakeLists.txt.
4. Add hockey_networking target.
5. Create apps/network_tests.
6. Add HockeyNetworkTests target.
7. Update root CMakeLists.txt.
8. Link client/server/tests to hockey_networking.
9. Confirm server still does not link renderer/editor/ImGui.
10. Configure/build.
```

## Step 2 — Settings and Init

```text
1. Add NetworkSettings.
2. Add config load/save.
3. Add Networking init/shutdown wrapper.
4. Add NetworkTypes/NetworkResult.
5. Add config updates.
```

## Step 3 — Protocol and Serialization

```text
1. Add ProtocolVersion.
2. Add MessageType.
3. Add MessageHeader.
4. Add MessageWriter.
5. Add MessageReader.
6. Add MessageSerializer.
7. Add all message structs.
8. Add MessageSerializationTests.
```

## Step 4 — Reliability Helpers

```text
1. Add SequenceBuffer.
2. Add AckTracker.
3. Add PacketLossTracker.
4. Add RateLimiter.
5. Add tests for each.
```

## Step 5 — Transport

```text
1. Add NetworkAddress.
2. Add NetworkChannel.
3. Add NetworkConnectionState.
4. Add NetworkStats.
5. Add NetworkTransport interface.
6. Add GameNetworkingSocketsTransport.
7. Add FakePacketTransport for tests.
8. Add FakeTransportTests.
```

## Step 6 — Handshake and Connection

```text
1. Add ClientHello/ServerHello/Reject messages.
2. Add NetworkServer basic start/stop.
3. Add NetworkClient connect/disconnect.
4. Implement handshake.
5. Add ClientServerConnectionTests.
```

## Step 7 — Lobby

```text
1. Add LobbyTypes.
2. Add LobbyState.
3. Add TeamRoleSelection.
4. Add ServerLobby.
5. Add ServerLobbyManager.
6. Add ClientLobby.
7. Add lobby messages.
8. Add LobbyTests.
9. Add TeamRoleSelectionTests.
```

## Step 8 — Match Start Flow

```text
1. Add LoadMatch/MatchLoaded/MatchStart/MatchEnd messages.
2. Server assigns slots to gameplay.
3. Client loads match scene.
4. Server waits for loaded clients.
5. Server starts match.
```

## Step 9 — Input Networking

```text
1. Add ClientInput message.
2. Add ClientInputSystem.
3. Add ServerInputSystem.
4. Pack button bitmask.
5. Validate owner/slot on server.
6. Add input rate limiting.
7. Add InputMessageTests.
```

## Step 10 — Snapshots

```text
1. Add NetworkSnapshot types.
2. Add ServerSnapshot message.
3. Add ServerSnapshotSystem.
4. Serialize full snapshots.
5. Add quantization helpers.
6. Add SnapshotSerializationTests.
```

## Step 11 — Replication Components

```text
1. Add NetworkIdentityComponent.
2. Add NetworkOwnerComponent.
3. Add ReplicatedTransformComponent.
4. Add ReplicationComponent.
5. Add serialization/metadata.
6. Assign network IDs on match start.
```

## Step 12 — Client Interpolation, Prediction, Reconciliation

```text
1. Add SnapshotBuffer.
2. Add SnapshotInterpolation.
3. Add ClientPrediction.
4. Add Reconciliation.
5. Add tests.
6. Integrate into client match connection.
```

## Step 13 — Reliable Events

```text
1. Add ReliableMatchEvent messages.
2. Replicate gameplay events.
3. Process each event once on client.
4. Ensure score/period/goal events replicate.
```

## Step 14 — Dedicated Server Integration

```text
1. Load network settings.
2. Start NetworkServer.
3. Create default lobby.
4. Process lobby messages.
5. Start match.
6. Apply client inputs to gameplay.
7. Send snapshots.
8. Broadcast reliable events.
9. Handle disconnects.
10. Remain headless.
```

## Step 15 — Game Client Integration

```text
1. Load network settings.
2. Connect to server from config/command line.
3. Complete handshake.
4. Join default lobby.
5. Send team/role/ready.
6. Load match.
7. Send inputs.
8. Receive snapshots.
9. Interpolate remote players.
10. Predict local player.
11. Reconcile local state.
12. Display/log stats.
```

## Step 16 — Scripts and Final Verification

```text
1. Add network server/client scripts.
2. Add local multiplayer scripts.
3. Run all tests.
4. Run one server and two clients locally.
5. Confirm movement/puck/score replicate.
6. Confirm disconnect handling.
7. Confirm server remains headless.
```

---

# 30. Verification Commands

## Windows

```powershell
.\scripts\windows\configure_debug.ps1
.\scripts\windows\build_debug.ps1
.\build\windows-debug\apps\network_tests\HockeyNetworkTests.exe --root .
.\scripts\windows\run_network_server.ps1
.\scripts\windows\run_client_player1.ps1
.\scripts\windows\run_client_player2.ps1
```

Full test suite:

```powershell
.\build\windows-debug\apps\core_tests\HockeyCoreTests.exe --root .
.\build\windows-debug\apps\ecs_tests\HockeyECSTests.exe --root .
.\build\windows-debug\apps\renderer_tests\HockeyRendererTests.exe --root .
.\build\windows-debug\apps\editor_tests\HockeyEditorTests.exe --root .
.\build\windows-debug\apps\asset_tests\HockeyAssetTests.exe --root .
.\build\windows-debug\apps\physics_tests\HockeyPhysicsTests.exe --root .
.\build\windows-debug\apps\gameplay_tests\HockeyGameplayTests.exe --root .
.\build\windows-debug\apps\network_tests\HockeyNetworkTests.exe --root .
```

## Linux

```bash
./scripts/linux/configure_debug.sh
./scripts/linux/build_debug.sh
./build/linux-debug/apps/network_tests/HockeyNetworkTests --root .
./scripts/linux/run_network_server.sh
./scripts/linux/run_client_player1.sh
./scripts/linux/run_client_player2.sh
```

Full test suite:

```bash
./build/linux-debug/apps/core_tests/HockeyCoreTests --root .
./build/linux-debug/apps/ecs_tests/HockeyECSTests --root .
./build/linux-debug/apps/renderer_tests/HockeyRendererTests --root .
./build/linux-debug/apps/editor_tests/HockeyEditorTests --root .
./build/linux-debug/apps/asset_tests/HockeyAssetTests --root .
./build/linux-debug/apps/physics_tests/HockeyPhysicsTests --root .
./build/linux-debug/apps/gameplay_tests/HockeyGameplayTests --root .
./build/linux-debug/apps/network_tests/HockeyNetworkTests --root .
```

---

# 31. Phase 8 Definition of Done

Phase 8 is complete only when all of this is true:

```text
GameNetworkingSockets dependency added
hockey_networking builds
HockeyNetworkTests builds
HockeyNetworkTests pass
Networking init/shutdown works
NetworkSettings load from config
GameNetworkingSockets transport works
server listen works
client connect works
handshake works
protocol version reject works
reliable messages work
unreliable messages work
message serialization works
malformed packets rejected safely
sequence buffer works
ack tracker works
packet loss tracker works
rate limiter works
lobby create/join works
team selection works
role selection works
slot selection works
ready state works
match start flow works
client input messages work
server validates input ownership
server rate-limits input
server applies input to gameplay
server authoritative gameplay runs
server snapshots serialize
server sends snapshots
client receives snapshots
snapshot interpolation works
local player prediction works
server reconciliation works
puck state replicates
score/match events replicate
reliable event deduplication works
disconnect handling works
timeout handling works
network stats work
client can connect by command line
server can host by command line
local server + two clients can run
movement replication works
puck replication works
goal/score replication works
server remains headless
server does not link renderer/editor/ImGui
Windows build works
Linux build works
no final matchmaking service exists
no Steam/EOS integration exists
no final animation/audio/UI polish exists
```

---

# 32. AI Completion Instruction

When Phase 8 is complete, AI agent should report:

```text
Phase 8 complete.

Implemented:
- hockey_networking
- HockeyNetworkTests
- GameNetworkingSockets initialization
- transport abstraction
- server/client transport
- protocol versioning
- binary message serialization
- handshake flow
- reliable/unreliable messages
- sequence/ack/loss tracking
- rate limiting
- lobby state
- team/role/slot selection
- ready state
- match loading/start flow
- client input streaming
- authoritative server input handling
- network snapshots
- snapshot interpolation
- local prediction
- reconciliation
- replicated puck/player/match state
- reliable gameplay event replication
- disconnect/timeout handling
- network stats
- dedicated server integration
- game client integration
- local multiplayer scripts

Verified:
- network tests
- local client/server connection
- lobby flow
- match start
- input replication
- snapshot replication
- prediction/reconciliation
- goal/score event replication
- disconnect handling
- Windows/Linux builds
- server remains headless
- no future-phase final services/polish were implemented
```
