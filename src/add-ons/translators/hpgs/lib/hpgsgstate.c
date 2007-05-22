/***********************************************************************
 *                                                                     *
 * $Id: hpgsgstate.c 270 2006-01-29 21:12:23Z softadm $
 *                                                                     *
 * hpgs - HPGl Script, a hpgl/2 interpreter, which uses a Postscript   *
 *        API for rendering a scene and thus renders to a variety of   *
 *        devices and fileformats.                                     *
 *                                                                     *
 * (C) 2004-2006 ev-i Informationstechnologie GmbH  http://www.ev-i.at *
 *                                                                     *
 * Author: Wolfgang Glas                                               *
 *                                                                     *
 *  hpgs is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU Lesser General Public          *
 * License as published by the Free Software Foundation; either        *
 * version 2.1 of the License, or (at your option) any later version.  *
 *                                                                     *
 * hpgs is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   *
 * Lesser General Public License for more details.                     *
 *                                                                     *
 * You should have received a copy of the GNU Lesser General Public    *
 * License along with this library; if not, write to the               *
 * Free Software  Foundation, Inc., 59 Temple Place, Suite 330,        *
 * Boston, MA  02111-1307  USA                                         *
 *                                                                     *
 ***********************************************************************
 *                                                                     *
 * The implementation of the public API for the gstate.                *
 *                                                                     *
 ***********************************************************************/

#include<hpgspaint.h>
#include<assert.h>
#include<string.h>

/*! Creates a new gstate on the heap.
    Use \c hpgs_gstate_destroy in order to destroy the returned
    gstate pointer.

    A null pointer is returned, if the system is out of memory.
*/
hpgs_gstate *hpgs_new_gstate()
{
  hpgs_gstate *ret = (hpgs_gstate *)malloc(sizeof(hpgs_gstate));

  if (ret)
    {
      ret->line_cap  = hpgs_cap_butt;
      ret->line_join = hpgs_join_miter;
      ret->color.r = 0.0;
      ret->color.g = 0.0;
      ret->color.b = 0.0;
      ret->miterlimit = 5.0;
      ret->linewidth   = 1.0;
      ret->n_dashes    = 0;
      ret->dash_lengths = 0;
      ret->dash_offset  = 0.0;
      // ROP3 function: destination = source or pattern.
      ret->rop3 = 252;
      ret->src_transparency = HPGS_TRUE;
      ret->pattern_transparency = HPGS_TRUE;

      ret->pattern_color.r = 0.0;
      ret->pattern_color.g = 0.0;
      ret->pattern_color.b = 0.0;
    }
  return ret;
}

/*! Destroys a gstate created using \c hpgs_new_gstate.
*/
void hpgs_gstate_destroy(hpgs_gstate *gstate)
{
  if (!gstate) return;
  if (gstate->dash_lengths) free(gstate->dash_lengths);
  free (gstate);
}

/*! Sets the dashes of \c gstate. The passed array and offset
    folow th esemantics of PostScipt's \c setdash command.

    Return value:
    \li 0 Success.
    \li -1 The system is out of memory.
*/
int hpgs_gstate_setdash(hpgs_gstate *gstate,
			const float *dash_lengths,
			unsigned n_dashes, double offset)
{
  if (gstate->dash_lengths) free(gstate->dash_lengths);

  gstate->n_dashes = n_dashes;
  gstate->dash_offset = offset;

  if (gstate->n_dashes)
    {
      gstate->dash_lengths = (float *)malloc(sizeof(float)*gstate->n_dashes);
      if (!gstate->dash_lengths)
	{
	  gstate->n_dashes = 0;
	  return -1;
	}

      memmove(gstate->dash_lengths,dash_lengths,sizeof(float)*gstate->n_dashes);
    }
  else
    gstate->dash_lengths = 0;

  return 0;
}
