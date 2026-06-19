# Phase 9 Plan — Complete Animation, Audio, UI, Polish, Optimization, Packaging, and Release-Ready QA

File path recommendation:

```text
docs/phase-plans/phase-09-polish-animation-audio-ui.md
```

AI instruction:

```text
Read `docs/phase-plans/phase-09-polish-animation-audio-ui.md` and implement Phase 9 exactly. Build the complete presentation, usability, optimization, packaging, and release-readiness layer for the hockey game. This includes animation, audio, game UI/HUD, controls/settings UX, graphics/audio/input settings screens, multiplayer UI flow, debugging overlays, profiling, performance optimization, packaging, install/run layout, and QA validation. Do not rewrite the engine architecture from earlier phases. Keep the dedicated server headless. Keep Windows/Linux compatibility.
```

---

# 0. Phase Summary

Phase 9 makes the game feel complete and shippable.

Previous phases built:

```text
foundation/core
ECS/scene/prefab system
Vulkan renderer
Unity-style map editor
asset pipeline
physics system
hockey gameplay simulation
networking/lobbies/replication
```

Phase 9 builds:

```text
animation system
audio system
game UI
HUD
main menu
settings menu
lobby UI
team/role select UI
pause menu
scoreboard
network stats overlay
graphics/audio/input settings
controller binding UI
accessibility/basic UX settings
performance profiling
CPU/GPU/network optimization
asset memory budgeting
server stability checks
client/editor/server packaging
release config layout
QA test passes
```

Phase 9 must produce:

```text
hockey_animation
hockey_audio
hockey_ui
HockeyPolishTests
packaged client/editor/server builds
```

Phase 9 updates:

```text
HockeyGameClient
HockeyMapEditor
HockeyDedicatedServer
hockey_renderer
hockey_assets
hockey_gameplay
hockey_networking
data/config
data/raw
data/cooked
scripts/windows
scripts/linux
tools/package
```

Phase 9 must preserve:

```text
dedicated server remains headless
Windows support
Linux support
clean dependency boundaries
existing tests
existing editor workflows
existing networked gameplay
```

---

# 1. Architecture Rules

## 1.1 New Libraries

Add:

```text
hockey_animation -> hockey_core, hockey_ecs, hockey_assets
hockey_audio     -> hockey_core, hockey_ecs, hockey_assets
hockey_ui        -> hockey_core, hockey_ecs, hockey_renderer, hockey_assets, hockey_networking, hockey_gameplay
```

Optional:

```text
hockey_ui -> hockey_audio
```

Only if UI sounds are triggered through UI library.

## 1.2 App Links

Allowed:

```text
HockeyGameClient -> hockey_animation, hockey_audio, hockey_ui
HockeyMapEditor  -> hockey_animation, hockey_audio only if preview/editor support is implemented
HockeyMapEditor  -> hockey_ui only if editor uses shared UI widgets intentionally
```

Dedicated server must not link:

```text
hockey_renderer
hockey_editor
hockey_audio
hockey_ui
hockey_animation runtime if it requires rendering/audio
ImGui
Vulkan
audio backends
```

Dedicated server may link pure data helpers only if separated and safe, but default should be no animation/audio/UI dependencies.

## 1.3 Responsibilities

`hockey_animation` owns:

```text
skeleton data
animation clips
animation sampling
animation blending
state machine/animation graph
locomotion animation selection
gameplay-to-animation parameter mapping
network animation state hints
editor preview helpers
```

`hockey_audio` owns:

```text
audio backend initialization
sound asset loading
music
SFX
spatial audio emitters
audio mixer/buses
volume settings
UI sounds
hockey gameplay sound triggers
audio streaming if used
```

`hockey_ui` owns:

```text
main menu
settings screens
lobby UI
team/role selection UI
HUD
scoreboard
pause menu
network stats overlay
debug overlays
controller binding UI
input prompts
basic accessibility UI options
```

## 1.4 Must Not Rewrite

Do not rewrite:

```text
core app loop
ECS architecture
renderer architecture
asset database architecture
physics system
gameplay simulation
network transport/protocol
editor framework
```

Phase 9 integrates and polishes existing systems.

---

# 2. Dependencies

Add dependencies only if appropriate.

Recommended audio dependency:

```json
"miniaudio"
```

Optional UI dependencies if a retained/custom UI is not already planned:

```text
No new dependency if using existing ImGui-like debug UI only for development.
For in-game UI, prefer a lightweight custom UI renderer using existing renderer text/quad systems,
or add RmlUi only if explicitly chosen and integrated cleanly.
```

Recommended text/font dependencies if not already present:

```json
"freetype"
```

Optional:

```json
"msdfgen"
```

for high-quality signed distance field fonts, if available and desired.

Do not add heavyweight middleware unless explicitly approved:

```text
FMOD
Wwise
Steamworks
EOS
anti-cheat SDK
```

For Phase 9 default, use:

```text
miniaudio for audio
renderer-backed custom game UI
FreeType/MSDF optional for text
```

---

# 3. Target Project Structure

Add:

