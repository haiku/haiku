/*
 * "$Id: escp2-channels.c,v 1.91 2010/12/19 02:51:37 rlk Exp $"
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

static inkgroup_t *default_black_inkgroup;

static void
load_subchannel(stp_mxml_node_t *node, stp_mxml_node_t *root, physical_subchannel_t *icl)
{
  const char *name;
  stp_mxml_node_t *child = node->child;
  name = stp_mxmlElementGetAttr(node, "color");
  if (name)
    icl->color = stp_xmlstrtol(name);
  name = stp_mxmlElementGetAttr(node, "subchannel");
  if (name)
    icl->subchannel = stp_xmlstrtol(name);
  else
    icl->subchannel = -1;
  name = stp_mxmlElementGetAttr(node, "headOffset");
  if (name)
    icl->head_offset = stp_xmlstrtol(name);
  name = stp_mxmlElementGetAttr(node, "name");
  if (name)
    icl->name = stp_strdup(name);
  name = stp_mxmlElementGetAttr(node, "text");
  if (name)
    icl->text = stp_strdup(name);
  while (child)
    {
      if (child->type == STP_MXML_ELEMENT)
	{
	  const char *param = child->value.element.name;
	  name = stp_mxmlElementGetAttr(child, "name");
	  if (name && !strcmp(param, "ChannelDensityParam"))
	    icl->channel_density = stp_strdup(name);
	  else if (name && !strcmp(param, "SubchannelTransitionParam"))
	    icl->subchannel_transition = stp_strdup(name);
	  else if (name && !strcmp(param, "SubchannelValueParam"))
	    icl->subchannel_value = stp_strdup(name);
	  else if (name && !strcmp(param, "SubchannelScaleParam"))
	    icl->subchannel_scale = stp_strdup(name);
	  else if (!strcmp(param, "SplitChannels"))
	    {
	      if (stp_mxmlElementGetAttr(child, "count"))
		icl->split_channel_count =
		  stp_xmlstrtoul(stp_mxmlElementGetAttr(child, "count"));
	      if (icl->split_channel_count > 0)
		{
		  char *endptr;
		  int count = 0;
		  stp_mxml_node_t *cchild = child->child;
		  icl->split_channels =
		    stp_zalloc(sizeof(short) * icl->split_channel_count);
		  while (cchild && count < icl->split_channel_count)
		    {
		      if (cchild->type == STP_MXML_TEXT)
			{
			  unsigned val =
			    strtoul(cchild->value.text.string, &endptr, 0);
			  if (endptr)
			    icl->split_channels[count++] = val;
			}
		      cchild = cchild->next;
		    }
		}
	    }
	}
      child = child->next;
    }
}

static void
load_channel(stp_mxml_node_t *node, stp_mxml_node_t *root, ink_channel_t *icl)
{
  const char *name;
  stp_mxml_node_t *child = node->child;
  int count = 0;
  while (child)
    {
      if (child->type == STP_MXML_ELEMENT &&
	  !strcmp(child->value.element.name, "subchannel"))
	count++;
      child = child->next;
    }
  name = stp_mxmlElementGetAttr(node, "name");
  if (name)
    icl->name = stp_strdup(name);
  icl->n_subchannels = count;
  icl->subchannels = stp_zalloc(sizeof(physical_subchannel_t) * count);
  count = 0;
  child = node->child;
  while (child)
    {
      if (child->type == STP_MXML_ELEMENT)
	{
	  if (!strcmp(child->value.element.name, "subchannel"))
	    load_subchannel(child, root, &(icl->subchannels[count++]));
	  else if (!strcmp(child->value.element.name, "HueCurve"))
	    {
	      stp_mxml_node_t *cchild = child->child;
	      stp_curve_t *curve;
	      const char *cref = stp_mxmlElementGetAttr(child, "ref");
	      if (cref)
		{
		  cchild = stp_mxmlFindElement(root, root, "curve", "name",
					       cref, STP_MXML_DESCEND);
		  STPI_ASSERT(cchild, NULL);
		}
	      else
		{
		  while (cchild && cchild->type != STP_MXML_ELEMENT)
		    cchild = cchild->next;
		  STPI_ASSERT(cchild, NULL);
		}
	      curve = stp_curve_create_from_xmltree(cchild);
	      icl->hue_curve = curve;
	    }
	  else if (!strcmp(child->value.element.name, "HueCurveParam"))
	    {
	      name = stp_mxmlElementGetAttr(child, "name");
	      if (name)
		icl->hue_curve_name = stp_strdup(name);
	    }
	}
      child = child->next;
    }
}

static void
load_inkname(stp_mxml_node_t *node, stp_mxml_node_t *root, inkname_t *inl,
	     inklist_t *ikl)
{
  const char *name;
  stp_mxml_node_t *child = node->child;
  int channel_count = 0;
  int aux_channel_count = 0;

  while (child)
    {
      if (child->type == STP_MXML_ELEMENT)
	{
	  if (!strcmp(child->value.element.name, "Channels"))
	    {
	      stp_mxml_node_t *cchild = child->child;
	      while (cchild)
		{
		  if (cchild->type == STP_MXML_ELEMENT &&
		      !strcmp(cchild->value.element.name, "channel"))
		    {
		      name = stp_mxmlElementGetAttr(cchild, "index");
		      if (name)
			{
			  unsigned idx = stp_xmlstrtoul(name);
			  if (idx + 1 > channel_count)
			    channel_count = idx + 1;
			}
		    }
		  cchild = cchild->next;
		}
	    }
	  else if (!strcmp(child->value.element.name, "AuxChannels"))
	    {
	      stp_mxml_node_t *cchild = child->child;
	      while (cchild)
		{
		  if (cchild->type == STP_MXML_ELEMENT &&
		      !strcmp(cchild->value.element.name, "channel"))
		    {
		      name = stp_mxmlElementGetAttr(cchild, "index");
		      if (name)
			{
			  unsigned idx = stp_xmlstrtoul(name);
			  if (idx + 1 > aux_channel_count)
			    aux_channel_count = idx + 1;
			}
		    }
		  cchild = cchild->next;
		}
	    }
	}
      child = child->next;
    }
  inl->channel_count = channel_count;
  if (channel_count > 0)
    inl->channels = stp_zalloc(sizeof(ink_channel_t) * channel_count);
  inl->aux_channel_count = aux_channel_count;
  if (aux_channel_count > 0)
    inl->aux_channels = stp_zalloc(sizeof(ink_channel_t) * aux_channel_count);
  name = stp_mxmlElementGetAttr(node, "name");
  if (name)
    inl->name = stp_strdup(name);
  name = stp_mxmlElementGetAttr(node, "text");
  if (name)
    inl->text = stp_strdup(name);
  name = stp_mxmlElementGetAttr(node, "InkID");
  if (name)
    {
      if (!strcmp(name, "CMYK"))
	inl->inkset = INKSET_CMYK;
      else if (!strcmp(name, "CcMmYK"))
	inl->inkset = INKSET_CcMmYK;
      else if (!strcmp(name, "CcMmYyK"))
	inl->inkset = INKSET_CcMmYyK;
      else if (!strcmp(name, "CcMmYKk"))
	inl->inkset = INKSET_CcMmYKk;
      else if (!strcmp(name, "Quadtone"))
	inl->inkset = INKSET_QUADTONE;
      else if (!strcmp(name, "Hextone"))
	inl->inkset = INKSET_HEXTONE;
      else if (!strcmp(name, "OTHER"))
	inl->inkset = INKSET_OTHER;
      else if (!strcmp(name, "Extended"))
	inl->inkset = INKSET_EXTENDED;
    }

  channel_count = 0;
  aux_channel_count = 0;
  child = node->child;
  while (child)
    {
      if (child->type == STP_MXML_ELEMENT)
	{
	  if (!strcmp(child->value.element.name, "Channels"))
	    {
	      stp_mxml_node_t *cchild = child->child;
	      while (cchild)
		{
		  if (cchild->type == STP_MXML_ELEMENT &&
		      !strcmp(cchild->value.element.name, "channel"))
		    {
		      name = stp_mxmlElementGetAttr(cchild, "index");
		      if (name)
			{
			  unsigned idx = stp_xmlstrtoul(name);
			  load_channel(cchild, root, &(inl->channels[idx]));
			}
		    }
		  cchild = cchild->next;
		}
	    }
	  else if (!strcmp(child->value.element.name, "AuxChannels"))
	    {
	      stp_mxml_node_t *cchild = child->child;
	      while (cchild)
		{
		  if (cchild->type == STP_MXML_ELEMENT &&
		      !strcmp(cchild->value.element.name, "channel"))
		    {
		      name = stp_mxmlElementGetAttr(cchild, "index");
		      if (name)
			{
			  unsigned idx = stp_xmlstrtoul(name);
			  load_channel(cchild, root, &(inl->aux_channels[idx]));
			}
		    }
		  cchild = cchild->next;
		}
	    }
	  else if (!strcmp(child->value.element.name, "initSequence") &&
		   child->child && child->child->type == STP_MXML_TEXT)
	    ikl->init_sequence = stp_xmlstrtoraw(child->child->value.text.string);
	  else if (!strcmp(child->value.element.name, "deinitSequence") &&
		   child->child && child->child->type == STP_MXML_TEXT)
	    ikl->deinit_sequence = stp_xmlstrtoraw(child->child->value.text.string);
	}
      child = child->next;
    }
}

static void
load_shades(stp_mxml_node_t *node, stp_mxml_node_t *root, inklist_t *ikl)
{
  stp_mxml_node_t *child = node->child;
  int count = 0;

  while (child)
    {
      if (child->type == STP_MXML_ELEMENT &&
	  !strcmp(child->value.element.name, "shade"))
	count++;
      child = child->next;
    }
  ikl->n_shades = count;
  ikl->shades = stp_zalloc(sizeof(shade_t) * count);
  count = 0;
  child = node->child;
  while (child)
    {
      if (child->type == STP_MXML_ELEMENT &&
	  !strcmp(child->value.element.name, "shade"))
	{
	  if (stp_mxmlElementGetAttr(child, "count"))
	    {
	      unsigned nshades =
		stp_xmlstrtoul(stp_mxmlElementGetAttr(child, "count"));
	      ikl->shades[count].n_shades = nshades;
	      if (nshades > 0)
		{
		  char *endptr;
		  stp_mxml_node_t *cchild = child->child;
		  ikl->shades[count].shades = stp_zalloc(sizeof(double) * nshades);
		  nshades = 0;
		  while (cchild && nshades < ikl->shades[count].n_shades)
		    {
		      if (cchild->type == STP_MXML_TEXT)
			{
			  double val =
			    strtod(cchild->value.text.string, &endptr);
			  if (endptr)
			    ikl->shades[count].shades[nshades++] = val;
			}
		      cchild = cchild->next;
		    }
		}
	    }
	  count++;
	}
      child = child->next;
    }
}

static void
load_inklist(stp_mxml_node_t *node, stp_mxml_node_t *root, inklist_t *ikl)
{
  const char *name;
  stp_mxml_node_t *child = node->child;
  int count = 0;

  while (child)
    {
      if (child->type == STP_MXML_ELEMENT)
	{
	  if (!strcmp(child->value.element.name, "InkName"))
	    count++;
	  else if (!strcmp(child->value.element.name, "initSequence") &&
		   child->child && child->child->type == STP_MXML_TEXT)
	    ikl->init_sequence = stp_xmlstrtoraw(child->child->value.text.string);
	  else if (!strcmp(child->value.element.name, "deinitSequence") &&
		   child->child && child->child->type == STP_MXML_TEXT)
	    ikl->deinit_sequence = stp_xmlstrtoraw(child->child->value.text.string);
	}
      child = child->next;
    }
  name = stp_mxmlElementGetAttr(node, "name");
  if (name)
    ikl->name = stp_strdup(name);
  name = stp_mxmlElementGetAttr(node, "text");
  if (name)
    ikl->text = stp_strdup(name);
  ikl->n_inks = count;
  ikl->inknames = stp_zalloc(sizeof(inkname_t) * count);
  count = 0;
  child = node->child;
  while (child)
    {
      if (child->type == STP_MXML_ELEMENT)
	{
	  if (!strcmp(child->value.element.name, "InkName"))
	    {
	      inkname_t *inl = &(ikl->inknames[count++]);
	      inl->init_sequence = ikl->init_sequence;
	      inl->deinit_sequence = ikl->deinit_sequence;
	      load_inkname(child, root, inl, ikl);
	    }
	  else if (!strcmp(child->value.element.name, "Shades"))
	    load_shades(child, root, ikl);
	}
      child = child->next;
    }
}

static inkgroup_t *
load_inkgroup(const char *name)
{
  stp_list_t *dirlist = stpi_data_path();
  stp_list_item_t *item;
  inkgroup_t *igl = NULL;
  item = stp_list_get_start(dirlist);
  while (item)
    {
      const char *dn = (const char *) stp_list_item_get_data(item);
      char *ffn = stpi_path_merge(dn, name);
      stp_mxml_node_t *inkgroup =
	stp_mxmlLoadFromFile(NULL, ffn, STP_MXML_NO_CALLBACK);
      stp_free(ffn);
      if (inkgroup)
	{
	  int count = 0;
	  stp_mxml_node_t *node = stp_mxmlFindElement(inkgroup, inkgroup,
						      "escp2:InkGroup", NULL,
						      NULL, STP_MXML_DESCEND);
	  if (node)
	    {
	      stp_mxml_node_t *child = node->child;
	      igl = stp_zalloc(sizeof(inkgroup_t));
	      while (child)
		{
		  if (child->type == STP_MXML_ELEMENT &&
		      !strcmp(child->value.element.name, "InkList"))
		    count++;
		  child = child->next;
		}
	      igl->n_inklists = count;
	      if (stp_mxmlElementGetAttr(node, "name"))
		igl->name = stp_strdup(stp_mxmlElementGetAttr(node, "name"));
	      else
		igl->name = stp_strdup(name);
	      igl->inklists = stp_zalloc(sizeof(inklist_t) * count);
	      child = node->child;
	      count = 0;
	      while (child)
		{
		  if (child->type == STP_MXML_ELEMENT &&
		      !strcmp(child->value.element.name, "InkList"))
		    load_inklist(child, node, &(igl->inklists[count++]));
		  child = child->next;
		}
	    }
	  stp_mxmlDelete(inkgroup);
	  break;
	}
      item = stp_list_item_next(item);
    }
  stp_list_destroy(dirlist);
  return igl;
}

int
stp_escp2_load_inkgroup(const stp_vars_t *v, const char *name)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  inkgroup_t *igl = load_inkgroup(name);
  STPI_ASSERT(igl, v);
  printdef->inkgroup = igl;
  return (igl != NULL);
}

const inkname_t *
stpi_escp2_get_default_black_inkset(void)
{
  if (! default_black_inkgroup)
    {
      default_black_inkgroup = load_inkgroup("escp2/inks/defaultblack.xml");
      STPI_ASSERT(default_black_inkgroup &&
		  default_black_inkgroup->n_inklists >= 1 &&
		  default_black_inkgroup->inklists[0].n_inks >= 1, NULL);
    }
  return &(default_black_inkgroup->inklists[0].inknames[0]);
}
