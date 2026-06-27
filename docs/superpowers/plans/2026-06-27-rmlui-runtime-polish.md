# RmlUi Runtime Polish Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Polish all existing runtime RmlUi screens and add visible hover/click/focus button states without changing game-client behavior.

**Architecture:** This is an asset-level pass. The shared `hockey.rcss` owns the visual language and interaction states; the `.rml` screen files keep their existing IDs and only gain lightweight styling classes. UI asset contract tests guard the required IDs and the new visual-state/style hooks.

**Tech Stack:** RmlUi RML/RCSS assets, C++20 UI asset contract tests, CMake/Ninja Windows debug preset.

---

## File Structure

- Modify `apps/ui_tests/src/UIAssetContractTests.cpp`: add failing asset contract tests for interaction states, reusable layout classes, and expanded HUD IDs.
- Replace `data/ui/themes/hockey.rcss`: define the polished shared theme, button pseudo-classes, menu layout classes, HUD classes, and range input styling.
- Modify `data/ui/screens/home.rml`: add `menu-panel`, `eyebrow`, `subtitle`, `button-stack`, and primary/secondary button classes while preserving all IDs.
- Modify `data/ui/screens/loading.rml`: add menu-panel/status structure.
- Modify `data/ui/screens/lobby.rml`: add menu-panel/status/button-stack structure.
- Modify `data/ui/screens/team_select.rml`: add menu-panel/team-selection button classes.
- Modify `data/ui/screens/settings.rml`: add menu-panel/field-group structure.
- Modify `data/ui/screens/pause_menu.rml`: add menu-panel/button-stack structure.
- Modify `data/ui/screens/scoreboard.rml`: add menu-panel/scoreboard-row structure.
- Modify `data/ui/screens/end_match.rml`: add menu-panel/final-score structure.
- Modify `data/ui/screens/match_hud.rml`: add HUD classes around existing live IDs.
- Modify `docs/phase-status/phase-09-polish-animation-audio-ui.md`: record that first-pass RmlUi visual interaction styling and asset contract coverage exist.

## Task 1: Add Failing UI Asset Contracts

**Files:**
- Modify: `apps/ui_tests/src/UIAssetContractTests.cpp`

- [x] **Step 1: Write the failing tests**

Add the following checks inside `RunUIAssetContractTests()` after the existing screen existence loop and before the existing home-screen ID checks:

```cpp
    const std::string theme = ReadProjectFile("data/ui/themes/hockey.rcss");
    HK_CHECK_MSG(Contains(theme, "button:hover"), "theme defines button hover state");
    HK_CHECK_MSG(Contains(theme, "button:active"), "theme defines button active/click state");
    HK_CHECK_MSG(Contains(theme, "button:focus"), "theme defines button focus state");
    HK_CHECK_MSG(Contains(theme, "button:disabled"), "theme defines button disabled state");
    HK_CHECK_MSG(Contains(theme, ".menu-panel"), "theme defines menu panel layout");
    HK_CHECK_MSG(Contains(theme, ".button-stack"), "theme defines button stack layout");
    HK_CHECK_MSG(Contains(theme, ".hud-pill"), "theme defines HUD pill layout");
    HK_CHECK_MSG(Contains(theme, ".shot-meter"), "theme defines shot meter layout");
```

Extend the existing HUD checks with:

```cpp
    HK_CHECK_MSG(Contains(hud, "id=\"period-label\""), "HUD has period label id");
    HK_CHECK_MSG(Contains(hud, "id=\"phase-label\""), "HUD has phase label id");
    HK_CHECK_MSG(Contains(hud, "id=\"local-player-label\""), "HUD has local player label id");
    HK_CHECK_MSG(Contains(hud, "id=\"possession-label\""), "HUD has possession label id");
    HK_CHECK_MSG(Contains(hud, "id=\"hud-notification\""), "HUD has notification id");
    HK_CHECK_MSG(Contains(hud, "class=\"score-team home-team\""), "HUD styles home score team block");
    HK_CHECK_MSG(Contains(hud, "class=\"score-team away-team\""), "HUD styles away score team block");
    HK_CHECK_MSG(Contains(hud, "class=\"shot-meter\""), "HUD styles shot charge as a meter");
```

Add per-screen layout contract checks after each corresponding screen is read:

```cpp
    HK_CHECK_MSG(Contains(home, "class=\"menu-panel home-panel\""), "home screen uses polished menu panel");
    HK_CHECK_MSG(Contains(home, "class=\"button-stack\""), "home screen groups buttons in a polished stack");
```

Read and check the remaining menu screens:

