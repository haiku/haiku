/***********************************************************************
 *                                                                     *
 * $Id: hpgspen.c 298 2006-03-05 18:18:03Z softadm $
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

/* 
  Internal 
*/
static int hpgs_reader_set_number_of_pens(hpgs_reader *reader, int npens)
{
  double *pw =(double *)realloc(reader->pen_widths,npens*sizeof(double));

  if (!pw)
    return hpgs_set_error(hpgs_i18n("Out of memory growing pen width array."));
  
  reader->pen_widths = pw;

  hpgs_color *pc = (hpgs_color *)realloc(reader->pen_colors,npens*sizeof(hpgs_color));

  if (!pc)
    return hpgs_set_error(hpgs_i18n("Out of memory growing pen color array."));
  
  reader->pen_colors = pc;

  for (;reader->npens<npens;++reader->npens)
    {
      reader->pen_widths[reader->npens] = 1.0;
      reader->pen_colors[reader->npens].r = 0.0;
      reader->pen_colors[reader->npens].g = 0.0;
      reader->pen_colors[reader->npens].b = 0.0;
    }

  return 0;
}

/*
  Do the pen select.
*/
int hpgs_reader_do_setpen(hpgs_reader *reader, int pen)
{
  double width;

  if (hpgs_reader_checkpath(reader)) return -1;

  if (pen < 0)
    return hpgs_set_error(hpgs_i18n("Illegal pen numer %d."),pen);

  if (pen >= reader->npens)
    {
      if (pen < 256)
        {
          if (hpgs_reader_set_number_of_pens(reader,pen+1))
            return -1;
        }
      else
        {
          if (reader->verbosity)
            hpgs_log(hpgs_i18n("Illegal pen number %d replaced by %d.\n"),
                     pen, pen % reader->npens);
          pen = pen %  reader->npens;
        }
    }

  reader->current_pen = pen;

  width = reader->pen_widths[pen];

  if (reader->pen_width_relative)
    width *= hypot(reader->P2.x-reader->P1.x,
		   reader->P2.y-reader->P1.y ) * 0.001 * HP_TO_PT;
  else
    width *= HP_TO_PT / reader->world_scale;

  width *= reader->page_scale;

  if (hpgs_setlinewidth(reader->device,width*reader->lw_factor))
    return -1;

  return hpgs_setrgbcolor(reader->device,
			  &reader->pen_colors[pen]);
}

/* 
  HPGL command NP (Number of Pens)
*/
int hpgs_reader_do_NP (hpgs_reader *reader)
{
  int npens=8;

  if (!reader->eoc &&
      hpgs_reader_read_int(reader,&npens)) return -1;

  if (npens <= reader->npens) return 0;

  return hpgs_reader_set_number_of_pens(reader,npens);
}

/* 
  HPGL command SP (Set Pen)
*/
int hpgs_reader_do_SP (hpgs_reader *reader)
{
  int pen=0;

  if (!reader->eoc &&
      hpgs_reader_read_int(reader,&pen)) return -1;

  return hpgs_reader_do_setpen(reader,pen);
}

