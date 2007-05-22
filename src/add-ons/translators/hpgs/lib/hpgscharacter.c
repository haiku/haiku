/***********************************************************************
 *                                                                     *
 * $Id: hpgscharacter.c 383 2007-03-18 18:29:22Z softadm $
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

// The size of a line feed measured in the cap height of a font.
#define HPGS_LF_FAC 1.75
#define HPGS_VSPACE_FAC 1.5

/* 
  HPGL command AD (Alternate font Definition)
*/
int hpgs_reader_do_AD (hpgs_reader *reader)
{
  while (!reader->eoc)
    {
      int kind;
      if (hpgs_reader_read_int(reader,&kind)) return -1;
      if (reader->eoc) return -1;

      switch (kind)
	{
	case 1:
	  if (hpgs_reader_read_int(reader,&reader->alternate_encoding))
	    return -1;
	  break;
	case 2:
	  if (hpgs_reader_read_int(reader,&reader->alternate_spacing))
	    return -1;
	  break;
	case 3:
	  if (hpgs_reader_read_double(reader,&reader->alternate_pitch))
	    return -1;
	  break;
	case 4:
	  if (hpgs_reader_read_double(reader,&reader->alternate_height))
	    return -1;
	  break;
	case 5:
	  if (hpgs_reader_read_int(reader,&reader->alternate_posture))
	    return -1;
	  break;
	case 6:
	  if (hpgs_reader_read_int(reader,&reader->alternate_weight))
	    return -1;
	  break;
	case 7:
	  if (hpgs_reader_read_int(reader,&reader->alternate_face))
	    return -1;
	  break;
	default:
	  return -1;
	}
    }
  return 0;
}

/* 
  HPGL command SD (Standard font Definition)
*/
int hpgs_reader_do_SD (hpgs_reader *reader)
{
  while (!reader->eoc)
    {
      int kind;
      if (hpgs_reader_read_int(reader,&kind)) return -1;
      if (reader->eoc) return -1;

      switch (kind)
	{
	case 1:
	  if (hpgs_reader_read_int(reader,&reader->default_encoding))
	    return -1;
	  break;
	case 2:
	  if (hpgs_reader_read_int(reader,&reader->default_spacing))
	    return -1;
	  break;
	case 3:
	  if (hpgs_reader_read_double(reader,&reader->default_pitch))
	    return -1;
	  break;
	case 4:
	  if (hpgs_reader_read_double(reader,&reader->default_height))
	    return -1;
	  break;
	case 5:
	  if (hpgs_reader_read_int(reader,&reader->default_posture))
	    return -1;
	  break;
	case 6:
	  if (hpgs_reader_read_int(reader,&reader->default_weight))
	    return -1;
	  break;
	case 7:
	  if (hpgs_reader_read_int(reader,&reader->default_face))
	    return -1;
	  break;
	default:
	  return -1;
	}
    }
  return 0;
}

