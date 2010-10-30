/*
 * "$Id: string-list.c,v 1.19 2005/06/15 01:13:41 rlk Exp $"
 *
 *   Print plug-in driver utility functions for the GIMP.
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
#include <string.h>

static void
free_list_element(void *item)
{
  stp_param_string_t *string = (stp_param_string_t *) (item);
  stp_free((char *) string->name);
  stp_free((char *) string->text);
  stp_free(string);
}

static const char *
namefunc(const void *item)
{
  const stp_param_string_t *string = (const stp_param_string_t *) (item);
  return string->name;
}

static void *
copyfunc(const void *item)
{
  const stp_param_string_t *string = (const stp_param_string_t *) (item);
  stp_param_string_t *new_string = stp_malloc(sizeof(stp_param_string_t));
  new_string->name = stp_strdup(string->name);
  new_string->text = stp_strdup(string->text);
  return new_string;
}

static const char *
long_namefunc(const void *item)
{
  const stp_param_string_t *string = (const stp_param_string_t *) (item);
  return string->text;
}

stp_string_list_t *
stp_string_list_create(void)
{
  stp_list_t *ret = stp_list_create();
  stp_list_set_freefunc(ret, free_list_element);
  stp_list_set_namefunc(ret, namefunc);
  stp_list_set_copyfunc(ret, copyfunc);
  stp_list_set_long_namefunc(ret, long_namefunc);
  return (stp_string_list_t *) ret;
}

void
stp_string_list_destroy(stp_string_list_t *list)
{
  stp_list_destroy((stp_list_t *) list);
}

stp_param_string_t *
stp_string_list_param(const stp_string_list_t *list, size_t element)
{
  stp_list_item_t *answer =
    stp_list_get_item_by_index((const stp_list_t *)list, element);
  if (answer)
    return (stp_param_string_t *) stp_list_item_get_data(answer);
  else
    return NULL;
}

stp_param_string_t *
stp_string_list_find(const stp_string_list_t *list, const char *name)
{
  stp_list_item_t *answer =
    stp_list_get_item_by_name((const stp_list_t *)list, name);
  if (answer)
    return (stp_param_string_t *) stp_list_item_get_data(answer);
  else
    return NULL;
}

size_t
stp_string_list_count(const stp_string_list_t *list)
{
  return stp_list_get_length((const stp_list_t *)list);
}

stp_string_list_t *
stp_string_list_create_copy(const stp_string_list_t *list)
{
  return (stp_string_list_t *) stp_list_copy((const stp_list_t *)list);
}

stp_string_list_t *
stp_string_list_create_from_params(const stp_param_string_t *list,
				   size_t count)
{
  size_t i = 0;
  stp_string_list_t *retval = stp_string_list_create();
  for (i = 0; i < count; i++)
    stp_string_list_add_string(retval, list[i].name, list[i].text);
  return retval;
}

void
stp_string_list_add_string(stp_string_list_t *list,
			   const char *name,
			   const char *text)
{
  stp_param_string_t *new_string = stp_malloc(sizeof(stp_param_string_t));
  new_string->name = stp_strdup(name);
  new_string->text = stp_strdup(text);
  stp_list_item_create((stp_list_t *) list, NULL, new_string);
}

void
stp_string_list_remove_string(stp_string_list_t *list,
			      const char *name)
{
  stp_list_item_t *item =
    stp_list_get_item_by_name((const stp_list_t *) list, name);
  if (item)
    stp_list_item_destroy((stp_list_t *) list, item);
}

int
stp_string_list_is_present(const stp_string_list_t *list,
			   const char *value)
{
  if (list && value &&
      stp_list_get_item_by_name((const stp_list_t *) list, value))
    return 1;
  else
    return 0;
}