/* 
  HPGL command PC (Pen Color)
*/
int hpgs_reader_do_PC (hpgs_reader *reader)
{
  int pen=-1;
  double r=-1.0e20,g=-1.0e20,b=-1.0e20;

  if (!reader->eoc && hpgs_reader_read_int(reader,&pen)) return -1;
  if (!reader->eoc && hpgs_reader_read_double(reader,&r)) return -1;
  if (!reader->eoc && hpgs_reader_read_double(reader,&g)) return -1;
  if (!reader->eoc && hpgs_reader_read_double(reader,&b)) return -1;

  if (pen >= reader->npens)
    {
      if (pen < 256)
        {
          if (hpgs_reader_set_number_of_pens(reader,pen+1))
            return -1;
        }
      else
        {
          if (reader->verbosity)
            hpgs_log(hpgs_i18n("PC: Illegal pen number %d.\n"),pen);

          return 0;
        }
    }

  if (pen < 0)
    {
      hpgs_reader_set_std_pen_colors(reader,0,reader->npens);
      pen = reader->current_pen;
    }
  else
    {
      if (r==-1.0e20 || g==-1.0e20 || b==-1.0e20)
        {
          hpgs_reader_set_std_pen_colors(reader,pen,1);
        }
      else
        {
          reader->pen_colors[pen].r =
            (r - reader->min_color.r) / (reader->max_color.r - reader->min_color.r);
          if (reader->pen_colors[pen].r < 0.0) reader->pen_colors[pen].r = 0.0;
          if (reader->pen_colors[pen].r > 1.0) reader->pen_colors[pen].r = 1.0;

          reader->pen_colors[pen].g =
            (g - reader->min_color.g) / (reader->max_color.g - reader->min_color.g);
          if (reader->pen_colors[pen].g < 0.0) reader->pen_colors[pen].g = 0.0;
          if (reader->pen_colors[pen].g > 1.0) reader->pen_colors[pen].g = 1.0;

          reader->pen_colors[pen].b =
            (b - reader->min_color.b) / (reader->max_color.b - reader->min_color.b);
          if (reader->pen_colors[pen].b < 0.0) reader->pen_colors[pen].b = 0.0;
          if (reader->pen_colors[pen].b > 1.0) reader->pen_colors[pen].b = 1.0;
        }
    }

  if (pen == reader->current_pen)
    if (hpgs_setrgbcolor(reader->device,
			 &reader->pen_colors[pen]))
      return -1;

  return 0;
}

/* 
  HPGL command CR (Color Range)
*/
int hpgs_reader_do_CR (hpgs_reader *reader)
{
  reader->min_color.r = 0.0;
  reader->min_color.g = 0.0;
  reader->min_color.b = 0.0;

  reader->max_color.r = 255.0;
  reader->max_color.g = 255.0;
  reader->max_color.b = 255.0;

  if (!reader->eoc)
    {
      if (hpgs_reader_read_double(reader,&reader->min_color.r)) return -1;
      if (reader->eoc) return -1;
      if (hpgs_reader_read_double(reader,&reader->max_color.r)) return -1;
      if (reader->eoc) return -1;
      if (hpgs_reader_read_double(reader,&reader->min_color.g)) return -1;
      if (reader->eoc) return -1;
      if (hpgs_reader_read_double(reader,&reader->max_color.g)) return -1;
      if (reader->eoc) return -1;
      if (hpgs_reader_read_double(reader,&reader->min_color.b)) return -1;
      if (reader->eoc) return -1;
      if (hpgs_reader_read_double(reader,&reader->max_color.b)) return -1;
    }

  return 0;
}

/*
  HPGL command LA (Line Attributes)
*/
int hpgs_reader_do_LA(hpgs_reader *reader)
{
  int    kind;
  int    ivalue;
  double rvalue;

  static hpgs_line_cap caps[5] =
    { hpgs_cap_butt,
      hpgs_cap_butt,
      hpgs_cap_square,
      hpgs_cap_round,
      hpgs_cap_round    };

  static hpgs_line_join joins[7] =
    { hpgs_join_miter,
      hpgs_join_miter,
      hpgs_join_miter,
      hpgs_join_miter,
      hpgs_join_round,
      hpgs_join_bevel,
      hpgs_join_miter };

  while (!reader->eoc)
    {
      if (hpgs_reader_read_int(reader,&kind)) return -1;
      if (reader->eoc) return -1;
      
      switch (kind)
	{
	case 3:
	  if (hpgs_reader_read_double(reader,&rvalue)) return -1;
	  if (hpgs_setmiterlimit(reader->device,rvalue)) return -1;
	  break;
	case 1:
	  if (hpgs_reader_read_int(reader,&ivalue)) return -1;
	  if (ivalue >= 0 && ivalue < 5 &&
	      hpgs_setlinecap(reader->device,caps[ivalue])) return -1;
	  break;
	case 2:
	  if (hpgs_reader_read_int(reader,&ivalue)) return -1;
	  if (ivalue >= 0 && ivalue < 7 &&
	      hpgs_setlinejoin(reader->device,joins[ivalue])) return -1;
	  break;
	default:
	  return -1;
	}
    }
  return 0;
}