```text
libs/
  animation/
    CMakeLists.txt
    include/Hockey/Animation/
      Animation.hpp
      AnimationSettings.hpp
      Skeleton.hpp
      AnimationClip.hpp
      AnimationSampler.hpp
      AnimationPose.hpp
      AnimationBlend.hpp
      AnimationGraph.hpp
      AnimationStateMachine.hpp
      AnimationComponents.hpp
      AnimationSystem.hpp
      AnimationEvents.hpp
      AnimationSerializer.hpp
      HockeyAnimationController.hpp
    src/
      Animation.cpp
      AnimationSettings.cpp
      Skeleton.cpp
      AnimationClip.cpp
      AnimationSampler.cpp
      AnimationPose.cpp
      AnimationBlend.cpp
      AnimationGraph.cpp
      AnimationStateMachine.cpp
      AnimationComponents.cpp
      AnimationSystem.cpp
      AnimationEvents.cpp
      AnimationSerializer.cpp
      HockeyAnimationController.cpp

  audio/
    CMakeLists.txt
    include/Hockey/Audio/
      Audio.hpp
      AudioSettings.hpp
      AudioDevice.hpp
      AudioClip.hpp
      AudioSource.hpp
      AudioMixer.hpp
      AudioBus.hpp
      AudioListener.hpp
      AudioComponents.hpp
      AudioSystem.hpp
      AudioEvents.hpp
      AudioAssetLoader.hpp
      HockeyAudioController.hpp
    src/
      Audio.cpp
      AudioSettings.cpp
      AudioDevice.cpp
      AudioClip.cpp
      AudioSource.cpp
      AudioMixer.cpp
      AudioBus.cpp
      AudioListener.cpp
      AudioComponents.cpp
      AudioSystem.cpp
      AudioEvents.cpp
      AudioAssetLoader.cpp
      HockeyAudioController.cpp

  ui/
    CMakeLists.txt
    include/Hockey/UI/
      UI.hpp
      UISettings.hpp
      UISystem.hpp
      UIContext.hpp
      UITheme.hpp
      UIFont.hpp
      UIText.hpp
      UIImage.hpp
      UIButton.hpp
      UISlider.hpp
      UICheckbox.hpp
      UIDropdown.hpp
      UIInputBinding.hpp
      UILayout.hpp
      UIScreen.hpp
      ScreenManager.hpp
      Screens/
        MainMenuScreen.hpp
        SettingsScreen.hpp
        GraphicsSettingsScreen.hpp
        AudioSettingsScreen.hpp
        ControlsSettingsScreen.hpp
        LobbyScreen.hpp
        TeamRoleSelectScreen.hpp
        LoadingScreen.hpp
        PauseScreen.hpp
        MatchHUD.hpp
        ScoreboardOverlay.hpp
        NetworkStatsOverlay.hpp
        DebugOverlay.hpp
        EndMatchScreen.hpp
      HUD/
        ScoreClockWidget.hpp
        TeamScoreWidget.hpp
        PlayerIndicatorWidget.hpp
        PuckIndicatorWidget.hpp
        GoalNotificationWidget.hpp
        FaceoffNotificationWidget.hpp
      Input/
        InputBinding.hpp
        InputBindingSet.hpp
        InputPrompt.hpp
        ControllerGlyphs.hpp
      Rendering/
        UIRenderer.hpp
        FontRenderer.hpp
        NineSlice.hpp
    src/
      UI.cpp
      UISettings.cpp
      UISystem.cpp
      UIContext.cpp
      UITheme.cpp
      UIFont.cpp
      UIText.cpp
      UIImage.cpp
      UIButton.cpp
      UISlider.cpp
      UICheckbox.cpp
      UIDropdown.cpp
      UIInputBinding.cpp
      UILayout.cpp
      UIScreen.cpp
      ScreenManager.cpp
      Screens/
        MainMenuScreen.cpp
        SettingsScreen.cpp
        GraphicsSettingsScreen.cpp
        AudioSettingsScreen.cpp
        ControlsSettingsScreen.cpp
        LobbyScreen.cpp
        TeamRoleSelectScreen.cpp
        LoadingScreen.cpp
        PauseScreen.cpp
        MatchHUD.cpp
        ScoreboardOverlay.cpp
        NetworkStatsOverlay.cpp
        DebugOverlay.cpp
        EndMatchScreen.cpp
      HUD/
        ScoreClockWidget.cpp
        TeamScoreWidget.cpp
        PlayerIndicatorWidget.cpp
        PuckIndicatorWidget.cpp
        GoalNotificationWidget.cpp
        FaceoffNotificationWidget.cpp
      Input/
        InputBinding.cpp
        InputBindingSet.cpp
        InputPrompt.cpp
        ControllerGlyphs.cpp
      Rendering/
        UIRenderer.cpp
        FontRenderer.cpp
        NineSlice.cpp

apps/
  polish_tests/
    CMakeLists.txt
    src/
      PolishTestsMain.cpp
      Test.hpp
      AnimationTests.cpp
      AudioTests.cpp
      UITests.cpp
      InputBindingTests.cpp
      SettingsPersistenceTests.cpp
      PackagingLayoutTests.cpp
      PerformanceBudgetTests.cpp

tools/
  package/
    package_windows.ps1
    package_linux.sh
    package_common.py
    manifest_client.json
    manifest_editor.json
    manifest_server.json

data/
  raw/
    animation/
    audio/
      sfx/
      music/
      ambience/
      ui/
    ui/
      fonts/
      textures/
      themes/
  cooked/
    packages/
  config/
    client.toml
    editor.toml
    server.toml
```

---

# 4. CMake Requirements

## 4.1 Root CMake

Add:

```cmake
add_subdirectory(libs/animation)
add_subdirectory(libs/audio)
add_subdirectory(libs/ui)
```

When tests are enabled:

```cmake
add_subdirectory(apps/polish_tests)
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
animation
audio
ui
editor
apps
```

## 4.2 New Targets

Create:

```text
hockey_animation
hockey_audio
hockey_ui
HockeyPolishTests
```

## 4.3 Link Rules

`hockey_animation` links:

```text
hockey_core
hockey_ecs
hockey_assets
```

`hockey_audio` links:

```text
hockey_core
hockey_ecs
hockey_assets
miniaudio or chosen audio backend
```

`hockey_ui` links:

```text
hockey_core
hockey_ecs
hockey_renderer
hockey_assets
hockey_gameplay
hockey_networking
```

Optional:

```text
hockey_audio
```

Do not link `hockey_ui` to editor unless intentionally sharing editor widgets.

Dedicated server must not link:

