/*
 * "$Id: color-conversions.c,v 1.20 2005/07/04 00:23:54 rlk Exp $"
 *
 *   Gimp-Print color management module - traditional Gimp-Print algorithm.
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
#include <gutenprint/curve-cache.h>
#include <math.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <string.h>
#include "color-conversion.h"

#ifdef __GNUC__
#define inline __inline__
#endif

/*
 * RGB to grayscale luminance constants...
 */

#define LUM_RED		31
#define LUM_GREEN	61
#define LUM_BLUE	8

/* rgb/hsl conversions taken from Gimp common/autostretch_hsv.c */

#define FMAX(a, b) ((a) > (b) ? (a) : (b))
#define FMIN(a, b) ((a) < (b) ? (a) : (b))

static inline void
calc_rgb_to_hsl(unsigned short *rgb, double *hue, double *sat,
		double *lightness)
{
  double red, green, blue;
  double h, s, l;
  double min, max;
  double delta;
  int maxval;

  red   = rgb[0] / 65535.0;
  green = rgb[1] / 65535.0;
  blue  = rgb[2] / 65535.0;

  if (red > green)
    {
      if (red > blue)
	{
	  max = red;
	  maxval = 0;
	}
      else
	{
	  max = blue;
	  maxval = 2;
	}
      min = FMIN(green, blue);
    }
  else
    {
      if (green > blue)
	{
	  max = green;
	  maxval = 1;
	}
      else
	{
	  max = blue;
	  maxval = 2;
	}
      min = FMIN(red, blue);
    }

  l = (max + min) / 2.0;
  delta = max - min;

  if (delta < .000001)	/* Suggested by Eugene Anikin <eugene@anikin.com> */
    {
      s = 0.0;
      h = 0.0;
    }
  else
    {
      if (l <= .5)
	s = delta / (max + min);
      else
	s = delta / (2 - max - min);

      if (maxval == 0)
	h = (green - blue) / delta;
      else if (maxval == 1)
	h = 2 + (blue - red) / delta;
      else
	h = 4 + (red - green) / delta;

      if (h < 0.0)
	h += 6.0;
      else if (h > 6.0)
	h -= 6.0;
    }

  *hue = h;
  *sat = s;
  *lightness = l;
}

static inline double
hsl_value(double n1, double n2, double hue)
{
  if (hue < 0)
    hue += 6.0;
  else if (hue > 6)
    hue -= 6.0;
  if (hue < 1.0)
    return (n1 + (n2 - n1) * hue);
  else if (hue < 3.0)
    return (n2);
  else if (hue < 4.0)
    return (n1 + (n2 - n1) * (4.0 - hue));
  else
    return (n1);
}

static inline void
calc_hsl_to_rgb(unsigned short *rgb, double h, double s, double l)
{
  if (s < .0000001)
    {
      if (l > 1)
	l = 1;
      else if (l < 0)
	l = 0;
      rgb[0] = l * 65535;
      rgb[1] = l * 65535;
      rgb[2] = l * 65535;
    }
  else
    {
      double m1, m2;
      double h1, h2;
      h1 = h + 2;
      h2 = h - 2;

      if (l < .5)
	m2 = l * (1 + s);
      else
	m2 = l + s - (l * s);
      m1 = (l * 2) - m2;
      rgb[0] = 65535 * hsl_value(m1, m2, h1);
      rgb[1] = 65535 * hsl_value(m1, m2, h);
      rgb[2] = 65535 * hsl_value(m1, m2, h2);
    }
}

static inline double
update_saturation(double sat, double adjust, double isat, int bright_colors)
{
  if (bright_colors || adjust < 1)
    sat *= adjust;
  else if (adjust > 1)
    {
      double s1 = sat * adjust;
      double s2 = 1.0 - ((1.0 - sat) * isat);
      sat = FMIN(s1, s2);
    }
  if (sat > 1)
    sat = 1.0;
  return sat;
}

static inline double
interpolate_value(const double *vec, double val)
{
  double base = floor(val);
  double frac = val - base;
  int ibase = (int) base;
  double lval = vec[ibase];
  if (frac > 0)
    lval += (vec[ibase + 1] - lval) * frac;
  return lval;
}

static inline void
update_saturation_from_rgb(unsigned short *rgb,
			   const unsigned short *brightness_lookup,
			   double adjust, double isat, int do_usermap)
{
  double h, s, l;
  calc_rgb_to_hsl(rgb, &h, &s, &l);
  if (do_usermap)
    {
      unsigned short ub = (unsigned short) (l * 65535);
      unsigned short val = brightness_lookup[ub];
      l = ((double) val) / 65535;
      if (val < ub)
	s = s * (65535 - ub) / (65535 - val);
    }
  s = update_saturation(s, adjust, isat, 0);
  calc_hsl_to_rgb(rgb, h, s, l);
}

static inline double
adjust_hue(const double *hue_map, double hue, size_t points)
{
  if (hue_map)
    {
      hue += interpolate_value(hue_map, hue * points / 6.0);
      if (hue < 0.0)
	hue += 6.0;
      else if (hue >= 6.0)
	hue -= 6.0;
    }
  return hue;
}

static inline void
adjust_hsl(unsigned short *rgbout, lut_t *lut, double ssat, double isat,
	   int split_saturation, int adjust_hue_only, int bright_colors)
{
  const double *hue_map = CURVE_CACHE_FAST_DOUBLE(&(lut->hue_map));
  const double *lum_map = CURVE_CACHE_FAST_DOUBLE(&(lut->lum_map));
  const double *sat_map = CURVE_CACHE_FAST_DOUBLE(&(lut->sat_map));
  if ((split_saturation || lum_map || hue_map || sat_map) &&
      (rgbout[0] != rgbout[1] || rgbout[0] != rgbout[2]))
    {
      size_t hue_count = CURVE_CACHE_FAST_COUNT(&(lut->hue_map));
      size_t lum_count = CURVE_CACHE_FAST_COUNT(&(lut->lum_map));
      size_t sat_count = CURVE_CACHE_FAST_COUNT(&(lut->sat_map));
      double h, s, l;
      double oh;
      rgbout[0] ^= 65535;
      rgbout[1] ^= 65535;
      rgbout[2] ^= 65535;
      calc_rgb_to_hsl(rgbout, &h, &s, &l);
      s = update_saturation(s, ssat, isat, 0);
      if (!adjust_hue_only && lut->sat_map.d_cache)
	{
	  double nh = h * sat_count / 6.0;
	  double tmp = interpolate_value(sat_map, nh);
	  if (tmp < .9999 || tmp > 1.0001)
	    {
	      s = update_saturation(s, tmp, tmp > 1.0 ? 1.0 / tmp : 1.0,
				    bright_colors);
	    }
	}
      oh = h;
      h = adjust_hue(hue_map, h, hue_count);
      calc_hsl_to_rgb(rgbout, h, s, l);

      if (!adjust_hue_only && s > 0.00001)
	{
	  /*
	   * Perform luminosity adjustment only on color component.
	   * This way the luminosity of the gray component won't be affected.
	   * We'll add the gray back at the end.
	   */

	  unsigned gray = FMIN(rgbout[0], FMIN(rgbout[1], rgbout[2]));
	  int i;
	  /*
	   * Scale the components by the amount of color left.
	   * This way the luminosity calculations will come out right.
	   */
	  if (gray > 0)
	    for (i = 0; i < 3; i++)
	      rgbout[i] = (rgbout[i] - gray) * 65535.0 / (65535 - gray);

	  calc_rgb_to_hsl(rgbout, &h, &s, &l);
	  if (lut->lum_map.d_cache && l > 0.00001 && l < .99999)
	    {
	      double nh = oh * lum_count / 6.0;
	      double oel = interpolate_value(lum_map,nh);
	      if (oel <= 1)
		l *= oel;
	      else
		{
		  double g1 = pow(l, 1.0 / oel);
		  double g2 = 1.0 - pow(1.0 - l, oel);
		  l = FMIN(g1, g2);
		}
	    }
	  calc_hsl_to_rgb(rgbout, h, s, l);
	  if (gray > 0)
	    for (i = 0; i < 3; i++)
	      rgbout[i] = gray + (rgbout[i] * (65535 - gray) / 65535.0);
	}

      rgbout[0] ^= 65535;
      rgbout[1] ^= 65535;
      rgbout[2] ^= 65535;
    }
}

static inline void
lookup_rgb(lut_t *lut, unsigned short *rgbout,
	   const unsigned short *red, const unsigned short *green,
	   const unsigned short *blue, unsigned steps)
{
  if (steps == 65536)
    {
      rgbout[0] = red[rgbout[0]];
      rgbout[1] = green[rgbout[1]];
      rgbout[2] = blue[rgbout[2]];
    }
  else
    {
      rgbout[0] = red[rgbout[0] / 257];
      rgbout[1] = green[rgbout[1] / 257];
      rgbout[2] = blue[rgbout[2] / 257];
    }
}

static inline int
short_eq(const unsigned short *i1, const unsigned short *i2, size_t count)
{
#if 1
  int i;
  for (i = 0; i < count; i++)
    if (i1[i] != i2[i])
      return 0;
  return 1;
#else
  return !memcmp(i1, i2, count * sizeof(unsigned short));
#endif
}

static inline void
short_copy(unsigned short *out, const unsigned short *in, size_t count)
{
#if 1
  int i;
  for (i = 0; i < count; i++)
    out[i] = in[i];
#else
  (void) memcpy(out, in, count * sizeof(unsigned short));
#endif
}

