/*
 * "$Id: print-vars.c,v 1.91 2010/12/05 21:38:15 rlk Exp $"
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

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <math.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <string.h>
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include "generic-options.h"

typedef struct
{
  char *name;
  stp_parameter_type_t typ;
  stp_parameter_activity_t active;
  union
  {
    int ival;
    int bval;
    double dval;
    stp_curve_t *cval;
    stp_array_t *aval;
    stp_raw_t rval;
  } value;
} value_t;

struct stp_compdata
{
  char *name;
  stp_copy_data_func_t copyfunc;
  stp_free_data_func_t freefunc;
  void *data;
};

struct stp_vars			/* Plug-in variables */
{
  char *driver;			/* Name of printer "driver" */
  char *color_conversion;       /* Color module in use */
  int	left;			/* Offset from left-upper corner, points */
  int	top;			/* ... */
  int	width;			/* Width of the image, points */
  int	height;			/* ... */
  int	page_width;		/* Width of page in points */
  int	page_height;		/* Height of page in points */
  stp_list_t *params[STP_PARAMETER_TYPE_INVALID];
  stp_list_t *internal_data;
  void (*outfunc)(void *data, const char *buffer, size_t bytes);
  void *outdata;
  void (*errfunc)(void *data, const char *buffer, size_t bytes);
  void *errdata;
  int verified;			/* Ensure that params are OK! */
};

static int standard_vars_initialized = 0;


void
stp_parameter_description_destroy(stp_parameter_t *desc)
{
  switch (desc->p_type)
    {
    case STP_PARAMETER_TYPE_CURVE:
      if (desc->bounds.curve)
	stp_curve_destroy(desc->bounds.curve);
      desc->bounds.curve = NULL;
      break;
    case STP_PARAMETER_TYPE_ARRAY:
      if (desc->bounds.array)
	stp_array_destroy(desc->bounds.array);
      desc->bounds.array = NULL;
      break;
    case STP_PARAMETER_TYPE_STRING_LIST:
      if (desc->bounds.str)
	stp_string_list_destroy(desc->bounds.str);
      desc->bounds.str = NULL;
      break;
    default:
      break;
    }
}
static stp_vars_t default_vars;

#define CHECK_VARS(v) STPI_ASSERT(v, NULL)

static const char *
value_namefunc(const void *item)
{
  const value_t *v = (const value_t *) (item);
  return v->name;
}

static void
value_freefunc(void *item)
{
  value_t *v = (value_t *) (item);
  switch (v->typ)
    {
    case STP_PARAMETER_TYPE_STRING_LIST:
    case STP_PARAMETER_TYPE_FILE:
    case STP_PARAMETER_TYPE_RAW:
      stp_free((void *) v->value.rval.data);
      break;
    case STP_PARAMETER_TYPE_CURVE:
      if (v->value.cval)
	stp_curve_destroy(v->value.cval);
      break;
    case STP_PARAMETER_TYPE_ARRAY:
      stp_array_destroy(v->value.aval);
      break;
    default:
      break;
    }
  stp_free(v->name);
  stp_free(v);
}

static stp_list_t *
create_vars_list(void)
{
  stp_list_t *ret = stp_list_create();
  stp_list_set_freefunc(ret, value_freefunc);
  stp_list_set_namefunc(ret, value_namefunc);
  return ret;
}

static void
copy_to_raw(stp_raw_t *raw, const void *data, size_t bytes)
{
  char *ndata = NULL;
  if (data)
    {
      ndata = stp_malloc(bytes + 1);
      memcpy(ndata, data, bytes);
      ndata[bytes] = '\0';
    }
  else
    bytes = 0;
  raw->data = (void *) ndata;
  raw->bytes = bytes;
}

static value_t *
value_copy(const void *item)
{
  value_t *ret = stp_malloc(sizeof(value_t));
  const value_t *v = (const value_t *) (item);
  ret->name = stp_strdup(v->name);
  ret->typ = v->typ;
  ret->active = v->active;
  switch (v->typ)
    {
    case STP_PARAMETER_TYPE_CURVE:
      ret->value.cval = stp_curve_create_copy(v->value.cval);
      break;
    case STP_PARAMETER_TYPE_ARRAY:
      ret->value.aval = stp_array_create_copy(v->value.aval);
      break;
    case STP_PARAMETER_TYPE_STRING_LIST:
    case STP_PARAMETER_TYPE_FILE:
    case STP_PARAMETER_TYPE_RAW:
      copy_to_raw(&(ret->value.rval), v->value.rval.data, v->value.rval.bytes);
      break;
    case STP_PARAMETER_TYPE_INT:
    case STP_PARAMETER_TYPE_DIMENSION:
    case STP_PARAMETER_TYPE_BOOLEAN:
      ret->value.ival = v->value.ival;
      break;
    case STP_PARAMETER_TYPE_DOUBLE:
      ret->value.dval = v->value.dval;
      break;
    default:
      break;
    }
  return ret;
}

static stp_list_t *
copy_value_list(const stp_list_t *src)
{
  stp_list_t *ret = create_vars_list();
  const stp_list_item_t *item = stp_list_get_start((const stp_list_t *)src);
  while (item)
    {
      stp_list_item_create(ret, NULL, value_copy(stp_list_item_get_data(item)));
      item = stp_list_item_next(item);
    }
  return ret;
}

static const char *
compdata_namefunc(const void *item)
{
  const compdata_t *cd = (const compdata_t *) (item);
  return cd->name;
}

static void
compdata_freefunc(void *item)
{
  compdata_t *cd = (compdata_t *) (item);
  if (cd->freefunc)
    (cd->freefunc)(cd->data);
  stp_free(cd->name);
  stp_free(cd);
}

static void *
compdata_copyfunc(const void *item)
{
  const compdata_t *cd = (const compdata_t *) (item);
  if (cd->copyfunc)
    return (cd->copyfunc)(cd->data);
  else
    return cd->data;
}

void
stp_allocate_component_data(stp_vars_t *v,
			     const char *name,
			     stp_copy_data_func_t copyfunc,
			     stp_free_data_func_t freefunc,
			     void *data)
{
  compdata_t *cd;
  stp_list_item_t *item;
  CHECK_VARS(v);
  cd = stp_malloc(sizeof(compdata_t));
  item = stp_list_get_item_by_name(v->internal_data, name);
  if (item)
    stp_list_item_destroy(v->internal_data, item);
  cd->name = stp_strdup(name);
  cd->copyfunc = copyfunc;
  cd->freefunc = freefunc;
  cd->data = data;
  stp_list_item_create(v->internal_data, NULL, cd);
}

void
stp_destroy_component_data(stp_vars_t *v, const char *name)
{
  stp_list_item_t *item;
  CHECK_VARS(v);
  item = stp_list_get_item_by_name(v->internal_data, name);
  if (item)
    stp_list_item_destroy(v->internal_data, item);
}

void *
stp_get_component_data(const stp_vars_t *v, const char *name)
{
  stp_list_item_t *item;
  CHECK_VARS(v);
  item = stp_list_get_item_by_name(v->internal_data, name);
  if (item)
    return ((compdata_t *) stp_list_item_get_data(item))->data;
  else
    return NULL;
}

static stp_list_t *
create_compdata_list(void)
{
  stp_list_t *ret = stp_list_create();
  stp_list_set_freefunc(ret, compdata_freefunc);
  stp_list_set_namefunc(ret, compdata_namefunc);
  return ret;
}

static stp_list_t *
copy_compdata_list(const stp_list_t *src)
{
  stp_list_t *ret = create_compdata_list();
  const stp_list_item_t *item = stp_list_get_start(src);
  while (item)
    {
      stp_list_item_create(ret, NULL, compdata_copyfunc(item));
      item = stp_list_item_next(item);
    }
  return ret;
}

static void
initialize_standard_vars(void)
{
  if (!standard_vars_initialized)
    {
      int i;
      for (i = 0; i < STP_PARAMETER_TYPE_INVALID; i++)
	default_vars.params[i] = create_vars_list();
      default_vars.driver = stp_strdup("ps2");
      default_vars.color_conversion = stp_strdup("traditional");
      default_vars.internal_data = create_compdata_list();
      standard_vars_initialized = 1;
    }
}

const stp_vars_t *
stp_default_settings(void)
{
  initialize_standard_vars();
  return (stp_vars_t *) &default_vars;
}

stp_vars_t *
stp_vars_create(void)
{
  int i;
  stp_vars_t *retval = stp_zalloc(sizeof(stp_vars_t));
  initialize_standard_vars();
  for (i = 0; i < STP_PARAMETER_TYPE_INVALID; i++)
    retval->params[i] = create_vars_list();
  retval->internal_data = create_compdata_list();
  stp_vars_copy(retval, (stp_vars_t *)&default_vars);
  return (retval);
}

void
stp_vars_destroy(stp_vars_t *v)
{
  int i;
  CHECK_VARS(v);
  for (i = 0; i < STP_PARAMETER_TYPE_INVALID; i++)
    stp_list_destroy(v->params[i]);
  stp_list_destroy(v->internal_data);
  STP_SAFE_FREE(v->driver);
  STP_SAFE_FREE(v->color_conversion);
  stp_free(v);
}