```text
hockey_animation
hockey_audio
hockey_ui
hockey_renderer
hockey_editor
```

unless animation has a separate pure data target. Default: keep server free of these dependencies.

---

# 5. Animation System

## 5.1 Animation Settings

Files:

```text
AnimationSettings.hpp/cpp
```

Implement:

```cpp
struct AnimationSettings {
    bool enabled = true;
    bool enableAnimationEvents = true;
    bool enableRootMotion = false;

    float defaultBlendTimeSeconds = 0.15f;
    uint32_t maxBones = 128;

    bool debugDrawSkeletons = false;
    bool logAnimationEvents = false;
};
```

Config:

```toml
[animation]
enabled = true
enable_animation_events = true
enable_root_motion = false
default_blend_time_seconds = 0.15
max_bones = 128
debug_draw_skeletons = false
```

## 5.2 Skeleton

Files:

```text
Skeleton.hpp/cpp
```

Implement:

```cpp
struct Bone {
    std::string name;
    int parentIndex = -1;
    glm::mat4 inverseBindPose{1.0f};
};

struct Skeleton {
    std::vector<Bone> bones;
    std::unordered_map<std::string, int> boneNameToIndex;

    int FindBone(const std::string& name) const;
    bool IsValid() const;
};
```

## 5.3 Animation Clip

Files:

```text
AnimationClip.hpp/cpp
```

Implement:

```cpp
struct TranslationKey {
    float time = 0.0f;
    glm::vec3 value{0.0f};
};

struct RotationKey {
    float time = 0.0f;
    glm::quat value{};
};

struct ScaleKey {
    float time = 0.0f;
    glm::vec3 value{1.0f};
};

struct BoneTrack {
    int boneIndex = -1;
    std::vector<TranslationKey> translations;
    std::vector<RotationKey> rotations;
    std::vector<ScaleKey> scales;
};

struct AnimationClip {
    AssetID id;
    std::string name;
    float durationSeconds = 0.0f;
    float ticksPerSecond = 1.0f;
    bool looping = true;
    std::vector<BoneTrack> tracks;
};
```

## 5.4 Pose and Sampling

Files:

```text
AnimationPose.hpp/cpp
AnimationSampler.hpp/cpp
```

Implement:

```cpp
struct LocalBoneTransform {
    glm::vec3 translation{0.0f};
    glm::quat rotation{};
    glm::vec3 scale{1.0f};
};

struct AnimationPose {
    std::vector<LocalBoneTransform> localTransforms;
    std::vector<glm::mat4> modelSpaceMatrices;
    std::vector<glm::mat4> skinningMatrices;
};
```

Sampler:

```cpp
class AnimationSampler {
public:
    static void Sample(const AnimationClip& clip, const Skeleton& skeleton, float timeSeconds, AnimationPose& outPose);
    static void BuildModelSpacePose(const Skeleton& skeleton, AnimationPose& pose);
    static void BuildSkinningMatrices(const Skeleton& skeleton, AnimationPose& pose);
};
```

## 5.5 Blending

Files:

```text
AnimationBlend.hpp/cpp
```

Implement:

```cpp
class AnimationBlend {
public:
    static void BlendPoses(const AnimationPose& a, const AnimationPose& b, float alpha, AnimationPose& outPose);
    static void AdditiveBlend(const AnimationPose& base, const AnimationPose& additive, float weight, AnimationPose& outPose);
};
```

## 5.6 Animation Components

Files:

```text
AnimationComponents.hpp/cpp
```

Implement ECS components:

```cpp
struct SkeletonComponent {
    AssetID skeletonAsset;
};

struct AnimatorComponent {
    AssetID animationGraphAsset;
    std::string currentState;
    float playbackTime = 0.0f;
    float playbackSpeed = 1.0f;
    bool playing = true;
};

struct AnimationParameterComponent {
    float speed = 0.0f;
    bool hasPuck = false;
    bool shooting = false;
    bool passing = false;
    bool checking = false;
    bool goalieAction = false;
};
```

Serialize and register metadata.

## 5.7 Animation Graph / State Machine

Files:

```text
AnimationGraph.hpp/cpp
AnimationStateMachine.hpp/cpp
```

Implement simple state machine:

```text
state name
animation clip reference
loop flag
blend time
conditions
parameters
transitions
```

Required hockey states:

```text
Idle
SkateForward
SkateBackward
Turn
StickHandle
Shoot
Pass
Check
PokeCheck
GoalieIdle
GoalieMove
GoalieSave
Celebrate
```

## 5.8 HockeyAnimationController

Files:

```text
HockeyAnimationController.hpp/cpp
```

Maps gameplay state to animation parameters:

```text
player speed -> locomotion blend
has puck -> stick handling state
shoot event -> shoot animation
pass event -> pass animation
check event -> check animation
goalie action -> goalie save animation
goal scored -> celebration animation
```

No final art is required, but system must support real clips.

If no animation assets exist, use test clips or generated placeholder clips only inside tests/dev assets. Do not hardcode final behavior to placeholders.

---

# 6. Audio System

## 6.1 Audio Settings

Files:

```text
AudioSettings.hpp/cpp
```

Implement:

```cpp
struct AudioSettings {
    bool enabled = true;

    float masterVolume = 1.0f;
    float musicVolume = 0.7f;
    float sfxVolume = 0.9f;
    float ambienceVolume = 0.8f;
    float uiVolume = 0.8f;
    float crowdVolume = 0.8f;

    bool muteWhenUnfocused = false;
    bool enableSpatialAudio = true;

    uint32_t sampleRate = 48000;
    uint32_t channels = 2;
};
```

Config:

```toml
[audio]
enabled = true
master_volume = 1.0
music_volume = 0.7
sfx_volume = 0.9
ambience_volume = 0.8
ui_volume = 0.8
crowd_volume = 0.8
mute_when_unfocused = false
enable_spatial_audio = true
sample_rate = 48000
channels = 2
```

