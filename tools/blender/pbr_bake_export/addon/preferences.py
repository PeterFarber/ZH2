"""Blender preferences and scene settings for PBR bake/export tooling."""


def _resolution_items():
    return (
        ("4096", "4K", "Bake 4096 x 4096 textures"),
        ("8192", "8K", "Bake 8192 x 8192 textures"),
        ("128", "Test 128", "Bake 128 x 128 textures for development verification"),
    )


def register():
    """Register addon preferences and scene settings."""
    import bpy

    global ZH2PBRBakeExportPreferences
    global ZH2PBRBakeExportSettings
    global _REGISTERED_CLASSES

    class ZH2PBRBakeExportPreferences(bpy.types.AddonPreferences):
        bl_idname = __package__

        user_agent: bpy.props.StringProperty(
            name="User Agent",
            default="ZH2-Blender-PBR-Bake-Export/0.1",
        )
        cache_root: bpy.props.StringProperty(
            name="Poly Haven Cache Root",
            default="//assets/_polyhaven_cache",
            subtype="DIR_PATH",
        )

        def draw(self, context):
            layout = self.layout
            layout.prop(self, "user_agent")
            layout.prop(self, "cache_root")

    class ZH2PBRBakeExportSettings(bpy.types.PropertyGroup):
        output_root: bpy.props.StringProperty(
            name="Output Root",
            default="//assets/blender_exports",
            subtype="DIR_PATH",
        )
        texture_resolution: bpy.props.EnumProperty(
            name="Texture Resolution",
            items=_resolution_items(),
            default="4096",
        )
        asset_mode: bpy.props.EnumProperty(
            name="Asset Mode",
            items=(
                ("HIGH", "High Poly", "Export the selected high-poly asset"),
                ("LOW", "Low Poly", "Generate a low-poly asset and baked texture set"),
            ),
            default="LOW",
        )
        autogenerate_low_poly: bpy.props.BoolProperty(
            name="Autogenerate Low Poly",
            default=True,
        )
        decimate_ratio: bpy.props.FloatProperty(
            name="Decimate Ratio",
            default=0.35,
            min=0.01,
            max=1.0,
        )
        material_source: bpy.props.EnumProperty(
            name="Material Source",
            items=(
                ("EXISTING", "Existing Material", "Use the selected object's material"),
                ("POLY_HAVEN", "Poly Haven", "Use a material downloaded from Poly Haven"),
            ),
            default="EXISTING",
        )
        polyhaven_query: bpy.props.StringProperty(
            name="Poly Haven Query",
            default="",
        )
        selected_polyhaven_asset: bpy.props.StringProperty(
            name="Selected Asset",
            default="",
        )
        allow_resolution_fallback: bpy.props.BoolProperty(
            name="Allow Resolution Fallback",
            default=True,
        )
        overwrite_existing: bpy.props.BoolProperty(
            name="Overwrite Existing Files",
            default=False,
        )

    _REGISTERED_CLASSES = (
        ZH2PBRBakeExportPreferences,
        ZH2PBRBakeExportSettings,
    )

    for cls in _REGISTERED_CLASSES:
        bpy.utils.register_class(cls)

    bpy.types.Scene.zh2_pbr_bake_export = bpy.props.PointerProperty(type=ZH2PBRBakeExportSettings)


def unregister():
    """Unregister addon preferences and scene settings."""
    import bpy

    global _REGISTERED_CLASSES

    if hasattr(bpy.types.Scene, "zh2_pbr_bake_export"):
        del bpy.types.Scene.zh2_pbr_bake_export

    for cls in reversed(globals().get("_REGISTERED_CLASSES", ())):
        try:
            bpy.utils.unregister_class(cls)
        except RuntimeError:
            pass

    _REGISTERED_CLASSES = ()
    globals().pop("ZH2PBRBakeExportSettings", None)
    globals().pop("ZH2PBRBakeExportPreferences", None)


_REGISTERED_CLASSES = ()
