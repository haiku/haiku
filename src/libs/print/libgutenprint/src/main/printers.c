/*
 * "$Id: printers.c,v 1.89 2010/08/07 02:30:38 rlk Exp $"
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
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include <math.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <string.h>
#include <stdlib.h>

#define FMIN(a, b) ((a) < (b) ? (a) : (b))


static void stpi_printvars_freefunc(void *item);
static const char* stpi_printvars_namefunc(const void *item);

static stp_list_t *printvars_list = NULL;

typedef struct stp_printvars
{
  const char *name;
  stp_vars_t *printvars;
} stp_printvars_t;

static void stpi_printer_freefunc(void *item);
static const char* stpi_printer_namefunc(const void *item);
static const char* stpi_printer_long_namefunc(const void *item);

static stp_list_t *printer_list = NULL;

struct stp_printer
{
  const char *driver;
  char       *long_name;        /* Long name for UI */
  char       *family;           /* Printer family */
  char	     *manufacturer;	/* Printer manufacturer */
  char	     *device_id; 	/* IEEE 1284 device ID */
  char       *foomatic_id;	/* Foomatic printer ID */
  int        model;             /* Model number */
  int	     vars_initialized;
  const stp_printfuncs_t *printfuncs;
  stp_vars_t *printvars;
};

static void
stpi_init_printvars_list(void)
{
  if (!printvars_list)
    {
      printvars_list = stp_list_create();
      stp_list_set_freefunc(printvars_list, stpi_printvars_freefunc);
      stp_list_set_namefunc(printvars_list, stpi_printvars_namefunc);
      stp_list_set_long_namefunc(printvars_list, stpi_printvars_namefunc);
    }
}

static int
stpi_init_printer_list(void)
{
  if(printer_list)
    stp_list_destroy(printer_list);
  printer_list = stp_list_create();
  stp_list_set_freefunc(printer_list, stpi_printer_freefunc);
  stp_list_set_namefunc(printer_list, stpi_printer_namefunc);
  stp_list_set_long_namefunc(printer_list, stpi_printer_long_namefunc);
  /* stp_list_set_sortfunc(printer_list, stpi_printer_sortfunc); */
  return 0;
}

int
stp_printer_model_count(void)
{
  if (printer_list == NULL)
    {
      stp_erprintf("No printer drivers found: "
		   "are STP_DATA_PATH and STP_MODULE_PATH correct?\n");
      stpi_init_printer_list();
    }
  return stp_list_get_length(printer_list);
}

const stp_printer_t *
stp_get_printer_by_index(int idx)
{
  stp_list_item_t *printer;
  if (printer_list == NULL)
    {
      stp_erprintf("No printer drivers found: "
		   "are STP_DATA_PATH and STP_MODULE_PATH correct?\n");
      stpi_init_printer_list();
    }
  printer = stp_list_get_item_by_index(printer_list, idx);
  if (printer == NULL)
    return NULL;
  return (const stp_printer_t *) stp_list_item_get_data(printer);
}

static void
stpi_printer_freefunc(void *item)
{
  stp_printer_t *printer = (stp_printer_t *) item;
  stp_free(printer->long_name);
  stp_free(printer->family);
  stp_free(printer);
}

/* ARGSUSED */
static void
stpi_printvars_freefunc(void *item)
{
}

static const char *
stpi_printvars_namefunc(const void *item)
{
  const stp_printvars_t *printvars = (const stp_printvars_t *) item;
  return printvars->name;
}

const char *
stp_printer_get_driver(const stp_printer_t *printer)
{
  return printer->driver;
}

static const char *
stpi_printer_namefunc(const void *item)
{
  const stp_printer_t *printer = (const stp_printer_t *) item;
  return printer->driver;
}

const char *
stp_printer_get_long_name(const stp_printer_t *printer)
{
  return printer->long_name;
}

static const char *
stpi_printer_long_namefunc(const void *item)
{
  const stp_printer_t *printer = (const stp_printer_t *) item;
  return printer->long_name;
}

const char *
stp_printer_get_device_id(const stp_printer_t *printer)
{
  return printer->device_id;
}

const char *
stp_printer_get_family(const stp_printer_t *printer)
{
  return printer->family;
}

const char *
stp_printer_get_manufacturer(const stp_printer_t *printer)
{
  return printer->manufacturer;
}

const char *
stp_printer_get_foomatic_id(const stp_printer_t *printer)
{
  return printer->foomatic_id;
}

int
stp_printer_get_model(const stp_printer_t *printer)
{
  return printer->model;
}

static inline const stp_printfuncs_t *
stpi_get_printfuncs(const stp_printer_t *printer)
{
  return printer->printfuncs;
}