## 6.2 Audio Backend

Files:

```text
Audio.hpp/cpp
AudioDevice.hpp/cpp
```

Implement:

```cpp
class Audio {
public:
    static Status Init(const AudioSettings& settings);
    static void Shutdown();
    static bool IsInitialized();
};
```

AudioDevice:

```cpp
class AudioDevice {
public:
    Status Init(const AudioSettings& settings);
    void Shutdown();
    void Update();

    bool IsRunning() const;
};
```

Use miniaudio or chosen backend.

## 6.3 Audio Clip

Files:

```text
AudioClip.hpp/cpp
AudioAssetLoader.hpp/cpp
```

Support:

```text
.wav
.ogg if supported
.flac if supported
```

AudioClip:

```cpp
struct AudioClip {
    AssetID id;
    std::string name;
    uint32_t sampleRate = 48000;
    uint32_t channels = 2;
    float durationSeconds = 0.0f;
};
```

## 6.4 Audio Mixer and Buses

Files:

```text
AudioMixer.hpp/cpp
AudioBus.hpp/cpp
```

Buses:

```text
Master
Music
SFX
Ambience
UI
Crowd
```

Implement:

```cpp
enum class AudioBusType {
    Master,
    Music,
    SFX,
    Ambience,
    UI,
    Crowd
};
```

Features:

```text
volume per bus
mute per bus
apply master volume
runtime setting updates
```

## 6.5 Audio Components

Files:

```text
AudioComponents.hpp/cpp
```

Implement:

```cpp
struct AudioListenerComponent {
    bool primary = true;
};

struct AudioSourceComponent {
    AssetID clipAsset;
    AudioBusType bus = AudioBusType::SFX;

    bool playOnStart = false;
    bool loop = false;
    bool spatial = true;

    float volume = 1.0f;
    float pitch = 1.0f;

    float minDistance = 1.0f;
    float maxDistance = 50.0f;
};
```

Serialize and register metadata.

## 6.6 Audio System

Files:

```text
AudioSystem.hpp/cpp
```

Implement:

```cpp
class AudioSystem final : public System {
public:
    explicit AudioSystem(AudioSettings settings);

    void OnStart(Scene& scene) override;
    void OnStop(Scene& scene) override;
    void OnUpdate(Scene& scene, float deltaTime) override;

    void PlayOneShot(AssetID clip, AudioBusType bus, float volume = 1.0f);
    void PlayOneShotAt(AssetID clip, glm::vec3 position, AudioBusType bus, float volume = 1.0f);
};
```

## 6.7 HockeyAudioController

Files:

```text
HockeyAudioController.hpp/cpp
```

Listen to gameplay/network events and trigger:

```text
puck hit boards
puck hit stick
puck shot
puck pass
skate loop
skate stop
body check
goal scored
goal horn
crowd cheer
crowd ambience
faceoff
UI click
UI back
UI confirm
```

No final audio assets required, but routing and event hooks must exist.

---

# 7. UI System

## 7.1 UI Settings

Files:

```text
UISettings.hpp/cpp
```

Implement:

```cpp
struct UISettings {
    bool enabled = true;
    float uiScale = 1.0f;
    std::string language = "en-US";

    bool showFPS = false;
    bool showFrameTime = false;
    bool showNetworkStats = false;
    bool showDebugOverlay = false;

    bool colorBlindFriendly = false;
    bool reduceMotion = false;
    bool largeText = false;
};
```

Config:

```toml
[ui]
enabled = true
ui_scale = 1.0
language = "en-US"
show_fps = false
show_frame_time = false
show_network_stats = false
show_debug_overlay = false
color_blind_friendly = false
reduce_motion = false
large_text = false
```

## 7.2 UI Core

Files:

```text
UIContext.hpp/cpp
UISystem.hpp/cpp
UIScreen.hpp/cpp
ScreenManager.hpp/cpp
UILayout.hpp/cpp
UITheme.hpp/cpp
```

Implement:

```cpp
class UIScreen {
public:
    virtual ~UIScreen() = default;

    virtual const char* Name() const = 0;
    virtual void OnEnter(UIContext& context) {}
    virtual void OnExit(UIContext& context) {}
    virtual void OnUpdate(UIContext& context, float deltaTime) {}
    virtual void OnRender(UIContext& context) = 0;
};
```

ScreenManager:

```cpp
class ScreenManager {
public:
    void Push(std::unique_ptr<UIScreen> screen);
    void Pop();
    void Replace(std::unique_ptr<UIScreen> screen);
    UIScreen* Current();

    void Update(UIContext& context, float deltaTime);
    void Render(UIContext& context);
};
```

## 7.3 UI Rendering

Files:

```text
Rendering/UIRenderer.hpp/cpp
Rendering/FontRenderer.hpp/cpp
Rendering/NineSlice.hpp/cpp
```

Implement renderer-backed UI drawing:

```text
rectangles
images
text
buttons
sliders
checkboxes
dropdowns
panels
progress bars
```

Text rendering must support:

```text
font loading
font size
text measurement
alignment
basic wrapping
```

Use asset system for fonts/textures.

## 7.4 UI Widgets

Implement:

```text
UIButton
UISlider
UICheckbox
UIDropdown
UIText
UIImage
UIInputBinding
```

Widgets must support:

```text
mouse
keyboard navigation
gamepad navigation where possible
focus state
hover state
pressed state
disabled state
```

---

# 8. Game Screens

## 8.1 Main Menu

Files:

```text
Screens/MainMenuScreen.hpp/cpp
```

Buttons:

```text
Play Offline
Connect to Server
Host Local Server
Settings
Map Editor if launched separately or shortcut
Quit
```

## 8.2 Loading Screen

Files:

```text
Screens/LoadingScreen.hpp/cpp
```

