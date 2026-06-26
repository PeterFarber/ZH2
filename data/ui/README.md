# Runtime UI Assets

Runtime menu and HUD assets live under `data/ui`.

- `screens/`: RmlUi `.rml` documents for client screens.
- `themes/`: shared `.rcss` stylesheets. Phase 9 v1 avoids layers, filters, masks, and custom shaders.
- `fonts/`: runtime font files plus license/source notes.
- `flows/`: `.clientflow.yaml` assets that wire screens to gameplay/background scenes.

Menus are RmlUi documents. Gameplay worlds and optional menu backgrounds remain ECS scene assets.
