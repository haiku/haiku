/*
 * "$Id: ui-utils.c,v 1.3 2006/05/28 16:59:04 rlk Exp $"
 *
 *   Main window code for Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu), Steve Miller (smiller@rni.net)
 *      and Michael Natterer (mitch@gimp.org)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gutenprint/gutenprint-intl-internal.h>
#include <gutenprintui2/gutenprintui.h>
#include "gutenprintui-internal.h"

#include <stdio.h>
#include <string.h>

static void default_errfunc(void *data, const char *buffer, size_t bytes);
static gchar *image_filename;
static stp_outfunc_t the_errfunc = default_errfunc;
static void *the_errdata = NULL;
static get_thumbnail_func_t thumbnail_func;
static void *thumbnail_private_data;

/*****************************************************************\
*                                                                 *
*             The following from libgimp/gimpdialog.c             *
*                                                                 *
\*****************************************************************/

typedef void (*StpuiBasicCallback) (GObject *object,
				    gpointer user_data);

/*  local callbacks of dialog_new ()  */
static gint
dialog_delete_callback (GtkWidget *widget,
			GdkEvent  *event,
			gpointer   data)
{
  StpuiBasicCallback cancel_callback;
  GtkWidget     *cancel_widget;

  cancel_callback =
    (StpuiBasicCallback) g_object_get_data (G_OBJECT (widget),
					    "dialog_cancel_callback");
  cancel_widget =
    (GtkWidget*) g_object_get_data (G_OBJECT (widget),
				    "dialog_cancel_widget");

  /*  the cancel callback has to destroy the dialog  */
  if (cancel_callback)
    (* cancel_callback) (G_OBJECT(cancel_widget), data);

  return TRUE;
}

/**
 * dialog_create_action_areav:
 * @dialog: The #GtkDialog you want to create the action_area for.
 * @args: A @va_list as obtained with va_start() describing the action_area
 *        buttons.
 *
 */
static void
dialog_create_action_areav (GtkDialog *dialog,
			    va_list    args)
{
  GtkWidget *hbbox = NULL;
  GtkWidget *button;

  /*  action area variables  */
  const gchar    *label;
  GtkSignalFunc   callback;
  gpointer        data;
  GObject        *slot_object;
  GtkWidget     **widget_ptr;
  gboolean        default_action;
  gboolean        connect_delete;

  gboolean delete_connected = FALSE;

  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTK_IS_DIALOG (dialog));

  /*  prepare the action_area  */
  label = va_arg (args, const gchar *);

  if (label)
    {
      gtk_container_set_border_width (GTK_CONTAINER (dialog->action_area), 2);
      gtk_box_set_homogeneous (GTK_BOX (dialog->action_area), FALSE);

      hbbox = gtk_hbutton_box_new ();
      gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
      gtk_box_pack_end (GTK_BOX (dialog->action_area), hbbox, FALSE, FALSE, 0);
      gtk_widget_show (hbbox);
    }

  /*  the action_area buttons  */
  while (label)
    {
      callback       = va_arg (args, GtkSignalFunc);
      data           = va_arg (args, gpointer);
      slot_object    = va_arg (args, GObject *);
      widget_ptr     = va_arg (args, GtkWidget **);
      default_action = va_arg (args, gboolean);
      connect_delete = va_arg (args, gboolean);

      button = gtk_button_new_with_label (label);
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);

      if (slot_object == (GObject *) 1)
	slot_object = G_OBJECT (dialog);

      if (data == NULL)
	data = dialog;

      if (callback)
	{
	  if (slot_object)
	    g_signal_connect_object (G_OBJECT (button), "clicked",
				     G_CALLBACK (callback),
				     slot_object, G_CONNECT_SWAPPED);
	  else
	    g_signal_connect (G_OBJECT (button), "clicked",
			      G_CALLBACK (callback),
			      data);
	}

      if (widget_ptr)
	*widget_ptr = button;

      if (connect_delete && callback && !delete_connected)
	{
	  gtk_object_set_data (GTK_OBJECT (dialog),
			       "dialog_cancel_callback",
			       (gpointer) callback);
	  gtk_object_set_data (GTK_OBJECT (dialog),
			       "dialog_cancel_widget",
			       slot_object ? slot_object : G_OBJECT (button));

	  /*  catch the WM delete event  */
	  g_signal_connect (G_OBJECT (dialog), "delete_event",
			    G_CALLBACK (dialog_delete_callback),
			    data);

	  delete_connected = TRUE;
	}

      if (default_action)
	gtk_widget_grab_default (button);
      gtk_widget_show (button);

      label = va_arg (args, gchar *);
    }
}