#define DEF_STRING_FUNCS(s, pre)					\
void									\
pre##_set_##s(stp_vars_t *v, const char *val)				\
{									\
  CHECK_VARS(v);							\
  if (val)								\
    stp_deprintf(STP_DBG_VARS, "set %s to %s (0x%p)\n", #s, val,	\
                 (const void *) v);					\
  else									\
    stp_deprintf(STP_DBG_VARS, "clear %s (0x%p)\n", #s,			\
                 (const void *) v);					\
  if (v->s == val)							\
    return;								\
  STP_SAFE_FREE(v->s);							\
  v->s = stp_strdup(val);						\
  v->verified = 0;							\
}									\
									\
void									\
pre##_set_##s##_n(stp_vars_t *v, const char *val, int n)		\
{									\
  CHECK_VARS(v);							\
  if (v->s == val)							\
    return;								\
  STP_SAFE_FREE(v->s);							\
  v->s = stp_strndup(val, n);						\
  v->verified = 0;							\
}									\
									\
const char *								\
pre##_get_##s(const stp_vars_t *v)					\
{									\
  CHECK_VARS(v);							\
  return v->s;								\
}

#define DEF_FUNCS(s, t, pre)				\
void							\
pre##_set_##s(stp_vars_t *v, t val)			\
{							\
  CHECK_VARS(v);                                        \
  v->verified = 0;					\
  v->s = val;						\
}							\
							\
t							\
pre##_get_##s(const stp_vars_t *v)			\
{							\
  CHECK_VARS(v);                                        \
  return v->s;						\
}

DEF_STRING_FUNCS(driver, stp)
DEF_STRING_FUNCS(color_conversion, stp)
DEF_FUNCS(left, int, stp)
DEF_FUNCS(top, int, stp)
DEF_FUNCS(width, int, stp)
DEF_FUNCS(height, int, stp)
DEF_FUNCS(page_width, int, stp)
DEF_FUNCS(page_height, int, stp)
DEF_FUNCS(outdata, void *, stp)
DEF_FUNCS(errdata, void *, stp)
DEF_FUNCS(outfunc, stp_outfunc_t, stp)
DEF_FUNCS(errfunc, stp_outfunc_t, stp)

void
stp_set_verified(stp_vars_t *v, int val)
{
  CHECK_VARS(v);
  v->verified = val;
}

int
stp_get_verified(const stp_vars_t *v)
{
  CHECK_VARS(v);
  return v->verified;
}

static void
set_default_raw_parameter(stp_list_t *list, const char *parameter,
			  const char *value, size_t bytes, int typ)
{
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  if (value && !item)
    {
      value_t *val = stp_malloc(sizeof(value_t));
      val->name = stp_strdup(parameter);
      val->typ = typ;
      val->active = STP_PARAMETER_DEFAULTED;
      stp_list_item_create(list, NULL, val);
      copy_to_raw(&(val->value.rval), value, bytes);
    }
}

static void
set_raw_parameter(stp_list_t *list, const char *parameter, const char *value,
		  size_t bytes, int typ)
{
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  if (value)
    {
      value_t *val;
      if (item)
	{
	  val = (value_t *) stp_list_item_get_data(item);
	  if (val->active == STP_PARAMETER_DEFAULTED)
	    val->active = STP_PARAMETER_ACTIVE;
	  stp_free((void *) val->value.rval.data);
	}
      else
	{
	  val = stp_malloc(sizeof(value_t));
	  val->name = stp_strdup(parameter);
	  val->typ = typ;
	  val->active = STP_PARAMETER_ACTIVE;
	  stp_list_item_create(list, NULL, val);
	}
      copy_to_raw(&(val->value.rval), value, bytes);
    }
  else if (item)
    stp_list_item_destroy(list, item);
}

void
stp_set_string_parameter_n(stp_vars_t *v, const char *parameter,
			   const char *value, size_t bytes)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_STRING_LIST];
  if (value)
    stp_deprintf(STP_DBG_VARS, "stp_set_string_parameter(0x%p, %s, %s)\n",
		 (const void *) v, parameter, value);
  else
    stp_deprintf(STP_DBG_VARS, "stp_set_string_parameter(0x%p, %s)\n",
		 (const void *) v, parameter);
  set_raw_parameter(list, parameter, value, bytes,
		    STP_PARAMETER_TYPE_STRING_LIST);
  stp_set_verified(v, 0);
}

void
stp_set_string_parameter(stp_vars_t *v, const char *parameter,
			 const char *value)
{
  int byte_count = 0;
  if (value)
      byte_count = strlen(value);
  stp_deprintf(STP_DBG_VARS, "stp_set_string_parameter(0x%p, %s, %s)\n",
	       (const void *) v, parameter, value ? value : "NULL");
  stp_set_string_parameter_n(v, parameter, value, byte_count);
  stp_set_verified(v, 0);
}

void
stp_set_default_string_parameter_n(stp_vars_t *v, const char *parameter,
				   const char *value, size_t bytes)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_STRING_LIST];
  stp_deprintf(STP_DBG_VARS, "stp_set_default_string_parameter(0x%p, %s, %s)\n",
	       (const void *) v, parameter, value ? value : "NULL");
  set_default_raw_parameter(list, parameter, value, bytes,
			    STP_PARAMETER_TYPE_STRING_LIST);
  stp_set_verified(v, 0);
}

void
stp_set_default_string_parameter(stp_vars_t *v, const char *parameter,
				 const char *value)
{
  int byte_count = 0;
  if (value)
    byte_count = strlen(value);
  stp_set_default_string_parameter_n(v, parameter, value, byte_count);
  stp_set_verified(v, 0);
}

void
stp_clear_string_parameter(stp_vars_t *v, const char *parameter)
{
  stp_set_string_parameter(v, parameter, NULL);
}

const char *
stp_get_string_parameter(const stp_vars_t *v, const char *parameter)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_STRING_LIST];
  value_t *val;
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  if (item)
    {
      val = (value_t *) stp_list_item_get_data(item);
      return val->value.rval.data;
    }
  else
    return NULL;
}

void
stp_set_raw_parameter(stp_vars_t *v, const char *parameter,
		      const void *value, size_t bytes)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_RAW];
  set_raw_parameter(list, parameter, value, bytes, STP_PARAMETER_TYPE_RAW);
  stp_set_verified(v, 0);
}

void
stp_set_default_raw_parameter(stp_vars_t *v, const char *parameter,
			      const void *value, size_t bytes)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_RAW];
  set_default_raw_parameter(list, parameter, value, bytes,
			    STP_PARAMETER_TYPE_RAW);
  stp_set_verified(v, 0);
}

void
stp_clear_raw_parameter(stp_vars_t *v, const char *parameter)
{
  stp_set_raw_parameter(v, parameter, NULL, 0);
}

const stp_raw_t *
stp_get_raw_parameter(const stp_vars_t *v, const char *parameter)
{
  const stp_list_t *list = v->params[STP_PARAMETER_TYPE_RAW];
  const value_t *val;
  const stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  if (item)
    {
      val = (const value_t *) stp_list_item_get_data(item);
      return &(val->value.rval);
    }
  else
    return NULL;
}

void
stp_set_file_parameter(stp_vars_t *v, const char *parameter,
		       const char *value)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_FILE];
  size_t byte_count = 0;
  if (value)
    byte_count = strlen(value);
  stp_deprintf(STP_DBG_VARS, "stp_set_file_parameter(0x%p, %s, %s)\n",
	       (const void *) v, parameter, value ? value : "NULL");
  set_raw_parameter(list, parameter, value, byte_count,
		    STP_PARAMETER_TYPE_FILE);
  stp_set_verified(v, 0);
}

void
stp_set_file_parameter_n(stp_vars_t *v, const char *parameter,
			 const char *value, size_t byte_count)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_FILE];
  stp_deprintf(STP_DBG_VARS, "stp_set_file_parameter(0x%p, %s, %s)\n",
	       (const void *) v, parameter, value ? value : "NULL");
  set_raw_parameter(list, parameter, value, byte_count,
		    STP_PARAMETER_TYPE_FILE);
  stp_set_verified(v, 0);
}

void
stp_set_default_file_parameter(stp_vars_t *v, const char *parameter,
			       const char *value)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_FILE];
  size_t byte_count = 0;
  if (value)
    byte_count = strlen(value);
  stp_deprintf(STP_DBG_VARS, "stp_set_default_file_parameter(0x%p, %s, %s)\n",
	       (const void *) v, parameter, value ? value : "NULL");
  set_default_raw_parameter(list, parameter, value, byte_count,
			    STP_PARAMETER_TYPE_FILE);
  stp_set_verified(v, 0);
}

void
stp_set_default_file_parameter_n(stp_vars_t *v, const char *parameter,
				 const char *value, size_t byte_count)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_FILE];
  stp_deprintf(STP_DBG_VARS, "stp_set_default_file_parameter(0x%p, %s, %s)\n",
	       (const void *) v, parameter, value ? value : "NULL");
  set_default_raw_parameter(list, parameter, value, byte_count,
			    STP_PARAMETER_TYPE_FILE);
  stp_set_verified(v, 0);
}