static unsigned
raw_cmy_to_kcmy(const stp_vars_t *vars, const unsigned short *in,
		unsigned short *out)
{
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));
  int width = lut->image_width;

  int i;
  int j;
  unsigned short nz[4];
  unsigned retval = 0;
  const unsigned short *input_cache = NULL;
  const unsigned short *output_cache = NULL;

  memset(nz, 0, sizeof(nz));

  for (i = 0; i < width; i++, out += 4, in += 3)
    {
      if (input_cache && short_eq(input_cache, in, 3))
	short_copy(out, output_cache, 4);
      else
	{
	  int c = in[0];
	  int m = in[1];
	  int y = in[2];
	  int k = FMIN(c, FMIN(m, y));
	  input_cache = in;
	  out[0] = 0;
	  for (j = 0; j < 3; j++)
	    out[j + 1] = in[j];
	  if (k > 0)
	    {
	      out[0] = k;
	      out[1] -= k;
	      out[2] -= k;
	      out[3] -= k;
	    }
	  output_cache = out;
	  for (j = 0; j < 4; j++)
	    if (out[j])
	      nz[j] = 1;
	}
    }
  for (j = 0; j < 4; j++)
    if (nz[j] == 0)
      retval |= (1 << j);
  return retval;
}

#define GENERIC_COLOR_FUNC(fromname, toname)				\
static unsigned								\
fromname##_to_##toname(const stp_vars_t *vars, const unsigned char *in,	\
		       unsigned short *out)				\
{									\
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	\
  if (!lut->printed_colorfunc)						\
    {									\
      lut->printed_colorfunc = 1;					\
      stp_dprintf(STP_DBG_COLORFUNC, vars,				\
		   "Colorfunc is %s_%d_to_%s, %s, %s, %d, %d\n",	\
		   #fromname, lut->channel_depth, #toname,		\
		   lut->input_color_description->name,			\
		   lut->output_color_description->name,			\
		   lut->steps, lut->invert_output);			\
    }									\
  if (lut->channel_depth == 8)						\
    return fromname##_8_to_##toname(vars, in, out);			\
  else									\
    return fromname##_16_to_##toname(vars, in, out);			\
}

#define COLOR_TO_COLOR_FUNC(T, bits)					     \
static unsigned								     \
color_##bits##_to_color(const stp_vars_t *vars, const unsigned char *in,     \
			unsigned short *out)				     \
{									     \
  int i;								     \
  double isat = 1.0;							     \
  double ssat = stp_get_float_parameter(vars, "Saturation");		     \
  double sbright = stp_get_float_parameter(vars, "Brightness");		     \
  int i0 = -1;								     \
  int i1 = -1;								     \
  int i2 = -1;								     \
  unsigned short o0 = 0;						     \
  unsigned short o1 = 0;						     \
  unsigned short o2 = 0;						     \
  unsigned short nz0 = 0;						     \
  unsigned short nz1 = 0;						     \
  unsigned short nz2 = 0;						     \
  const unsigned short *red;						     \
  const unsigned short *green;						     \
  const unsigned short *blue;						     \
  const unsigned short *brightness;					     \
  const unsigned short *contrast;					     \
  const T *s_in = (const T *) in;					     \
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	     \
  int compute_saturation = ssat <= .99999 || ssat >= 1.00001;		     \
  int split_saturation = ssat > 1.4;					     \
  int bright_color_adjustment = 0;					     \
  int hue_only_color_adjustment = 0;					     \
  int do_user_adjustment = 0;						     \
  if (lut->color_correction->correction == COLOR_CORRECTION_BRIGHT)	     \
    bright_color_adjustment = 1;					     \
  if (lut->color_correction->correction == COLOR_CORRECTION_HUE)	     \
    hue_only_color_adjustment = 1;					     \
  if (sbright != 1)							     \
    do_user_adjustment = 1;						     \
  compute_saturation |= do_user_adjustment;				     \
									     \
  for (i = CHANNEL_C; i <= CHANNEL_Y; i++)				     \
    stp_curve_resample(stp_curve_cache_get_curve(&(lut->channel_curves[i])), \
		       1 << bits);					     \
  stp_curve_resample							     \
    (stp_curve_cache_get_curve(&(lut->brightness_correction)), 65536);	     \
  stp_curve_resample							     \
    (stp_curve_cache_get_curve(&(lut->contrast_correction)), 1 << bits);     \
  red =									     \
    stp_curve_cache_get_ushort_data(&(lut->channel_curves[CHANNEL_C]));	     \
  green =								     \
    stp_curve_cache_get_ushort_data(&(lut->channel_curves[CHANNEL_M]));	     \
  blue =								     \
    stp_curve_cache_get_ushort_data(&(lut->channel_curves[CHANNEL_Y]));	     \
  brightness=								     \
    stp_curve_cache_get_ushort_data(&(lut->brightness_correction));	     \
  contrast =								     \
    stp_curve_cache_get_ushort_data(&(lut->contrast_correction));	     \
  (void) stp_curve_cache_get_double_data(&(lut->hue_map));		     \
  (void) stp_curve_cache_get_double_data(&(lut->lum_map));		     \
  (void) stp_curve_cache_get_double_data(&(lut->sat_map));		     \
									     \
  if (split_saturation)							     \
    ssat = sqrt(ssat);							     \
  if (ssat > 1)								     \
    isat = 1.0 / ssat;							     \
  for (i = 0; i < lut->image_width; i++)				     \
    {									     \
      if (i0 == s_in[0] && i1 == s_in[1] && i2 == s_in[2])		     \
	{								     \
	  out[0] = o0;							     \
	  out[1] = o1;							     \
	  out[2] = o2;							     \
	}								     \
      else								     \
	{								     \
	  i0 = s_in[0];							     \
	  i1 = s_in[1];							     \
	  i2 = s_in[2];							     \
	  out[0] = i0 * (65535u / (unsigned) ((1 << bits) - 1));	     \
	  out[1] = i1 * (65535u / (unsigned) ((1 << bits) - 1));	     \
	  out[2] = i2 * (65535u / (unsigned) ((1 << bits) - 1));	     \
	  lookup_rgb(lut, out, contrast, contrast, contrast, 1 << bits);     \
	  if ((compute_saturation))					     \
	    update_saturation_from_rgb(out, brightness, ssat, isat,	     \
				       do_user_adjustment);		     \
	  adjust_hsl(out, lut, ssat, isat, split_saturation,		     \
		     hue_only_color_adjustment, bright_color_adjustment);    \
	  lookup_rgb(lut, out, red, green, blue, 1 << bits);		     \
	  o0 = out[0];							     \
	  o1 = out[1];							     \
	  o2 = out[2];							     \
	  nz0 |= o0;							     \
	  nz1 |= o1;							     \
	  nz2 |= o2;							     \
	}								     \
      s_in += 3;							     \
      out += 3;								     \
    }									     \
  return (nz0 ? 0 : 1) +  (nz1 ? 0 : 2) +  (nz2 ? 0 : 4);		     \
}

COLOR_TO_COLOR_FUNC(unsigned char, 8)
COLOR_TO_COLOR_FUNC(unsigned short, 16)
GENERIC_COLOR_FUNC(color, color)

/*
 * 'rgb_to_rgb()' - Convert rgb image data to RGB.
 */

#define FAST_COLOR_TO_COLOR_FUNC(T, bits)				      \
static unsigned								      \
color_##bits##_to_color_fast(const stp_vars_t *vars, const unsigned char *in, \
			     unsigned short *out)			      \
{									      \
  int i;								      \
  int i0 = -1;								      \
  int i1 = -1;								      \
  int i2 = -1;								      \
  int o0 = 0;								      \
  int o1 = 0;								      \
  int o2 = 0;								      \
  int nz0 = 0;								      \
  int nz1 = 0;								      \
  int nz2 = 0;								      \
  const T *s_in = (const T *) in;					      \
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	      \
  const unsigned short *red;						      \
  const unsigned short *green;						      \
  const unsigned short *blue;						      \
  const unsigned short *brightness;					      \
  const unsigned short *contrast;					      \
  double isat = 1.0;							      \
  double saturation = stp_get_float_parameter(vars, "Saturation");	      \
  double sbright = stp_get_float_parameter(vars, "Brightness");		      \
  int compute_saturation = saturation <= .99999 || saturation >= 1.00001;     \
  int do_user_adjustment = 0;						      \
  if (sbright != 1)							      \
    do_user_adjustment = 1;						      \
  compute_saturation |= do_user_adjustment;				      \
									      \
  for (i = CHANNEL_C; i <= CHANNEL_Y; i++)				      \
    stp_curve_resample(lut->channel_curves[i].curve, 65536);		      \
  stp_curve_resample							      \
    (stp_curve_cache_get_curve(&(lut->brightness_correction)), 65536);	      \
  stp_curve_resample							      \
    (stp_curve_cache_get_curve(&(lut->contrast_correction)), 1 << bits);      \
  red =									      \
    stp_curve_cache_get_ushort_data(&(lut->channel_curves[CHANNEL_C]));	      \
  green =								      \
    stp_curve_cache_get_ushort_data(&(lut->channel_curves[CHANNEL_M]));	      \
  blue =								      \
    stp_curve_cache_get_ushort_data(&(lut->channel_curves[CHANNEL_Y]));	      \
  brightness=								      \
    stp_curve_cache_get_ushort_data(&(lut->brightness_correction));	      \
  contrast =								      \
    stp_curve_cache_get_ushort_data(&(lut->contrast_correction));	      \
									      \
  if (saturation > 1)							      \
    isat = 1.0 / saturation;						      \
  for (i = 0; i < lut->image_width; i++)				      \
    {									      \
      if (i0 == s_in[0] && i1 == s_in[1] && i2 == s_in[2])		      \
	{								      \
	  out[0] = o0;							      \
	  out[1] = o1;							      \
	  out[2] = o2;							      \
	}								      \
      else								      \
	{								      \
	  i0 = s_in[0];							      \
	  i1 = s_in[1];							      \
	  i2 = s_in[2];							      \
	  out[0] = contrast[s_in[0]];					      \
	  out[1] = contrast[s_in[1]];					      \
	  out[2] = contrast[s_in[2]];					      \
	  if ((compute_saturation))					      \
	    update_saturation_from_rgb(out, brightness, saturation, isat, 1); \
	  out[0] = red[out[0]];						      \
	  out[1] = green[out[1]];					      \
	  out[2] = blue[out[2]];					      \
	  o0 = out[0];							      \
	  o1 = out[1];							      \
	  o2 = out[2];							      \
	  nz0 |= o0;							      \
	  nz1 |= o1;							      \
	  nz2 |= o2;							      \
	}								      \
      s_in += 3;							      \
      out += 3;								      \
    }									      \
  return (nz0 ? 0 : 1) +  (nz1 ? 0 : 2) +  (nz2 ? 0 : 4);		      \
}

