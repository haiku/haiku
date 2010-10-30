/*
 * "$Id: escp2-papers.c,v 1.119 2010/08/04 00:33:56 rlk Exp $"
 *
 *   Print plug-in EPSON ESC/P2 driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU eral Public License as published by the Free
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

static stp_mxml_node_t *
get_media_size_xml(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  return printdef->media_sizes;
}

int
stp_escp2_load_media_sizes(const stp_vars_t *v, const char *name)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  stp_list_t *dirlist = stpi_data_path();
  stp_list_item_t *item;
  int found = 0;
  item = stp_list_get_start(dirlist);
  while (item)
    {
      const char *dn = (const char *) stp_list_item_get_data(item);
      char *ffn = stpi_path_merge(dn, name);
      stp_mxml_node_t *sizes =
	stp_mxmlLoadFromFile(NULL, ffn, STP_MXML_NO_CALLBACK);
      stp_free(ffn);
      if (sizes)
	{
	  stp_mxml_node_t **xnode =
	    (stp_mxml_node_t **) &(printdef->media_sizes);
	  *xnode = sizes;
	  found = 1;
	  break;
	}
      item = stp_list_item_next(item);
    }
  stp_list_destroy(dirlist);
  STPI_ASSERT(found, v);
  return found;
}

void
stp_escp2_set_media_size(stp_vars_t *v, const stp_vars_t *src)
{
  const char *name = stp_get_string_parameter(src, "PageSize");
  if (name)
    {
      stp_mxml_node_t *node = get_media_size_xml(src);
      stp_mxml_node_t *xnode = stp_mxmlFindElement(node, node, "MediaSize",
						   "name", name, STP_MXML_DESCEND);
      if (xnode)
	{
	  stp_vars_fill_from_xmltree_ref(xnode->child, node, v);
	  return;
	}
      xnode = stp_mxmlFindElement(node, node, "MediaSize", "type", "default",
				  STP_MXML_DESCEND);
      if (xnode)
	{
	  stp_vars_fill_from_xmltree_ref(xnode->child, node, v);
	  return;
	}
    }
}

static const char *
paper_namefunc(const void *item)
{
  const paper_t *p = (const paper_t *) (item);
  return p->cname;
}

int
stp_escp2_load_media(const stp_vars_t *v, const char *name)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  stp_list_t *dirlist = stpi_data_path();
  stp_list_item_t *item;
  int found = 0;
  item = stp_list_get_start(dirlist);
  while (item)
    {
      const char *dn = (const char *) stp_list_item_get_data(item);
      char *ffn = stpi_path_merge(dn, name);
      stp_mxml_node_t *media =
	stp_mxmlLoadFromFile(NULL, ffn, STP_MXML_NO_CALLBACK);
      stp_free(ffn);
      if (media)
	{
	  stp_mxml_node_t **xnode =
	    (stp_mxml_node_t **) &(printdef->media);
	  stp_list_t **xlist =
	    (stp_list_t **) &(printdef->media_cache);
	  stp_string_list_t **xpapers =
	    (stp_string_list_t **) &(printdef->papers);
	  stp_mxml_node_t *node = stp_mxmlFindElement(media, media,
						      "escp2:papers", NULL,
						      NULL, STP_MXML_DESCEND);
	  *xnode = media;
	  *xlist = stp_list_create();
	  stp_list_set_namefunc(*xlist, paper_namefunc);
	  *xpapers = stp_string_list_create();
	  if (node)
	    {
	      node = node->child;
	      while (node)
		{
		  if (node->type == STP_MXML_ELEMENT &&
		      strcmp(node->value.element.name, "paper") == 0)
		    stp_string_list_add_string(*xpapers,
					       stp_mxmlElementGetAttr(node, "name"),
					       stp_mxmlElementGetAttr(node, "text"));
		  node = node->next;
		}
	    }
	  found = 1;
	  break;
	}
      item = stp_list_item_next(item);
    }
  stp_list_destroy(dirlist);
  STPI_ASSERT(found, v);
  return found;
}

static stp_mxml_node_t *
get_media_xml(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  return printdef->media;
}

static stp_list_t *
get_media_cache(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  return printdef->media_cache;
}

int
stp_escp2_has_media_feature(const stp_vars_t *v, const char *name)
{
  stp_mxml_node_t *doc = get_media_xml(v);
  if (doc)
    return (stp_mxmlFindElement(doc, doc, "feature", "name", name,
				STP_MXML_DESCEND) != NULL);
  else
    return 0;
}


static paper_t *
build_media_type(const stp_vars_t *v, const char *name, const inklist_t *ink,
		 const res_t *res)
{
  stp_mxml_node_t *node;
  stp_mxml_node_t *doc = get_media_xml(v);
  const char *pclass;
  paper_t *answer;
  stp_vars_t *vv = stp_vars_create();
  if (!doc)
    return NULL;
  node = stp_mxmlFindElement(doc, doc, "paper", "name", name, STP_MXML_DESCEND);
  if (!node)
    return NULL;
  answer = stp_zalloc(sizeof(paper_t));
  answer->name = stp_mxmlElementGetAttr(node, "name");
  answer->text = gettext(stp_mxmlElementGetAttr(node, "text"));
  pclass = stp_mxmlElementGetAttr(node, "class");
  answer->v = vv;
  if (! pclass || strcasecmp(pclass, "plain") == 0)
    answer->paper_class = PAPER_PLAIN;
  else if (strcasecmp(pclass, "good") == 0)
    answer->paper_class = PAPER_GOOD;
  else if (strcasecmp(pclass, "photo") == 0)
    answer->paper_class = PAPER_PHOTO;
  else if (strcasecmp(pclass, "premium") == 0)
    answer->paper_class = PAPER_PREMIUM_PHOTO;
  else if (strcasecmp(pclass, "transparency") == 0)
    answer->paper_class = PAPER_TRANSPARENCY;
  else
    answer->paper_class = PAPER_PLAIN;
  answer->preferred_ink_type = stp_mxmlElementGetAttr(node, "PreferredInktype");
  answer->preferred_ink_set = stp_mxmlElementGetAttr(node, "PreferredInkset");
  stp_vars_fill_from_xmltree_ref(node->child, doc, vv);
  if (ink && ink->name)
    {
      stp_mxml_node_t *inknode = stp_mxmlFindElement(node, node, "ink",
						     "name", ink->name,
						     STP_MXML_DESCEND);
      STPI_ASSERT(inknode, v);
      stp_vars_fill_from_xmltree_ref(inknode->child, doc, vv);
    }
  if (res && res->name)
    {
      stp_mxml_node_t *resnode = stp_mxmlFindElement(node, node, "resolution",
						     "name", res->name,
						     STP_MXML_DESCEND);
      if (resnode)
	stp_vars_fill_from_xmltree_ref(resnode->child, doc, vv);
    }
  return answer;
}

static char *
build_media_id(const char *name, const inklist_t *ink, const res_t *res)
{
  char *answer;
  stp_asprintf(&answer, "%s %s %s",
	       name,
	       ink ? ink->name : "",
	       res ? res->name : "");
  return answer;
}

static const paper_t *
get_media_type_named(const stp_vars_t *v, const char *name,
		     int ignore_res)
{
  paper_t *answer = NULL;
  int i;
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  const stp_string_list_t *p = printdef->papers;
  const res_t *res = ignore_res ? NULL : stp_escp2_find_resolution(v);
  const inklist_t *inklist = stp_escp2_inklist(v);
  char *media_id = build_media_id(name, inklist, res);
  stp_list_t *cache = get_media_cache(v);
  stp_list_item_t *li = stp_list_get_item_by_name(cache, media_id);
  if (li)
    {
      stp_free(media_id);
      answer = (paper_t *) stp_list_item_get_data(li);
    }
  else
    {
      int paper_type_count = stp_string_list_count(p);
      for (i = 0; i < paper_type_count; i++)
	{
	  if (!strcmp(name, stp_string_list_param(p, i)->name))
	    {
#ifdef HAVE_LOCALE_H
	      char *locale = stp_strdup(setlocale(LC_ALL, NULL));
	      setlocale(LC_ALL, "C");
#endif
	      answer = build_media_type(v, name, inklist, res);
#ifdef HAVE_LOCALE_H
	      setlocale(LC_ALL, locale);
	      stp_free(locale);
#endif
	      break;
	    }
	}
      if (answer)
	{
	  answer->cname = media_id;
	  stp_list_item_create(cache, NULL, answer);
	}
    }
  return answer;
}

const paper_t *
stp_escp2_get_media_type(const stp_vars_t *v, int ignore_res)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  const stp_string_list_t *p = printdef->papers;
  if (p)
    {
      const char *name = stp_get_string_parameter(v, "MediaType");
      if (name)
	return get_media_type_named(v, name, ignore_res);
    }
  return NULL;
}

const paper_t *
stp_escp2_get_default_media_type(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  const stp_string_list_t *p = printdef->papers;
  if (p)
    {
      int paper_type_count = stp_string_list_count(p);
      if (paper_type_count >= 0)
	return get_media_type_named(v, stp_string_list_param(p, 0)->name, 1);
    }
  return NULL;
}


static const char *
slots_namefunc(const void *item)
{
  const input_slot_t *p = (const input_slot_t *) (item);
  return p->name;
}

int
stp_escp2_load_input_slots(const stp_vars_t *v, const char *name)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  stp_list_t *dirlist = stpi_data_path();
  stp_list_item_t *item;
  int found = 0;
  item = stp_list_get_start(dirlist);
  while (item)
    {
      const char *dn = (const char *) stp_list_item_get_data(item);
      char *ffn = stpi_path_merge(dn, name);
      stp_mxml_node_t *slots =
	stp_mxmlLoadFromFile(NULL, ffn, STP_MXML_NO_CALLBACK);
      stp_free(ffn);
      if (slots)
	{
	  stp_mxml_node_t **xnode =
	    (stp_mxml_node_t **) &(printdef->slots);
	  stp_list_t **xlist =
	    (stp_list_t **) &(printdef->slots_cache);
	  stp_string_list_t **xslots =
	    (stp_string_list_t **) &(printdef->input_slots);
	  stp_mxml_node_t *node = stp_mxmlFindElement(slots, slots,
						      "escp2:InputSlots", NULL,
						      NULL, STP_MXML_DESCEND);
	  *xnode = slots;
	  *xlist = stp_list_create();
	  stp_list_set_namefunc(*xlist, slots_namefunc);
	  *xslots = stp_string_list_create();
	  if (node)
	    {
	      node = node->child;
	      while (node)
		{
		  if (node->type == STP_MXML_ELEMENT &&
		      strcmp(node->value.element.name, "slot") == 0)
		    stp_string_list_add_string(*xslots,
					       stp_mxmlElementGetAttr(node, "name"),
					       stp_mxmlElementGetAttr(node, "text"));
		  node = node->next;
		}
	    }
	  found = 1;
	  break;
	}
      item = stp_list_item_next(item);
    }
  stp_list_destroy(dirlist);
  STPI_ASSERT(found, v);
  return found;
}

static stp_mxml_node_t *
get_slots_xml(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  return printdef->slots;
}

static stp_list_t *
get_slots_cache(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  return printdef->slots_cache;
}

static input_slot_t *
build_input_slot(const stp_vars_t *v, const char *name)
{
  stp_mxml_node_t *node, *n1;
  stp_mxml_node_t *doc = get_slots_xml(v);
  input_slot_t *answer;
  if (!doc)
    return NULL;
  node = stp_mxmlFindElement(doc, doc, "slot", "name", name, STP_MXML_DESCEND);
  if (!node)
    return NULL;
  answer = stp_zalloc(sizeof(input_slot_t));
  answer->name = stp_mxmlElementGetAttr(node, "name");
  answer->text = gettext(stp_mxmlElementGetAttr(node, "text"));
  n1 = stp_mxmlFindElement(node, node, "CD", NULL, NULL, STP_MXML_DESCEND);
  if (n1)
    answer->is_cd = 1;
  n1 = stp_mxmlFindElement(node, node, "RollFeed", NULL, NULL, STP_MXML_DESCEND);
  if (n1)
    {
      answer->is_roll_feed = 1;
      if (stp_mxmlFindElement(n1, n1, "CutAll", NULL, NULL, STP_MXML_DESCEND))
	answer->roll_feed_cut_flags |= ROLL_FEED_CUT_ALL;
      if (stp_mxmlFindElement(n1, n1, "CutLast", NULL, NULL, STP_MXML_DESCEND))
	answer->roll_feed_cut_flags |= ROLL_FEED_CUT_LAST;
      if (stp_mxmlFindElement(n1, n1, "DontEject", NULL, NULL, STP_MXML_DESCEND))
	answer->roll_feed_cut_flags |= ROLL_FEED_DONT_EJECT;
    }
  n1 = stp_mxmlFindElement(node, node, "Duplex", NULL, NULL, STP_MXML_DESCEND);
  if (n1)
    {
      if (stp_mxmlFindElement(n1, n1, "Tumble", NULL, NULL, STP_MXML_DESCEND))
	answer->duplex |= DUPLEX_TUMBLE;
      if (stp_mxmlFindElement(n1, n1, "NoTumble", NULL, NULL, STP_MXML_DESCEND))
	answer->duplex |= DUPLEX_NO_TUMBLE;
    }
  n1 = stp_mxmlFindElement(node, node, "InitSequence", NULL, NULL, STP_MXML_DESCEND);
  if (n1 && n1->child && n1->child->type == STP_MXML_TEXT)
    answer->init_sequence = stp_xmlstrtoraw(n1->child->value.text.string);
  n1 = stp_mxmlFindElement(node, node, "DeinitSequence", NULL, NULL, STP_MXML_DESCEND);
  if (n1 && n1->child && n1->child->type == STP_MXML_TEXT)
    answer->deinit_sequence = stp_xmlstrtoraw(n1->child->value.text.string);
  n1 = stp_mxmlFindElement(node, node, "ExtraHeight", NULL, NULL, STP_MXML_DESCEND);
  if (n1 && n1->child && n1->child->type == STP_MXML_TEXT)
    answer->extra_height = stp_xmlstrtoul(n1->child->value.text.string);
  return answer;
}

int
stp_escp2_printer_supports_rollfeed(const stp_vars_t *v)
{
  stp_mxml_node_t *node = get_slots_xml(v);
  if (stp_mxmlFindElement(node, node, "RollFeed", NULL, NULL, STP_MXML_DESCEND))
    return 1;
  else
    return 0;
}

int
stp_escp2_printer_supports_print_to_cd(const stp_vars_t *v)
{
  stp_mxml_node_t *node = get_slots_xml(v);
  if (stp_mxmlFindElement(node, node, "CD", NULL, NULL, STP_MXML_DESCEND))
    return 1;
  else
    return 0;
}

int
stp_escp2_printer_supports_duplex(const stp_vars_t *v)
{
  stp_mxml_node_t *node = get_slots_xml(v);
  if (stp_mxmlFindElement(node, node, "Duplex", NULL, NULL, STP_MXML_DESCEND))
    return 1;
  else
    return 0;
}

static const input_slot_t *
get_input_slot_named(const stp_vars_t *v, const char *name)
{
  input_slot_t *answer = NULL;
  int i;
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  const stp_string_list_t *p = printdef->input_slots;
  stp_list_t *cache = get_slots_cache(v);
  stp_list_item_t *li = stp_list_get_item_by_name(cache, name);
  if (li)
    answer = (input_slot_t *) stp_list_item_get_data(li);
  else
    {
      int slot_count = stp_string_list_count(p);
      for (i = 0; i < slot_count; i++)
	{
	  if (!strcmp(name, stp_string_list_param(p, i)->name))
	    {
#ifdef HAVE_LOCALE_H
	      char *locale = stp_strdup(setlocale(LC_ALL, NULL));
	      setlocale(LC_ALL, "C");
#endif
	      answer = build_input_slot(v, name);
#ifdef HAVE_LOCALE_H
	      setlocale(LC_ALL, locale);
	      stp_free(locale);
#endif
	      break;
	    }
	}
      if (answer)
	stp_list_item_create(cache, NULL, answer);
    }
  return answer;
}

const input_slot_t *
stp_escp2_get_input_slot(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  const stp_string_list_t *p = printdef->input_slots;
  if (p)
    {
      const char *name = stp_get_string_parameter(v, "InputSlot");
      if (name)
	return get_input_slot_named(v, name);
    }
  return NULL;
}
