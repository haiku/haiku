/***********************************************************************
 *                                                                     *
 * $Id: hpgsbase.c 270 2006-01-29 21:12:23Z softadm $
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
 * The implementation of basic helper functions.                       *
 *                                                                     *
 ***********************************************************************/

#include<hpgs.h>
#include<string.h>

/*!
   Parses a physical paper size with a given unit and returns the
   width and the height of the paper in PostScript pt (1/72 inch).

   The standard formats A4,A3,A2,A1 an A0 as well
   as their landscape counterparts A4l,A3l,A2l,A1l and A0l are
   accepted.

   If the string does not represent a standard paper size,
   it must be of the format <width>x<height>, where <width>
   and <height> must follow the convention of \c hpgs_parse_papersize. 

   The function returns 0, if the string matches these conventions or
   -1, if the string is in a wrong format.
*/
int hpgs_parse_papersize(const char *str, double *pt_width, double *pt_height)
{
  if (strcmp(str,"A4")==0)
    {
      *pt_width = 595.91084545538825881889;
      *pt_height = 842.74519960822752756850;
    }
  else if (strcmp(str,"A3")==0)
    {
      *pt_width = 842.74519960822752756850;
      *pt_height = 1191.82169091077651766614;
    }
  else if (strcmp(str,"A2")==0)
    {
      *pt_width = 1191.82169091077651766614;
      *pt_height = 1685.49039921645505516535;
    }
  else if (strcmp(str,"A1")==0)
    {
      *pt_width = 1685.49039921645505516535;
      *pt_height = 2383.64338182155303536062;
    }
  else if (strcmp(str,"A0")==0)
    {
      *pt_width = 2383.64338182155303536062;
      *pt_height = 3370.98079843291011035905;
    }
  else if (strcmp(str,"A4l")==0)
    {
      *pt_height = 595.91084545538825881889;
      *pt_width = 842.74519960822752756850;
    }
  else if (strcmp(str,"A3l")==0)
    {
      *pt_height = 842.74519960822752756850;
      *pt_width = 1191.82169091077651766614;
    }
  else if (strcmp(str,"A2l")==0)
    {
      *pt_height = 1191.82169091077651766614;
      *pt_width = 1685.49039921645505516535;
    }
  else if (strcmp(str,"A1l")==0)
    {
      *pt_height = 1685.49039921645505516535;
      *pt_width = 2383.64338182155303536062;
    }
  else if (strcmp(str,"A0l")==0)
    {
      *pt_height = 2383.64338182155303536062;
      *pt_width = 3370.98079843291011035905;
    }
  else
    {
      // find the 'x'
      const char *x=strchr(str,'x');
      char wstr[32];
      int l;

      if (!x) return -1;
      l=x-str;
      if (l > 31) return -1;
      
      strncpy(wstr,str,l);
      wstr[l] = '\0';

      if (hpgs_parse_length(wstr,pt_width)) return -1;
      if (hpgs_parse_length(str+l+1,pt_height)) return -1;
    }
  return 0;
}

/*!
   Parses a physical length with a given unit and returns the length
   in PostScript pt (1/72 inch).

   The following formats are accepted:

   \verbatim
     27
     37pt
     4.5inch
     0.37m
     1.5cm
     11.34mm
   \endverbatim

   If no unit is specified, PostScript points (1/72 inch) are assumed.

   The function returns 0, if the string matches these conventions or
   -1, if the string is in a wrong format.

*/
int hpgs_parse_length(const char *str, double *pt_length)
{
  const char *unit="pt";

  char * endptr = (char*)str;

  *pt_length = strtod(str,&endptr);

  if (endptr == str) return -1;
  if (*endptr) unit = endptr;

  if (strcmp(unit,"cm")==0)
    {
      *pt_length *= 72.0 / 2.54;
    }
  else if (strcmp(unit,"mm")==0)
    {
      *pt_length *= 72.0 / 25.4;
    }
  else if (strcmp(unit,"m")==0)
    {
      *pt_length *= 72.0 / 0.0254;
    }
  else if (strcmp(unit,"inch")==0)
    {
      *pt_length *= 72.0;
    }
  else if (strcmp(unit,"pt"))
    return -1;

  return 0;
}