/**
 * dialog_new:
 * @title: The dialog's title which will be set with gtk_window_set_title().
 * @position: The dialog's initial position which will be set with
 *            gtk_window_set_position().
 * @resizable: Is the dialog resizable?, ...
 * @allow_grow: ... it't @allow_grow flag and ...
 * @auto_shrink: ... it's @auto_shrink flag which will all be set with
 *               gtk_window_set_policy().
 * @...: A #NULL terminated @va_list destribing the action_area buttons.
 *
 * This function simply packs the action_area arguments passed in "..."
 * into a @va_list variable and passes everything to dialog_newv().
 *
 * For a description of the format of the @va_list describing the
 * action_area buttons see dialog_create_action_areav().
 *
 * Returns: A #GtkDialog.
 *
 */
GtkWidget *
stpui_dialog_new (const gchar       *title,
		  GtkWindowPosition  position,
		  gboolean           resizable,

		  /* specify action area buttons as va_list:
		   *  const gchar    *label,
		   *  GtkSignalFunc   callback,
		   *  gpointer        data,
		   *  GObject      *slot_object,
		   *  GtkWidget     **widget_ptr,
		   *  gboolean        default_action,
		   *  gboolean        connect_delete,
		   */

		  ...)
{
  GtkWidget *dialog;
  va_list    args;

  va_start (args, resizable);

  g_return_val_if_fail (title != NULL, NULL);

  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), title);
  gtk_window_set_position (GTK_WINDOW (dialog), position);
  gtk_window_set_resizable (GTK_WINDOW (dialog), resizable);

  /*  prepare the action_area  */
  dialog_create_action_areav (GTK_DIALOG (dialog), args);

  va_end (args);

  return dialog;
}

/*****************************************************************\
*                                                                 *
*             The following from libgimp/gimpwidgets.c            *
*                                                                 *
\*****************************************************************/

/**
 * option_menu_new:
 * @menu_only: #TRUE if the function should return a #GtkMenu only.
 * @...:       A #NULL terminated @va_list describing the menu items.
 *
 * Returns: A #GtkOptionMenu or a #GtkMenu (depending on @menu_only).
 **/
GtkWidget *
stpui_option_menu_new(gboolean            menu_only,

		      /* specify menu items as va_list:
		       *  const gchar    *label,
		       *  GtkSignalFunc   callback,
		       *  gpointer        data,
		       *  gpointer        user_data,
		       *  GtkWidget     **widget_ptr,
		       *  gboolean        active
		       */

		       ...)
{
  GtkWidget *menu;
  GtkWidget *menuitem;

  /*  menu item variables  */
  const gchar    *label;
  GtkSignalFunc   callback;
  gpointer        data;
  gpointer        user_data;
  GtkWidget     **widget_ptr;
  gboolean        active;

  va_list args;
  gint    i;
  gint    initial_index;

  menu = gtk_menu_new ();

  /*  create the menu items  */
  initial_index = 0;

  va_start (args, menu_only);
  label = va_arg (args, const gchar *);

  for (i = 0; label; i++)
    {
      callback   = va_arg (args, GtkSignalFunc);
      data       = va_arg (args, gpointer);
      user_data  = va_arg (args, gpointer);
      widget_ptr = va_arg (args, GtkWidget **);
      active     = va_arg (args, gboolean);

      if (strcmp (label, "---"))
	{
	  menuitem = gtk_menu_item_new_with_label (label);

	  g_signal_connect (G_OBJECT (menuitem), "activate",
			    callback,
			    data);

	  if (user_data)
	    gtk_object_set_user_data (GTK_OBJECT (menuitem), user_data);
	}
      else
	{
	  menuitem = gtk_menu_item_new ();

	  gtk_widget_set_sensitive (menuitem, FALSE);
	}

      gtk_menu_append (GTK_MENU (menu), menuitem);

      if (widget_ptr)
	*widget_ptr = menuitem;

      gtk_widget_show (menuitem);

      /*  remember the initial menu item  */
      if (active)
	initial_index = i;

      label = va_arg (args, const gchar *);
    }
  va_end (args);

  if (! menu_only)
    {
      GtkWidget *optionmenu;

      optionmenu = gtk_option_menu_new ();
      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);

      /*  select the initial menu item  */
      gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu), initial_index);

      return optionmenu;
    }

  return menu;
}

