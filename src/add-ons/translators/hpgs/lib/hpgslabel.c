/***********************************************************************
 *                                                                     *
 * $Id: hpgslabel.c 381 2007-02-20 09:06:38Z softadm $
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
 * The implementation of old-style HPGL labels.                        *
 *                                                                     *
 ***********************************************************************/

#include <hpgsreader.h>
#include <string.h>
#include <errno.h>
#include <iconv.h>
#if defined ( __MINGW32__ ) || defined ( _MSC_VER )
#include<malloc.h>
#else
#include<alloca.h>
#endif

#define HPGS_LABEL_NFACES 4
#define HPGS_LABEL_NENC 27

typedef struct hpgs_ttf_face_info_st hpgs_ttf_face_info;

struct hpgs_ttf_face_info_st
{
  const char *font_name;
  double x_fac;
  double y_fac;
};

#ifdef WIN32
#define HPGS_LABEL_UCS_TYPE unsigned
#define HPGS_LABEL_UCS_ENC  "UCS-4-INTERNAL"
#else
#define HPGS_LABEL_UCS_TYPE wchar_t
#define HPGS_LABEL_UCS_ENC  "WCHAR_T"
#endif

static hpgs_ttf_face_info face_infos[HPGS_LABEL_NFACES] =
  {
    { "LetterGothic",             1.48, 1.295 },
    { "LetterGothic Italic",      1.48, 1.295 },
    { "LetterGothic Bold",        1.48, 1.295 },
    { "LetterGothic Bold Italic", 1.48, 1.295 }
  };

static const char *encoding_names[HPGS_LABEL_NENC] =
  {
    "HP-ROMAN8",
    "LATIN1",  // 14
    "LATIN2",  // 78
    "LATIN5",  // 174
    "GREEK8",  // 263
    "CSISO25FRENCH", // 6
    "CSISO69FRENCH", // 38
    "CSISO14JISC6220RO", // 11
    "CSISO11SWEDISHFORNAMES", // 19
    "CSISO10SWEDISH", // 115
    "CSISO60NORWEGIAN1", // 4
    "CSISO61NORWEGIAN2", // 36
    "CSISO21GERMAN", // 39
    "CSISO15ITALIAN", // 9
    "CSISO17SPANISH", // 83
    "CSISO85SPANISH2", // 211
    "CSISO16PORTUGESE", // 147
    "CSISO84PORTUGESE2", // 179
    "HEBREW",  // 8,264
    "CYRILLIC",  // 18, 50
    "ECMA-CYRILLIC", // 334
    "MS-MAC-CYRILLIC", // 114
    "WINDOWS-1252", // 309
    "CP850",  // 405
    "CP852", // 565
    "ARABIC7", // 22
    "ARABIC" // 278
  };

static int hpgs_device_label_internal(hpgs_device *dev,
                                      hpgs_point *pos,
                                      const char *str, int str_len,
                                      int face,
                                      iconv_t encoding,
                                      const char* encoding_name,
                                      int posture,
                                      int weight,
                                      const hpgs_point *left_vec,
                                      const hpgs_point *up_vec,
                                      const hpgs_point *space_vec)
{
  int iface = 0;
  size_t i,len,inbytesleft;
  HPGS_LABEL_UCS_TYPE ucs_str[HPGS_MAX_LABEL_SIZE];
  size_t outbytesleft;
  char *inbuf;
  char *outbuf;

  // get the face index.
  if (posture)
    iface += 1;

  if (weight >= 3)
    iface += 2;

  // open the face.
  hpgs_font *font = hpgs_find_font(face_infos[iface].font_name);
      
  if (!font) return -1;

  // convert to UNICODE
  if (str_len > HPGS_MAX_LABEL_SIZE) str_len=HPGS_MAX_LABEL_SIZE;
  inbytesleft = str_len;
  outbytesleft = str_len*sizeof(HPGS_LABEL_UCS_TYPE);
  inbuf =  hpgs_alloca(str_len);

  if (!inbuf)
    return hpgs_set_error(hpgs_i18n("unable to allocate %d bytes of temporary string data."),
                          str_len);
  memcpy(inbuf,str,str_len);

  outbuf = (char*)ucs_str;

  len=
    iconv(encoding,&inbuf,&inbytesleft,&outbuf,&outbytesleft);

  if (len == (size_t)(-1))
    return hpgs_set_error(hpgs_i18n("unable to convert string to encoding %s: %s."),
                          encoding_name,strerror(errno));

  len = (str_len*sizeof(HPGS_LABEL_UCS_TYPE) - outbytesleft) / sizeof(HPGS_LABEL_UCS_TYPE);

  // OK, now let's go.
  hpgs_matrix m;

  m.mxx = left_vec->x * face_infos[iface].x_fac;
  m.mxy = up_vec->x * face_infos[iface].y_fac;
  m.myx = left_vec->y * face_infos[iface].x_fac;
  m.myy = up_vec->y * face_infos[iface].y_fac;

  for (i=0;i<len;i++)
    {
      m.dx = pos->x;
      m.dy = pos->y;

      unsigned gid = hpgs_font_get_glyph_id(font,(int)ucs_str[i]);

      if (hpgs_font_draw_glyph(font,dev,&m,gid))
        return -1;
      
      pos->x += space_vec->x;
      pos->y += space_vec->y;
    }

  return 0;
}

