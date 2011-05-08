


#include <gutenprintui2/gutenprintui.h>

/* enumerations from "../../include/gutenprintui2/gutenprintui.h" */
GType
orient_t_orient_t_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { ORIENT_AUTO, "ORIENT_AUTO", "auto" },
      { ORIENT_PORTRAIT, "ORIENT_PORTRAIT", "portrait" },
      { ORIENT_LANDSCAPE, "ORIENT_LANDSCAPE", "landscape" },
      { ORIENT_UPSIDEDOWN, "ORIENT_UPSIDEDOWN", "upsidedown" },
      { ORIENT_SEASCAPE, "ORIENT_SEASCAPE", "seascape" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("orient_t", values);
  }
  return etype;
}
GType
command_t_command_t_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { COMMAND_TYPE_DEFAULT, "COMMAND_TYPE_DEFAULT", "default" },
      { COMMAND_TYPE_CUSTOM, "COMMAND_TYPE_CUSTOM", "custom" },
      { COMMAND_TYPE_FILE, "COMMAND_TYPE_FILE", "file" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("command_t", values);
  }
  return etype;
}

/* enumerations from "../../include/gutenprintui2/curve.h" */
GType
stpui_curve_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { STPUI_CURVE_TYPE_LINEAR, "STPUI_CURVE_TYPE_LINEAR", "linear" },
      { STPUI_CURVE_TYPE_SPLINE, "STPUI_CURVE_TYPE_SPLINE", "spline" },
      { STPUI_CURVE_TYPE_FREE, "STPUI_CURVE_TYPE_FREE", "free" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("StpuiCurveType", values);
  }
  return etype;
}



