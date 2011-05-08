/*
 * "$Id: escp2-driver.c,v 1.57 2010/12/19 02:51:37 rlk Exp $"
 *
 *   Print plug-in EPSON ESC/P2 driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
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

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include <gutenprint/gutenprint-intl-internal.h>
#include "gutenprint-internal.h"
#include <string.h>
#include "print-escp2.h"

#ifdef __GNUC__
#define inline __inline__
#endif

static escp2_privdata_t *
get_privdata(stp_vars_t *v)
{
  return (escp2_privdata_t *) stp_get_component_data(v, "Driver");
}

static void
escp2_reset_printer(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  /*
   * Magic initialization string that's needed to take printer out of
   * packet mode.
   */
  if (pd->preinit_sequence)
    stp_write_raw(pd->preinit_sequence, v);

  stp_send_command(v, "\033@", "");
}

static void
print_remote_param(stp_vars_t *v, const char *param, const char *value)
{
  stp_send_command(v, "\033(R", "bcscs", '\0', param, ':',
		    value ? value : "NULL");
  stp_send_command(v, "\033", "ccc", 0, 0, 0);
}

static void
print_remote_int_param(stp_vars_t *v, const char *param, int value)
{
  char buf[64];
  (void) snprintf(buf, 64, "%d", value);
  print_remote_param(v, param, buf);
}

static void
print_remote_float_param(stp_vars_t *v, const char *param, double value)
{
  char buf[64];
  (void) snprintf(buf, 64, "%f", value);
  print_remote_param(v, param, buf);
}

