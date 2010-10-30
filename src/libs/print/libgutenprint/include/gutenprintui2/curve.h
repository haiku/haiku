/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

/*
 * NOTE this widget is considered too specialized/little-used for
 * GTK+, and will in the future be moved to some other package.  If
 * your application needs this widget, feel free to use it, as the
 * widget does work and is useful in some applications; it's just not
 * of general interest. However, we are not accepting new features for
 * the widget, and it will eventually move out of the GTK+
 * distribution.
 */

#ifndef GUTENPRINTUI_CURVE_H
#define GUTENPRINTUI_CURVE_H


#include <gdk/gdk.h>
#include <gtk/gtkdrawingarea.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define STPUI_TYPE_CURVE                  (stpui_curve_get_type ())
#define STPUI_CURVE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), STPUI_TYPE_CURVE, StpuiCurve))
#define STPUI_CURVE_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), STPUI_TYPE_CURVE, StpuiCurveClass))
#define STPUI_IS_CURVE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), STPUI_TYPE_CURVE))
#define STPUI_IS_CURVE_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), STPUI_TYPE_CURVE))
#define STPUI_CURVE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), STPUI_TYPE_CURVE, StpuiCurveClass))


/* Curve types */
typedef enum
{
  STPUI_CURVE_TYPE_LINEAR,       /* linear interpolation */
  STPUI_CURVE_TYPE_SPLINE,       /* spline interpolation */
  STPUI_CURVE_TYPE_FREE          /* free form curve */
} StpuiCurveType;


typedef struct _StpuiCurve	StpuiCurve;
typedef struct _StpuiCurveClass	StpuiCurveClass;


struct _StpuiCurve
{
  GtkDrawingArea graph;

  gint cursor_type;
  gfloat min_x;
  gfloat max_x;
  gfloat min_y;
  gfloat max_y;
  GdkPixmap *pixmap;
  StpuiCurveType curve_type;
  gint height;                  /* (cached) graph height in pixels */
  gint grab_point;              /* point currently grabbed */
  gint last;

  /* (cached) curve points: */
  gint num_points;
  GdkPoint *point;

  /* control points: */
  gint num_ctlpoints;           /* number of control points */
  gfloat (*ctlpoint)[2];        /* array of control points */
};

struct _StpuiCurveClass
{
  GtkDrawingAreaClass parent_class;

  void (* curve_type_changed) (StpuiCurve *curve);

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


GType		stpui_curve_get_type	(void) G_GNUC_CONST;
GtkWidget*	stpui_curve_new		(void);
void		stpui_curve_reset	(StpuiCurve *curve);
void		stpui_curve_set_gamma	(StpuiCurve *curve, gfloat gamma_);
void		stpui_curve_set_range	(StpuiCurve *curve,
					 gfloat min_x, gfloat max_x,
					 gfloat min_y, gfloat max_y);
void		stpui_curve_get_vector	(StpuiCurve *curve,
					 int veclen, gfloat vector[]);
void		stpui_curve_set_vector	(StpuiCurve *curve,
					 int veclen, const gfloat vector[]);
void		stpui_curve_set_curve_type (StpuiCurve *curve, StpuiCurveType type);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* GUTENPRINTUI_CURVE_H */
