/*
 * "$Id: print-escp2-data.c,v 1.273 2010/08/04 00:33:56 rlk Exp $"
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include "print-escp2.h"
#include <limits.h>

typedef struct
{
  const char *attr_name;
  short bit_shift;
  short bit_width;
} escp2_printer_attr_t;

static const escp2_printer_attr_t escp2_printer_attrs[] =
{
  { "command_mode",		0, 4 },
  { "zero_margin",		4, 3 },
  { "variable_mode",		7, 1 },
  { "graymode",		 	8, 1 },
  { "fast_360",			9, 1 },
  { "send_zero_advance",       10, 1 },
  { "supports_ink_change",     11, 1 },
  { "packet_mode",             12, 1 },
  { "interchangeable_ink",     13, 1 },
  { "envelope_landscape",      14, 1 },
};

static stpi_escp2_printer_t *escp2_model_capabilities;

static int escp2_model_count = 0;

static void
load_model_from_file(const stp_vars_t *v, stp_mxml_node_t *xmod, int model)
{
  stp_mxml_node_t *tmp = xmod->child;
  stpi_escp2_printer_t *p = stp_escp2_get_printer(v);
  int found_black_head_config = 0;
  int found_fast_head_config = 0;
  p->max_black_resolution = -1;
  p->cd_x_offset = -1;
  p->cd_y_offset = -1;
  p->duplex_left_margin = SHRT_MIN;
  p->duplex_right_margin = SHRT_MIN;
  p->duplex_top_margin = SHRT_MIN;
  p->duplex_bottom_margin = SHRT_MIN;
  while (tmp)
    {
      if (tmp->type == STP_MXML_ELEMENT)
	{
	  const char *name = tmp->value.element.name;
	  const char *target = stp_mxmlElementGetAttr(tmp, "src");
	  if (target)
	    {
	      if (!strcmp(name, "media"))
		stp_escp2_load_media(v, target);
	      else if (!strcmp(name, "inputSlots"))
		stp_escp2_load_input_slots(v, target);
	      else if (!strcmp(name, "mediaSizes"))
		stp_escp2_load_media_sizes(v, target);
	      else if (!strcmp(name, "printerWeaves"))
		stp_escp2_load_printer_weaves(v, target);
	      else if (!strcmp(name, "qualityPresets"))
		stp_escp2_load_quality_presets(v, target);
	      else if (!strcmp(name, "resolutions"))
		stp_escp2_load_resolutions(v, target);
	      else if (!strcmp(name, "inkGroup"))
		stp_escp2_load_inkgroup(v, target);
	    }
	  else if (tmp->child && tmp->child->type == STP_MXML_TEXT)
	    {
	      stp_mxml_node_t *child = tmp->child;
	      const char *val = child->value.text.string;
	      if (!strcmp(name, "verticalBorderlessSequence"))
		{
		  STPI_ASSERT(!p->vertical_borderless_sequence, NULL);
		  p->vertical_borderless_sequence = stp_xmlstrtoraw(val);
		}
	      else if (!strcmp(name, "preinitSequence"))
		{
		  STPI_ASSERT(!p->preinit_sequence, NULL);
		  p->preinit_sequence = stp_xmlstrtoraw(val);
		}
	      else if (!strcmp(name, "preinitRemoteSequence"))
		{
		  STPI_ASSERT(!p->preinit_remote_sequence, NULL);
		  p->preinit_remote_sequence = stp_xmlstrtoraw(val);
		}
	      else if (!strcmp(name, "postinitRemoteSequence"))
		{
		  STPI_ASSERT(!p->postinit_remote_sequence, NULL);
		  p->postinit_remote_sequence = stp_xmlstrtoraw(val);
		}
	      else if (!strcmp(name, "commandSet"))
		{
		  if (!strcmp(val, "1998"))
		    p->flags |= MODEL_COMMAND_1998;
		  else if (!strcmp(val, "1999"))
		    p->flags |= MODEL_COMMAND_1999;
		  else if (!strcmp(val, "2000"))
		    p->flags |= MODEL_COMMAND_2000;
		  else if (!strcmp(val, "Pro"))
		    p->flags |= MODEL_COMMAND_PRO;
		}
	      else if (!strcmp(name, "borderless"))
		{
		  if (!strcmp(val, "No"))
		    p->flags |= MODEL_ZEROMARGIN_NO;
		  else if (!strcmp(val, "Yes"))
		    p->flags |= MODEL_ZEROMARGIN_YES;
		  else if (!strcmp(val, "Full"))
		    p->flags |= MODEL_ZEROMARGIN_FULL;
		  else if (!strcmp(val, "VerticalRestricted"))
		    p->flags |= MODEL_ZEROMARGIN_RESTR;
		  else if (!strcmp(val, "HorizontalOnly"))
		    p->flags |= MODEL_ZEROMARGIN_H_ONLY;
		}
	      else if (!strcmp(name, "preferredEnvelopeOrientation") &&
		       !strcmp(val, "Landscape"))
		p->flags |= MODEL_ENVELOPE_LANDSCAPE_YES;
	      else if (!strcmp(name, "headConfiguration"))
		{
		  const char *htype = stp_mxmlElementGetAttr(tmp, "type");
		  unsigned long data[4] = { 0, 0, 0, 0 };
		  while (child)
		    {
		      if (child->type == STP_MXML_ELEMENT && child->child &&
			  child->child->type == STP_MXML_TEXT)
			{
			  const char *cname = child->value.element.name;
			  const char *cval = child->child->value.text.string;
			  if (!strcmp(cname, "Nozzles"))
			    data[0] = stp_xmlstrtoul(cval);
			  else if (!strcmp(cname, "MinNozzles"))
			    data[1] = stp_xmlstrtoul(cval);
			  else if (!strcmp(cname, "FirstNozzle"))
			    data[2] = stp_xmlstrtoul(cval);
			  else if (!strcmp(cname, "NozzleSeparation"))
			    data[3] = stp_xmlstrtoul(cval);
			}
		      child = child->next;
		    }		      
		  if (!strcmp(htype, "default"))
		    {
		      p->nozzles = data[0];
		      p->min_nozzles = data[1];
		      p->nozzle_start = data[2];
		      p->nozzle_separation = data[3];
		      if (!found_black_head_config)
			{
			  p->black_nozzles = data[0];
			  p->min_black_nozzles = data[1];
			  p->black_nozzle_start = data[2];
			  p->black_nozzle_separation = data[3];
			}
		      if (!found_fast_head_config)
			{
			  p->fast_nozzles = data[0];
			  p->min_fast_nozzles = data[1];
			  p->fast_nozzle_start = data[2];
			  p->fast_nozzle_separation = data[3];
			}
		    }
		  else if (!strcmp(htype, "black"))
		    {
		      p->black_nozzles = data[0];
		      p->min_black_nozzles = data[1];
		      p->black_nozzle_start = data[2];
		      p->black_nozzle_separation = data[3];
		      found_black_head_config = 1;
		    }
		  else if (!strcmp(htype, "fast"))
		    {
		      p->fast_nozzles = data[0];
		      p->min_fast_nozzles = data[1];
		      p->fast_nozzle_start = data[2];
		      p->fast_nozzle_separation = data[3];
		      found_fast_head_config = 1;
		    }
		}
	      else if (!strcmp(name, "margins"))
		{
		  const char *itype = stp_mxmlElementGetAttr(tmp, "interleave");
		  const char *mtype = stp_mxmlElementGetAttr(tmp, "media");
		  const char *dtype = stp_mxmlElementGetAttr(tmp, "duplex");
		  unsigned long data[4];
		  int i = 0;
		  while (child && i < 4)
		    {
		      if (child->type == STP_MXML_TEXT)
			data[i++] = stp_xmlstrtoul(child->value.text.string);
		      child = child->next;
		    }		      
		  if (dtype && !strcmp(dtype, "duplex"))
		    {
		      p->duplex_left_margin = data[0];
		      p->duplex_right_margin = data[1];
		      p->duplex_top_margin = data[2];
		      p->duplex_bottom_margin = data[3];
		    }
		  else if (itype && !strcmp(itype, "soft") &&
			   mtype && !strcmp(mtype, "sheet"))
		    {
		      p->left_margin = data[0];
		      p->right_margin = data[1];
		      p->top_margin = data[2];
		      p->bottom_margin = data[3];
		    }
		  else if (itype && !strcmp(itype, "printer") &&
			   mtype && !strcmp(mtype, "sheet"))
		    {
		      p->m_left_margin = data[0];
		      p->m_right_margin = data[1];
		      p->m_top_margin = data[2];
		      p->m_bottom_margin = data[3];
		    }
		  else if (itype && !strcmp(itype, "soft") &&
			   mtype && !strcmp(mtype, "roll"))
		    {
		      p->roll_left_margin = data[0];
		      p->roll_right_margin = data[1];
		      p->roll_top_margin = data[2];
		      p->roll_bottom_margin = data[3];
		    }
		  else if (itype && !strcmp(itype, "printer") &&
			   mtype && !strcmp(mtype, "roll"))
		    {
		      p->m_roll_left_margin = data[0];
		      p->m_roll_right_margin = data[1];
		      p->m_roll_top_margin = data[2];
		      p->m_roll_bottom_margin = data[3];
		    }
		}
	      else if (!strcmp(name, "physicalChannels"))
		p->physical_channels = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "baseSeparation"))
		p->base_separation = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "resolutionScale"))
		p->resolution_scale = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "maxBlackResolution"))
		p->max_black_resolution = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "minimumResolution"))
		{
		  p->min_hres = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->min_vres = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "maximumResolution"))
		{
		  p->max_hres = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->max_vres = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "extraVerticalFeed"))
		p->extra_feed = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "separationRows"))
		p->separation_rows = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "pseudoSeparationRows"))
		p->pseudo_separation_rows = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "zeroMarginOffset"))
		p->zero_margin_offset = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "microLeftMargin"))
		p->micro_left_margin = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "initialVerticalOffset"))
		p->initial_vertical_offset = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "blackInitialVerticalOffset"))
		p->black_initial_vertical_offset = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "extra720DPISeparation"))
		p->extra_720dpi_separation = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "minHorizontalAlignment"))
		p->min_horizontal_position_alignment = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "baseHorizontalAlignment"))
		p->base_horizontal_position_alignment = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "bidirectionalAutoUpperLimit"))
		p->bidirectional_upper_limit = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "minimumMediaSize"))
		{
		  p->min_paper_width = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->min_paper_height = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "maximumMediaSize"))
		{
		  p->max_paper_width = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->max_paper_height = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "maximumImageableArea"))
		{
		  p->max_imageable_width = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->max_imageable_height = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "CDOffset"))
		{
		  p->cd_x_offset = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->cd_y_offset = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "CDMediaSize"))
		{
		  p->cd_page_width = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->cd_page_height = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "extraBottom"))
		p->paper_extra_bottom = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "AlignmentChoices"))
		{
		  p->alignment_passes =
		    stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->alignment_choices =
		    stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->alternate_alignment_passes =
		    stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->alternate_alignment_choices =
		    stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "ChannelNames"))
		{
		  p->channel_names = stp_string_list_create();
		  while (child)
		    {
		      if (child->type == STP_MXML_ELEMENT &&
			  !strcmp(child->value.element.name, "ChannelName"))
			{
			  const char *cname = stp_mxmlElementGetAttr(child, "name");
			  stp_string_list_add_string(p->channel_names, cname, cname);
			}
		      child = child->next;
		    }
		}
	      else if (!strcmp(name, "resolutions"))
		stp_escp2_load_resolutions_from_xml(v, tmp);
	    }
	  else
	    {
	      if (!strcmp(name, "supportsVariableDropsizes"))
		p->flags |= MODEL_VARIABLE_YES;
	      else if (!strcmp(name, "hasFastGraymode"))
		p->flags |= MODEL_GRAYMODE_YES;
	      else if (!strcmp(name, "hasFast360DPI"))
		p->flags |= MODEL_FAST_360_YES;
	      else if (!strcmp(name, "sendZeroAdvance"))
		p->flags |= MODEL_SEND_ZERO_ADVANCE_YES;
	      else if (!strcmp(name, "supportsInkChange"))
		p->flags |= MODEL_SUPPORTS_INK_CHANGE_YES;
	      else if (!strcmp(name, "supportsD4Mode"))
		p->flags |= MODEL_PACKET_MODE_YES;
	      else if (!strcmp(name, "hasInterchangeableInkCartridges"))
		p->flags |= MODEL_INTERCHANGEABLE_INK_YES;
	      else if (!strcmp(name, "resolutions"))
		stp_escp2_load_resolutions_from_xml(v, tmp);
	    }
	}
      tmp = tmp->next;
    }
}

void
stp_escp2_load_model(const stp_vars_t *v, int model)
{
  stp_list_t *dirlist = stpi_data_path();
  stp_list_item_t *item;
  char buf[1024];
  int found = 0;

  stp_xml_init();
  sprintf(buf, "escp2/model/model_%d.xml", model);
  item = stp_list_get_start(dirlist);
  while (item)
    {
      const char *dn = (const char *) stp_list_item_get_data(item);
      char *fn = stpi_path_merge(dn, buf);
      stp_mxml_node_t *doc = stp_mxmlLoadFromFile(NULL, fn, STP_MXML_NO_CALLBACK);
      stp_free(fn);
      if (doc)
	{
	  stp_mxml_node_t *node =
	    stp_mxmlFindElement(doc, doc, "escp2:model", NULL, NULL,
				STP_MXML_DESCEND);
	  if (node)
	    {
	      const char *stmp = stp_mxmlElementGetAttr(node, "id");
	      STPI_ASSERT(stmp && stp_xmlstrtol(stmp) == model, v);
	      load_model_from_file(v, node, model);
	      found = 1;
	    }
	  stp_mxmlDelete(doc);
	  if (found)
	    break;
	}
      item = stp_list_item_next(item);
    }
  stp_xml_exit();
  stp_list_destroy(dirlist);
  STPI_ASSERT(found, v);
}

stpi_escp2_printer_t *
stp_escp2_get_printer(const stp_vars_t *v)
{
  int model = stp_get_model_id(v);
  STPI_ASSERT(model >= 0, v);
  if (!escp2_model_capabilities)
    {
      escp2_model_capabilities =
	stp_zalloc(sizeof(stpi_escp2_printer_t) * (model + 1));
      escp2_model_count = model + 1;
    }
  else if (model >= escp2_model_count)
    {
      escp2_model_capabilities =
	stp_realloc(escp2_model_capabilities,
		    sizeof(stpi_escp2_printer_t) * (model + 1));
      (void) memset(escp2_model_capabilities + escp2_model_count, 0,
		    sizeof(stpi_escp2_printer_t) * (model + 1 - escp2_model_count));
      escp2_model_count = model + 1;
    }
  if (!(escp2_model_capabilities[model].active))
    {
#ifdef HAVE_LOCALE_H
      char *locale = stp_strdup(setlocale(LC_ALL, NULL));
      setlocale(LC_ALL, "C");
#endif
      escp2_model_capabilities[model].active = 1;
      stp_escp2_load_model(v, model);
#ifdef HAVE_LOCALE_H
      setlocale(LC_ALL, locale);
      stp_free(locale);
#endif
    }
  return &(escp2_model_capabilities[model]);
}

model_featureset_t
stp_escp2_get_cap(const stp_vars_t *v, escp2_model_option_t feature)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  model_featureset_t featureset =
    (((1ul << escp2_printer_attrs[feature].bit_width) - 1ul) <<
     escp2_printer_attrs[feature].bit_shift);
  return printdef->flags & featureset;
}

int
stp_escp2_has_cap(const stp_vars_t *v, escp2_model_option_t feature,
		  model_featureset_t class)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  model_featureset_t featureset =
    (((1ul << escp2_printer_attrs[feature].bit_width) - 1ul) <<
     escp2_printer_attrs[feature].bit_shift);
  return ((printdef->flags & featureset) == class);
}
