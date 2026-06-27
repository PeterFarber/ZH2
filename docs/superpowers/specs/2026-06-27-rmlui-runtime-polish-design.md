# RmlUi Runtime Polish Design

Date: 2026-06-27

## Goal

Polish the existing runtime RmlUi screens so the game UI feels more intentional and interactive without expanding Phase 9 scope into new UI systems. The work applies to all current RmlUi assets under `data/ui`: home, loading, lobby, team select, match HUD, pause menu, settings, scoreboard, and end match.

The polish must preserve existing element IDs, client-flow screen names, and current runtime actions.

## Scope

Update shared RmlUi assets only:

- `data/ui/themes/hockey.rcss`
- Current screen documents under `data/ui/screens/*.rml`
- UI asset contract tests under `apps/ui_tests`
- Phase 9 status notes if the verification state changes

The pass adds:

- A shared hockey broadcast visual style.
- Better menu layout structure with reusable classes.
- Button hover, active/click, focus, and disabled states.
- Cleaner HUD scoreboard, status, notification, possession, player, and shot-charge styling.
- Basic styling for range inputs used by settings.

## Non-Goals

This pass does not add:

- New RmlUi runtime C++ systems.
- UI sounds.
- Settings persistence behavior.
- New gameplay actions.
- New screen flow states.
- Network lobby behavior.
- Animation/audio integration.

## Visual Direction

Use a restrained hockey broadcast style:

- Dark translucent glass panels over the rendered scene.
- Ice-blue and cyan highlights for primary UI affordances.
- White and cool gray text for readability.
- Small red accents for away/opponent emphasis only where useful.
- Squared or lightly rounded panel and button shapes, avoiding oversized decorative cards.

The result should read as a functional game UI, not a marketing page.

## Screen Structure

Each menu-style screen uses a consistent structure:

- `screen` for full-screen placement.
- `menu-panel` for the main translucent panel.
- `eyebrow`, `title`, and optional `subtitle` for hierarchy.
- `button-stack` for grouped commands.
- `status` and `status-warning` for runtime messages.

Existing IDs remain unchanged. New classes only provide styling hooks.

The HUD uses separate classes:

- `hud-root`
- `score-strip`
- `score-team`
- `score`
- `hud-pill`
- `hud-side`
- `shot-meter`
- `notification`

## Interaction States

Buttons must expose clear visual feedback:

- Base: readable high-contrast fill, border, and text.
- Hover: brighter border/fill and stronger highlight.
- Active/click: slightly darker/pressed visual treatment.
- Focus: visible outline for keyboard/controller-style navigation.
- Disabled: muted fill, muted text, and no active-looking highlight.

No runtime code changes are required for these states; they are RCSS pseudo-class styles.

## Data Flow

The client flow still loads screens from `data/ui/flows/default.clientflow.yaml`. RmlUi documents continue to link the shared theme with:

```xml
<link type="text/rcss" href="../themes/hockey.rcss"/>
```

Game code continues to find controls by their existing IDs and bind live HUD text to the existing HUD IDs.

## Error Handling

Asset-level polish should not introduce new runtime error paths. The main failure mode is a missing or renamed ID breaking existing bindings, so tests must guard required IDs and screen assets.

The theme should avoid unsupported assumptions where possible and keep the documents valid RML.

## Testing

Add focused UI asset contract coverage before changing production assets:

- `hockey.rcss` contains button `:hover`, `:active`, `:focus`, and `:disabled` states.
- The shared theme contains the new layout classes used by screens.
- All existing screens keep their required IDs.
- HUD IDs remain present for score, clock, period, phase, player label, possession, shot charge, and notification.

Verification command:

```powershell
.\build\windows-debug\apps\ui_tests\HockeyUITests.exe --root .
```

If build artifacts are stale, rebuild `HockeyUITests` first with the existing CMake preset.

## Acceptance Criteria

- All current RmlUi screens use the polished shared theme.
- Buttons visibly respond to hover, active/click, focus, and disabled states.
- Menu screens have consistent panel layout and spacing.
- HUD presentation is cleaner and remains readable over gameplay.
- Existing RmlUi IDs and client-flow screen paths are preserved.
- UI asset tests pass.
- Phase 9 status reflects any changed verification coverage.
