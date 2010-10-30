/*
 * "$Id: printers.h,v 1.7 2010/08/07 02:30:38 rlk Exp $"
 *
 *   libgimpprint printer functions.
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

/**
 * @file gutenprint/printers.h
 * @brief Printer functions.
 */

#ifndef GUTENPRINT_PRINTERS_H
#define GUTENPRINT_PRINTERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gutenprint/string-list.h>
#include <gutenprint/list.h>
#include <gutenprint/vars.h>

/**
 * The printer type represents a printer model.  A particular
 * printer model must selected in order to be able to print.  Each
 * printer model provides default print options through a default
 * vars object.
 *
 * @defgroup printer printer
 * @{
 */

struct stp_printer;
/** The printer opaque data type (representation of printer model). */
typedef struct stp_printer stp_printer_t;

/**
 * Get the number of available printer models.
 * @returns the number of printer models.
 */
extern int stp_printer_model_count(void);

/**
 * Get a printer model by its index number.
 * @param idx the index number.  This must not be greater than (total
 * number of printers - 1).
 * @returns a pointer to the printer model, or NULL on failure.  The
 * pointer should not be freed.
 */
extern const stp_printer_t *stp_get_printer_by_index(int idx);

/**
 * Get a printer model by its long (translated) name.
 * @param long_name the printer model's long (translated) name.
 * @returns a pointer to the printer model, or NULL on failure.  The
 * pointer should not be freed.
 */
extern const stp_printer_t *stp_get_printer_by_long_name(const char *long_name);

/**
 * Get a printer model by its short name.
 * @param driver the printer model's short (driver) name.
 * @returns a pointer to the printer model, or NULL on failure.  The
 * pointer should not be freed.
 */
extern const stp_printer_t *stp_get_printer_by_driver(const char *driver);

/**
 * Get a printer model by its IEEE 1284 device ID.
 * @param device_id the printer model's device ID.
 * @returns a pointer to the printer model, or NULL on failure.  The
 * pointer should not be freed.
 */
extern const stp_printer_t *stp_get_printer_by_device_id(const char *device_id);

/**
 * Get a printer model by its foomatic ID.
 * @param foomatic_id the printer model's foomatic ID
 * @returns a pointer to the printer model, or NULL on failure.  The
 * pointer should not be freed.
 */
extern const stp_printer_t *stp_get_printer_by_foomatic_id(const char *foomatic_id);

/**
 * Get the printer model from a vars object.
 * @param v the vars to use.
 * @returns a pointer to the printer model, or NULL on failure.  The
 * pointer should not be freed.
 */
extern const stp_printer_t *stp_get_printer(const stp_vars_t *v);

/**
 * Get the printer index number from the printer model short (driver) name.
 * @deprecated There should never be any need to use this function.
 * @param driver the printer model's short (driver) name.
 * @returns the index number, or -1 on failure.
 */
extern int stp_get_printer_index_by_driver(const char *driver);

/**
 * Get a printer model's long (translated) name.
 * @param p the printer model to use.
 * @returns the long name (should never be freed).
 */
extern const char *stp_printer_get_long_name(const stp_printer_t * p);

/**
 * Get a printer model's short (driver) name.
 * @param p the printer model to use.
 * @returns the short name (should never be freed).
 */
extern const char *stp_printer_get_driver(const stp_printer_t *p);

/**
 * Get a printer model's IEEE 1284 device ID, if known.
 * @param p the printer model to use.
 * @returns the IEEE 1284 device ID, or NULL if not known.
 */
extern const char *stp_printer_get_device_id(const stp_printer_t *p);

/**
 * Get a printer model's family name.
 * The family name is the name of the modular "family" driver this
 * model uses.
 * @param p the printer model to use.
 * @returns the family name (should never be freed).
 */
extern const char *stp_printer_get_family(const stp_printer_t *p);

/**
 * Get a printer model's manufacturer's name.
 * @param p the printer model to use.
 * @returns the manufacturer's name (should never be freed).
 */
extern const char *stp_printer_get_manufacturer(const stp_printer_t *p);

/**
 * Get a printer model's foomatic ID
 * @param p the printer model to use.
 * @returns the foomatic ID or NULL (should never be freed)
 */
extern const char *stp_printer_get_foomatic_id(const stp_printer_t *p);