/*****************************************************************\
*                                                                 *
*             The following from libgimp/gimpwidgets.c            *
*                                                                 *
\*****************************************************************/

/**
 * spin_button_new:
 * @adjustment:     Returns the spinbutton's #GtkAdjustment.
 * @value:          The initial value of the spinbutton.
 * @lower:          The lower boundary.
 * @upper:          The uppper boundary.
 * @step_increment: The spinbutton's step increment.
 * @page_increment: The spinbutton's page increment (mouse button 2).
 * @page_size:      The spinbutton's page size.
 * @climb_rate:     The spinbutton's climb rate.
 * @digits:         The spinbutton's number of decimal digits.
 *
 * This function is a shortcut for gtk_adjustment_new() and a subsequent
 * gtk_spin_button_new() and does some more initialisation stuff like
 * setting a standard minimun horizontal size.
 *
 * Returns: A #GtkSpinbutton and it's #GtkAdjustment.
 **/
static GtkWidget *
spin_button_new (GtkObject **adjustment,  /* return value */
		      gfloat      value,
		      gfloat      lower,
		      gfloat      upper,
		      gfloat      step_increment,
		      gfloat      page_increment,
		      gfloat      page_size,
		      gfloat      climb_rate,
		      guint       digits)
{
  GtkWidget *spinbutton;

  *adjustment = gtk_adjustment_new (value, lower, upper,
				    step_increment, page_increment, page_size);

  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (*adjustment),
				    climb_rate, digits);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, -1);

  return spinbutton;
}

static void
scale_entry_unconstrained_adjustment_callback (GtkAdjustment *adjustment,
					       GtkAdjustment *other_adj)
{
  g_signal_handlers_block_matched (G_OBJECT (other_adj),
				   G_SIGNAL_MATCH_DATA,
				   0,
				   0,
				   NULL,
				   NULL,
				   adjustment);

  gtk_adjustment_set_value (other_adj, adjustment->value);

  g_signal_handlers_unblock_matched (G_OBJECT (other_adj),
				     G_SIGNAL_MATCH_DATA,
				     0,
				     0,
				     NULL,
				     NULL,
				     adjustment);
}

/**
 * scale_entry_new:
 * @table:               The #GtkTable the widgets will be attached to.
 * @column:              The column to start with.
 * @row:                 The row to attach the widgets.
 * @text:                The text for the #GtkLabel which will appear
 *                       left of the #GtkHScale.
 * @scale_usize:         The minimum horizontal size of the #GtkHScale.
 * @spinbutton_usize:    The minimum horizontal size of the #GtkSpinButton.
 * @value:               The initial value.
 * @lower:               The lower boundary.
 * @upper:               The upper boundary.
 * @step_increment:      The step increment.
 * @page_increment:      The page increment.
 * @digits:              The number of decimal digits.
 * @constrain:           #TRUE if the range of possible values of the
 *                       #GtkSpinButton should be the same as of the #GtkHScale.
 * @unconstrained_lower: The spinbutton's lower boundary
 *                       if @constrain == #FALSE.
 * @unconstrained_upper: The spinbutton's upper boundary
 *                       if @constrain == #FALSE.
 * @tooltip:             A tooltip message for the scale and the spinbutton.
 *
 * This function creates a #GtkLabel, a #GtkHScale and a #GtkSpinButton and
 * attaches them to a 3-column #GtkTable.
 *
 * Returns: The #GtkSpinButton's #GtkAdjustment.
 **/