FAST_COLOR_TO_COLOR_FUNC(unsigned char, 8)
FAST_COLOR_TO_COLOR_FUNC(unsigned short, 16)
GENERIC_COLOR_FUNC(color, color_fast)

#define RAW_COLOR_TO_COLOR_FUNC(T, bits)				    \
static unsigned								    \
color_##bits##_to_color_raw(const stp_vars_t *vars, const unsigned char *in,\
			    unsigned short *out)			    \
{									    \
  int i;								    \
  int j;								    \
  int nz = 0;								    \
  const T *s_in = (const T *) in;					    \
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	    \
  unsigned mask = 0;							    \
  if (lut->invert_output)						    \
    mask = 0xffff;							    \
									    \
  for (i = 0; i < lut->image_width; i++)				    \
    {									    \
      unsigned bit = 1;							    \
      for (j = 0; j < 3; j++, bit += bit)				    \
	{								    \
	  out[j] = (s_in[j] * (65535 / ((1 << bits) - 1))) ^ mask;	    \
	  if (out[j])							    \
	    nz |= bit;							    \
	}								    \
      s_in += 3;							    \
      out += 3;								    \
    }									    \
  return nz;								    \
}

RAW_COLOR_TO_COLOR_FUNC(unsigned char, 8)
RAW_COLOR_TO_COLOR_FUNC(unsigned short, 16)
GENERIC_COLOR_FUNC(color, color_raw)

/*
 * 'gray_to_rgb()' - Convert gray image data to RGB.
 */

#define GRAY_TO_COLOR_FUNC(T, bits)					    \
static unsigned								    \
gray_##bits##_to_color(const stp_vars_t *vars, const unsigned char *in,	    \
		   unsigned short *out)					    \
{									    \
  int i;								    \
  int i0 = -1;								    \
  int o0 = 0;								    \
  int o1 = 0;								    \
  int o2 = 0;								    \
  int nz0 = 0;								    \
  int nz1 = 0;								    \
  int nz2 = 0;								    \
  const T *s_in = (const T *) in;					    \
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	    \
  const unsigned short *red;						    \
  const unsigned short *green;						    \
  const unsigned short *blue;						    \
  const unsigned short *user;						    \
									    \
  for (i = CHANNEL_C; i <= CHANNEL_Y; i++)				    \
    stp_curve_resample(lut->channel_curves[i].curve, 65536);		    \
  stp_curve_resample							    \
    (stp_curve_cache_get_curve(&(lut->user_color_correction)), 1 << bits);  \
  red =									    \
    stp_curve_cache_get_ushort_data(&(lut->channel_curves[CHANNEL_C]));	    \
  green =								    \
    stp_curve_cache_get_ushort_data(&(lut->channel_curves[CHANNEL_M]));	    \
  blue =								    \
    stp_curve_cache_get_ushort_data(&(lut->channel_curves[CHANNEL_Y]));	    \
  user =								    \
    stp_curve_cache_get_ushort_data(&(lut->user_color_correction));	    \
									    \
  for (i = 0; i < lut->image_width; i++)				    \
    {									    \
      if (i0 == s_in[0])						    \
	{								    \
	  out[0] = o0;							    \
	  out[1] = o1;							    \
	  out[2] = o2;							    \
	}								    \
      else								    \
	{								    \
	  i0 = s_in[0];							    \
	  out[0] = red[user[s_in[0]]];					    \
	  out[1] = green[user[s_in[0]]];				    \
	  out[2] = blue[user[s_in[0]]];					    \
	  o0 = out[0];							    \
	  o1 = out[1];							    \
	  o2 = out[2];							    \
	  nz0 |= o0;							    \
	  nz1 |= o1;							    \
	  nz2 |= o2;							    \
	}								    \
      s_in += 1;							    \
      out += 3;								    \
    }									    \
  return (nz0 ? 0 : 1) +  (nz1 ? 0 : 2) +  (nz2 ? 0 : 4);		    \
}

GRAY_TO_COLOR_FUNC(unsigned char, 8)
GRAY_TO_COLOR_FUNC(unsigned short, 16)
GENERIC_COLOR_FUNC(gray, color)

#define GRAY_TO_COLOR_RAW_FUNC(T, bits)					   \
static unsigned								   \
gray_##bits##_to_color_raw(const stp_vars_t *vars, const unsigned char *in,\
			   unsigned short *out)				   \
{									   \
  int i;								   \
  int nz = 7;								   \
  const T *s_in = (const T *) in;					   \
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	   \
  unsigned mask = 0;							   \
  if (lut->invert_output)						   \
    mask = 0xffff;							   \
									   \
  for (i = 0; i < lut->image_width; i++)				   \
    {									   \
      unsigned outval = (s_in[0] * (65535 / (1 << bits))) ^ mask;	   \
      out[0] = outval;							   \
      out[1] = outval;							   \
      out[2] = outval;							   \
      if (outval)							   \
	nz = 0;								   \
      s_in++;								   \
      out += 3;								   \
    }									   \
  return nz;								   \
}

GRAY_TO_COLOR_RAW_FUNC(unsigned char, 8)
GRAY_TO_COLOR_RAW_FUNC(unsigned short, 16)
GENERIC_COLOR_FUNC(gray, color_raw)

#define COLOR_TO_KCMY_FUNC(name, name2, name3, name4, bits)		    \
static unsigned								    \
name##_##bits##_to_##name2(const stp_vars_t *vars, const unsigned char *in, \
			   unsigned short *out)				    \
{									    \
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	    \
  size_t real_steps = lut->steps;					    \
  unsigned status;							    \
  if (!lut->cmy_tmp)							    \
    lut->cmy_tmp = stp_malloc(4 * 2 * lut->image_width);		    \
  name##_##bits##_to_##name3(vars, in, lut->cmy_tmp);			    \
  lut->steps = 65536;							    \
  status = name4##_cmy_to_kcmy(vars, lut->cmy_tmp, out);		    \
  lut->steps = real_steps;						    \
  return status;							    \
}

COLOR_TO_KCMY_FUNC(gray, kcmy, color, raw, 8)
COLOR_TO_KCMY_FUNC(gray, kcmy, color, raw, 16)
GENERIC_COLOR_FUNC(gray, kcmy)

COLOR_TO_KCMY_FUNC(gray, kcmy_raw, color_raw, raw, 8)
COLOR_TO_KCMY_FUNC(gray, kcmy_raw, color_raw, raw, 16)
GENERIC_COLOR_FUNC(gray, kcmy_raw)

COLOR_TO_KCMY_FUNC(color, kcmy, color, raw, 8)
COLOR_TO_KCMY_FUNC(color, kcmy, color, raw, 16)
GENERIC_COLOR_FUNC(color, kcmy)

COLOR_TO_KCMY_FUNC(color, kcmy_fast, color_fast, raw, 8)
COLOR_TO_KCMY_FUNC(color, kcmy_fast, color_fast, raw, 16)
GENERIC_COLOR_FUNC(color, kcmy_fast)

COLOR_TO_KCMY_FUNC(color, kcmy_raw, color_raw, raw, 8)
COLOR_TO_KCMY_FUNC(color, kcmy_raw, color_raw, raw, 16)
GENERIC_COLOR_FUNC(color, kcmy_raw)


#define COLOR_TO_KCMY_THRESHOLD_FUNC(T, name)				\
static unsigned								\
name##_to_kcmy_threshold(const stp_vars_t *vars,			\
			const unsigned char *in,			\
			unsigned short *out)				\
{									\
  int i;								\
  int z = 15;								\
  const T *s_in = (const T *) in;					\
  unsigned high_bit = ((1 << ((sizeof(T) * 8) - 1)));			\
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	\
  int width = lut->image_width;						\
  unsigned mask = 0;							\
  memset(out, 0, width * 4 * sizeof(unsigned short));			\
  if (lut->invert_output)						\
    mask = (1 << (sizeof(T) * 8)) - 1;					\
									\
  for (i = 0; i < width; i++, out += 4, s_in += 3)			\
    {									\
      unsigned c = s_in[0] ^ mask;					\
      unsigned m = s_in[1] ^ mask;					\
      unsigned y = s_in[2] ^ mask;					\
      unsigned k = (c < m ? (c < y ? c : y) : (m < y ? m : y));		\
      if (k >= high_bit)						\
	{								\
	  c -= k;							\
	  m -= k;							\
	  y -= k;							\
	}								\
      if (k >= high_bit)						\
	{								\
	  z &= 0xe;							\
	  out[0] = 65535;						\
	}								\
      if (c >= high_bit)						\
	{								\
	  z &= 0xd;							\
	  out[1] = 65535;						\
	}								\
      if (m >= high_bit)						\
	{								\
	  z &= 0xb;							\
	  out[2] = 65535;						\
	}								\
      if (y >= high_bit)						\
	{								\
	  z &= 0x7;							\
	  out[3] = 65535;						\
	}								\
    }									\
  return z;								\
}

COLOR_TO_KCMY_THRESHOLD_FUNC(unsigned char, color_8)
COLOR_TO_KCMY_THRESHOLD_FUNC(unsigned short, color_16)
GENERIC_COLOR_FUNC(color, kcmy_threshold)

#define CMYK_TO_KCMY_THRESHOLD_FUNC(T, name)				\
static unsigned								\
name##_to_kcmy_threshold(const stp_vars_t *vars,			\
			const unsigned char *in,			\
			unsigned short *out)				\
{									\
  int i;								\
  int z = 15;								\
  const T *s_in = (const T *) in;					\
  unsigned desired_high_bit = 0;					\
  unsigned high_bit = 1 << ((sizeof(T) * 8) - 1);			\
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	\
  int width = lut->image_width;						\
  memset(out, 0, width * 4 * sizeof(unsigned short));			\
  if (!lut->invert_output)						\
    desired_high_bit = high_bit;					\
									\
  for (i = 0; i < width; i++, out += 4, s_in += 4)			\
    {									\
      if ((s_in[3] & high_bit) == desired_high_bit)			\
	{								\
	  z &= 0xe;							\
	  out[0] = 65535;						\
	}								\
      if ((s_in[0] & high_bit) == desired_high_bit)			\
	{								\
	  z &= 0xd;							\
	  out[1] = 65535;						\
	}								\
      if ((s_in[1] & high_bit) == desired_high_bit)			\
	{								\
	  z &= 0xb;							\
	  out[2] = 65535;						\
	}								\
      if ((s_in[2] & high_bit) == desired_high_bit)			\
	{								\
	  z &= 0x7;							\
	  out[3] = 65535;						\
	}								\
    }									\
  return z;								\
}