/**
 * Get a printer model's model number.
 * The model number is used internally by the "family" driver module,
 * and has no meaning out of that context.  It bears no relation to
 * the model name/number actually found on the printer itself.
 * @param p the printer model to use.
 * @returns the model number.
 */
extern int stp_printer_get_model(const stp_printer_t *p);

/**
 * Get the default vars for a particular printer model.
 * The default vars should be copied to a new vars object and
 * customised prior to printing.
 * @param p the printer model to use.
 * @returns the printer model's default vars.
 */
extern const stp_vars_t *stp_printer_get_defaults(const stp_printer_t *p);

/**
 * Set a vars object to use a particular driver, and set the parameters
 * to their defaults.
 * @param v the vars to use.
 * @param p the printer model to use.
 */
extern void stp_set_printer_defaults(stp_vars_t *v, const stp_printer_t *p);

/**
 * Set a vars object to use a particular driver, and set any unset parameters
 * to their defaults.
 * @param v the vars to use.
 * @param p the printer model to use.
 */
extern void stp_set_printer_defaults_soft(stp_vars_t *v, const stp_printer_t *p);


/**
 * Print the image.
 * @warning stp_job_start() must be called prior to the first call to
 * this function.
 * @param v the vars to use.
 * @param image the image to print.
 * @returns 0 on failure, 1 on success, 2 on abort requested by the
 * driver.
 */
extern int stp_print(const stp_vars_t *v, stp_image_t *image);

/**
 * Start a print job.
 * @warning This function must be called prior to the first call to
 * stp_print().
 * @param v the vars to use.
 * @param image the image to print.
 * @returns 1 on success, 0 on failure.
 */
extern int stp_start_job(const stp_vars_t *v, stp_image_t *image);

/**
 * End a print job.
 * @param v the vars to use.
 * @param image the image to print.
 * @returns 1 on success, 0 on failure.
 */
extern int stp_end_job(const stp_vars_t *v, stp_image_t *image);

/**
 * Retrieve options that need to be passed to the underlying print
 * system.
 * @param v the vars to use.
 * @returns list of options in a string list ('name' is the name
 * of the option; 'text' is the value it takes on).  NULL return means
 * no external options are required.  User must stp_string_list_destroy
 * the list after use.
 */
extern stp_string_list_t *stp_get_external_options(const stp_vars_t *v);

typedef struct
{
  stp_parameter_list_t (*list_parameters)(const stp_vars_t *v);
  void  (*parameters)(const stp_vars_t *v, const char *name,
		      stp_parameter_t *);
  void  (*media_size)(const stp_vars_t *v, int *width, int *height);
  void  (*imageable_area)(const stp_vars_t *v,
			  int *left, int *right, int *bottom, int *top);
  void  (*maximum_imageable_area)(const stp_vars_t *v, int *left, int *right,
				  int *bottom, int *top);
  void  (*limit)(const stp_vars_t *v, int *max_width, int *max_height,
                 int *min_width, int *min_height);
  int   (*print)(const stp_vars_t *v, stp_image_t *image);
  void  (*describe_resolution)(const stp_vars_t *v, int *x, int *y);
  const char *(*describe_output)(const stp_vars_t *v);
  int   (*verify)(stp_vars_t *v);
  int   (*start_job)(const stp_vars_t *v, stp_image_t *image);
  int   (*end_job)(const stp_vars_t *v, stp_image_t *image);
  stp_string_list_t *(*get_external_options)(const stp_vars_t *v);
} stp_printfuncs_t;

typedef struct stp_family
{
  const stp_printfuncs_t *printfuncs;   /* printfuncs for the printer */
  stp_list_t             *printer_list; /* list of printers */
} stp_family_t;

extern int stp_get_model_id(const stp_vars_t *v);

extern int stp_verify_printer_params(stp_vars_t *v);

extern int stp_family_register(stp_list_t *family);
extern int stp_family_unregister(stp_list_t *family);
extern void stp_initialize_printer_defaults(void);

extern stp_parameter_list_t stp_printer_list_parameters(const stp_vars_t *v);

extern void
stp_printer_describe_parameter(const stp_vars_t *v, const char *name,
			       stp_parameter_t *description);

const char *stp_describe_output(const stp_vars_t *v);

/** @} */

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_PRINTERS_H */
/*
 * End of "$Id: printers.h,v 1.7 2010/08/07 02:30:38 rlk Exp $".
 */