Show:

```text
loading status
progress if available
scene name
connection status
```

## 8.3 Lobby Screen

Files:

```text
Screens/LobbyScreen.hpp/cpp
```

Show:

```text
connected players
team assignments
roles
ready state
ping
server name
start button for host/dev
leave button
```

## 8.4 Team/Role Select Screen

Files:

```text
Screens/TeamRoleSelectScreen.hpp/cpp
```

Allow:

```text
select Home/Away
select Skater/Goalie
select slot:
  HomeSkater0
  HomeSkater1
  HomeSkater2
  HomeGoalie
  AwaySkater0
  AwaySkater1
  AwaySkater2
  AwayGoalie
ready/unready
```

Must disable unavailable slots.

## 8.5 Settings Screen

Files:

```text
Screens/SettingsScreen.hpp/cpp
Screens/GraphicsSettingsScreen.hpp/cpp
Screens/AudioSettingsScreen.hpp/cpp
Screens/ControlsSettingsScreen.hpp/cpp
```

Settings categories:

```text
Graphics
Audio
Controls
Gameplay
Network
Accessibility
```

Graphics settings must edit renderer settings from Phase 3.

Audio settings must edit audio settings from Phase 9.

Controls settings must edit bindings.

Network settings can edit connect address/port and stats overlay toggles.

## 8.6 Match HUD

Files:

```text
Screens/MatchHUD.hpp/cpp
HUD/ScoreClockWidget.hpp/cpp
HUD/TeamScoreWidget.hpp/cpp
HUD/PlayerIndicatorWidget.hpp/cpp
HUD/PuckIndicatorWidget.hpp/cpp
HUD/GoalNotificationWidget.hpp/cpp
HUD/FaceoffNotificationWidget.hpp/cpp
```

HUD shows:

```text
home score
away score
period
clock
player role/team
puck possession indicator
goal notification
faceoff notification
connection warning
```

## 8.7 Pause Screen

Files:

```text
Screens/PauseScreen.hpp/cpp
```

Options:

```text
Resume
Settings
Leave Match
Quit to Desktop
```

## 8.8 Scoreboard Overlay

Files:

```text
Screens/ScoreboardOverlay.hpp/cpp
```

Show:

```text
players
team
role
score contribution placeholder
ping
ready/connected state
```

## 8.9 Network Stats Overlay

Files:

```text
Screens/NetworkStatsOverlay.hpp/cpp
```

Show:

```text
ping
packet loss
incoming kbps
outgoing kbps
snapshot rate
input send rate
interpolation buffer size
prediction error
server tick
client tick
```

## 8.10 End Match Screen

Files:

```text
Screens/EndMatchScreen.hpp/cpp
```

Show:

```text
winner
final score
return to lobby
main menu
```

---

# 9. Input Bindings and Controls UX

## 9.1 InputBinding

Files:

```text
Input/InputBinding.hpp/cpp
Input/InputBindingSet.hpp/cpp
Input/InputPrompt.hpp/cpp
Input/ControllerGlyphs.hpp/cpp
```

Implement action-based input:

```cpp
enum class InputAction {
    Move,
    Aim,
    Sprint,
    Shoot,
    Pass,
    Check,
    PokeCheck,
    GoalieAction,
    SwitchPlayer,
    Pause,
    Scoreboard,
    MenuConfirm,
    MenuBack,
    MenuUp,
    MenuDown,
    MenuLeft,
    MenuRight
};
```

Binding sets:

```text
keyboard/mouse
Xbox controller
PlayStation controller generic
Steam Deck/Linux gamepad generic if detectable
```

## 9.2 Rebinding UI

Controls settings screen must support:

```text
view bindings
change keyboard binding
change mouse binding
change gamepad binding
reset to defaults
detect conflicts
save/load bindings
```

## 9.3 Input Settings

Add settings:

```toml
[input]
mouse_sensitivity = 1.0
gamepad_look_sensitivity = 1.0
left_stick_deadzone = 0.12
right_stick_deadzone = 0.12
trigger_deadzone = 0.05
rumble_enabled = true
rumble_strength = 1.0
invert_y = false
```

---

# 10. Graphics Settings UI

Graphics settings screen must expose Phase 3 renderer settings:

```text
display mode
resolution
monitor
VSync
FPS limit
HDR
brightness
field of view
render scale
dynamic resolution
upscaler
sharpening
graphics preset
texture quality
anisotropy
shadow quality
shadow distance
ambient occlusion
reflection quality
model quality
arena detail
board/glass detail
bloom
motion blur
depth of field
lens flare
volumetric lighting
particle quality
ice quality
ice reflection quality
ice scratch quality
skate spray quality
puck trail quality
jersey quality
goal net quality
anti-aliasing
tone mapper
film grain
chromatic aberration
vignette
show FPS/frame time/GPU stats
```

Required behavior:

```text
Apply
Revert
Reset to Defaults
Some changes apply immediately.
Some changes require restart/recreate swapchain.
Show confirmation timer for display mode/resolution changes if implemented.
```

---

# 11. Audio Settings UI

Expose:

```text
master volume
music volume
SFX volume
ambience volume
UI volume
crowd volume
mute when unfocused
spatial audio
output device if backend supports it
test sound button
```

Changes must apply at runtime.

---

# 12. Gameplay and Network Settings UI

Gameplay settings:

```text
camera FOV if considered gameplay/camera
controller sensitivity
rumble
debug gameplay overlay
```

Network settings:

```text
server address
server port
player display name
show network stats
interpolation delay
prediction enabled
reconciliation enabled
packet logging developer toggle
```

Do not expose unsafe server-authoritative settings as client-controlled.

---

# 13. Accessibility and UX Options

Implement basic options:

```text
UI scale
large text
color-blind friendly team colors
reduce motion
disable camera shake if camera shake exists
disable chromatic aberration
subtitles/captions placeholder for future
```