CMYK_TO_KCMY_THRESHOLD_FUNC(unsigned char, cmyk_8)
CMYK_TO_KCMY_THRESHOLD_FUNC(unsigned short, cmyk_16)
GENERIC_COLOR_FUNC(cmyk, kcmy_threshold)

#define KCMY_TO_KCMY_THRESHOLD_FUNC(T, name)				\
static unsigned								\
name##_to_kcmy_threshold(const stp_vars_t *vars,			\
			 const unsigned char *in,			\
			 unsigned short *out)				\
{									\
  int i;								\
  int j;								\
  unsigned nz[4];							\
  unsigned z = 0xf;							\
  const T *s_in = (const T *) in;					\
  unsigned desired_high_bit = 0;					\
  unsigned high_bit = 1 << ((sizeof(T) * 8) - 1);			\
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	\
  int width = lut->image_width;						\
  memset(out, 0, width * 4 * sizeof(unsigned short));			\
  if (!lut->invert_output)						\
    desired_high_bit = high_bit;					\
  for (i = 0; i < 4; i++)						\
    nz[i] = z & ~(1 << i);						\
									\
  for (i = 0; i < width; i++)						\
    {									\
      for (j = 0; j < 4; j++)						\
	{								\
	  if ((*s_in++ & high_bit) == desired_high_bit)			\
	    {								\
	      z &= nz[j];						\
	      *out = 65535;						\
	    }								\
	  out++;							\
	}								\
    }									\
  return z;								\
}

KCMY_TO_KCMY_THRESHOLD_FUNC(unsigned char, kcmy_8)
KCMY_TO_KCMY_THRESHOLD_FUNC(unsigned short, kcmy_16)
GENERIC_COLOR_FUNC(kcmy, kcmy_threshold)

#define GRAY_TO_COLOR_THRESHOLD_FUNC(T, name, bits, channels)		\
static unsigned								\
gray_##bits##_to_##name##_threshold(const stp_vars_t *vars,		\
				    const unsigned char *in,		\
				    unsigned short *out)		\
{									\
  int i;								\
  int z = (1 << channels) - 1;						\
  int desired_high_bit = 0;						\
  unsigned high_bit = 1 << ((sizeof(T) * 8) - 1);			\
  const T *s_in = (const T *) in;					\
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	\
  int width = lut->image_width;						\
  memset(out, 0, width * channels * sizeof(unsigned short));		\
  if (!lut->invert_output)						\
    desired_high_bit = high_bit;					\
									\
  for (i = 0; i < width; i++, out += channels, s_in++)			\
    {									\
      if ((s_in[0] & high_bit) == desired_high_bit)			\
	{								\
	  int j;							\
	  z = 0;							\
	  for (j = 0; j < channels; j++)				\
	    out[j] = 65535;						\
	}								\
    }									\
  return z;								\
}


GRAY_TO_COLOR_THRESHOLD_FUNC(unsigned char, color, 8, 3)
GRAY_TO_COLOR_THRESHOLD_FUNC(unsigned short, color, 16, 3)
GENERIC_COLOR_FUNC(gray, color_threshold)

GRAY_TO_COLOR_THRESHOLD_FUNC(unsigned char, kcmy, 8, 4)
GRAY_TO_COLOR_THRESHOLD_FUNC(unsigned short, kcmy, 16, 4)
GENERIC_COLOR_FUNC(gray, kcmy_threshold)

#define COLOR_TO_COLOR_THRESHOLD_FUNC(T, name)				\
static unsigned								\
name##_to_color_threshold(const stp_vars_t *vars,			\
		       const unsigned char *in,				\
		       unsigned short *out)				\
{									\
  int i;								\
  int z = 7;								\
  int desired_high_bit = 0;						\
  unsigned high_bit = ((1 << ((sizeof(T) * 8) - 1)) * 4);		\
  const T *s_in = (const T *) in;					\
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	\
  int width = lut->image_width;						\
  memset(out, 0, width * 3 * sizeof(unsigned short));			\
  if (!lut->invert_output)						\
    desired_high_bit = high_bit;					\
									\
  for (i = 0; i < width; i++, out += 3, s_in += 3)			\
    {									\
      if ((s_in[0] & high_bit) == desired_high_bit)			\
	{								\
	  z &= 6;							\
	  out[0] = 65535;						\
	}								\
      if ((s_in[1] & high_bit) == desired_high_bit)			\
	{								\
	  z &= 5;							\
	  out[1] = 65535;						\
	}								\
      if ((s_in[1] & high_bit) == desired_high_bit)			\
	{								\
	  z &= 3;							\
	  out[2] = 65535;						\
	}								\
    }									\
  return z;								\
}

COLOR_TO_COLOR_THRESHOLD_FUNC(unsigned char, color_8)
COLOR_TO_COLOR_THRESHOLD_FUNC(unsigned short, color_16)
GENERIC_COLOR_FUNC(color, color_threshold)

#define COLOR_TO_GRAY_THRESHOLD_FUNC(T, name, channels, max_channels)	\
static unsigned								\
name##_to_gray_threshold(const stp_vars_t *vars,			\
			const unsigned char *in,			\
			unsigned short *out)				\
{									\
  int i;								\
  int z = 1;								\
  int desired_high_bit = 0;						\
  unsigned high_bit = ((1 << ((sizeof(T) * 8) - 1)));			\
  const T *s_in = (const T *) in;					\
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	\
  int width = lut->image_width;						\
  memset(out, 0, width * sizeof(unsigned short));			\
  if (!lut->invert_output)						\
    desired_high_bit = high_bit;					\
									\
  for (i = 0; i < width; i++, out++, s_in += channels)			\
    {									\
      unsigned gval =							\
	(max_channels - channels) * (1 << ((sizeof(T) * 8) - 1));	\
      int j;								\
      for (j = 0; j < channels; j++)					\
	gval += s_in[j];						\
      gval /= channels;							\
      if ((gval & high_bit) == desired_high_bit)			\
	{								\
	  out[0] = 65535;						\
	  z = 0;							\
	}								\
    }									\
  return z;								\
}

COLOR_TO_GRAY_THRESHOLD_FUNC(unsigned char, cmyk_8, 4, 4)
COLOR_TO_GRAY_THRESHOLD_FUNC(unsigned short, cmyk_16, 4, 4)
GENERIC_COLOR_FUNC(cmyk, gray_threshold)

COLOR_TO_GRAY_THRESHOLD_FUNC(unsigned char, kcmy_8, 4, 4)
COLOR_TO_GRAY_THRESHOLD_FUNC(unsigned short, kcmy_16, 4, 4)
GENERIC_COLOR_FUNC(kcmy, gray_threshold)

COLOR_TO_GRAY_THRESHOLD_FUNC(unsigned char, color_8, 3, 3)
COLOR_TO_GRAY_THRESHOLD_FUNC(unsigned short, color_16, 3, 3)
GENERIC_COLOR_FUNC(color, gray_threshold)

COLOR_TO_GRAY_THRESHOLD_FUNC(unsigned char, gray_8, 1, 1)
COLOR_TO_GRAY_THRESHOLD_FUNC(unsigned short, gray_16, 1, 1)
GENERIC_COLOR_FUNC(gray, gray_threshold)

#define CMYK_TO_COLOR_FUNC(namein, name2, T, bits, offset)		      \
static unsigned								      \
namein##_##bits##_to_##name2(const stp_vars_t *vars, const unsigned char *in, \
			   unsigned short *out)				      \
{									      \
  int i;								      \
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	      \
  unsigned status;							      \
  size_t real_steps = lut->steps;					      \
  const T *s_in = (const T *) in;					      \
  unsigned short *tmp;							      \
  int width = lut->image_width;						      \
  unsigned mask = 0;							      \
									      \
  if (!lut->cmy_tmp)							      \
    lut->cmy_tmp = stp_malloc(3 * 2 * lut->image_width);		      \
  tmp = lut->cmy_tmp;							      \
  memset(lut->cmy_tmp, 0, width * 3 * sizeof(unsigned short));		      \
  if (lut->invert_output)						      \
    mask = 0xffff;							      \
									      \
  for (i = 0; i < width; i++, tmp += 3, s_in += 4)			      \
    {									      \
      unsigned c = (s_in[0 + offset] + s_in[(3 + offset) % 4]) *	      \
	(65535 / ((1 << bits) - 1));					      \
      unsigned m = (s_in[1 + offset] + s_in[(3 + offset) % 4]) *	      \
	(65535 / ((1 << bits) - 1));					      \
      unsigned y = (s_in[2 + offset] + s_in[(3 + offset) % 4]) *	      \
	(65535 / ((1 << bits) - 1));					      \
      if (c > 65535)							      \
	c = 65535;							      \
      if (m > 65535)							      \
	m = 65535;							      \
      if (y > 65535)							      \
	y = 65535;							      \
      tmp[0] = c ^ mask;						      \
      tmp[1] = m ^ mask;						      \
      tmp[2] = y ^ mask;						      \
    }									      \
  lut->steps = 65536;							      \
  status =								      \
    color_16_to_##name2(vars, (const unsigned char *) lut->cmy_tmp, out);     \
  lut->steps = real_steps;						      \
  return status;							      \
}