```cpp
    const std::string loading = ReadProjectFile("data/ui/screens/loading.rml");
    HK_CHECK_MSG(Contains(loading, "class=\"menu-panel loading-panel\""), "loading screen uses polished menu panel");

    const std::string lobby = ReadProjectFile("data/ui/screens/lobby.rml");
    HK_CHECK_MSG(Contains(lobby, "id=\"lobby-back\""), "lobby screen keeps Back button id");
    HK_CHECK_MSG(Contains(lobby, "class=\"menu-panel lobby-panel\""), "lobby screen uses polished menu panel");

    const std::string teamSelect = ReadProjectFile("data/ui/screens/team_select.rml");
    HK_CHECK_MSG(Contains(teamSelect, "id=\"select-home-skater\""), "team select keeps Home Skater button id");
    HK_CHECK_MSG(Contains(teamSelect, "id=\"select-away-skater\""), "team select keeps Away Skater button id");
    HK_CHECK_MSG(Contains(teamSelect, "id=\"select-goalie\""), "team select keeps Goalie button id");
    HK_CHECK_MSG(Contains(teamSelect, "id=\"team-ready\""), "team select keeps Ready button id");
    HK_CHECK_MSG(Contains(teamSelect, "class=\"menu-panel team-panel\""), "team select uses polished menu panel");

    const std::string pause = ReadProjectFile("data/ui/screens/pause_menu.rml");
    HK_CHECK_MSG(Contains(pause, "id=\"resume-game\""), "pause menu keeps Resume button id");
    HK_CHECK_MSG(Contains(pause, "id=\"return-to-menu\""), "pause menu keeps Return to Menu button id");
    HK_CHECK_MSG(Contains(pause, "id=\"pause-quit\""), "pause menu keeps Quit button id");
    HK_CHECK_MSG(Contains(pause, "class=\"menu-panel pause-panel\""), "pause menu uses polished menu panel");

    const std::string settings = ReadProjectFile("data/ui/screens/settings.rml");
    HK_CHECK_MSG(Contains(settings, "id=\"ui-scale\""), "settings screen keeps UI scale input id");
    HK_CHECK_MSG(Contains(settings, "id=\"settings-back\""), "settings screen keeps Back button id");
    HK_CHECK_MSG(Contains(settings, "class=\"menu-panel settings-panel\""), "settings screen uses polished menu panel");

    const std::string scoreboard = ReadProjectFile("data/ui/screens/scoreboard.rml");
    HK_CHECK_MSG(Contains(scoreboard, "id=\"scoreboard-home\""), "scoreboard keeps home score id");
    HK_CHECK_MSG(Contains(scoreboard, "id=\"scoreboard-away\""), "scoreboard keeps away score id");
    HK_CHECK_MSG(Contains(scoreboard, "id=\"scoreboard-close\""), "scoreboard keeps Close button id");
    HK_CHECK_MSG(Contains(scoreboard, "class=\"menu-panel scoreboard-panel\""), "scoreboard uses polished menu panel");

    const std::string endMatch = ReadProjectFile("data/ui/screens/end_match.rml");
    HK_CHECK_MSG(Contains(endMatch, "id=\"final-score\""), "end match keeps final score id");
    HK_CHECK_MSG(Contains(endMatch, "id=\"end-return-to-menu\""), "end match keeps Return to Menu button id");
    HK_CHECK_MSG(Contains(endMatch, "class=\"menu-panel final-panel\""), "end match uses polished menu panel");
```

- [x] **Step 2: Build the test target**

Run:

```powershell
. .\scripts\windows\Env.ps1
cmake --build --preset build-windows-debug --target HockeyUITests
```

Expected: build succeeds because only tests changed.

- [x] **Step 3: Run the test to verify RED**

Run:

```powershell
.\build\windows-debug\apps\ui_tests\HockeyUITests.exe --root .
```

Expected: FAIL. The first expected failures are missing `button:hover`, `button:active`, `button:focus`, `.menu-panel`, `.button-stack`, `.hud-pill`, `.shot-meter`, and menu/HUD class hooks.

Do not commit the failing test alone.

## Task 2: Replace Shared RmlUi Theme

**Files:**
- Modify: `data/ui/themes/hockey.rcss`

- [x] **Step 1: Replace the shared theme**

Replace the full contents of `data/ui/themes/hockey.rcss` with:

```css
body {
    font-family: "Inter Variable";
    color: #f4f8fb;
    background-color: rgba(5, 11, 18, 184);
}

.screen {
    width: 100%;
    height: 100%;
    padding: 48px;
}

.menu-panel {
    width: 456px;
    padding: 28px;
    background-color: rgba(8, 18, 29, 224);
    border-width: 1px;
    border-color: #6fb8d8;
    border-radius: 8px;
}

.home-panel {
    margin-top: 44px;
}

.loading-panel,
.lobby-panel,
.team-panel,
.settings-panel,
.pause-panel,
.scoreboard-panel,
.final-panel {
    margin-top: 64px;
}

.eyebrow {
    margin-bottom: 8px;
    color: #7ed6f8;
    font-size: 13px;
    text-transform: uppercase;
}

.title {
    color: #ffffff;
    font-size: 44px;
    margin-bottom: 12px;
}

.subtitle {
    width: 360px;
    margin-bottom: 24px;
    color: #b8c7d9;
    font-size: 16px;
}

.button-stack {
    display: block;
    width: 344px;
}

button {
    display: block;
    width: 344px;
    min-height: 48px;
    margin-bottom: 12px;
    padding: 12px 18px;
    color: #f8fbff;
    background-color: #166596;
    border-width: 1px;
    border-color: #8edcff;
    border-radius: 6px;
    font-size: 16px;
}

button:hover {
    color: #ffffff;
    background-color: #2189c4;
    border-color: #d6f5ff;
}

button:active {
    color: #e7f6ff;
    background-color: #0e4b72;
    border-color: #64b7df;
}

button:focus {
    border-width: 2px;
    border-color: #ffffff;
}

button:disabled {
    color: #8e9aaa;
    background-color: #24303d;
    border-color: #465462;
}

.button-secondary {
    background-color: #1b3345;
    border-color: #5f91aa;
}

.button-danger {
    background-color: #6b2532;
    border-color: #d97883;
}

.status {
    margin-top: 12px;
    color: #b8c7d9;
    font-size: 15px;
}

.status-warning {
    color: #ffd88a;
}

.field-group {
    width: 360px;
    margin-bottom: 24px;
    padding: 16px;
    background-color: rgba(17, 32, 45, 210);
    border-width: 1px;
    border-color: #37586c;
    border-radius: 6px;
}

.field-label {
    display: block;
    margin-bottom: 8px;
    color: #e8f3fb;
}

input {
    width: 320px;
    height: 28px;
    margin-bottom: 8px;
}

.scoreboard-row {
    width: 344px;
    margin-bottom: 12px;
    padding: 14px 16px;
    background-color: rgba(16, 31, 45, 218);
    border-width: 1px;
    border-color: #426b82;
    border-radius: 6px;
    color: #f4f8fb;
    font-size: 22px;
}

.final-score {
    width: 344px;
    margin-bottom: 24px;
    padding: 18px;
    text-align: center;
    background-color: rgba(17, 32, 45, 218);
    border-width: 1px;
    border-color: #8edcff;
    border-radius: 6px;
    color: #ffffff;
    font-size: 34px;
}

.hud-root {
    width: 100%;
    height: 100%;
    color: #f6f9fc;
}

.score-strip {
    position: absolute;
    top: 22px;
    left: 50%;
    width: 560px;
    margin-left: -280px;
    padding: 10px 14px;
    background-color: rgba(6, 14, 23, 224);
    border-width: 1px;
    border-color: #7ed6f8;
    border-radius: 8px;
}

.score-team {
    display: inline-block;
    width: 126px;
}

.home-team {
    color: #bff1ff;
}

.away-team {
    color: #ffd1d7;
}

.score {
    font-size: 30px;
}

.clock-block {
    display: inline-block;
    width: 178px;
    text-align: center;
}

#match-clock {
    color: #ffffff;
    font-size: 24px;
}

#period-label {
    color: #a9bfd1;
    font-size: 14px;
}

.hud-side {
    position: absolute;
    left: 28px;
    bottom: 34px;
    width: 260px;
}

.hud-pill {
    width: 230px;
    margin-top: 8px;
    padding: 9px 12px;
    background-color: rgba(7, 16, 26, 212);
    border-width: 1px;
    border-color: #456b82;
    border-radius: 6px;
    color: #d9e8f2;
}

.shot-meter {
    position: absolute;
    right: 28px;
    bottom: 34px;
    width: 112px;
    padding: 12px;
    text-align: center;
    background-color: rgba(10, 28, 38, 220);
    border-width: 1px;
    border-color: #8edcff;
    border-radius: 8px;
    color: #ffffff;
    font-size: 24px;
}

.notification {
    position: absolute;
    top: 94px;
    left: 50%;
    width: 440px;
    margin-left: -220px;
    padding: 10px 14px;
    text-align: center;
    color: #ffffff;
    background-color: rgba(7, 16, 26, 196);
    border-width: 1px;
    border-color: #6fb8d8;
    border-radius: 6px;
}

#hud-notification {
    top: 144px;
    color: #ffd88a;
}
```

