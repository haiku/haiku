/*
 * "$Id: color.c,v 1.11 2010/08/04 00:33:56 rlk Exp $"
 *
 *   Gimp-Print color module interface.
 *
 *   Copyright (C) 2003  Roger Leigh (rleigh@debian.org)
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
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include <string.h>
#include <stdlib.h>


static const char* stpi_color_namefunc(const void *item);
static const char* stpi_color_long_namefunc(const void *item);

static stp_list_t *color_list = NULL;


static int
stpi_init_color_list(void)
{
  if(color_list)
    stp_list_destroy(color_list);
  color_list = stp_list_create();
  stp_list_set_namefunc(color_list, stpi_color_namefunc);
  stp_list_set_long_namefunc(color_list, stpi_color_long_namefunc);
  /* stp_list_set_sortfunc(color_list, stpi_color_sortfunc); */

  return 0;
}

static inline void
check_list(void)
{
  if (color_list == NULL)
    {
      stp_erprintf("No color drivers found: "
		   "are STP_DATA_PATH and STP_MODULE_PATH correct?\n");
      stpi_init_color_list();
    }
}



int
stp_color_count(void)
{
  if (color_list == NULL)
    {
      stp_erprintf("No color modules found: "
		   "is STP_MODULE_PATH correct?\n");
      stpi_init_color_list();
    }
  return stp_list_get_length(color_list);
}

#define CHECK_COLOR(c) STPI_ASSERT(c != NULL, NULL)

const stp_color_t *
stp_get_color_by_index(int idx)
{
  stp_list_item_t *color;

  check_list();

  color = stp_list_get_item_by_index(color_list, idx);
  if (color == NULL)
    return NULL;
  return (const stp_color_t *) stp_list_item_get_data(color);
}


static const char *
stpi_color_namefunc(const void *item)
{
  const stp_color_t *color = (const stp_color_t *) item;
  CHECK_COLOR(color);
  return color->short_name;
}


static const char *
stpi_color_long_namefunc(const void *item)
{
  const stp_color_t *color = (const stp_color_t *) item;
  CHECK_COLOR(color);
  return color->long_name;
}


const char *
stp_color_get_name(const stp_color_t *c)
{
  const stp_color_t *val = (const stp_color_t *) c;
  CHECK_COLOR(val);
  return val->short_name;
}

const char *
stp_color_get_long_name(const stp_color_t *c)
{
  const stp_color_t *val = (const stp_color_t *) c;
  CHECK_COLOR(val);
  return gettext(val->long_name);
}


static const stp_colorfuncs_t *
stpi_get_colorfuncs(const stp_color_t *c)
{
  const stp_color_t *val = (const stp_color_t *) c;
  CHECK_COLOR(val);
  return val->colorfuncs;
}


const stp_color_t *
stp_get_color_by_name(const char *name)
{
  stp_list_item_t *color;

  check_list();

  color = stp_list_get_item_by_name(color_list, name);
  if (!color)
    return NULL;
  return (const stp_color_t *) stp_list_item_get_data(color);
}

const stp_color_t *
stp_get_color_by_colorfuncs(stp_colorfuncs_t *colorfuncs)
{
  stp_list_item_t *color_item;
  stp_color_t *color;

  check_list();

  color_item = stp_list_get_start(color_list);
  while (color_item)
    {
      color = (stp_color_t *) stp_list_item_get_data(color_item);
      if (color->colorfuncs == colorfuncs)
	return color;
      color_item = stp_list_item_next(color_item);
    }
  return NULL;
}


int
stp_color_init(stp_vars_t *v,
	       stp_image_t *image,
	       size_t steps)
{
  const stp_colorfuncs_t *colorfuncs =
    stpi_get_colorfuncs(stp_get_color_by_name(stp_get_color_conversion(v)));
  return colorfuncs->init(v, image, steps);
}

int
stp_color_get_row(stp_vars_t *v,
		  stp_image_t *image,
		  int row,
		  unsigned *zero_mask)
{
  const stp_colorfuncs_t *colorfuncs =
    stpi_get_colorfuncs(stp_get_color_by_name(stp_get_color_conversion(v)));
  return colorfuncs->get_row(v, image, row, zero_mask);
}

stp_parameter_list_t
stp_color_list_parameters(const stp_vars_t *v)
{
  const stp_colorfuncs_t *colorfuncs =
    stpi_get_colorfuncs(stp_get_color_by_name(stp_get_color_conversion(v)));
  return colorfuncs->list_parameters(v);
}

void
stp_color_describe_parameter(const stp_vars_t *v, const char *name,
			     stp_parameter_t *description)
{
  const stp_colorfuncs_t *colorfuncs =
    stpi_get_colorfuncs(stp_get_color_by_name(stp_get_color_conversion(v)));
  colorfuncs->describe_parameter(v, name, description);
}


int
stp_color_register(const stp_color_t *color)
{
  if (color_list == NULL)
    {
      stpi_init_color_list();
      stp_deprintf(STP_DBG_COLORFUNC,
		   "stpi_color_register(): initialising color_list...\n");
    }

  CHECK_COLOR(color);

  if (color)
    {
      /* Add new color algorithm if it does not already exist */
      if (stp_get_color_by_name(color->short_name) == NULL)
	{
	  stp_deprintf
	    (STP_DBG_COLORFUNC,
	     "stpi_color_register(): registered colour module \"%s\"\n",
	     color->short_name);
	  stp_list_item_create(color_list, NULL, color);
	}
    }

  return 0;
}

int
stp_color_unregister(const stp_color_t *color)
{
  stp_list_item_t *color_item;
  stp_color_t *color_data;

  if (color_list == NULL)
    {
      stpi_init_color_list();
      stp_deprintf
	(STP_DBG_COLORFUNC,
	 "stpi_family_unregister(): initialising color_list...\n");
    }

  CHECK_COLOR(color);

  color_item = stp_list_get_start(color_list);
  while (color_item)
    {
      color_data = (stp_color_t *) stp_list_item_get_data(color_item);
      if (strcmp(color->short_name, color_data->short_name) == 0)
	{
	  stp_deprintf
	    (STP_DBG_COLORFUNC,
	     "stpi_color_unregister(): unregistered colour module \"%s\"\n",
	     color->short_name);
	  stp_list_item_destroy(color_list, color_item);
	  break;
	}
      color_item = stp_list_item_next(color_item);
    }

  return 0;
}

