"""Blender addon entrypoint for PBR bake/export tooling."""

bl_info = {
    "name": "Hockey PBR Bake Export",
    "author": "HockeyGame",
    "version": (0, 1, 0),
    "blender": (4, 0, 0),
    "location": "File > Export",
    "description": "Bake PBR texture sets and export game-ready GLB assets.",
    "category": "Import-Export",
}

from . import operators, panel, preferences


MODULES = (preferences, operators, panel)


def register():
    """Register Blender addon types."""
    for module in MODULES:
        module.register()


def unregister():
    """Unregister Blender addon types."""
    for module in reversed(MODULES):
        module.unregister()