- [x] **Step 2: Run the UI tests to verify theme-only state**

Run:

```powershell
.\build\windows-debug\apps\ui_tests\HockeyUITests.exe --root .
```

Expected: still FAIL because the RML screens do not yet use the new class hooks, but the theme pseudo-class failures are gone.

Do not commit yet.

## Task 3: Update RML Screen Markup and Phase Status

**Files:**
- Modify: `data/ui/screens/home.rml`
- Modify: `data/ui/screens/loading.rml`
- Modify: `data/ui/screens/lobby.rml`
- Modify: `data/ui/screens/team_select.rml`
- Modify: `data/ui/screens/settings.rml`
- Modify: `data/ui/screens/pause_menu.rml`
- Modify: `data/ui/screens/scoreboard.rml`
- Modify: `data/ui/screens/end_match.rml`
- Modify: `data/ui/screens/match_hud.rml`
- Modify: `docs/phase-status/phase-09-polish-animation-audio-ui.md`

- [x] **Step 1: Replace `home.rml` body content**

Keep the existing `<head>` and replace the `<body>` block with:

```xml
<body>
    <div id="home-screen" class="screen">
        <div class="menu-panel home-panel">
            <div class="eyebrow">Runtime Preview</div>
            <div class="title">ZH2</div>
            <div class="subtitle">Fast four-on-four hockey with a dedicated simulation core.</div>
            <div class="button-stack">
                <button id="play-offline">Play Offline</button>
                <button id="open-lobby" class="button-secondary" disabled="disabled">Lobby</button>
                <button id="open-settings" class="button-secondary">Settings</button>
                <button id="quit-game" class="button-danger">Quit</button>
            </div>
            <div id="network-status" class="status status-warning">Networking unavailable</div>
        </div>
    </div>
</body>
```

- [x] **Step 2: Replace `loading.rml` body content**

```xml
<body>
    <div id="loading-screen" class="screen">
        <div class="menu-panel loading-panel">
            <div class="eyebrow">Game Flow</div>
            <div class="title">Loading</div>
            <div id="loading-status" class="status">Preparing the rink</div>
        </div>
    </div>
</body>
```

- [x] **Step 3: Replace `lobby.rml` body content**

```xml
<body>
    <div id="lobby-screen" class="screen">
        <div class="menu-panel lobby-panel">
            <div class="eyebrow">Online</div>
            <div class="title">Lobby</div>
            <div class="subtitle">Network lobby flow is reserved for Phase 8 integration.</div>
            <div id="lobby-status" class="status status-warning">Networking unavailable</div>
            <div class="button-stack">
                <button id="lobby-back" class="button-secondary">Back</button>
            </div>
        </div>
    </div>
</body>
```

- [x] **Step 4: Replace `team_select.rml` body content**

```xml
<body>
    <div id="team-select-screen" class="screen">
        <div class="menu-panel team-panel">
            <div class="eyebrow">Lineup</div>
            <div class="title">Team Select</div>
            <div class="subtitle">Choose a side and role before readying up.</div>
            <div class="button-stack">
                <button id="select-home-skater">Home Skater</button>
                <button id="select-away-skater" class="button-secondary">Away Skater</button>
                <button id="select-goalie" class="button-secondary">Goalie</button>
                <button id="team-ready">Ready</button>
            </div>
        </div>
    </div>
</body>
```

- [x] **Step 5: Replace `settings.rml` body content**

```xml
<body>
    <div id="settings-screen" class="screen">
        <div class="menu-panel settings-panel">
            <div class="eyebrow">Preferences</div>
            <div class="title">Settings</div>
            <div class="field-group">
                <label class="field-label">UI Scale</label>
                <input id="ui-scale" type="range" min="0.5" max="3.0" step="0.1"/>
                <div class="status">Adjusts runtime RmlUi scale.</div>
            </div>
            <div class="button-stack">
                <button id="settings-back" class="button-secondary">Back</button>
            </div>
        </div>
    </div>
</body>
```

- [x] **Step 6: Replace `pause_menu.rml` body content**

```xml
<body>
    <div id="pause-screen" class="screen">
        <div class="menu-panel pause-panel">
            <div class="eyebrow">Match</div>
            <div class="title">Paused</div>
            <div class="button-stack">
                <button id="resume-game">Resume</button>
                <button id="return-to-menu" class="button-secondary">Return to Menu</button>
                <button id="pause-quit" class="button-danger">Quit</button>
            </div>
        </div>
    </div>
</body>
```

