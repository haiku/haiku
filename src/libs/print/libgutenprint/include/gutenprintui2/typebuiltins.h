


#ifndef GUTENPRINTUI_TYPE_BUILTINS_H
#define GUTENPRINTUI_TYPE_BUILTINS_H

#include <glib-object.h>

G_BEGIN_DECLS
/* enumerations from "gutenprintui.h" */
GType orient_t_orient_t_get_type (void);
#define STPUI_TYPE_ORIENT_T (orient_t_orient_t_get_type())
GType command_t_command_t_get_type (void);
#define STPUI_TYPE_COMMAND_T (command_t_command_t_get_type())
/* enumerations from "curve.h" */
GType stpui_curve_type_get_type (void);
#define STPUI_TYPE_CURVE_TYPE (stpui_curve_type_get_type())
G_END_DECLS

#endif /* GUTENPRINTUI_TYPE_BUILTINS_H */