void
stp_clear_file_parameter(stp_vars_t *v, const char *parameter)
{
  stp_set_file_parameter(v, parameter, NULL);
}

const char *
stp_get_file_parameter(const stp_vars_t *v, const char *parameter)
{
  const stp_list_t *list = v->params[STP_PARAMETER_TYPE_FILE];
  const value_t *val;
  const stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  if (item)
    {
      val = (const value_t *) stp_list_item_get_data(item);
      return val->value.rval.data;
    }
  else
    return NULL;
}

void
stp_set_curve_parameter(stp_vars_t *v, const char *parameter,
			const stp_curve_t *curve)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_CURVE];
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  stp_deprintf(STP_DBG_VARS, "stp_set_curve_parameter(0x%p, %s)\n",
	       (const void *) v, parameter);
  if (curve)
    {
      value_t *val;
      if (item)
	{
	  val = (value_t *) stp_list_item_get_data(item);
	  if (val->active == STP_PARAMETER_DEFAULTED)
	    val->active = STP_PARAMETER_ACTIVE;
	  if (val->value.cval)
	    stp_curve_destroy(val->value.cval);
	}
      else
	{
	  val = stp_malloc(sizeof(value_t));
	  val->name = stp_strdup(parameter);
	  val->typ = STP_PARAMETER_TYPE_CURVE;
	  val->active = STP_PARAMETER_ACTIVE;
	  stp_list_item_create(list, NULL, val);
	}
      val->value.cval = stp_curve_create_copy(curve);
    }
  else if (item)
    stp_list_item_destroy(list, item);
  stp_set_verified(v, 0);
}

void
stp_set_default_curve_parameter(stp_vars_t *v, const char *parameter,
				const stp_curve_t *curve)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_CURVE];
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  stp_deprintf(STP_DBG_VARS, "stp_set_default_curve_parameter(0x%p, %s)\n",
	       (const void *) v, parameter);
  if (!item)
    {
      if (curve)
	{
	  value_t *val;
	  val = stp_malloc(sizeof(value_t));
	  val->name = stp_strdup(parameter);
	  val->typ = STP_PARAMETER_TYPE_CURVE;
	  val->active = STP_PARAMETER_DEFAULTED;
	  stp_list_item_create(list, NULL, val);
	  val->value.cval = stp_curve_create_copy(curve);
	}
    }
  stp_set_verified(v, 0);
}

void
stp_clear_curve_parameter(stp_vars_t *v, const char *parameter)
{
  stp_set_curve_parameter(v, parameter, NULL);
}

const stp_curve_t *
stp_get_curve_parameter(const stp_vars_t *v, const char *parameter)
{
  const stp_list_t *list = v->params[STP_PARAMETER_TYPE_CURVE];
  const value_t *val;
  const stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  if (item)
    {
      val = (value_t *) stp_list_item_get_data(item);
      return val->value.cval;
    }
  else
    return NULL;
}

void
stp_set_array_parameter(stp_vars_t *v, const char *parameter,
			const stp_array_t *array)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_ARRAY];
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  stp_deprintf(STP_DBG_VARS, "stp_set_array_parameter(0x%p, %s)\n",
	       (const void *) v, parameter);
  if (array)
    {
      value_t *val;
      if (item)
	{
	  val = (value_t *) stp_list_item_get_data(item);
	  if (val->active == STP_PARAMETER_DEFAULTED)
	    val->active = STP_PARAMETER_ACTIVE;
	  stp_array_destroy(val->value.aval);
	}
      else
	{
	  val = stp_malloc(sizeof(value_t));
	  val->name = stp_strdup(parameter);
	  val->typ = STP_PARAMETER_TYPE_ARRAY;
	  val->active = STP_PARAMETER_ACTIVE;
	  stp_list_item_create(list, NULL, val);
	}
      val->value.aval = stp_array_create_copy(array);
    }
  else if (item)
    stp_list_item_destroy(list, item);
  stp_set_verified(v, 0);
}

void
stp_set_default_array_parameter(stp_vars_t *v, const char *parameter,
				const stp_array_t *array)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_ARRAY];
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  stp_deprintf(STP_DBG_VARS, "stp_set_default_array_parameter(0x%p, %s)\n",
	       (const void *) v, parameter);
  if (!item)
    {
      if (array)
	{
	  value_t *val;
	  val = stp_malloc(sizeof(value_t));
	  val->name = stp_strdup(parameter);
	  val->typ = STP_PARAMETER_TYPE_ARRAY;
	  val->active = STP_PARAMETER_DEFAULTED;
	  stp_list_item_create(list, NULL, val);
	  val->value.aval = stp_array_create_copy(array);
	}
    }
  stp_set_verified(v, 0);
}

void
stp_clear_array_parameter(stp_vars_t *v, const char *parameter)
{
  stp_set_array_parameter(v, parameter, NULL);
}

const stp_array_t *
stp_get_array_parameter(const stp_vars_t *v, const char *parameter)
{
  const stp_list_t *list = v->params[STP_PARAMETER_TYPE_ARRAY];
  const value_t *val;
  const stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  if (item)
    {
      val = (const value_t *) stp_list_item_get_data(item);
      return val->value.aval;
    }
  else
    return NULL;
}

void
stp_set_int_parameter(stp_vars_t *v, const char *parameter, int ival)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_INT];
  value_t *val;
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  stp_deprintf(STP_DBG_VARS, "stp_set_int_parameter(0x%p, %s, %d)\n",
	       (const void *) v, parameter, ival);
  if (item)
    {
      val = (value_t *) stp_list_item_get_data(item);
      if (val->active == STP_PARAMETER_DEFAULTED)
	val->active = STP_PARAMETER_ACTIVE;
    }
  else
    {
      val = stp_malloc(sizeof(value_t));
      val->name = stp_strdup(parameter);
      val->typ = STP_PARAMETER_TYPE_INT;
      val->active = STP_PARAMETER_ACTIVE;
      stp_list_item_create(list, NULL, val);
    }
  val->value.ival = ival;
  stp_set_verified(v, 0);
}

void
stp_set_default_int_parameter(stp_vars_t *v, const char *parameter, int ival)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_INT];
  value_t *val;
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  stp_deprintf(STP_DBG_VARS, "stp_set_default_int_parameter(0x%p, %s, %d)\n",
	       (const void *) v, parameter, ival);
  if (!item)
    {
      val = stp_malloc(sizeof(value_t));
      val->name = stp_strdup(parameter);
      val->typ = STP_PARAMETER_TYPE_INT;
      val->active = STP_PARAMETER_DEFAULTED;
      stp_list_item_create(list, NULL, val);
      val->value.ival = ival;
    }
  stp_set_verified(v, 0);
}

void
stp_clear_int_parameter(stp_vars_t *v, const char *parameter)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_INT];
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  stp_deprintf(STP_DBG_VARS, "stp_clear_int_parameter(0x%p, %s)\n",
	       (const void *) v, parameter);
  if (item)
    stp_list_item_destroy(list, item);
  stp_set_verified(v, 0);
}

int
stp_get_int_parameter(const stp_vars_t *v, const char *parameter)
{
  const stp_list_t *list = v->params[STP_PARAMETER_TYPE_INT];
  const stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  if (item)
    {
      const value_t *val = (const value_t *) stp_list_item_get_data(item);
      return val->value.ival;
    }
  else
    {
      stp_parameter_t desc;
      stp_describe_parameter(v, parameter, &desc);
      if (desc.p_type == STP_PARAMETER_TYPE_INT)
	{
	  int intval = desc.deflt.integer;
	  stp_parameter_description_destroy(&desc);
	  return intval;
	}
      else
	{
	  stp_parameter_description_destroy(&desc);
	  stp_erprintf
	    ("Gutenprint: Attempt to retrieve unset integer parameter %s\n",
	     parameter);
	  return 0;
	}
    }
}

void
stp_set_boolean_parameter(stp_vars_t *v, const char *parameter, int ival)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_BOOLEAN];
  value_t *val;
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  stp_deprintf(STP_DBG_VARS, "stp_set_boolean_parameter(0x%p, %s, %d)\n",
	       (const void *) v, parameter, ival);
  if (item)
    {
      val = (value_t *) stp_list_item_get_data(item);
      if (val->active == STP_PARAMETER_DEFAULTED)
	val->active = STP_PARAMETER_ACTIVE;
    }
  else
    {
      val = stp_malloc(sizeof(value_t));
      val->name = stp_strdup(parameter);
      val->typ = STP_PARAMETER_TYPE_BOOLEAN;
      val->active = STP_PARAMETER_ACTIVE;
      stp_list_item_create(list, NULL, val);
    }
  if (ival)
    val->value.ival = 1;
  else
    val->value.ival = 0;
  stp_set_verified(v, 0);
}

