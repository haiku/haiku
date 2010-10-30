/* GTK - The GIMP Toolkit
 * Copyright (C) 1997 David Mosberger
 *               2004 Roger Leigh
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
 * Incorporated into Gutenprint by Roger Leigh in 2004.  Subsequent
 * changes are by the Gutenprint team.
 */

#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtkdialog.h>
#include <gtk/gtkdrawingarea.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktable.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkwindow.h>

#include <gutenprint/gutenprint-intl-internal.h>

#include <gutenprintui2/curve.h>
#include <gutenprintui2/gammacurve.h>

static GtkVBoxClass *parent_class = NULL;


/* forward declarations: */
static void stpui_gamma_curve_class_init (StpuiGammaCurveClass *class);
static void stpui_gamma_curve_init (StpuiGammaCurve *curve);
static void stpui_gamma_curve_destroy (GtkObject *object);

static void curve_type_changed_callback (GtkWidget *w, gpointer data);
static void button_realize_callback     (GtkWidget *w);
static void button_toggled_callback     (GtkWidget *w, gpointer data);
static void button_clicked_callback     (GtkWidget *w, gpointer data);

enum
  {
    LINEAR = 0,
    SPLINE,
    FREE,
    GAMMA,
    RESET,
    NUM_XPMS
  };

static const char *xpm[][27] =
  {
    /* spline: */
    {
      /* width height ncolors chars_per_pixel */
      "16 16 4 1",
      /* colors */
      ". c None",
      "B c #000000",
      "+ c #BC2D2D",
      "r c #FF0000",
      /* pixels */
      "..............BB",
      ".........rrrrrrB",
      ".......rr.......",
      ".....B+.........",
      "....BBB.........",
      "....+B..........",
      "....r...........",
      "...r............",
      "...r............",
      "..r.............",
      "..r.............",
      ".r..............",
      ".r..............",
      ".r..............",
      "B+..............",
      "BB.............."
    },
    /* linear: */
    {
      /* width height ncolors chars_per_pixel */
      "16 16 5 1",
      /* colors */
      ". c None", /* transparent */
      "B c #000000",
      "' c #7F7F7F",
      "+ c #824141",
      "r c #FF0000",
      /* pixels */
      "..............BB",
      "..............+B",
      "..............r.",
      ".............r..",
      ".............r..",
      "....'B'.....r...",
      "....BBB.....r...",
      "....+B+....r....",
      "....r.r....r....",
      "...r...r..r.....",
      "...r...r..r.....",
      "..r.....rB+.....",
      "..r.....BBB.....",
      ".r......'B'.....",
      "B+..............",
      "BB.............."
    },
    /* free: */
    {
      /* width height ncolors chars_per_pixel */
      "16 16 2 1",
      /* colors */
      ". c None",
      "r c #FF0000",
      /* pixels */
      "................",
      "................",
      "......r.........",
      "......r.........",
      ".......r........",
      ".......r........",
      ".......r........",
      "........r.......",
      "........r.......",
      "........r.......",
      ".....r...r.rrrrr",
      "....r....r......",
      "...r.....r......",
      "..r.......r.....",
      ".r........r.....",
      "r..............."
    },
    /* gamma: */
    {
      /* width height ncolors chars_per_pixel */
      "16 16 10 1",
      /* colors */
      ". c None",
      "B c #000000",
      "& c #171717",
      "# c #2F2F2F",
      "X c #464646",
      "= c #5E5E5E",
      "/ c #757575",
      "+ c #8C8C8C",
      "- c #A4A4A4",
      "` c #BBBBBB",
      /* pixels */
      "................",
      "................",
      "................",
      "....B=..+B+.....",
      "....X&`./&-.....",
      "...../+.XX......",
      "......B.B+......",
      "......X.X.......",
      "......X&+.......",
      "......-B........",
      "....../=........",
      "......#B........",
      "......BB........",
      "......B#........",
      "................",
      "................"
    },
    /* reset: */
    {
      /* width height ncolors chars_per_pixel */
      "16 16 4 1",
      /* colors */
      ". c None",
      "B c #000000",
      "+ c #824141",
      "r c #FF0000",
      /* pixels */
      "..............BB",
      "..............+B",
      ".............r..",
      "............r...",
      "...........r....",
      "..........r.....",
      ".........r......",
      "........r.......",
      ".......r........",
      "......r.........",
      ".....r..........",
      "....r...........",
      "...r............",
      "..r.............",
      "B+..............",
      "BB.............."
    }
  };