static void
print_debug_params(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  stp_parameter_list_t params = stp_get_parameter_list(v);
  int count = stp_parameter_list_count(params);
  int i;
  print_remote_param(v, "Package", PACKAGE);
  print_remote_param(v, "Version", VERSION);
  print_remote_param(v, "Release Date", RELEASE_DATE);
  print_remote_param(v, "Driver", stp_get_driver(v));
  print_remote_int_param(v, "Left", stp_get_left(v));
  print_remote_int_param(v, "Top", stp_get_top(v));
  print_remote_int_param(v, "Page Width", stp_get_page_width(v));
  print_remote_int_param(v, "Page Height", stp_get_page_height(v));
  print_remote_int_param(v, "Model", stp_get_model_id(v));
  print_remote_int_param(v, "Ydpi", pd->res->vres);
  print_remote_int_param(v, "Xdpi", pd->res->hres);
  print_remote_int_param(v, "Printed_ydpi", pd->res->printed_vres);
  print_remote_int_param(v, "Printed_xdpi", pd->res->printed_hres);
/*
  print_remote_int_param(v, "Use_softweave", pd->res->softweave);
  print_remote_int_param(v, "Printer_weave", pd->res->printer_weave);
*/
  print_remote_int_param(v, "Use_printer_weave", pd->use_printer_weave);
  print_remote_int_param(v, "Duplex", pd->duplex);
  print_remote_int_param(v, "Page_left", pd->page_left);
  print_remote_int_param(v, "Page_right", pd->page_right);
  print_remote_int_param(v, "Page_top", pd->page_top);
  print_remote_int_param(v, "Page_bottom", pd->page_bottom);
  print_remote_int_param(v, "Page_width", pd->page_width);
  print_remote_int_param(v, "Page_height", pd->page_height);
  print_remote_int_param(v, "Page_true_height", pd->page_true_height);
  print_remote_int_param(v, "Page_extra_height", pd->page_extra_height);
  print_remote_int_param(v, "Paper_extra_bottom", pd->paper_extra_bottom);
  print_remote_int_param(v, "Image_left", pd->image_left);
  print_remote_int_param(v, "Image_top", pd->image_top);
  print_remote_int_param(v, "Image_width", pd->image_width);
  print_remote_int_param(v, "Image_height", pd->image_height);
  print_remote_int_param(v, "Image_scaled_width", pd->image_scaled_width);
  print_remote_int_param(v, "Image_scaled_height", pd->image_scaled_height);
  print_remote_int_param(v, "Image_printed_width", pd->image_printed_width);
  print_remote_int_param(v, "Image_printed_height", pd->image_printed_height);
  print_remote_int_param(v, "Image_left_position", pd->image_left_position);
  print_remote_int_param(v, "Nozzles", pd->nozzles);
  print_remote_int_param(v, "Nozzle_separation", pd->nozzle_separation);
  print_remote_int_param(v, "Horizontal_passes", pd->horizontal_passes);
  print_remote_int_param(v, "Vertical_passes", pd->res->vertical_passes);
  print_remote_int_param(v, "Physical_xdpi", pd->physical_xdpi);
  print_remote_int_param(v, "Page_management_units", pd->page_management_units);
  print_remote_int_param(v, "Vertical_units", pd->vertical_units);
  print_remote_int_param(v, "Horizontal_units", pd->horizontal_units);
  print_remote_int_param(v, "Micro_units", pd->micro_units);
  print_remote_int_param(v, "Unit_scale", pd->unit_scale);
  print_remote_int_param(v, "Zero_advance", pd->send_zero_pass_advance);
  print_remote_int_param(v, "Bits", pd->bitwidth);
  print_remote_int_param(v, "Drop Size", pd->drop_size);
  print_remote_int_param(v, "Initial_vertical_offset", pd->initial_vertical_offset);
  print_remote_int_param(v, "Channels_in_use", pd->channels_in_use);
  print_remote_int_param(v, "Logical_channels", pd->logical_channels);
  print_remote_int_param(v, "Physical_channels", pd->physical_channels);
  print_remote_int_param(v, "Use_black_parameters", pd->use_black_parameters);
  print_remote_int_param(v, "Use_fast_360", pd->use_fast_360);
  print_remote_int_param(v, "Command_set", pd->command_set);
  print_remote_int_param(v, "Variable_dots", pd->variable_dots);
  print_remote_int_param(v, "Has_graymode", pd->has_graymode);
  print_remote_int_param(v, "Base_separation", pd->base_separation);
  print_remote_int_param(v, "Resolution_scale", pd->resolution_scale);
#if 0
  print_remote_int_param(v, "Printing_resolution", pd->printing_resolution);
#endif
  print_remote_int_param(v, "Separation_rows", pd->separation_rows);
  print_remote_int_param(v, "Pseudo_separation_rows", pd->pseudo_separation_rows);
  print_remote_int_param(v, "Extra_720dpi_separation", pd->extra_720dpi_separation);
  print_remote_int_param(v, "Use_aux_channels", pd->use_aux_channels);
  print_remote_param(v, "Ink name", pd->inkname->name);
  print_remote_int_param(v, "  channels", pd->inkname->channel_count);
  print_remote_int_param(v, "  inkset", pd->inkname->inkset);
  for (i = 0; i < count; i++)
    {
      const stp_parameter_t *p = stp_parameter_list_param(params, i);
      switch (p->p_type)
	{
	case STP_PARAMETER_TYPE_DOUBLE:
	  if (stp_check_float_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
	    print_remote_float_param(v, p->name,
				     stp_get_float_parameter(v, p->name));
	  break;
	case STP_PARAMETER_TYPE_INT:
	  if (stp_check_int_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
	    print_remote_int_param(v, p->name,
				   stp_get_int_parameter(v, p->name));
	  break;
	case STP_PARAMETER_TYPE_DIMENSION:
	  if (stp_check_dimension_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
	    print_remote_int_param(v, p->name,
				   stp_get_dimension_parameter(v, p->name));
	  break;
	case STP_PARAMETER_TYPE_BOOLEAN:
	  if (stp_check_boolean_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
	    print_remote_int_param(v, p->name,
				   stp_get_boolean_parameter(v, p->name));
	  break;
	case STP_PARAMETER_TYPE_STRING_LIST:
	  if (stp_check_string_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
	    print_remote_param(v, p->name,
			       stp_get_string_parameter(v, p->name));
	  break;
	case STP_PARAMETER_TYPE_CURVE:
	  if (stp_check_curve_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
	    {
	      char *curve =
		stp_curve_write_string(stp_get_curve_parameter(v, p->name));
	      print_remote_param(v, p->name, curve);
	      stp_free(curve);
	    }
	  break;
	default:
	  break;
	}
    }
  stp_parameter_list_destroy(params);
  stp_send_command(v, "\033", "ccc", 0, 0, 0);
}

static void
escp2_set_remote_sequence(stp_vars_t *v)
{
  /* Magic remote mode commands, whatever they do */
  escp2_privdata_t *pd = get_privdata(v);
  const stp_vars_t *pv = pd->media_settings;

  if (stp_get_debug_level() & STP_DBG_MARK_FILE)
    print_debug_params(v);
  if (pd->advanced_command_set || pd->input_slot)
    {
      /* Enter remote mode */
      stp_send_command(v, "\033(R", "bcs", 0, "REMOTE1");
      /* Per the manual, job setup comes first, then SN command */
      if (pd->input_slot &&
	  pd->input_slot->roll_feed_cut_flags == ROLL_FEED_CUT_ALL)
	      stp_send_command(v, "JS", "bh", 0);
      if (pd->preinit_remote_sequence)
	stp_write_raw(pd->preinit_remote_sequence, v);
      if (stp_check_int_parameter(pv, "FeedAdjustment", STP_PARAMETER_ACTIVE))
	stp_send_command(v, "SN", "bccc", 0, 4,
			 stp_get_int_parameter(pv, "FeedAdjustment"));
      if (stp_check_int_parameter(pv, "VacuumIntensity", STP_PARAMETER_ACTIVE))
	stp_send_command(v, "SN", "bccc", 0, 5,
			 stp_get_int_parameter(pv, "VacuumIntensity"));
      if (stp_check_float_parameter(pv, "ScanDryTime", STP_PARAMETER_ACTIVE))
	stp_send_command(v, "DR", "bcch", 0, 0,
			 (int) (stp_get_float_parameter(pv, "ScanDryTime") * 1000));
      if (stp_check_float_parameter(pv, "ScanMinDryTime", STP_PARAMETER_ACTIVE))
	stp_send_command(v, "DR", "bcch", 0, 0x40,
			 (int) (stp_get_float_parameter(pv, "ScanMinDryTime") * 1000));
      if (stp_check_float_parameter(pv, "PageDryTime", STP_PARAMETER_ACTIVE))
	stp_send_command(v, "DR", "bcch", 0, 1,
			 (int) stp_get_float_parameter(pv, "PageDryTime"));
      /* Next comes paper path */
      if (pd->input_slot)
	{
	  int divisor = pd->base_separation / 360;
	  int height = pd->page_true_height * 5 / divisor;
	  if (pd->input_slot->init_sequence)
	    stp_write_raw(pd->input_slot->init_sequence, v);
	  switch (pd->input_slot->roll_feed_cut_flags)
	    {
	    case ROLL_FEED_CUT_ALL:
	      stp_send_command(v, "CO", "bccccl", 0, 0, 1, 0, 0);
	      stp_send_command(v, "CO", "bccccl", 0, 0, 0, 0, height);
	      break;
	    case ROLL_FEED_CUT_LAST:
	      stp_send_command(v, "CO", "bccccl", 0, 0, 1, 0, 0);
	      stp_send_command(v, "CO", "bccccl", 0, 0, 2, 0, height);
	      break;
	    default:
	      break;
	    }
	}
      if (stp_check_int_parameter(pv, "PaperMedia", STP_PARAMETER_ACTIVE))
	stp_send_command(v, "MI", "bcccc", 0, 1,
			 stp_get_int_parameter(pv, "PaperMedia"),
			 (stp_check_int_parameter(pv, "PaperMediaSize", STP_PARAMETER_ACTIVE) ?
			  stp_get_int_parameter(pv, "PaperMediaSize") :
			  99));	/* User-defined size (for now!) */
      if (pd->duplex)
	{
	  /* If there's ever duplex no tumble, we'll need to special
	     case it, too */
	  if (pd->duplex == DUPLEX_TUMBLE && (pd->input_slot->duplex & DUPLEX_TUMBLE))
	    stp_send_command(v, "DP", "bcc", 0, 2); /* Auto duplex */
	  else
	    stp_send_command(v, "DP", "bcc", 0, 2); /* Auto duplex */
	}
      if (stp_check_int_parameter(pv, "PaperThickness", STP_PARAMETER_ACTIVE))
	stp_send_command(v, "PH", "bcc", 0,
			 stp_get_int_parameter(pv, "PaperThickness"));
      if (stp_check_int_parameter(pv, "FeedSequence", STP_PARAMETER_ACTIVE))
	stp_send_command(v, "SN", "bccc", 0, 0,
			 stp_get_int_parameter(pv, "FeedSequence"));
      if (stp_check_int_parameter(pv, "PlatenGap", STP_PARAMETER_ACTIVE))
	stp_send_command(v, "US", "bccc", 0, 1,
			 stp_get_int_parameter(pv, "PlatenGap"));
      if (stp_get_boolean_parameter(v, "FullBleed"))
	{
	  stp_send_command(v, "FP", "bch", 0,
			   (unsigned short) -pd->zero_margin_offset);
	  if (pd->borderless_sequence)
	    stp_write_raw(pd->borderless_sequence, v);
	}
      if (pd->inkname->init_sequence)
	stp_write_raw(pd->inkname->init_sequence, v);
      /* Exit remote mode */

      stp_send_command(v, "\033", "ccc", 0, 0, 0);
    }
}

static void
escp2_set_graphics_mode(stp_vars_t *v)
{
  stp_send_command(v, "\033(G", "bc", 1);
}

static void
escp2_set_resolution(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  if (pd->use_extended_commands)
    stp_send_command(v, "\033(U", "bccch",
		     pd->unit_scale / pd->page_management_units,
		     pd->unit_scale / pd->vertical_units,
		     pd->unit_scale / pd->horizontal_units,
		     pd->unit_scale);
  else
    stp_send_command(v, "\033(U", "bc",
		     pd->unit_scale / pd->page_management_units);
}

static void
escp2_set_color(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  if (pd->use_fast_360)
    stp_send_command(v, "\033(K", "bcc", 0, 3);
  else if (pd->has_graymode)
    stp_send_command(v, "\033(K", "bcc", 0,
		     (pd->use_black_parameters ? 1 : 2));
}

static void
escp2_set_printer_weave(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  if (pd->printer_weave)
    stp_write_raw(pd->printer_weave, v);
  else
    stp_send_command(v, "\033(i", "bc", 0);
}

static void
escp2_set_printhead_speed(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  const char *direction = stp_get_string_parameter(v, "PrintingDirection");
  int unidirectional = -1;
  if (direction && strcmp(direction, "Unidirectional") == 0)
    unidirectional = 1;
  else if (direction && strcmp(direction, "Bidirectional") == 0)
    unidirectional = 0;
  else if (pd->bidirectional_upper_limit >= 0 &&
	   pd->res->printed_hres * pd->res->printed_vres *
	   pd->res->vertical_passes >= pd->bidirectional_upper_limit)
    {
      stp_dprintf(STP_DBG_ESCP2, v,
		  "Setting unidirectional: hres %d vres %d passes %d total %d limit %d\n",
		  pd->res->printed_hres, pd->res->printed_vres,
		  pd->res->vertical_passes,
		  (pd->res->printed_hres * pd->res->printed_vres *
		   pd->res->vertical_passes),
		  pd->bidirectional_upper_limit);
      unidirectional = 1;
    }
  else if (pd->bidirectional_upper_limit >= 0)
    {
      stp_dprintf(STP_DBG_ESCP2, v,
		  "Setting bidirectional: hres %d vres %d passes %d total %d limit %d\n",
		  pd->res->printed_hres, pd->res->printed_vres,
		  pd->res->vertical_passes,
		  (pd->res->printed_hres * pd->res->printed_vres *
		   pd->res->vertical_passes),
		  pd->bidirectional_upper_limit);
      unidirectional = 0;
    }
  if (unidirectional == 1)
    {
      stp_send_command(v, "\033U", "c", 1);
      if (pd->res->hres > pd->physical_xdpi)
	stp_send_command(v, "\033(s", "bc", 2);
    }
  else if (unidirectional == 0)
    stp_send_command(v, "\033U", "c", 0);
}

static void
escp2_set_dot_size(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  /* Dot size */
  if (pd->drop_size >= 0)
    stp_send_command(v, "\033(e", "bcc", 0, pd->drop_size);
}

static void
escp2_set_page_height(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  int l = (pd->page_true_height + pd->paper_extra_bottom) *
    pd->page_management_units / 72;
  if (pd->use_extended_commands)
    stp_send_command(v, "\033(C", "bl", l);
  else
    stp_send_command(v, "\033(C", "bh", l);
}

static void
escp2_set_margins(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  int bot = pd->page_management_units * pd->page_bottom / 72;
  int top = pd->page_management_units * pd->page_top / 72;

  top += pd->initial_vertical_offset;
  top -= pd->page_extra_height;
  bot += pd->page_extra_height;
  if (pd->use_extended_commands &&
      (pd->command_set == MODEL_COMMAND_2000 ||
       pd->command_set == MODEL_COMMAND_PRO))
    stp_send_command(v, "\033(c", "bll", top, bot);
  else
    stp_send_command(v, "\033(c", "bhh", top, bot);
}

static void
escp2_set_paper_dimensions(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  if (pd->advanced_command_set)
    {
      const stp_vars_t *pv = pd->media_settings;
      int w = pd->page_true_width * pd->page_management_units / 72;
      int h = (pd->page_true_height + pd->paper_extra_bottom) *
	pd->page_management_units / 72;
      stp_send_command(v, "\033(S", "bll", w, h);
      if (stp_check_int_parameter(pv, "PrintMethod", STP_PARAMETER_ACTIVE))
	stp_send_command(v, "\033(m", "bc", 
			 stp_get_int_parameter(pv, "PrintMethod"));
    }
}

static void
escp2_set_printhead_resolution(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  if (pd->use_extended_commands)
    {
      int xres;
      int yres = pd->resolution_scale;

      xres = pd->resolution_scale / pd->physical_xdpi;

      if (pd->command_set == MODEL_COMMAND_PRO && pd->printer_weave)
	yres = yres /  pd->res->vres;
      else if (pd->split_channel_count > 1)
	yres = yres * pd->nozzle_separation / pd->base_separation *
	  pd->split_channel_count;
      else
	yres = yres * pd->nozzle_separation / pd->base_separation;

      /* Magic resolution cookie */
      stp_send_command(v, "\033(D", "bhcc", pd->resolution_scale, yres, xres);
    }
}

static void
set_vertical_position(stp_vars_t *v, stp_pass_t *pass)
{
  escp2_privdata_t *pd = get_privdata(v);
  int advance = pass->logicalpassstart - pd->last_pass_offset -
    (pd->separation_rows - 1);
  advance = advance * pd->vertical_units / pd->res->printed_vres;
  if (pass->logicalpassstart > pd->last_pass_offset ||
      (pd->send_zero_pass_advance && pass->pass > pd->last_pass) ||
      pd->printing_initial_vertical_offset != 0)
    {
      advance += pd->printing_initial_vertical_offset;
      pd->printing_initial_vertical_offset = 0;
      if (pd->use_extended_commands)
	stp_send_command(v, "\033(v", "bl", advance);
      else
	stp_send_command(v, "\033(v", "bh", advance);
      pd->last_pass_offset = pass->logicalpassstart;
      pd->last_pass = pass->pass;
    }
}

static void
set_color(stp_vars_t *v, stp_pass_t *pass, int color)
{
  escp2_privdata_t *pd = get_privdata(v);
  if (pd->last_color != color && ! pd->use_extended_commands)
    {
      int ncolor = pd->channels[color]->color;
      int subchannel = pd->channels[color]->subchannel;
      if (subchannel >= 0)
	stp_send_command(v, "\033(r", "bcc", subchannel, ncolor);
      else
	stp_send_command(v, "\033r", "c", ncolor);
      pd->last_color = color;
    }
}

static void
set_horizontal_position(stp_vars_t *v, stp_pass_t *pass, int vertical_subpass)
{
  escp2_privdata_t *pd = get_privdata(v);
  int microoffset = (vertical_subpass & (pd->horizontal_passes - 1)) *
    pd->image_scaled_width / pd->image_printed_width;
  int pos = pd->image_left_position + microoffset;

  if (pos != 0)
    {
      if (pd->command_set == MODEL_COMMAND_PRO || pd->variable_dots)
	stp_send_command(v, "\033($", "bl", pos);
      else if (pd->advanced_command_set || pd->res->hres > 720)
	stp_send_command(v, "\033(\\", "bhh", pd->micro_units, pos);
      else
	stp_send_command(v, "\033\\", "h", pos);
    }
}

static void
send_print_command(stp_vars_t *v, stp_pass_t *pass, int ncolor, int nlines)
{
  escp2_privdata_t *pd = get_privdata(v);
  int lwidth = (pd->image_printed_width + (pd->horizontal_passes - 1)) /
    pd->horizontal_passes;
  if (pd->command_set == MODEL_COMMAND_PRO || pd->variable_dots)
    {
      int nwidth = pd->bitwidth * ((lwidth + 7) / 8);
      stp_send_command(v, "\033i", "ccchh", ncolor,
		       (stp_get_debug_level() & STP_DBG_NO_COMPRESSION) ? 0 : 1,
		       pd->bitwidth, nwidth, nlines);
    }
  else
    {
      int ygap = 3600 / pd->vertical_units;
      int xgap = 3600 / pd->physical_xdpi;
      if (pd->nozzles == 1)
	{
	  if (pd->vertical_units == 720 && pd->extra_720dpi_separation)
	    ygap *= pd->extra_720dpi_separation;
	}
      else if (pd->extra_720dpi_separation)
	ygap *= pd->extra_720dpi_separation;
      else if (pd->pseudo_separation_rows > 0)
	ygap *= pd->pseudo_separation_rows;
      else
	ygap *= pd->separation_rows;
      stp_send_command(v, "\033.", "cccch",
		       (stp_get_debug_level() & STP_DBG_NO_COMPRESSION) ? 0 : 1,
		       ygap, xgap, nlines, lwidth);
    }
}

static void
send_extra_data(stp_vars_t *v, int extralines)
{
  escp2_privdata_t *pd = get_privdata(v);
  int lwidth = (pd->image_printed_width + (pd->horizontal_passes - 1)) /
    pd->horizontal_passes;
  if (stp_get_debug_level() & STP_DBG_NO_COMPRESSION)
    {
      int i, k;
      for (k = 0; k < extralines; k++)
	for (i = 0; i < pd->bitwidth * (lwidth + 7) / 8; i++)
	  stp_putc(0, v);
    }
  else
    {
      int k, l;
      int bytes_to_fill = pd->bitwidth * ((lwidth + 7) / 8);
      int full_blocks = bytes_to_fill / 128;
      int leftover = bytes_to_fill % 128;
      int total_bytes = extralines * (full_blocks + 1) * 2;
      unsigned char *buf = stp_malloc(total_bytes);
      total_bytes = 0;
      for (k = 0; k < extralines; k++)
	{
	  for (l = 0; l < full_blocks; l++)
	    {
	      buf[total_bytes++] = 129;
	      buf[total_bytes++] = 0;
	    }
	  if (leftover == 1)
	    {
	      buf[total_bytes++] = 1;
	      buf[total_bytes++] = 0;
	    }
	  else if (leftover > 0)
	    {
	      buf[total_bytes++] = 257 - leftover;
	      buf[total_bytes++] = 0;
	    }
	}
      stp_zfwrite((const char *) buf, total_bytes, 1, v);
      stp_free(buf);
    }
}

void
stpi_escp2_init_printer(stp_vars_t *v)
{
  escp2_reset_printer(v);
  escp2_set_remote_sequence(v);
  escp2_set_graphics_mode(v);
  escp2_set_resolution(v);
  escp2_set_color(v);
  escp2_set_printer_weave(v);
  escp2_set_printhead_speed(v);
  escp2_set_dot_size(v);
  escp2_set_printhead_resolution(v);
  escp2_set_page_height(v);
  escp2_set_margins(v);
  escp2_set_paper_dimensions(v);
}

void
stpi_escp2_deinit_printer(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  stp_puts("\033@", v);	/* ESC/P2 reset */
  if (pd->advanced_command_set || pd->input_slot)
    {
      stp_send_command(v, "\033(R", "bcs", 0, "REMOTE1");
      if (pd->inkname->deinit_sequence)
	stp_write_raw(pd->inkname->deinit_sequence, v);
      if (pd->input_slot && pd->input_slot->deinit_sequence)
	stp_write_raw(pd->input_slot->deinit_sequence, v);
      /* Load settings from NVRAM */
      stp_send_command(v, "LD", "b");

      /* Magic deinit sequence reported by Simone Falsini */
      if (pd->deinit_remote_sequence)
	stp_write_raw(pd->deinit_remote_sequence, v);
      /* Exit remote mode */
      stp_send_command(v, "\033", "ccc", 0, 0, 0);
    }
}

void
stpi_escp2_flush_pass(stp_vars_t *v, int passno, int vertical_subpass)
{
  int j;
  escp2_privdata_t *pd = get_privdata(v);
  stp_lineoff_t *lineoffs = stp_get_lineoffsets_by_pass(v, passno);
  stp_lineactive_t *lineactive = stp_get_lineactive_by_pass(v, passno);
  const stp_linebufs_t *bufs = stp_get_linebases_by_pass(v, passno);
  stp_pass_t *pass = stp_get_pass_by_pass(v, passno);
  stp_linecount_t *linecount = stp_get_linecount_by_pass(v, passno);
  int minlines = pd->min_nozzles;
  int nozzle_start = pd->nozzle_start;

  for (j = 0; j < pd->channels_in_use; j++)
    {
      if (lineactive->v[j] > 0)
	{
	  int ncolor = pd->channels[j]->color;
	  int subchannel = pd->channels[j]->subchannel;
	  int nlines = linecount->v[j];
	  int extralines = 0;
	  set_vertical_position(v, pass);
	  set_color(v, pass, j);
	  if (subchannel >= 0)
	    ncolor |= (subchannel << 4);

	  if (pd->split_channels)
	    {
	      int sc = pd->split_channel_count;
	      int k, l;
	      int minlines_lo, nozzle_start_lo;
	      minlines /= sc;
	      nozzle_start /= sc;
	      minlines_lo = pd->min_nozzles - (minlines * sc);
	      nozzle_start_lo = pd->nozzle_start - (nozzle_start * sc);
	      for (k = 0; k < sc; k++)
		{
		  int ml = minlines + (k < minlines_lo ? 1 : 0);
		  int ns = nozzle_start + (k < nozzle_start_lo ? 1 : 0);
		  int lc = ((nlines + (sc - k - 1)) / sc);
		  int base = (pd->nozzle_start + k) % sc;
		  if (lc < ml)
		    extralines = ml - lc;
		  else
		    extralines = 0;
		  extralines -= ns;
		  if (extralines < 0)
		    extralines = 0;
		  if (lc + extralines > 0)
		    {
		      int sc_off = k + j * sc;
		      set_horizontal_position(v, pass, vertical_subpass);
		      send_print_command(v, pass, pd->split_channels[sc_off],
					 lc + extralines + ns);
		      if (ns > 0)
			send_extra_data(v, ns);
		      for (l = 0; l < lc; l++)
			{
			  int sp = (l * sc) + base;
			  unsigned long offset = sp * pd->split_channel_width;
			  if (!(stp_get_debug_level() & STP_DBG_NO_COMPRESSION))
			    {
			      unsigned char *comp_ptr;
			      stp_pack_tiff(v, bufs->v[j] + offset,
					    pd->split_channel_width,
					    pd->comp_buf, &comp_ptr, NULL, NULL);
			      stp_zfwrite((const char *) pd->comp_buf,
					  comp_ptr - pd->comp_buf, 1, v);
			    }
			  else
			    stp_zfwrite((const char *) bufs->v[j] + offset,
					pd->split_channel_width, 1, v);
			}
		      if (extralines > 0)
			send_extra_data(v, extralines);
		      stp_send_command(v, "\r", "");
		    }
		}
	    }
	  else
	    {
	      set_horizontal_position(v, pass, vertical_subpass);
	      if (nlines < minlines)
		{
		  extralines = minlines - nlines;
		  nlines = minlines;
		}
	      send_print_command(v, pass, ncolor, nlines);
	      extralines -= nozzle_start;
	      /*
	       * Send the data
	       */
	      if (nozzle_start)
		send_extra_data(v, nozzle_start);
	      stp_zfwrite((const char *)bufs->v[j], lineoffs->v[j], 1, v);
	      if (extralines > 0)
		send_extra_data(v, extralines);
	      stp_send_command(v, "\r", "");
	    }
	  pd->printed_something = 1;
	}
      lineoffs->v[j] = 0;
      linecount->v[j] = 0;
    }
}

void
stpi_escp2_terminate_page(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  if (!pd->input_slot ||
      !(pd->input_slot->roll_feed_cut_flags & ROLL_FEED_DONT_EJECT))
    {
      if (!pd->printed_something)
	stp_send_command(v, "\n", "");
      stp_send_command(v, "\f", "");	/* Eject page */
    }
}