const stp_printer_t *
stp_get_printer_by_long_name(const char *long_name)
{
  stp_list_item_t *printer_item;
  if (printer_list == NULL)
    {
      stp_erprintf("No printer drivers found: "
		   "are STP_DATA_PATH and STP_MODULE_PATH correct?\n");
      stpi_init_printer_list();
    }
  printer_item = stp_list_get_item_by_long_name(printer_list, long_name);
  if (!printer_item)
    return NULL;
  return (const stp_printer_t *) stp_list_item_get_data(printer_item);
}

const stp_printer_t *
stp_get_printer_by_driver(const char *driver)
{
  stp_list_item_t *printer_item;
  if (printer_list == NULL)
    {
      stp_erprintf("No printer drivers found: "
		   "are STP_DATA_PATH and STP_MODULE_PATH correct?\n");
      stpi_init_printer_list();
    }
  printer_item = stp_list_get_item_by_name(printer_list, driver);
  if (!printer_item)
    return NULL;
  return (const stp_printer_t *) stp_list_item_get_data(printer_item);
}

const stp_printer_t *
stp_get_printer_by_device_id(const char *device_id)
{
  stp_list_item_t *printer_item;
  if (printer_list == NULL)
    {
      stp_erprintf("No printer drivers found: "
		   "are STP_DATA_PATH and STP_MODULE_PATH correct?\n");
      stpi_init_printer_list();
    }
  if (! device_id || strcmp(device_id, "") == 0)
    return NULL;

  printer_item = stp_list_get_start(printer_list);
  while (printer_item)
    {
      if (strcmp(((const stp_printer_t *) stp_list_item_get_data(printer_item))->device_id,
		 device_id) == 0)
	return ((const stp_printer_t *) stp_list_item_get_data(printer_item));
      printer_item = stp_list_item_next(printer_item);
    }
  return NULL;
}

const stp_printer_t *
stp_get_printer_by_foomatic_id(const char *foomatic_id)
{
  stp_list_item_t *printer_item;
  if (printer_list == NULL)
    {
      stp_erprintf("No printer drivers found: "
		   "are STP_DATA_PATH and STP_MODULE_PATH correct?\n");
      stpi_init_printer_list();
    }
  if (! foomatic_id || strcmp(foomatic_id, "") == 0)
    return NULL;

  printer_item = stp_list_get_start(printer_list);
  while (printer_item)
    {
      if (strcmp(((const stp_printer_t *) stp_list_item_get_data(printer_item))->foomatic_id,
		 foomatic_id) == 0)
	return ((const stp_printer_t *) stp_list_item_get_data(printer_item));
      printer_item = stp_list_item_next(printer_item);
    }
  return NULL;
}

int
stp_get_printer_index_by_driver(const char *driver)
{
  /* There should be no need to ever know the index! */
  int idx = 0;
  for (idx = 0; idx < stp_printer_model_count(); idx++)
    {
      const stp_printer_t *printer = stp_get_printer_by_index(idx);
      if (!strcmp(stp_printer_get_driver(printer), driver))
	return idx;
    }
  return -1;
}

const stp_printer_t *
stp_get_printer(const stp_vars_t *v)
{
  return stp_get_printer_by_driver(stp_get_driver(v));
}

int
stp_get_model_id(const stp_vars_t *v)
{
  const stp_printer_t *printer = stp_get_printer_by_driver(stp_get_driver(v));
  return printer->model;
}

stp_parameter_list_t
stp_printer_list_parameters(const stp_vars_t *v)
{
  const stp_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  return (printfuncs->list_parameters)(v);
}

void
stp_printer_describe_parameter(const stp_vars_t *v, const char *name,
			       stp_parameter_t *description)
{
  const stp_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  (printfuncs->parameters)(v, name, description);
}