/* 
  HPGL command CP (Character Plot)
*/
int hpgs_reader_do_CP (hpgs_reader *reader)
{
  double w,h,ca,sa,spaces=0.0;
  double lines=reader->current_text_line ? 1.0 : -1.0;
  hpgs_point space_vec;
  hpgs_point lf_vec;

 
  if (hpgs_reader_checkpath(reader)) return -1;

  if (reader->eoc)
    reader->current_point = reader->cr_point;
  else
    {
      if (hpgs_reader_read_double(reader,&spaces)) return -1;
      if (hpgs_reader_read_double(reader,&lines)) return -1;
    }
  
  w =
    reader->current_char_size.x ?  (1.5 * reader->current_char_size.x) :
    (72.0 / (reader->alternate_font ?
	     reader->alternate_pitch :
	     reader->default_pitch    ));

  h =
    reader->current_char_size.y ? reader->current_char_size.y :
    (reader->alternate_font ?
     reader->alternate_height :
     reader->default_height    );

  ca = cos(reader->current_label_angle);
  sa = sin(reader->current_label_angle);

  switch (reader->current_text_path % 4)
    {
    case 0:
      space_vec.x = ca * (1.0+reader->current_extra_space.x) * w;
      space_vec.y = sa * (1.0+reader->current_extra_space.x) * w;
      lf_vec.x =  sa * (HPGS_LF_FAC+reader->current_extra_space.y) * h;
      lf_vec.y = -ca * (HPGS_LF_FAC+reader->current_extra_space.y) * h;
      break;
    case 1:
      space_vec.x =  sa * (HPGS_VSPACE_FAC+reader->current_extra_space.y) * h;
      space_vec.y = -ca * (HPGS_VSPACE_FAC+reader->current_extra_space.y) * h;
      lf_vec.x = -ca * (HPGS_LF_FAC+reader->current_extra_space.x) * w;
      lf_vec.y = -sa * (HPGS_LF_FAC+reader->current_extra_space.x) * w;
      break;
    case 2:
      space_vec.x = -ca * (1.0+reader->current_extra_space.x) * w;
      space_vec.y = -sa * (1.0+reader->current_extra_space.x) * w;
      lf_vec.x = -sa * (HPGS_LF_FAC+reader->current_extra_space.y) * h;
      lf_vec.y =  ca * (HPGS_LF_FAC+reader->current_extra_space.y) * h;
      break;
    default: // 3
      space_vec.x = -sa * (HPGS_VSPACE_FAC+reader->current_extra_space.y) * h;
      space_vec.y =  ca * (HPGS_VSPACE_FAC+reader->current_extra_space.y) * h;
      lf_vec.x = ca * (HPGS_LF_FAC+reader->current_extra_space.x) * w;
      lf_vec.y = sa * (HPGS_LF_FAC+reader->current_extra_space.x) * w;
    }

  if (reader->current_text_line)
    lines = -lines;

  reader->current_point.x += space_vec.x * spaces;
  reader->current_point.y += space_vec.y * spaces;
  reader->current_point.x += lf_vec.x * lines;
  reader->current_point.y += lf_vec.y * lines;

  return 0;
}

/* 
  HPGL command DI (absolute DIrection)
*/
int hpgs_reader_do_DI (hpgs_reader *reader)
{
  double run  = 1.0;
  double rise = 0.0;

  if (!reader->eoc)
    {
      if (hpgs_reader_read_double(reader,&run)) return -1;
      if (hpgs_reader_read_double(reader,&rise)) return -1;
    }

  reader->current_label_angle = atan2(rise,run);
  
  return 0;
}

/* 
  HPGL command DR (Relative Direction)
*/
int hpgs_reader_do_DR (hpgs_reader *reader)
{
  double run  = 0.01;
  double rise = 0.0;

  if (!reader->eoc)
    {
      if (hpgs_reader_read_double(reader,&run)) return -1;
      if (hpgs_reader_read_double(reader,&rise)) return -1;
    }

  run  *= (reader->P2.x - reader->P1.x);
  rise *= (reader->P2.y - reader->P1.y);

  reader->current_label_angle = atan2(rise,run);
  
  return 0;
}

/* 
  HPGL command DV (Define Variable Text path)
*/
int hpgs_reader_do_DV (hpgs_reader *reader)
{
  reader->current_text_path = 0;
  reader->current_text_line = 0;

  if (!reader->eoc &&
      hpgs_reader_read_int(reader,&reader->current_text_path)) return -1;
  
  if (!reader->eoc &&
      hpgs_reader_read_int(reader,&reader->current_text_line)) return -1;
  
  return 0;
}

/* 
  HPGL command ES (Extra Space)
*/
int hpgs_reader_do_ES (hpgs_reader *reader)
{
  reader->current_extra_space.x = 0.0;
  reader->current_extra_space.y = 0.0;

  if (!reader->eoc &&
      hpgs_reader_read_double(reader,&reader->current_extra_space.x)) return -1;
  
  if (!reader->eoc &&
      hpgs_reader_read_double(reader,&reader->current_extra_space.y)) return -1;
  
  return 0;
}

/* 
  HPGL command LO (Label Origin)
*/
int hpgs_reader_do_LO (hpgs_reader *reader)
{
  reader->current_label_origin = 1;

  if (!reader->eoc &&
      hpgs_reader_read_int(reader,&reader->current_label_origin)) return -1;
  
  return 0;
}