GtkObject *
stpui_scale_entry_new(GtkTable    *table,
		      gint         column,
		      gint         row,
		      const gchar *text,
		      gint         scale_usize,
		      gint         spinbutton_usize,
		      gfloat       value,
		      gfloat       lower,
		      gfloat       upper,
		      gfloat       step_increment,
		      gfloat       page_increment,
		      guint        digits,
		      gboolean     constrain,
		      gfloat       unconstrained_lower,
		      gfloat       unconstrained_upper,
		      const gchar *tooltip)
{
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *spinbutton;
  GtkObject *adjustment;
  GtkObject *return_adj;

  label = gtk_label_new (text);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label,
                    column + 1, column + 2, row, row + 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  if (! constrain &&
      unconstrained_lower <= lower &&
      unconstrained_upper >= upper)
    {
      GtkObject *constrained_adj;

      constrained_adj = gtk_adjustment_new (value, lower, upper,
					    step_increment, page_increment,
					    0.0);

      spinbutton = spin_button_new (&adjustment, value,
				    unconstrained_lower,
				    unconstrained_upper,
				    step_increment, page_increment, 0.0,
				    1.0, digits);

      g_signal_connect
	(G_OBJECT (constrained_adj), "value_changed",
	 G_CALLBACK (scale_entry_unconstrained_adjustment_callback),
	 adjustment);

      g_signal_connect
	(G_OBJECT (adjustment), "value_changed",
	 G_CALLBACK (scale_entry_unconstrained_adjustment_callback),
	 constrained_adj);

      return_adj = adjustment;

      adjustment = constrained_adj;
    }
  else
    {
      spinbutton = spin_button_new (&adjustment, value, lower, upper,
				    step_increment, page_increment, 0.0,
				    1.0, digits);

      return_adj = adjustment;
    }

  if (spinbutton_usize > 0)
    gtk_widget_set_usize (spinbutton, spinbutton_usize, -1);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  if (scale_usize > 0)
    gtk_widget_set_usize (scale, scale_usize, -1);
  gtk_scale_set_digits (GTK_SCALE (scale), digits);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_table_attach (GTK_TABLE (table), scale,
		    column + 2, column + 3, row, row + 1,
		    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (scale);

  gtk_table_attach (GTK_TABLE (table), spinbutton,
		    column + 3, column + 4, row, row + 1,
		    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (spinbutton);

  if (tooltip)
    {
      stpui_set_help_data (scale, tooltip);
      stpui_set_help_data (spinbutton, tooltip);
    }

  gtk_object_set_data (GTK_OBJECT (return_adj), "label", label);
  gtk_object_set_data (GTK_OBJECT (return_adj), "scale", scale);
  gtk_object_set_data (GTK_OBJECT (return_adj), "spinbutton", spinbutton);

  return return_adj;
}

/**
 * table_attach_aligned:
 * @table:      The #GtkTable the widgets will be attached to.
 * @column:     The column to start with.
 * @row:        The row to attach the eidgets.
 * @label_text: The text for the #GtkLabel which will be attached left of the
 *              widget.
 * @xalign:     The horizontal alignment of the #GtkLabel.
 * @yalign:     The vertival alignment of the #GtkLabel.
 * @widget:     The #GtkWidget to attach right of the label.
 * @colspan:    The number of columns the widget will use.
 * @left_align: #TRUE if the widget should be left-aligned.
 *
 * Note that the @label_text can be #NULL and that the widget will be attached
 * starting at (@column + 1) in this case, too.
 **/
void
stpui_table_attach_aligned (GtkTable    *table,
			    gint         column,
			    gint         row,
			    const gchar *label_text,
			    gfloat       xalign,
			    gfloat       yalign,
			    GtkWidget   *widget,
			    gint         colspan,
			    gboolean     left_align)
{
  if (label_text)
    {
      GtkWidget *label;

      label = gtk_label_new (label_text);
      gtk_misc_set_alignment (GTK_MISC (label), xalign, yalign);
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
      gtk_table_attach (table, label, column, column + 1, row, row + 1,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);
    }

  gtk_widget_show (widget);
  if (left_align)
    {
      GtkWidget *alignment;

      alignment = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
      gtk_container_add (GTK_CONTAINER (alignment), widget);

      widget = alignment;
    }

  gtk_table_attach (table, widget, column + 1, column + 1 + colspan,
		    row, row + 1,
		    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  gtk_widget_show (widget);
}

/*****************************************************************\
*                                                                 *
*        The following borrowed from libgimp/gimphelpui.c         *
*                                                                 *
\*****************************************************************/

static GtkTooltips * tool_tips  = NULL;

/**
 * stpui_help_init:
 *
 * This function initializes GIMP's help system.
 *
 * Currently it only creates a #GtkTooltips object with gtk_tooltips_new()
 * which will be used by help_stpui_set_help_data().
 **/
void
stpui_help_init (void)
{
  tool_tips = gtk_tooltips_new ();
}

/**
 * stpui_help_free:
 *
 * This function frees the memory used by the #GtkTooltips created by
 * stpui_help_init().
 **/
void
stpui_help_free (void)
{
  gtk_object_destroy (GTK_OBJECT (tool_tips));
  gtk_object_unref   (GTK_OBJECT (tool_tips));
}

/**
 * help_enable_tooltips:
 *
 * This function calls gtk_tooltips_enable().
 **/
void
stpui_enable_help (void)
{
  gtk_tooltips_enable (tool_tips);
}

/**
 * help_disable_tooltips:
 *
 * This function calls gtk_tooltips_disable().
 **/

void
stpui_disable_help (void)
{
  gtk_tooltips_disable (tool_tips);
}

void
stpui_set_help_data (GtkWidget *widget, const gchar *tooltip)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (tooltip)
    {
      gtk_tooltips_set_tip (tool_tips, widget, tooltip, NULL);
    }
}

/*****************************************************************\
*                                                                 *
*                     The below is new code                       *
*                                                                 *
\*****************************************************************/


void
stpui_set_image_filename(const char *name)
{
  if (name && name == image_filename)
    return;
  if (image_filename)
    g_free(image_filename);
  if (name)
    image_filename = g_strdup(name);
  else
    image_filename = g_strdup("");
}

const char *
stpui_get_image_filename(void)
{
  stpui_set_image_filename(image_filename);
  return(image_filename);
}

static void
default_errfunc(void *data, const char *buffer, size_t bytes)
{
  fwrite(buffer, 1, bytes, data ? data : stderr);
}

void
stpui_set_errfunc(stp_outfunc_t wfunc)
{
  the_errfunc = wfunc;
}

stp_outfunc_t
stpui_get_errfunc(void)
{
  return the_errfunc;
}

void
stpui_set_errdata(void *errdata)
{
  the_errdata = errdata;
}

void *
stpui_get_errdata(void)
{
  return the_errdata;
}

void
stpui_set_thumbnail_func(get_thumbnail_func_t func)
{
  thumbnail_func = func;
}

get_thumbnail_func_t
stpui_get_thumbnail_func(void)
{
  return thumbnail_func;
}

void
stpui_set_thumbnail_data(void *data)
{
  thumbnail_private_data = data;
}

void *
stpui_get_thumbnail_data(void)
{
  return thumbnail_private_data;
}

GtkWidget *
stpui_create_entry(GtkWidget *table, int hpos, int vpos, const char *text,
		   const char *help, GCallback callback)
{
  GtkWidget *entry = gtk_entry_new();
  gtk_widget_set_usize(entry, 60, 0);
  stpui_table_attach_aligned(GTK_TABLE(table), hpos, vpos, text,
			     0.0, 0.5, entry, 1, TRUE);
  stpui_set_help_data(entry, help);
  g_signal_connect(G_OBJECT(entry), "activate",
		   G_CALLBACK(callback), NULL);
  return entry;
}

GSList *
stpui_create_radio_button(radio_group_t *radio, GSList *group,
			  GtkWidget *table, int hpos, int vpos,
			  GCallback callback)
{
  radio->button = gtk_radio_button_new_with_label(group, gettext(radio->name));
  group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio->button));
  stpui_table_attach_aligned(GTK_TABLE(table), hpos, vpos, NULL, 0.5, 0.5,
			     radio->button, 1, FALSE);
  stpui_set_help_data(radio->button, gettext(radio->help));
  g_signal_connect(G_OBJECT(radio->button), "toggled",
		   G_CALLBACK(callback), (gpointer) radio->value);
  return group;
}