Team colors should support a color-blind friendly palette.

---

# 14. Game Client Flow

Update `HockeyGameClient`.

Required flow:

```text
startup
load settings
initialize renderer/audio/ui/network/gameplay
show main menu
offline play path
connect to server path
lobby path
team/role select path
loading screen
match HUD
pause menu
end match screen
return to lobby/menu
shutdown cleanly
```

Temporary command-line flows from Phase 8 should still work.

## 14.1 Offline Play

Main menu:

```text
Play Offline
```

Behavior:

```text
load local scene
initialize gameplay/physics
create local players/bots or controllable player
start match
show HUD
```

Bot AI is not required unless simple deterministic idle/test inputs already exist.

## 14.2 Online Play

Main menu:

```text
Connect to Server
```

Behavior:

```text
connect to address
show connection status
enter lobby
select team/role
ready up
load match
play
```

---

# 15. Animation Integration

## 15.1 Gameplay to Animation

Animation system must observe gameplay state and set parameters:

```text
speed
hasPuck
shooting
passing
checking
goalieAction
match phase
celebration
```

## 15.2 Renderer Integration

Renderer must support skinned meshes if not already added.

Required renderer additions:

```text
skinned mesh vertex format
bone matrix buffer
skinned mesh shader path
animation pose upload
fallback static rendering if no skeleton
```

If skinned mesh support was already partly done in asset pipeline, finish it here.

## 15.3 Network Animation

Use networked gameplay state and event hints.

Do not replicate full bone transforms.

Replicate/derive:

```text
movement speed
facing direction
has puck
shot/pass/check events
goalie action
celebration state
```

---

# 16. Audio Integration

Audio should respond to:

```text
UI button click
UI confirm/back
menu music
arena ambience
crowd loop
skating loop
puck hit boards
puck hit stick
puck shot
puck pass
body check
goal horn
crowd cheer
faceoff
match start/end
```

Use gameplay events and physics contact events.

Implement event-to-sound mapping:

```text
GameplayEventType -> AudioCue
PhysicsContactEvent -> AudioCue based on materials/entities
```

Audio must respect volume buses.

---

# 17. Performance and Profiling

## 17.1 Profiling

Use existing or add Tracy if selected.

Track:

```text
CPU frame time
GPU frame time
network update time
physics step time
gameplay update time
animation update time
audio update time
UI update/render time
asset loading time
```

Add profiling zones/macros if project has none:

```cpp
HK_PROFILE_FRAME();
HK_PROFILE_SCOPE("Name");
```

If Tracy is used, isolate it behind macros so builds work without it.

## 17.2 Performance Budgets

Define target budgets:

```text
Client frame target: 60 FPS minimum
Client frame time target: <= 16.67 ms at target settings
Server tick target: 60 Hz
Server tick time target: <= 8 ms typical, <= 16 ms worst under 8 players
Network snapshot rate: 30 Hz default
Input rate: 60 Hz default
```

## 17.3 Optimization Passes

Perform:

```text
reduce per-frame allocations
cache ECS queries where appropriate
batch UI draw calls
batch renderer submissions where possible
avoid blocking GPU waits
avoid unnecessary asset reloads
optimize network serialization allocations
optimize snapshot interpolation buffers
optimize physics debug draw only when enabled
```

## 17.4 Performance Overlay

Debug overlay should show:

```text
FPS
frame time
CPU frame ms
GPU frame ms
physics ms
gameplay ms
render ms
UI ms
network ms
draw calls
triangles
VRAM usage
RAM usage if available
ping
packet loss
incoming/outgoing kbps
server tick
client tick
```

---

# 18. Memory and Asset Budgets

Implement budget reporting:

```text
texture memory
mesh memory
material count
audio memory/streaming memory
animation memory
UI texture/font memory
```

Settings/config:

```toml
[budgets]
texture_budget_mb = 4096
mesh_budget_mb = 1024
audio_budget_mb = 512
animation_budget_mb = 512
```

Warnings:

```text
log when budget exceeded
show in debug overlay/stats
```

---

# 19. Packaging

## 19.1 Package Targets

Create packaging scripts for:

```text
Windows Client
Linux Client
Windows Map Editor
Linux Map Editor
Windows Dedicated Server
Linux Dedicated Server
```

Output layout:

```text
dist/
  windows/
    client/
      HockeyGameClient.exe
      data/
      config/
      shaders/
      assets/
      licenses/
      README.txt

    editor/
      HockeyMapEditor.exe
      data/
      config/
      shaders/
      assets/
      licenses/
      README.txt

    server/
      HockeyDedicatedServer.exe
      data/
      config/
      licenses/
      README_SERVER.txt

  linux/
    client/
      HockeyGameClient
      data/
      config/
      shaders/
      assets/
      licenses/
      README.txt

    editor/
      HockeyMapEditor
      data/
      config/
      shaders/
      assets/
      licenses/
      README.txt

    server/
      HockeyDedicatedServer
      data/
      config/
      licenses/
      README_SERVER.txt
```

## 19.2 Package Scripts

Add:

```text
tools/package/package_windows.ps1
tools/package/package_linux.sh
tools/package/package_common.py
```

Scripts must:

```text
build release
copy executable
copy required dynamic libraries
copy cooked assets
copy shaders
copy default configs
copy licenses
copy README
exclude raw dev-only files from client/server packages unless configured
```

## 19.3 Runtime Path Rules

Packaged builds must not require:

```text
source tree
vcpkg tree
build tree
current working directory equals project root
```

The `Paths` system must resolve packaged data folders.

## 19.4 Server Package

Server package must exclude:

```text
renderer shaders if not needed
editor assets
UI textures if not needed
audio files if not needed
raw art/source files
```

Server package includes:

```text
server config
cooked gameplay/scene data required by server
licenses
README_SERVER
```

