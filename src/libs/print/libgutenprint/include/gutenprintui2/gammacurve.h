/* GTK - The GIMP Toolkit
 * Copyright (C) 1997 David Mosberger
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

#ifndef GUTENPRINTUI_GAMMA_CURVE_H
#define GUTENPRINTUI_GAMMA_CURVE_H


#include <gdk/gdk.h>
#include <gtk/gtkvbox.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define STPUI_TYPE_GAMMA_CURVE            (stpui_gamma_curve_get_type ())
#define STPUI_GAMMA_CURVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), STPUI_TYPE_GAMMA_CURVE, StpuiGammaCurve))
#define STPUI_GAMMA_CURVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), STPUI_TYPE_GAMMA_CURVE, StpuiGammaCurveClass))
#define STPUI_IS_GAMMA_CURVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), STPUI_TYPE_GAMMA_CURVE))
#define STPUI_IS_GAMMA_CURVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), STPUI_TYPE_GAMMA_CURVE))
#define STPUI_GAMMA_CURVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), STPUI_TYPE_GAMMA_CURVE, StpuiGammaCurveClass))

typedef struct _StpuiGammaCurve		StpuiGammaCurve;
typedef struct _StpuiGammaCurveClass	StpuiGammaCurveClass;


struct _StpuiGammaCurve
{
  GtkVBox vbox;

  GtkWidget *table;
  GtkWidget *curve;
  GtkWidget *button[5];	/* spline, linear, free, gamma, reset */

  gfloat gamma;
  GtkWidget *gamma_dialog;
  GtkWidget *gamma_text;
};

struct _StpuiGammaCurveClass
{
  GtkVBoxClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


GType      stpui_gamma_curve_get_type (void) G_GNUC_CONST;
GtkWidget* stpui_gamma_curve_new      (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* GUTENPRINTUI_GAMMACURVE_H */