void
stp_set_default_boolean_parameter(stp_vars_t *v, const char *parameter,
				  int ival)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_BOOLEAN];
  value_t *val;
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  stp_deprintf(STP_DBG_VARS, "stp_set_default_boolean_parameter(0x%p, %s, %d)\n",
	       (const void *) v, parameter, ival);
  if (!item)
    {
      val = stp_malloc(sizeof(value_t));
      val->name = stp_strdup(parameter);
      val->typ = STP_PARAMETER_TYPE_BOOLEAN;
      val->active = STP_PARAMETER_DEFAULTED;
      stp_list_item_create(list, NULL, val);
      if (ival)
	val->value.ival = 1;
      else
	val->value.ival = 0;
    }
  stp_set_verified(v, 0);
}

void
stp_clear_boolean_parameter(stp_vars_t *v, const char *parameter)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_BOOLEAN];
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  stp_deprintf(STP_DBG_VARS, "stp_clear_boolean_parameter(0x%p, %s)\n",
	       (const void *) v, parameter);
  if (item)
    stp_list_item_destroy(list, item);
  stp_set_verified(v, 0);
}

int
stp_get_boolean_parameter(const stp_vars_t *v, const char *parameter)
{
  const stp_list_t *list = v->params[STP_PARAMETER_TYPE_BOOLEAN];
  const stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  if (item)
    {
      const value_t *val = (const value_t *) stp_list_item_get_data(item);
      return val->value.ival;
    }
  else
    {
      stp_parameter_t desc;
      stp_describe_parameter(v, parameter, &desc);
      if (desc.p_type == STP_PARAMETER_TYPE_BOOLEAN)
	{
	  int boolean = desc.deflt.boolean;
	  stp_parameter_description_destroy(&desc);
	  return boolean;
	}
      else
	{
	  stp_parameter_description_destroy(&desc);
	  stp_erprintf
	    ("Gutenprint: Attempt to retrieve unset boolean parameter %s\n",
	     parameter);
	  return 0;
	}
    }
}

void
stp_set_dimension_parameter(stp_vars_t *v, const char *parameter, int ival)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_DIMENSION];
  value_t *val;
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  stp_deprintf(STP_DBG_VARS, "stp_set_dimension_parameter(0x%p, %s, %d)\n",
	       (const void *) v, parameter, ival);
  if (item)
    {
      val = (value_t *) stp_list_item_get_data(item);
      if (val->active == STP_PARAMETER_DEFAULTED)
	val->active = STP_PARAMETER_ACTIVE;
    }
  else
    {
      val = stp_malloc(sizeof(value_t));
      val->name = stp_strdup(parameter);
      val->typ = STP_PARAMETER_TYPE_DIMENSION;
      val->active = STP_PARAMETER_ACTIVE;
      stp_list_item_create(list, NULL, val);
    }
  val->value.ival = ival;
  stp_set_verified(v, 0);
}

void
stp_set_default_dimension_parameter(stp_vars_t *v, const char *parameter, int ival)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_DIMENSION];
  value_t *val;
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  stp_deprintf(STP_DBG_VARS, "stp_set_default_dimension_parameter(0x%p, %s, %d)\n",
	       (const void *) v, parameter, ival);
  if (!item)
    {
      val = stp_malloc(sizeof(value_t));
      val->name = stp_strdup(parameter);
      val->typ = STP_PARAMETER_TYPE_DIMENSION;
      val->active = STP_PARAMETER_DEFAULTED;
      stp_list_item_create(list, NULL, val);
      val->value.ival = ival;
    }
  stp_set_verified(v, 0);
}

void
stp_clear_dimension_parameter(stp_vars_t *v, const char *parameter)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_DIMENSION];
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  stp_deprintf(STP_DBG_VARS, "stp_clear_dimension_parameter(0x%p, %s)\n",
	       (const void *) v, parameter);
  if (item)
    stp_list_item_destroy(list, item);
  stp_set_verified(v, 0);
}

int
stp_get_dimension_parameter(const stp_vars_t *v, const char *parameter)
{
  const stp_list_t *list = v->params[STP_PARAMETER_TYPE_DIMENSION];
  const stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  if (item)
    {
      const value_t *val = (const value_t *) stp_list_item_get_data(item);
      return val->value.ival;
    }
  else
    {
      stp_parameter_t desc;
      stp_describe_parameter(v, parameter, &desc);
      if (desc.p_type == STP_PARAMETER_TYPE_DIMENSION)
	{
	  int intval = desc.deflt.integer;
	  stp_parameter_description_destroy(&desc);
	  return intval;
	}
      else
	{
	  stp_parameter_description_destroy(&desc);
	  stp_erprintf
	    ("Gutenprint: Attempt to retrieve unset dimension parameter %s\n",
	     parameter);
	  return 0;
	}
    }
}

void
stp_set_float_parameter(stp_vars_t *v, const char *parameter, double dval)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_DOUBLE];
  value_t *val;
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  stp_deprintf(STP_DBG_VARS, "stp_set_float_parameter(0x%p, %s, %f)\n",
	       (const void *) v, parameter, dval);
  if (item)
    {
      val = (value_t *) stp_list_item_get_data(item);
      if (val->active == STP_PARAMETER_DEFAULTED)
	val->active = STP_PARAMETER_ACTIVE;
    }
  else
    {
      val = stp_malloc(sizeof(value_t));
      val->name = stp_strdup(parameter);
      val->typ = STP_PARAMETER_TYPE_DOUBLE;
      val->active = STP_PARAMETER_ACTIVE;
      stp_list_item_create(list, NULL, val);
    }
  val->value.dval = dval;
  stp_set_verified(v, 0);
}

void
stp_set_default_float_parameter(stp_vars_t *v, const char *parameter,
				double dval)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_DOUBLE];
  value_t *val;
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  stp_deprintf(STP_DBG_VARS, "stp_set_default_float_parameter(0x%p, %s, %f)\n",
	       (const void *) v, parameter, dval);
  if (!item)
    {
      val = stp_malloc(sizeof(value_t));
      val->name = stp_strdup(parameter);
      val->typ = STP_PARAMETER_TYPE_DOUBLE;
      val->active = STP_PARAMETER_DEFAULTED;
      stp_list_item_create(list, NULL, val);
      val->value.dval = dval;
    }
  stp_set_verified(v, 0);
}

void
stp_clear_float_parameter(stp_vars_t *v, const char *parameter)
{
  stp_list_t *list = v->params[STP_PARAMETER_TYPE_DOUBLE];
  stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  stp_deprintf(STP_DBG_VARS, "stp_clear_float_parameter(0x%p, %s)\n",
	       (const void *) v, parameter);
  if (item)
    stp_list_item_destroy(list, item);
  stp_set_verified(v, 0);
}

double
stp_get_float_parameter(const stp_vars_t *v, const char *parameter)
{
  const stp_list_t *list = v->params[STP_PARAMETER_TYPE_DOUBLE];
  const stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
  if (item)
    {
      const value_t *val = (value_t *) stp_list_item_get_data(item);
      return val->value.dval;
    }
  else
    {
      stp_parameter_t desc;
      stp_describe_parameter(v, parameter, &desc);
      if (desc.p_type == STP_PARAMETER_TYPE_DOUBLE)
	{
	  double dbl = desc.deflt.dbl;
	  stp_parameter_description_destroy(&desc);
	  return dbl;
	}
      else
	{
	  stp_parameter_description_destroy(&desc);
	  stp_erprintf
	    ("Gutenprint: Attempt to retrieve unset float parameter %s\n",
	     parameter);
	  return 1.0;
	}
    }
}

void
stp_scale_float_parameter(stp_vars_t *v, const char *parameter,
			  double scale)
{
  double val;
  if (stp_check_float_parameter(v, parameter, STP_PARAMETER_DEFAULTED))
    val = stp_get_float_parameter(v, parameter);
  else
    {
      stp_parameter_t desc;
      stp_describe_parameter(v, parameter, &desc);
      if (desc.p_type != STP_PARAMETER_TYPE_DOUBLE)
	{
	  stp_parameter_description_destroy(&desc);
	  return;
	}
      val = desc.deflt.dbl;
      stp_parameter_description_destroy(&desc);
    }
  stp_deprintf(STP_DBG_VARS, "stp_scale_float_parameter(%p, %s, %f*%f)\n",
	       (const void *) v, parameter, val, scale);
  stp_set_float_parameter(v, parameter, val * scale);
}

void
stp_clear_parameter(stp_vars_t *v, const char *parameter, stp_parameter_type_t type)
{
  switch (type)
    {
    case STP_PARAMETER_TYPE_STRING_LIST:
      stp_clear_string_parameter(v, parameter);
      break;
    case STP_PARAMETER_TYPE_FILE:
      stp_clear_file_parameter(v, parameter);
      break;
    case STP_PARAMETER_TYPE_DOUBLE:
      stp_clear_float_parameter(v, parameter);
      break;
    case STP_PARAMETER_TYPE_INT:
      stp_clear_int_parameter(v, parameter);
      break;
    case STP_PARAMETER_TYPE_DIMENSION:
      stp_clear_dimension_parameter(v, parameter);
      break;
    case STP_PARAMETER_TYPE_BOOLEAN:
      stp_clear_boolean_parameter(v, parameter);
      break;
    case STP_PARAMETER_TYPE_CURVE:
      stp_clear_curve_parameter(v, parameter);
      break;
    case STP_PARAMETER_TYPE_ARRAY:
      stp_clear_array_parameter(v, parameter);
      break;
    case STP_PARAMETER_TYPE_RAW:
      stp_clear_raw_parameter(v, parameter);
      break;
    default:
      stp_eprintf(v, "Attempt to clear unknown type parameter!\n");
    }
}