CMYK_TO_COLOR_FUNC(cmyk, color, unsigned char, 8, 0)
CMYK_TO_COLOR_FUNC(cmyk, color, unsigned short, 16, 0)
GENERIC_COLOR_FUNC(cmyk, color)
CMYK_TO_COLOR_FUNC(kcmy, color, unsigned char, 8, 1)
CMYK_TO_COLOR_FUNC(kcmy, color, unsigned short, 16, 1)
GENERIC_COLOR_FUNC(kcmy, color)
CMYK_TO_COLOR_FUNC(cmyk, color_threshold, unsigned char, 8, 0)
CMYK_TO_COLOR_FUNC(cmyk, color_threshold, unsigned short, 16, 0)
GENERIC_COLOR_FUNC(cmyk, color_threshold)
CMYK_TO_COLOR_FUNC(kcmy, color_threshold, unsigned char, 8, 1)
CMYK_TO_COLOR_FUNC(kcmy, color_threshold, unsigned short, 16, 1)
GENERIC_COLOR_FUNC(kcmy, color_threshold)
CMYK_TO_COLOR_FUNC(cmyk, color_fast, unsigned char, 8, 0)
CMYK_TO_COLOR_FUNC(cmyk, color_fast, unsigned short, 16, 0)
GENERIC_COLOR_FUNC(cmyk, color_fast)
CMYK_TO_COLOR_FUNC(kcmy, color_fast, unsigned char, 8, 1)
CMYK_TO_COLOR_FUNC(kcmy, color_fast, unsigned short, 16, 1)
GENERIC_COLOR_FUNC(kcmy, color_fast)
CMYK_TO_COLOR_FUNC(cmyk, color_raw, unsigned char, 8, 0)
CMYK_TO_COLOR_FUNC(cmyk, color_raw, unsigned short, 16, 0)
GENERIC_COLOR_FUNC(cmyk, color_raw)
CMYK_TO_COLOR_FUNC(kcmy, color_raw, unsigned char, 8, 1)
CMYK_TO_COLOR_FUNC(kcmy, color_raw, unsigned short, 16, 1)
GENERIC_COLOR_FUNC(kcmy, color_raw)

#define CMYK_TO_KCMY_FUNC(T, size)					    \
static unsigned								    \
cmyk_##size##_to_kcmy(const stp_vars_t *vars,				    \
		      const unsigned char *in,				    \
		      unsigned short *out)				    \
{									    \
  int i;								    \
  unsigned retval = 0;							    \
  int j;								    \
  int nz[4];								    \
  const T *s_in = (const T *) in;					    \
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	    \
  const unsigned short *user;						    \
  const unsigned short *maps[4];					    \
									    \
  for (i = 0; i < 4; i++)						    \
    {									    \
      stp_curve_resample(lut->channel_curves[i].curve, 65536);		    \
      maps[i] = stp_curve_cache_get_ushort_data(&(lut->channel_curves[i])); \
    }									    \
  stp_curve_resample(lut->user_color_correction.curve, 1 << size);	    \
  user = stp_curve_cache_get_ushort_data(&(lut->user_color_correction));    \
									    \
  memset(nz, 0, sizeof(nz));						    \
									    \
  for (i = 0; i < lut->image_width; i++, out += 4)			    \
    {									    \
      for (j = 0; j < 4; j++)						    \
	{								    \
	  int outpos = (j + 1) & 3;					    \
	  int inval = *s_in++;						    \
	  nz[outpos] |= inval;						    \
	  out[outpos] = maps[outpos][user[inval]];			    \
	}								    \
    }									    \
  for (j = 0; j < 4; j++)						    \
    if (nz[j] == 0)							    \
      retval |= (1 << j);						    \
  return retval;							    \
}

CMYK_TO_KCMY_FUNC(unsigned char, 8)
CMYK_TO_KCMY_FUNC(unsigned short, 16)
GENERIC_COLOR_FUNC(cmyk, kcmy)

#define KCMY_TO_KCMY_FUNC(T, size)					    \
static unsigned								    \
kcmy_##size##_to_kcmy(const stp_vars_t *vars,				    \
		      const unsigned char *in,				    \
		      unsigned short *out)				    \
{									    \
  int i;								    \
  unsigned retval = 0;							    \
  int j;								    \
  int nz[4];								    \
  const T *s_in = (const T *) in;					    \
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	    \
  const unsigned short *user;						    \
  const unsigned short *maps[4];					    \
									    \
  for (i = 0; i < 4; i++)						    \
    {									    \
      stp_curve_resample(lut->channel_curves[i].curve, 65536);		    \
      maps[i] = stp_curve_cache_get_ushort_data(&(lut->channel_curves[i])); \
    }									    \
  stp_curve_resample(lut->user_color_correction.curve, 1 << size);	    \
  user = stp_curve_cache_get_ushort_data(&(lut->user_color_correction));    \
									    \
  memset(nz, 0, sizeof(nz));						    \
									    \
  for (i = 0; i < lut->image_width; i++, out += 4)			    \
    {									    \
      for (j = 0; j < 4; j++)						    \
	{								    \
	  int inval = *s_in++;						    \
	  nz[j] |= inval;						    \
	  out[j] = maps[j][user[inval]];				    \
	}								    \
    }									    \
  for (j = 0; j < 4; j++)						    \
    if (nz[j] == 0)							    \
      retval |= (1 << j);						    \
  return retval;							    \
}

KCMY_TO_KCMY_FUNC(unsigned char, 8)
KCMY_TO_KCMY_FUNC(unsigned short, 16)
GENERIC_COLOR_FUNC(kcmy, kcmy)


#define GRAY_TO_GRAY_FUNC(T, bits)					   \
static unsigned								   \
gray_##bits##_to_gray(const stp_vars_t *vars,				   \
		      const unsigned char *in,				   \
		      unsigned short *out)				   \
{									   \
  int i;								   \
  int i0 = -1;								   \
  int o0 = 0;								   \
  int nz = 0;								   \
  const T *s_in = (const T *) in;					   \
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	   \
  int width = lut->image_width;						   \
  const unsigned short *composite;					   \
  const unsigned short *user;						   \
									   \
  stp_curve_resample							   \
    (stp_curve_cache_get_curve(&(lut->channel_curves[CHANNEL_K])), 65536); \
  composite =								   \
    stp_curve_cache_get_ushort_data(&(lut->channel_curves[CHANNEL_K]));	   \
  stp_curve_resample(lut->user_color_correction.curve, 1 << bits);	   \
  user = stp_curve_cache_get_ushort_data(&(lut->user_color_correction));   \
									   \
  memset(out, 0, width * sizeof(unsigned short));			   \
									   \
  for (i = 0; i < lut->image_width; i++)				   \
    {									   \
      if (i0 != s_in[0])						   \
	{								   \
	  i0 = s_in[0];							   \
	  o0 = composite[user[i0]];					   \
	  nz |= o0;							   \
	}								   \
      out[0] = o0;							   \
      s_in ++;								   \
      out ++;								   \
    }									   \
  return nz == 0;							   \
}

GRAY_TO_GRAY_FUNC(unsigned char, 8)
GRAY_TO_GRAY_FUNC(unsigned short, 16)
GENERIC_COLOR_FUNC(gray, gray)

#define COLOR_TO_GRAY_FUNC(T, bits)					      \
static unsigned								      \
color_##bits##_to_gray(const stp_vars_t *vars,				      \
		       const unsigned char *in,				      \
		       unsigned short *out)				      \
{									      \
  int i;								      \
  int i0 = -1;								      \
  int i1 = -1;								      \
  int i2 = -1;								      \
  int o0 = 0;								      \
  int nz = 0;								      \
  const T *s_in = (const T *) in;					      \
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	      \
  int l_red = LUM_RED;							      \
  int l_green = LUM_GREEN;						      \
  int l_blue = LUM_BLUE;						      \
  const unsigned short *composite;					      \
  const unsigned short *user;						      \
									      \
  stp_curve_resample							      \
    (stp_curve_cache_get_curve(&(lut->channel_curves[CHANNEL_K])), 65536);    \
  composite =								      \
    stp_curve_cache_get_ushort_data(&(lut->channel_curves[CHANNEL_K]));	      \
  stp_curve_resample(lut->user_color_correction.curve, 1 << bits);	      \
  user = stp_curve_cache_get_ushort_data(&(lut->user_color_correction));      \
									      \
  if (lut->input_color_description->color_model == COLOR_BLACK)		      \
    {									      \
      l_red = (100 - l_red) / 2;					      \
      l_green = (100 - l_green) / 2;					      \
      l_blue = (100 - l_blue) / 2;					      \
    }									      \
									      \
  for (i = 0; i < lut->image_width; i++)				      \
    {									      \
      if (i0 != s_in[0] || i1 != s_in[1] || i2 != s_in[2])		      \
	{								      \
	  i0 = s_in[0];							      \
	  i1 = s_in[1];							      \
	  i2 = s_in[2];							      \
	  o0 =								      \
	    composite[user[(i0 * l_red + i1 * l_green + i2 * l_blue) / 100]]; \
	  nz |= o0;							      \
	}								      \
      out[0] = o0;							      \
      s_in += 3;							      \
      out ++;								      \
    }									      \
  return nz == 0;							      \
}

COLOR_TO_GRAY_FUNC(unsigned char, 8)
COLOR_TO_GRAY_FUNC(unsigned short, 16)
GENERIC_COLOR_FUNC(color, gray)


