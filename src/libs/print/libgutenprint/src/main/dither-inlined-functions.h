/*
 * "$Id: dither-inlined-functions.h,v 1.6 2004/09/17 18:38:18 rleigh Exp $"
 *
 *   Performance-critical functions that should be inlined, based on
 *   measurements.
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

#ifndef GUTENPRINT_INTERNAL_DITHER_INLINED_FUNCTIONS_H
#define GUTENPRINT_INTERNAL_DITHER_INLINED_FUNCTIONS_H

/*
 * Inlining has yielded significant (measured) speedup, even with the
 * more complicated dither function. --rlk 20011219
 */

static inline unsigned
ditherpoint(const stpi_dither_t *d, stp_dither_matrix_impl_t *mat, int x)
{
  if (mat->fast_mask)
    return mat->matrix[(mat->last_y_mod +
			((x + mat->x_offset) & mat->fast_mask))];
  /*
   * This rather bizarre code is an attempt to avoid having to compute a lot
   * of modulus and multiplication operations, which are typically slow.
   */

  if (x == mat->last_x + 1)
    {
      mat->last_x_mod++;
      mat->index++;
      if (mat->last_x_mod >= mat->x_size)
	{
	  mat->last_x_mod -= mat->x_size;
	  mat->index -= mat->x_size;
	}
    }
  else if (x == mat->last_x - 1)
    {
      mat->last_x_mod--;
      mat->index--;
      if (mat->last_x_mod < 0)
	{
	  mat->last_x_mod += mat->x_size;
	  mat->index += mat->x_size;
	}
    }
  else if (x != mat->last_x)
    {
      mat->last_x_mod = (x + mat->x_offset) % mat->x_size;
      mat->index = mat->last_x_mod + mat->last_y_mod;
    }
  mat->last_x = x;
  return mat->matrix[mat->index];
}

static inline void
set_row_ends(stpi_dither_channel_t *dc, int x)
{
  if (dc->row_ends[0] == -1)
    dc->row_ends[0] = x;
  dc->row_ends[1] = x;
}

#endif /* GUTENPRINT_INTERNAL_DITHER_INLINED_FUNCTIONS_H */