static void
set_printer_defaults(stp_vars_t *v, int core_only, int soft)
{
  stp_parameter_list_t *params;
  int count;
  int i;
  stp_parameter_t desc;
  params = stp_get_parameter_list(v);
  count = stp_parameter_list_count(params);
  for (i = 0; i < count; i++)
    {
      const stp_parameter_t *p = stp_parameter_list_param(params, i);
      if (p->is_mandatory &&
	  (!core_only || p->p_class == STP_PARAMETER_CLASS_CORE))
	{
	  stp_describe_parameter(v, p->name, &desc);
	  switch (p->p_type)
	    {
	    case STP_PARAMETER_TYPE_STRING_LIST:
	      if (!soft ||
		  !stp_check_string_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
		{
		  stp_set_string_parameter(v, p->name, desc.deflt.str);
		  stp_set_string_parameter_active(v, p->name, STP_PARAMETER_ACTIVE);
		}
	      break;
	    case STP_PARAMETER_TYPE_DOUBLE:
	      if (!soft ||
		  !stp_check_float_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
		{
		  stp_set_float_parameter(v, p->name, desc.deflt.dbl);
		  stp_set_float_parameter_active(v, p->name, STP_PARAMETER_ACTIVE);
		}
	      break;
	    case STP_PARAMETER_TYPE_INT:
	      if (!soft ||
		  !stp_check_int_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
		{
		  stp_set_int_parameter(v, p->name, desc.deflt.integer);
		  stp_set_int_parameter_active(v, p->name, STP_PARAMETER_ACTIVE);
		}
	      break;
	    case STP_PARAMETER_TYPE_DIMENSION:
	      if (!soft ||
		  !stp_check_dimension_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
		{
		  stp_set_dimension_parameter(v, p->name, desc.deflt.dimension);
		  stp_set_dimension_parameter_active(v, p->name, STP_PARAMETER_ACTIVE);
		}
	      break;
	    case STP_PARAMETER_TYPE_BOOLEAN:
	      if (!soft ||
		  !stp_check_boolean_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
		{
		  stp_set_boolean_parameter(v, p->name, desc.deflt.boolean);
		  stp_set_boolean_parameter_active(v, p->name, STP_PARAMETER_ACTIVE);
		}
	      break;
	    case STP_PARAMETER_TYPE_CURVE:
	      if (!soft ||
		  !stp_check_curve_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
		{
		  stp_set_curve_parameter(v, p->name, desc.deflt.curve);
		  stp_set_curve_parameter_active(v, p->name, STP_PARAMETER_ACTIVE);
		}
	      break;
	    case STP_PARAMETER_TYPE_ARRAY:
	      if (!soft ||
		  !stp_check_array_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
		{
		  stp_set_array_parameter(v, p->name, desc.deflt.array);
		  stp_set_array_parameter_active(v, p->name, STP_PARAMETER_ACTIVE);
		}
	      break;
	    default:
	      break;
	    }
	  stp_parameter_description_destroy(&desc);
	}
    }
  stp_parameter_list_destroy(params);
}

void
stp_set_printer_defaults(stp_vars_t *v, const stp_printer_t *printer)
{
  stp_set_driver(v, stp_printer_get_driver(printer));
  set_printer_defaults(v, 0, 0);
}

void
stp_set_printer_defaults_soft(stp_vars_t *v, const stp_printer_t *printer)
{
  stp_set_driver(v, stp_printer_get_driver(printer));
  set_printer_defaults(v, 0, 1);
}

void
stp_initialize_printer_defaults(void)
{
  if (printer_list == NULL)
    {
      stpi_init_printer_list();
      stp_deprintf
	(STP_DBG_PRINTERS,
	 "stpi_family_register(): initialising printer_list...\n");
    }
}

const stp_vars_t *
stp_printer_get_defaults(const stp_printer_t *printer)
{
  if (! printer->vars_initialized)
    {
      stp_printer_t *nc_printer = (stp_printer_t *) printer;
      stp_deprintf(STP_DBG_PRINTERS, "  ==>init %s\n", printer->driver);
      set_printer_defaults (nc_printer->printvars, 1, 0);
      nc_printer->vars_initialized = 1;
    }
  return printer->printvars;
}

void
stp_get_media_size(const stp_vars_t *v, int *width, int *height)
{
  const stp_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  (printfuncs->media_size)(v, width, height);
}

void
stp_get_imageable_area(const stp_vars_t *v,
		       int *left, int *right, int *bottom, int *top)
{
  const stp_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  (printfuncs->imageable_area)(v, left, right, bottom, top);
}

void
stp_get_maximum_imageable_area(const stp_vars_t *v,
			       int *left, int *right, int *bottom, int *top)
{
  const stp_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  (printfuncs->maximum_imageable_area)(v, left, right, bottom, top);
}

void
stp_get_size_limit(const stp_vars_t *v, int *max_width, int *max_height,
		   int *min_width, int *min_height)
{
  const stp_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  (printfuncs->limit)(v, max_width, max_height, min_width,min_height);
}

void
stp_describe_resolution(const stp_vars_t *v, int *x, int *y)
{
  const stp_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  (printfuncs->describe_resolution)(v, x, y);
}

const char *
stp_describe_output(const stp_vars_t *v)
{
  const stp_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  return (printfuncs->describe_output)(v);
}

int
stp_verify(stp_vars_t *v)
{
  const stp_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  stp_vars_t *nv = stp_vars_create_copy(v);
  int status;
  stp_prune_inactive_options(nv);
  status = (printfuncs->verify)(nv);
  stp_set_verified(v, stp_get_verified(nv));
  stp_vars_destroy(nv);
  return status;
}

int
stp_print(const stp_vars_t *v, stp_image_t *image)
{
  const stp_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  return (printfuncs->print)(v, image);
}

int
stp_start_job(const stp_vars_t *v, stp_image_t *image)
{
  const stp_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  if (!stp_get_string_parameter(v, "JobMode") ||
      strcmp(stp_get_string_parameter(v, "JobMode"), "Page") == 0)
    return 1;
  if (printfuncs->start_job)
    return (printfuncs->start_job)(v, image);
  else
    return 1;
}

int
stp_end_job(const stp_vars_t *v, stp_image_t *image)
{
  const stp_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  if (!stp_get_string_parameter(v, "JobMode") ||
      strcmp(stp_get_string_parameter(v, "JobMode"), "Page") == 0)
    return 1;
  if (printfuncs->end_job)
    return (printfuncs->end_job)(v, image);
  else
    return 1;
}

stp_string_list_t *
stp_get_external_options(const stp_vars_t *v)
{
  const stp_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  if (printfuncs->get_external_options)
    return (printfuncs->get_external_options)(v);
  else
    return NULL;
}

static int
verify_string_param(const stp_vars_t *v, const char *parameter,
		    stp_parameter_t *desc, int quiet)
{
  stp_parameter_verify_t answer = PARAMETER_OK;
  stp_dprintf(STP_DBG_VARS, v, "    Verifying string %s\n", parameter);
  if (desc->is_mandatory ||
      stp_check_string_parameter(v, parameter, STP_PARAMETER_ACTIVE))
    {
      const char *checkval = stp_get_string_parameter(v, parameter);
      stp_string_list_t *vptr = desc->bounds.str;
      size_t count = 0;
      int i;
      stp_dprintf(STP_DBG_VARS, v, "     value %s\n",
		  checkval ? checkval : "(null)");
      if (vptr)
	count = stp_string_list_count(vptr);
      answer = PARAMETER_BAD;
      if (checkval == NULL)
	{
	  if (count == 0)
	    answer = PARAMETER_OK;
	  else
	    {
	      if (!quiet)
		stp_eprintf(v, _("Value must be set for %s\n"), parameter);
	      answer = PARAMETER_BAD;
	    }
	}
      else if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(checkval, stp_string_list_param(vptr, i)->name))
	      {
		answer = PARAMETER_OK;
		break;
	      }
	  if (!answer && !quiet)
	    stp_eprintf(v, _("`%s' is not a valid %s\n"), checkval, parameter);
	}
      else if (strlen(checkval) == 0)
	answer = PARAMETER_OK;
      else if (!quiet)
	stp_eprintf(v, _("`%s' is not a valid %s\n"), checkval, parameter);
    }
  stp_parameter_description_destroy(desc);
  return answer;
}

static int
verify_double_param(const stp_vars_t *v, const char *parameter,
		    stp_parameter_t *desc, int quiet)
{
  stp_dprintf(STP_DBG_VARS, v, "    Verifying double %s\n", parameter);
  if (desc->is_mandatory ||
      stp_check_float_parameter(v, parameter, STP_PARAMETER_ACTIVE))
    {
      double checkval = stp_get_float_parameter(v, parameter);
      if (checkval < desc->bounds.dbl.lower ||
	  checkval > desc->bounds.dbl.upper)
	{
	  if (!quiet)
	    stp_eprintf(v, _("%s must be between %f and %f (is %f)\n"),
			parameter, desc->bounds.dbl.lower,
			desc->bounds.dbl.upper, checkval);
	  return PARAMETER_BAD;
	}
    }
  return PARAMETER_OK;
}

static int
verify_int_param(const stp_vars_t *v, const char *parameter,
		 stp_parameter_t *desc, int quiet)
{
  stp_dprintf(STP_DBG_VARS, v, "    Verifying int %s\n", parameter);
  if (desc->is_mandatory ||
      stp_check_int_parameter(v, parameter, STP_PARAMETER_ACTIVE))
    {
      int checkval = stp_get_int_parameter(v, parameter);
      if (checkval < desc->bounds.integer.lower ||
	  checkval > desc->bounds.integer.upper)
	{
	  if (!quiet)
	    stp_eprintf(v, _("%s must be between %d and %d (is %d)\n"),
			parameter, desc->bounds.integer.lower,
			desc->bounds.integer.upper, checkval);
	  stp_parameter_description_destroy(desc);
	  return PARAMETER_BAD;
	}
    }
  stp_parameter_description_destroy(desc);
  return PARAMETER_OK;
}

static int
verify_dimension_param(const stp_vars_t *v, const char *parameter,
		 stp_parameter_t *desc, int quiet)
{
  stp_dprintf(STP_DBG_VARS, v, "    Verifying dimension %s\n", parameter);
  if (desc->is_mandatory ||
      stp_check_dimension_parameter(v, parameter, STP_PARAMETER_ACTIVE))
    {
      int checkval = stp_get_dimension_parameter(v, parameter);
      if (checkval < desc->bounds.dimension.lower ||
	  checkval > desc->bounds.dimension.upper)
	{
	  if (!quiet)
	    stp_eprintf(v, _("%s must be between %d and %d (is %d)\n"),
			parameter, desc->bounds.dimension.lower,
			desc->bounds.dimension.upper, checkval);
	  stp_parameter_description_destroy(desc);
	  return PARAMETER_BAD;
	}
    }
  stp_parameter_description_destroy(desc);
  return PARAMETER_OK;
}

static int
verify_curve_param(const stp_vars_t *v, const char *parameter,
		   stp_parameter_t *desc, int quiet)
{
  stp_parameter_verify_t answer = 1;
  stp_dprintf(STP_DBG_VARS, v, "    Verifying curve %s\n", parameter);
  if (desc->bounds.curve &&
      (desc->is_mandatory ||
       stp_check_curve_parameter(v, parameter, STP_PARAMETER_ACTIVE)))
    {
      const stp_curve_t *checkval = stp_get_curve_parameter(v, parameter);
      if (checkval)
	{
	  double u0, l0;
	  double u1, l1;
	  stp_curve_get_bounds(checkval, &l0, &u0);
	  stp_curve_get_bounds(desc->bounds.curve, &l1, &u1);
	  if (u0 > u1 || l0 < l1)
	    {
	      if (!quiet)
		stp_eprintf(v, _("%s bounds must be between %f and %f\n"),
			    parameter, l1, u1);
	      answer = PARAMETER_BAD;
	    }
	  if (stp_curve_get_wrap(checkval) !=
	      stp_curve_get_wrap(desc->bounds.curve))
	    {
	      if (!quiet)
		stp_eprintf(v, _("%s wrap mode must be %s\n"),
			    parameter,
			    (stp_curve_get_wrap(desc->bounds.curve) ==
			     STP_CURVE_WRAP_NONE) ?
			    _("no wrap") : _("wrap around"));
	      answer = PARAMETER_BAD;
	    }
	}
    }
  stp_parameter_description_destroy(desc);
  return answer;
}

stp_parameter_verify_t
stp_verify_parameter(const stp_vars_t *v, const char *parameter,
		     int quiet)
{
  stp_parameter_t desc;
  quiet = 0;
  stp_describe_parameter(v, parameter, &desc);
  stp_dprintf(STP_DBG_VARS, v, "  Verifying %s %d %d\n", parameter,
	      desc.is_active, desc.read_only);
  if (!desc.is_active || desc.read_only)
    {
      stp_parameter_description_destroy(&desc);
      return PARAMETER_INACTIVE;
    }
  switch (desc.p_type)
    {
    case STP_PARAMETER_TYPE_STRING_LIST:
      return verify_string_param(v, parameter, &desc, quiet);
    case STP_PARAMETER_TYPE_DOUBLE:
      return verify_double_param(v, parameter, &desc, quiet);
    case STP_PARAMETER_TYPE_INT:
      return verify_int_param(v, parameter, &desc, quiet);
    case STP_PARAMETER_TYPE_DIMENSION:
      return verify_dimension_param(v, parameter, &desc, quiet);
    case STP_PARAMETER_TYPE_CURVE:
      return verify_curve_param(v, parameter, &desc, quiet);
    case STP_PARAMETER_TYPE_RAW:
    case STP_PARAMETER_TYPE_FILE:
      stp_parameter_description_destroy(&desc);
      return PARAMETER_OK;		/* No way to verify this here */
    case STP_PARAMETER_TYPE_BOOLEAN:
      stp_parameter_description_destroy(&desc);
      return PARAMETER_OK;		/* Booleans always OK */
    default:
      if (!quiet)
	stp_eprintf(v, _("Unknown type parameter %s (%d)\n"),
		    parameter, desc.p_type);
      stp_parameter_description_destroy(&desc);
      return 0;
    }
}

#define CHECK_INT_RANGE(v, component, min, max)				    \
do									    \
{									    \
  if (stp_get_##component((v)) < (min) || stp_get_##component((v)) > (max)) \
    {									    \
      answer = 0;							    \
      stp_eprintf(v, _("%s out of range (value %d, min %d, max %d)\n"),     \
		  #component, stp_get_##component(v), min, max);	    \
    }									    \
} while (0)

#define CHECK_INT_RANGE_INTERNAL(v, component, min, max)		      \
do									      \
{									      \
  if (stpi_get_##component((v)) < (min) || stpi_get_##component((v)) > (max)) \
    {									      \
      answer = 0;							      \
      stp_eprintf(v, _("%s out of range (value %d, min %d, max %d)\n"),       \
		  #component, stpi_get_##component(v), min, max);	      \
    }									      \
} while (0)

typedef struct
{
  char *data;
  size_t bytes;
} errbuf_t;

static void
fill_buffer_writefunc(void *priv, const char *buffer, size_t bytes)
{
  errbuf_t *errbuf = (errbuf_t *) priv;
  if (errbuf->bytes == 0)
    errbuf->data = stp_malloc(bytes + 1);
  else
    errbuf->data = stp_realloc(errbuf->data, errbuf->bytes + bytes + 1);
  memcpy(errbuf->data + errbuf->bytes, buffer, bytes);
  errbuf->bytes += bytes;
  errbuf->data[errbuf->bytes] = '\0';
}

int
stp_verify_printer_params(stp_vars_t *v)
{
  errbuf_t errbuf;
  stp_outfunc_t ofunc = stp_get_errfunc(v);
  void *odata = stp_get_errdata(v);
  stp_parameter_list_t params;
  int nparams;
  int i;
  int answer = 1;
  int left, top, bottom, right;
  const char *pagesize = stp_get_string_parameter(v, "PageSize");

  stp_dprintf(STP_DBG_VARS, v, "** Entering stp_verify_printer_params(0x%p)\n",
	      (void *) v);

  stp_set_errfunc((stp_vars_t *) v, fill_buffer_writefunc);
  stp_set_errdata((stp_vars_t *) v, &errbuf);

  errbuf.data = NULL;
  errbuf.bytes = 0;

  if (pagesize && strlen(pagesize) > 0)
    {
      if (stp_verify_parameter(v, "PageSize", 0) == 0)
	answer = 0;
    }
  else
    {
      int width, height, min_height, min_width;
      stp_get_size_limit(v, &width, &height, &min_width, &min_height);
      if (stp_get_page_height(v) <= min_height ||
	  stp_get_page_height(v) > height ||
	  stp_get_page_width(v) <= min_width || stp_get_page_width(v) > width)
	{
	  answer = 0;
	  stp_eprintf(v, _("Page size is not valid\n"));
	}
      stp_dprintf(STP_DBG_PAPER, v,
		  "page size max %d %d min %d %d actual %d %d\n",
		  width, height, min_width, min_height,
		  stp_get_page_width(v), stp_get_page_height(v));
    }

  stp_get_imageable_area(v, &left, &right, &bottom, &top);

  stp_dprintf(STP_DBG_PAPER, v,
	      "page      left %d top %d right %d bottom %d\n",
	      left, top, right, bottom);
  stp_dprintf(STP_DBG_PAPER, v,
	      "requested left %d top %d width %d height %d\n",
	      stp_get_left(v), stp_get_top(v),
	      stp_get_width(v), stp_get_height(v));

  if (stp_get_top(v) < top)
    {
      answer = 0;
      stp_eprintf(v, _("Top margin must not be less than %d\n"), top);
    }

  if (stp_get_left(v) < left)
    {
      answer = 0;
      stp_eprintf(v, _("Left margin must not be less than %d\n"), left);
    }

  if (stp_get_height(v) <= 0)
    {
      answer = 0;
      stp_eprintf(v, _("Height must be greater than zero\n"));
    }

  if (stp_get_width(v) <= 0)
    {
      answer = 0;
      stp_eprintf(v, _("Width must be greater than zero\n"));
    }

  if (stp_get_left(v) + stp_get_width(v) > right)
    {
      answer = 0;
      stp_eprintf(v, _("Image is too wide for the page: left margin is %d, width %d, right edge is %d\n"),
		  stp_get_left(v), stp_get_width(v), right);
    }

  if (stp_get_top(v) + stp_get_height(v) > bottom)
    {
      answer = 0;
      stp_eprintf(v, _("Image is too long for the page: top margin is %d, height %d, bottom edge is %d\n"),
		  stp_get_top(v), stp_get_height(v), bottom);
    }

  params = stp_get_parameter_list(v);
  nparams = stp_parameter_list_count(params);
  for (i = 0; i < nparams; i++)
    {
      const stp_parameter_t *param = stp_parameter_list_param(params, i);
      stp_dprintf(STP_DBG_VARS, v, "Checking %s %d %d\n", param->name,
		  param->is_active, param->verify_this_parameter);

      if (strcmp(param->name, "PageSize") != 0 &&
	  param->is_active && param->verify_this_parameter &&
	  stp_verify_parameter(v, param->name, 0) == 0)
	answer = 0;
    }
  stp_parameter_list_destroy(params);
  stp_set_errfunc((stp_vars_t *) v, ofunc);
  stp_set_errdata((stp_vars_t *) v, odata);
  stp_set_verified((stp_vars_t *) v, answer);
  if (errbuf.bytes > 0)
    {
      stp_eprintf(v, "%s", errbuf.data);
      stp_free(errbuf.data);
    }
  stp_dprintf(STP_DBG_VARS, v, "** Exiting stp_verify_printer_params(0x%p) => %d\n",
	      (void *) v, answer);
  return answer;
}

static const stp_vars_t *
stp_find_params(const char *name, const char *family)
{
  if (printvars_list)
    {
      char *stmp =
	stp_malloc(strlen(family) + strlen("::") + strlen(name) + 1);
      stp_list_item_t *item;
      strcpy(stmp, family);
      strcat(stmp, "::");
      strcat(stmp, name);
      item = stp_list_get_item_by_name(printvars_list, stmp);
      if (item)
	{
	  stp_free(stmp);
	  return ((const stp_printvars_t *)
		  stp_list_item_get_data(item))->printvars;
	}
      strcpy(stmp, name);
      item = stp_list_get_item_by_name(printvars_list, stmp);
      stp_free(stmp);
      if (item)
	return ((const stp_printvars_t *)
		stp_list_item_get_data(item))->printvars;
    }
  return NULL;
}

int
stp_family_register(stp_list_t *family)
{
  stp_list_item_t *printer_item;
  const stp_printer_t *printer;

  if (printer_list == NULL)
    {
      stpi_init_printer_list();
      stp_deprintf
	(STP_DBG_PRINTERS,
	 "stpi_family_register(): initialising printer_list...\n");
    }

  if (family)
    {
      printer_item = stp_list_get_start(family);

      while(printer_item)
	{
	  printer = (const stp_printer_t *) stp_list_item_get_data(printer_item);
	  if (!stp_list_get_item_by_name(printer_list, printer->driver))
	    stp_list_item_create(printer_list, NULL, printer);
	  printer_item = stp_list_item_next(printer_item);
	}
    }

  return 0;
}

int
stp_family_unregister(stp_list_t *family)
{
  stp_list_item_t *printer_item;
  stp_list_item_t *old_printer_item;
  const stp_printer_t *printer;

  if (printer_list == NULL)
    {
      stpi_init_printer_list();
      stp_deprintf
	(STP_DBG_PRINTERS,
	 "stpi_family_unregister(): initialising printer_list...\n");
    }

  if (family)
    {
      printer_item = stp_list_get_start(family);

      while(printer_item)
	{
	  printer = (const stp_printer_t *) stp_list_item_get_data(printer_item);
	  old_printer_item =
	    stp_list_get_item_by_name(printer_list, printer->driver);

	  if (old_printer_item)
	    stp_list_item_destroy(printer_list, old_printer_item);
	  printer_item = stp_list_item_next(printer_item);
	}
    }
  return 0;
}

static stp_printvars_t *
stp_printvars_create_from_xmltree(stp_mxml_node_t *printer,
				  const char *family)
{
  stp_mxml_node_t *prop;	/* Temporary node pointer */
  const char *stmp;		/* Temporary string */
  char *sbuf;
  stp_printvars_t *outprintvars;
  outprintvars = stp_zalloc(sizeof(stp_printvars_t));
  if (!outprintvars)
    return NULL;
  outprintvars->printvars = stp_vars_create();
  if (outprintvars->printvars == NULL)
    {
      stp_free(outprintvars);
      return NULL;
    }
  stmp = stp_mxmlElementGetAttr(printer, "name");
  if (!stmp)
    {
      stp_vars_destroy(outprintvars->printvars);
      stp_free(outprintvars);
      return NULL;
    }
  sbuf = stp_malloc(strlen(family) + strlen("::") + strlen(stmp) + 1);
  strcpy(sbuf, family);
  strcat(sbuf, "::");
  strcat(sbuf, stmp);
  outprintvars->name = sbuf;
  prop = printer->child;
  stp_deprintf(STP_DBG_XML, ">>stp_printvars_create_from_xmltree: %p, %s\n",
	       (void *) (outprintvars->printvars), outprintvars->name);
  stp_vars_fill_from_xmltree(prop, outprintvars->printvars);
  stp_deprintf(STP_DBG_XML, "<<stp_printvars_create_from_xmltree: %p, %s\n",
	       (void *) (outprintvars->printvars), outprintvars->name);
  return outprintvars;
}


/*
 * Parse the printer node, and return the generated printer.  Returns
 * NULL on failure.
 */
static stp_printer_t*
stp_printer_create_from_xmltree(stp_mxml_node_t *printer, /* The printer node */
				const char *family,       /* Family name */
				const stp_printfuncs_t *printfuncs)
                                                       /* Family printfuncs */
{
  stp_mxml_node_t *prop;	/* Temporary node pointer */
  const char *stmp;		/* Temporary string */
  stp_printer_t *outprinter;	/* Generated printer */
  int
    driver = 0,			/* Check driver */
    long_name = 0;

  outprinter = stp_zalloc(sizeof(stp_printer_t));
  if (!outprinter)
    return NULL;
  stmp = stp_mxmlElementGetAttr(printer, "parameters");
  if (stmp && !stp_find_params(stmp, family))
    stp_erprintf("stp_printer_create_from_xmltree: cannot find parameters %s::%s\n",
		 family, stmp);
  if (stmp && stp_find_params(stmp, family))
    outprinter->printvars = stp_vars_create_copy(stp_find_params(stmp, family));
  else
    outprinter->printvars = stp_vars_create();
  if (outprinter->printvars == NULL)
    {
      stp_free(outprinter);
      return NULL;
    }

  stmp = stp_mxmlElementGetAttr(printer, "driver");
  stp_set_driver(outprinter->printvars, (const char *) stmp);

  outprinter->long_name = stp_strdup(stp_mxmlElementGetAttr(printer, "name"));
  outprinter->manufacturer = stp_strdup(stp_mxmlElementGetAttr(printer, "manufacturer"));
  outprinter->model = stp_xmlstrtol(stp_mxmlElementGetAttr(printer, "model"));
  outprinter->family = stp_strdup((const char *) family);
  stmp = stp_mxmlElementGetAttr(printer, "deviceid");
  if (stmp)
    outprinter->device_id = stp_strdup(stmp);
  stmp = stp_mxmlElementGetAttr(printer, "foomaticid");
  if (stmp)
    outprinter->foomatic_id = stp_strdup(stmp);

  if (stp_get_driver(outprinter->printvars))
    driver = 1;
  if (outprinter->long_name)
    long_name = 1;

  outprinter->printfuncs = printfuncs;

  prop = printer->child;
  stp_vars_fill_from_xmltree(prop, outprinter->printvars);
  if (driver && long_name && printfuncs)
    {
      if (stp_get_debug_level() & STP_DBG_XML)
	{
	  stmp = stp_mxmlElementGetAttr(printer, "driver");
	  stp_erprintf("stp_printer_create_from_xmltree: printer: %s\n", stmp);
	}
      outprinter->driver = stp_get_driver(outprinter->printvars);
      return outprinter;
    }
  stp_free(outprinter);
  return NULL;
}

/*
 * Parse the <family> node.
 */
static void
stpi_xml_process_family(stp_mxml_node_t *family)     /* The family node */
{
  stp_list_t *family_module_list = NULL;      /* List of valid families */
  stp_list_item_t *family_module_item;        /* Current family */
  const char *family_name;                       /* Name of family */
  stp_mxml_node_t *printer;                         /* printer child node */
  stp_module_t *family_module_data;           /* Family module data */
  stp_family_t *family_data = NULL;  /* Family data */
  int family_valid = 0;                       /* Is family valid? */

  family_module_list = stp_module_get_class(STP_MODULE_CLASS_FAMILY);
  if (!family_module_list)
    return;

  family_name = stp_mxmlElementGetAttr(family, "name");
  family_module_item = stp_list_get_start(family_module_list);
  while (family_module_item)
    {
      family_module_data = (stp_module_t *)
	stp_list_item_get_data(family_module_item);
      if (!strcmp(family_name, family_module_data->name))
	{
	  stp_deprintf(STP_DBG_XML,
		       "stpi_xml_process_family: family module: %s\n",
		       family_module_data->name);
	  family_data = family_module_data->syms;
	  if (family_data->printer_list == NULL)
	    family_data->printer_list = stp_list_create();
	  family_valid = 1;
	}
      family_module_item = stp_list_item_next(family_module_item);
    }

  printer = family->child;
  while (family_valid && printer)
    {
      if (printer->type == STP_MXML_ELEMENT)
	{
	  const char *printer_name = printer->value.element.name;
	  if (!strcmp(printer_name, "printer"))
	    {
	      stp_printer_t *outprinter =
		stp_printer_create_from_xmltree(printer, family_name,
						family_data->printfuncs);
	      if (outprinter)
		stp_list_item_create(family_data->printer_list, NULL,
				      outprinter);
	    }
	  else if (!strcmp(printer_name, "parameters"))
	    {
	      stp_printvars_t *printvars =
		stp_printvars_create_from_xmltree(printer, family_name);
	      if (printvars)
		{
		  stpi_init_printvars_list();
		  stp_list_item_create(printvars_list, NULL, printvars);
		}
	    }
	}
      printer = printer->next;
    }

  stp_list_destroy(family_module_list);
  return;
}

/*
 * Parse the <printdef> node.
 */
static int
stpi_xml_process_printdef(stp_mxml_node_t *printdef, const char *file) /* The printdef node */
{
  stp_mxml_node_t *family;                          /* Family child node */

  family = printdef->child;
  while (family)
    {
      if (family->type == STP_MXML_ELEMENT)
	{
	  const char *family_name = family->value.element.name;
	  if (!strcmp(family_name, "family"))
	    {
	      stpi_xml_process_family(family);
	    }
	}
      family = family->next;
    }
  return 1;
}

void
stpi_init_printer(void)
{
  stp_register_xml_parser("printdef", stpi_xml_process_printdef);
  stp_register_xml_preload("printers.xml");
}