#define CMYK_TO_GRAY_FUNC(T, bits)					    \
static unsigned								    \
cmyk_##bits##_to_gray(const stp_vars_t *vars,				    \
		      const unsigned char *in,				    \
		      unsigned short *out)				    \
{									    \
  int i;								    \
  int i0 = -1;								    \
  int i1 = -1;								    \
  int i2 = -1;								    \
  int i3 = -4;								    \
  int o0 = 0;								    \
  int nz = 0;								    \
  const T *s_in = (const T *) in;					    \
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	    \
  int l_red = LUM_RED;							    \
  int l_green = LUM_GREEN;						    \
  int l_blue = LUM_BLUE;						    \
  int l_white = 0;							    \
  const unsigned short *composite;					    \
  const unsigned short *user;						    \
									    \
  stp_curve_resample							    \
    (stp_curve_cache_get_curve(&(lut->channel_curves[CHANNEL_K])), 65536);  \
  composite =								    \
    stp_curve_cache_get_ushort_data(&(lut->channel_curves[CHANNEL_K]));	    \
  stp_curve_resample(lut->user_color_correction.curve, 1 << bits);	    \
  user = stp_curve_cache_get_ushort_data(&(lut->user_color_correction));    \
									    \
  if (lut->input_color_description->color_model == COLOR_BLACK)		    \
    {									    \
      l_red = (100 - l_red) / 3;					    \
      l_green = (100 - l_green) / 3;					    \
      l_blue = (100 - l_blue) / 3;					    \
      l_white = (100 - l_white) / 3;					    \
    }									    \
									    \
  for (i = 0; i < lut->image_width; i++)				    \
    {									    \
      if (i0 != s_in[0] || i1 != s_in[1] || i2 != s_in[2] || i3 != s_in[3]) \
	{								    \
	  i0 = s_in[0];							    \
	  i1 = s_in[1];							    \
	  i2 = s_in[2];							    \
	  i3 = s_in[3];							    \
	  o0 = composite[user[(i0 * l_red + i1 * l_green +		    \
			  i2 * l_blue + i3 * l_white) / 100]];		    \
	  nz |= o0;							    \
	}								    \
      out[0] = o0;							    \
      s_in += 4;							    \
      out ++;								    \
    }									    \
  return nz ? 0 : 1;							    \
}

CMYK_TO_GRAY_FUNC(unsigned char, 8)
CMYK_TO_GRAY_FUNC(unsigned short, 16)
GENERIC_COLOR_FUNC(cmyk, gray)

#define KCMY_TO_GRAY_FUNC(T, bits)					    \
static unsigned								    \
kcmy_##bits##_to_gray(const stp_vars_t *vars,				    \
		      const unsigned char *in,				    \
		      unsigned short *out)				    \
{									    \
  int i;								    \
  int i0 = -1;								    \
  int i1 = -1;								    \
  int i2 = -1;								    \
  int i3 = -4;								    \
  int o0 = 0;								    \
  int nz = 0;								    \
  const T *s_in = (const T *) in;					    \
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	    \
  int l_red = LUM_RED;							    \
  int l_green = LUM_GREEN;						    \
  int l_blue = LUM_BLUE;						    \
  int l_white = 0;							    \
  const unsigned short *composite;					    \
  const unsigned short *user;						    \
									    \
  stp_curve_resample							    \
    (stp_curve_cache_get_curve(&(lut->channel_curves[CHANNEL_K])), 65536);  \
  composite =								    \
    stp_curve_cache_get_ushort_data(&(lut->channel_curves[CHANNEL_K]));	    \
  stp_curve_resample(lut->user_color_correction.curve, 1 << bits);	    \
  user = stp_curve_cache_get_ushort_data(&(lut->user_color_correction));    \
									    \
  if (lut->input_color_description->color_model == COLOR_BLACK)		    \
    {									    \
      l_red = (100 - l_red) / 3;					    \
      l_green = (100 - l_green) / 3;					    \
      l_blue = (100 - l_blue) / 3;					    \
      l_white = (100 - l_white) / 3;					    \
    }									    \
									    \
  for (i = 0; i < lut->image_width; i++)				    \
    {									    \
      if (i0 != s_in[0] || i1 != s_in[1] || i2 != s_in[2] || i3 != s_in[3]) \
	{								    \
	  i0 = s_in[0];							    \
	  i1 = s_in[1];							    \
	  i2 = s_in[2];							    \
	  i3 = s_in[3];							    \
	  o0 = composite[user[(i0 * l_red + i1 * l_green +		    \
			  i2 * l_blue + i3 * l_white) / 100]];		    \
	  nz |= o0;							    \
	}								    \
      out[0] = o0;							    \
      s_in += 4;							    \
      out ++;								    \
    }									    \
  return nz ? 0 : 1;							    \
}

KCMY_TO_GRAY_FUNC(unsigned char, 8)
KCMY_TO_GRAY_FUNC(unsigned short, 16)
GENERIC_COLOR_FUNC(kcmy, gray)

#define GRAY_TO_GRAY_RAW_FUNC(T, bits)					\
static unsigned								\
gray_##bits##_to_gray_raw(const stp_vars_t *vars,			\
			  const unsigned char *in,			\
			  unsigned short *out)				\
{									\
  int i;								\
  int nz = 0;								\
  const T *s_in = (const T *) in;					\
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	\
  int width = lut->image_width;						\
  unsigned mask = 0;							\
  if (lut->invert_output)						\
    mask = 0xffff;							\
									\
  memset(out, 0, width * sizeof(unsigned short));			\
									\
  for (i = 0; i < lut->image_width; i++)				\
    {									\
      out[0] = (s_in[0] * (65535 / ((1 << bits) - 1))) ^ mask;		\
      nz |= out[0];							\
      s_in ++;								\
      out ++;								\
    }									\
  return nz == 0;							\
}

GRAY_TO_GRAY_RAW_FUNC(unsigned char, 8)
GRAY_TO_GRAY_RAW_FUNC(unsigned short, 16)
GENERIC_COLOR_FUNC(gray, gray_raw)

#define COLOR_TO_GRAY_RAW_FUNC(T, bits, invertable, name2)		\
static unsigned								\
color_##bits##_to_gray_##name2(const stp_vars_t *vars,			\
			       const unsigned char *in,			\
			       unsigned short *out)			\
{									\
  int i;								\
  int i0 = -1;								\
  int i1 = -1;								\
  int i2 = -1;								\
  int o0 = 0;								\
  int nz = 0;								\
  const T *s_in = (const T *) in;					\
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	\
  int l_red = LUM_RED;							\
  int l_green = LUM_GREEN;						\
  int l_blue = LUM_BLUE;						\
  unsigned mask = 0;							\
  if (lut->invert_output && invertable)					\
    mask = 0xffff;							\
									\
  if (lut->input_color_description->color_model == COLOR_BLACK)		\
    {									\
      l_red = (100 - l_red) / 2;					\
      l_green = (100 - l_green) / 2;					\
      l_blue = (100 - l_blue) / 2;					\
    }									\
									\
  for (i = 0; i < lut->image_width; i++)				\
    {									\
      if (i0 != s_in[0] || i1 != s_in[1] || i2 != s_in[2])		\
	{								\
	  i0 = s_in[0];							\
	  i1 = s_in[1];							\
	  i2 = s_in[2];							\
	  o0 = (i0 * (65535 / ((1 << bits) - 1)) * l_red +		\
		i1 * (65535 / ((1 << bits) - 1)) * l_green +		\
		i2 * (65535 / ((1 << bits) - 1)) * l_blue) / 100;	\
	  o0 ^= mask;							\
	  nz |= o0;							\
	}								\
      out[0] = o0;							\
      s_in += 3;							\
      out ++;								\
    }									\
  return nz == 0;							\
}

COLOR_TO_GRAY_RAW_FUNC(unsigned char, 8, 1, raw)
COLOR_TO_GRAY_RAW_FUNC(unsigned short, 16, 1, raw)
GENERIC_COLOR_FUNC(color, gray_raw)
COLOR_TO_GRAY_RAW_FUNC(unsigned char, 8, 0, noninvert)
COLOR_TO_GRAY_RAW_FUNC(unsigned short, 16, 0, noninvert)


#define CMYK_TO_GRAY_RAW_FUNC(T, bits, invertable, name2)		    \
static unsigned								    \
cmyk_##bits##_to_gray_##name2(const stp_vars_t *vars,			    \
			      const unsigned char *in,			    \
			      unsigned short *out)			    \
{									    \
  int i;								    \
  int i0 = -1;								    \
  int i1 = -1;								    \
  int i2 = -1;								    \
  int i3 = -4;								    \
  int o0 = 0;								    \
  int nz = 0;								    \
  const T *s_in = (const T *) in;					    \
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	    \
  int l_red = LUM_RED;							    \
  int l_green = LUM_GREEN;						    \
  int l_blue = LUM_BLUE;						    \
  int l_white = 0;							    \
  unsigned mask = 0;							    \
  if (lut->invert_output && invertable)					    \
    mask = 0xffff;							    \
									    \
  if (lut->input_color_description->color_model == COLOR_BLACK)		    \
    {									    \
      l_red = (100 - l_red) / 3;					    \
      l_green = (100 - l_green) / 3;					    \
      l_blue = (100 - l_blue) / 3;					    \
      l_white = (100 - l_white) / 3;					    \
    }									    \
									    \
  for (i = 0; i < lut->image_width; i++)				    \
    {									    \
      if (i0 != s_in[0] || i1 != s_in[1] || i2 != s_in[2] || i3 != s_in[3]) \
	{								    \
	  i0 = s_in[0];							    \
	  i1 = s_in[1];							    \
	  i2 = s_in[2];							    \
	  i3 = s_in[3];							    \
	  o0 = (i0 * (65535 / ((1 << bits) - 1)) * l_red +		    \
		i1 * (65535 / ((1 << bits) - 1)) * l_green +		    \
		i2 * (65535 / ((1 << bits) - 1)) * l_blue +		    \
		i3 * (65535 / ((1 << bits) - 1)) * l_white) / 100;	    \
	  o0 ^= mask;							    \
	  nz |= o0;							    \
	}								    \
      out[0] = o0;							    \
      s_in += 4;							    \
      out ++;								    \
    }									    \
  return nz ? 0 : 1;							    \
}

CMYK_TO_GRAY_RAW_FUNC(unsigned char, 8, 1, raw)
CMYK_TO_GRAY_RAW_FUNC(unsigned short, 16, 1, raw)
GENERIC_COLOR_FUNC(cmyk, gray_raw)
CMYK_TO_GRAY_RAW_FUNC(unsigned char, 8, 0, noninvert)
CMYK_TO_GRAY_RAW_FUNC(unsigned short, 16, 0, noninvert)

