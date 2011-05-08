/*
 * "$id: vars.h,v 1.3.4.4 2004/03/09 03:00:25 rlk Exp $"
 *
 *   libgimpprint stp_vars_t core functions.
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
 * @file gutenprint/vars.h
 * @brief Print job functions.
 */

#ifndef GUTENPRINT_VARS_H
#define GUTENPRINT_VARS_H

#include <gutenprint/array.h>
#include <gutenprint/curve.h>
#include <gutenprint/string-list.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The vars data type contains all the information about a print job,
 * this includes information such as the printer model, paper size,
 * print resolution etc.  Most of these job options are expressed as
 * parameters which vary according to the model and other options
 * selected.
 *
 * The representation of printer settings has changed dramatically from 4.2.
 * All (well most, anyway) settings outside of basics such as the printer
 * model and sizing settings are now typed parameters.
 *
 * @defgroup vars vars
 * @{
 */

struct stp_vars;
/** The vars opaque data type. */
typedef struct stp_vars stp_vars_t;

/**
 * Parameter types.
 * The following types are permitted for a printer setting.  Not all
 * are currently implemented.
 */
typedef enum
{
  STP_PARAMETER_TYPE_STRING_LIST, /*!< Single string choice from a list. */
  STP_PARAMETER_TYPE_INT,	/*!< Integer. */
  STP_PARAMETER_TYPE_BOOLEAN,	/*!< Boolean. */
  STP_PARAMETER_TYPE_DOUBLE,	/*!< Floating point number. */
  STP_PARAMETER_TYPE_CURVE,	/*!< Curve. */
  STP_PARAMETER_TYPE_FILE,	/*!< Filename (NYI, need to consider security). */
  STP_PARAMETER_TYPE_RAW,	/*!< Raw, opaque data. */
  STP_PARAMETER_TYPE_ARRAY,     /*!< Array. */
  STP_PARAMETER_TYPE_DIMENSION, /*!< Linear dimension. */
  STP_PARAMETER_TYPE_INVALID    /*!< Invalid type (should never be used). */
} stp_parameter_type_t;

/**
 * Parameter class.
 * What kind of setting this is, for the purpose of user interface
 * representation.
 */
typedef enum
{
  STP_PARAMETER_CLASS_FEATURE,	/*!< Printer feature. */
  STP_PARAMETER_CLASS_OUTPUT,	/*!< Output control. */
  STP_PARAMETER_CLASS_CORE,	/*!< Core Gimp-Print parameter. */
  STP_PARAMETER_CLASS_INVALID   /*!< Invalid class (should never be used). */
} stp_parameter_class_t;

/**
 * Parameter level.
 * What "level" a setting is at, for UI design.
 */
typedef enum
{
  STP_PARAMETER_LEVEL_BASIC,     /*!< Basic parameter, shown by all UIs. */
  STP_PARAMETER_LEVEL_ADVANCED,  /*!< Advanced parameter, shown by advanced UIs. */
  STP_PARAMETER_LEVEL_ADVANCED1, /*!< Advanced1 parameter, shown by advanced UIs. */
  STP_PARAMETER_LEVEL_ADVANCED2, /*!< Advanced2 parameter, shown by advanced UIs. */
  STP_PARAMETER_LEVEL_ADVANCED3, /*!< Advanced3 parameter, shown by advanced UIs. */
  STP_PARAMETER_LEVEL_ADVANCED4, /*!< Advanced4 parameter, shown by advanced UIs. */
  STP_PARAMETER_LEVEL_INTERNAL,	 /*!< Parameters used only within Gimp-Print. */
  STP_PARAMETER_LEVEL_EXTERNAL,	 /*!< Parameters used only outside Gimp-Print. */
  STP_PARAMETER_LEVEL_INVALID    /*!< Invalid level (should never be used). */
} stp_parameter_level_t;

/**
 * Parameter activity.
 * Whether a parameter is currently active (i. e. whether its value
 * should be used by the driver or not).  All parameters default to being
 * active unless explicitly "turned off".
 */
typedef enum
{
  STP_PARAMETER_INACTIVE,  /*!< Parameter is inactive (unused). */
  STP_PARAMETER_DEFAULTED, /*!< Parameter is set to its default value. */
  STP_PARAMETER_ACTIVE     /*!< Parameter is active (used). */
} stp_parameter_activity_t;

/*
 * Other parameter types
 */

/** Raw parameter. */
typedef struct
{
  size_t bytes;     /*!< Size of data. */
  const void *data; /*!< Raw data. */
} stp_raw_t;

#define STP_RAW(x) { sizeof((x)), (x) }
#define STP_RAW_STRING(x) { sizeof((x)) - 1, (x) }

/** double_bound (range) parameter. */
typedef struct
{
  double lower; /*!< Lower bound. */
  double upper; /*!< Upper bound. */
} stp_double_bound_t;

/** int_bound (range) parameter. */
typedef struct
{
  int lower; /*!< Lower bound. */
  int upper; /*!< Upper bound. */
} stp_int_bound_t;

#define STP_CHANNEL_NONE ((unsigned char) -1)

/** Parameter description. */
typedef struct
{
  const char *name;		 /*!< Internal name (key). */
  const char *text;		 /*!< User-visible name. */
  const char *category;		 /*!< User-visible category name. */
  const char *help;		 /*!< Help string. */
  stp_parameter_type_t p_type;   /*!< Parameter type. */
  stp_parameter_class_t p_class; /*!< Parameter class. */
  stp_parameter_level_t p_level; /*!< Parameter level. */
  unsigned char is_mandatory;    /*!< The parameter is required, even when set inactive. */
  unsigned char is_active;       /*!< Is the parameter active? */
  unsigned char channel;         /*!< The channel to which this parameter applies */
  unsigned char verify_this_parameter;	 /*!< Should the verify system check this parameter? */
  unsigned char read_only;
  union
  {
    stp_curve_t *curve;       /*!< curve parameter value. */
    stp_double_bound_t dbl;  /*!< double_bound parameter value. */
    stp_int_bound_t integer; /*!< int_bound parameter value. */
    stp_int_bound_t dimension; /*!< int_bound parameter value. */
    stp_string_list_t *str;   /*!< string_list parameter value. */
    stp_array_t *array;      /*!< array parameter value. */
  } bounds; /*!< Limits on the values the parameter may take. */
  union
  {
    stp_curve_t *curve; /*!< Default curve parameter value. */
    double dbl;         /*!< Default double parameter value. */
    int dimension;      /*!< Default dimension parameter value. */
    int integer;        /*!< Default int parameter value. */
    int boolean;        /*!< Default boolean parameter value. */
    const char *str;    /*!< Default string parameter value. */
    stp_array_t *array; /*!< Default array parameter value. */
  } deflt; /*!< Default value of the parameter. */
} stp_parameter_t;

/** The parameter_list opaque data type. */
typedef void *stp_parameter_list_t;
/** The constant parameter_list opaque data type. */
typedef const void *stp_const_parameter_list_t;

/**
 * Output function supplied by the calling application.
 * There are two output functions supplied by the caller, one to send
 * output data and one to report errors.
 * @param data a pointer to an opaque object owned by the calling
 *             application.
 * @param buffer the data to output.
 * @param bytes the size of buffer (in bytes).
 */
typedef void (*stp_outfunc_t) (void *data, const char *buffer, size_t bytes);


/****************************************************************
*                                                               *
* BASIC PRINTER SETTINGS                                        *
*                                                               *
****************************************************************/

/**
 * Create a new vars object.
 * @returns the newly created vars object.
 */
extern stp_vars_t *stp_vars_create(void);

/**
 * Copy a vars object.
 * Both dest and source must be valid vars objects previously
 * created with stp_vars_create().
 * @param dest the destination vars.
 * @param source the source vars.
 */
extern void stp_vars_copy(stp_vars_t *dest, const stp_vars_t *source);

/**
 * Copy and allocate a vars object.
 * source must be a valid vars object previously created with
 * stp_vars_create().
 * @param source the source vars.
 * @returns the new copy of the vars.
 */
extern stp_vars_t *stp_vars_create_copy(const stp_vars_t *source);

/**
 * Destroy a vars object.
 * It is an error to destroy the vars more than once.
 * @param v the vars to destroy.
 */
extern void stp_vars_destroy(stp_vars_t *v);

/**
 * Set the name of the printer driver.
 * @param v the vars to use.
 * @param val the name to set.
 */
extern void stp_set_driver(stp_vars_t *v, const char *val);

/**
 * Set the name of the printer driver.
 * @param v the vars to use.
 * @param val the name to set.
 * @param bytes the length of val (in bytes).
 */
extern void stp_set_driver_n(stp_vars_t *v, const char *val, int bytes);

/**
 * Get the name of the printer driver.
 * @returns the name of the printer driver (must not be freed).
 */
extern const char *stp_get_driver(const stp_vars_t *v);

/**
 * Set the name of the color conversion routine, if not the default.
 * @param v the vars to use.
 * @param val the name to set.
 */
extern void stp_set_color_conversion(stp_vars_t *v, const char *val);

/**
 * Set the name of the color conversion routine, if not the default.
 * @param v the vars to use.
 * @param val the name to set.
 * @param bytes the length of val (in bytes).
 */
extern void stp_set_color_conversion_n(stp_vars_t *v, const char *val, int bytes);

/**
 * Get the name of the color conversion routine.
 * @returns the name of the color conversion routine (must not be freed).
 */
extern const char *stp_get_color_conversion(const stp_vars_t *v);

/*
 * Set/get the position and size of the image
 */

/**
 * Set the left edge of the image.
 * @param v the vars to use.
 * @param val the value to set.
 */
extern void stp_set_left(stp_vars_t *v, int val);

/**
 * Get the left edge of the image.
 * @returns the left edge.
 */
extern int stp_get_left(const stp_vars_t *v);

/**
 * Set the top edge of the image.
 * @param v the vars to use.
 * @param val the value to set.
 */
extern void stp_set_top(stp_vars_t *v, int val);

/**
 * Get the top edge of the image.
 * @returns the left edge.
 */
extern int stp_get_top(const stp_vars_t *v);

/**
 * Set the width of the image.
 * @param v the vars to use.
 * @param val the value to set.
 */
extern void stp_set_width(stp_vars_t *v, int val);

/**
 * Get the width edge of the image.
 * @returns the left edge.
 */
extern int stp_get_width(const stp_vars_t *v);

/**
 * Set the height of the image.
 * @param v the vars to use.
 * @param val the value to set.
 */
extern void stp_set_height(stp_vars_t *v, int val);

/**
 * Get the height of the image.
 * @returns the left edge.
 */
extern int stp_get_height(const stp_vars_t *v);

/*
 * For custom page widths, these functions may be used.
 */

/**
 * Set the page width.
 * @param v the vars to use.
 * @param val the value to set.
 */
extern void stp_set_page_width(stp_vars_t *v, int val);

/**
 * Get the page width.
 * @returns the page width.
 */
extern int stp_get_page_width(const stp_vars_t *v);

/**
 * Set the page height.
 * @param v the vars to use.
 * @param val the value to set.
 */
extern void stp_set_page_height(stp_vars_t *v, int val);

/**
 * Get the page height.
 * @returns the page height.
 */
extern int stp_get_page_height(const stp_vars_t *v);

/**
 * Set the function used to print output information.
 * These must be supplied by the caller.  outdata is passed as an
 * arguments to outfunc; typically it will be a file descriptor.
 * @param v the vars to use.
 * @param val the value to set.
 */
extern void stp_set_outfunc(stp_vars_t *v, stp_outfunc_t val);

/**
 * Get the function used to print output information.
 * @param v the vars to use.
 * @returns the outfunc.
 */
extern stp_outfunc_t stp_get_outfunc(const stp_vars_t *v);

/**
 * Set the function used to print error and diagnostic information.
 * These must be supplied by the caller.  errdata is passed as an
 * arguments to errfunc; typically it will be a file descriptor.
 * @param v the vars to use.
 * @param val the value to set.
 */
extern void stp_set_errfunc(stp_vars_t *v, stp_outfunc_t val);

/**
 * Get the function used to print output information.
 * @param v the vars to use.
 * @returns the outfunc.
 */
extern stp_outfunc_t stp_get_errfunc(const stp_vars_t *v);

/**
 * Set the output data.
 * @param v the vars to use.
 * @param val the output data.  This will typically be a file
 * descriptor, but it is entirely up to the caller exactly what type
 * this might be.
 */
extern void stp_set_outdata(stp_vars_t *v, void *val);

/**
 * Get the output data.
 * @param v the vars to use.
 * @returns the output data.
 */
extern void *stp_get_outdata(const stp_vars_t *v);

/**
 * Set the error data.
 * @param v the vars to use.
 * @param val the error data.  This will typically be a file
 * descriptor, but it is entirely up to the caller exactly what type
 * this might be.
 */
extern void stp_set_errdata(stp_vars_t *v, void *val);

/**
 * Get the error data.
 * @param v the vars to use.
 * @returns the output data.
 */
extern void *stp_get_errdata(const stp_vars_t *v);

/**
 * Merge defaults for a printer with user-chosen settings.
 * @deprecated This is likely to go away.
 * @param user the destination vars.
 * @param print the vars to merge into user.
 */
extern void stp_merge_printvars(stp_vars_t *user, const stp_vars_t *print);


/****************************************************************
*                                                               *
* PARAMETER MANAGEMENT                                          *
*                                                               *
****************************************************************/

/**
 * List the available parameters for the currently chosen settings.
 * This does not fill in the bounds and defaults; it merely provides
 * a list of settings.  To fill in detailed information for a setting,
 * use stp_describe_parameter.
 * @param v the vars to use.
 * @returns a list of available parameters (must be freed with
 * stp_parameter_list_destroy()).
 */
extern stp_parameter_list_t stp_get_parameter_list(const stp_vars_t *v);

/**
 * List the number of available parameters for the currently chosen
 * settings.
 * @param list the parameter_list to use.
 * @returns the number of parameters.
 */
extern size_t stp_parameter_list_count(stp_const_parameter_list_t list);

/**
 * Find a parameter by its name.
 * @param list the parameter_list to use.
 * @param name the name of the parameter.
 * @returns a pointer to the parameter (must not be freed), or NULL if
 * no parameter was found.
 */
extern const stp_parameter_t *
stp_parameter_find(stp_const_parameter_list_t list, const char *name);

/**
 * Find a parameter by its index number.
 * @param list the parameter_list to use.
 * @param item the index number of the parameter (must not be greater
 * than stp_parameter_list_count - 1).
 * @returns a pointer to the parameter (must not be freed), or NULL if
 * no parameter was found.
 */
extern const stp_parameter_t *
stp_parameter_list_param(stp_const_parameter_list_t list, size_t item);

/**
 * Destroy a parameter_list.
 * It is an error to destroy the parameter_list more than once.
 * @param list the parameter_list to destroy.
 */
extern void stp_parameter_list_destroy(stp_parameter_list_t list);

/**
 * Create a parameter_list.
 * @returns the newly created parameter_list.
 */
extern stp_parameter_list_t stp_parameter_list_create(void);

/**
 * Add a parameter to a parameter_list.
 * @param list the parameter_list to use.
 * @param item the parameter to add.
 */
extern void stp_parameter_list_add_param(stp_parameter_list_t list,
					 const stp_parameter_t *item);

/**
 * Copy and allocate a parameter_list.
 * A new parameter_list will be created, and then the contents of
 * source will be
 * copied into it.
 * @param list the source parameter_list.
 * @returns the new copy of the parameter_list.
 */
extern stp_parameter_list_t
stp_parameter_list_copy(stp_const_parameter_list_t list);

/**
 * Append one parameter_list to another.
 * @param list the destination list (to append to).
 * @param append the list of paramters to append.  Each item that does
 * not already exist in list will be appended.
 */
extern void
stp_parameter_list_append(stp_parameter_list_t list,
			  stp_const_parameter_list_t append);

/**
 * Describe a parameter in detail.
 * All of the parameter fields will be populated.
 * @param v the vars to use.
 * @param name the name of the parameter.
 * @param description a pointer to an stp_parameter_t to store the
 * parameter description in.
 */
extern void
stp_describe_parameter(const stp_vars_t *v, const char *name,
		       stp_parameter_t *description);

/**
 * Destroy a parameter description.
 * This must be called even if the stp_parameter_t was not allocated
 * with malloc, since some members are dynamically allocated.
 * @param description the parameter description to destroy.
 */
extern void stp_parameter_description_destroy(stp_parameter_t *description);

/**
 * Find a parameter by its name from a vars object.
 * @param v the vars to use.
 * @param name the name of the parameter.
 * @returns a pointer to the parameter (must not be freed), or NULL if
 * no parameter was found.
 */
extern const stp_parameter_t *
stp_parameter_find_in_settings(const stp_vars_t *v, const char *name);

/**
 * Set a string parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 */
extern void stp_set_string_parameter(stp_vars_t *v, const char *parameter,
				     const char *value);

/**
 * Set a string parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set (must not contain NUL).
 * @param bytes the length of value (in bytes).
 */
extern void stp_set_string_parameter_n(stp_vars_t *v, const char *parameter,
				       const char *value, size_t bytes);

/**
 * Set a file parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 */
extern void stp_set_file_parameter(stp_vars_t *v, const char *parameter,
				   const char *value);

/**
 * Set a file parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set (must not contain NUL).
 * @param bytes the length of value (in bytes).
 */
extern void stp_set_file_parameter_n(stp_vars_t *v, const char *parameter,
				     const char *value, size_t bytes);

/**
 * Set a float parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 */
extern void stp_set_float_parameter(stp_vars_t *v, const char *parameter,
				    double value);

/**
 * Set an integer parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 */
extern void stp_set_int_parameter(stp_vars_t *v, const char *parameter,
				  int value);

/**
 * Set a dimension parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 */
extern void stp_set_dimension_parameter(stp_vars_t *v, const char *parameter,
					int value);

/**
 * Set a boolean parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 */
extern void stp_set_boolean_parameter(stp_vars_t *v, const char *parameter,
				      int value);

/**
 * Set a curve parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 */
extern void stp_set_curve_parameter(stp_vars_t *v, const char *parameter,
				    const stp_curve_t *value);

/**
 * Set an array parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 */
extern void stp_set_array_parameter(stp_vars_t *v, const char *parameter,
				    const stp_array_t *value);

/**
 * Set a raw parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 * @param bytes the length of value (in bytes).
 */
extern void stp_set_raw_parameter(stp_vars_t *v, const char *parameter,
				  const void *value, size_t bytes);

/**
 * Multiply the value of a float parameter by a scaling factor.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param scale the factor to multiply the value by.
 */
extern void stp_scale_float_parameter(stp_vars_t *v, const char *parameter,
				      double scale);

/**
 * Set a default string parameter.
 * The value is set if the parameter is not already set.  This avoids
 * having to check if the parameter is set prior to setting it, if you
 * do not want to override the existing value.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 */
extern void stp_set_default_string_parameter(stp_vars_t *v,
					     const char *parameter,
					     const char *value);

/**
 * Set a default string parameter.
 * The value is set if the parameter is not already set.  This avoids
 * having to check if the parameter is set prior to setting it, if you
 * do not want to override the existing value.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set (must not contain NUL).
 * @param bytes the length of value (in bytes).
 */
extern void stp_set_default_string_parameter_n(stp_vars_t *v,
					       const char *parameter,
					       const char *value, size_t bytes);

/**
 * Set a default file parameter.
 * The value is set if the parameter is not already set.  This avoids
 * having to check if the parameter is set prior to setting it, if you
 * do not want to override the existing value.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 */
extern void stp_set_default_file_parameter(stp_vars_t *v,
					   const char *parameter,
					   const char *value);

/**
 * Set a default file parameter.
 * The value is set if the parameter is not already set.  This avoids
 * having to check if the parameter is set prior to setting it, if you
 * do not want to override the existing value.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set (must not contain NUL).
 * @param bytes the length of value (in bytes).
 */
extern void stp_set_default_file_parameter_n(stp_vars_t *v,
					     const char *parameter,
					     const char *value, size_t bytes);

/**
 * Set a default float parameter.
 * The value is set if the parameter is not already set.  This avoids
 * having to check if the parameter is set prior to setting it, if you
 * do not want to override the existing value.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 */
extern void stp_set_default_float_parameter(stp_vars_t *v,
					    const char *parameter,
					    double value);

/**
 * Set a default integer parameter.
 * The value is set if the parameter is not already set.  This avoids
 * having to check if the parameter is set prior to setting it, if you
 * do not want to override the existing value.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 */
extern void stp_set_default_int_parameter(stp_vars_t *v,
					  const char *parameter,
					  int value);

/**
 * Set a default dimension parameter.
 * The value is set if the parameter is not already set.  This avoids
 * having to check if the parameter is set prior to setting it, if you
 * do not want to override the existing value.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 */
extern void stp_set_default_dimension_parameter(stp_vars_t *v,
						const char *parameter,
						int value);

/**
 * Set a default boolean parameter.
 * The value is set if the parameter is not already set.  This avoids
 * having to check if the parameter is set prior to setting it, if you
 * do not want to override the existing value.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 */
extern void stp_set_default_boolean_parameter(stp_vars_t *v,
					      const char *parameter,
					      int value);

/**
 * Set a default curve parameter.
 * The value is set if the parameter is not already set.  This avoids
 * having to check if the parameter is set prior to setting it, if you
 * do not want to override the existing value.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 */
extern void stp_set_default_curve_parameter(stp_vars_t *v,
					    const char *parameter,
					    const stp_curve_t *value);

/**
 * Set a default array parameter.
 * The value is set if the parameter is not already set.  This avoids
 * having to check if the parameter is set prior to setting it, if you
 * do not want to override the existing value.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 */
extern void stp_set_default_array_parameter(stp_vars_t *v,
					    const char *parameter,
					    const stp_array_t *value);

/**
 * Set a default raw parameter.
 * The value is set if the parameter is not already set.  This avoids
 * having to check if the parameter is set prior to setting it, if you
 * do not want to override the existing value.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param value the value to set.
 * @param bytes the length of value (in bytes).
 */
extern void stp_set_default_raw_parameter(stp_vars_t *v,
					  const char *parameter,
					  const void *value, size_t bytes);

/**
 * Get a string parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the string, or NULL if no parameter was found.
 */
extern const char *stp_get_string_parameter(const stp_vars_t *v,
					    const char *parameter);

/**
 * Get a file parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the filename, or NULL if no parameter was found.
 */
extern const char *stp_get_file_parameter(const stp_vars_t *v,
					  const char *parameter);

/**
 * Get a float parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the float value.
 */
extern double stp_get_float_parameter(const stp_vars_t *v,
					    const char *parameter);

/**
 * Get an integer parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the integer value.
 */
extern int stp_get_int_parameter(const stp_vars_t *v,
				 const char *parameter);

/**
 * Get a dimension parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the dimension (integer) value.
 */
extern int stp_get_dimension_parameter(const stp_vars_t *v,
				       const char *parameter);

/**
 * Get a boolean parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the boolean value.
 */
extern int stp_get_boolean_parameter(const stp_vars_t *v,
				     const char *parameter);

/**
 * Get a curve parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the curve, or NULL if no parameter was found.
 */
extern const stp_curve_t *stp_get_curve_parameter(const stp_vars_t *v,
						  const char *parameter);

/**
 * Get an array parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the array, or NULL if no parameter was found.
 */
extern const stp_array_t *stp_get_array_parameter(const stp_vars_t *v,
						  const char *parameter);

/**
 * Get a raw parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the raw data, or NULL if no parameter was found.
 */
extern const stp_raw_t *stp_get_raw_parameter(const stp_vars_t *v,
					      const char *parameter);

/**
 * Clear a string parameter.
 * The parameter is set to NULL.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 */
extern void stp_clear_string_parameter(stp_vars_t *v, const char *parameter);

/**
 * Clear a file parameter.
 * The parameter is set to NULL.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 */
extern void stp_clear_file_parameter(stp_vars_t *v, const char *parameter);

/**
 * Clear (remove) a float parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 */
extern void stp_clear_float_parameter(stp_vars_t *v, const char *parameter);

/**
 * Clear (remove) an integer parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 */
extern void stp_clear_int_parameter(stp_vars_t *v, const char *parameter);

/**
 * Clear (remove) a dimension parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 */
extern void stp_clear_dimension_parameter(stp_vars_t *v, const char *parameter);

/**
 * Clear (remove) a boolean parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 */
extern void stp_clear_boolean_parameter(stp_vars_t *v, const char *parameter);

/**
 * Clear a curve parameter.
 * The parameter is set to NULL.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 */
extern void stp_clear_curve_parameter(stp_vars_t *v, const char *parameter);

/**
 * Clear an array parameter.
 * The parameter is set to NULL.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 */
extern void stp_clear_array_parameter(stp_vars_t *v, const char *parameter);

/**
 * Clear a raw parameter.
 * The parameter is set to NULL.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 */
extern void stp_clear_raw_parameter(stp_vars_t *v, const char *parameter);

/**
 * Clear a parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param type the type of the parameter.
 */
extern void stp_clear_parameter(stp_vars_t *v, const char *parameter, stp_parameter_type_t type);


/**
 * List all string parameters.
 * The return value must be freed after use.
 * @param v the vars to use.
 */
extern stp_string_list_t *stp_list_string_parameters(const stp_vars_t *v);

/**
 * List all file parameters.
 * The return value must be freed after use.
 * @param v the vars to use.
 */
extern stp_string_list_t *stp_list_file_parameters(const stp_vars_t *v);

/**
 * List all float parameters.
 * The return value must be freed after use.
 * @param v the vars to use.
 */
extern stp_string_list_t *stp_list_float_parameters(const stp_vars_t *v);

/**
 * List all integer parameters.
 * The return value must be freed after use.
 * @param v the vars to use.
 */
extern stp_string_list_t *stp_list_int_parameters(const stp_vars_t *v);

/**
 * List all dimension parameters.
 * The return value must be freed after use.
 * @param v the vars to use.
 */
extern stp_string_list_t *stp_list_dimension_parameters(const stp_vars_t *v);

/**
 * List all boolean parameters.
 * The return value must be freed after use.
 * @param v the vars to use.
 */
extern stp_string_list_t *stp_list_boolean_parameters(const stp_vars_t *v);

/**
 * List all curve parameters.
 * The return value must be freed after use.
 * @param v the vars to use.
 */
extern stp_string_list_t *stp_list_curve_parameters(const stp_vars_t *v);

/**
 * List all array parameters.
 * The return value must be freed after use.
 * @param v the vars to use.
 */
extern stp_string_list_t *stp_list_array_parameters(const stp_vars_t *v);

/**
 * List all raw parameters.
 * The return value must be freed after use.
 * @param v the vars to use.
 */
extern stp_string_list_t *stp_list_raw_parameters(const stp_vars_t *v);

/**
 * List all parameters.
 * The return value must be freed after use.
 * @param v the vars to use.
 * @param type the type of the parameter.
 */
extern stp_string_list_t *stp_list_parameters(const stp_vars_t *v,
					      stp_parameter_type_t type);


/**
 * Set the activity of a string parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the activity status to set (should be set to
 * STP_PARAMETER_ACTIVE or STP_PARAMETER_INACTIVE).
 */
extern void stp_set_string_parameter_active(stp_vars_t *v,
					    const char *parameter,
					    stp_parameter_activity_t active);

/**
 * Set the activity of a file parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the activity status to set (should be set to
 * STP_PARAMETER_ACTIVE or STP_PARAMETER_INACTIVE).
 */
extern void stp_set_file_parameter_active(stp_vars_t *v,
					  const char *parameter,
					  stp_parameter_activity_t active);

/**
 * Set the activity of a float parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the activity status to set (should be set to
 * STP_PARAMETER_ACTIVE or STP_PARAMETER_INACTIVE).
 */
extern void stp_set_float_parameter_active(stp_vars_t *v,
					 const char *parameter,
					 stp_parameter_activity_t active);

/**
 * Set the activity of an integer parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the activity status to set (should be set to
 * STP_PARAMETER_ACTIVE or STP_PARAMETER_INACTIVE).
 */
extern void stp_set_int_parameter_active(stp_vars_t *v,
					 const char *parameter,
					 stp_parameter_activity_t active);

/**
 * Set the activity of a dimension parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the activity status to set (should be set to
 * STP_PARAMETER_ACTIVE or STP_PARAMETER_INACTIVE).
 */
extern void stp_set_dimension_parameter_active(stp_vars_t *v,
					       const char *parameter,
					       stp_parameter_activity_t active);

/**
 * Set the activity of a boolean parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the activity status to set (should be set to
 * STP_PARAMETER_ACTIVE or STP_PARAMETER_INACTIVE).
 */
extern void stp_set_boolean_parameter_active(stp_vars_t *v,
					     const char *parameter,
					     stp_parameter_activity_t active);

/**
 * Set the activity of a curveparameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the activity status to set (should be set to
 * STP_PARAMETER_ACTIVE or STP_PARAMETER_INACTIVE).
 */
extern void stp_set_curve_parameter_active(stp_vars_t *v,
					   const char *parameter,
					   stp_parameter_activity_t active);

/**
 * Set the activity of an array parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the activity status to set (should be set to
 * STP_PARAMETER_ACTIVE or STP_PARAMETER_INACTIVE).
 */
extern void stp_set_array_parameter_active(stp_vars_t *v,
					   const char *parameter,
					   stp_parameter_activity_t active);

/**
 * Set the activity of a raw parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the activity status to set (should be set to
 * STP_PARAMETER_ACTIVE or STP_PARAMETER_INACTIVE).
 */
extern void stp_set_raw_parameter_active(stp_vars_t *v,
					 const char *parameter,
					 stp_parameter_activity_t active);

/**
 * Set the activity of a parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the activity status to set (should be set to
 * STP_PARAMETER_ACTIVE or STP_PARAMETER_INACTIVE).
 * @param type the type of the parameter.
 */
extern void stp_set_parameter_active(stp_vars_t *v,
				     const char *parameter,
				     stp_parameter_activity_t active,
				     stp_parameter_type_t type);

/**
 * Check if a string parameter is set.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the minimum activity status.
 */
extern int stp_check_string_parameter(const stp_vars_t *v, const char *parameter,
				      stp_parameter_activity_t active);

/**
 * Check if a file parameter is set.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the minimum activity status.
 */
extern int stp_check_file_parameter(const stp_vars_t *v, const char *parameter,
				    stp_parameter_activity_t active);

/**
 * Check if a float parameter is set.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the minimum activity status.
 */
extern int stp_check_float_parameter(const stp_vars_t *v, const char *parameter,
				     stp_parameter_activity_t active);

/**
 * Check if an integer parameter is set.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the minimum activity status.
 */
extern int stp_check_int_parameter(const stp_vars_t *v, const char *parameter,
				   stp_parameter_activity_t active);

/**
 * Check if a dimension parameter is set.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the minimum activity status.
 */
extern int stp_check_dimension_parameter(const stp_vars_t *v, const char *parameter,
					 stp_parameter_activity_t active);

/**
 * Check if a boolean parameter is set.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the minimum activity status.
 */
extern int stp_check_boolean_parameter(const stp_vars_t *v, const char *parameter,
				       stp_parameter_activity_t active);

/**
 * Check if a curve parameter is set.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the minimum activity status.
 */
extern int stp_check_curve_parameter(const stp_vars_t *v, const char *parameter,
				     stp_parameter_activity_t active);

/**
 * Check if an array parameter is set.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the minimum activity status.
 */
extern int stp_check_array_parameter(const stp_vars_t *v, const char *parameter,
				     stp_parameter_activity_t active);

/**
 * Check if a raw parameter is set.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the minimum activity status.
 */
extern int stp_check_raw_parameter(const stp_vars_t *v, const char *parameter,
				   stp_parameter_activity_t active);

/**
 * Check if a parameter is set.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param active the minimum activity status.
 * @param type the type of the parameter.
 */
extern int stp_check_parameter(const stp_vars_t *v, const char *parameter,
			       stp_parameter_activity_t active,
			       stp_parameter_type_t type);

/**
 * Get the activity status of a string parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the activity status.
 */
extern stp_parameter_activity_t
stp_get_string_parameter_active(const stp_vars_t *v, const char *parameter);

/**
 * Get the activity status of a file parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the activity status.
 */
extern stp_parameter_activity_t
stp_get_file_parameter_active(const stp_vars_t *v, const char *parameter);

/**
 * Get the activity status of a float parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the activity status.
 */
extern stp_parameter_activity_t
stp_get_float_parameter_active(const stp_vars_t *v, const char *parameter);

/**
 * Get the activity status of an integer parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the activity status.
 */
extern stp_parameter_activity_t
stp_get_int_parameter_active(const stp_vars_t *v, const char *parameter);

/**
 * Get the activity status of a dimension parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the activity status.
 */
extern stp_parameter_activity_t
stp_get_dimension_parameter_active(const stp_vars_t *v, const char *parameter);

/**
 * Get the activity status of a boolean parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the activity status.
 */
extern stp_parameter_activity_t
stp_get_boolean_parameter_active(const stp_vars_t *v, const char *parameter);

/**
 * Get the activity status of a curve parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the activity status.
 */
extern stp_parameter_activity_t
stp_get_curve_parameter_active(const stp_vars_t *v, const char *parameter);

/**
 * Get the activity status of an array parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the activity status.
 */
extern stp_parameter_activity_t
stp_get_array_parameter_active(const stp_vars_t *v, const char *parameter);

/**
 * Get the activity status of a raw parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @returns the activity status.
 */
extern stp_parameter_activity_t
stp_get_raw_parameter_active(const stp_vars_t *v, const char *parameter);

/**
 * Get the activity status of a parameter.
 * @param v the vars to use.
 * @param parameter the name of the parameter.
 * @param type the type of the parameter.
 */
extern stp_parameter_activity_t
stp_get_parameter_active(const stp_vars_t *v, const char *parameter,
			 stp_parameter_type_t type);



/****************************************************************
*                                                               *
* INFORMATIONAL QUERIES                                         *
*                                                               *
****************************************************************/

/**
 * Get the media (paper) size.
 * Retrieve the media size of the media type set in V, expressed in units
 * of 1/72".  If the media size is invalid, width and height will be set
 * to -1.  Values of 0 for width or height indicate that the dimension
 * is variable, so that custom page sizes or roll paper can be used.
 * In this case, the size limit should be used to determine maximum and
 * minimum values permitted.
 * @param v the vars to use.
 * @param width a pointer to an int to store the media width in.
 * @param height a pointer to an int to store the media height in.
 */
extern void stp_get_media_size(const stp_vars_t *v, int *width, int *height);

/**
 * Get the imagable area of the page.
 * Retrieve the boundaries of the printable area of the page.  In combination
 * with the media size, this can be used to determine the actual printable
 * region, which callers can use to place the image precisely.  The
 * dimensions are relative to the top left of the physical page.
 *
 * If a customizable page size is used (see stp_printer_get_media_size),
 * the actual desired width and/or height must be filled in using
 * stp_set_page_width and/or stp_set_page_height.  If these are not filled
 * in, the margins will be returned.
 *
 * Returned values may be negative if a printer is capable of full bleed
 * by printing beyond the physical boundaries of the page.
 *
 * If the media size stored in V is invalid, the return values
 * will be indeterminate.  It is up to the user to specify legal values.
 * @param v the vars to use.
 * @param left a pointer to a int to store the left edge in.
 * @param right a pointer to a int to store the right edge in.
 * @param bottom a pointer to a int to store the bottom edge in.
 * @param top a pointer to a int to store the top edge in.
 */
extern void stp_get_imageable_area(const stp_vars_t *v, int *left, int *right,
				   int *bottom, int *top);

/**
 * Get the maximum imagable area of the page.
 * Retrieve the maximum (regardless of settings other than page sise)
 * boundaries of the printable area of the page.  In combination
 * with the media size, this can be used to determine the actual printable
 * region, which callers can use to place the image precisely.  The
 * dimensions are relative to the top left of the physical page.
 *
 * If a customizable page size is used (see stp_printer_get_media_size),
 * the actual desired width and/or height must be filled in using
 * stp_set_page_width and/or stp_set_page_height.  If these are not filled
 * in, the margins will be returned.
 *
 * Returned values may be negative if a printer is capable of full bleed
 * by printing beyond the physical boundaries of the page.
 *
 * If the media size stored in V is invalid, the return values
 * will be indeterminate.  It is up to the user to specify legal values.
 * @param v the vars to use.
 * @param left a pointer to a int to store the left edge in.
 * @param right a pointer to a int to store the right edge in.
 * @param bottom a pointer to a int to store the bottom edge in.
 * @param top a pointer to a int to store the top edge in.
 */
extern void stp_get_maximum_imageable_area(const stp_vars_t *v, int *left,
					   int *right, int *bottom, int *top);

/**
 * Get the media size limits.
 * Retrieve the minimum and maximum size limits for custom media sizes
 * with the current printer settings.
 * @param v the vars to use.
 * @param max_width a pointer to a int to store the maximum width in.
 * @param max_height a pointer to a int to store the maximum height in.
 * @param min_width a pointer to a int to store the minimum width in.
 * @param min_height a pointer to a int to store the minimum height in.
 */
extern void
stp_get_size_limit(const stp_vars_t *v, int *max_width, int *max_height,
		   int *min_width, int *min_height);


/**
 * Retrieve the printing resolution of the selected resolution.  If the
 * resolution is invalid, -1 will be returned in both x and y.
 * @param v the vars to use.
 * @param x a pointer to a int to store the horizontal resolution in.
 * @param y a pointer to a int to store the vertical resolution in.
 */
extern void stp_describe_resolution(const stp_vars_t *v, int *x, int *y);

/**
 * Verify parameters.
 * Verify that the parameters selected are consistent with those allowed
 * by the driver.  This must be called prior to printing; failure to do
 * so will result in printing failing.
 * @param v the vars to use.
 * @returns 0 on failure, 1 on success; other status values are reserved.
 */
extern int stp_verify(stp_vars_t *v);

/**
 * Get default global settings.  The main use of this is to provide a
 * usable stp_vars_t for purposes of parameter inquiry in the absence
 * of a specific printer.  This is currently used in a variety of
 * places to get information on the standard color parameters without
 * querying a particular printer.
 * @returns the default settings.
 */
extern const stp_vars_t *stp_default_settings(void);

/**
 * Get the value of a specified category for the specified parameter.
 * @param v the vars to use.
 * @param desc the parameter description to use (must already be described)
 * @param category the name of the category to search for.
 * @returns the value of the category or NULL.  String must be freed by caller.
 */
extern char *stp_parameter_get_category(const stp_vars_t *v,
					const stp_parameter_t *desc,
					const char *category);

/**
 * Determine whether a parameter has a category with the specified value.
 * If a null value is passed in, return whether the parameter has
 * the category at all.  Return -1 if any other error condition (null
 * vars, desc, or category).
 * @param v the vars to use.
 * @param desc the parameter description to use (must already be described)
 * @param category the name of the category to search for.
 * @param value the value of the category to search for.
 * @returns whether the parameter has the category with the specified value. 
 */
extern int stp_parameter_has_category_value(const stp_vars_t *v,
					    const stp_parameter_t *desc,
					    const char *category,
					    const char *value);

/**
 * Get the list of categories and their values for the specified parameter.
 * @param v the vars to use.
 * @param desc the parameter description to use (must already be described)
 * @returns the list of categories.
 */
extern stp_string_list_t *stp_parameter_get_categories(const stp_vars_t *v,
						       const stp_parameter_t *desc);

typedef void *(*stp_copy_data_func_t)(void *);
typedef void (*stp_free_data_func_t)(void *);

typedef enum
{
  PARAMETER_BAD,
  PARAMETER_OK,
  PARAMETER_INACTIVE
} stp_parameter_verify_t;

extern void stp_allocate_component_data(stp_vars_t *v,
					const char *name,
					stp_copy_data_func_t copyfunc,
					stp_free_data_func_t freefunc,
					void *data);
extern void stp_destroy_component_data(stp_vars_t *v, const char *name);

struct stp_compdata;
typedef struct stp_compdata compdata_t;

extern void *stp_get_component_data(const stp_vars_t *v, const char *name);

extern stp_parameter_verify_t stp_verify_parameter(const stp_vars_t *v,
						   const char *parameter,
						   int quiet);
extern int stp_get_verified(const stp_vars_t *v);
extern void stp_set_verified(stp_vars_t *v, int value);

extern void stp_copy_options(stp_vars_t *vd, const stp_vars_t *vs);

extern void
stp_fill_parameter_settings(stp_parameter_t *desc,
			    const stp_parameter_t *param);

  /** @} */

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_VARS_H */
/*
 * End of "$Id: vars.h,v 1.8 2010/12/05 21:38:14 rlk Exp $".
 */
