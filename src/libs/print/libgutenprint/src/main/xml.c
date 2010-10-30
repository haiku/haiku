/*
 * "$Id: xml.c,v 1.43 2010/08/04 00:33:57 rlk Exp $"
 *
 *   XML parser - process Gutenprint XML data with mxml.
 *
 *   Copyright 2002-2003 Roger Leigh (rleigh@debian.org)
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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#if defined(HAVE_VARARGS_H) && !defined(HAVE_STDARG_H)
#include <varargs.h>
#else
#include <stdarg.h>
#endif

typedef struct
{
  char *name;
  stp_xml_parse_func parse_func;
} stpi_xml_parse_registry;

static stp_list_t *stpi_xml_registry;

static stp_list_t *stpi_xml_preloads;

static const char *
xml_registry_namefunc(const void *item)
{
  const stpi_xml_parse_registry *xmlp = (const stpi_xml_parse_registry *) item;
  return xmlp->name;
}

static void
xml_registry_freefunc(void *item)
{
  stpi_xml_parse_registry *xmlp = (stpi_xml_parse_registry *) item;
  stp_free(xmlp->name);
  stp_free(xmlp);
}

static const char *
xml_preload_namefunc(const void *item)
{
  return (const char *) item;
}

static void
xml_preload_freefunc(void *item)
{
  stp_free(item);
}

void
stp_register_xml_parser(const char *name, stp_xml_parse_func parse_func)
{
  stpi_xml_parse_registry *xmlp;
  stp_list_item_t *item = stp_list_get_item_by_name(stpi_xml_registry, name);
  if (item)
    xmlp = (stpi_xml_parse_registry *) stp_list_item_get_data(item);
  else
    {
      xmlp = stp_malloc(sizeof(stpi_xml_parse_registry));
      xmlp->name = stp_strdup(name);
      stp_list_item_create(stpi_xml_registry, NULL, xmlp);
    }
  xmlp->parse_func = parse_func;
}

void
stp_unregister_xml_parser(const char *name)
{
  stp_list_item_t *item = stp_list_get_item_by_name(stpi_xml_registry, name);
  if (item)
    stp_list_item_destroy(stpi_xml_registry, item);
}

void
stp_register_xml_preload(const char *filename)
{
  stp_list_item_t *item = stp_list_get_item_by_name(stpi_xml_preloads, filename);
  if (!item)
    {
      char *the_filename = stp_strdup(filename);
      stp_list_item_create(stpi_xml_preloads, NULL, the_filename);
    }
}

void
stp_unregister_xml_preload(const char *name)
{
  stp_list_item_t *item = stp_list_get_item_by_name(stpi_xml_preloads, name);
  if (item)
    stp_list_item_destroy(stpi_xml_preloads, item);
}


static void stpi_xml_process_gutenprint(stp_mxml_node_t *gutenprint, const char *file);

static char *saved_locale;                 /* Saved LC_ALL */
static int xml_is_initialised;                 /* Flag for init */

void
stp_xml_preinit(void)
{
  static int xml_is_preinitialized = 0;
  if (!xml_is_preinitialized)
    {
      stpi_xml_registry = stp_list_create();
      stp_list_set_freefunc(stpi_xml_registry, xml_registry_freefunc);
      stp_list_set_namefunc(stpi_xml_registry, xml_registry_namefunc);
      stpi_xml_preloads = stp_list_create();
      stp_list_set_freefunc(stpi_xml_preloads, xml_preload_freefunc);
      stp_list_set_namefunc(stpi_xml_preloads, xml_preload_namefunc);
    }
}    

/*
 * Call before using any of the static functions in this file.  All
 * public functions should call this before using any mxml
 * functions.
 */
void
stp_xml_init(void)
{
  stp_deprintf(STP_DBG_XML, "stp_xml_init: entering at level %d\n",
	       xml_is_initialised);
  if (xml_is_initialised >= 1)
    {
      xml_is_initialised++;
      return;
    }

  /* Set some locale facets to "C" */
#ifdef HAVE_LOCALE_H
  saved_locale = stp_strdup(setlocale(LC_ALL, NULL));
  stp_deprintf(STP_DBG_XML, "stp_xml_init: saving locale %s\n", saved_locale);
  setlocale(LC_ALL, "C");
#endif

  xml_is_initialised = 1;
}

/*
 * Call after using any of the static functions in this file.  All
 * public functions should call this after using any mxml functions.
 */
void
stp_xml_exit(void)
{
  stp_deprintf(STP_DBG_XML, "stp_xml_exit: entering at level %d\n",
	       xml_is_initialised);
  if (xml_is_initialised > 1) /* don't restore original state */
    {
      xml_is_initialised--;
      return;
    }
  else if (xml_is_initialised < 1)
    return;

  /* Restore locale */
#ifdef HAVE_LOCALE_H
  stp_deprintf(STP_DBG_XML, "stp_xml_init: restoring locale %s\n", saved_locale);
  setlocale(LC_ALL, saved_locale);
  stp_free(saved_locale);
  saved_locale = NULL;
#endif
  xml_is_initialised = 0;
}