---

# 20. QA and Stability

## 20.1 Automated Test Suite

`HockeyPolishTests` should include:

```text
AnimationTests
AudioTests
UITests
InputBindingTests
SettingsPersistenceTests
PackagingLayoutTests
PerformanceBudgetTests
```

Run all previous tests too:

```text
HockeyCoreTests
HockeyECSTests
HockeyRendererTests
HockeyEditorTests
HockeyAssetTests
HockeyPhysicsTests
HockeyGameplayTests
HockeyNetworkTests
HockeyPolishTests
```

## 20.2 Manual QA Matrix

Test on Windows:

```text
client launches packaged
editor launches packaged
server launches packaged
offline match starts
network server starts
client connects to server
two clients connect to server
team/role selection works
match starts
movement works
puck works
goal/score works
pause menu works
settings save/reload
audio plays
controller works
keyboard/mouse works
disconnect handling works
```

Test on Linux:

```text
same as Windows
Wayland session if available
X11 session if available
gamepad detection
server headless over terminal
```

## 20.3 Long-Running Server Test

Run dedicated server for:

```text
1 hour minimum local stability test
```

Verify:

```text
no crash
no unbounded memory growth
tick time stable
client reconnect works
logs stay readable
```

## 20.4 Soak Test Script

Add:

```text
scripts/windows/soak_server.ps1
scripts/linux/soak_server.sh
```

Optional local bot/test input mode can be used.

---

# 21. Error Handling and User-Facing Messages

Improve user-facing error messages for:

```text
renderer init failure
missing Vulkan driver
missing assets
failed audio device
failed server connection
protocol mismatch
server full
kicked/disconnected
lost connection
failed scene load
failed settings save
```

Do not show raw internal exceptions to normal users.

Log detailed internal errors to log files.

---

# 22. Logging

Ensure logs are separated:

```text
client.log
editor.log
server.log
asset_tool.log
```

Add rotating logs if useful:

```text
client.previous.log
server.previous.log
```

Log levels configurable:

```toml
[logging]
level = "Info"
file_enabled = true
console_enabled = true
```

---

# 23. Release Configs

Create default release configs:

```text
data/config/default_client.toml
data/config/default_editor.toml
data/config/default_server.toml
```

Package copies these as:

```text
config/client.toml
config/editor.toml
config/server.toml
```

Do not overwrite user-edited config on update unless explicitly requested.

---

# 24. Implementation Order for AI Agents

Implement in this exact order.

## Step 1 — Target Setup

```text
1. Add chosen dependencies to vcpkg.json.
2. Create libs/animation.
3. Create libs/audio.
4. Create libs/ui.
5. Create apps/polish_tests.
6. Create CMake targets.
7. Update app links.
8. Verify dedicated server does not link animation/audio/ui/renderer/editor.
9. Configure/build.
```

## Step 2 — Animation Core

```text
1. Add AnimationSettings.
2. Add Skeleton.
3. Add AnimationClip.
4. Add AnimationPose.
5. Add AnimationSampler.
6. Add AnimationBlend.
7. Add AnimationComponents.
8. Add AnimationSystem.
9. Add HockeyAnimationController.
10. Add AnimationTests.
```

## Step 3 — Audio Core

```text
1. Add AudioSettings.
2. Add Audio init/shutdown.
3. Add AudioDevice.
4. Add AudioClip.
5. Add AudioMixer/Buses.
6. Add AudioComponents.
7. Add AudioSystem.
8. Add HockeyAudioController.
9. Add AudioTests.
```

## Step 4 — UI Core

```text
1. Add UISettings.
2. Add UIContext.
3. Add ScreenManager.
4. Add UIScreen.
5. Add UITheme.
6. Add UI layout/widgets.
7. Add UIRenderer.
8. Add FontRenderer.
9. Add UITests.
```

## Step 5 — Input Bindings

```text
1. Add InputAction enum.
2. Add InputBinding.
3. Add InputBindingSet.
4. Add default keyboard/mouse bindings.
5. Add default controller bindings.
6. Add conflict detection.
7. Add save/load bindings.
8. Add InputBindingTests.
```

## Step 6 — Game Screens

```text
1. Add MainMenuScreen.
2. Add LoadingScreen.
3. Add LobbyScreen.
4. Add TeamRoleSelectScreen.
5. Add SettingsScreen.
6. Add GraphicsSettingsScreen.
7. Add AudioSettingsScreen.
8. Add ControlsSettingsScreen.
9. Add PauseScreen.
10. Add EndMatchScreen.
```

## Step 7 — HUD and Overlays

```text
1. Add MatchHUD.
2. Add ScoreClockWidget.
3. Add TeamScoreWidget.
4. Add PlayerIndicatorWidget.
5. Add PuckIndicatorWidget.
6. Add GoalNotificationWidget.
7. Add FaceoffNotificationWidget.
8. Add ScoreboardOverlay.
9. Add NetworkStatsOverlay.
10. Add DebugOverlay.
```

## Step 8 — Client Flow

```text
1. Wire UI into HockeyGameClient.
2. Implement main menu flow.
3. Implement offline play flow.
4. Implement connect-to-server flow.
5. Implement lobby/team/role flow.
6. Implement loading screen.
7. Implement match HUD.
8. Implement pause/end match flows.
```

## Step 9 — Animation Integration

```text
1. Connect gameplay state to animation parameters.
2. Connect animation system to ECS update.
3. Add skinned mesh renderer support if missing.
4. Add network animation hints.
5. Verify fallback behavior without real animation assets.
```

## Step 10 — Audio Integration

```text
1. Connect UI sounds.
2. Connect gameplay event sounds.
3. Connect physics contact sounds.
4. Add music/ambience loop support.
5. Add runtime volume setting updates.
6. Verify audio shutdown/restart.
```