int
stp_check_parameter(const stp_vars_t *v,
		    const char *parameter,
		    stp_parameter_activity_t active,
		    stp_parameter_type_t p_type)
{
  if (p_type >= STP_PARAMETER_TYPE_STRING_LIST &&
      p_type < STP_PARAMETER_TYPE_INVALID)
    {
      const stp_list_t *list = v->params[p_type];
      const stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
      if (item &&
	  active <= ((const value_t *) stp_list_item_get_data(item))->active)
	return 1;
      else
	return 0;
    }
  return 0;
}

#define CHECK_FUNCTION(type, index)					\
int									\
stp_check_##type##_parameter(const stp_vars_t *v, const char *parameter, \
			     stp_parameter_activity_t active)		\
{									\
  return stp_check_parameter(v, parameter, active, index);		\
}

CHECK_FUNCTION(string, STP_PARAMETER_TYPE_STRING_LIST)
CHECK_FUNCTION(file, STP_PARAMETER_TYPE_FILE)
CHECK_FUNCTION(float, STP_PARAMETER_TYPE_DOUBLE)
CHECK_FUNCTION(int, STP_PARAMETER_TYPE_INT)
CHECK_FUNCTION(dimension, STP_PARAMETER_TYPE_DIMENSION)
CHECK_FUNCTION(boolean, STP_PARAMETER_TYPE_BOOLEAN)
CHECK_FUNCTION(curve, STP_PARAMETER_TYPE_CURVE)
CHECK_FUNCTION(array, STP_PARAMETER_TYPE_ARRAY)
CHECK_FUNCTION(raw, STP_PARAMETER_TYPE_RAW)

stp_string_list_t *
stp_list_parameters(const stp_vars_t *v, stp_parameter_type_t p_type)
{
  if (p_type >= STP_PARAMETER_TYPE_STRING_LIST &&
      p_type < STP_PARAMETER_TYPE_INVALID)
    {
      const stp_list_t *list = v->params[p_type];
      stp_string_list_t *answer = stp_string_list_create();
      const stp_list_item_t *li = stp_list_get_start(list);
      while (li)
	{
	  const value_t *val = (value_t *) stp_list_item_get_data(li);
	  stp_string_list_add_string(answer, val->name, val->name);
	  li = stp_list_item_next(li);
	}
      return answer;
    }
  return NULL;
}

#define LIST_FUNCTION(type, index)			\
stp_string_list_t *					\
stp_list_##type##_parameters(const stp_vars_t *v)	\
{							\
  return stp_list_parameters(v, index);			\
}

LIST_FUNCTION(string, STP_PARAMETER_TYPE_STRING_LIST)
LIST_FUNCTION(file, STP_PARAMETER_TYPE_FILE)
LIST_FUNCTION(float, STP_PARAMETER_TYPE_DOUBLE)
LIST_FUNCTION(int, STP_PARAMETER_TYPE_INT)
LIST_FUNCTION(dimension, STP_PARAMETER_TYPE_DIMENSION)
LIST_FUNCTION(boolean, STP_PARAMETER_TYPE_BOOLEAN)
LIST_FUNCTION(curve, STP_PARAMETER_TYPE_CURVE)
LIST_FUNCTION(array, STP_PARAMETER_TYPE_ARRAY)
LIST_FUNCTION(raw, STP_PARAMETER_TYPE_RAW)

stp_parameter_activity_t
stp_get_parameter_active(const stp_vars_t *v, const char *parameter,
			 stp_parameter_type_t p_type)
{
  if (p_type >= STP_PARAMETER_TYPE_STRING_LIST &&
      p_type < STP_PARAMETER_TYPE_INVALID)
    {
      const stp_list_t *list = v->params[p_type];
      const stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
      if (item)
	return ((const value_t *) stp_list_item_get_data(item))->active;
      else
	return 0;
    }
  return 0;
}

#define GET_PARAMETER_ACTIVE_FUNCTION(type, index)			\
stp_parameter_activity_t						\
stp_get_##type##_parameter_active(const stp_vars_t *v, const char *parameter) \
{									\
  return stp_get_parameter_active(v, parameter, index);			\
}

GET_PARAMETER_ACTIVE_FUNCTION(string, STP_PARAMETER_TYPE_STRING_LIST)
GET_PARAMETER_ACTIVE_FUNCTION(file, STP_PARAMETER_TYPE_FILE)
GET_PARAMETER_ACTIVE_FUNCTION(float, STP_PARAMETER_TYPE_DOUBLE)
GET_PARAMETER_ACTIVE_FUNCTION(int, STP_PARAMETER_TYPE_INT)
GET_PARAMETER_ACTIVE_FUNCTION(dimension, STP_PARAMETER_TYPE_DIMENSION)
GET_PARAMETER_ACTIVE_FUNCTION(boolean, STP_PARAMETER_TYPE_BOOLEAN)
GET_PARAMETER_ACTIVE_FUNCTION(curve, STP_PARAMETER_TYPE_CURVE)
GET_PARAMETER_ACTIVE_FUNCTION(array, STP_PARAMETER_TYPE_ARRAY)
GET_PARAMETER_ACTIVE_FUNCTION(raw, STP_PARAMETER_TYPE_RAW)

void
stp_set_parameter_active(stp_vars_t *v,
			 const char *parameter,
			 stp_parameter_activity_t active,
			 stp_parameter_type_t p_type)
{
  if (p_type >= STP_PARAMETER_TYPE_STRING_LIST &&
      p_type < STP_PARAMETER_TYPE_INVALID)
    {
      const stp_list_t *list = v->params[p_type];
      const stp_list_item_t *item = stp_list_get_item_by_name(list, parameter);
      if (item && (active == STP_PARAMETER_ACTIVE ||
		   active == STP_PARAMETER_INACTIVE))
	((value_t *) stp_list_item_get_data(item))->active = active;
    }
}

