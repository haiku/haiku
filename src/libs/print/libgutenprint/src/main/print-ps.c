/*
 * "$Id: print-ps.c,v 1.104 2011/03/04 13:05:08 rlk Exp $"
 *
 *   Print plug-in Adobe PostScript driver for the GIMP.
 *
 *   Copyright 1997-2002 Michael Sweet (mike@easysw.com) and
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
#include <time.h>
#include <string.h>
#include <math.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include "xmlppd.h"

#ifdef _MSC_VER
#define strncasecmp(s,t,n) _strnicmp(s,t,n)
#define strcasecmp(s,t) _stricmp(s,t)
#endif

/*
 * Local variables...
 */

static char *m_ppd_file = NULL;
static stp_mxml_node_t *m_ppd = NULL;


/*
 * Local functions...
 */

static void	ps_hex(const stp_vars_t *, unsigned short *, int);
static void	ps_ascii85(const stp_vars_t *, unsigned short *, int, int);

static const stp_parameter_t the_parameters[] =
{
  {
    "PPDFile", N_("PPDFile"), "Color=Yes,Category=Basic Printer Setup",
    N_("PPD File"),
    STP_PARAMETER_TYPE_FILE, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "PageSize", N_("Page Size"), "Color=No,Category=Basic Printer Setup",
    N_("Size of the paper being printed to"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "ModelName", N_("Model Name"), "Color=Yes,Category=Basic Printer Setup",
    N_("PPD File Model Name"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_INTERNAL, 0, 0, STP_CHANNEL_NONE, 0, 0
  },
  {
    "PrintingMode", N_("Printing Mode"), "Color=Yes,Category=Core Parameter",
    N_("Printing Output Mode"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
};

static const int the_parameter_count =
sizeof(the_parameters) / sizeof(const stp_parameter_t);

static int
ps_option_to_param(stp_parameter_t *param, stp_mxml_node_t *option)
{
  const char *group_text = stp_mxmlElementGetAttr(option, "grouptext");

  if (group_text != NULL)
    param->category = group_text;
  else
    param->category = NULL;

  param->text = stp_mxmlElementGetAttr(option, "text");
  param->help = stp_mxmlElementGetAttr(option, "text");
  if (stp_mxmlElementGetAttr(option, "stptype"))
    {
      const char *default_value = stp_mxmlElementGetAttr(option, "default");
      double stp_default_value = strtod(stp_mxmlElementGetAttr(option, "stpdefault"), 0);
      double lower_bound = strtod(stp_mxmlElementGetAttr(option, "stplower"), NULL);
      double upper_bound = strtod(stp_mxmlElementGetAttr(option, "stpupper"), NULL);
      param->p_type = atoi(stp_mxmlElementGetAttr(option, "stptype"));
      param->is_mandatory = atoi(stp_mxmlElementGetAttr(option, "stpmandatory"));
      param->p_class = atoi(stp_mxmlElementGetAttr(option, "stpclass"));
      param->p_level = atoi(stp_mxmlElementGetAttr(option, "stplevel"));
      param->channel = (unsigned char) atoi(stp_mxmlElementGetAttr(option, "stpchannel"));
      param->read_only = 0;
      param->is_active = 1;
      param->verify_this_parameter = 1;
      param->name = stp_mxmlElementGetAttr(option, "stpname");
      stp_deprintf(STP_DBG_PS,
		   "Gutenprint parameter %s type %d mandatory %d class %d level %d channel %d default %s %f",
		   param->name, param->p_type, param->is_mandatory,
		   param->p_class, param->p_level, param->channel,
		   default_value, stp_default_value);
      switch (param->p_type)
	{
	case STP_PARAMETER_TYPE_DOUBLE:
	  param->deflt.dbl = stp_default_value;
	  param->bounds.dbl.upper = upper_bound;
	  param->bounds.dbl.lower = lower_bound;
	  stp_deprintf(STP_DBG_PS, " %.3f %.3f %.3f\n",
		       param->deflt.dbl, param->bounds.dbl.upper,
		       param->bounds.dbl.lower);
	  break;
	case STP_PARAMETER_TYPE_DIMENSION:
	  param->deflt.dimension = atoi(default_value);
	  param->bounds.dimension.upper = (int) upper_bound;
	  param->bounds.dimension.lower = (int) lower_bound;
	  stp_deprintf(STP_DBG_PS, " %d %d %d\n",
		       param->deflt.dimension, param->bounds.dimension.upper,
		       param->bounds.dimension.lower);
	  break;
	case STP_PARAMETER_TYPE_INT:
	  param->deflt.integer = atoi(default_value);
	  param->bounds.integer.upper = (int) upper_bound;
	  param->bounds.integer.lower = (int) lower_bound;
	  stp_deprintf(STP_DBG_PS, " %d %d %d\n",
		       param->deflt.integer, param->bounds.integer.upper,
		       param->bounds.integer.lower);
	  break;
	case STP_PARAMETER_TYPE_BOOLEAN:
	  param->deflt.boolean = strcasecmp(default_value, "true") == 0 ? 1 : 0;
	  stp_deprintf(STP_DBG_PS, " %d\n", param->deflt.boolean);
	  break;
	default:
	  stp_deprintf(STP_DBG_PS, "\n");
	  break;
	}
    }
  else
    {
      const char *ui = stp_mxmlElementGetAttr(option, "ui");
      param->name = stp_mxmlElementGetAttr(option, "name");
      if (strcasecmp(ui, "Boolean") == 0)
	param->p_type = STP_PARAMETER_TYPE_BOOLEAN;
      else
	param->p_type = STP_PARAMETER_TYPE_STRING_LIST;
      if (strcmp(param->name, "PageSize") == 0)
	param->p_class = STP_PARAMETER_CLASS_CORE;
      else
	param->p_class = STP_PARAMETER_CLASS_FEATURE;
      param->p_level = STP_PARAMETER_LEVEL_BASIC;
      param->is_mandatory = 1;
      param->is_active = 1;
      param->channel = -1;
      param->verify_this_parameter = 1;
      param->read_only = 0;
    }

  return 0;
}

/*
 * 'ps_parameters()' - Return the parameter values for the given parameter.
 */

static int
ppd_whitespace_callback(stp_mxml_node_t *node, int where)
{
  return 0;
}

static int
check_ppd_file(const stp_vars_t *v)
{
  const char *ppd_file = stp_get_file_parameter(v, "PPDFile");

  if (ppd_file == NULL || ppd_file[0] == 0)
    {
      stp_dprintf(STP_DBG_PS, v, "Empty PPD file\n");
      return 0;
    }
  else if (m_ppd_file && strcmp(m_ppd_file, ppd_file) == 0)
    {
      stp_dprintf(STP_DBG_PS, v, "Not replacing PPD file %s\n", m_ppd_file);
      return 1;
    }
  else
    {
      stp_dprintf(STP_DBG_PS, v, "Replacing PPD file %s with %s\n",
		  m_ppd_file ? m_ppd_file : "(null)",
		  ppd_file ? ppd_file : "(null)");
      if (m_ppd != NULL)
	stp_mxmlDelete(m_ppd);
      m_ppd = NULL;

      if (m_ppd_file)
	stp_free(m_ppd_file);
      m_ppd_file = NULL;

      if ((m_ppd = stpi_xmlppd_read_ppd_file(ppd_file)) == NULL)
	{
	  stp_eprintf(v, "Unable to open PPD file %s\n", ppd_file);
	  return 0;
	}
      if (stp_get_debug_level() & STP_DBG_PS)
	{
	  char *ppd_stuff = stp_mxmlSaveAllocString(m_ppd, ppd_whitespace_callback);
	  stp_dprintf(STP_DBG_PS, v, "%s", ppd_stuff);
	  stp_free(ppd_stuff);
	}

      m_ppd_file = stp_strdup(ppd_file);
      return 1;
    }
}    
  

static stp_parameter_list_t
ps_list_parameters(const stp_vars_t *v)
{
  stp_parameter_list_t *ret = stp_parameter_list_create();
  stp_mxml_node_t *option;
  int i;
  int status = check_ppd_file(v);
  stp_dprintf(STP_DBG_PS, v, "Adding parameters from %s (%d)\n",
	      m_ppd_file ? m_ppd_file : "(null)", status);

  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(the_parameters[i]));

  if (status)
    {
      int num_options = stpi_xmlppd_find_option_count(m_ppd);
      stp_dprintf(STP_DBG_PS, v, "Found %d parameters\n", num_options);
      for (i=0; i < num_options; i++)
	{
	  /* MEMORY LEAK!!! */
	  stp_parameter_t *param = stp_malloc(sizeof(stp_parameter_t));
	  option = stpi_xmlppd_find_option_index(m_ppd, i);
	  if (option)
	    {
	      ps_option_to_param(param, option);
	      if (param->p_type != STP_PARAMETER_TYPE_INVALID &&
		  strcmp(param->name, "PageRegion") != 0 &&
		  strcmp(param->name, "PageSize") != 0)
		{
		  stp_dprintf(STP_DBG_PS, v, "Adding parameter %s %s\n",
			      param->name, param->text);
		  stp_parameter_list_add_param(ret, param);
		}
	      else
		stp_free(param);
	    }
	}
    }
  return ret;
}

static void
ps_parameters_internal(const stp_vars_t *v, const char *name,
		       stp_parameter_t *description)
{
  int		i;
  stp_mxml_node_t *option;
  int status = 0;
  int num_choices;
  const char *defchoice;

  description->p_type = STP_PARAMETER_TYPE_INVALID;
  description->deflt.str = 0;
  description->is_active = 0;

  if (name == NULL)
    return;

  status = check_ppd_file(v);

  for (i = 0; i < the_parameter_count; i++)
  {
    if (strcmp(name, the_parameters[i].name) == 0)
      {
	stp_fill_parameter_settings(description, &(the_parameters[i]));
	if (strcmp(name, "PPDFile") == 0)
	  description->is_active = 1;
	else if (strcmp(name, "ModelName") == 0)
	  {
	    const char *nickname;
	    description->bounds.str = stp_string_list_create();
	    if (m_ppd && stp_mxmlElementGetAttr(m_ppd, "nickname"))
	      nickname = stp_mxmlElementGetAttr(m_ppd, "nickname");
	    else
	      nickname = _("None; please provide a PPD file");
	    stp_string_list_add_string(description->bounds.str,
				       nickname, nickname);
	    description->deflt.str = nickname;
	    description->is_active = 1;
	    return;
	  }
	else if (strcmp(name, "PrintingMode") == 0)
	  {
	    if (! m_ppd || strcmp(stp_mxmlElementGetAttr(m_ppd, "color"), "1") == 0)
	      {
		description->bounds.str = stp_string_list_create();
		stp_string_list_add_string
		  (description->bounds.str, "Color", _("Color"));
		stp_string_list_add_string
		  (description->bounds.str, "BW", _("Black and White"));
		description->deflt.str =
		  stp_string_list_param(description->bounds.str, 0)->name;
		description->is_active = 1;
	      }
	    else
	      description->is_active = 0;
	    return;
	  }
      }
  }

  if (!status && strcmp(name, "PageSize") != 0)
    return;
  if ((option = stpi_xmlppd_find_option_named(m_ppd, name)) == NULL)
  {
    if (strcmp(name, "PageSize") == 0)
      {
	/* Provide a default set of page sizes */
	description->bounds.str = stp_string_list_create();
	stp_string_list_add_string
	  (description->bounds.str, "Letter", _("Letter"));
	stp_string_list_add_string
	  (description->bounds.str, "A4", _("A4"));
	stp_string_list_add_string
	  (description->bounds.str, "Custom", _("Custom"));
	description->deflt.str =
	  stp_string_list_param(description->bounds.str, 0)->name;
	description->is_active = 1;
	return;
      }
    else
      {
	char *tmp = stp_malloc(strlen(name) + 4);
	strcpy(tmp, "Stp");
	strncat(tmp, name, strlen(name) + 3);
	if ((option = stpi_xmlppd_find_option_named(m_ppd, tmp)) == NULL)
	  {
	    stp_dprintf(STP_DBG_PS, v, "no parameter %s", name);
	    stp_free(tmp);
	    return;
	  }
	stp_free(tmp);
      }
  }

  ps_option_to_param(description, option);
  if (description->p_type != STP_PARAMETER_TYPE_STRING_LIST)
    return;
  num_choices = atoi(stp_mxmlElementGetAttr(option, "num_choices"));
  defchoice = stp_mxmlElementGetAttr(option, "default");
  description->bounds.str = stp_string_list_create();

  stp_dprintf(STP_DBG_PS, v, "describe parameter %s, output name=[%s] text=[%s] category=[%s] choices=[%d] default=[%s]\n",
	      name, description->name, description->text,
	      description->category, num_choices, defchoice);

  /* Describe all choices for specified option. */
  for (i=0; i < num_choices; i++)
  {
    stp_mxml_node_t *choice = stpi_xmlppd_find_choice_index(option, i);
    const char *choice_name = stp_mxmlElementGetAttr(choice, "name");
    const char *choice_text = stp_mxmlElementGetAttr(choice, "text");
    stp_string_list_add_string(description->bounds.str, choice_name, choice_text);
    stp_dprintf(STP_DBG_PS, v, "    parameter %s, choice %d [%s] [%s]",
		name, i, choice_name, choice_text);
    if (strcmp(choice_name, defchoice) == 0)
      {
	stp_dprintf(STP_DBG_PS, v,
		    "        parameter %s, choice %d [%s] DEFAULT\n",
		    name, i, choice_name);
	description->deflt.str = choice_name;
      }
  }

  if (!description->deflt.str)
    {
	stp_dprintf(STP_DBG_PS, v,
		    "        parameter %s, defaulting to [%s]",
		    name, stp_string_list_param(description->bounds.str, 0)->name);
	description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
    }
  if (stp_string_list_count(description->bounds.str) > 0)
    description->is_active = 1;
  return;
}

static void
ps_parameters(const stp_vars_t *v, const char *name,
	      stp_parameter_t *description)
{
#ifdef HAVE_LOCALE_H
  char *locale = stp_strdup(setlocale(LC_ALL, NULL));
  setlocale(LC_ALL, "C");
#endif
  ps_parameters_internal(v, name, description);
#ifdef HAVE_LOCALE_H
  setlocale(LC_ALL, locale);
  stp_free(locale);
#endif
}

/*
 * 'ps_media_size()' - Return the size of the page.
 */

static void
ps_media_size_internal(const stp_vars_t *v,		/* I */
		       int  *width,		/* O - Width in points */
		       int  *height)		/* O - Height in points */
{
  const char *pagesize = stp_get_string_parameter(v, "PageSize");
  int status = check_ppd_file(v);
  if (!pagesize)
    pagesize = "";

  stp_dprintf(STP_DBG_PS, v,
	      "ps_media_size(%d, \'%s\', \'%s\', %p, %p)\n",
	      stp_get_model_id(v), m_ppd_file, pagesize,
	      (void *) width, (void *) height);

  stp_default_media_size(v, width, height);

  if (status)
    {
      stp_mxml_node_t *paper = stpi_xmlppd_find_page_size(m_ppd, pagesize);
      if (paper)
	{
	  *width = atoi(stp_mxmlElementGetAttr(paper, "width"));
	  *height = atoi(stp_mxmlElementGetAttr(paper, "height"));
	}
      else
	{
	  *width = 0;
	  *height = 0;
	}
    }

  stp_dprintf(STP_DBG_PS, v, "dimensions %d %d\n", *width, *height);
  return;
}

static void
ps_media_size(const stp_vars_t *v, int *width, int *height)
{
#ifdef HAVE_LOCALE_H
  char *locale = stp_strdup(setlocale(LC_ALL, NULL));
  setlocale(LC_ALL, "C");
#endif
  ps_media_size_internal(v, width, height);
#ifdef HAVE_LOCALE_H
  setlocale(LC_ALL, locale);
  stp_free(locale);
#endif
}

/*
 * 'ps_imageable_area()' - Return the imageable area of the page.
 */

static void
ps_imageable_area_internal(const stp_vars_t *v,      /* I */
			   int  use_max_area, /* I - Use maximum area */
			   int  *left,	/* O - Left position in points */
			   int  *right,	/* O - Right position in points */
			   int  *bottom, /* O - Bottom position in points */
			   int  *top)	/* O - Top position in points */
{
  int width, height;
  const char *pagesize = stp_get_string_parameter(v, "PageSize");
  if (!pagesize)
    pagesize = "";

  /* Set some defaults. */
  ps_media_size_internal(v, &width, &height);
  *left   = 0;
  *right  = width;
  *top    = 0;
  *bottom = height;

  if (check_ppd_file(v))
    {
      stp_mxml_node_t *paper = stpi_xmlppd_find_page_size(m_ppd, pagesize);
      if (paper)
	{
	  double pleft = atoi(stp_mxmlElementGetAttr(paper, "left"));
	  double pright = atoi(stp_mxmlElementGetAttr(paper, "right"));
	  double ptop = atoi(stp_mxmlElementGetAttr(paper, "top"));
	  double pbottom = atoi(stp_mxmlElementGetAttr(paper, "bottom"));
	  stp_dprintf(STP_DBG_PS, v, "size=l %f r %f b %f t %f h %d w %d\n",
		      pleft, pright, pbottom, ptop, height, width);
	  *left = (int) pleft;
	  *right = (int) pright;
	  *top = height - (int) ptop;
	  *bottom = height - (int) pbottom;
	  stp_dprintf(STP_DBG_PS, v, ">>>> l %d r %d b %d t %d h %d w %d\n",
		      *left, *right, *bottom, *top, height, width);
	}
    }

  if (use_max_area)
  {
    if (*left > 0)
      *left = 0;
    if (*right < width)
      *right = width;
    if (*top > 0)
      *top = 0;
    if (*bottom < height)
      *bottom = height;
  }

  stp_dprintf(STP_DBG_PS, v, "pagesize %s max_area=%d l %d r %d b %d t %d h %d w %d\n",
	      pagesize ? pagesize : "(null)",
	      use_max_area, *left, *right, *bottom, *top, width, height);

  return;
}

static void
ps_imageable_area(const stp_vars_t *v,      /* I */
                  int  *left,		/* O - Left position in points */
                  int  *right,		/* O - Right position in points */
                  int  *bottom,		/* O - Bottom position in points */
                  int  *top)		/* O - Top position in points */
{
#ifdef HAVE_LOCALE_H
  char *locale = stp_strdup(setlocale(LC_ALL, NULL));
  setlocale(LC_ALL, "C");
#endif
  ps_imageable_area_internal(v, 0, left, right, bottom, top);
#ifdef HAVE_LOCALE_H
  setlocale(LC_ALL, locale);
  stp_free(locale);
#endif
}

static void
ps_maximum_imageable_area(const stp_vars_t *v,      /* I */
			  int  *left,	/* O - Left position in points */
			  int  *right,	/* O - Right position in points */
			  int  *bottom,	/* O - Bottom position in points */
			  int  *top)	/* O - Top position in points */
{
#ifdef HAVE_LOCALE_H
  char *locale = stp_strdup(setlocale(LC_ALL, NULL));
  setlocale(LC_ALL, "C");
#endif
  ps_imageable_area_internal(v, 1, left, right, bottom, top);
#ifdef HAVE_LOCALE_H
  setlocale(LC_ALL, locale);
  stp_free(locale);
#endif
}

static void
ps_limit(const stp_vars_t *v,  		/* I */
	 int *width,
	 int *height,
	 int *min_width,
	 int *min_height)
{
  *width =	INT_MAX;
  *height =	INT_MAX;
  *min_width =	1;
  *min_height =	1;
}

/*
 * This is really bogus...
 */
static void
ps_describe_resolution_internal(const stp_vars_t *v, int *x, int *y)
{
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  *x = -1;
  *y = -1;
  if (resolution)
    sscanf(resolution, "%dx%d", x, y);
  return;
}

static void
ps_describe_resolution(const stp_vars_t *v, int *x, int *y)
{
#ifdef HAVE_LOCALE_H
  char *locale = stp_strdup(setlocale(LC_ALL, NULL));
  setlocale(LC_ALL, "C");
#endif
  ps_describe_resolution_internal(v, x, y);
#ifdef HAVE_LOCALE_H
  setlocale(LC_ALL, locale);
  stp_free(locale);
#endif
}

static const char *
ps_describe_output(const stp_vars_t *v)
{
  const char *print_mode = stp_get_string_parameter(v, "PrintingMode");
  const char *input_image_type = stp_get_string_parameter(v, "InputImageType");
  if (print_mode && strcmp(print_mode, "Color") == 0)
    {
      if (input_image_type && (strcmp(input_image_type, "CMYK") == 0 ||
			       strcmp(input_image_type, "KCMY") == 0))
	return "CMYK";
      else
	return "RGB";
    }
  else
    return "Whitescale";
}

static stp_string_list_t *
ps_external_options(const stp_vars_t *v)
{
  stp_parameter_list_t param_list = ps_list_parameters(v);
  stp_string_list_t *answer;
  char *tmp;
  char *ppd_name = NULL;
  int i;
#ifdef HAVE_LOCALE_H
  char *locale;
#endif
  if (! param_list)
    return NULL;
  answer = stp_string_list_create();
#ifdef HAVE_LOCALE_H
  locale = stp_strdup(setlocale(LC_ALL, NULL));
  setlocale(LC_ALL, "C");
#endif
  for (i = 0; i < stp_parameter_list_count(param_list); i++)
    {
      const stp_parameter_t *param = stp_parameter_list_param(param_list, i);
      stp_parameter_t desc;
      stp_describe_parameter(v, param->name, &desc);
      if (desc.is_active)
	{
	  stp_mxml_node_t *option;
	  if (m_ppd &&
	      (option = stpi_xmlppd_find_option_named(m_ppd, desc.name)) == NULL)
	    {
	      ppd_name = stp_malloc(strlen(desc.name) + 4);
	      strcpy(ppd_name, "Stp");
	      strncat(ppd_name, desc.name, strlen(desc.name) + 3);
	      if ((option = stpi_xmlppd_find_option_named(m_ppd, ppd_name)) == NULL)
		{
		  stp_dprintf(STP_DBG_PS, v, "no parameter %s", desc.name);
		  STP_SAFE_FREE(ppd_name);
		}
	    }
	  switch (desc.p_type)
	    {
	    case STP_PARAMETER_TYPE_STRING_LIST:
	      if (stp_get_string_parameter(v, desc.name) &&
		  strcmp(stp_get_string_parameter(v, desc.name),
			 desc.deflt.str))
		{
		  stp_dprintf(STP_DBG_PS, v, "Adding string parameter %s (%s): %s %s\n",
			      desc.name, ppd_name ? ppd_name : "(null)",
			      stp_get_string_parameter(v, desc.name),
			      desc.deflt.str);
		  stp_string_list_add_string(answer,
					     ppd_name ? ppd_name : desc.name,
					     stp_get_string_parameter(v, desc.name));
		}
	      break;
	    case STP_PARAMETER_TYPE_INT:
	      if (stp_get_int_parameter(v, desc.name) != desc.deflt.integer)
		{
		  stp_dprintf(STP_DBG_PS, v, "Adding integer parameter %s (%s): %d %d\n",
			      desc.name, ppd_name ? ppd_name : "(null)",
			      stp_get_int_parameter(v, desc.name),
			      desc.deflt.integer);
		  stp_asprintf(&tmp, "%d", stp_get_int_parameter(v, desc.name));
		  stp_string_list_add_string(answer,
					     ppd_name ? ppd_name : desc.name,
					     tmp);
		  stp_free(tmp);
		}
	      break;
	    case STP_PARAMETER_TYPE_BOOLEAN:
	      if (stp_get_boolean_parameter(v, desc.name) != desc.deflt.boolean)
		{
		  stp_dprintf(STP_DBG_PS, v, "Adding boolean parameter %s (%s): %d %d\n",
			      desc.name, ppd_name ? ppd_name : "(null)",
			      stp_get_boolean_parameter(v, desc.name),
			      desc.deflt.boolean);
		  stp_asprintf(&tmp, "%s",
			       stp_get_boolean_parameter(v, desc.name) ?
			       "True" : "False");
		  stp_string_list_add_string(answer,
					     ppd_name ? ppd_name : desc.name,
					     tmp);
		  stp_free(tmp);
		}
	      break;
	    case STP_PARAMETER_TYPE_DOUBLE:
	      if (fabs(stp_get_float_parameter(v, desc.name) - desc.deflt.dbl) > .00001)
		{
		  stp_dprintf(STP_DBG_PS, v, "Adding float parameter %s (%s): %.3f %.3f\n",
			      desc.name, ppd_name ? ppd_name : "(null)",
			      stp_get_float_parameter(v, desc.name),
			      desc.deflt.dbl);
		  stp_asprintf(&tmp, "%.3f",
			       stp_get_float_parameter(v, desc.name));
		  stp_string_list_add_string(answer,
					     ppd_name ? ppd_name : desc.name,
					     tmp);
		  stp_free(tmp);
		}
	      break;
	    case STP_PARAMETER_TYPE_DIMENSION:
	      if (stp_get_dimension_parameter(v, desc.name) !=
		  desc.deflt.dimension)
		{
		  stp_dprintf(STP_DBG_PS, v, "Adding dimension parameter %s (%s): %d %d\n",
			      desc.name, ppd_name ? ppd_name : "(null)",
			      stp_get_dimension_parameter(v, desc.name),
			      desc.deflt.dimension);
		  stp_asprintf(&tmp, "%d",
			       stp_get_dimension_parameter(v, desc.name));
		  stp_string_list_add_string(answer,
					     ppd_name ? ppd_name : desc.name,
					     tmp);
		  stp_free(tmp);
		}
	      break;
	    default:
	      break;
	    }
	  STP_SAFE_FREE(ppd_name);
	}
      stp_parameter_description_destroy(&desc);
    }
#ifdef HAVE_LOCALE_H
  setlocale(LC_ALL, locale);
  stp_free(locale);
#endif
  return answer;
}

/*
 * 'ps_print_device_settings()' - output postscript code from PPD into the
 * postscript stream.
 */

static void
ps_print_device_settings(stp_vars_t *v)
{
  int i;
  stp_parameter_list_t param_list = ps_list_parameters(v);
  if (! param_list)
    return;
  stp_puts("%%BeginSetup\n", v);
  for (i = 0; i < stp_parameter_list_count(param_list); i++)
    {
      const stp_parameter_t *param = stp_parameter_list_param(param_list, i);
      stp_parameter_t desc;
      stp_describe_parameter(v, param->name, &desc);
      if (desc.is_active)
	{
	  switch (desc.p_type)
	    {
	    case STP_PARAMETER_TYPE_STRING_LIST:
	    case STP_PARAMETER_TYPE_BOOLEAN:
	      {
		const char *val=NULL;
		const char *defval=NULL;

		/* If this is a bool parameter, set val to "True" or "False" - otherwise fetch from string parameter. */
		if(desc.p_type==STP_PARAMETER_TYPE_BOOLEAN)
		  {
		    val=stp_get_boolean_parameter(v,desc.name) ? "True" : "False";
		    defval=desc.deflt.boolean ? "True" : "False";
		  }
		else
		  {
		    val=stp_get_string_parameter(v,desc.name);
		    defval=desc.deflt.str;
		  }

		/* We only include the option's code if it's set to a value other than the default. */
		if(val && defval && (strcmp(val,defval)!=0))
		  {
		    if(m_ppd)
		      {
			/* If we have a PPD xml tree we hunt for the appropriate "option" and "choice"... */
			stp_mxml_node_t *node=m_ppd;
			node=stp_mxmlFindElement(node,node, "option", "name", desc.name, STP_MXML_DESCEND);
			if(node)
			  {
			    node=stp_mxmlFindElement(node,node, "choice", "name", val, STP_MXML_DESCEND);
			    if(node)
			      {
				if(node->child && node->child->value.opaque && (strlen(node->child->value.opaque)>1))
				  {
				    /* If we have opaque data for the child, we use %%BeginFeature and copy the code verbatim. */
				    stp_puts("[{\n", v);
				    stp_zprintf(v, "%%%%BeginFeature: *%s %s\n", desc.name, val);
				    if(node->child->value.opaque)
				      stp_puts(node->child->value.opaque,v);
				    stp_puts("\n%%EndFeature\n", v);
				    stp_puts("} stopped cleartomark\n", v);
				  }
				else
				  {
				    /* If we don't have code, we use %%IncludeFeature instead. */
				    stp_puts("[{\n", v);
				    stp_zprintf(v, "%%%%IncludeFeature: *%s %s\n", desc.name, val);
				    if(node->child->value.opaque)
				      stp_puts(node->child->value.opaque,v);
				    stp_puts("} stopped cleartomark\n", v);
				  }
			      }
			  }
		      }
		  }
	      }
	      break;
	    case STP_PARAMETER_TYPE_INT:
	      if(stp_get_int_parameter(v,desc.name)!=desc.deflt.integer)
		{
		  stp_puts("[{\n", v);
		  stp_zprintf(v, "%%%%IncludeFeature: *%s %d\n", desc.name,
			      stp_get_int_parameter(v, desc.name));
		  stp_puts("} stopped cleartomark\n", v);
		}
	      break;
	    case STP_PARAMETER_TYPE_DOUBLE:
	      if(stp_get_float_parameter(v,desc.name)!=desc.deflt.dbl)
		{
		  stp_puts("[{\n", v);
		  stp_zprintf(v, "%%%%IncludeFeature: *%s %f\n", desc.name,
			      stp_get_float_parameter(v, desc.name));
		  stp_puts("} stopped cleartomark\n", v);
		}
	      break;
	    case STP_PARAMETER_TYPE_DIMENSION:
	      if(stp_get_dimension_parameter(v,desc.name)!=desc.deflt.dimension)
		{
		  stp_puts("[{\n", v);
		  stp_zprintf(v, "%%%%IncludeFeature: *%s %d\n", desc.name,
			      stp_get_dimension_parameter(v, desc.name));
		  stp_puts("} stopped cleartomark\n", v);
		}
	      break;
	    default:
	      break;
	    }
	}
      stp_parameter_description_destroy(&desc);
    }
  stp_puts("%%EndSetup\n", v);
  stp_parameter_list_destroy(param_list);
}

/*
 * 'ps_print()' - Print an image to a PostScript printer.
 */

static int
ps_print_internal(stp_vars_t *v, stp_image_t *image)
{
  int		status = 1;
  int		model = stp_get_model_id(v);
  const char    *print_mode = stp_get_string_parameter(v, "PrintingMode");
  const char *input_image_type = stp_get_string_parameter(v, "InputImageType");
  unsigned short *out = NULL;
  int		top = stp_get_top(v);
  int		left = stp_get_left(v);
  int		y;		/* Looping vars */
  int		page_left,	/* Left margin of page */
		page_right,	/* Right margin of page */
		page_top,	/* Top of page */
		page_bottom,	/* Bottom of page */
		page_width,	/* Width of page */
		page_height,	/* Height of page */
		paper_width,	/* Width of physical page */
		paper_height,	/* Height of physical page */
		out_width,	/* Width of image on page */
		out_height,	/* Height of image on page */
		out_channels,	/* Output bytes per pixel */
		out_ps_height,	/* Output height (Level 2 output) */
		out_offset;	/* Output offset (Level 2 output) */
  time_t	curtime;	/* Current time of day */
  unsigned	zero_mask;
  int           image_height,
		image_width;
  int		color_out = 0;
  int		cmyk_out = 0;

  if (print_mode && strcmp(print_mode, "Color") == 0)
    color_out = 1;
  if (color_out &&
      input_image_type && (strcmp(input_image_type, "CMYK") == 0 ||
			   strcmp(input_image_type, "KCMY") == 0))
    cmyk_out = 1;

  stp_image_init(image);

 /*
  * Compute the output size...
  */

  out_width = stp_get_width(v);
  out_height = stp_get_height(v);

  ps_imageable_area_internal(v, 0, &page_left, &page_right, &page_bottom, &page_top);
  ps_media_size_internal(v, &paper_width, &paper_height);
  page_width = page_right - page_left;
  page_height = page_bottom - page_top;

  image_height = stp_image_height(image);
  image_width = stp_image_width(image);

 /*
  * Output a standard PostScript header with DSC comments...
  */

  curtime = time(NULL);

  top = paper_height - top;

  stp_dprintf(STP_DBG_PS, v,
	      "out_width = %d, out_height = %d\n", out_width, out_height);
  stp_dprintf(STP_DBG_PS, v,
	      "page_left = %d, page_right = %d, page_bottom = %d, page_top = %d\n",
	      page_left, page_right, page_bottom, page_top);
  stp_dprintf(STP_DBG_PS, v, "left = %d, top = %d\n", left, top);
  stp_dprintf(STP_DBG_PS, v, "page_width = %d, page_height = %d\n",
	      page_width, page_height);

  stp_dprintf(STP_DBG_PS, v, "bounding box l %d b %d r %d t %d\n",
	      page_left, paper_height - page_bottom,
	      page_right, paper_height - page_top);

  stp_puts("%!PS-Adobe-3.0\n", v);
#ifdef HAVE_CONFIG_H
  stp_zprintf(v, "%%%%Creator: %s/Gutenprint %s (%s)\n",
	      stp_image_get_appname(image), VERSION, RELEASE_DATE);
#else
  stp_zprintf(v, "%%%%Creator: %s/Gutenprint\n", stp_image_get_appname(image));
#endif
  stp_zprintf(v, "%%%%CreationDate: %s", ctime(&curtime));
  stp_zprintf(v, "%%%%BoundingBox: %d %d %d %d\n",
	      page_left, paper_height - page_bottom,
	      page_right, paper_height - page_top);
  stp_puts("%%DocumentData: Clean7Bit\n", v);
  stp_zprintf(v, "%%%%LanguageLevel: %d\n", model + 1);
  stp_puts("%%Pages: 1\n", v);
  stp_puts("%%Orientation: Portrait\n", v);
  stp_puts("%%EndComments\n", v);

  ps_print_device_settings(v);

 /*
  * Output the page...
  */

  stp_puts("%%Page: 1 1\n", v);
  stp_puts("gsave\n", v);

  stp_zprintf(v, "%d %d translate\n", left, top);

  /* Force locale to "C", because decimal numbers in Postscript must
     always be printed with a decimal point rather than the
     locale-specific setting. */

  stp_zprintf(v, "%.3f %.3f scale\n",
	      (double)out_width / ((double)image_width),
	      (double)out_height / ((double)image_height));

  stp_channel_reset(v);
  stp_channel_add(v, 0, 0, 1.0);
  if (color_out)
    {
      stp_channel_add(v, 1, 0, 1.0);
      stp_channel_add(v, 2, 0, 1.0);
      if (cmyk_out)
	{
	  stp_channel_add(v, 3, 0, 1.0);
	  stp_set_string_parameter(v, "STPIOutputType", "CMYK");
	}
      else
	stp_set_string_parameter(v, "STPIOutputType", "RGB");
    }
  else
    stp_set_string_parameter(v, "STPIOutputType", "Whitescale");

  stp_set_boolean_parameter(v, "SimpleGamma", 1);

  out_channels = stp_color_init(v, image, 256);

  if (model == 0)
  {
    stp_zprintf(v, "/picture %d string def\n", image_width * out_channels);

    stp_zprintf(v, "%d %d 8\n", image_width, image_height);

    stp_puts("[ 1 0 0 -1 0 1 ]\n", v);

    if (cmyk_out)
      stp_puts("{currentfile picture readhexstring pop} false 4 colorimage\n", v);
    else if (color_out)
      stp_puts("{currentfile picture readhexstring pop} false 3 colorimage\n", v);
    else
      stp_puts("{currentfile picture readhexstring pop} image\n", v);

    for (y = 0; y < image_height; y ++)
    {
      if (stp_color_get_row(v, image, y, &zero_mask))
	{
	  status = 2;
	  break;
	}

      out = stp_channel_get_input(v);

      /* Convert from KCMY to CMYK */
      if (cmyk_out)
	{
	  int x;
	  unsigned short *pos = out;
	  for (x = 0; x < image_width; x++, pos += 4)
	    {
	      unsigned short p0 = pos[0];
	      pos[0] = pos[1];
	      pos[1] = pos[2];
	      pos[2] = pos[3];
	      pos[3] = p0;
	    }
	}
      ps_hex(v, out, image_width * out_channels);
    }
  }
  else
  {
    unsigned short *tmp_buf =
      stp_malloc(sizeof(unsigned short) * (image_width * out_channels + 4));
    if (cmyk_out)
      stp_puts("/DeviceCMYK setcolorspace\n", v);
    else if (color_out)
      stp_puts("/DeviceRGB setcolorspace\n", v);
    else
      stp_puts("/DeviceGray setcolorspace\n", v);

    stp_puts("<<\n", v);
    stp_puts("\t/ImageType 1\n", v);

    stp_zprintf(v, "\t/Width %d\n", image_width);
    stp_zprintf(v, "\t/Height %d\n", image_height);
    stp_puts("\t/BitsPerComponent 8\n", v);

    if (cmyk_out)
      stp_puts("\t/Decode [ 0 1 0 1 0 1 0 1 ]\n", v);
    else if (color_out)
      stp_puts("\t/Decode [ 0 1 0 1 0 1 ]\n", v);
    else
      stp_puts("\t/Decode [ 0 1 ]\n", v);

    stp_puts("\t/DataSource currentfile /ASCII85Decode filter\n", v);

    if ((image_width * 72 / out_width) < 100)
      stp_puts("\t/Interpolate true\n", v);

    stp_puts("\t/ImageMatrix [ 1 0 0 -1 0 1 ]\n", v);

    stp_puts(">>\n", v);
    stp_puts("image\n", v);

    for (y = 0, out_offset = 0; y < image_height; y ++)
    {
      unsigned short *where;
      /* FIXME!!! */
      if (stp_color_get_row(v, image, y /*, out + out_offset */ , &zero_mask))
	{
	  status = 2;
	  break;
	}
      out = stp_channel_get_input(v);
      if (out_offset > 0)
	{
	  memcpy(tmp_buf + out_offset, out,
		 image_width * out_channels * sizeof(unsigned short));
	  where = tmp_buf;
	}
      else
	where = out;

      /* Convert from KCMY to CMYK */
      if (cmyk_out)
	{
	  int x;
	  unsigned short *pos = where;
	  for (x = 0; x < image_width; x++, pos += 4)
	    {
	      unsigned short p0 = pos[0];
	      pos[0] = pos[1];
	      pos[1] = pos[2];
	      pos[2] = pos[3];
	      pos[3] = p0;
	    }
	}

      out_ps_height = out_offset + image_width * out_channels;

      if (y < (image_height - 1))
      {
	ps_ascii85(v, where, out_ps_height & ~3, 0);
        out_offset = out_ps_height & 3;
      }
      else
      {
        ps_ascii85(v, where, out_ps_height, 1);
        out_offset = 0;
      }

      if (out_offset > 0)
        memcpy(tmp_buf, where + out_ps_height - out_offset,
	       out_offset * sizeof(unsigned short));
    }
    stp_free(tmp_buf);
  }
  stp_image_conclude(image);

  stp_puts("grestore\n", v);
  stp_puts("showpage\n", v);
  stp_puts("%%Trailer\n", v);
  stp_puts("%%EOF\n", v);
  return status;
}

static int
ps_print(const stp_vars_t *v, stp_image_t *image)
{
  int status;
#ifdef HAVE_LOCALE_H
  char *locale;
#endif
  stp_vars_t *nv = stp_vars_create_copy(v);
  stp_prune_inactive_options(nv);
  if (!stp_verify(nv))
    {
      stp_eprintf(nv, "Print options not verified; cannot print.\n");
      return 0;
    }
#ifdef HAVE_LOCALE_H
  locale = stp_strdup(setlocale(LC_ALL, NULL));
  setlocale(LC_ALL, "C");
#endif
  status = ps_print_internal(nv, image);
#ifdef HAVE_LOCALE_H
  setlocale(LC_ALL, locale);
  stp_free(locale);
#endif
  stp_vars_destroy(nv);
  return status;
}


/*
 * 'ps_hex()' - Print binary data as a series of hexadecimal numbers.
 */

static void
ps_hex(const stp_vars_t *v,	/* I - File to print to */
       unsigned short   *data,	/* I - Data to print */
       int              length)	/* I - Number of bytes to print */
{
  int		col;		/* Current column */
  static const char	*hex = "0123456789ABCDEF";

  col = 0;
  while (length > 0)
  {
    unsigned char pixel = (*data & 0xff00) >> 8;
   /*
    * Put the hex chars out to the file; note that we don't use stp_zprintf()
    * for speed reasons...
    */

    stp_putc(hex[pixel >> 4], v);
    stp_putc(hex[pixel & 15], v);

    data ++;
    length --;

    col += 2;
    if (col >= 72)
    {
      col = 0;
      stp_putc('\n', v);
    }
  }

  if (col > 0)
    stp_putc('\n', v);
}


/*
 * 'ps_ascii85()' - Print binary data as a series of base-85 numbers.
 */

static void
ps_ascii85(const stp_vars_t *v,	/* I - File to print to */
	   unsigned short *data,	/* I - Data to print */
	   int            length,	/* I - Number of bytes to print */
	   int            last_line)	/* I - Last line of raster data? */
{
  int		i;			/* Looping var */
  unsigned	b;			/* Binary data word */
  unsigned char	c[5];			/* ASCII85 encoded chars */
  static int	column = 0;		/* Current column */

#define OUTBUF_SIZE 4096
  unsigned char outbuffer[OUTBUF_SIZE+10];
  int outp=0;

  while (length > 3)
  {
    unsigned char d0 = (data[0] & 0xff00) >> 8;
    unsigned char d1 = (data[1] & 0xff00) >> 8;
    unsigned char d2 = (data[2] & 0xff00) >> 8;
    unsigned char d3 = (data[3] & 0xff00) >> 8;
    b = (((((d0 << 8) | d1) << 8) | d2) << 8) | d3;

    if (b == 0)
    {
      outbuffer[outp++]='z';
      column ++;
    }
    else
    {
      outbuffer[outp+4] = (b % 85) + '!';
      b /= 85;
      outbuffer[outp+3] = (b % 85) + '!';
      b /= 85;
      outbuffer[outp+2] = (b % 85) + '!';
      b /= 85;
      outbuffer[outp+1] = (b % 85) + '!';
      b /= 85;
      outbuffer[outp] = b + '!';

      outp+=5;
      column += 5;
    }

    if (column > 72)
    {
      outbuffer[outp++]='\n';
      column = 0;
    }

    if(outp>=OUTBUF_SIZE)
	{
		stp_zfwrite((const char *)outbuffer, outp, 1, v);
		outp=0;
	}

    data += 4;
    length -= 4;
  }

  if(outp)
    stp_zfwrite((const char *)outbuffer, outp, 1, v);

  if (last_line)
  {
    if (length > 0)
    {
      for (b = 0, i = length; i > 0; b = (b << 8) | data[0], data ++, i --);

      c[4] = (b % 85) + '!';
      b /= 85;
      c[3] = (b % 85) + '!';
      b /= 85;
      c[2] = (b % 85) + '!';
      b /= 85;
      c[1] = (b % 85) + '!';
      b /= 85;
      c[0] = b + '!';

      stp_zfwrite((const char *)c, length + 1, 1, v);
    }

    stp_puts("~>\n", v);
    column = 0;
  }
}


static const stp_printfuncs_t print_ps_printfuncs =
{
  ps_list_parameters,
  ps_parameters,
  ps_media_size,
  ps_imageable_area,
  ps_maximum_imageable_area,
  ps_limit,
  ps_print,
  ps_describe_resolution,
  ps_describe_output,
  stp_verify_printer_params,
  NULL,
  NULL,
  ps_external_options
};


static stp_family_t print_ps_module_data =
  {
    &print_ps_printfuncs,
    NULL
  };


static int
print_ps_module_init(void)
{
  return stp_family_register(print_ps_module_data.printer_list);
}


static int
print_ps_module_exit(void)
{
  return stp_family_unregister(print_ps_module_data.printer_list);
}


/* Module header */
#define stp_module_version print_ps_LTX_stp_module_version
#define stp_module_data print_ps_LTX_stp_module_data

stp_module_version_t stp_module_version = {0, 0};

stp_module_t stp_module_data =
  {
    "ps",
    VERSION,
    "Postscript family driver",
    STP_MODULE_CLASS_FAMILY,
    NULL,
    print_ps_module_init,
    print_ps_module_exit,
    (void *) &print_ps_module_data
  };

