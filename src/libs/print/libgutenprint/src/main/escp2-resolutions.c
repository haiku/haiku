/*
 * "$Id: escp2-resolutions.c,v 1.52 2010/08/04 00:33:56 rlk Exp $"
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


int
stp_escp2_load_printer_weaves_from_xml(const stp_vars_t *v,
				       stp_mxml_node_t *node)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  printer_weave_list_t *xpw = stp_malloc(sizeof(printer_weave_list_t));
  int count = 0;
  stp_mxml_node_t *child = node->child;
  while (child)
    {
      if (child->type == STP_MXML_ELEMENT &&
	  !strcmp(child->value.element.name, "weave"))
	count++;
      child = child->next;
    }
  printdef->printer_weaves = xpw;
  if (stp_mxmlElementGetAttr(node, "name"))
    xpw->name = stp_strdup(stp_mxmlElementGetAttr(node, "name"));
  xpw->n_printer_weaves = count;
  xpw->printer_weaves = stp_zalloc(sizeof(printer_weave_t) * count);
  child = node->child;
  count = 0;
  while (child)
    {
      if (child->type == STP_MXML_ELEMENT &&
	  !strcmp(child->value.element.name, "weave"))
	{
	  const char *wname = stp_mxmlElementGetAttr(child, "name");
	  const char *wtext = stp_mxmlElementGetAttr(child, "text");
	  const char *cmd = stp_mxmlElementGetAttr(child, "command");
	  if (wname)
	    xpw->printer_weaves[count].name = stp_strdup(wname);
	  if (wtext)
	    xpw->printer_weaves[count].text = stp_strdup(wtext);
	  if (cmd)
	    xpw->printer_weaves[count].command = stp_xmlstrtoraw(cmd);
	  count++;
	}
      child = child->next;
    }
  return 1;
}

int
stp_escp2_load_printer_weaves(const stp_vars_t *v, const char *name)
{
  stp_list_t *dirlist = stpi_data_path();
  stp_list_item_t *item;
  int found = 0;
  item = stp_list_get_start(dirlist);
  while (item)
    {
      const char *dn = (const char *) stp_list_item_get_data(item);
      char *ffn = stpi_path_merge(dn, name);
      stp_mxml_node_t *weaves =
	stp_mxmlLoadFromFile(NULL, ffn, STP_MXML_NO_CALLBACK);
      stp_free(ffn);
      if (weaves)
	{
	  stp_mxml_node_t *node = stp_mxmlFindElement(weaves, weaves,
						      "escp2:PrinterWeaves", NULL,
						      NULL, STP_MXML_DESCEND);
	  if (node)
	    stp_escp2_load_printer_weaves_from_xml(v, node);
	  stp_mxmlDelete(weaves);
	  found = 1;
	  break;
	}
      item = stp_list_item_next(item);
    }
  stp_list_destroy(dirlist);
  STPI_ASSERT(found, v);
  return found;
}

int
stp_escp2_load_resolutions_from_xml(const stp_vars_t *v, stp_mxml_node_t *node)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  resolution_list_t *xrs = stp_malloc(sizeof(resolution_list_t));
  int count = 0;
  stp_mxml_node_t *child = node->child;
  while (child)
    {
      if (child->type == STP_MXML_ELEMENT &&
	  !strcmp(child->value.element.name, "resolution"))
	count++;
      child = child->next;
    }
  printdef->resolutions = xrs;
  if (stp_mxmlElementGetAttr(node, "name"))
    xrs->name = stp_strdup(stp_mxmlElementGetAttr(node, "name"));
  xrs->n_resolutions = count;
  xrs->resolutions = stp_zalloc(sizeof(res_t) * count);
  child = node->child;
  count = 0;
  while (child)
    {
      if (child->type == STP_MXML_ELEMENT &&
	  !strcmp(child->value.element.name, "resolution"))
	{
	  res_t *res = &(xrs->resolutions[count]);
	  stp_mxml_node_t *cchild = child->child;
	  const char *wname = stp_mxmlElementGetAttr(child, "name");
	  const char *wtext = stp_mxmlElementGetAttr(child, "text");
	  res->v = stp_vars_create();
	  res->vertical_passes = 1;
	  if (wname)
	    res->name = stp_strdup(wname);
	  if (wtext)
	    res->text = stp_strdup(wtext);
	  stp_vars_fill_from_xmltree_ref(cchild, node, res->v);
	  while (cchild)
	    {
	      if (cchild->type == STP_MXML_ELEMENT)
		{
		  const char *elt = cchild->value.element.name;
		  if (cchild->type == STP_MXML_ELEMENT &&
		      (!strcmp(elt, "physicalResolution") ||
		       !strcmp(elt, "printedResolution")))
		    {
		      long data[2] = { 0, 0 };
		      stp_mxml_node_t *ccchild = cchild->child;
		      data[0] = stp_xmlstrtol(ccchild->value.text.string);
		      ccchild = ccchild->next;
		      data[1] = stp_xmlstrtol(ccchild->value.text.string);
		      if (!strcmp(elt, "physicalResolution"))
			{
			  res->hres = data[0];
			  res->vres = data[1];
			}			      
		      else if (!strcmp(elt, "printedResolution"))
			{
			  res->printed_hres = data[0];
			  res->printed_vres = data[1];
			}			      
		    }
		  else if (!strcmp(elt, "verticalPasses") &&
			   cchild->child &&
			   cchild->child->type == STP_MXML_TEXT)
		    res->vertical_passes = stp_xmlstrtol(cchild->child->value.text.string);
		  else if (!strcmp(elt, "printerWeave") &&
			   stp_mxmlElementGetAttr(cchild, "command"))
		    res->command = stp_xmlstrtoraw(stp_mxmlElementGetAttr(cchild, "command"));
		}
	      cchild = cchild->next;
	    }
	  if (!res->printed_hres)
	    res->printed_hres = res->hres;
	  if (!res->printed_vres)
	    res->printed_vres = res->vres;
	  count++;
	}
      child = child->next;
    }
  return 1;
}

int
stp_escp2_load_resolutions(const stp_vars_t *v, const char *name)
{
  stp_list_t *dirlist = stpi_data_path();
  stp_list_item_t *item;
  int found = 0;
  item = stp_list_get_start(dirlist);
  while (item)
    {
      const char *dn = (const char *) stp_list_item_get_data(item);
      char *ffn = stpi_path_merge(dn, name);
      stp_mxml_node_t *resolutions =
	stp_mxmlLoadFromFile(NULL, ffn, STP_MXML_NO_CALLBACK);
      stp_free(ffn);
      if (resolutions)
	{
	  stp_mxml_node_t *node = stp_mxmlFindElement(resolutions, resolutions,
						      "escp2:resolutions", NULL,
						      NULL, STP_MXML_DESCEND);
	  if (node)
	    stp_escp2_load_resolutions_from_xml(v, node);
	  stp_mxmlDelete(resolutions);
	  found = 1;
	  break;
	}
      item = stp_list_item_next(item);
    }
  stp_list_destroy(dirlist);
  STPI_ASSERT(found, v);
  return found;
}

int
stp_escp2_load_quality_presets_from_xml(const stp_vars_t *v, stp_mxml_node_t *node)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  quality_list_t *qpw = stp_malloc(sizeof(quality_list_t));
  int count = 0;
  stp_mxml_node_t *child = node->child;
  while (child)
    {
      if (child->type == STP_MXML_ELEMENT &&
	  !strcmp(child->value.element.name, "quality"))
	count++;
      child = child->next;
    }
  printdef->quality_list = qpw;
  if (stp_mxmlElementGetAttr(node, "name"))
    qpw->name = stp_strdup(stp_mxmlElementGetAttr(node, "name"));
  qpw->n_quals = count;
  qpw->qualities = stp_zalloc(sizeof(quality_t) * count);
  child = node->child;
  count = 0;
  while (child)
    {
      if (child->type == STP_MXML_ELEMENT &&
	  !strcmp(child->value.element.name, "quality"))
	{
	  stp_mxml_node_t *cchild = child->child;
	  const char *wname = stp_mxmlElementGetAttr(child, "name");
	  const char *wtext = stp_mxmlElementGetAttr(child, "text");
	  if (wname)
	    qpw->qualities[count].name = stp_strdup(wname);
	  if (wtext)
	    qpw->qualities[count].text = stp_strdup(wtext);
	  while (cchild)
	    {
	      if (cchild->type == STP_MXML_ELEMENT &&
		  (!strcmp(cchild->value.element.name, "minimumResolution") ||
		   !strcmp(cchild->value.element.name, "maximumResolution") ||
		   !strcmp(cchild->value.element.name, "desiredResolution")))
		{
		  long data[2] = { 0, 0 };
		  stp_mxml_node_t *ccchild = cchild->child;
		  data[0] = stp_xmlstrtol(ccchild->value.text.string);
		  ccchild = ccchild->next;
		  data[1] = stp_xmlstrtol(ccchild->value.text.string);
		  if (!strcmp(cchild->value.element.name, "minimumResolution"))
		    {
		      qpw->qualities[count].min_hres = data[0];
		      qpw->qualities[count].min_vres = data[1];
		    }			      
		  else if (!strcmp(cchild->value.element.name, "maximumResolution"))
		    {
		      qpw->qualities[count].max_hres = data[0];
		      qpw->qualities[count].max_vres = data[1];
		    }			      
		  else if (!strcmp(cchild->value.element.name, "desiredResolution"))
		    {
		      qpw->qualities[count].desired_hres = data[0];
		      qpw->qualities[count].desired_vres = data[1];
		    }			      
		}
	      cchild = cchild->next;
	    }
	  count++;
	}
      child = child->next;
    }
  return 1;
}

int
stp_escp2_load_quality_presets(const stp_vars_t *v, const char *name)
{
  stp_list_t *dirlist = stpi_data_path();
  stp_list_item_t *item;
  int found = 0;
  item = stp_list_get_start(dirlist);
  while (item)
    {
      const char *dn = (const char *) stp_list_item_get_data(item);
      char *ffn = stpi_path_merge(dn, name);
      stp_mxml_node_t *qualities =
	stp_mxmlLoadFromFile(NULL, ffn, STP_MXML_NO_CALLBACK);
      stp_free(ffn);
      if (qualities)
	{
	  stp_mxml_node_t *node = stp_mxmlFindElement(qualities, qualities,
						      "escp2:QualityPresets", NULL,
						      NULL, STP_MXML_DESCEND);
	  if (node)
	    stp_escp2_load_quality_presets_from_xml(v, node);
	  stp_mxmlDelete(qualities);
	  found = 1;
	  break;
	}
      item = stp_list_item_next(item);
    }
  stp_list_destroy(dirlist);
  STPI_ASSERT(found, v);
  return found;
}
