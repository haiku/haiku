/*
 * "$Id: dither-ordered.c,v 1.31 2010/08/04 00:33:56 rlk Exp $"
 *
 *   Ordered dither algorithm
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include "dither-impl.h"
#include "dither-inlined-functions.h"


typedef struct {
  size_t channels;
  double *drops;
  unsigned short *lut;
} stpi_new_ordered_t;

typedef struct {
  unsigned short shift;
  unsigned short mask;
  unsigned short x_mask;
  stpi_new_ordered_t *ord_new;
} stpi_ordered_t;

static int
compare_channels(const stpi_dither_channel_t *dc1,
		 const stpi_dither_channel_t *dc2)
{
  int i;
  if (dc1->nlevels != dc2->nlevels)
    return 0;
  for (i = 0; i < dc1->nlevels; i++)
    if (dc1->ranges[i].upper->value != dc2->ranges[i].upper->value)
      return 0;
  return 1;
}

const static double dp_fraction = 0.5;

static void
init_dither_channel_new(stpi_dither_channel_t *dc, stp_vars_t *v)
{
  int i, j, k;
  double bp = 0;
  double lbp = 0;
  double lower_bottom = 0;
  double lower_middle = 0;
  double lower_top = 0;
  double upper_bottom = 0;
  double upper_middle = 0;
  double upper_top = 0;
  
  double *breakpoints;
  double *val;
  unsigned short *data;
  stpi_new_ordered_t *ord = stp_malloc(sizeof(stpi_new_ordered_t));
  ((stpi_ordered_t *) (dc->aux_data))->ord_new = ord;
  ord->channels = dc->nlevels - 1;
  ord->drops = stp_malloc(sizeof(double) * (ord->channels + 1));
  breakpoints = stp_malloc(sizeof(double) * (ord->channels + 1));
  val = stp_malloc(sizeof(double) * ord->channels);
  data = stp_malloc(sizeof(unsigned short) * 65536 * ord->channels);
  ord->lut = data;
  for (j = 0; j < ord->channels; j++)
    {
      stpi_dither_segment_t *dd = &(dc->ranges[j]);
      ord->drops[j] = (double) dd->upper->value / 65535.0;
    }
  ord->drops[ord->channels] = 1;
  for (j = 0; j < ord->channels; j++)
    {
      if (j == 0)
	breakpoints[j] = 65535 * ord->drops[j] * dp_fraction;
      else
	breakpoints[j] = 65535 * ((ord->drops[j] * dp_fraction) +
				  (ord->drops[j - 1] * (1.0 - dp_fraction)));
      stp_dprintf(STP_DBG_INK, v, "        size %.3f bp %5.0f\n",
		  ord->drops[j], breakpoints[j]);
    }
  breakpoints[ord->channels] = 65535;
  j = 0;
  for (i = 0; i <= ord->channels; i++)
    {
      lbp = bp;
      bp = breakpoints[i];
      lower_bottom = upper_middle;
      upper_bottom = 0;
      lower_middle = upper_top;
      lower_top = 0;
      if (i == ord->channels)
	upper_top = 0;
      else
	upper_top = 65535 * dp_fraction;
      if (i > 0)
	upper_middle = 65535 - upper_top;
      while (j <= bp)
	{
	  double range_point = (j - lbp) / (bp - lbp);
	  double uv, mv, lv;
	  int total_ink = 0;
	  for (k = 0; k < ord->channels; k++)
	    val[k] = 0;
	  uv = lower_top + (upper_top - lower_top) * range_point;
	  mv = lower_middle + (upper_middle - lower_middle) * range_point;
	  lv = lower_bottom + (upper_bottom - lower_bottom) * range_point;

	  if (i < ord->channels)
	    val[i] = (unsigned short) uv;
	  if (i > 0)
	    val[i - 1] = (unsigned short) mv;
	  if (i > 1)
	    val[i - 2] = (unsigned short) lv;
	  for (k = ord->channels - 1; k >= 0; k--)
	    {
	      total_ink += val[k];
	      if (total_ink > 65535)
		total_ink = 65535;
	      data[k] = total_ink;
	    }
	  if ((stp_get_debug_level() & STP_DBG_INK) && (j % 257 == 0))
	    {
	      stp_dprintf(STP_DBG_INK, v, "    %5d:", j);
	      for (k = 0; k < ord->channels; k++)
		stp_dprintf(STP_DBG_INK, v, " %9.3f", val[k]);
	      stp_dprintf(STP_DBG_INK, v, "  ");
	      for (k = 0; k < ord->channels; k++)
		stp_dprintf(STP_DBG_INK, v, " %9.3f", breakpoints[k]);
	      stp_dprintf(STP_DBG_INK, v, "  ");
	      for (k = 0; k < ord->channels; k++)
		stp_dprintf(STP_DBG_INK, v, " %5d", data[k]);
	      stp_dprintf(STP_DBG_INK, v, "\n");
	    }
	  data += ord->channels;
	  j++;
	}
    }
  stp_free(breakpoints);
  stp_free(val);
}

static inline void
print_color_ordered_new(const stpi_dither_t *d, stpi_dither_channel_t *dc,
			int val, int x, int y, unsigned char bit, int length)
{
  int i;
  int j;
  unsigned bits;
  int levels = dc->nlevels - 1;
  unsigned dpoint = ditherpoint(d, &(dc->dithermat), x);
  const stpi_ordered_t *o = (const stpi_ordered_t *) dc->aux_data;
  const stpi_new_ordered_t *ord = (const stpi_new_ordered_t *) o->ord_new;
  unsigned short swhere = (unsigned short) val;
  unsigned short *where = ord ? ord->lut + (val * levels) : &swhere;
  /*
   * Look for the appropriate range into which the input value falls.
   */
  for (i = levels - 1; i >= 0; i--)
    {
      if (dpoint < where[i])
	{
	  stpi_dither_segment_t *dd = &(dc->ranges[i]);
	  bits = dd->upper->bits;
	  if (bits)
	    {
	      unsigned char *tptr = dc->ptr + d->ptr_offset;

	      /*
	       * Lay down all of the bits in the pixel.
	       */
	      set_row_ends(dc, x);
	      for (j = 1; j <= bits; j += j, tptr += length)
		{
		  if (j & bits)
		    tptr[0] |= bit;
		}

	    }
	  return;
	}
    }
}