GType
stpui_gamma_curve_get_type (void)
{
  static GType gamma_curve_type = 0;

  if (!gamma_curve_type)
    {
      static const GTypeInfo gamma_curve_info =
      {
	sizeof (StpuiGammaCurveClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) stpui_gamma_curve_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (StpuiGammaCurve),
	0,		/* n_preallocs */
	(GInstanceInitFunc) stpui_gamma_curve_init,
      };

      gamma_curve_type = g_type_register_static (GTK_TYPE_VBOX, "StpuiGammaCurve",
						 &gamma_curve_info, 0);
    }
  return gamma_curve_type;
}

static void
stpui_gamma_curve_class_init (StpuiGammaCurveClass *class)
{
  GtkObjectClass *object_class;

  parent_class = g_type_class_peek_parent (class);

  object_class = (GtkObjectClass *) class;
  object_class->destroy = stpui_gamma_curve_destroy;
}

static void
stpui_gamma_curve_init (StpuiGammaCurve *curve)
{
  GtkWidget *vbox;
  int i;

  curve->gamma = 1.0;

  curve->table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (curve->table), 3);
  gtk_container_add (GTK_CONTAINER (curve), curve->table);

  curve->curve = stpui_curve_new ();
  g_signal_connect (curve->curve, "curve_type_changed",
		    G_CALLBACK (curve_type_changed_callback), curve);
  gtk_table_attach_defaults (GTK_TABLE (curve->table), curve->curve, 0, 1, 0, 1);

  vbox = gtk_vbox_new (/* homogeneous */ FALSE, /* spacing */ 3);
  gtk_table_attach (GTK_TABLE (curve->table), vbox, 1, 2, 0, 1, 0, 0, 0, 0);

  /* toggle buttons: */
  for (i = 0; i < 3; ++i)
    {
      curve->button[i] = gtk_toggle_button_new ();
      g_object_set_data (G_OBJECT (curve->button[i]), "_StpuiGammaCurveIndex",
			 GINT_TO_POINTER (i));
      gtk_container_add (GTK_CONTAINER (vbox), curve->button[i]);
      g_signal_connect (curve->button[i], "realize",
			G_CALLBACK (button_realize_callback), NULL);
      g_signal_connect (curve->button[i], "toggled",
			G_CALLBACK (button_toggled_callback), curve);
      gtk_widget_show (curve->button[i]);
    }

  /* push buttons: */
  for (i = 3; i < 5; ++i)
    {
      curve->button[i] = gtk_button_new ();
      g_object_set_data (G_OBJECT (curve->button[i]), "_StpuiGammaCurveIndex",
			 GINT_TO_POINTER (i));
      gtk_container_add (GTK_CONTAINER (vbox), curve->button[i]);
      g_signal_connect (curve->button[i], "realize",
			G_CALLBACK (button_realize_callback), NULL);
      g_signal_connect (curve->button[i], "clicked",
			G_CALLBACK (button_clicked_callback), curve);
      gtk_widget_show (curve->button[i]);
    }

  gtk_widget_show (vbox);
  gtk_widget_show (curve->table);
  gtk_widget_show (curve->curve);
}

static void
button_realize_callback (GtkWidget *w)
{
  GtkWidget *pixmap;
  GdkBitmap *mask;
  GdkPixmap *pm;
  int i;

  i = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (w), "_StpuiGammaCurveIndex"));
  pm = gdk_pixmap_create_from_xpm_d (w->window, &mask,
				     &w->style->bg[GTK_STATE_NORMAL], (gchar **)xpm[i]);

  pixmap = gtk_image_new_from_pixmap (pm, mask);
  gtk_container_add (GTK_CONTAINER (w), pixmap);
  gtk_widget_show (pixmap);

  g_object_unref (pm);
  g_object_unref (mask);
}

static void
button_toggled_callback (GtkWidget *w, gpointer data)
{
  StpuiGammaCurve *c = data;
  StpuiCurveType type;
  int active, i;

  if (!GTK_TOGGLE_BUTTON (w)->active)
    return;

  active = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (w), "_StpuiGammaCurveIndex"));

  for (i = 0; i < 3; ++i)
    if ((i != active) && GTK_TOGGLE_BUTTON (c->button[i])->active)
      break;

  if (i < 3)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (c->button[i]), FALSE);

  switch (active)
    {
    case 0:  type = STPUI_CURVE_TYPE_SPLINE; break;
    case 1:  type = STPUI_CURVE_TYPE_LINEAR; break;
    default: type = STPUI_CURVE_TYPE_FREE; break;
    }
  stpui_curve_set_curve_type (STPUI_CURVE (c->curve), type);
}

