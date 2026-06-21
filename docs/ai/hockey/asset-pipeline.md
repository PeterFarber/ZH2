# Hockey Asset Pipeline

Use this guide for hockey asset scale, orientation, naming, collision proxies,
materials, texture paths, import/export checks, and Blender MCP asset work.

## Asset Goals

Assets should be consistent, testable in-game, and easy to replace as quality
improves.

## Important Checks

When working with assets, check:

- scale
- origin/pivot
- forward/up axes
- object names
- material names
- texture paths
- collision proxies
- export format
- runtime import expectations

## Hockey Asset Categories

Common ZH2 assets include:

- player/skater
- stick
- puck
- goal frame and net
- boards
- glass
- ice surface
- arena props
- collision proxy meshes

## Suggested Naming

```text
player_body
player_stick
puck_render
puck_collision
goal_frame
goal_net
rink_boards_collision
ice_surface
```

## Blender MCP Rule

Use Blender MCP only for asset-related work. Do not use it for unrelated source
edits, build configuration, or shell operations.

## Completion Notes

Report:

- source asset
- export path
- scale/orientation assumptions
- materials/textures changed
- collision proxy status
- manual verification needed in Blender or the game
