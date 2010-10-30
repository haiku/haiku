/*
 * "$Id: dither-impl.h,v 1.32 2008/02/18 14:20:17 rlk Exp $"
 *
 *   Internal implementation of dither algorithms
 *
 *   Copyright 1997-2003 Michael Sweet (mike@easysw.com) and
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

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifndef GUTENPRINT_INTERNAL_DITHER_IMPL_H
#define GUTENPRINT_INTERNAL_DITHER_IMPL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>

#ifdef __GNUC__
#define inline __inline__
#endif

#define D_FLOYD_HYBRID 0
#define D_ADAPTIVE_BASE 4
#define D_ADAPTIVE_HYBRID (D_ADAPTIVE_BASE | D_FLOYD_HYBRID)
#define D_ORDERED_BASE 8
#define D_ORDERED (D_ORDERED_BASE)
#define D_FAST_BASE 16
#define D_FAST (D_FAST_BASE)
#define D_VERY_FAST (D_FAST_BASE + 1)
#define D_EVENTONE 32
#define D_UNITONE 64
#define D_EVENBETTER 128
#define D_HYBRID_EVENTONE (D_ORDERED_BASE | D_EVENTONE)
#define D_HYBRID_UNITONE (D_ORDERED_BASE | D_UNITONE)
#define D_HYBRID_EVENBETTER (D_ORDERED_BASE | D_EVENBETTER)
#define D_PREDITHERED 256
#define D_ORDERED_NEW 512
#define D_ORDERED_SEGMENTED 1024
#define D_ORDERED_SEGMENTED_NEW (D_ORDERED_SEGMENTED | D_ORDERED_NEW)
#define D_INVALID -2

#define DITHER_FAST_STEPS (6)

typedef struct
{
  const char *name;
  const char *text;
  int id;
} stpi_dither_algorithm_t;

#define ERROR_ROWS 2

#define MAX_SPREAD 32

typedef void stpi_ditherfunc_t(stp_vars_t *, int, const unsigned short *, int,
			       int, const unsigned char *);

/*
 * An end of a dither segment, describing one ink
 */

typedef struct ink_defn
{
  unsigned range;
  unsigned value;
  unsigned bits;
} stpi_ink_defn_t;

/*
 * A segment of the entire 0-65535 intensity range.
 */

typedef struct dither_segment
{
  stpi_ink_defn_t *lower;
  stpi_ink_defn_t *upper;
  unsigned range_span;
  unsigned value_span;
  int is_same_ink;
  int is_equal;
} stpi_dither_segment_t;

typedef struct dither_channel
{
  unsigned randomizer;		/* With Floyd-Steinberg dithering, control */
				/* how much randomness is applied to the */
				/* threshold values (0-65535). */
  unsigned bit_max;
  unsigned signif_bits;
  unsigned density;
  double darkness;		/* Relative darkness of this ink */

  int v;
  int o;
  int b;
  int very_fast;

  stpi_ink_defn_t *ink_list;

  int nlevels;
  stpi_dither_segment_t *ranges;

  int error_rows;
  int **errs;

  stp_dither_matrix_impl_t pick;
  stp_dither_matrix_impl_t dithermat;
  int row_ends[2];
  unsigned char *ptr;
  void *aux_data;		/* aux_freefunc for dither should free this */
} stpi_dither_channel_t;

typedef struct dither
{
  int src_width;		/* Input width */
  int dst_width;		/* Output width */

  int spread;			/* With Floyd-Steinberg, how widely the */
  int spread_mask;		/* error is distributed.  This should be */
				/* between 12 (very broad distribution) and */
				/* 19 (very narrow) */

  int stpi_dither_type;

  int adaptive_limit;

  int x_aspect;			/* Aspect ratio numerator */
  int y_aspect;			/* Aspect ratio denominator */


  int *offset0_table;
  int *offset1_table;

  int d_cutoff;

  int last_line_was_empty;
  int ptr_offset;
  int error_rows;

  int finalized;		/* When dither is first called, calculate
				 * some things */

  stp_dither_matrix_impl_t dither_matrix;
  stpi_dither_channel_t *channel;
  unsigned channel_count;
  unsigned total_channel_count;
  unsigned *channel_index;
  unsigned *subchannel_count;

  stpi_ditherfunc_t *ditherfunc;
  void *aux_data;
  void (*aux_freefunc)(struct dither *);
} stpi_dither_t;

#define CHANNEL(d, c) ((d)->channel[(c)])
#define CHANNEL_COUNT(d) ((d)->total_channel_count)

#define USMIN(a, b) ((a) < (b) ? (a) : (b))


extern stpi_ditherfunc_t stpi_dither_predithered;
extern stpi_ditherfunc_t stpi_dither_very_fast;
extern stpi_ditherfunc_t stpi_dither_ordered;
extern stpi_ditherfunc_t stpi_dither_ed;
extern stpi_ditherfunc_t stpi_dither_et;
extern stpi_ditherfunc_t stpi_dither_ut;

extern void stpi_dither_reverse_row_ends(stpi_dither_t *d);
extern int stpi_dither_translate_channel(stp_vars_t *v, unsigned channel,
					 unsigned subchannel);
extern void stpi_dither_channel_destroy(stpi_dither_channel_t *channel);
extern void stpi_dither_finalize(stp_vars_t *v);
extern int *stpi_dither_get_errline(stpi_dither_t *d, int row, int color);


#define ADVANCE_UNIDIRECTIONAL(d, bit, input, width, xerror, xstep, xmod) \
do									  \
{									  \
  bit >>= 1;								  \
  if (bit == 0)								  \
    {									  \
      d->ptr_offset++;							  \
      bit = 128;							  \
    }									  \
  input += xstep;							  \
  if (xmod)								  \
    {									  \
      xerror += xmod;							  \
      if (xerror >= d->dst_width)					  \
	{								  \
	  xerror -= d->dst_width;					  \
	  input += (width);						  \
	}								  \
    }									  \
} while (0)

#define ADVANCE_REVERSE(d, bit, input, width, xerror, xstep, xmod)	\
do									\
{									\
  if (bit == 128)							\
    {									\
      d->ptr_offset--;							\
      bit = 1;								\
    }									\
  else									\
    bit <<= 1;								\
  input -= xstep;							\
  if (xmod)								\
    {									\
      xerror -= xmod;							\
      if (xerror < 0)							\
	{								\
	  xerror += d->dst_width;					\
	  input -= (width);						\
	}								\
    }									\
} while (0)

#define ADVANCE_BIDIRECTIONAL(d,bit,in,dir,width,xer,xstep,xmod,err,S)	\
do									\
{									\
  int ii;								\
  int jj;								\
  for (ii = 0; ii < width; ii++)					\
    for (jj = 0; jj < S; jj++)						\
      err[ii][jj] += dir;						\
  if (dir == 1)								\
    ADVANCE_UNIDIRECTIONAL(d, bit, in, width, xer, xstep, xmod);	\
  else									\
    ADVANCE_REVERSE(d, bit, in, width, xer, xstep, xmod);		\
} while (0)

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_INTERNAL_DITHER_IMPL_H */
/*
 * End of "$Id: dither-impl.h,v 1.32 2008/02/18 14:20:17 rlk Exp $".
 */
