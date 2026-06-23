"""3D View UI panel for PBR bake/export tooling."""


def register():
    """Register the PBR bake/export panel."""
    import bpy

    global ZH2_PT_PBRBakeExportPanel

    class ZH2_PT_PBRBakeExportPanel(bpy.types.Panel):
        bl_idname = "ZH2_PT_pbr_bake_export"
        bl_label = "PBR Bake Export"
        bl_space_type = "VIEW_3D"
        bl_region_type = "UI"
        bl_category = "ZH2 Assets"

        def draw(self, context):
            layout = self.layout
            settings = context.scene.zh2_pbr_bake_export

            layout.prop(settings, "output_root")
            layout.prop(settings, "texture_resolution")
            layout.prop(settings, "asset_mode")

            if settings.asset_mode == "LOW":
                layout.prop(settings, "autogenerate_low_poly")
                layout.prop(settings, "decimate_ratio")

            layout.prop(settings, "material_source")

            if settings.material_source == "POLY_HAVEN":
                layout.prop(settings, "polyhaven_query")
                layout.operator("zh2.polyhaven_search")
                layout.prop(settings, "selected_polyhaven_asset")
                layout.prop(settings, "allow_resolution_fallback")
                layout.operator("zh2.polyhaven_download")
                layout.prop(settings, "selected_downloaded_polyhaven_material")
                layout.operator("zh2.polyhaven_apply_material")

            layout.prop(settings, "overwrite_existing")
            row = layout.row(align=True)
            row.operator("zh2.bake_selected")
            row.operator("zh2.export_selected")

    bpy.utils.register_class(ZH2_PT_PBRBakeExportPanel)


def unregister():
    """Unregister the PBR bake/export panel."""
    import bpy

    cls = globals().get("ZH2_PT_PBRBakeExportPanel")
    if cls is not None:
        try:
            bpy.utils.unregister_class(cls)
        except RuntimeError:
            pass
        globals().pop("ZH2_PT_PBRBakeExportPanel", None)