void
stp_xml_parse_file_named(const char *name)
{
  stp_list_t *file_list = stpi_list_files_on_data_path(name); /* List of XML files */
  stp_list_item_t *item;                 /* Pointer to current list item */
  item = stp_list_get_start(file_list);
  while (item)
    {
      stp_deprintf(STP_DBG_XML,
		   "stp_xml_parse_file_named: source file: %s\n",
		   (const char *) stp_list_item_get_data(item));
      stp_xml_parse_file((const char *) stp_list_item_get_data(item));
      item = stp_list_item_next(item);
    }
  stp_list_destroy(file_list);
}
  

/*
 * Read all available XML files.
 */
int
stp_xml_init_defaults(void)
{
  stp_list_item_t *item;                 /* Pointer to current list item */

  stp_xml_init();

  /* Parse each XML file */
  item = stp_list_get_start(stpi_xml_preloads);
  while (item)
    {
      stp_deprintf(STP_DBG_XML, "stp_xml_init_defaults: source file: %s\n",
		   (const char *) stp_list_item_get_data(item));
      stp_xml_parse_file_named((const char *) stp_list_item_get_data(item));
      item = stp_list_item_next(item);
    }
  stp_list_destroy(stpi_xml_preloads);

  stp_xml_exit();

  return 0;
}


/*
 * Parse a single XML file.
 */
int
stp_xml_parse_file(const char *file) /* File to parse */
{
  stp_mxml_node_t *doc;
  stp_mxml_node_t *cur;
  FILE *fp;

  stp_deprintf(STP_DBG_XML, "stp_xml_parse_file: reading  `%s'...\n", file);

  fp = fopen(file, "r");
  if (!fp)
    {
      stp_erprintf("stp_xml_parse_file: unable to open %s: %s\n", file,
		   strerror(errno));
      return 1;
    }

  stp_xml_init();

  doc = stp_mxmlLoadFile(NULL, fp, STP_MXML_NO_CALLBACK);
  fclose(fp);

  cur = doc->child;
  while (cur &&
	 (cur->type != STP_MXML_ELEMENT ||
	  (strcmp(cur->value.element.name, "gutenprint") != 0 &&
	   strcmp(cur->value.element.name, "gimp-print") != 0)))
    cur = cur->next;

  if (cur == NULL || cur->type != STP_MXML_ELEMENT)
    {
      stp_erprintf("stp_xml_parse_file: %s: parse error\n", file);
      stp_mxmlDelete(doc);
      return 1;
    }

  if (strcmp(cur->value.element.name, "gutenprint") != 0 &&
      strcmp(cur->value.element.name, "gimp-print") != 0)
    {
      stp_erprintf
	("XML file of the wrong type, root node is %s != (gutenprint || gimp-print)",
	 cur->value.element.name);
      stp_mxmlDelete(doc);
      return 1;
    }

  /* The XML file was read and is the right format */

  stpi_xml_process_gutenprint(cur, file);
  stp_mxmlDelete(doc);

  stp_xml_exit();

  return 0;
}

/*
 * Convert a text string into an integer.
 */
long
stp_xmlstrtol(const char *textval)
{
  long val; /* The value to return */
  val = strtol(textval, (char **)NULL, 0);

  return val;
}

/*
 * Convert a text string into an unsigned int.
 */
unsigned long
stp_xmlstrtoul(const char *textval)
{
  unsigned long val; /* The value to return */
  val = strtoul(textval, (char **)NULL, 0);

  return val;
}

/*
 * Convert a text string into a double.
 */
double
stp_xmlstrtod(const char *textval)
{
  double val; /* The value to return */
  val = strtod(textval, (char **)NULL);

  return val;
}

/*
 * Convert an encoded text string into a raw.
 */
stp_raw_t *
stp_xmlstrtoraw(const char *textval)
{
  size_t tcount;
  stp_raw_t *raw;
  unsigned char *answer;
  unsigned char *aptr;
  if (! textval || *textval == 0)
    return NULL;
  tcount = strlen(textval);
  raw = stp_zalloc(sizeof(stp_raw_t));
  answer = stp_malloc(tcount + 1); /* Worst case -- we may not need it all */
  aptr = answer;
  raw->data = answer;
  while (*textval)
    {
      if (*textval != '\\')
	{
	  *aptr++ = *textval++;
	  raw->bytes++;
	}
      else
	{
	  textval++;
	  if (textval[0] >= '0' && textval[0] <= '3' &&
	      textval[1] >= '0' && textval[1] <= '7' &&
	      textval[2] >= '0' && textval[2] <= '7')
	    {
	      *aptr++ = (((textval[0] - '0') << 6) +
			 ((textval[1] - '0') << 3) +
			 ((textval[2] - '0') << 0));
	      raw->bytes++;
	      textval += 3;
	    }
	  else if (textval[0] == '\0' || textval[1] == '\0' || textval[2] == '\0')
	    break;
	  else
	    textval += 3;
	}
    }
  *aptr = '\0';
  return raw;
}