static inline void
print_color_ordered(const stpi_dither_t *d, stpi_dither_channel_t *dc, int val,
		    int x, int y, unsigned char bit, int length)
{
  int i;
  int j;
  unsigned bits;
  int levels = dc->nlevels - 1;

  /*
   * Look for the appropriate range into which the input value falls.
   */
  for (i = levels; i >= 0; i--)
    {
      stpi_dither_segment_t *dd = &(dc->ranges[i]);

      if (val > dd->lower->value)
	{
	  /*
	   * Where are we within the range.
	   */

	  unsigned rangepoint = val - dd->lower->value;
	  if (dd->value_span < 65535)
	    rangepoint = rangepoint * 65535 / dd->value_span;

	  if (rangepoint >= ditherpoint(d, &(dc->dithermat), x))
	    bits = dd->upper->bits;
	  else
	    bits = dd->lower->bits;

	  if (bits)
	    {
	      unsigned char *tptr = dc->ptr + d->ptr_offset;

	      /*
	       * Lay down all of the bits in the pixel.
	       */
	      set_row_ends(dc, x);
	      for (j = 1; j <= bits; j += j, tptr += length)
		{
		  if (j & bits)
		    tptr[0] |= bit;
		}

	    }
	  return;
	}
    }
}

static void
free_dither_ordered(stpi_dither_t *d)
{
  int i;
  stpi_dither_channel_t *dc0 = &CHANNEL(d, 0);
  stpi_ordered_t *o0 = dc0->aux_data;
  stpi_new_ordered_t *no0 = NULL;
  if (o0)
    no0 = o0->ord_new;
  for (i = CHANNEL_COUNT(d) - 1; i >= 0 ; i--)
    {
      stpi_dither_channel_t *dc = &CHANNEL(d, i);
      if (dc->aux_data)
	{
	  stpi_ordered_t *ord = (stpi_ordered_t *) dc->aux_data;
	  if (ord->ord_new && (i == 0 || ord->ord_new != no0))
	    {
	      stpi_new_ordered_t *no = (stpi_new_ordered_t *) ord->ord_new;
	      if (no->drops)
		stp_free(no->drops);
	      if (no->lut)
		stp_free(no->lut);
	      stp_free(no);
	    }
	  stp_free(dc->aux_data);
	  dc->aux_data = NULL;
	}
    }
  stp_free(d->aux_data);
}