#define KCMY_TO_GRAY_RAW_FUNC(T, bits, invertable, name2)		    \
static unsigned								    \
kcmy_##bits##_to_gray_##name2(const stp_vars_t *vars,			    \
			      const unsigned char *in,			    \
			      unsigned short *out)			    \
{									    \
  int i;								    \
  int i0 = -1;								    \
  int i1 = -1;								    \
  int i2 = -1;								    \
  int i3 = -4;								    \
  int o0 = 0;								    \
  int nz = 0;								    \
  const T *s_in = (const T *) in;					    \
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	    \
  int l_red = LUM_RED;							    \
  int l_green = LUM_GREEN;						    \
  int l_blue = LUM_BLUE;						    \
  int l_white = 0;							    \
  unsigned mask = 0;							    \
  if (lut->invert_output && invertable)					    \
    mask = 0xffff;							    \
									    \
  if (lut->input_color_description->color_model == COLOR_BLACK)		    \
    {									    \
      l_red = (100 - l_red) / 3;					    \
      l_green = (100 - l_green) / 3;					    \
      l_blue = (100 - l_blue) / 3;					    \
      l_white = (100 - l_white) / 3;					    \
    }									    \
									    \
  for (i = 0; i < lut->image_width; i++)				    \
    {									    \
      if (i0 != s_in[0] || i1 != s_in[1] || i2 != s_in[2] || i3 != s_in[3]) \
	{								    \
	  i0 = s_in[0];							    \
	  i1 = s_in[1];							    \
	  i2 = s_in[2];							    \
	  i3 = s_in[3];							    \
	  o0 = (i0 * (65535 / ((1 << bits) - 1)) * l_white +		    \
		i1 * (65535 / ((1 << bits) - 1)) * l_red +		    \
		i2 * (65535 / ((1 << bits) - 1)) * l_green +		    \
		i3 * (65535 / ((1 << bits) - 1)) * l_blue) / 100;	    \
	  o0 ^= mask;							    \
	  nz |= o0;							    \
	}								    \
      out[0] = o0;							    \
      s_in += 4;							    \
      out ++;								    \
    }									    \
  return nz ? 0 : 1;							    \
}

KCMY_TO_GRAY_RAW_FUNC(unsigned char, 8, 1, raw)
KCMY_TO_GRAY_RAW_FUNC(unsigned short, 16, 1, raw)
GENERIC_COLOR_FUNC(kcmy, gray_raw)
KCMY_TO_GRAY_RAW_FUNC(unsigned char, 8, 0, noninvert)
KCMY_TO_GRAY_RAW_FUNC(unsigned short, 16, 0, noninvert)

#define CMYK_TO_KCMY_RAW_FUNC(T, bits)					\
static unsigned								\
cmyk_##bits##_to_kcmy_raw(const stp_vars_t *vars,			\
			  const unsigned char *in,			\
			  unsigned short *out)				\
{									\
  int i;								\
  int j;								\
  int nz[4];								\
  unsigned retval = 0;							\
  const T *s_in = (const T *) in;					\
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	\
									\
  memset(nz, 0, sizeof(nz));						\
  for (i = 0; i < lut->image_width; i++)				\
    {									\
      out[0] = s_in[3] * (65535 / ((1 << bits) - 1));			\
      out[1] = s_in[0] * (65535 / ((1 << bits) - 1));			\
      out[2] = s_in[1] * (65535 / ((1 << bits) - 1));			\
      out[3] = s_in[2] * (65535 / ((1 << bits) - 1));			\
      for (j = 0; j < 4; j++)						\
	nz[j] |= out[j];						\
      s_in += 4;							\
      out += 4;								\
    }									\
  for (j = 0; j < 4; j++)						\
    if (nz[j] == 0)							\
      retval |= (1 << j);						\
  return retval;							\
}

CMYK_TO_KCMY_RAW_FUNC(unsigned char, 8)
CMYK_TO_KCMY_RAW_FUNC(unsigned short, 16)
GENERIC_COLOR_FUNC(cmyk, kcmy_raw)

#define KCMY_TO_KCMY_RAW_FUNC(T, bits)					\
static unsigned								\
kcmy_##bits##_to_kcmy_raw(const stp_vars_t *vars,			\
			  const unsigned char *in,			\
			  unsigned short *out)				\
{									\
  int i;								\
  int j;								\
  int nz[4];								\
  unsigned retval = 0;							\
  const T *s_in = (const T *) in;					\
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	\
									\
  memset(nz, 0, sizeof(nz));						\
  for (i = 0; i < lut->image_width; i++)				\
    {									\
      for (j = 0; j < 4; j++)						\
	{								\
	  out[j] = s_in[j] * (65535 / ((1 << bits) - 1));		\
	  nz[j] |= out[j];						\
	}								\
      s_in += 4;							\
      out += 4;								\
    }									\
  for (j = 0; j < 4; j++)						\
    if (nz[j] == 0)							\
      retval |= (1 << j);						\
  return retval;							\
}

KCMY_TO_KCMY_RAW_FUNC(unsigned char, 8)
KCMY_TO_KCMY_RAW_FUNC(unsigned short, 16)
GENERIC_COLOR_FUNC(kcmy, kcmy_raw)

#define DESATURATED_FUNC(name, name2, bits)				   \
static unsigned								   \
name##_##bits##_to_##name2##_desaturated(const stp_vars_t *vars,	   \
				         const unsigned char *in,	   \
				         unsigned short *out)		   \
{									   \
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	   \
  size_t real_steps = lut->steps;					   \
  unsigned status;							   \
  if (!lut->gray_tmp)							   \
    lut->gray_tmp = stp_malloc(2 * lut->image_width);			   \
  name##_##bits##_to_gray_noninvert(vars, in, lut->gray_tmp);		   \
  lut->steps = 65536;							   \
  status = gray_16_to_##name2(vars, (unsigned char *) lut->gray_tmp, out); \
  lut->steps = real_steps;						   \
  return status;							   \
}

DESATURATED_FUNC(color, color, 8)
DESATURATED_FUNC(color, color, 16)
GENERIC_COLOR_FUNC(color, color_desaturated)
DESATURATED_FUNC(color, kcmy, 8)
DESATURATED_FUNC(color, kcmy, 16)
GENERIC_COLOR_FUNC(color, kcmy_desaturated)

DESATURATED_FUNC(cmyk, color, 8)
DESATURATED_FUNC(cmyk, color, 16)
GENERIC_COLOR_FUNC(cmyk, color_desaturated)
DESATURATED_FUNC(cmyk, kcmy, 8)
DESATURATED_FUNC(cmyk, kcmy, 16)
GENERIC_COLOR_FUNC(cmyk, kcmy_desaturated)

DESATURATED_FUNC(kcmy, color, 8)
DESATURATED_FUNC(kcmy, color, 16)
GENERIC_COLOR_FUNC(kcmy, color_desaturated)
DESATURATED_FUNC(kcmy, kcmy, 8)
DESATURATED_FUNC(kcmy, kcmy, 16)
GENERIC_COLOR_FUNC(kcmy, kcmy_desaturated)

#define CMYK_DISPATCH(name)						\
static unsigned								\
CMYK_to_##name(const stp_vars_t *vars, const unsigned char *in,		\
	       unsigned short *out)					\
{									\
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	\
  if (lut->input_color_description->color_id == COLOR_ID_CMYK)		\
    return cmyk_to_##name(vars, in, out);				\
  else if (lut->input_color_description->color_id == COLOR_ID_KCMY)	\
    return kcmy_to_##name(vars, in, out);				\
  else									\
    {									\
      stp_eprintf(vars, "Bad dispatch to CMYK_to_%s: %d\n", #name,	\
		  lut->input_color_description->color_id);		\
      return 0;								\
    }									\
}

CMYK_DISPATCH(color)
CMYK_DISPATCH(color_raw)
CMYK_DISPATCH(color_fast)
CMYK_DISPATCH(color_threshold)
CMYK_DISPATCH(color_desaturated)
CMYK_DISPATCH(kcmy)
CMYK_DISPATCH(kcmy_raw)
CMYK_DISPATCH(kcmy_threshold)
CMYK_DISPATCH(kcmy_desaturated)
CMYK_DISPATCH(gray)
CMYK_DISPATCH(gray_raw)
CMYK_DISPATCH(gray_threshold)

#define RAW_TO_RAW_THRESHOLD_FUNC(T, name)				\
static unsigned								\
name##_to_raw_threshold(const stp_vars_t *vars,				\
			const unsigned char *in,			\
			unsigned short *out)				\
{									\
  int i;								\
  int j;								\
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	\
  unsigned nz[STP_CHANNEL_LIMIT];					\
  unsigned z = (1 << lut->out_channels) - 1;				\
  const T *s_in = (const T *) in;					\
  unsigned desired_high_bit = 0;					\
  unsigned high_bit = 1 << ((sizeof(T) * 8) - 1);			\
  int width = lut->image_width;						\
  memset(out, 0, width * lut->out_channels * sizeof(unsigned short));	\
  if (!lut->invert_output)						\
    desired_high_bit = high_bit;					\
  for (i = 0; i < lut->out_channels; i++)				\
    nz[i] = z & ~(1 << i);						\
									\
  for (i = 0; i < width; i++)						\
    {									\
      for (j = 0; j < lut->out_channels; j++)				\
	{								\
	  if ((*s_in++ & high_bit) == desired_high_bit)			\
	    {								\
	      z &= nz[j];						\
	      *out = 65535;						\
	    }								\
	  out++;							\
	}								\
    }									\
  return z;								\
}

RAW_TO_RAW_THRESHOLD_FUNC(unsigned char, raw_8)
RAW_TO_RAW_THRESHOLD_FUNC(unsigned short, raw_16)
GENERIC_COLOR_FUNC(raw, raw_threshold)

