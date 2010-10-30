/*
 * "$Id: curve-cache.h,v 1.2 2005/10/18 02:08:16 rlk Exp $"
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

/**
 * @file gutenprint/curve-cache.h
 * @brief Curve caching functions.
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifndef GUTENPRINT_CURVE_CACHE_H
#define GUTENPRINT_CURVE_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gutenprint/curve.h>

typedef struct
{
  stp_curve_t *curve;
  const double *d_cache;
  const unsigned short *s_cache;
  size_t count;
} stp_cached_curve_t;

extern void stp_curve_free_curve_cache(stp_cached_curve_t *cache);

extern void stp_curve_cache_curve_data(stp_cached_curve_t *cache);

extern stp_curve_t *stp_curve_cache_get_curve(stp_cached_curve_t *cache);

extern void stp_curve_cache_curve_invalidate(stp_cached_curve_t *cache);

extern void stp_curve_cache_set_curve(stp_cached_curve_t *cache,
				      stp_curve_t *curve);

extern void stp_curve_cache_set_curve_copy(stp_cached_curve_t *cache,
					   const stp_curve_t *curve);

extern size_t stp_curve_cache_get_count(stp_cached_curve_t *cache);

extern const unsigned short *stp_curve_cache_get_ushort_data(stp_cached_curve_t *cache);

extern const double *stp_curve_cache_get_double_data(stp_cached_curve_t *cache);

extern void stp_curve_cache_copy(stp_cached_curve_t *dest,
				 const stp_cached_curve_t *src);

#define CURVE_CACHE_FAST_USHORT(cache) ((cache)->s_cache)
#define CURVE_CACHE_FAST_DOUBLE(cache) ((cache)->d_cache)
#define CURVE_CACHE_FAST_COUNT(cache) ((cache)->count)

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_CURVE_CACHE_H */