static void
init_dither_ordered(stpi_dither_t *d, stp_vars_t *v)
{
  int i;
  d->aux_data = stp_malloc(1);
  d->aux_freefunc = &free_dither_ordered;
  stp_dprintf(STP_DBG_INK, v, "init_dither_ordered\n");
  for (i = 0; i < CHANNEL_COUNT(d); i++)
    {
      stpi_dither_channel_t *dc = &CHANNEL(d, i);
      stpi_ordered_t *s;
      dc->aux_data = stp_malloc(sizeof(stpi_ordered_t));
      s = (stpi_ordered_t *) dc->aux_data;
      s->ord_new = NULL;
      if (d->stpi_dither_type & D_ORDERED_SEGMENTED)
	{
	  s->shift = 16 - dc->signif_bits;
	  s->mask = ((1 << dc->signif_bits) - 1) << s->shift;
	  s->x_mask = ~(s->mask);
	  stp_dprintf(STP_DBG_INK, v, "   channel %d: shift %d mask 0x%x x_mask 0x%x\n",
		      i, s->shift, s->mask, s->x_mask);
	}
      if (d->stpi_dither_type & D_ORDERED_NEW)
	{
	  if (dc->nlevels < 2)
	    stp_dprintf(STP_DBG_INK, v, "    channel %d ignored\n", i);
	  else if (i == 0 || !compare_channels(&CHANNEL(d, 0), dc))
	    {
	      stp_dprintf(STP_DBG_INK, v, "    channel %d\n", i);
	      init_dither_channel_new(dc, v);
	    }
	  else
	    {
	      stp_dprintf(STP_DBG_INK, v, "    channel %d duplicated from channel 0\n", i);
	      s->ord_new = ((stpi_ordered_t *) (CHANNEL(d, 0).aux_data))->ord_new;
	    }
	}
    }
}

