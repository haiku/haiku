/*
 * "$Id: color.h,v 1.2 2005/10/18 02:08:16 rlk Exp $"
 *
 *   libgimpprint color functions.
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
 * @file gutenprint/color.h
 * @brief Color functions.
 */

#ifndef GUTENPRINT_COLOR_H
#define GUTENPRINT_COLOR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The color data type is responsible for providing colour
 * conversion features.  Color modules provide the actual
 * functionality, so different colour management modules may provide
 * the application with different services (for example, colour
 * profiles).

 * @defgroup color color
 * @{
 */

typedef struct
{
  int (*init)(stp_vars_t *v, stp_image_t *image, size_t steps);
  int (*get_row)(stp_vars_t *v, stp_image_t *image,
		 int row, unsigned *zero_mask);
  stp_parameter_list_t (*list_parameters)(const stp_vars_t *v);
  void (*describe_parameter)(const stp_vars_t *v, const char *name,
			     stp_parameter_t *description);
} stp_colorfuncs_t;


typedef struct stp_color
{
  const char *short_name;       /* Color module name */
  const char *long_name;        /* Long name for UI */
  const stp_colorfuncs_t *colorfuncs;
} stp_color_t;

/*
 * Initialize the color machinery.  Return value is the number
 * of columns of output
 */
extern int stp_color_init(stp_vars_t *v, stp_image_t *image, size_t steps);

/*
 * Acquire input and perform color conversion.  Return value
 * is status; zero is success.
 */
extern int stp_color_get_row(stp_vars_t *v, stp_image_t *image,
			     int row, unsigned *zero_mask);

extern stp_parameter_list_t stp_color_list_parameters(const stp_vars_t *v);

extern void stp_color_describe_parameter(const stp_vars_t *v, const char *name,
					 stp_parameter_t *description);

extern int
stp_color_register(const stp_color_t *color);

extern int
stp_color_unregister(const stp_color_t *color);

/**
 * Get the number of available color modules.
 * @returns the number of color modules.
 */
extern int
stp_color_count(void);

/**
 * Get a color module by its name.
 * @param name the short unique name.
 * number of papers - 1).
 * @returns a pointer to the color module, or NULL on failure.
 */
extern const stp_color_t *
stp_get_color_by_name(const char *name);

/**
 * Get a color module by its index number.
 * @param idx the index number.  This must not be greater than (total
 * number of papers - 1).
 * @returns a pointer to the color module, or NULL on failure.
 */
extern const stp_color_t *
stp_get_color_by_index(int idx);

extern const stp_color_t *
stp_get_color_by_colorfuncs(stp_colorfuncs_t *colorfuncs);

/**
 * Get the short (untranslated) name of a color module.
 * @param c the color module to use.
 * @returns the short name.
 */
extern const char *
stp_color_get_name(const stp_color_t *c);

/**
 * Get the long (translated) name of a color module.
 * @param c the color module to use.
 * @returns the long name.
 */
extern const char *
stp_color_get_long_name(const stp_color_t *c);

  /** @} */

#ifdef __cplusplus
  }
#endif


#endif /* GUTENPRINT_COLOR_H */
/*
 * End of "$Id: color.h,v 1.2 2005/10/18 02:08:16 rlk Exp $".
 */