## Step 11 — Settings and Accessibility

```text
1. Wire graphics settings UI to renderer settings.
2. Wire audio settings UI to audio settings.
3. Wire controls settings UI to input bindings.
4. Wire network settings UI.
5. Add UI scale/large text/color-blind/reduce motion settings.
6. Add SettingsPersistenceTests.
```

## Step 12 — Profiling and Optimization

```text
1. Add profiling macros or Tracy integration.
2. Add frame/tick/network metrics.
3. Add debug overlay metrics.
4. Remove obvious per-frame allocations.
5. Optimize UI draw batching where possible.
6. Optimize network serialization allocations.
7. Add performance budget warnings.
8. Add PerformanceBudgetTests.
```

## Step 13 — Packaging

```text
1. Add tools/package scripts.
2. Add package manifests.
3. Package Windows client.
4. Package Windows editor.
5. Package Windows server.
6. Package Linux client.
7. Package Linux editor.
8. Package Linux server.
9. Verify packages run outside build tree.
10. Add PackagingLayoutTests.
```

## Step 14 — QA and Soak

```text
1. Run all automated tests.
2. Run Windows packaged client/editor/server.
3. Run Linux packaged client/editor/server.
4. Run local offline match.
5. Run local network server + two clients.
6. Run server soak test.
7. Fix crashes/leaks/log spam.
8. Verify dedicated server remains headless.
```

---

# 25. Verification Commands

## Windows

```powershell
.\scripts\windows\configure_debug.ps1
.\scripts\windows\build_debug.ps1

.\build\windows-debug\apps\core_tests\HockeyCoreTests.exe --root .
.\build\windows-debug\apps\ecs_tests\HockeyECSTests.exe --root .
.\build\windows-debug\apps\renderer_tests\HockeyRendererTests.exe --root .
.\build\windows-debug\apps\editor_tests\HockeyEditorTests.exe --root .
.\build\windows-debug\apps\asset_tests\HockeyAssetTests.exe --root .
.\build\windows-debug\apps\physics_tests\HockeyPhysicsTests.exe --root .
.\build\windows-debug\apps\gameplay_tests\HockeyGameplayTests.exe --root .
.\build\windows-debug\apps\network_tests\HockeyNetworkTests.exe --root .
.\build\windows-debug\apps\polish_tests\HockeyPolishTests.exe --root .

.\scripts\windows\run_client.ps1
.\scripts\windows\run_editor.ps1
.\scripts\windows\run_server.ps1

.\tools\package\package_windows.ps1
```

## Linux

```bash
./scripts/linux/configure_debug.sh
./scripts/linux/build_debug.sh

./build/linux-debug/apps/core_tests/HockeyCoreTests --root .
./build/linux-debug/apps/ecs_tests/HockeyECSTests --root .
./build/linux-debug/apps/renderer_tests/HockeyRendererTests --root .
./build/linux-debug/apps/editor_tests/HockeyEditorTests --root .
./build/linux-debug/apps/asset_tests/HockeyAssetTests --root .
./build/linux-debug/apps/physics_tests/HockeyPhysicsTests --root .
./build/linux-debug/apps/gameplay_tests/HockeyGameplayTests --root .
./build/linux-debug/apps/network_tests/HockeyNetworkTests --root .
./build/linux-debug/apps/polish_tests/HockeyPolishTests --root .

./scripts/linux/run_client.sh
./scripts/linux/run_editor.sh
./scripts/linux/run_server.sh

./tools/package/package_linux.sh
```

---

# 26. Phase 9 Definition of Done

Phase 9 is complete only when all of this is true:

```text
hockey_animation builds
hockey_audio builds
hockey_ui builds
HockeyPolishTests builds
HockeyPolishTests pass
animation settings load/save
skeleton data works
animation clip sampling works
pose blending works
animation graph/state machine works
hockey animation controller maps gameplay state
skinned mesh path works or documented fallback works
audio settings load/save
audio device initializes
audio clips load/play
audio mixer buses work
volume settings apply at runtime
UI sounds work
gameplay event sounds work
arena/music/crowd loops work
UI settings load/save
main menu works
offline play flow works
connect-to-server flow works
lobby UI works
team/role select UI works
loading screen works
match HUD works
scoreboard overlay works
network stats overlay works
pause menu works
end match screen works
graphics settings UI works
audio settings UI works
controls settings UI works
input rebinding works
controller prompts/glyph path works
accessibility options work
debug overlay works
profiling metrics work
performance budgets reported
packaging scripts work
Windows client package runs
Windows editor package runs
Windows server package runs
Linux client package runs
Linux editor package runs
Linux server package runs
packaged builds run outside source/build tree
server remains headless
server soak test passes
local offline match works
local network match works
settings persist
logs are useful
all previous tests still pass
Windows build works
Linux build works
```

---

# 27. AI Completion Instruction

When Phase 9 is complete, AI agent should report:

```text
Phase 9 complete.

Implemented:
- hockey_animation
- hockey_audio
- hockey_ui
- HockeyPolishTests
- animation clip/skeleton/pose/blend/state machine support
- hockey animation controller
- audio device/mixer/buses/clips/sources
- hockey audio event controller
- UI framework and renderer
- main menu
- settings screens
- lobby UI
- team/role select UI
- loading screen
- match HUD
- pause menu
- scoreboard
- network stats overlay
- debug overlay
- controls rebinding
- graphics/audio/input/network/accessibility settings
- profiling/performance metrics
- memory/asset budget reporting
- packaging scripts
- Windows/Linux client packages
- Windows/Linux editor packages
- Windows/Linux server packages
- QA/soak test scripts

Verified:
- all tests
- client offline flow
- client online flow
- editor launch/package
- server package/headless run
- local network match
- settings persistence
- audio/UI/animation integration
- Windows/Linux package execution
- no architecture boundaries were broken
```