void
stpi_dither_ordered(stp_vars_t *v,
		    int row,
		    const unsigned short *raw,
		    int duplicate_line,
		    int zero_mask,
		    const unsigned char *mask)
{
  stpi_dither_t *d = (stpi_dither_t *) stp_get_component_data(v, "Dither");
  int		x,
		length;
  unsigned char	bit;
  int i;
  int one_bit_only = 1;
  int one_level_only = 1;

  int xerror, xstep, xmod;

  if ((zero_mask & ((1 << CHANNEL_COUNT(d)) - 1)) ==
      ((1 << CHANNEL_COUNT(d)) - 1))
    return;

  length = (d->dst_width + 7) / 8;

  bit = 128;
  xstep  = CHANNEL_COUNT(d) * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = 0;

  for (i = 0; i < CHANNEL_COUNT(d); i++)
    {
      stpi_dither_channel_t *dc = &(CHANNEL(d, i));
      if (dc->nlevels != 1)
	one_level_only = 0;
      if (dc->nlevels != 1 || dc->ranges[0].upper->bits != 1)
	one_bit_only = 0;
    }
  if (! one_bit_only && ! d->aux_data &&
      (d->stpi_dither_type & (D_ORDERED_SEGMENTED | D_ORDERED_NEW)))
    init_dither_ordered(d, v);

  if (one_bit_only)
    {
      for (x = 0; x < d->dst_width; x ++)
	{
	  if (!mask || (*(mask + d->ptr_offset) & bit))
	    {
	      for (i = 0; i < CHANNEL_COUNT(d); i++)
		{
		  if (raw[i] &&
		      raw[i] >= ditherpoint(d, &(CHANNEL(d, i).dithermat), x))
		    {
		      set_row_ends(&(CHANNEL(d, i)), x);
		      CHANNEL(d, i).ptr[d->ptr_offset] |= bit;
		    }
		}
	    }
	  ADVANCE_UNIDIRECTIONAL(d, bit, raw, CHANNEL_COUNT(d),
				 xerror, xstep, xmod);
	}
    }
  else if (d->stpi_dither_type & D_ORDERED_SEGMENTED)
    {
      for (x = 0; x < d->dst_width; x ++)
	{
	  if (!mask || (*(mask + d->ptr_offset) & bit))
	    {
	      for (i = 0; i < CHANNEL_COUNT(d); i++)
		{
		  stpi_dither_channel_t *dc = &CHANNEL(d, i);
		  stpi_ordered_t *s = (stpi_ordered_t *) dc->aux_data;
		  unsigned short bits = raw[i] >> s->shift;
		  unsigned short val = raw[i] << dc->signif_bits;
		  val |= val >> s->shift;

		  if (bits)
		    {
		      if (val &&
			  val >= ditherpoint(d, &(CHANNEL(d, i).dithermat), x))
			{
			  int j;
			  unsigned char *tptr = dc->ptr + d->ptr_offset;
			  set_row_ends(dc, x);
			  for (j = 1; j <= bits; j += j, tptr += length)
			    {
			      if (j & bits)
				tptr[0] |= bit;
			    }
			}
		    }
		  else if (CHANNEL(d, i).ptr && val)
		    {
		      if (d->stpi_dither_type & D_ORDERED_NEW)
			print_color_ordered_new(d, &(CHANNEL(d, i)), val, x,
						row, bit, length);
		      else
			print_color_ordered(d, &(CHANNEL(d, i)), val, x,
					    row, bit, length);
		    }
		}
	    }
	  ADVANCE_UNIDIRECTIONAL(d, bit, raw, CHANNEL_COUNT(d),
				 xerror, xstep, xmod);
	}
    }
  else if (one_level_only || !(d->stpi_dither_type == D_ORDERED_NEW))
    {
      for (x = 0; x != d->dst_width; x ++)
	{
	  if (!mask || (*(mask + d->ptr_offset) & bit))
	    {
	      for (i = 0; i < CHANNEL_COUNT(d); i++)
		{
		  if (CHANNEL(d, i).ptr && raw[i])
		    print_color_ordered(d, &(CHANNEL(d, i)), raw[i], x, row,
					bit, length);
		}
	    }
	  ADVANCE_UNIDIRECTIONAL(d, bit, raw, CHANNEL_COUNT(d), xerror,
				 xstep, xmod);
	}
    }
  else
    {
      for (x = 0; x != d->dst_width; x ++)
	{
	  if (!mask || (*(mask + d->ptr_offset) & bit))
	    {
	      for (i = 0; i < CHANNEL_COUNT(d); i++)
		{
		  if (CHANNEL(d, i).ptr && raw[i])
		    print_color_ordered_new(d, &(CHANNEL(d, i)), raw[i], x,
					    row, bit, length);
		}
	    }
	  ADVANCE_UNIDIRECTIONAL(d, bit, raw, CHANNEL_COUNT(d), xerror,
				 xstep, xmod);
	}
    }
}