#define SET_PARAMETER_ACTIVE_FUNCTION(type, index)			\
void									\
stp_set_##type##_parameter_active(stp_vars_t *v, const char *parameter, \
				  stp_parameter_activity_t active)	\
{									\
  stp_deprintf(STP_DBG_VARS,						\
	       "stp_set_%s_parameter_active(0x%p, %s, %d)\n",		\
	       #type, (const void *) v, parameter, active);		\
  stp_set_parameter_active(v, parameter, active, index);		\
}

SET_PARAMETER_ACTIVE_FUNCTION(string, STP_PARAMETER_TYPE_STRING_LIST)
SET_PARAMETER_ACTIVE_FUNCTION(file, STP_PARAMETER_TYPE_FILE)
SET_PARAMETER_ACTIVE_FUNCTION(float, STP_PARAMETER_TYPE_DOUBLE)
SET_PARAMETER_ACTIVE_FUNCTION(int, STP_PARAMETER_TYPE_INT)
SET_PARAMETER_ACTIVE_FUNCTION(dimension, STP_PARAMETER_TYPE_DIMENSION)
SET_PARAMETER_ACTIVE_FUNCTION(boolean, STP_PARAMETER_TYPE_BOOLEAN)
SET_PARAMETER_ACTIVE_FUNCTION(curve, STP_PARAMETER_TYPE_CURVE)
SET_PARAMETER_ACTIVE_FUNCTION(array, STP_PARAMETER_TYPE_ARRAY)
SET_PARAMETER_ACTIVE_FUNCTION(raw, STP_PARAMETER_TYPE_RAW)

void
stp_fill_parameter_settings(stp_parameter_t *desc,
			    const stp_parameter_t *param)
{
  if (param)
    {
      desc->p_type = param->p_type;
      desc->p_level = param->p_level;
      desc->p_class = param->p_class;
      desc->is_mandatory = param->is_mandatory;
      desc->is_active = param->is_active;
      desc->channel = param->channel;
      desc->verify_this_parameter = param->verify_this_parameter;
      desc->read_only = param->read_only;
      desc->name = param->name;
      STPI_ASSERT(param->text, NULL);
      desc->text = gettext(param->text);
      STPI_ASSERT(param->category, NULL);
      desc->category = gettext(param->category);
      desc->help = param->help ? gettext(param->help) : NULL;
      return;
    }
}

void
stp_vars_copy(stp_vars_t *vd, const stp_vars_t *vs)
{
  int i;

  if (vs == vd)
    return;
  stp_set_driver(vd, stp_get_driver(vs));
  stp_set_color_conversion(vd, stp_get_color_conversion(vs));
  stp_set_left(vd, stp_get_left(vs));
  stp_set_top(vd, stp_get_top(vs));
  stp_set_width(vd, stp_get_width(vs));
  stp_set_height(vd, stp_get_height(vs));
  stp_set_page_width(vd, stp_get_page_width(vs));
  stp_set_page_height(vd, stp_get_page_height(vs));
  stp_set_outdata(vd, stp_get_outdata(vs));
  stp_set_errdata(vd, stp_get_errdata(vs));
  stp_set_outfunc(vd, stp_get_outfunc(vs));
  stp_set_errfunc(vd, stp_get_errfunc(vs));
  for (i = 0; i < STP_PARAMETER_TYPE_INVALID; i++)
    {
      stp_list_destroy(vd->params[i]);
      vd->params[i] = copy_value_list(vs->params[i]);
    }
  stp_list_destroy(vd->internal_data);
  vd->internal_data = copy_compdata_list(vs->internal_data);
  stp_set_verified(vd, stp_get_verified(vs));
}

void
stp_prune_inactive_options(stp_vars_t *v)
{
  stp_parameter_list_t params = stp_get_parameter_list(v);
  int i;
  for (i = 0; i < STP_PARAMETER_TYPE_INVALID; i++)
    {
      stp_list_t *list = v->params[i];
      stp_list_item_t *item = stp_list_get_start(list);
      while (item)
	{
	  stp_list_item_t *next = stp_list_item_next(item);
	  value_t *var = (value_t *)stp_list_item_get_data(item);
	  if (var->active < STP_PARAMETER_DEFAULTED ||
	      !(stp_parameter_find(params, var->name)))
	    stp_list_item_destroy(list, item);
	  item = next;
	}
    }
  stp_parameter_list_destroy(params);
}

stp_vars_t *
stp_vars_create_copy(const stp_vars_t *vs)
{
  stp_vars_t *vd = stp_vars_create();
  stp_vars_copy(vd, vs);
  return (vd);
}

static const char *
param_namefunc(const void *item)
{
  const stp_parameter_t *param = (const stp_parameter_t *)(item);
  return param->name;
}

static const char *
param_longnamefunc(const void *item)
{
  const stp_parameter_t *param = (const stp_parameter_t *) (item);
  return param->text;
}

stp_parameter_list_t
stp_parameter_list_create(void)
{
  stp_list_t *ret = stp_list_create();
  stp_list_set_namefunc(ret, param_namefunc);
  stp_list_set_long_namefunc(ret, param_longnamefunc);
  return (stp_parameter_list_t) ret;
}

void
stp_parameter_list_add_param(stp_parameter_list_t list,
			     const stp_parameter_t *item)
{
  stp_list_t *ilist = (stp_list_t *) list;
  stp_list_item_create(ilist, NULL, item);
}

static void
debug_print_parameter_description(const stp_parameter_t *desc, const char *who,
				  const stp_vars_t *v)
{
  int i;
  char *curve;
  if (! (stp_get_debug_level() & STP_DBG_VARS))
    return;
  stp_deprintf(STP_DBG_VARS, "Describe %s: vars 0x%p from %s type %d class %d level %d\n",
	       desc->name, (const void *) v, who,
	       desc->p_type, desc->p_class, desc->p_level);
  stp_deprintf(STP_DBG_VARS, "   driver %s mandatory %d active %d channel %d verify %d ro %d\n",
	       stp_get_driver(v), desc->is_mandatory, desc->is_active,
	       desc->channel, desc->verify_this_parameter, desc->read_only);
  switch (desc->p_type)
    {
    case STP_PARAMETER_TYPE_STRING_LIST:
      stp_deprintf(STP_DBG_VARS,
		   "   String default: %s\n",
		   desc->deflt.str ? desc->deflt.str : "(null)");
      if (desc->bounds.str)
	for (i = 0; i < stp_string_list_count(desc->bounds.str); i++)
	  {
	    if (i == 0)
	      stp_deprintf(STP_DBG_VARS, "          Choices: %s\n",
			   stp_string_list_param(desc->bounds.str, i)->name);
	    else
	      stp_deprintf(STP_DBG_VARS, "                 : %s\n",
			   stp_string_list_param(desc->bounds.str, i)->name);
	  }
      break;
    case STP_PARAMETER_TYPE_INT:
      stp_deprintf(STP_DBG_VARS,
		   "   Integer default: %d Bounds: %d %d\n",
		   desc->deflt.integer,
		   desc->bounds.integer.lower, desc->bounds.integer.upper);
      break;
    case STP_PARAMETER_TYPE_DIMENSION:
      stp_deprintf(STP_DBG_VARS,
		   "   Dimension default: %d Bounds: %d %d\n",
		   desc->deflt.dimension,
		   desc->bounds.dimension.lower, desc->bounds.dimension.upper);
      break;
    case STP_PARAMETER_TYPE_BOOLEAN:
      stp_deprintf(STP_DBG_VARS,
		   "   Boolean default: %d\n", desc->deflt.boolean);
      break;
    case STP_PARAMETER_TYPE_DOUBLE:
      stp_deprintf(STP_DBG_VARS,
		   "   Double default: %f Bounds: %f %f\n",
		   desc->deflt.dbl,
		   desc->bounds.dbl.lower, desc->bounds.dbl.upper);
      break;
    case STP_PARAMETER_TYPE_FILE:
      stp_deprintf(STP_DBG_VARS, "   File (no default)\n");
      break;
    case STP_PARAMETER_TYPE_RAW:
      stp_deprintf(STP_DBG_VARS, "   Raw (no default)\n");
      break;
    case STP_PARAMETER_TYPE_CURVE:
      curve = stp_curve_write_string(desc->deflt.curve);
      stp_deprintf(STP_DBG_VARS,
		   "   Curve default: %s\n", curve);
      stp_free(curve);
      curve = stp_curve_write_string(desc->bounds.curve);
      stp_deprintf(STP_DBG_VARS,
		   "          bounds: %s\n", curve);
      stp_free(curve);
      break;
    case STP_PARAMETER_TYPE_ARRAY:
      stp_deprintf(STP_DBG_VARS, "   Array\n");
      break;
    case STP_PARAMETER_TYPE_INVALID:
      stp_deprintf(STP_DBG_VARS, "   *** Invalid ***\n");
      break;
    default:
      stp_deprintf(STP_DBG_VARS, "   Unknown type!\n");
    }
}

void
stp_describe_parameter(const stp_vars_t *v, const char *name,
		       stp_parameter_t *description)
{
  description->p_type = STP_PARAMETER_TYPE_INVALID;
/* Set these to NULL in case stpi_*_describe_parameter() doesn't */
  description->bounds.str = NULL;
  description->deflt.str = NULL;
  stp_printer_describe_parameter(v, name, description);
  if (description->p_type != STP_PARAMETER_TYPE_INVALID)
    {
      debug_print_parameter_description(description, "driver", v);
      return;
    }
  stp_color_describe_parameter(v, name, description);
  if (description->p_type != STP_PARAMETER_TYPE_INVALID)
    {
      debug_print_parameter_description(description, "color", v);
      return;
    }
  stp_dither_describe_parameter(v, name, description);
  if (description->p_type != STP_PARAMETER_TYPE_INVALID)
    {
      debug_print_parameter_description(description, "dither", v);
      return;
    }
  stpi_describe_generic_parameter(v, name, description);
  if (description->p_type != STP_PARAMETER_TYPE_INVALID)
    debug_print_parameter_description(description, "generic", v);
  else
    stp_deprintf(STP_DBG_VARS, "Describing invalid parameter %s\n", name);
}

stp_string_list_t *
stp_parameter_get_categories(const stp_vars_t *v, const stp_parameter_t *desc)
{
  const char *dptr;
  stp_string_list_t *answer;
  int count = 0;
  if (!v || !desc || !(desc->category))
    return NULL;
  answer = stp_string_list_create();
  dptr = desc->category;
  while (dptr)
    {
      const char *xptr = strchr(dptr, '=');
      if (xptr)
	{
	  char *name = stp_strndup(dptr, xptr - dptr);
	  char *text;
	  dptr = xptr + 1;
	  xptr = strchr(dptr, ',');
	  if (xptr)
	    {
	      text = stp_strndup(dptr, xptr - dptr);
	      dptr = xptr + 1;
	    }
	  else
	    {
	      text = stp_strdup(dptr);
	      dptr = NULL;
	    }
	  stp_string_list_add_string(answer, name, text);
	  stp_free(name);
	  stp_free(text);
	  count++;
	}
      else
	dptr = NULL;
    }
  if (count == 0)
    {
      stp_string_list_destroy(answer);
      return NULL;
    }
  else
    return answer;
}

char *
stp_parameter_get_category(const stp_vars_t *v, const stp_parameter_t *desc,
			   const char *category)
{
  const char *dptr;
  char *cptr;
  int len;
  if (!v || !desc || !(desc->category) || !category)
    return NULL;
  dptr = desc->category;
  stp_asprintf(&cptr, "%s=", category);
  len = stp_strlen(cptr);
  while (dptr)
    {
      if (strncmp(dptr, cptr, len) == 0)
	{
	  const char *xptr;
	  char *answer;
	  dptr += len;
	  xptr = strchr(dptr, ',');
	  if (xptr)
	    answer = stp_strndup(dptr, xptr - dptr);
	  else
	    answer = stp_strdup(dptr);
	  stp_free(cptr);
	  return answer;
	}
      dptr = strchr(dptr, ',');
      if (dptr)
	dptr++;
    }
  return NULL;
}

int
stp_parameter_has_category_value(const stp_vars_t *v,
				 const stp_parameter_t *desc,
				 const char *category, const char *value)
{
  const char *dptr;
  char *cptr;
  int answer = 0;
  if (!v || !desc || !category)
    return -1;
  cptr = stp_parameter_get_category(v, desc, category);
  if (cptr == NULL)
    return 0;
  if (value == NULL || strcmp(value, cptr) == 0)
    answer = 1;
  stp_free(cptr);
  return answer;
}

const stp_parameter_t *
stp_parameter_find_in_settings(const stp_vars_t *v, const char *name)
{
  stp_parameter_list_t param_list = stp_get_parameter_list(v);
  const stp_parameter_t *param = stp_parameter_find(param_list, name);
  stp_parameter_list_destroy(param_list);
  return param;
}

size_t
stp_parameter_list_count(stp_const_parameter_list_t list)
{
  const stp_list_t *ilist = (const stp_list_t *)list;
  return stp_list_get_length(ilist);
}

const stp_parameter_t *
stp_parameter_find(stp_const_parameter_list_t list, const char *name)
{
  const stp_list_t *ilist = (const stp_list_t *)list;
  const stp_list_item_t *item = stp_list_get_item_by_name(ilist, name);
  if (item)
    return (const stp_parameter_t *) stp_list_item_get_data(item);
  else
    return NULL;
}

const stp_parameter_t *
stp_parameter_list_param(stp_const_parameter_list_t list, size_t item)
{
  const stp_list_t *ilist = (const stp_list_t *)list;
  if (item >= stp_list_get_length(ilist))
    return NULL;
  else
    return (const stp_parameter_t *)
      stp_list_item_get_data(stp_list_get_item_by_index(ilist, item));
}

void
stp_parameter_list_destroy(stp_parameter_list_t list)
{
  stp_list_destroy((stp_list_t *)list);
}

stp_parameter_list_t
stp_parameter_list_copy(stp_const_parameter_list_t list)
{
  stp_list_t *ret = stp_parameter_list_create();
  int i;
  size_t count = stp_parameter_list_count(list);
  for (i = 0; i < count; i++)
    stp_list_item_create(ret, NULL, stp_parameter_list_param(list, i));
  return (stp_parameter_list_t) ret;
}

void
stp_parameter_list_append(stp_parameter_list_t list,
			  stp_const_parameter_list_t append)
{
  int i;
  stp_list_t *ilist = (stp_list_t *)list;
  size_t count = stp_parameter_list_count(append);
  for (i = 0; i < count; i++)
    {
      const stp_parameter_t *param = stp_parameter_list_param(append, i);
      if (!stp_list_get_item_by_name(ilist, param->name))
	stp_list_item_create(ilist, NULL, param);
    }
}

static void
fill_vars_from_xmltree(stp_mxml_node_t *prop, stp_mxml_node_t *root,
		       stp_vars_t *v)
{
#ifdef HAVE_LOCALE_H
  char *locale = stp_strdup(setlocale(LC_ALL, NULL));
  setlocale(LC_ALL, "C");
#endif
  while (prop)
    {
      if (prop->type == STP_MXML_ELEMENT &&
	  !strcmp(prop->value.element.name, "parameter") &&
	  (prop->child || stp_mxmlElementGetAttr(prop, "name")))
	{
	  stp_mxml_node_t *child = prop->child;
	  const char *prop_name = prop->value.element.name;
	  const char *p_type = stp_mxmlElementGetAttr(prop, "type");
	  const char *p_name = stp_mxmlElementGetAttr(prop, "name");
	  if (!strcmp(prop_name, "parameter") && (!p_type || !p_name))
	    stp_erprintf("Bad property found!\n");
	  else if (!strcmp(prop_name, "parameter"))
	    {
	      const char *active = stp_mxmlElementGetAttr(prop, "active");
	      const char *cref = stp_mxmlElementGetAttr(prop, "ref");
	      stp_mxml_node_t *cnode = child;
	      stp_parameter_type_t type = STP_PARAMETER_TYPE_INVALID;
	      if (cref && root)
		{
		  cnode = stp_mxmlFindElement(root, root, "namedParam",
					      "name", cref,
					      STP_MXML_DESCEND);
		  STPI_ASSERT(cnode && cnode->type == STP_MXML_ELEMENT &&
			 cnode->child, v);
		  stp_deprintf(STP_DBG_XML, "Found parameter ref %s\n", cref);
		  cnode = cnode->child;
		}
	      if (strcmp(p_type, "float") == 0)
		{
		  if (cnode->type == STP_MXML_TEXT)
		    {
		      stp_set_float_parameter
			(v, p_name, stp_xmlstrtod(cnode->value.text.string));
		      type = STP_PARAMETER_TYPE_DOUBLE;
		      if (stp_get_debug_level() & STP_DBG_XML)
			stp_deprintf(STP_DBG_XML, "  Set float '%s' to '%s' (%f)\n",
				     p_name, cnode->value.text.string,
				     stp_get_float_parameter(v, p_name));
		    }
		}
	      else if (strcmp(p_type, "integer") == 0)
		{
		  if (cnode->type == STP_MXML_TEXT)
		    {
		      stp_set_int_parameter
			(v, p_name, (int) stp_xmlstrtol(cnode->value.text.string));
		      type = STP_PARAMETER_TYPE_DOUBLE;
		      if (stp_get_debug_level() & STP_DBG_XML)
			stp_deprintf(STP_DBG_XML, "  Set int '%s' to '%s' (%d)\n",
				     p_name, cnode->value.text.string,
				     stp_get_int_parameter(v, p_name));
		    }
		}
	      else if (strcmp(p_type, "dimension") == 0)
		{
		  if (cnode->type == STP_MXML_TEXT)
		    {
		      stp_set_dimension_parameter
			(v, p_name, (int) stp_xmlstrtol(cnode->value.text.string));
		      type = STP_PARAMETER_TYPE_DOUBLE;
		      if (stp_get_debug_level() & STP_DBG_XML)
			stp_deprintf(STP_DBG_XML, "  Set dimension '%s' to '%s' (%d)\n",
				     p_name, cnode->value.text.string,
				     stp_get_dimension_parameter(v, p_name));
		    }
		}
	      else if (strcmp(p_type, "boolean") == 0)
		{
		  if (cnode->type == STP_MXML_TEXT)
		    {
		      stp_set_boolean_parameter
			(v, p_name, (int) stp_xmlstrtol(cnode->value.text.string));
		      type = STP_PARAMETER_TYPE_DOUBLE;
		      if (stp_get_debug_level() & STP_DBG_XML)
			stp_deprintf(STP_DBG_XML, "  Set bool '%s' to '%s' (%d)\n",
				     p_name, cnode->value.text.string,
				     stp_get_boolean_parameter(v, p_name));
		    }
		}
	      else if (strcmp(p_type, "string") == 0)
		{
		  if (cnode->type == STP_MXML_TEXT)
		    {
		      stp_set_string_parameter
			(v, p_name, cnode->value.text.string);
		      type = STP_PARAMETER_TYPE_DOUBLE;
		      if (stp_get_debug_level() & STP_DBG_XML)
			stp_deprintf(STP_DBG_XML, "  Set string '%s' to '%s' (%s)\n",
				     p_name, cnode->value.text.string,
				     stp_get_string_parameter(v, p_name));
		    }
		}
	      else if (strcmp(p_type, "file") == 0)
		{
		  if (cnode->type == STP_MXML_TEXT)
		    {
		      stp_set_file_parameter
			(v, p_name, cnode->value.text.string);
		      type = STP_PARAMETER_TYPE_DOUBLE;
		      if (stp_get_debug_level() & STP_DBG_XML)
			stp_deprintf(STP_DBG_XML, "  Set file '%s' to '%s' (%s)\n",
				     p_name, cnode->value.text.string,
				     stp_get_file_parameter(v, p_name));
		    }
		}
	      else if (strcmp(p_type, "raw") == 0)
		{
		  if (cnode->type == STP_MXML_TEXT)
		    {
		      stp_raw_t *raw = stp_xmlstrtoraw(cnode->value.text.string);
		      if (raw)
			{
			  stp_set_raw_parameter(v, p_name, raw->data,raw->bytes);
			  type = STP_PARAMETER_TYPE_DOUBLE;
			  stp_deprintf(STP_DBG_XML, "  Set raw '%s' to '%s'\n",
				       p_name, cnode->value.text.string);
			  stp_free((void *) raw->data);
			  stp_free(raw);
			}
		    }
		}
	      else if (strcmp(p_type, "curve") == 0)
		{
		  stp_curve_t *curve;
		  while (cnode->type != STP_MXML_ELEMENT && cnode->next)
		    cnode = cnode->next;
		  STPI_ASSERT(cnode, v);
		  curve = stp_curve_create_from_xmltree(cnode);
		  STPI_ASSERT(curve, v);
		  stp_set_curve_parameter(v, p_name, curve);
		  type = STP_PARAMETER_TYPE_DOUBLE;
		  if (stp_get_debug_level() & STP_DBG_XML)
		    {
		      char *cv = stp_curve_write_string(curve);
		      stp_deprintf(STP_DBG_XML, "  Set curve '%s' (%s)\n",
				   p_name, cv);
		      stp_free(cv);
		    }
		  stp_curve_destroy(curve);
		}
	      else if (strcmp(p_type, "array") == 0)
		{
		  stp_array_t *array;
		  while (cnode->type != STP_MXML_ELEMENT && cnode->next)
		    cnode = cnode->next;
		  STPI_ASSERT(cnode, v);
		  array = stp_array_create_from_xmltree(cnode);
		  STPI_ASSERT(array, v);
		  type = STP_PARAMETER_TYPE_DOUBLE;
		  stp_set_array_parameter(v, p_name, array);
		  stp_deprintf(STP_DBG_XML, "  Set array '%s'\n", p_name);
		  stp_array_destroy(array);
		}
	      else
		{
		  stp_erprintf("Bad property %s type %s\n", p_name, p_type);
		}
	      if (active && type != STP_PARAMETER_TYPE_INVALID)
		{
		  if (strcmp(active, "active") == 0)
		    stp_set_parameter_active(v, p_name, STP_PARAMETER_ACTIVE, type);
		  else if (strcmp(active, "inactive") == 0)
		    stp_set_parameter_active(v, p_name, STP_PARAMETER_INACTIVE, type);
		  else if (strcmp(active, "default") == 0)
		    stp_set_parameter_active(v, p_name, STP_PARAMETER_DEFAULTED, type);
		}

	    }
	  else if (child->type == STP_MXML_TEXT)
	    {
	      if (!strcmp(prop_name, "driver"))
		stp_set_driver(v, child->value.text.string);
	      else if (!strcmp(prop_name, "color_conversion"))
		stp_set_color_conversion(v, child->value.text.string);
	      else if (!strcmp(prop_name, "left"))
		stp_set_left(v, stp_xmlstrtol(child->value.text.string));
	      else if (!strcmp(prop_name, "top"))
		stp_set_top(v, stp_xmlstrtol(child->value.text.string));
	      else if (!strcmp(prop_name, "width"))
		stp_set_width(v, stp_xmlstrtol(child->value.text.string));
	      else if (!strcmp(prop_name, "height"))
		stp_set_height(v, stp_xmlstrtol(child->value.text.string));
	      else if (!strcmp(prop_name, "page_width"))
		stp_set_page_width(v, stp_xmlstrtol(child->value.text.string));
	      else if (!strcmp(prop_name, "page_height"))
		stp_set_page_height(v, stp_xmlstrtol(child->value.text.string));
	    }
	}
      prop = prop->next;
    }
#ifdef HAVE_LOCALE_H
  setlocale(LC_ALL, locale);
  stp_free(locale);
#endif
}

void
stp_vars_fill_from_xmltree(stp_mxml_node_t *prop, stp_vars_t *v)
{
  fill_vars_from_xmltree(prop, NULL, v);
}

stp_vars_t *
stp_vars_create_from_xmltree(stp_mxml_node_t *da)
{
  stp_vars_t *v = stp_vars_create();
  fill_vars_from_xmltree(da, NULL, v);
  return v;
}

void
stp_vars_fill_from_xmltree_ref(stp_mxml_node_t *prop, stp_mxml_node_t *root,
			       stp_vars_t *v)
{
  fill_vars_from_xmltree(prop, root, v);
}

stp_vars_t *
stp_vars_create_from_xmltree_ref(stp_mxml_node_t *da, stp_mxml_node_t *root)
{
  stp_vars_t *v = stp_vars_create();
  fill_vars_from_xmltree(da, root, v);
  return v;
}

static void
add_text_node(stp_mxml_node_t *node, const char *element, const char *value)
{
  if (value)
    stp_mxmlNewOpaque(stp_mxmlNewElement(node, element), value);
}

stp_mxml_node_t *
stp_xmltree_create_from_vars(const stp_vars_t *v)
{
  stp_mxml_node_t *varnode;
  int i;
  if (!v)
    return NULL;
  varnode = stp_mxmlNewElement(NULL, "vars");
  add_text_node(varnode, "driver", stp_get_driver(v));
  add_text_node(varnode, "color_conversion", stp_get_color_conversion(v));
  stp_mxmlNewInteger(stp_mxmlNewElement(varnode, "left"), stp_get_left(v));
  stp_mxmlNewInteger(stp_mxmlNewElement(varnode, "top"), stp_get_top(v));
  stp_mxmlNewInteger(stp_mxmlNewElement(varnode, "width"), stp_get_width(v));
  stp_mxmlNewInteger(stp_mxmlNewElement(varnode, "height"), stp_get_height(v));
  stp_mxmlNewInteger(stp_mxmlNewElement(varnode, "page_width"), stp_get_page_width(v));
  stp_mxmlNewInteger(stp_mxmlNewElement(varnode, "page_height"), stp_get_page_height(v));
  for (i = STP_PARAMETER_TYPE_STRING_LIST; i < STP_PARAMETER_TYPE_INVALID; i++)
    {
      stp_string_list_t *list = stp_list_parameters(v, i);
      if (list)
	{
	  int j;
	  int count = stp_string_list_count(list);
	  for (j = 0; j < count; j++)
	    {
	      const stp_param_string_t *pstr = stp_string_list_param(list, j);
	      const char *name = pstr->name;
	      char *data;
	      stp_mxml_node_t *node = stp_mxmlNewElement(varnode, "parameter");
	      stp_parameter_activity_t active =
		stp_get_parameter_active(v, name, i);
	      stp_mxmlElementSetAttr(node, "name", name);
	      stp_mxmlElementSetAttr(node, "active",
				     (active == STP_PARAMETER_INACTIVE ?
				      "inactive" :
				      (active == STP_PARAMETER_DEFAULTED ?
				       "default" : "active")));
	      switch (i)
		{
		case STP_PARAMETER_TYPE_STRING_LIST:
		  stp_mxmlElementSetAttr(node, "type", "string");
		  data = stp_strtoxmlstr(stp_get_string_parameter(v, name));
		  if (data)
		    {
		      stp_mxmlNewOpaque(node, data);
		      stp_free(data);
		    }
		  break;
		case STP_PARAMETER_TYPE_INT:
		  stp_mxmlElementSetAttr(node, "type", "integer");
		  stp_mxmlNewInteger(node, stp_get_int_parameter(v, name));
		  break;
		case STP_PARAMETER_TYPE_BOOLEAN:
		  stp_mxmlElementSetAttr(node, "type", "boolean");
		  stp_mxmlNewInteger(node, stp_get_boolean_parameter(v, name));
		  break;
		case STP_PARAMETER_TYPE_DOUBLE:
		  stp_mxmlElementSetAttr(node, "type", "float");
		  stp_mxmlNewReal(node, stp_get_float_parameter(v, name));
		  break;
		case STP_PARAMETER_TYPE_CURVE:
		  stp_mxmlElementSetAttr(node, "type", "curve");
		  stp_mxmlAdd(node, STP_MXML_ADD_AFTER, NULL,
			      stp_xmltree_create_from_curve(stp_get_curve_parameter(v, name)));
		  break;
		case STP_PARAMETER_TYPE_FILE:
		  stp_mxmlElementSetAttr(node, "type", "file");
		  data = stp_strtoxmlstr(stp_get_file_parameter(v, name));
		  if (data)
		    {
		      stp_mxmlNewOpaque(node, data);
		      stp_free(data);
		    }
		  break;
		case STP_PARAMETER_TYPE_RAW:
		  stp_mxmlElementSetAttr(node, "type", "raw");
		  data = stp_rawtoxmlstr(stp_get_raw_parameter(v, name));
		  if (data)
		    {
		      stp_mxmlNewOpaque(node, data);
		      stp_free(data);
		    }
		  break;
		case STP_PARAMETER_TYPE_ARRAY:
		  stp_mxmlElementSetAttr(node, "type", "array");
		  stp_mxmlAdd(node, STP_MXML_ADD_AFTER, NULL,
			      stp_xmltree_create_from_array(stp_get_array_parameter(v, name)));
		  break;
		case STP_PARAMETER_TYPE_DIMENSION:
		  stp_mxmlElementSetAttr(node, "type", "dimension");
		  stp_mxmlNewInteger(node, stp_get_dimension_parameter(v, name));
		  break;
		default:
		  stp_mxmlElementSetAttr(node, "type", "INVALID!");
		  break;
		}
	    }
	  stp_string_list_destroy(list);
	}
    }
  return varnode;
}
