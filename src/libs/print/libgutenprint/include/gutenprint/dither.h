/*
 * "$Id: dither.h,v 1.1 2004/09/17 18:38:01 rleigh Exp $"
 *
 *   libgimpprint dither header.
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
 *
 * Revision History:
 *
 *   See ChangeLog
 */

/**
 * @file gutenprint/dither.h
 * @brief Dither functions.
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifndef GUTENPRINT_DITHER_H
#define GUTENPRINT_DITHER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*
 * STP_ECOLOR_K must be 0
 */
#define STP_ECOLOR_K  0
#define STP_ECOLOR_C  1
#define STP_ECOLOR_M  2
#define STP_ECOLOR_Y  3
#define STP_NCOLORS (4)

typedef struct stp_dither_matrix_short
{
  int x;
  int y;
  int bytes;
  int prescaled;
  const unsigned short *data;
} stp_dither_matrix_short_t;

typedef struct stp_dither_matrix_normal
{
  int x;
  int y;
  int bytes;
  int prescaled;
  const unsigned *data;
} stp_dither_matrix_normal_t;

typedef struct stp_dither_matrix_generic
{
  int x;
  int y;
  int bytes;
  int prescaled;
  const void *data;
} stp_dither_matrix_generic_t;

typedef struct dither_matrix_impl
{
  int base;
  int exp;
  int x_size;
  int y_size;
  int total_size;
  int last_x;
  int last_x_mod;
  int last_y;
  int last_y_mod;
  int index;
  int i_own;
  int x_offset;
  int y_offset;
  unsigned fast_mask;
  unsigned *matrix;
} stp_dither_matrix_impl_t;

extern void stp_dither_matrix_iterated_init(stp_dither_matrix_impl_t *mat, size_t size,
					    size_t exponent, const unsigned *array);
extern void stp_dither_matrix_shear(stp_dither_matrix_impl_t *mat,
				    int x_shear, int y_shear);
extern void stp_dither_matrix_init(stp_dither_matrix_impl_t *mat, int x_size,
				   int y_size, const unsigned int *array,
				   int transpose, int prescaled);
extern void stp_dither_matrix_init_short(stp_dither_matrix_impl_t *mat, int x_size,
					 int y_size,
					 const unsigned short *array,
					 int transpose, int prescaled);
extern int stp_dither_matrix_validate_array(const stp_array_t *array);
extern void stp_dither_matrix_init_from_dither_array(stp_dither_matrix_impl_t *mat,
						     const stp_array_t *array,
						     int transpose);
extern void stp_dither_matrix_destroy(stp_dither_matrix_impl_t *mat);
extern void stp_dither_matrix_clone(const stp_dither_matrix_impl_t *src,
				    stp_dither_matrix_impl_t *dest,
				    int x_offset, int y_offset);
extern void stp_dither_matrix_copy(const stp_dither_matrix_impl_t *src,
				   stp_dither_matrix_impl_t *dest);
extern void stp_dither_matrix_scale_exponentially(stp_dither_matrix_impl_t *mat,
						  double exponent);
extern void stp_dither_matrix_set_row(stp_dither_matrix_impl_t *mat, int y);
extern stp_array_t *stp_find_standard_dither_array(int x_aspect, int y_aspect);


typedef struct stp_dotsize
{
  unsigned bit_pattern;
  double value;
} stp_dotsize_t;

typedef struct stp_shade
{
  double value;
  int numsizes;
  const stp_dotsize_t *dot_sizes;
} stp_shade_t;

extern stp_parameter_list_t stp_dither_list_parameters(const stp_vars_t *v);

extern void
stp_dither_describe_parameter(const stp_vars_t *v, const char *name,
			      stp_parameter_t *description);

extern void stp_dither_init(stp_vars_t *v, stp_image_t *image,
			    int out_width, int xdpi, int ydpi);
extern void stp_dither_set_iterated_matrix(stp_vars_t *v, size_t edge,
					   size_t iterations,
					   const unsigned *data,
					   int prescaled,
					   int x_shear, int y_shear);
extern void stp_dither_set_matrix(stp_vars_t *v, const stp_dither_matrix_generic_t *mat,
				  int transpose, int x_shear, int y_shear);
extern void stp_dither_set_matrix_from_dither_array(stp_vars_t *v,
						    const stp_array_t *array,
						    int transpose);
extern void stp_dither_set_transition(stp_vars_t *v, double);
extern void stp_dither_set_randomizer(stp_vars_t *v, int color, double);
extern void stp_dither_set_ink_spread(stp_vars_t *v, int spread);
extern void stp_dither_set_adaptive_limit(stp_vars_t *v, double limit);
extern int stp_dither_get_first_position(stp_vars_t *v, int color, int subchan);
extern int stp_dither_get_last_position(stp_vars_t *v, int color, int subchan);
extern void stp_dither_set_inks_simple(stp_vars_t *v, int color, int nlevels,
				       const double *levels, double density,
				       double darkness);
extern void stp_dither_set_inks_full(stp_vars_t *v, int color, int nshades,
				     const stp_shade_t *shades,
				     double density, double darkness);
extern void stp_dither_set_inks(stp_vars_t *v, int color,
				double density, double darkness,
				int nshades, const double *svalues,
				int ndotsizes, const double *dvalues);


extern void stp_dither_add_channel(stp_vars_t *v, unsigned char *data,
				   unsigned channel, unsigned subchannel);

extern unsigned char *stp_dither_get_channel(stp_vars_t *v,
					     unsigned channel,
					     unsigned subchannel);

extern void stp_dither(stp_vars_t *v, int row, int duplicate_line,
		       int zero_mask, const unsigned char *mask);

/* #ifdef STP_TESTDITHER */
extern void stp_dither_internal(stp_vars_t *v, int row,
				const unsigned short *input,
				int duplicate_line, int zero_mask,
				const unsigned char *mask);
/* #endif */

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_DITHER_H */
/*
 * End of "$Id: dither.h,v 1.1 2004/09/17 18:38:01 rleigh Exp $".
 */