/* 
  HPGL command SI (absolute character SIze)
*/
int hpgs_reader_do_SI (hpgs_reader *reader)
{
  reader->current_char_size.x = 0.0;
  reader->current_char_size.y = 0.0;

  if (reader->eoc) return 0;

  if (hpgs_reader_read_double(reader,&reader->current_char_size.x)) return -1;
  if (hpgs_reader_read_double(reader,&reader->current_char_size.y)) return -1;
  
  reader->current_char_size.x *= 72.0 / 2.54;
  reader->current_char_size.y *= 72.0 / 2.54;

  return 0;
}

/* 
  HPGL command SR (Relative character Size)
*/
int hpgs_reader_do_SR (hpgs_reader *reader)
{
  reader->current_char_size.x = 0.0;
  reader->current_char_size.y = 0.0;

  if (reader->eoc) return 0;

  if (hpgs_reader_read_double(reader,&reader->current_char_size.x)) return -1;
  if (hpgs_reader_read_double(reader,&reader->current_char_size.y)) return -1;
  
  reader->current_char_size.x *= (reader->P2.x-reader->P1.x) * 0.01 * HP_TO_PT;
  reader->current_char_size.y *= (reader->P2.y-reader->P1.y) * 0.01 * HP_TO_PT;

  return 0;
}

/* 
  HPGL command SL (character SLant)
*/
int hpgs_reader_do_SL (hpgs_reader *reader)
{
  reader->current_slant = 0.0;

  if (!reader->eoc &&
      hpgs_reader_read_double(reader,&reader->current_slant)) return -1;
  
  return 0;
}

/* 
  HPGL command SA (Select Alternate font)
*/
int hpgs_reader_do_SA (hpgs_reader *reader)
{
  reader->alternate_font = 1;
  return 0;
}

/* 
  HPGL command SS (Select Standard font)
*/
int hpgs_reader_do_SS(hpgs_reader *reader)
{
  reader->alternate_font = 0;
  return 0;
}

/* 
  HPGL command DT (Define label Terminator)
*/
int hpgs_reader_do_DT (hpgs_reader *reader)
{
  reader->bytes_ignored = 0;

  reader->last_byte = hpgs_getc(reader->in);
  if (reader->last_byte == EOF)
    return -1;

  if (reader->last_byte == ';')
    {
      reader->label_term = '\003';
      reader->label_term_ignore = 1;
      reader->eoc = 1;
      return 0;
    }
  else
    {
      reader->label_term = reader->last_byte;
      reader->last_byte = hpgs_getc(reader->in);
      if (reader->last_byte == EOF)
	return -1;
    }
  
  if (reader->last_byte == ',')
    {
      reader->label_term_ignore = 0;

      while (isspace(reader->last_byte = hpgs_getc(reader->in))) ;

      while (isdigit(reader->last_byte = hpgs_getc(reader->in)))
	if (reader->last_byte != '0') 
	  reader->label_term_ignore = 1;

      if (reader->last_byte == EOF)
	return -1;
    }
  else
    reader->label_term_ignore = 1;

  while (isspace(reader->last_byte)) reader->last_byte = hpgs_getc(reader->in);

  if (isalpha(reader->last_byte) || reader->last_byte == HPGS_ESC)
    reader->bytes_ignored = 1;
  else
    {
      if (reader->last_byte != ';')
        return -1;

      reader->bytes_ignored = 0;
    }

  reader->eoc = 1;
  return 0;
}

/* 
  HPGL command SM (Symbol Mode)
*/
int hpgs_reader_do_SM (hpgs_reader *reader)
{
  reader->bytes_ignored = 0;

  if (reader->eoc)
    return 0;

  reader->last_byte = hpgs_getc(reader->in);
  if (reader->last_byte == EOF)
    return -1;

  return  hpgs_reader_check_param_end(reader);
}

