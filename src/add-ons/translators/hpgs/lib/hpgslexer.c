/***********************************************************************
 *                                                                     *
 * $Id: hpgslexer.c 298 2006-03-05 18:18:03Z softadm $
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
#include <string.h>
#include <ctype.h>

int hpgs_reader_check_param_end(hpgs_reader *reader)
{
  do
    {
      reader->last_byte = hpgs_getc(reader->in);
      if (reader->last_byte == EOF)
	return -1;
    }
  while (isspace(reader->last_byte));

  reader->bytes_ignored = 0;

  if (reader->last_byte == ';')
    {
      reader->bytes_ignored = 0;
      reader->eoc = 1;
      return 0;
    }
  if (reader->last_byte == ',')
    {
      do
        {
          reader->last_byte = hpgs_getc(reader->in);
          if (reader->last_byte == EOF)
            return -1;
        }
      while (isspace(reader->last_byte));

      if (isalpha(reader->last_byte) || reader->last_byte == HPGS_ESC)
        {
          reader->bytes_ignored = 1;
          reader->eoc = 1;
          return 0;
        }
      else
        {
          hpgs_ungetc(reader->last_byte,reader->in);
          reader->bytes_ignored = 0;
          reader->eoc = 0;
          return 0;
        }
    }
  if (reader->last_byte == '"' || reader->last_byte == '\'' ||
      isdigit(reader->last_byte) ||
      reader->last_byte == '+' || reader->last_byte == '-' ||
      reader->last_byte == '.')
    {
      hpgs_ungetc(reader->last_byte,reader->in);
      reader->bytes_ignored = 0;
      reader->eoc = 0;
      return 0;
    }
  if (isalpha(reader->last_byte) || reader->last_byte == HPGS_ESC)
    {
      reader->bytes_ignored = 1;
      reader->eoc = 1;
      return 0;
    }
  return -1;
}

/* read an integer argument from the stream - PCL version
   return values: -1... read error/EOF
                   0... no integer found.
		   1... integer found.
*/
int hpgs_reader_read_pcl_int(hpgs_reader *reader, int *x, int *sign)
{
  *sign = 0;
  *x=0;

  reader->last_byte = hpgs_getc(reader->in);
  if (reader->last_byte == EOF)
	return -1;

  if (reader->last_byte == '-')
    { *sign = -1; reader->last_byte=hpgs_getc(reader->in); }
  else if (reader->last_byte == '+')
    { *sign = 1; reader->last_byte=hpgs_getc(reader->in); }
 
  if (reader->last_byte == EOF)
    return -1;

  if (!isdigit(reader->last_byte))
    { hpgs_ungetc(reader->last_byte,reader->in); return 0; }

  do
    {
      *x = 10 * (*x) + (reader->last_byte-'0');

      reader->last_byte = hpgs_getc(reader->in);
      if (reader->last_byte == EOF)
	return -1;
    }
  while (isdigit(reader->last_byte));

  hpgs_ungetc(reader->last_byte,reader->in);
  if (*sign < 0)
    *x = -*x;
  
  return 1;
}

/* read an integer argument from the stream 
   return values: -1... read error/EOF
                   0... success.
		   1... End of command.
*/
int hpgs_reader_read_int(hpgs_reader *reader, int *x)
{
  int sign = 0;

  if (reader->eoc) return -1;

  *x=0;

  do
    {
      reader->last_byte = hpgs_getc(reader->in);
      if (reader->last_byte == EOF)
	return -1;
    }
  while (isspace(reader->last_byte) || reader->last_byte==',');

  if (reader->last_byte == '-')
    sign = 1;
  else if (reader->last_byte == '+')
    sign = 0;
  else if (isdigit(reader->last_byte))
    hpgs_ungetc(reader->last_byte,reader->in);
  else
    return -1;

  while (1)
    {
      reader->last_byte = hpgs_getc(reader->in);
      if (reader->last_byte == EOF)
	return -1;
	
      if (isdigit(reader->last_byte))
	*x = 10 * (*x) + (reader->last_byte-'0');
      else
	break;
    }

  hpgs_ungetc(reader->last_byte,reader->in);

  return hpgs_reader_check_param_end(reader);
}

