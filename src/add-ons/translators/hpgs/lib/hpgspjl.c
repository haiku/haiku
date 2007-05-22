/***********************************************************************
 *                                                                     *
 * $Id: hpgspjl.c 384 2007-03-18 18:31:15Z softadm $
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
 * The implementation of the PJL interpreter.                          *
 *                                                                     *
 ***********************************************************************/

#include <hpgsreader.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

//#define HPGS_PJL_DEBUG

static int pjl_read_line (hpgs_reader *reader, char term, char *buf, size_t buf_size)
{
  int i=0;

  while ((reader->last_byte = hpgs_getc(reader->in)) != EOF)
    {
      if (i == 0 && reader->last_byte== HPGS_ESC)
        {
          hpgs_ungetc(reader->last_byte,reader->in);
          return -2;
        }

      if (reader->last_byte==term)
        {
          while ((reader->last_byte = hpgs_getc(reader->in)) != EOF)
            if (!isspace(reader->last_byte))
              {
                hpgs_ungetc(reader->last_byte,reader->in);
                break;
              }

          while (i > 0 && (buf[i-1]=='\0' || isspace(buf[i-1])))
            --i;

          if (i<buf_size) buf[i]='\0';

          return i;
        }

      if (reader->last_byte == '\n' && i==4 && strncmp(buf,"@PJL",4)==0)
        {
          i=0;
          continue;
        }

      if (reader->last_byte == '\n') continue;

      if (reader->last_byte != '\r' && i<buf_size)
        {
          buf[i] = (i < buf_size-1) ? (char)reader->last_byte : '\0';
          ++i;
        }
    }

  return EOF;
}

/*!
  Read a PJL block of the file.

  Return values:
   \li -1 Error, unexpected EOF.
   \li 0 Success, ESC found, continue with ESC evaluation.
   \li 1 Success, return to HPGL context.
   \li 2 Success, plotsize set, skip interpretation of file.
   \li 3 Success, go to PCL context.
*/
int hpgs_reader_do_PJL(hpgs_reader *reader)
{
  char buf[256];
  int r,arg,arg_sign;
  int x_size = 0;
  int y_size = 0;
  reader->bytes_ignored = 0;
  reader->eoc = 0;
  size_t pos = 0;

  while ((r=pjl_read_line(reader,' ',buf,sizeof(buf))) != EOF)
    {
      if (r==-2) return 0; // ESC found.

      // HPGL autodetection.
      if (pos >= 0 && r>=2 &&
          buf[0] >= 'A' && buf[0] <= 'Z' &&
          buf[1] >= 'A' && buf[1] <= 'Z'   )
        {
          if (hpgs_istream_seek(reader->in,pos))
            return -1;

          // overtake PCL papersize, if given.
          if (x_size > 0 && y_size > 0)
            {
              // PJL paper sizes are apparently in PCL units.
              x_size *= reader->pcl_scale/HP_TO_PT;
              y_size *= reader->pcl_scale/HP_TO_PT;
              
              if (reader->verbosity)
                hpgs_log(hpgs_i18n("Setting PJL papersize %dx%d.\n"),x_size,y_size);

              switch (hpgs_reader_set_plotsize(reader,x_size,y_size))
                {
                case 2:
                  return 2;
                case 0:
                  break;
                default:
                  return -1; 
                }
            }
       
          // return to HPGL context.
          return 1;
        }

      if (strcmp(buf,"@PJL"))
        return hpgs_set_error(hpgs_i18n("Illegal line heading <%s> in PJL."),buf);
      if (pjl_read_line(reader,' ',buf,sizeof(buf)) < 0)
        return -1;

      if (strcmp(buf,"SET") == 0)
        {
          if (pjl_read_line(reader,'=',buf,sizeof(buf)) < 0)
            return -1;

          if (strcmp(buf,"PAPERLENGTH") == 0)
            {
              switch (hpgs_reader_read_pcl_int(reader,&arg,&arg_sign))
                {
                case -1:
                  return -1;
                case 1:
                  break;
                default:
                  return hpgs_set_error(hpgs_i18n("Missing number in PJL SET PAPERLENGTH."));
                }
             
              x_size = arg;
            }
          else if (strcmp(buf,"PAPERWIDTH") == 0)
            {
              switch (hpgs_reader_read_pcl_int(reader,&arg,&arg_sign))
                {
                case -1:
                  return -1;
                case 1:
                  break;
                default:
                  return hpgs_set_error(hpgs_i18n("Missing number in PJL SET PAPERWIDTH."));
                }
             
              y_size = arg;
            }
          else if (strcmp(buf,"RESOLUTION") == 0)
            {
              switch (hpgs_reader_read_pcl_int(reader,&arg,&arg_sign))
                {
                case -1:
                  return -1;
                case 1:
                  break;
                default:
                  return hpgs_set_error(hpgs_i18n("Missing number in PJL SET RESOLUTION."));
                }
             
              reader->pcl_scale = 72.0/(double)arg;
              reader->pcl_raster_res = arg;
            }
        }
      else if (strcmp(buf,"ENTER") == 0)
        {
          if (pjl_read_line(reader,'=',buf,sizeof(buf)) < 0)
            return -1;

          if (strcmp(buf,"LANGUAGE") == 0)
            {
              if (pjl_read_line(reader,'\n',buf,sizeof(buf)) < 0)
                return -1;
              
              if (x_size > 0 && y_size > 0)
                {
                  // PJL paper sizes are apparently in PCL units.
                  x_size *= reader->pcl_scale/HP_TO_PT;
                  y_size *= reader->pcl_scale/HP_TO_PT;
                    
                  if (reader->verbosity)
                    hpgs_log(hpgs_i18n("Setting PJL papersize %dx%d\n"),x_size,y_size);

                  switch (hpgs_reader_set_plotsize(reader,x_size,y_size))
                    {
                    case 2:
                      return 2;
                    case 0:
                      break;
                    default:
                      return -1; 
                    }
                }

              if (strcmp(buf,"HPGL2")   == 0 ||
                  strcmp(buf,"HPGL/2")  == 0 ||
                  strcmp(buf,"HP-GL2")  == 0 ||
                  strcmp(buf,"HP-GL/2") == 0   )
                {
                  // probe for escape character
                  if ((reader->last_byte = hpgs_getc(reader->in)) == EOF)
                    return -1;
   
                  hpgs_ungetc(reader->last_byte,reader->in);
                  return reader->last_byte == HPGS_ESC ? 0 : 1;
                }
              else if (strcmp(buf,"PCL3GUI") == 0 ||
                       strcmp(buf,"PCL3") == 0 ||
                       strcmp(buf,"PCL5") == 0   )
                return 3;
              else
                return hpgs_set_error(hpgs_i18n("Invalid language <%s> in PJL ENTER LANGUAGE."),
                                      buf);
            }
        }
        
      if (pjl_read_line(reader,'\n',buf,sizeof(buf)) < 0)
        return -1;

      // store position for HPGL autodetection.
      if (hpgs_istream_tell(reader->in,&pos))
        return hpgs_set_error(hpgs_i18n("Cannot get file position in PJL interpretation."));
    }
  
  return -1;
}