/* 
  HPGL command LB (LaBel)
*/
int hpgs_reader_do_LB (hpgs_reader *reader)
{
  char str[HPGS_MAX_LABEL_SIZE];
  double w,h,ca,sa;
  hpgs_point left_vec;
  hpgs_point up_vec;
  hpgs_point space_vec;
  hpgs_point lf_vec;
  hpgs_point adj_vec;
  int i0=0;
  int ipos=0;
  int i;

  if (hpgs_reader_read_label_string(reader,str)) return -1;

  if (hpgs_reader_checkpath(reader)) return -1;

  w =
    reader->current_char_size.x ? (1.5 * reader->current_char_size.x) :
    (72.0 / (reader->alternate_font ?
	     reader->alternate_pitch :
	     reader->default_pitch    ));

  h =
    reader->current_char_size.y ? reader->current_char_size.y :
    (reader->alternate_font ?
     reader->alternate_height :
     reader->default_height    );

  ca = cos(reader->current_label_angle);
  sa = sin(reader->current_label_angle);

  left_vec.x = ca * w;
  left_vec.y = sa * w;

  up_vec.x = (reader->current_slant * ca - sa) * h;
  up_vec.y = (reader->current_slant * sa + ca) * h;

  switch (reader->current_text_path % 4)
    {
    case 0:
      space_vec.x = ca * (1.0+reader->current_extra_space.x) * w;
      space_vec.y = sa * (1.0+reader->current_extra_space.x) * w;
      lf_vec.x =  sa * (HPGS_LF_FAC+reader->current_extra_space.y) * h;
      lf_vec.y = -ca * (HPGS_LF_FAC+reader->current_extra_space.y) * h;
      adj_vec.x =  sa * h;
      adj_vec.y = -ca * h;
      break;
    case 1:
      space_vec.x =  sa * (HPGS_VSPACE_FAC+reader->current_extra_space.y) * h;
      space_vec.y = -ca * (HPGS_VSPACE_FAC+reader->current_extra_space.y) * h;
      lf_vec.x = -ca * (HPGS_LF_FAC+reader->current_extra_space.x) * w;
      lf_vec.y = -sa * (HPGS_LF_FAC+reader->current_extra_space.x) * w;
      adj_vec.x = -ca * w;
      adj_vec.y = -sa * w;
      break;
    case 2:
      space_vec.x = -ca * (1.0+reader->current_extra_space.x) * w;
      space_vec.y = -sa * (1.0+reader->current_extra_space.x) * w;
      lf_vec.x = -sa * (HPGS_LF_FAC+reader->current_extra_space.y) * h;
      lf_vec.y =  ca * (HPGS_LF_FAC+reader->current_extra_space.y) * h;
      adj_vec.x = -sa * h;
      adj_vec.y =  ca * h;
      break;
    default: // 3
      space_vec.x = -sa * (HPGS_VSPACE_FAC+reader->current_extra_space.y) * h;
      space_vec.y =  ca * (HPGS_VSPACE_FAC+reader->current_extra_space.y) * h;
      lf_vec.x = ca * (HPGS_LF_FAC+reader->current_extra_space.x) * w;
      lf_vec.y = sa * (HPGS_LF_FAC+reader->current_extra_space.x) * w;
      adj_vec.x = ca * w;
      adj_vec.y = sa * w;
    }

  if (reader->current_text_line)
   {
     lf_vec.x = -lf_vec.x;
     lf_vec.y = -lf_vec.y;
   }

  hpgs_matrix_scale(&left_vec, &reader->page_matrix,&left_vec );
  hpgs_matrix_scale(&up_vec,   &reader->page_matrix,&up_vec   );
  hpgs_matrix_scale(&space_vec,&reader->page_matrix,&space_vec);
  hpgs_matrix_scale(&lf_vec,   &reader->page_matrix,&lf_vec   );
  hpgs_matrix_scale(&adj_vec,  &reader->page_matrix,&adj_vec  );

  hpgs_point adj = { 0.0, 0.0 };

  // got through the string and search for control characters.
  for (i=0;i<=HPGS_MAX_LABEL_SIZE;i++)
    {
      if (i < HPGS_MAX_LABEL_SIZE && (unsigned char)str[i] >= 32)
	{
	  if (ipos == 0 && reader->current_label_origin % 10 > 1)
	    {
	      // Everytime we start a new portion of the label,
	      // adapt the current point in order to meet the given label origin.

	      // lookahead for the new CR of EOS.
	      int j;
	      double x_adj,y_adj,nchars=0.0;

	      for (j=i;j<=HPGS_MAX_LABEL_SIZE;j++)
		{
		  if (j < HPGS_MAX_LABEL_SIZE && (unsigned char)str[j] >= 32)
		    nchars+=1.0;
		  else
		    if (j >= HPGS_MAX_LABEL_SIZE || str[j] == 0 || str[j] == 13) break;
		}

	      if (nchars && (reader->current_text_path % 2))
		nchars -= reader->current_extra_space.y / (1.0 + reader->current_extra_space.y);
	      else
		nchars -= reader->current_extra_space.x / (1.0 + reader->current_extra_space.x);

	      x_adj = -0.5 * nchars * ((reader->current_label_origin % 10 - 1) / 3);
	      y_adj =  -0.5 * ((reader->current_label_origin % 10 - 1) % 3);
	  
	      if (reader->current_label_origin / 10)
		{
		  x_adj -= 0.25 * ((reader->current_label_origin % 10 - 1) / 3 - 1);
		  y_adj -= 0.25 * ((reader->current_label_origin % 10 - 1) % 3 - 1);
		}

	      adj.x += x_adj * space_vec.x - y_adj * adj_vec.x;
	      adj.y += x_adj * space_vec.y - y_adj * adj_vec.y;
	    }

	  if (ipos == 0)
	    switch (reader->current_text_path % 4)
	      {
	      case 1:
		adj.x -= up_vec.x;
		adj.y -= up_vec.y;
		break;
	      case 2:
		adj.x -= left_vec.x + up_vec.x;
		adj.y -= left_vec.y + up_vec.y;
		break;
	      case 3:
		adj.x -= left_vec.x;
		adj.y -= left_vec.y;
	      }

	  ++ipos;
	  continue;
	}

      reader->current_point.x += adj.x;
      reader->current_point.y += adj.y;

      if (i>i0 &&
	  hpgs_reader_label(reader,str+i0,i-i0,
                            reader->alternate_font ?
                            reader->alternate_face :
                            reader->default_face,
                            reader->alternate_font ?
                            reader->alternate_encoding :
                            reader->default_encoding,
                            reader->alternate_font ?
                            reader->alternate_posture :
                            reader->default_posture,
                            reader->alternate_font ?
                            reader->alternate_weight :
                            reader->default_weight,
                            &left_vec,
                            &up_vec,
                            &space_vec))
	return -1;

      reader->current_point.x -= adj.x;
      reader->current_point.y -= adj.y;

      adj.x = 0.0;
      adj.y = 0.0;

      if (i >= HPGS_MAX_LABEL_SIZE || str[i] == 0) break;

      i0 = i+1;

      switch (str[i])
	{
	case 8: // backspace
	  reader->current_point.x -= space_vec.x;
	  reader->current_point.y -= space_vec.y;
	  break;

	case 9: // horizontal tab.
	  reader->current_point.x += space_vec.x * (8 - ipos % 8);
	  reader->current_point.y += space_vec.y * (8 - ipos % 8);
	  ipos += (8 - ipos % 8);
	  break;

	case 10: // linefeed.
	  reader->cr_point.x += lf_vec.x;
	  reader->cr_point.y += lf_vec.y;
	  reader->current_point.x += lf_vec.x;
	  reader->current_point.y += lf_vec.y;
	  break;
	  
	case 13: // carriage return.
	  reader->current_point.x = reader->cr_point.x;
	  reader->current_point.y = reader->cr_point.y;
	  ipos=0;
	  break;
	  
	case 14: // shift out.
	  reader->alternate_font = 1;
	  break;
	  
	case 15: // shift in.
	  reader->alternate_font = 0;
	  break;
	  
	default:
	  if (reader->verbosity >= 1)
	    hpgs_log(hpgs_i18n("LB: Ignoring unknown control character <%d>.\n"),
                               (int)str[i]);
	}
    }

  return 0;
}

/* 
  HPGL command MG (MessaGe)
*/
int hpgs_reader_do_MG (hpgs_reader *reader)
{
  char str[HPGS_MAX_LABEL_SIZE];

  if (reader->eoc) return 0;

  if (hpgs_reader_read_new_string(reader,str)) return -1;

  if (reader->verbosity >= 1)
    hpgs_log(hpgs_i18n("Message <%.*s>.\n"),HPGS_MAX_LABEL_SIZE,str);

  return 0;
}