int hpgs_reader_label(hpgs_reader *reader,
                      const char *str, int str_len,
                      int face,
                      int encoding,
                      int posture,
                      int weight,
                      const hpgs_point *left_vec,
                      const hpgs_point *up_vec,
                      const hpgs_point *space_vec)
{
  int ienc;

  // open the encoding.
  switch (encoding)
    {
    case 14:  ienc = 1; break;
    case 78:  ienc = 2; break;
    case 174: ienc = 3; break;
    case 263: ienc = 4; break;
    case 6:   ienc = 5; break;
    case 38:  ienc = 6; break;
    case 11:  ienc = 7; break;
    case 19:  ienc = 8; break;
    case 115: ienc = 9; break;
    case 4:   ienc = 10; break;
    case 36:  ienc = 11; break;
    case 39:  ienc = 12; break;
    case 9:   ienc = 13; break;
    case 83:  ienc = 14; break;
    case 211: ienc = 15; break;
    case 147: ienc = 16; break;
    case 179: ienc = 17; break;
    case 8:  case 264: ienc = 18; break;
    case 18: case  50: ienc = 19; break;
    case 334: ienc = 20; break;
    case 114: ienc = 21; break;
    case 309: ienc = 22; break;
    case 405: ienc = 23; break;
    case 565: ienc = 24; break;
    case 22:  ienc = 25; break;
    case 278: ienc = 26; break;
    default: ienc = 0;
    }

  iconv_t ic = iconv_open(HPGS_LABEL_UCS_ENC,encoding_names[ienc]);

  if (ic == (iconv_t)(-1))
    return hpgs_set_error(hpgs_i18n("unable to open encoding %s."),
                          encoding_names[ienc]);

  int ret =hpgs_device_label_internal(reader->device,&reader->current_point,
                                      str,str_len,face,
                                      ic,encoding_names[ienc],
                                      posture,weight,left_vec,up_vec,space_vec);

  iconv_close(ic);
  return ret;
}

int hpgs_device_label(hpgs_device *dev,
                      hpgs_point *pos,
                      const char *str, int str_len,
                      int face,
                      const char *encoding,
                      int posture,
                      int weight,
                      const hpgs_point *left_vec,
                      const hpgs_point *up_vec,
                      const hpgs_point *space_vec)
{
  // open the encoding.
  iconv_t ic = iconv_open(HPGS_LABEL_UCS_ENC,encoding);
  int ret;

  if (ic == (iconv_t)(-1))
    return hpgs_set_error(hpgs_i18n("unable to open encoding %s."),encoding);

  ret = hpgs_device_label_internal(dev,pos,str,str_len,face,
                                   ic,encoding,
                                   posture,weight,left_vec,up_vec,space_vec);
  iconv_close(ic);
  return ret;
}