void
stpui_set_adjustment_tooltip (GtkObject *adj, const gchar *tip)
{
  stpui_set_help_data (GTK_WIDGET (SCALE_ENTRY_SCALE (adj)), tip);
  stpui_set_help_data (GTK_WIDGET (SCALE_ENTRY_SPINBUTTON (adj)), tip);
}

static GtkWidget *
table_label(GtkTable *table, gint column, gint row)
{
  GList *children = table->children;;
  while (children)
    {
      GtkTableChild *child = (GtkTableChild *)children->data;
      if (child->left_attach == column + 1 && child->top_attach == row)
	return child->widget;
      children = children->next;
    }
  return NULL;
}

void
stpui_create_new_combo(option_t *option, GtkWidget *table,
		       int hpos, int vpos, gboolean is_optional)
{
  GtkWidget *event_box = gtk_event_box_new();
  GtkWidget *combo = gtk_combo_new();

  option->checkbox = gtk_check_button_new();
  gtk_table_attach(GTK_TABLE(table), option->checkbox,
		   hpos, hpos + 1, vpos, vpos + 1,
		   GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);

  option->info.list.combo = combo;
  gtk_container_add(GTK_CONTAINER(event_box), combo);
  gtk_widget_show(combo);
  gtk_widget_show(event_box);
  stpui_set_help_data(event_box, gettext(option->fast_desc->help));
  stpui_table_attach_aligned
    (GTK_TABLE(table), hpos + 1, vpos, gettext(option->fast_desc->text),
     0.0, 0.5, event_box, 2, TRUE);
  option->info.list.label = table_label(GTK_TABLE(table), hpos, vpos);
}

