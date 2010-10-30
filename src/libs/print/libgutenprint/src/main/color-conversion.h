/*
 * "$Id: color-conversion.h,v 1.12 2008/01/21 23:19:39 rlk Exp $"
 *
 *   Gutenprint color management module - traditional Gimp-Print algorithm.
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

#ifndef GUTENPRINT_INTERNAL_COLOR_CONVERSION_H
#define GUTENPRINT_INTERNAL_COLOR_CONVERSION_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include <gutenprint/curve-cache.h>

typedef enum
{
  COLOR_CORRECTION_DEFAULT,
  COLOR_CORRECTION_UNCORRECTED,
  COLOR_CORRECTION_BRIGHT,
  COLOR_CORRECTION_HUE,
  COLOR_CORRECTION_ACCURATE,
  COLOR_CORRECTION_THRESHOLD,
  COLOR_CORRECTION_DESATURATED,
  COLOR_CORRECTION_DENSITY,
  COLOR_CORRECTION_RAW,
  COLOR_CORRECTION_PREDITHERED
} color_correction_enum_t;

typedef struct
{
  const char *name;
  const char *text;
  color_correction_enum_t correction;
  int correct_hsl;
} color_correction_t;

typedef enum
{
  COLOR_WHITE,			/* RGB */
  COLOR_BLACK,			/* CMY */
  COLOR_UNKNOWN			/* Printer-specific uninterpreted */
} color_model_t;

#define CHANNEL_K	0
#define CHANNEL_C	1
#define CHANNEL_M	2
#define CHANNEL_Y	3
#define CHANNEL_W	4
#define CHANNEL_R	5
#define CHANNEL_G	6
#define CHANNEL_B	7
#define CHANNEL_MAX	8

#define CMASK_K		(1 << CHANNEL_K)
#define CMASK_C		(1 << CHANNEL_C)
#define CMASK_M		(1 << CHANNEL_M)
#define CMASK_Y		(1 << CHANNEL_Y)
#define CMASK_W		(1 << CHANNEL_W)
#define CMASK_R		(1 << CHANNEL_R)
#define CMASK_G		(1 << CHANNEL_G)
#define CMASK_B		(1 << CHANNEL_B)
#define CMASK_RAW       (1 << CHANNEL_MAX)

typedef struct
{
  unsigned channel_id;
  const char *gamma_name;
  const char *curve_name;
  const char *rgb_gamma_name;
  const char *rgb_curve_name;
} channel_param_t;

/* Color conversion function */
typedef unsigned (*stp_convert_t)(const stp_vars_t *vars,
				  const unsigned char *in,
				  unsigned short *out);

#define CMASK_NONE   (0)
#define CMASK_RGB    (CMASK_R | CMASK_G | CMASK_B)
#define CMASK_CMY    (CMASK_C | CMASK_M | CMASK_Y)
#define CMASK_CMYK   (CMASK_CMY | CMASK_K)
#define CMASK_ALL    (CMASK_CMYK | CMASK_RGB | CMASK_W)
#define CMASK_EVERY  (CMASK_ALL | CMASK_RAW)

typedef enum
{
  COLOR_ID_GRAY,
  COLOR_ID_WHITE,
  COLOR_ID_RGB,
  COLOR_ID_CMY,
  COLOR_ID_CMYK,
  COLOR_ID_KCMY,
  COLOR_ID_RAW
} color_id_t;

typedef struct
{
  const char *name;
  int input;
  int output;
  color_id_t color_id;
  color_model_t color_model;
  unsigned channels;
  int channel_count;
  color_correction_enum_t default_correction;
  stp_convert_t conversion_function;
} color_description_t;

typedef struct
{
  const char *name;
  size_t bits;
} channel_depth_t;

typedef struct
{
  unsigned steps;
  int channel_depth;
  int image_width;
  int in_channels;
  int out_channels;
  int channels_are_initialized;
  int invert_output;
  const color_description_t *input_color_description;
  const color_description_t *output_color_description;
  const color_correction_t *color_correction;
  stp_cached_curve_t brightness_correction;
  stp_cached_curve_t contrast_correction;
  stp_cached_curve_t user_color_correction;
  stp_cached_curve_t channel_curves[STP_CHANNEL_LIMIT];
  double gamma_values[STP_CHANNEL_LIMIT];
  double print_gamma;
  double app_gamma;
  double screen_gamma;
  double contrast;
  double brightness;
  int linear_contrast_adjustment;
  int printed_colorfunc;
  int simple_gamma_correction;
  stp_cached_curve_t hue_map;
  stp_cached_curve_t lum_map;
  stp_cached_curve_t sat_map;
  unsigned short *gray_tmp;	/* Color -> Gray */
  unsigned short *cmy_tmp;	/* CMY -> CMYK */
  unsigned char *in_data;
} lut_t;

extern unsigned stpi_color_convert_to_gray(const stp_vars_t *v,
					   const unsigned char *,
					   unsigned short *);
extern unsigned stpi_color_convert_to_color(const stp_vars_t *v,
					    const unsigned char *,
					    unsigned short *);
extern unsigned stpi_color_convert_to_kcmy(const stp_vars_t *v,
					   const unsigned char *,
					   unsigned short *);
extern unsigned stpi_color_convert_raw(const stp_vars_t *v,
				       const unsigned char *,
				       unsigned short *);

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_INTERNAL_COLOR_CONVERSION_H */