/*
  HPGL command LT (Line Type)
*/
int hpgs_reader_do_LT(hpgs_reader *reader)
{
  float dashes[20];
  int i,ndash;
  int linetype=0;
  double patlen = 4.0;
  int    mode = 0;

  if (hpgs_reader_checkpath(reader)) return -1;

  if (!reader->eoc &&
      hpgs_reader_read_int(reader,&linetype)) return -1;

  if (!reader->eoc &&
      hpgs_reader_read_double(reader,&patlen)) return -1;

  if (!reader->eoc &&
      hpgs_reader_read_int(reader,&mode)) return -1;

  if (linetype < -8 || linetype > 8)
    {
      if (reader->verbosity)
	hpgs_log(hpgs_i18n("LT: Illegal linetype %d.\n"),linetype);
      return 0;
    }

  // line type are store as percentages.
  patlen *= 0.01;

  if (mode)
    patlen *= MM_TO_PT;
  else
    patlen *= hypot(reader->P2.x-reader->P1.x,
		    reader->P2.y-reader->P1.y ) * 0.01 * HP_TO_PT;

  ndash = reader->linetype_nsegs[linetype+8];
  if (ndash > 20) ndash = 20;

  for (i=0;i<ndash;++i)
    dashes[i] = reader->linetype_segs[linetype+8][i] * patlen;
  
  return hpgs_setdash(reader->device,
		      dashes,ndash,0.0);
}

/* 
  HPGL command PW (Pen Width)
*/
int hpgs_reader_do_PW (hpgs_reader *reader)
{
  int pen=-1;
  double width=1.0;

  if (!reader->eoc)
    if (hpgs_reader_read_double(reader,&width)) return -1;
  if (!reader->eoc)
    {
      if (hpgs_reader_read_int(reader,&pen)) return -1;

      if (pen < 0 || pen >= reader->npens)
        {
          if (pen >= reader->npens && pen < 256)
            {
              if (hpgs_reader_set_number_of_pens(reader,pen+1))
                return -1;
            }
          else
            {
              if (reader->verbosity)
                hpgs_log(hpgs_i18n("PW: Illegal pen number %d.\n"),pen);
              
              return 0;
            }
        }
    }

  if (reader->verbosity >= 2)
    hpgs_log("PW: pen,width,rel = %d,%g,%d.\n",pen,width,reader->pen_width_relative);

  if (reader->pen_width_relative)
    width *= 10.0;
  else
    width *= MM_TO_PT;

  if (pen < 0)
    {
      int i;
      for (i=0;i<reader->npens;++i)
	reader->pen_widths[i] = width;
    }
  else
    reader->pen_widths[pen] = width;

  if (pen < 0 || pen == reader->current_pen)
    {
      if (hpgs_reader_checkpath(reader)) return -1;

      if (reader->pen_width_relative)
        width *= hypot(reader->P2.x-reader->P1.x,
                       reader->P2.y-reader->P1.y ) * 0.001 * HP_TO_PT;
      else
        width *= HP_TO_PT / reader->world_scale;

      width *= reader->page_scale;

      if (hpgs_setlinewidth(reader->device,width*reader->lw_factor))
	return -1;
    }

  return 0;
}

/* 
  HPGL command WU (Width Unit selection)
*/
int hpgs_reader_do_WU (hpgs_reader *reader)
{
  reader->pen_width_relative=0;

  if (!reader->eoc &&
      hpgs_reader_read_int(reader,&reader->pen_width_relative)) return -1;
  return 0;
}

