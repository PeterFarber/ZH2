"""Temporary Blender operator shells for PBR bake/export workflows."""


def register():
    """Register temporary operator shells."""
    import bpy

    global ZH2_OT_PolyhavenSearch
    global ZH2_OT_PolyhavenDownload
    global ZH2_OT_BakeExportSelected
    global _REGISTERED_CLASSES

    class ZH2_OT_PolyhavenSearch(bpy.types.Operator):
        bl_idname = "zh2.polyhaven_search"
        bl_label = "Search Poly Haven"

        def execute(self, context):
            self.report({"INFO"}, "Poly Haven search operator registered")
            return {"FINISHED"}

    class ZH2_OT_PolyhavenDownload(bpy.types.Operator):
        bl_idname = "zh2.polyhaven_download"
        bl_label = "Download Material"

        def execute(self, context):
            self.report({"INFO"}, "Poly Haven download operator registered")
            return {"FINISHED"}

    class ZH2_OT_BakeExportSelected(bpy.types.Operator):
        bl_idname = "zh2.bake_export_selected"
        bl_label = "Bake + Export Selected"

        def execute(self, context):
            self.report({"INFO"}, "Bake and export operator registered")
            return {"FINISHED"}

    _REGISTERED_CLASSES = (
        ZH2_OT_PolyhavenSearch,
        ZH2_OT_PolyhavenDownload,
        ZH2_OT_BakeExportSelected,
    )

    for cls in _REGISTERED_CLASSES:
        bpy.utils.register_class(cls)


def unregister():
    """Unregister temporary operator shells."""
    import bpy

    global _REGISTERED_CLASSES

    for cls in reversed(globals().get("_REGISTERED_CLASSES", ())):
        try:
            bpy.utils.unregister_class(cls)
        except RuntimeError:
            pass

    _REGISTERED_CLASSES = ()
    globals().pop("ZH2_OT_BakeExportSelected", None)
    globals().pop("ZH2_OT_PolyhavenDownload", None)
    globals().pop("ZH2_OT_PolyhavenSearch", None)


_REGISTERED_CLASSES = ()