/* read a double argument from the stream 
   return values: -1... read error/EOF
                   0... success.
		   1... End of command.
*/
int hpgs_reader_read_double(hpgs_reader *reader, double *x)
{
  int sign = 0;
  
  if (reader->eoc) return -1;

  *x=0.0;

  do
    {
      reader->last_byte = hpgs_getc(reader->in);
      if (reader->last_byte == EOF)
	return -1;
    }
  while (isspace(reader->last_byte) || reader->last_byte==',');

  if (reader->last_byte == '-')
    sign = 1;
  else if (reader->last_byte == '+')
    sign = 0;
  else if (!isdigit(reader->last_byte) && reader->last_byte != '.')
    return -1;

  if (isdigit(reader->last_byte))
    hpgs_ungetc(reader->last_byte,reader->in);

  if (reader->last_byte != '.')
    while (1)
      {
	reader->last_byte = hpgs_getc(reader->in);
	if (reader->last_byte == EOF)
	  return -1;
	
	if (isdigit(reader->last_byte))
	  *x = 10.0 * (*x) + (reader->last_byte-'0');
	else
	  break;
      }

  if (reader->last_byte == '.')
    {
      double xx=1.0; 

      while (1)
	{
	  reader->last_byte = hpgs_getc(reader->in);
	  if (reader->last_byte == EOF)
	    return -1;
	  
	  xx *= 0.1;

	  if (isdigit(reader->last_byte))
	    *x += xx * (reader->last_byte-'0');
	  else
	    break;
	}
    }

  hpgs_ungetc(reader->last_byte,reader->in);

  if (sign) *x = -*x;

  return hpgs_reader_check_param_end(reader);
}

int hpgs_reader_read_point(hpgs_reader *reader, hpgs_point *p, int xform)
{
  if (hpgs_reader_read_double(reader,&p->x)) return -1;
  if (reader->eoc) return -1;
  if (hpgs_reader_read_double(reader,&p->y)) return -1;

  switch (xform)
    {
    case 1:
      hpgs_matrix_xform (p,&reader->total_matrix,p);
      break;
    case -1:
      hpgs_matrix_scale (p,&reader->total_matrix,p);
      break;
    default:
      break;
    }

  return 0;
}

/* read an integer argument from the stream 
   return values: -1... read error/EOF
                   0... success.
		   1... End of command.
*/
int hpgs_reader_read_new_string(hpgs_reader *reader, char *str)
{
  int term=',';
  int i=0;

  if (reader->eoc) return -1;

  do
    {
      reader->last_byte = hpgs_getc(reader->in);
      if (reader->last_byte == EOF)
	return -1;
    }
  while (isspace(reader->last_byte));

  if (reader->last_byte == '\'' || reader->last_byte=='"')
    term = reader->last_byte;
  else
    str[i++] = reader->last_byte;

  do
    {
      reader->last_byte = hpgs_getc(reader->in);
      if (reader->last_byte == EOF)
	return -1;

      if (i<HPGS_MAX_LABEL_SIZE)
	str[i] = reader->last_byte;

      ++i;

      if (term == ',' && reader->last_byte == ';') break;
    }
  while (reader->last_byte != term);

  if (i) --i;
  str[i] = '\0';

  return hpgs_reader_check_param_end(reader);
}

int hpgs_reader_read_label_string(hpgs_reader *reader, char *str)
{
  int i=0;

  if (reader->eoc) return -1;

  reader->bytes_ignored = 0;

  do
    {
      reader->last_byte = hpgs_getc(reader->in);
      if (reader->last_byte == EOF)
	return -1;

      if (i<HPGS_MAX_LABEL_SIZE)
	str[i] = reader->last_byte;

      ++i;
    }
  while (reader->last_byte != reader->label_term);

  if (reader->label_term_ignore && i)
    --i;

  str[i] = '\0';

  return hpgs_reader_check_param_end(reader);
}