char *
stp_rawtoxmlstr(const stp_raw_t *raw)
{
  if (raw && raw->bytes > 0)
    {
      int i;
      const unsigned char *data = (const unsigned char *) (raw->data);
      char *answer = stp_malloc((raw->bytes * 4) + 1); /* \012 */
      unsigned char *aptr = (unsigned char *) answer;
      for (i = 0; i < raw->bytes; i++)
	{
	  if (data[i] > ' ' && data[i] < '\177' && data[i] != '\\' &&
	      data[i] != '<' && data[i] != '>' && data[i] != '&')
	    *aptr++ = data[i];
	  else
	    {
	      *aptr++ = '\\';
	      *aptr++ = '0' + ((data[i] & '\300') >> 6);
	      *aptr++ = '0' + ((data[i] & '\070') >> 3);
	      *aptr++ = '0' + ((data[i] & '\007') >> 0);
	    }
	}
      *aptr = '\0';
      return answer;
    }
  return NULL;
}

char *
stp_strtoxmlstr(const char *str)
{
  if (str && strlen(str) > 0)
    {
      int i;
      int bytes = strlen(str);
      const unsigned char *data = (const unsigned char *) (str);
      char *answer = stp_malloc((bytes * 4) + 1); /* "\012" is worst case */
      unsigned char *aptr = (unsigned char *) answer;
      for (i = 0; i < bytes; i++)
	{
	  if (data[i] > ' ' && data[i] < '\177' && data[i] != '\\' &&
	      data[i] != '<' && data[i] != '>' && data[i] != '&')
	    *aptr++ = data[i];
	  else
	    {
	      *aptr++ = '\\';
	      *aptr++ = '0' + ((data[i] & '\300') >> 6);
	      *aptr++ = '0' + ((data[i] & '\070') >> 3);
	      *aptr++ = '0' + ((data[i] & '\007') >> 0);
	    }
	}
      *aptr = '\0';
      return answer;
    }
  return NULL;
}

void
stp_prtraw(const stp_raw_t *raw, FILE *fp)
{
  if (raw && raw->bytes > 0)
    {
      int i;
      const unsigned char *data = (const unsigned char *) (raw->data);
      for (i = 0; i < raw->bytes; i++)
	{
	  if (data[i] > ' ' && data[i] < '\177' && data[i] != '\\' &&
	      data[i] != '<' && data[i] != '>' && data[i] != '&')
	    fputc(data[i], fp);
	  else
	    {
	      fputc('\\', fp);
	      fputc('0' + ((data[i] & '\300') >> 6), fp);
	      fputc('0' + ((data[i] & '\070') >> 3), fp);
	      fputc('0' + ((data[i] & '\007') >> 0), fp);
	    }
	}
    }
}


/*
 * Find a node in an XML tree.  This function takes an xmlNodePtr,
 * followed by a NULL-terminated list of nodes which are required.
 * For example stp_xml_get_node(myroot, "gutenprint", "dither") will
 * return the first dither node in the tree.  Additional dither nodes
 * cannot be accessed with this function.
 */
stp_mxml_node_t *
stp_xml_get_node(stp_mxml_node_t *xmlroot, ...)
{
  stp_mxml_node_t *child;
  va_list ap;
  const char *target = NULL;

  va_start(ap, xmlroot);

  child = xmlroot;
  target = va_arg(ap, const char *);

  while (target && child)
    {
      child = stp_mxmlFindElement(child, child, target, NULL, NULL, STP_MXML_DESCEND);
      target = va_arg(ap, const char *);
    }
  va_end(ap);
  return child;
}

static void
stpi_xml_process_node(stp_mxml_node_t *node, const char *file)
{
  stp_list_item_t *item =
    stp_list_get_item_by_name(stpi_xml_registry, node->value.element.name);
  if (item)
    {
      stpi_xml_parse_registry *xmlp =
	(stpi_xml_parse_registry *) stp_list_item_get_data(item);
      (xmlp->parse_func)(node, file);
    }
}

/*
 * Parse the <gutenprint> root node.
 */
static void
stpi_xml_process_gutenprint(stp_mxml_node_t *cur, const char *file) /* The node to parse */
{
  stp_mxml_node_t *child;                       /* Child node pointer */

  child = cur->child;
  while (child)
    {
      /* process nodes with corresponding parser */
      if (child->type == STP_MXML_ELEMENT)
	stpi_xml_process_node(child, file);
      child = child->next;
    }
}

/*
 * Create a basic gutenprint XML document tree root
 */
stp_mxml_node_t *
stp_xmldoc_create_generic(void)
{
  stp_mxml_node_t *doc;
  stp_mxml_node_t *rootnode;

  /* Create the XML tree */
  doc = stp_mxmlNewElement(NULL, "?xml");
  stp_mxmlElementSetAttr(doc, "version", "1.0");

  rootnode = stp_mxmlNewElement(doc, "gutenprint");
  stp_mxmlElementSetAttr
    (rootnode, "xmlns", "http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0");
  stp_mxmlElementSetAttr
    (rootnode, "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
  stp_mxmlElementSetAttr
    (rootnode, "xsi:schemaLocation",
     "http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gutenprint.xsd");

  return doc;
}