static void
gamma_cancel_callback (GtkWidget *w, gpointer data)
{
  StpuiGammaCurve *c = data;

  gtk_widget_destroy (c->gamma_dialog);
}

static void
gamma_ok_callback (GtkWidget *w, gpointer data)
{
  StpuiGammaCurve *c = data;
  const gchar *start;
  gchar *end;
  gfloat v;

  start = gtk_entry_get_text (GTK_ENTRY (c->gamma_text));
  if (start)
    {
      v = g_strtod (start, &end);
      if (end > start && v > 0.0)
	c->gamma = v;
    }
  stpui_curve_set_gamma (STPUI_CURVE (c->curve), c->gamma);
  gamma_cancel_callback (w, data);
}

static void
button_clicked_callback (GtkWidget *w, gpointer data)
{
  StpuiGammaCurve *c = data;
  int active;

  active = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (w), "_StpuiGammaCurveIndex"));
  if (active == 3)
    {
      /* set gamma */
      if (c->gamma_dialog)
	return;
      else
	{
	  GtkWidget *vbox, *hbox, *label, *button;
	  gchar buf[64];

	  c->gamma_dialog = gtk_dialog_new ();
	  gtk_window_set_screen (GTK_WINDOW (c->gamma_dialog),
				 gtk_widget_get_screen (w));
	  gtk_window_set_title (GTK_WINDOW (c->gamma_dialog), _("Gamma"));
	  g_object_add_weak_pointer (G_OBJECT (c->gamma_dialog),
				     (gpointer *)&c->gamma_dialog);

	  vbox = GTK_DIALOG (c->gamma_dialog)->vbox;

	  hbox = gtk_hbox_new (/* homogeneous */ FALSE, 0);
	  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 2);
	  gtk_widget_show (hbox);

	  label = gtk_label_new_with_mnemonic (_("_Gamma value"));
	  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
	  gtk_widget_show (label);

	  sprintf (buf, "%g", c->gamma);
	  c->gamma_text = gtk_entry_new ();
          gtk_label_set_mnemonic_widget (GTK_LABEL (label), c->gamma_text);
	  gtk_entry_set_text (GTK_ENTRY (c->gamma_text), buf);
	  gtk_box_pack_start (GTK_BOX (hbox), c->gamma_text, TRUE, TRUE, 2);
	  gtk_widget_show (c->gamma_text);

	  /* fill in action area: */
	  hbox = GTK_DIALOG (c->gamma_dialog)->action_area;

          button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	  g_signal_connect (button, "clicked",
			    G_CALLBACK (gamma_cancel_callback), c);
	  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
	  gtk_widget_show (button);

          button = gtk_button_new_from_stock (GTK_STOCK_OK);
	  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	  g_signal_connect (button, "clicked",
			    G_CALLBACK (gamma_ok_callback), c);
	  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
	  gtk_widget_grab_default (button);
	  gtk_widget_show (button);

	  gtk_widget_show (c->gamma_dialog);
	}
    }
  else
    {
      /* reset */
      stpui_curve_reset (STPUI_CURVE (c->curve));
    }
}

static void
curve_type_changed_callback (GtkWidget *w, gpointer data)
{
  StpuiGammaCurve *c = data;
  StpuiCurveType new_type;
  int active;

  new_type = STPUI_CURVE (w)->curve_type;
  switch (new_type)
    {
    case STPUI_CURVE_TYPE_SPLINE: active = 0; break;
    case STPUI_CURVE_TYPE_LINEAR: active = 1; break;
    default:		          active = 2; break;
    }
  if (!GTK_TOGGLE_BUTTON (c->button[active])->active)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (c->button[active]), TRUE);
}

GtkWidget*
stpui_gamma_curve_new (void)
{
  return g_object_new (STPUI_TYPE_GAMMA_CURVE, NULL);
}

static void
stpui_gamma_curve_destroy (GtkObject *object)
{
  StpuiGammaCurve *c;

  g_return_if_fail (STPUI_IS_GAMMA_CURVE (object));

  c = STPUI_GAMMA_CURVE (object);

  if (c->gamma_dialog)
    gtk_widget_destroy (c->gamma_dialog);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