const char *
stpui_combo_get_name(GtkWidget   *combo,
		     const stp_string_list_t *options)
{
  if (options)
    {
      gint   i;
      gint num_options = stp_string_list_count(options);
      const gchar *text = (gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)));

      if (text == NULL)
	return (NULL);

      if (num_options == 0)
	return ((const char *)text);

      for (i = 0; i < num_options; i ++)
	if (strcmp(stp_string_list_param(options, i)->text, text) == 0)
	  return (stp_string_list_param(options, i)->name);
    }
  return (NULL);
}

void
stpui_create_scale_entry(option_t    *opt,
			 GtkTable    *table,
			 gint         column,
			 gint         row,
			 const gchar *text,
			 gint         scale_usize,
			 gint         spinbutton_usize,
			 gfloat       value,
			 gfloat       lower,
			 gfloat       upper,
			 gfloat       step_increment,
			 gfloat       page_increment,
			 guint        digits,
			 gboolean     constrain,
			 gfloat       unconstrained_lower,
			 gfloat       unconstrained_upper,
			 const gchar *tooltip,
			 gboolean     is_optional)
{
  opt->checkbox = gtk_check_button_new();
  gtk_table_attach(GTK_TABLE(table), opt->checkbox,
		   column, column + 1, row, row + 1,
		   GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
  opt->info.flt.adjustment =
    stpui_scale_entry_new(table, column, row, text, scale_usize,
			  spinbutton_usize, value, lower, upper,
			  step_increment, page_increment, digits, constrain,
			  unconstrained_lower, unconstrained_upper, tooltip);
}