- [x] **Step 7: Replace `scoreboard.rml` body content**

```xml
<body>
    <div id="scoreboard-screen" class="screen">
        <div class="menu-panel scoreboard-panel">
            <div class="eyebrow">Match</div>
            <div class="title">Scoreboard</div>
            <div id="scoreboard-home" class="scoreboard-row home-team">Home 0</div>
            <div id="scoreboard-away" class="scoreboard-row away-team">Away 0</div>
            <div class="button-stack">
                <button id="scoreboard-close" class="button-secondary">Close</button>
            </div>
        </div>
    </div>
</body>
```

- [x] **Step 8: Replace `end_match.rml` body content**

```xml
<body>
    <div id="end-match-screen" class="screen">
        <div class="menu-panel final-panel">
            <div class="eyebrow">Match Complete</div>
            <div class="title">Final</div>
            <div id="final-score" class="final-score">0 - 0</div>
            <div class="button-stack">
                <button id="end-return-to-menu">Return to Menu</button>
            </div>
        </div>
    </div>
</body>
```

- [x] **Step 9: Replace `match_hud.rml` body content**

```xml
<body>
    <div id="match-hud" class="hud-root">
        <div class="score-strip">
            <span class="score-team home-team">HOME <span id="home-score" class="score">0</span></span>
            <span class="clock-block">
                <span id="match-clock">20:00</span>
                <span id="period-label">P1</span>
            </span>
            <span class="score-team away-team">AWAY <span id="away-score" class="score">0</span></span>
        </div>
        <div id="phase-label" class="notification">Faceoff</div>
        <div class="hud-side">
            <div id="local-player-label" class="hud-pill">Player 1</div>
            <div id="possession-label" class="hud-pill">Loose puck</div>
        </div>
        <div id="shot-charge" class="shot-meter">0%</div>
        <div id="hud-notification" class="notification"></div>
    </div>
</body>
```

- [x] **Step 10: Update Phase 9 status**

In `docs/phase-status/phase-09-polish-animation-audio-ui.md`, update the existing Started / Partial bullet that begins `Runtime RmlUi foundation exists` to include:

```text
shared polished RCSS with button hover/active/focus/disabled states
```

Update the existing bullet that begins `First-pass RML/RCSS assets exist` to include:

```text
with first-pass polished layout classes across current runtime screens
```

- [x] **Step 11: Build `HockeyUITests`**

Run:

```powershell
. .\scripts\windows\Env.ps1
cmake --build --preset build-windows-debug --target HockeyUITests
```

Expected: build succeeds.

- [x] **Step 12: Run focused UI verification**

Run:

```powershell
.\build\windows-debug\apps\ui_tests\HockeyUITests.exe --root .
```

Expected: PASS with `0 failed`.

- [x] **Step 13: Review scoped diff**

Run:

```powershell
git diff -- apps\ui_tests\src\UIAssetContractTests.cpp data\ui\themes\hockey.rcss data\ui\screens\home.rml data\ui\screens\loading.rml data\ui\screens\lobby.rml data\ui\screens\team_select.rml data\ui\screens\settings.rml data\ui\screens\pause_menu.rml data\ui\screens\scoreboard.rml data\ui\screens\end_match.rml data\ui\screens\match_hud.rml docs\phase-status\phase-09-polish-animation-audio-ui.md
```

Expected: diff contains only the RmlUi polish, tests, and Phase 9 note.

- [x] **Step 14: Commit only the scoped changes**

First confirm there is no unrelated staged work:

```powershell
git diff --cached --name-status
```

Expected: no output.

Stage and commit only this task's files:

```powershell
git add -- apps\ui_tests\src\UIAssetContractTests.cpp data\ui\themes\hockey.rcss data\ui\screens\home.rml data\ui\screens\loading.rml data\ui\screens\lobby.rml data\ui\screens\team_select.rml data\ui\screens\settings.rml data\ui\screens\pause_menu.rml data\ui\screens\scoreboard.rml data\ui\screens\end_match.rml data\ui\screens\match_hud.rml docs\phase-status\phase-09-polish-animation-audio-ui.md
git commit -m "Polish runtime RmlUi screens"
```

Expected: commit includes only the listed files.

## Final Verification

- [x] **Step 1: Run focused verification after commit**

Run:

```powershell
.\build\windows-debug\apps\ui_tests\HockeyUITests.exe --root .
```

Expected: PASS with `0 failed`.

- [x] **Step 2: Report phase status**

Report that Phase 9 status changed because the RmlUi visual interaction styling and UI asset contract coverage changed.
