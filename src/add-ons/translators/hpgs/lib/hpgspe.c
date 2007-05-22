/***********************************************************************
 *                                                                     *
 * $Id: hpgspe.c 347 2006-09-21 09:36:01Z softadm $
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
 * The implementation of the HPGL reader.                              *
 *                                                                     *
 ***********************************************************************/

#include <hpgsreader.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

//#define HPGS_DEBUG_PE

/* 
  HPGL command PE (Polyline Encoded)
*/
static int base32_decode (hpgs_reader *reader, unsigned *value)
{
  int digit;
  int i=0;

  *value=0;

  while (i<7)
    {
      if (reader->last_byte == EOF) return -1;

      reader->last_byte &= 0x7f;

      if (reader->last_byte > 94)
	digit = reader->last_byte - 95;
      else
	digit = reader->last_byte - 63;

      if (digit >= 0 && digit <= 31) 
	{
	  *value |= digit << (5*i);

	  if (reader->last_byte > 94) return 0;
	  ++i;
	}
      else
        // ignore all escape characters, including space.
	if (reader->last_byte > 32) return -1;

      reader->last_byte = hpgs_getc(reader->in);
    }

  return -1;
}

static int base64_decode (hpgs_reader *reader, unsigned *value)
{
  int digit;
  int i=0;

  *value=0;

  while (i<6)
    {
      if (reader->last_byte == EOF) return -1;

      if (reader->last_byte > 190)
	digit = reader->last_byte - 191;
      else
	digit = reader->last_byte - 63;

      if (digit >= 0 && digit <= 63) 
	{
	  *value |= digit << (6*i);
	  
	  if (reader->last_byte > 190) return 0;
	  ++i;
	}
      else
        // ignore all escape characters, including space.
	if (reader->last_byte > 32) return -1;

      reader->last_byte = hpgs_getc(reader->in);
    }
  return -1;
}

static int int_decode (hpgs_reader *reader, int *value, int b32)
{
  unsigned uval;

  if (b32)
    { if (base32_decode(reader,&uval)) return -1; }
  else
    { if (base64_decode(reader,&uval)) return -1; }

  if (uval & 1)
    *value = - (uval >> 1);
  else
    *value = uval >> 1;

  return 0;
}


static int double_decode (hpgs_reader *reader, double *value,
			  double fract_fac, int b32)
{
  int int_val;

  if (int_decode(reader,&int_val,b32))
    return -1;

  *value = int_val * fract_fac;
  return 0;
}

static int coord_decode (hpgs_reader *reader, hpgs_point *p,
			 double fract_fac, int b32, int rel)
{
  if (double_decode (reader,&p->x,fract_fac,b32)) return -1;
  reader->last_byte = hpgs_getc(reader->in);
  if (double_decode (reader,&p->y,fract_fac,b32)) return -1;

  if (rel)
    {
      hpgs_matrix_scale (p,&reader->total_matrix,p);
      p->x+=reader->current_point.x;
      p->y+=reader->current_point.y;
    }
  else
    {
      hpgs_matrix_xform (p,&reader->total_matrix,p);
    }

  return 0;
}


int hpgs_reader_do_PE(hpgs_reader *reader)
{
  hpgs_point p0;
  hpgs_point p;

  double fract_fac = 1.0;
  int rel = 1;
  int b32 = 0;
  int rect = 0;
  reader->pen_down = 1;

#ifdef HPGS_DEBUG_PE
  hpgs_log("PE start ***************\n");
#endif

  while (1)
    {
      reader->last_byte = hpgs_getc(reader->in);

      if (reader->last_byte == EOF || reader->last_byte == ';')
	{
	  reader->eoc = 1;
	  reader->bytes_ignored = 0;
	  return 0;
	}

      switch (reader->last_byte & 0x7f)
	{
	case '7':
	  b32 = 1;
	  break;
	case '9':
	  rect = 1;
	  reader->pen_down = 0;
	  break;
	case ':':
	  {
	    int pen;
	    reader->last_byte = hpgs_getc(reader->in);
	    if (int_decode(reader,&pen,b32)) return -1;
	    if (hpgs_reader_do_setpen(reader,pen)) return -1;
	  }
	  break;
	case '>':
	  {
	    int fract_bits;
	    reader->last_byte = hpgs_getc(reader->in);
	    if (int_decode(reader,&fract_bits,b32)) return -1;

	    fract_fac = 1.0;
	    while (--fract_bits >= 0)
	      fract_fac *= 0.5;
	  }
	  break;
	case '<':
	  reader->pen_down = 0;
	  rect = 0;
	  break;
	case '=':
	  rel = 0;
	  break;
	default:
	  if (isspace(reader->last_byte)) break;

	  if (coord_decode(reader,&p,fract_fac,b32,rel))
            {
              // Well, some HPGL drivers are so broken, that they
              // can't write decent PE vommands.
              // So let's  hand over control to the recovery code in
              // hpgs_read().
              //
              if (reader->last_byte != EOF)
                {
                  reader->eoc = 1;
                  reader->bytes_ignored = 1;
                  return 0;
                }

              return -1;
            }

#ifdef HPGS_DEBUG_PE
	  hpgs_log("p,down,rect = (%lg,%lg),%d,%d.\n",p.x,p.y,reader->pen_down,rect);
#endif
	  switch (rect)
	    {
	    case 1:
	      if (hpgs_reader_checkpath(reader)) return -1;
	      p0 = p;
	      if (hpgs_reader_moveto(reader,&p0)) return -1;
	      rect = 2;
	      break;
	    case 2:
	      {
		hpgs_point pp;
		pp.x=p.x;
		pp.y=p0.y;
		if (hpgs_reader_lineto(reader,&pp)) return -1;
		if (hpgs_reader_lineto(reader,&p))
		  return -1;
		pp.x=p0.x;
		pp.y=p.y;
		if (hpgs_reader_lineto(reader,&pp)) return -1;
		if (hpgs_reader_lineto(reader,&p0)) return -1;
		// only do a fill, if we are not in polygon mode.
		if (!reader->polygon_mode &&
		    hpgs_reader_fill(reader,1)) return -1;
	      }
	      reader->pen_down = 1;
	      rect = 1;
	      break;
	    default:
	      if (reader->pen_down)
		{
		  if (hpgs_reader_lineto(reader,&p)) return -1;
		}
	      else
		{
		  if (hpgs_reader_checkpath(reader)) return -1;
		  if (hpgs_reader_moveto(reader,&p)) return -1;
		}

	      reader->pen_down = 1;
	    }
	  rel = 1;
	}
    }
}