#define RAW_TO_RAW_FUNC(T, size)					    \
static unsigned								    \
raw_##size##_to_raw(const stp_vars_t *vars,				    \
		    const unsigned char *in,				    \
		    unsigned short *out)				    \
{									    \
  int i;								    \
  unsigned retval = 0;							    \
  int j;								    \
  int nz[STP_CHANNEL_LIMIT];						    \
  const T *s_in = (const T *) in;					    \
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	    \
  const unsigned short *maps[STP_CHANNEL_LIMIT];			    \
  const unsigned short *user;						    \
									    \
  for (i = 0; i < lut->out_channels; i++)				    \
    {									    \
      stp_curve_resample(lut->channel_curves[i].curve, 65536);		    \
      maps[i] = stp_curve_cache_get_ushort_data(&(lut->channel_curves[i])); \
    }									    \
  stp_curve_resample(lut->user_color_correction.curve, 1 << size);	    \
  user = stp_curve_cache_get_ushort_data(&(lut->user_color_correction));    \
									    \
  memset(nz, 0, sizeof(nz));						    \
									    \
  for (i = 0; i < lut->image_width; i++, out += lut->out_channels)	    \
    {									    \
      for (j = 0; j < lut->out_channels; j++)				    \
	{								    \
	  int inval = *s_in++;						    \
	  nz[j] |= inval;						    \
	  out[j] = maps[j][user[inval]];				    \
	}								    \
    }									    \
  for (j = 0; j < lut->out_channels; j++)				    \
    if (nz[j] == 0)							    \
      retval |= (1 << j);						    \
  return retval;							    \
}

RAW_TO_RAW_FUNC(unsigned char, 8)
RAW_TO_RAW_FUNC(unsigned short, 16)
GENERIC_COLOR_FUNC(raw, raw)


#define RAW_TO_RAW_RAW_FUNC(T, bits)					\
static unsigned								\
raw_##bits##_to_raw_raw(const stp_vars_t *vars,				\
		        const unsigned char *in,			\
		        unsigned short *out)				\
{									\
  int i;								\
  int j;								\
  int nz[STP_CHANNEL_LIMIT];						\
  unsigned retval = 0;							\
  const T *s_in = (const T *) in;					\
  lut_t *lut = (lut_t *)(stp_get_component_data(vars, "Color"));	\
  int colors = lut->in_channels;					\
									\
  memset(nz, 0, sizeof(nz));						\
  for (i = 0; i < lut->image_width; i++)				\
    {									\
      for (j = 0; j < colors; j++)					\
	{								\
	  nz[j] |= s_in[j];						\
	  out[j] = s_in[j] * (65535 / ((1 << bits) - 1));		\
	}								\
      s_in += colors;							\
      out += colors;							\
    }									\
  for (j = 0; j < colors; j++)						\
    if (nz[j] == 0)							\
      retval |= (1 << j);						\
  return retval;							\
}

RAW_TO_RAW_RAW_FUNC(unsigned char, 8)
RAW_TO_RAW_RAW_FUNC(unsigned short, 16)
GENERIC_COLOR_FUNC(raw, raw_raw)


#define CONVERSION_FUNCTION_WITH_FAST(from, to, from2)		\
static unsigned							\
generic_##from##_to_##to(const stp_vars_t *v,			\
			 const unsigned char *in,		\
			 unsigned short *out)			\
{								\
  lut_t *lut = (lut_t *)(stp_get_component_data(v, "Color"));	\
  switch (lut->color_correction->correction)			\
    {								\
    case COLOR_CORRECTION_UNCORRECTED:				\
      return from2##_to_##to##_fast(v, in, out);		\
    case COLOR_CORRECTION_ACCURATE:				\
    case COLOR_CORRECTION_BRIGHT:				\
    case COLOR_CORRECTION_HUE:					\
      return from2##_to_##to(v, in, out);			\
    case COLOR_CORRECTION_DESATURATED:				\
      return from2##_to_##to##_desaturated(v, in, out);		\
    case COLOR_CORRECTION_THRESHOLD:				\
    case COLOR_CORRECTION_PREDITHERED:				\
      return from2##_to_##to##_threshold(v, in, out);		\
    case COLOR_CORRECTION_DENSITY:				\
    case COLOR_CORRECTION_RAW:					\
      return from2##_to_##to##_raw(v, in, out);			\
    default:							\
      return (unsigned) -1;					\
    }								\
}

#define CONVERSION_FUNCTION_WITHOUT_FAST(from, to, from2)	\
static unsigned							\
generic_##from##_to_##to(const stp_vars_t *v,			\
			 const unsigned char *in,		\
			 unsigned short *out)			\
{								\
  lut_t *lut = (lut_t *)(stp_get_component_data(v, "Color"));	\
  switch (lut->color_correction->correction)			\
    {								\
    case COLOR_CORRECTION_UNCORRECTED:				\
    case COLOR_CORRECTION_ACCURATE:				\
    case COLOR_CORRECTION_BRIGHT:				\
    case COLOR_CORRECTION_HUE:					\
      return from2##_to_##to(v, in, out);			\
    case COLOR_CORRECTION_DESATURATED:				\
      return from2##_to_##to##_desaturated(v, in, out);		\
    case COLOR_CORRECTION_THRESHOLD:				\
    case COLOR_CORRECTION_PREDITHERED:				\
      return from2##_to_##to##_threshold(v, in, out);		\
    case COLOR_CORRECTION_DENSITY:				\
    case COLOR_CORRECTION_RAW:					\
      return from2##_to_##to##_raw(v, in, out);			\
    default:							\
      return (unsigned) -1;					\
    }								\
}

#define CONVERSION_FUNCTION_WITHOUT_DESATURATED(from, to, from2)	\
static unsigned								\
generic_##from##_to_##to(const stp_vars_t *v,				\
			 const unsigned char *in,			\
			 unsigned short *out)				\
{									\
  lut_t *lut = (lut_t *)(stp_get_component_data(v, "Color"));		\
  switch (lut->color_correction->correction)				\
    {									\
    case COLOR_CORRECTION_UNCORRECTED:					\
    case COLOR_CORRECTION_ACCURATE:					\
    case COLOR_CORRECTION_BRIGHT:					\
    case COLOR_CORRECTION_HUE:						\
    case COLOR_CORRECTION_DESATURATED:					\
      return from2##_to_##to(v, in, out);				\
    case COLOR_CORRECTION_THRESHOLD:					\
    case COLOR_CORRECTION_PREDITHERED:					\
      return from2##_to_##to##_threshold(v, in, out);			\
    case COLOR_CORRECTION_DENSITY:					\
    case COLOR_CORRECTION_RAW:						\
      return from2##_to_##to##_raw(v, in, out);				\
    default:								\
      return (unsigned) -1;						\
    }									\
}

CONVERSION_FUNCTION_WITH_FAST(cmyk, color, CMYK)
CONVERSION_FUNCTION_WITH_FAST(color, color, color)
CONVERSION_FUNCTION_WITH_FAST(color, kcmy, color)
CONVERSION_FUNCTION_WITHOUT_FAST(cmyk, kcmy, CMYK)
CONVERSION_FUNCTION_WITHOUT_DESATURATED(cmyk, gray, CMYK)
CONVERSION_FUNCTION_WITHOUT_DESATURATED(color, gray, color)
CONVERSION_FUNCTION_WITHOUT_DESATURATED(gray, gray, gray)
CONVERSION_FUNCTION_WITHOUT_DESATURATED(gray, color, gray)
CONVERSION_FUNCTION_WITHOUT_DESATURATED(gray, kcmy, gray)

unsigned
stpi_color_convert_to_gray(const stp_vars_t *v,
			   const unsigned char *in,
			   unsigned short *out)
{
  lut_t *lut = (lut_t *)(stp_get_component_data(v, "Color"));
  switch (lut->input_color_description->color_id)
    {
    case COLOR_ID_GRAY:
    case COLOR_ID_WHITE:
      return generic_gray_to_gray(v, in, out);
    case COLOR_ID_RGB:
    case COLOR_ID_CMY:
      return generic_color_to_gray(v, in, out);
    case COLOR_ID_CMYK:
    case COLOR_ID_KCMY:
      return generic_cmyk_to_gray(v, in, out);
    default:
      return (unsigned) -1;
    }
}

unsigned
stpi_color_convert_to_color(const stp_vars_t *v,
			    const unsigned char *in,
			    unsigned short *out)
{
  lut_t *lut = (lut_t *)(stp_get_component_data(v, "Color"));
  switch (lut->input_color_description->color_id)
    {
    case COLOR_ID_GRAY:
    case COLOR_ID_WHITE:
      return generic_gray_to_color(v, in, out);
    case COLOR_ID_RGB:
    case COLOR_ID_CMY:
      return generic_color_to_color(v, in, out);
    case COLOR_ID_CMYK:
    case COLOR_ID_KCMY:
      return generic_cmyk_to_color(v, in, out);
    default:
      return (unsigned) -1;
    }
}

unsigned
stpi_color_convert_to_kcmy(const stp_vars_t *v,
			   const unsigned char *in,
			   unsigned short *out)
{
  lut_t *lut = (lut_t *)(stp_get_component_data(v, "Color"));
  switch (lut->input_color_description->color_id)
    {
    case COLOR_ID_GRAY:
    case COLOR_ID_WHITE:
      return generic_gray_to_kcmy(v, in, out);
    case COLOR_ID_RGB:
    case COLOR_ID_CMY:
      return generic_color_to_kcmy(v, in, out);
    case COLOR_ID_CMYK:
    case COLOR_ID_KCMY:
      return generic_cmyk_to_kcmy(v, in, out);
    default:
      return (unsigned) -1;
    }
}

unsigned
stpi_color_convert_raw(const stp_vars_t *v,
		       const unsigned char *in,
		       unsigned short *out)
{
  lut_t *lut = (lut_t *)(stp_get_component_data(v, "Color"));
  switch (lut->color_correction->correction)
    {
    case COLOR_CORRECTION_THRESHOLD:
    case COLOR_CORRECTION_PREDITHERED:
      return raw_to_raw_threshold(v, in, out);
    case COLOR_CORRECTION_UNCORRECTED:
    case COLOR_CORRECTION_BRIGHT:
    case COLOR_CORRECTION_HUE:
    case COLOR_CORRECTION_ACCURATE:
    case COLOR_CORRECTION_DESATURATED:
      return raw_to_raw(v, in, out);
    case COLOR_CORRECTION_RAW:
    case COLOR_CORRECTION_DEFAULT:
    case COLOR_CORRECTION_DENSITY:
      return raw_to_raw_raw(v, in, out);
    default:
      return (unsigned) -1;
    }
}
