/***********************************************************************
 *                                                                     *
 * $Id: hpgsfont.c 385 2007-03-18 18:32:07Z softadm $
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
 * The implementation of the public API for ttf font handling.         *
 *                                                                     *
 ***********************************************************************/

#include<hpgsfont.h>
#include<string.h>

#ifdef WIN32
#include <windows.h>
#else
#include<dirent.h>
#endif

#if defined ( __MINGW32__ ) || defined ( _MSC_VER )
#include<malloc.h>
#else
#include<alloca.h>
#endif

static hpgs_mutex_t font_dir_mutex;
static hpgs_font_dentry *font_directory = 0;
static size_t font_directory_sz = 0;
static size_t font_directory_alloc_sz = 0;

// Standard glyph names in mac encoding cf.
// to the OpenType spec.
static char *std_glyph_names[] = {
  // 0 - 9
  ".notdef",".null","nonmarkingreturn","space","exclam","quotedbl","numbersign","dollar","percent","ampersand",
  // 10 - 19
  "quotesingle","parenleft","parenright","asterisk","plus","comma","hyphen","period","slash","zero",
  // 20 - 29
  "one","two","three","four","five","six","seven","eight","nine","colon",
  // 30 - 39
  "semicolon","less","equal","greater","question","at","A","B","C","D",
  // 40 - 49
  "E","F","G","H","I","J","K","L","M","N",
  // 50 - 59
  "O","P","Q","R","S","T","U","V","W","X",
  // 60 - 69
  "Y","Z","bracketleft","backslash","bracketright","asciicircum","underscore","grave","a","b",
  // 70 - 79
  "c","d","e","f","g","h","i","j","k","l",
  // 80 - 89
  "m","n","o","p","q","r","s","t","u","v",
  // 90 - 99
  "w","x","y","z","braceleft","bar","braceright","asciitilde","Adieresis","Aring",
  // 100 - 109
  "Ccedilla","Eacute","Ntilde","Odieresis","Udieresis","aacute","agrave","acircumflex","adieresis","atilde",
  // 110 - 119
  "aring","ccedilla","eacute","egrave","ecircumflex","edieresis","iacute","igrave","icircumflex","idieresis",
  // 120 - 129
  "ntilde","oacute","ograve","ocircumflex","odieresis","otilde","uacute","ugrave","ucircumflex","udieresis",
  // 130 - 139
  "dagger","degree","cent","sterling","section","bullet","paragraph","germandbls","registered","copyright",
  // 140 - 149
  "trademark","acute","dieresis","notequal","AE","Oslash","infinity","plusminus","lessequal","greaterequal",
  // 150 - 159
  "yen","mu","partialdiff","summation","product","pi","integral","ordfeminine","ordmasculine","Omega",
  // 160 - 169
  "ae","oslash","questiondown","exclamdown","logicalnot","radical","florin","approxequal","Delta","guillemotleft",
  // 170 - 179
  "guillemotright","ellipsis","space","Agrave","Atilde","Otilde","OE","oe","endash","emdash",
  // 180 - 189
  "quotedblleft","quotedblright","quoteleft","quoteright","divide","lozenge","ydieresis","Ydieresis","fraction","currency",
  // 190 - 199
  "guilsinglleft","guilsinglright","fi","fl","daggerdbl","periodcentered","quotesinglbase","quotedblbase","perthousand","Acircumflex",
  // 200 - 209
  "Ecircumflex","Aacute","Edieresis","Egrave","Iacute","Icircumflex","Idieresis","Igrave","Oacute","Ocircumflex",
  // 210 - 219
  0,"Ograve","Uacute","Ucircumflex","Ugrave","dotlessi","circumflex","tilde","macron","breve",
  // 220 - 229
  "dotaccent","ring","cedilla","hungarumlaut","ogonek","caron","Lslash","lslash","Scaron","scaron",
  // 230 - 239
  "Zcaron","zcaron","brokenbar","Eth","eth","Yacute","yacute","Thorn","thorn","minus",
  // 240 - 249
  "multiply","onesuperior","twosuperior","threesuperior","onehalf","onequarter","threequarters","franc","Gbreve","gbreve",
  // 250 - 257
  "Idotaccent","Scedilla","scedilla","Cacute","cacute","Ccaron","ccaron","dcroat"
};

void hpgs_font_init()
{
  hpgs_mutex_init(&font_dir_mutex);
}

void hpgs_font_cleanup()
{
  hpgs_mutex_lock(&font_dir_mutex);
  
  size_t i;

  for (i=0;i<font_directory_sz;++i)
    {
      if (font_directory[i].font_name) free (font_directory[i].font_name);
      if (font_directory[i].filename) free (font_directory[i].filename);

      if (font_directory[i].font)
        hpgs_destroy_font(font_directory[i].font);
    }

  if (font_directory) free(font_directory);

  font_directory = 0;
  font_directory_sz = 0;
  font_directory_alloc_sz = 0;

  hpgs_mutex_unlock(&font_dir_mutex);

  hpgs_mutex_destroy(&font_dir_mutex);
}

static void push_font_to_directory(char *name, char *fn)
{
  if (font_directory_sz>=font_directory_alloc_sz)
    {
      hpgs_font_dentry *d = realloc(font_directory,sizeof(hpgs_font_dentry)*2*font_directory_alloc_sz);
      
      if (!d) { free(name); free(fn); return; }
      
      font_directory = d;
      font_directory_alloc_sz *= 2;
    }
  
  font_directory[font_directory_sz].font_name = name;
  font_directory[font_directory_sz].filename = fn;
  font_directory[font_directory_sz].font = 0;

  // hpgs_log("name,fn=%s,%s.\n",name,fn);

  ++font_directory_sz;
}

// helpers for big-endian binary file treatment.
static int read_uint32(hpgs_istream *is, uint32_t *x)
{
  int c1,c2,c3,c4;

  c1=hpgs_getc(is);
  if (c1<0) return -1;
  c2=hpgs_getc(is);
  if (c2<0) return -1;
  c3=hpgs_getc(is);
  if (c3<0) return -1;
  c4=hpgs_getc(is);
  if (c4<0) return -1;

  *x =
    (((uint32_t)(c1))<<24)|
    (((uint32_t)(c2))<<16)|
    (((uint32_t)(c3))<<8)|
    ((uint32_t)(c4));

  return 0;
}

static int read_int32(hpgs_istream *is, int32_t *x) { return read_uint32(is,(uint32_t *)x); }

static int read_uint16(hpgs_istream *is, uint16_t *x)
{
  int c1,c2;

  c1=hpgs_getc(is);
  if (c1<0) return -1;
  c2=hpgs_getc(is);
  if (c2<0) return -1;

  *x =
    (((uint16_t)(c1))<<8)|
    ((uint16_t)(c2));

  return 0;
}

static int read_int16(hpgs_istream *is, int16_t *x) { return read_uint16(is,(uint16_t *)x); }

static int read_uint8(hpgs_istream *is, uint8_t *x)
{
  int c;

  c=hpgs_getc(is);
  if (c<0) return -1;

  *x = (uint8_t)(c);
  return 0;
}

static int read_int8(hpgs_istream *is, int8_t *x) { return read_uint8(is,(uint8_t *)x); }

// convert the next n bytes from the stream as ucs2 string
// to a utf-8 string.
//
static int ucs2_stream_string_to_utf8(hpgs_istream *is, size_t n, char *str)
{
  size_t i;

  for (i=0;i<n;i+=2)
    {
      uint16_t file_code;

      if (read_uint16(is,&file_code)) return -1;

      if (file_code < 0x80)
        *str++ = file_code;
      else if (file_code < 0x800)
        {
          *str++ = (file_code >> 6) | 0xC0;
          *str++ = (file_code & 0x3f) | 0x80;
        }
      else
        {
          *str++ = (file_code >> 12) | 0xE0;
          *str++ = ((file_code >> 6) & 0x3f) | 0x80;
          *str++ = (file_code & 0x3f) | 0x80;
        }
    }

  *str++ = 0;
  return 0;
}

static int ascii_stream_string_to_utf8(hpgs_istream *is, size_t n, char *str)
{
  size_t i;

  for (i=0;i<n;++i)
    {
      int c = hpgs_getc(is);

      if (c < 0) return -1;

      *str++ = c;
    }

  *str++ = 0;

  return 0;
}

static int read_table_entry(hpgs_istream *is, hpgs_font_table_entry *e)
{
  if (read_uint32(is,&e->tag)) return -1;
  if (read_uint32(is,&e->checksum)) return -1;
  if (read_uint32(is,&e->offset)) return -1;
  if (read_uint32(is,&e->length)) return -1;
  return 0;
}

static void init_font_cmap_data(hpgs_font_cmap_data *cmap_data)
{
  cmap_data->ranges_sz = 0;
  cmap_data->ranges = 0;
  cmap_data->glyphIndexArray_sz = 0;
  cmap_data->glyphIndexArray = 0;
}

static void cleanup_font_cmap_data(hpgs_font_cmap_data *cmap_data)
{
  if (cmap_data->ranges) free(cmap_data->ranges);
  if (cmap_data->glyphIndexArray) free(cmap_data->glyphIndexArray);
}

static void init_font_post_data(hpgs_font_post_data *post_data)
{
  memset(post_data,0,sizeof(hpgs_font_post_data));
}

static void cleanup_font_post_data(hpgs_font_post_data *post_data)
{
  if (post_data->names) free((void*)post_data->names);
  if (post_data->name_data) free((void*)post_data->name_data);
}

static void init_font_glyph_data(hpgs_font_glyph_data *glyph_data)
{
  hpgs_bbox_null(&glyph_data->bbox);
 
  glyph_data->points_sz = 0;
  glyph_data->points = 0;
  
  glyph_data->refs_sz = 0;
  glyph_data->refs_alloc_sz = 0;
  glyph_data->refs = 0;
}

static void cleanup_font_glyph_data(hpgs_font_glyph_data *glyph_data)
{
  if (glyph_data->points) free(glyph_data->points);
  if (glyph_data->refs) free(glyph_data->refs);
}

static int font_glyph_data_push_ref(hpgs_font_glyph_data *glyph_data,const hpgs_matrix *m, uint16_t gid)
{
  if (HPGS_GROW_ARRAY_MIN_SIZE(glyph_data,hpgs_font_glyph_ref,refs,refs_sz,refs_alloc_sz,8)) return -1;

  glyph_data->refs[glyph_data->refs_sz].matrix = *m;
  glyph_data->refs[glyph_data->refs_sz].gid = gid;
  ++glyph_data->refs_sz;
  return 0;
}

static unsigned cmap_find_unicode(const hpgs_font *font, int unicode)
{
  size_t i0 = 0;
  size_t i1 = font->cmap_data.ranges_sz;

  if (unicode <= 0 || unicode > 65535) return 0;

  // binary search for the endCode.
  while (i0 < i1)
    {
      size_t i = i0+(i1-i0)/2;

      if (font->cmap_data.ranges[i].endCode < unicode)
        i0 = i+1;
      else
        i1 = i;
    }

  // undefined glyph.
  if (i0 >= font->cmap_data.ranges_sz || unicode < font->cmap_data.ranges[i0].startCode)
    return 0;

  const hpgs_font_cmap_code_range *range = &font->cmap_data.ranges[i0];

  uint16_t gid;

  if (range->idRangeOffset)
      {
        size_t idx =
          (range->idRangeOffset/2 - (font->cmap_data.ranges_sz - i0)) +
          unicode - range->startCode;

        if (idx < 0 || idx >= font->cmap_data.glyphIndexArray_sz)
          return 0;

        gid = font->cmap_data.glyphIndexArray[idx];
      }
    else
      gid = unicode + range->idDelta;

  if (gid >= font->maxp_data.numGlyphs) gid = 0;
  return gid;
}


static uint32_t make_ttf_tag(const char *c)
{
  return 
    (((uint32_t)(unsigned char)(c[0]))<<24)|
    (((uint32_t)(unsigned char)(c[1]))<<16)|
    (((uint32_t)(unsigned char)(c[2]))<<8)|
    ((uint32_t)(unsigned char)(c[3]));
}

static int read_header(hpgs_font_header *header, hpgs_istream *is)
{
  int i;

  header->name_table = -1;
  header->head_table = -1;
  header->hhea_table = -1;
  header->maxp_table = -1;
  header->hmtx_table = -1;
  header->loca_table = -1;
  header->cmap_table = -1;
  header->post_table = -1;
  header->glyf_table = -1;
  header->kern_table = -1;

  if (read_int32(is,&header->version)) return -1;


  // Chaeck the version tag.
  if (header->version != 0x00010000 /* TrueType */) return -1;

  /* OpenType uses a header->version == 0x4f54544f,
     but OpenType support is out of sight now, since this
     involves interpretation of CFF compact PostScript font
     programs. */

  if (read_uint16(is,&header->numtab)) return -1;
  if (read_uint16(is,&header->searchRange)) return -1;
  if (read_uint16(is,&header->entrySel)) return -1;
  if (read_uint16(is,&header->rangeShift)) return -1;
  
  if (header->numtab > HPGS_FONT_MAX_TTF_TABLES) return -1;
      
  for (i=0;i<header->numtab;++i)
    {
      if (read_table_entry(is,&header->tables[i])) return -1;
  
      if (header->tables[i].tag == make_ttf_tag("name"))
        header->name_table = i;
      else if (header->tables[i].tag == make_ttf_tag("head"))
        header->head_table = i;
      else if (header->tables[i].tag == make_ttf_tag("hhea"))
        header->hhea_table = i;
      else if (header->tables[i].tag == make_ttf_tag("maxp"))
        header->maxp_table = i;
      else if (header->tables[i].tag == make_ttf_tag("hmtx"))
        header->hmtx_table = i;
      else if (header->tables[i].tag == make_ttf_tag("loca"))
        header->loca_table = i;
      else if (header->tables[i].tag == make_ttf_tag("cmap"))
        header->cmap_table = i;
      else if (header->tables[i].tag == make_ttf_tag("post"))
        header->post_table = i;
      else if (header->tables[i].tag == make_ttf_tag("glyf"))
        header->glyf_table = i;
      else if (header->tables[i].tag == make_ttf_tag("kern"))
        header->kern_table = i;
    }

  return 0;
}

static int seek_table(hpgs_istream *is,
                      hpgs_font_header *header, int table)
{
  if (table < 0) return -1;

  return hpgs_istream_seek(is,header->tables[table].offset);
}

static int seek_font_table(hpgs_font *font, int table)
{
  if (table < 0) return -1;

  return hpgs_istream_seek(font->is,font->header.tables[table].offset);
}

static int  seek_font_table_off(hpgs_font *font, int table, size_t off)
{
  if (table < 0) return -1;

  return hpgs_istream_seek(font->is,font->header.tables[table].offset+off);
}

static char *scan_name_table(hpgs_font_header *header, 
                             hpgs_istream *is)
{
  if (seek_table(is,header,header->name_table) < 0) return 0;

  uint16_t  format;  // Format selector (=0).
  uint16_t  count;   // Number of name records.
  uint16_t  stringOffset; //  Offset to start of string storage (from start of table).
  
  if (read_uint16(is,&format)) return 0;
  if (read_uint16(is,&count)) return 0;
  if (read_uint16(is,&stringOffset)) return 0;
  
  int i;

  for (i=0;i<(int)count;++i)
    {
      uint16_t  platformID; // Platform ID.
      uint16_t  encodingID; // Platform-specific encoding ID.
      uint16_t  languageID; // Language ID.
      uint16_t  nameID;     // Name ID.
      uint16_t  length;     // String length (in bytes).
      uint16_t  offset;     // String offset from start of storage area (in bytes).

      if (read_uint16(is,&platformID)) return 0;
      if (read_uint16(is,&encodingID)) return 0;
      if (read_uint16(is,&languageID)) return 0;
      if (read_uint16(is,&nameID)) return 0;
      if (read_uint16(is,&length)) return 0;
      if (read_uint16(is,&offset)) return 0;

      // if (nameID     == 4)
      //   hpgs_log("platformID,encodingID,languageID = %d,%d,%d.\n",
      //            (int)platformID,(int)encodingID,(int)languageID);

      // check for
      // platformID 1 (Apple)
      // encodingID 0 (Roman)
      // languageID 0 (English)
      // nameID     4 (full font name)
      //
      if (platformID == 1 &&
          encodingID == 0 &&
          languageID == 0 &&
          nameID     == 4   )
        {
          if (hpgs_istream_seek(is,
                                header->tables[header->name_table].offset+
                                (size_t)stringOffset+
                                (size_t)offset ) < 0)
            return 0;

          char *name = alloca((size_t)length * 3 + 1);

          if (ascii_stream_string_to_utf8(is,(size_t)length,name))
            return 0;

          return strdup(name);
        }

      // check for
      // platformID 0 (Unicode)
      // encodingID 3 (Unicode 2.0 BMP)
      // languageID 0 (not used for platform ID 0)
      // nameID     4 (full font name)
      //
      if (platformID == 0 &&
          encodingID == 3 &&
          languageID == 0 &&
          nameID     == 4   )
        {
          if (hpgs_istream_seek(is,
                                header->tables[header->name_table].offset+
                                (size_t)stringOffset+
                                (size_t)offset ) < 0)
            return 0;


          char *name = alloca((size_t)length * 3 + 1);

          if (ucs2_stream_string_to_utf8(is,(size_t)length,name))
            return 0;

          return strdup(name);
        }
    }

  return 0;
}

static int hpgs_scan_font_dir(const char *path)
{
  hpgs_istream *is=0;
  hpgs_font_header header;
  
#ifdef WIN32
  // go through font directory.
  char *pat =  hpgs_sprintf_malloc("%s\\*.ttf",path);

  if (!pat)
    return hpgs_set_error(hpgs_i18n("Out of memory reading font directory %s."),path);
  
  HANDLE d;
  WIN32_FIND_DATAA data;

  if ((d=FindFirstFileA(pat,&data)) == INVALID_HANDLE_VALUE)
    {
      free(pat);
      return hpgs_set_error(hpgs_i18n("Cannot open font directory %s."),path);
    }

  free(pat);

  do
    {
     char * fn = hpgs_sprintf_malloc("%s\\%s",path,data.cFileName);

      if (!fn) 
        {
          FindClose(d);
          return hpgs_set_error(hpgs_i18n("Out of memory reading font directory %s."),path);
        }

      is = hpgs_new_file_istream(fn);
      if (!is) { free(fn); continue; }

      if (read_header(&header,is) == 0)
        {
          char *name = scan_name_table(&header,is);

          if (name)
            push_font_to_directory(name,fn);
          else
            free(fn);
        }
      else
        free(fn);
      
      hpgs_istream_close(is);
      is = 0;
    }
  while (FindNextFileA(d,&data));
  
  FindClose(d);

#else
  // go through font directory.
  DIR *d;
  struct dirent *data;

  if ((d=opendir(path))==0)
    return hpgs_set_error(hpgs_i18n("Cannot open font directory %s."),path);

  while ((data=readdir(d)) != 0)
    {
      int l = strlen(data->d_name);

      if (strcmp(data->d_name+l-4,".ttf") &&
          strcmp(data->d_name+l-4,".TTF")   ) continue;

      char * fn = hpgs_sprintf_malloc("%s/%s",path,data->d_name);

      if (!fn) 
        {
          closedir (d);
          return hpgs_set_error(hpgs_i18n("Out of memory reading font directory %s."),path);
        }

      is = hpgs_new_file_istream(fn);
      if (!is) { free(fn); continue; }

      if (read_header(&header,is) == 0)
        {
          char *name = scan_name_table(&header,is);

          if (name)
            push_font_to_directory(name,fn);
          else
            free(fn);
        }
      else
        free(fn);
      
      hpgs_istream_close(is);
      is = 0;
    }

  closedir (d);
#endif
  return 0;
}

static int font_dentry_compare(const void *a, const void *b)
{
  hpgs_font_dentry *fa = (hpgs_font_dentry *)a;
  hpgs_font_dentry *fb = (hpgs_font_dentry *)b;

  return strcmp(fa->font_name,fb->font_name);
}

static int hpgs_scan_font_dirs()
{
  // already scanned ?
  hpgs_mutex_lock(&font_dir_mutex);

  if (font_directory) { hpgs_mutex_unlock(&font_dir_mutex); return 0; }

  font_directory = (hpgs_font_dentry *)malloc(sizeof(hpgs_font_dentry)*256);
  if (!font_directory)
    {
      hpgs_mutex_unlock(&font_dir_mutex);
      return hpgs_set_error(hpgs_i18n("Out of memory scanning font directories."));
    }
  font_directory_alloc_sz = 256;
  font_directory_sz = 0;

#ifdef WIN32
  const char *root = getenv("SYSTEMROOT");
  char *path =  hpgs_sprintf_malloc("%s\\fonts",root);
  int ret = hpgs_scan_font_dir(path);
  free(path);
#else

  const char *font_path = getenv("HPGS_FONT_PATH");

  if (!font_path || strlen(font_path) <= 0)
    font_path = "/usr/X11R6/lib/X11/fonts/truetype:/usr/X11R6/lib/X11/fonts/TTF:/usr/local/share/fonts:/usr/local/share/fonts/truetype:/usr/local/share/fonts/TTF";

  char *font_path_dup = hpgs_alloca(strlen(font_path));

  strcpy (font_path_dup,font_path);

  int npaths=0,ret=0;
  int nerrors = 0;

  char *fp;
  char *last_font = font_path_dup;

  for (fp = font_path_dup;;++fp)
    {
      if (*fp == ':' || *fp == 0)
        {
          hpgs_bool eos = (*fp == 0);
          if (!eos) *fp = 0;
          
          if (fp - last_font > 0)
            {
              if (hpgs_scan_font_dir(last_font))
                {
                  ++nerrors;
                  hpgs_log("hpgs_scan_font_dirs: %s\n",hpgs_get_error());
                }
              ++npaths;
            }

          if (eos) break;
          last_font = fp + 1;
        }
    }

  if (nerrors == npaths)
    ret = -1;

#endif
  qsort(font_directory,font_directory_sz,sizeof(hpgs_font_dentry),font_dentry_compare);

  hpgs_mutex_unlock(&font_dir_mutex);
  return ret;
}

static int read_head_table(hpgs_font *font)
{
  if (seek_font_table(font,font->header.head_table) < 0) return -1;

  if (read_uint32(font->is,&font->head_data.version)) return -1;
  if (read_uint32(font->is,&font->head_data.fontRevision)) return -1;
  if (read_uint32(font->is,&font->head_data.checkSumAdjustment)) return -1;
  if (read_uint32(font->is,&font->head_data.magicNumber)) return -1;
  if (read_uint16(font->is,&font->head_data.flags)) return -1;
  if (read_uint16(font->is,&font->head_data.unitsPerEm)) return -1;
  if (read_int32(font->is,&font->head_data.created_low)) return -1;
  if (read_int32(font->is,&font->head_data.created_high)) return -1;
  if (read_int32(font->is,&font->head_data.modified_low)) return -1;
  if (read_int32(font->is,&font->head_data.modified_high)) return -1;
  if (read_int16(font->is,&font->head_data.xMin)) return -1;
  if (read_int16(font->is,&font->head_data.yMin)) return -1;
  if (read_int16(font->is,&font->head_data.xMax)) return -1;
  if (read_int16(font->is,&font->head_data.yMax)) return -1;
  if (read_uint16(font->is,&font->head_data.macStyle)) return -1;
  if (read_int16(font->is,&font->head_data.lowestRecPPEM)) return -1;
  if (read_int16(font->is,&font->head_data.fontDirectionHint)) return -1;
  if (read_int16(font->is,&font->head_data.indexToLocFormat)) return -1;
  if (read_int16(font->is,&font->head_data.glyphDataFormat)) return -1;

  return 0;
}

static int read_hhea_table(hpgs_font *font)
{
  if (seek_font_table(font,font->header.hhea_table) < 0) return -1;

  if (read_uint32(font->is,&font->hhea_data.version)) return -1;
  if (read_int16(font->is,&font->hhea_data.ascent)) return -1;
  if (read_int16(font->is,&font->hhea_data.descent)) return -1;
  if (read_int16(font->is,&font->hhea_data.lineGap)) return -1;
  if (read_uint16(font->is,&font->hhea_data.advanceWidthMax)) return -1;
  if (read_int16(font->is,&font->hhea_data.minLeftSideBearing)) return -1;
  if (read_int16(font->is,&font->hhea_data.minRightSideBearing)) return -1;
  if (read_int16(font->is,&font->hhea_data.xMaxExtent)) return -1;
  if (read_int16(font->is,&font->hhea_data.caretSlopeRise)) return -1;
  if (read_int16(font->is,&font->hhea_data.caretSlopeRun)) return -1;
  if (read_int16(font->is,&font->hhea_data.caretOffset)) return -1;
  if (read_int16(font->is,&font->hhea_data.reserved1)) return -1;
  if (read_int16(font->is,&font->hhea_data.reserved2)) return -1;
  if (read_int16(font->is,&font->hhea_data.reserved3)) return -1;
  if (read_int16(font->is,&font->hhea_data.reserved4)) return -1;
  if (read_int16(font->is,&font->hhea_data.metricDataFormat)) return -1;
  if (read_uint16(font->is,&font->hhea_data.numOfLongHorMetrics)) return -1;

  return 0;
}

static int read_maxp_table(hpgs_font *font)
{
  if (seek_font_table(font,font->header.maxp_table) < 0) return -1;

  if (read_uint32(font->is,&font->maxp_data.version)) return -1;
  if (read_uint16(font->is,&font->maxp_data.numGlyphs)) return -1;
  if (read_uint16(font->is,&font->maxp_data.maxPoints)) return -1;
  if (read_uint16(font->is,&font->maxp_data.maxContours)) return -1;
  if (read_uint16(font->is,&font->maxp_data.maxComponentPoints)) return -1;
  if (read_uint16(font->is,&font->maxp_data.maxComponentContours)) return -1;
  if (read_uint16(font->is,&font->maxp_data.maxZones)) return -1;
  if (read_uint16(font->is,&font->maxp_data.maxTwilightPoints)) return -1;
  if (read_uint16(font->is,&font->maxp_data.maxStorage)) return -1;
  if (read_uint16(font->is,&font->maxp_data.maxFunctionDefs)) return -1;
  if (read_uint16(font->is,&font->maxp_data.maxInstructionDefs)) return -1;
  if (read_uint16(font->is,&font->maxp_data.maxStackElements)) return -1;
  if (read_uint16(font->is,&font->maxp_data.maxSizeOfInstructions)) return -1;
  if (read_uint16(font->is,&font->maxp_data.maxComponentElements)) return -1;
  if (read_uint16(font->is,&font->maxp_data.maxComponentDepth)) return -1;

  return 0;
}

static int read_hmtx_table(hpgs_font *font)
{
  if (font->hhea_data.numOfLongHorMetrics < 1) return -1;

  if (seek_font_table(font,font->header.hmtx_table) < 0) return -1;

  font->hmtx_data = (hpgs_font_longHorMetrics *)
    malloc(sizeof(hpgs_font_longHorMetrics)*font->maxp_data.numGlyphs);

  if (!font->hmtx_data) return -1;

  int i = 0;

  while (i<font->hhea_data.numOfLongHorMetrics)
    {
      if (read_uint16(font->is,&font->hmtx_data[i].advanceWidth)) return -1;
      if (read_int16(font->is,&font->hmtx_data[i].leftSideBearing)) return -1;
      ++i;
    }

  while (i<font->maxp_data.numGlyphs)
    {
      font->hmtx_data[i].advanceWidth =
        font->hmtx_data[font->hhea_data.numOfLongHorMetrics-1].advanceWidth;
      
      if (read_int16(font->is,&font->hmtx_data[i].leftSideBearing)) return -1;
      ++i;
    }

  return 0;
}

static int read_loca_table(hpgs_font *font)
{
  if (seek_font_table(font,font->header.loca_table) < 0) return -1;

  if (font->head_data.indexToLocFormat)
    font->loca_data_sz = font->header.tables[font->header.loca_table].length/4;
  else
    font->loca_data_sz = font->header.tables[font->header.loca_table].length/2;

  font->loca_data = (uint32_t*)malloc(sizeof(uint32_t)*font->loca_data_sz);

  if (!font->loca_data) return -1;

  size_t i;

  if (font->head_data.indexToLocFormat)
    {
      for (i=0;i<font->loca_data_sz;++i)
        if (read_uint32(font->is,&font->loca_data[i])) return -1;
    }
  else
    {
      for (i=0;i<font->loca_data_sz;++i)
        {
          uint16_t x;

          if (read_uint16(font->is,&x)) return -1;

          font->loca_data[i] = ((uint32_t)x) << 1;
        }
    }

  return 0;
}

static int kern_pair_compare(const void *a, const void *b)
{
  hpgs_font_kern_pair *ka = (hpgs_font_kern_pair *)a;
  hpgs_font_kern_pair *kb = (hpgs_font_kern_pair *)b;

  if (ka->left_gid < kb->left_gid) return -1;
  if (ka->left_gid > kb->left_gid) return 1;

  if (ka->right_gid < kb->right_gid) return -1;
  if (ka->right_gid > kb->right_gid) return 1;

  return 0;
}

static int read_kern_table(hpgs_font *font)
{
  // kerning must not be available, so this is not an error.
  if (font->header.kern_table < 0) return 0;

  if (seek_font_table(font,font->header.kern_table) < 0) return -1;

  uint16_t version; // Table version number (starts at 0)
  uint16_t nTables; // Number of subtables in the kerning table.

  if (read_uint16(font->is,&version)) return -1;
  if (read_uint16(font->is,&nTables)) return -1;

  // extra offset for seeking next subtable.
  size_t extra_off = 4;
  
  uint16_t iTable;
  
  for (iTable = 0; iTable < nTables; ++iTable)
    {
      if (iTable &&
          seek_font_table_off(font,font->header.kern_table,extra_off) < 0)
        return -1;
      
      uint16_t length; // Length of the subtable, in bytes (including this header).
      uint16_t coverage; // What type of information is contained in this table.
  
      if (read_uint16(font->is,&version))  return -1;
      if (read_uint16(font->is,&length))   return -1;
      if (read_uint16(font->is,&coverage)) return -1;

      extra_off += length;

      // we seek a table with horzonal kern data in format 0.
      if ((coverage & 0xfff3) == 0x0001)
        {
          uint16_t  nPairs; // This gives the number of kerning pairs in the table.
          uint16_t  searchRange; // The largest power of two less than or equal to the value of nPairs, multiplied by the size in bytes of an entry in the table.
          uint16_t  entrySelector; // This is calculated as log2 of the largest power of two less than or equal to the value of nPairs. This value indicates how many iterations of the search loop will have to be made. (For example, in a list of eight items, there would have to be three iterations of the loop).
          uint16_t  rangeShift; // The value of nPairs minus the largest power of two less than or equal to nPairs, and then multiplied by the size in bytes of an entry in the table.
          
          if (read_uint16(font->is,&nPairs))  return -1;
          if (read_uint16(font->is,&searchRange))  return -1;
          if (read_uint16(font->is,&entrySelector))  return -1;
          if (read_uint16(font->is,&rangeShift))  return -1;

          font->kern_data_sz = nPairs;

          font->kern_data =
            (hpgs_font_kern_pair*)malloc(sizeof(hpgs_font_kern_pair)*font->kern_data_sz);

          if (!font->kern_data) return -1;

          size_t i;

          for (i=0;i<font->kern_data_sz;++i)
            {
              int16_t value;

              if (read_uint16(font->is,&font->kern_data[i].left_gid)) return -1;
              if (read_uint16(font->is,&font->kern_data[i].right_gid)) return -1;
              if (read_int16(font->is,&value)) return -1;

              font->kern_data[i].value = (double)value/(double)font->head_data.unitsPerEm;
            }

          // That's all we can interpret.
          break;
        }
    }

  if (font->kern_data)
    qsort(font->kern_data,font->kern_data_sz,sizeof(hpgs_font_kern_pair),kern_pair_compare);

  return 0;
}

static int read_cmap_table(hpgs_font *font)
{
  if (seek_font_table(font,font->header.cmap_table) < 0) return -1;

  uint16_t dummy;
  uint16_t numberSubtables;

  if (read_uint16(font->is,&dummy)) return -1;
  if (read_uint16(font->is,&numberSubtables)) return -1;
    
  uint32_t offset = 0;

  // search type 4 subtable.
  while (numberSubtables)
    {
      uint16_t platformID;
      uint16_t platformSpecificID;
        
     if (read_uint16(font->is,&platformID)) return -1;
     if (read_uint16(font->is,&platformSpecificID)) return -1;
     if (read_uint32(font->is,&offset)) return -1;

     if (platformID == 3 && platformSpecificID == 1)
       break;

     --numberSubtables;
    }

  // no suitable subtable found.
  if (!numberSubtables) return -1;

  if (seek_font_table_off(font,font->header.cmap_table,offset) < 0)
    return -1;

  if (read_uint16(font->is,&dummy)) return -1; // version

  // character map is not of type 4.
  if (dummy != 4) return -1;

  uint16_t length;
  if (read_uint16(font->is,&length)) return -1;
    
  if (read_uint16(font->is,&dummy)) return -1; // language
  if (read_uint16(font->is,&dummy)) return -1; // segCountX2
    
  font->cmap_data.ranges_sz = dummy/2;

  font->cmap_data.ranges = (hpgs_font_cmap_code_range*)
    malloc(sizeof(hpgs_font_cmap_code_range) * font->cmap_data.ranges_sz);

  font->cmap_data.glyphIndexArray_sz =
    length - 8*2 - 4*2 * font->cmap_data.ranges_sz;

  font->cmap_data.glyphIndexArray = (uint16_t*)
    malloc(sizeof(uint16_t)*font->cmap_data.glyphIndexArray_sz);

  if (read_uint16(font->is,&dummy)) return -1; // searchRange
  if (read_uint16(font->is,&dummy)) return -1; // entrySelector
  if (read_uint16(font->is,&dummy)) return -1; // rangeShift

  size_t i;

  for (i=0; i<font->cmap_data.ranges_sz; ++i)
    if (read_uint16(font->is,&font->cmap_data.ranges[i].endCode)) return -1;
      
  if (read_uint16(font->is,&dummy)) return -1; // reservedPad

  for (i=0; i<font->cmap_data.ranges_sz; ++i)
    if (read_uint16(font->is,&font->cmap_data.ranges[i].startCode)) return -1;
    
  for (i=0; i<font->cmap_data.ranges_sz; ++i)
    if (read_uint16(font->is,&font->cmap_data.ranges[i].idDelta)) return -1;
      
  for (i=0; i<font->cmap_data.ranges_sz; ++i)
    if (read_uint16(font->is,&font->cmap_data.ranges[i].idRangeOffset)) return -1;
      
  for (i=0; i<font->cmap_data.glyphIndexArray_sz; ++i)
    if (read_uint16(font->is,&font->cmap_data.glyphIndexArray[i])) return -1;

  return 0;
}

static int read_post_table(hpgs_font *font)
{
  if (seek_font_table(font,font->header.post_table) < 0) return -1;

  if (read_int32(font->is,&font->post_data.version)) return -1;

  if (font->post_data.version != 0x00020000) return -1;

  if (read_int32(font->is,&font->post_data.italicAngle)) return -1;
  if (read_int16(font->is,&font->post_data.underlinePosition)) return -1;
  if (read_int16(font->is,&font->post_data.underlineThickness)) return -1;
  if (read_uint32(font->is,&font->post_data.isFixedPitch)) return -1;
  if (read_uint32(font->is,&font->post_data.minMemType42)) return -1;
  if (read_uint32(font->is,&font->post_data.maxMemType42)) return -1;
  if (read_uint32(font->is,&font->post_data.minMemType1)) return -1;
  if (read_uint32(font->is,&font->post_data.maxMemType1)) return -1;
  if (read_uint16(font->is,&font->post_data.numberOfGlyphs)) return -1;
  
  font->post_data.names = (const char**)malloc((size_t)font->post_data.numberOfGlyphs*sizeof(const char**));

  if (!font->post_data.names) return -1;
  
  size_t name_data_sz =
    font->header.tables[font->header.post_table].length - 34 - 2*(size_t)font->post_data.numberOfGlyphs;
  char *name_data = malloc(name_data_sz);
  
  size_t ig;
 
  uint16_t *glyphNameIndex = hpgs_alloca((size_t)font->post_data.numberOfGlyphs*sizeof(uint16_t));
  
  size_t numberNewGlyphs = 0;

  for (ig=0; ig<font->post_data.numberOfGlyphs; ++ig)
    {
      if (read_uint16(font->is,&glyphNameIndex[ig])) return -1;

      if (glyphNameIndex[ig] >= numberNewGlyphs)
        numberNewGlyphs = (size_t)glyphNameIndex[ig] + 1;
    }

  uint32_t *name_offsets = 0;

  if (numberNewGlyphs > 258)
    {
      numberNewGlyphs -= 258;

      name_offsets = hpgs_alloca(numberNewGlyphs*sizeof(uint32_t));

      size_t id = 0;
  
      for (ig=0; ig<numberNewGlyphs; ++ig)
        {
          uint8_t name_l;

          if (read_uint8(font->is,&name_l)) return -1;
     
          name_offsets[ig] = id;

          while (name_l)
            {
              if (id >= name_data_sz) return -1;
              int c;
              if ((c=hpgs_getc(font->is)) < 0)
                return -1;

              name_data[id] = c;
              ++id;
              --name_l;
            }

          if (id >= name_data_sz) return -1;
          name_data[id] = '\0';
          ++id;
        }
    }
  else
    numberNewGlyphs = 0;

  for (ig=0; ig<font->post_data.numberOfGlyphs; ++ig)
    {
      if (glyphNameIndex[ig] < 258)
        font->post_data.names[ig] = std_glyph_names[glyphNameIndex[ig]];
      else if  ((size_t)glyphNameIndex[ig] - 258 < numberNewGlyphs)
        font->post_data.names[ig] = name_data + name_offsets[(size_t)glyphNameIndex[ig] - 258];
      else
        font->post_data.names[ig] = 0;
    }
  
  font->post_data.name_data = name_data;

  return 0;
}

static int load_glyph_gid(hpgs_font *font,
                          hpgs_font_glyph_data *glyph_data,
                          uint16_t gid)
{
  // character out of range for loca data ?
  if (gid+1 >= font->loca_data_sz) return -1;

  init_font_glyph_data(glyph_data);

  // check for an empty glyph description.
  if (font->loca_data[gid+1] <= font->loca_data[gid])
    return 0;

  // seek character data in file.
  if (seek_font_table_off(font,font->header.glyf_table,
                          (size_t)font->loca_data[gid]) < 0)
    return -1;
 
  int16_t numberOfContours;
  int16_t xMin,yMin,xMax,yMax;

  if (read_int16(font->is,&numberOfContours)) return -1;
  if (read_int16(font->is,&xMin)) return -1;
  if (read_int16(font->is,&yMin)) return -1;
  if (read_int16(font->is,&xMax)) return -1;
  if (read_int16(font->is,&yMax)) return -1;
 
  glyph_data->bbox.llx = (double)xMin/(double)font->head_data.unitsPerEm;
  glyph_data->bbox.lly = (double)yMin/(double)font->head_data.unitsPerEm;
  glyph_data->bbox.urx = (double)xMax/(double)font->head_data.unitsPerEm;
  glyph_data->bbox.ury = (double)yMax/(double)font->head_data.unitsPerEm;

  // hpgs_log("bbox: %lg,%lg,%lg,%lg.\n",
  //          glyph_data->bbox.llx,glyph_data->bbox.lly,glyph_data->bbox.urx,glyph_data->bbox.ury);

   if (numberOfContours > 0)
     {
       // simple glyph description.
       uint16_t *endPtsOfContours = (uint16_t *)hpgs_alloca(numberOfContours*sizeof(uint16_t));
       
       unsigned ic;
       for (ic = 0; ic<numberOfContours; ++ic)
         {
           if (read_uint16(font->is,&endPtsOfContours[ic])) goto error;
           // hpgs_log("endPtsOfContours[%u]: %d.\n",
           //          ic,(int)endPtsOfContours[ic]);
         }
       
       glyph_data->points_sz = endPtsOfContours[numberOfContours-1]+1;
       glyph_data->points = (hpgs_font_glyph_point*)malloc(sizeof(hpgs_font_glyph_point)*glyph_data->points_sz);

       if (!glyph_data->points) goto error;

       // read instructions.
       uint16_t instructionLength;

       if (read_uint16(font->is,&instructionLength))goto error;

       uint8_t *instructions = (uint8_t *)hpgs_alloca(instructionLength);

       if (hpgs_istream_read(instructions,1,instructionLength,font->is) != instructionLength)
         goto error;

       // read and unfold flags.
       uint8_t *flags = (uint8_t *)hpgs_alloca(glyph_data->points_sz);
   
       size_t i = 0;

       while (i<glyph_data->points_sz)
         {
           if (read_uint8(font->is,&flags[i])) goto error;

           // check for repat flag
           if (flags[i] & (1<<3))
             {
               uint8_t f = flags[i];
               uint8_t repeat;
               if (read_uint8(font->is,&repeat)) goto error;

               while (repeat && i<glyph_data->points_sz)
                 {
                   --repeat;
                   ++i;
                   flags[i] = f;
                 }
             }

           ++i;
         }

       // read the x coordinates.
       int16_t *xCoordinates = (int16_t *)hpgs_alloca(glyph_data->points_sz*sizeof(int16_t));

       for (i=0;i<glyph_data->points_sz;++i)
         {
           if (flags[i] & (1<<1)) // x-Short vector
             {
               uint8_t x;

               if (read_uint8(font->is,&x)) goto error;
               
               if (flags[i] & (1<<4)) // sign of x
                 xCoordinates[i] = (int16_t)x;
               else
                 xCoordinates[i] = -(int16_t)x;
             }
           else
             {
               if (flags[i] & (1<<4)) // This x is same
                 {
                   xCoordinates[i] = 0;
                 }
               else
                 {
                   if (read_int16(font->is,&xCoordinates[i])) goto error;
                 }
             }
 
           // hpgs_log("flags,x[%u]: %2.2x,%d.\n",
           //          (unsigned)i,(unsigned)flags[i],(int)xCoordinates[i]);
          }

       // read the y coordinates.
       int16_t *yCoordinates = (int16_t *)hpgs_alloca(glyph_data->points_sz*sizeof(int16_t));

       for (i=0;i<glyph_data->points_sz;++i)
         {
           if (flags[i] & (1<<2)) // y-Short vector
             {
               uint8_t y;

               if (read_uint8(font->is,&y)) goto error;
               
               if (flags[i] & (1<<5)) // sign of y
                 yCoordinates[i] = (int16_t)y;
               else
                 yCoordinates[i] = -(int16_t)y;
             }
           else
             {
               if (flags[i] & (1<<5)) // This y is same
                 {
                   yCoordinates[i] = 0;
                 }
               else
                 {
                   if (read_int16(font->is,&yCoordinates[i])) goto error;
                 }
             }
           // hpgs_log("flags,y[%u]: %2.2x,%d.\n",
           //          (unsigned)i,(unsigned)flags[i],(int)yCoordinates[i]);
         }

       // now unfold the contour data.
       i = 0;
       
       hpgs_point p = { 0.0, 0.0 };

       for (ic = 0; ic<numberOfContours; ++ic)
         {
           int ii = 0;

           while (i <= endPtsOfContours[ic])
             {
               p.x += (double)xCoordinates[i]/(double)font->head_data.unitsPerEm;
               p.y += (double)yCoordinates[i]/(double)font->head_data.unitsPerEm;

               glyph_data->points[i].p = p;

               if (ii == 0)
                 glyph_data->points[i].flags = HPGS_FONT_GLYPH_POINT_START;
               else if (flags[i] & (1<<0)) // on curve flags
                 glyph_data->points[i].flags = HPGS_FONT_GLYPH_POINT_ONCURVE;
               else
                 glyph_data->points[i].flags = HPGS_FONT_GLYPH_POINT_CONTROL;

               // hpgs_log("p: %d,%lg,%lg.\n",
               //          glyph_data->points[i].flags,
               //          p.x*(double)font->head_data.unitsPerEm,p.y*(double)font->head_data.unitsPerEm);

               ++i;
               ++ii;
             }
         }

     }
   else if (numberOfContours < 0)
     {
       // composite glyph.
       uint16_t flags;
       uint16_t glyphIndex;
       
       hpgs_matrix m;
       
       do
         {
           if (read_uint16(font->is,&flags)) goto error;
           if (read_uint16(font->is,&glyphIndex)) goto error;

           // read offsets.
           if (flags & 1) // ARG_1_AND_2_ARE_WORDS
             {
               // offsets are 2 byte values
               int16_t dx;
               int16_t dy;
              
               if (read_int16(font->is,&dx)) goto error;
               if (read_int16(font->is,&dy)) goto error;

               // if (flags & (1 << 1)) FIXME ARGS_ARE_XY_VALUES
               m.dx = (double)dx/(double)font->head_data.unitsPerEm;
               m.dy = (double)dy/(double)font->head_data.unitsPerEm;
             }
           else
             {
               // offsets are byte values
               int8_t dx;
               int8_t dy;

               if (read_int8(font->is,&dx)) goto error;
               if (read_int8(font->is,&dy)) goto error;

               m.dx = (double)dx/(double)font->head_data.unitsPerEm;
               m.dy = (double)dy/(double)font->head_data.unitsPerEm;
             }

           // read transformation matrix.
           if (flags & (1 << 3)) // WE_HAVE_A_SCALE
             {
               int16_t scale;
               if (read_int16(font->is,&scale)) goto error;
              
               m.mxx = m.myy = (double)scale/16384.0;
               m.mxy = m.myx = 0.0;
             }
           else if (flags & (1 << 6)) // WE_HAVE_AN_X_AND_Y_SCALE
             {
               int16_t xscale;
               int16_t yscale;
               if (read_int16(font->is,&xscale)) goto error;
               if (read_int16(font->is,&yscale)) goto error;

               m.mxx = (double)xscale/16384.0;
               m.myy = (double)yscale/16384.0;
               m.mxy = m.myx = 0.0;
             }
           else if (flags & (1 << 7)) // WE_HAVE_A_TWO_BY_TWO
             {
               int16_t mxx;
               int16_t mxy;
               int16_t myx;
               int16_t myy;

               if (read_int16(font->is,&mxx)) goto error;
               if (read_int16(font->is,&mxy)) goto error;
               if (read_int16(font->is,&myx)) goto error;
               if (read_int16(font->is,&myy)) goto error;

               m.mxx = (double)mxx/16384.0;
               m.mxy = (double)mxy/16384.0;
               m.myx = (double)myx/16384.0;
               m.myy = (double)myy/16384.0;
             }
           else
             {
               m.mxx = m.myy = 1.0;
               m.mxy = m.myx = 0.0;
             }
           
           if (font_glyph_data_push_ref(glyph_data,&m,glyphIndex))
             goto error;

           // hpgs_log("ref: %d,(%lg,%lg,%lg,%lg,%lg,%lg).\n",
           //          (int)glyphIndex,
           //          m.dx,m.dy,m.mxx,m.mxy,m.myx,m.myy);
         }
       while (flags & (1 << 5)); // MORE_COMPONENTS
     }

   return 0;

 error:
  cleanup_font_glyph_data(glyph_data);
  return -1;
}

static hpgs_font_glyph_data *find_glyph_gid(hpgs_font *font,                                            
                                            unsigned gid)
{
  if (gid > font->maxp_data.numGlyphs) gid = 0;

  // cache hit ?
  if (font->glyph_cache_positions[gid] >= 0)
    return &font->glyph_cache[font->glyph_cache_positions[gid]];

  hpgs_mutex_lock(&font->mutex);

  if (font->glyph_cache_positions[gid] >= 0)
    {
      hpgs_mutex_unlock(&font->mutex);
      return &font->glyph_cache[font->glyph_cache_positions[gid]];
    }

  // try to load new glyph.
  hpgs_font_glyph_data *glyph_data =
    &font->glyph_cache[font->n_cached_glyphs];

  if (load_glyph_gid(font,glyph_data,gid) == 0)
    {
      // seek glyph in kern table.
      size_t i0 = 0;
      size_t i1 = font->kern_data_sz;

      while (i0<i1)
        {
          size_t i = i0+(i1-i0)/2;

          if (font->kern_data[i].left_gid < gid)
            i0 = i+1;
          else
            i1 = i;
        }

      if (i0 < font->kern_data_sz && font->kern_data[i0].left_gid == gid)
        {
          glyph_data->begin_kern_pair = i0;

          // find end of equal range.
          i1 = font->kern_data_sz;
          
          while (i0<i1)
            {
              size_t i = i0+(i1-i0)/2;
              
              if (font->kern_data[i].left_gid <= gid)
                i0 = i+1;
              else
                i1 = i;
            }

          glyph_data->end_kern_pair = i0;
        }
      else
        {
          glyph_data->begin_kern_pair = 0;
          glyph_data->end_kern_pair = 0;
        }

      // enter successfully read glyph into cache.
      font->glyph_cache_positions[gid] = font->n_cached_glyphs;
      ++font->n_cached_glyphs;
    }
  else
    glyph_data = 0;

  hpgs_mutex_unlock(&font->mutex);

  return glyph_data;
}

static double find_kern_value(hpgs_font *font,                                            
                              uint16_t gid,
                              uint16_t gid_n)
{
  if (!font->kern_data) return 0.0;

  hpgs_font_glyph_data *glyph_data = find_glyph_gid(font,gid);

  if (gid_n > font->maxp_data.numGlyphs) gid_n = 0;

  size_t i0 = glyph_data->begin_kern_pair;
  size_t i1 = glyph_data->end_kern_pair;
      
  while (i0<i1)
    {
      size_t i = i0+(i1-i0)/2;
      
      if (font->kern_data[i].right_gid < gid_n)
        i0 = i+1;
      else
        i1 = i;
    }
      
  if (i0 < glyph_data->end_kern_pair &&
      font->kern_data[i0].right_gid == gid_n)
    return font->kern_data[i0].value;

  return 0.0;
}

static hpgs_font *hpgs_open_font(const char *fn, const char *name)
{
  hpgs_font *ret = (hpgs_font *)malloc(sizeof(hpgs_font));

  if (!ret)
    {
      hpgs_set_error(hpgs_i18n("Out of memory allocating font structure for font %s."),name);
      return 0;
    } 

  ret->is = 0;
  ret->hmtx_data = 0;
  ret->loca_data = 0;
  ret->loca_data_sz = 0;

  ret->kern_data = 0;
  ret->kern_data_sz = 0;

  ret->glyph_cache_positions = 0;
  ret->n_cached_glyphs = 0; 
  ret->glyph_cache = 0;

  init_font_cmap_data(&ret->cmap_data);

  init_font_post_data(&ret->post_data);

  ret->is = hpgs_new_file_istream(fn);

  if (!ret->is)
    {
      hpgs_set_error(hpgs_i18n("Cannot open font %s in file %s."),name,fn);
      goto error;
    }

  if (read_header(&ret->header,ret->is))
    {
      hpgs_set_error(hpgs_i18n("Error reading header of font %s."),name);
      goto error;
    }

  if (read_head_table(ret))
    {
      hpgs_set_error(hpgs_i18n("Error reading table %s of font %s."),"head",name);
      goto error;
    }

  if (read_hhea_table(ret))
    {
      hpgs_set_error(hpgs_i18n("Error reading table %s of font %s."),"hhea",name);
      goto error;
    }

  if (read_maxp_table(ret))
    {
      hpgs_set_error(hpgs_i18n("Error reading table %s of font %s."),"maxp",name);
      goto error;
    }

  if (read_hmtx_table(ret))
    {
      hpgs_set_error(hpgs_i18n("Error reading table %s of font %s."),"hmtx",name);
      goto error;
    }

  if (read_loca_table(ret))
    {
      hpgs_set_error(hpgs_i18n("Error reading table %s of font %s."),"loca",name);
      goto error;
    }

  if (read_cmap_table(ret))
    {
      hpgs_set_error(hpgs_i18n("Error reading table %s of font %s."),"cmap",name);
      goto error;
    }
 
  if (read_post_table(ret))
    {
      hpgs_set_error(hpgs_i18n("Error reading table %s of font %s."),"post",name);
      goto error;
    }
 
  if (read_kern_table(ret))
    {
      hpgs_set_error(hpgs_i18n("Error reading table %s of font %s."),"kern",name);
      goto error;
    }

  ret->glyph_cache =
    (hpgs_font_glyph_data *)malloc(sizeof(hpgs_font_glyph_data)*ret->maxp_data.numGlyphs);

  if (!ret->glyph_cache) goto error;

  ret->glyph_cache_positions =
    (int *)malloc(sizeof(int)*ret->maxp_data.numGlyphs);

  if (!ret->glyph_cache_positions) goto error;

  int i;
  
  for (i=0;i<ret->maxp_data.numGlyphs;++i)
    ret->glyph_cache_positions[i] = -1;

  hpgs_mutex_init(&ret->mutex);
  ret->nref = 1;
  return ret;

 error:
  if (ret->glyph_cache_positions) free(ret->glyph_cache_positions);
  if (ret->glyph_cache) free(ret->glyph_cache);

  if (ret->kern_data) free(ret->kern_data);
  cleanup_font_cmap_data(&ret->cmap_data);
  cleanup_font_post_data(&ret->post_data);
  if (ret->loca_data) free(ret->loca_data);
  if (ret->hmtx_data) free(ret->hmtx_data);
  if (ret->is) hpgs_istream_close(ret->is);
  free(ret);
  return 0;
}


/*!
   Loads the given font from disk. Depending on your operating system,
   the appropriate font directories are scanned for a truetype file which
   contains the font of the given name.
   \c name is expected to be utf-8 encoded, which is almost no problem,
   since all known font names are ASCII-encoded.
 */
hpgs_font *hpgs_find_font(const char *name)
{
  // read font directory if not done yet
  if (hpgs_scan_font_dirs()) return 0;

  // binary search in font directory.
  size_t i0 = 0;
  size_t i1 = font_directory_sz;

  // binary search for the endCode.
  while (i0 < i1)
    {
      size_t i = i0+(i1-i0)/2;

      if (strcmp(font_directory[i].font_name,name) < 0)
        i0 = i+1;
      else
        i1 = i;
    }

  if (i0 >= font_directory_sz || strcmp(font_directory[i0].font_name,name))
    {
      hpgs_set_error(hpgs_i18n("Cannot find font %s."),name);
      return 0;
    }

  if (font_directory[i0].font == 0)
    {
      hpgs_mutex_lock(&font_dir_mutex);
      if (font_directory[i0].font == 0)
        font_directory[i0].font = hpgs_open_font(font_directory[i0].filename,
                                                 font_directory[i0].font_name);
      hpgs_mutex_unlock(&font_dir_mutex);
    }

  if (font_directory[i0].font == 0) return 0;
     
  hpgs_mutex_lock(&font_directory[i0].font->mutex);
  ++font_directory[i0].font->nref;
  hpgs_mutex_unlock(&font_directory[i0].font->mutex);
  return font_directory[i0].font;
}


/*!
   Frees all resources associated with this font object.
 */
void hpgs_destroy_font(hpgs_font *font)
{
  hpgs_mutex_lock(&font->mutex);
  --font->nref;
  hpgs_bool have_refs = (font->nref>0);
  hpgs_mutex_unlock(&font->mutex);

  if (have_refs) return;

  if (font->glyph_cache_positions) free(font->glyph_cache_positions);

  unsigned gid;

  for (gid=0;gid<font->n_cached_glyphs;++gid)
    cleanup_font_glyph_data(&font->glyph_cache[gid]);

  if (font->glyph_cache) free(font->glyph_cache);

  if (font->kern_data) free(font->kern_data);

  cleanup_font_cmap_data(&font->cmap_data);
  cleanup_font_post_data(&font->post_data);
  if (font->loca_data) free(font->loca_data);
  if (font->hmtx_data) free(font->hmtx_data);
  if (font->is) hpgs_istream_close(font->is);
  hpgs_mutex_destroy(&font->mutex);
  free(font);
}

/*!
   Gets the maximal ascent of the font.
 */
double hpgs_font_get_ascent(hpgs_font *font)
{
  return (double)font->hhea_data.ascent / (double)font->head_data.unitsPerEm;
}

/*!
   Gets the maximal descent of the font.
 */
double hpgs_font_get_descent(hpgs_font *font)
{
  return (double)font->hhea_data.descent / (double)font->head_data.unitsPerEm;
}

/*!
   Gets the typographical line gap of the font.
 */
double hpgs_font_get_line_gap(hpgs_font *font)
{
  return (double)font->hhea_data.lineGap / (double)font->head_data.unitsPerEm;
}

/*!
   Gets the height of capital letters above the baseline,
   computed from the size of the capital letter M.
 */
double hpgs_font_get_cap_height(hpgs_font *font)
{
  unsigned gid = cmap_find_unicode(font,'M');

  hpgs_font_glyph_data *glyph_data = find_glyph_gid(font,gid);

  if (!glyph_data)
    return hpgs_set_error(hpgs_i18n("Error reading glyph number %d from file."),(int)gid);

  return glyph_data->bbox.ury;
}

/*!
   Gets the number of glyphs in this font. This information might
   be useful, if you keep a hash map with additional information for
   the glyphs.
 */
unsigned hpgs_font_get_glyph_count(hpgs_font *font)
{
  return font->maxp_data.numGlyphs;
}

/*!
   Gets the glyph ID of the given unicode character. The
   returned glpyh id is used by several other font functions.
 */
unsigned hpgs_font_get_glyph_id(hpgs_font *font, int uc)
{
  return cmap_find_unicode(font,uc);
}

/*!
   Gets the PostScript glyph name of the specified glpyh id.
   This function retunrs a null pointer, if the glyph does not have
   a PostScript name. In this situation you can use a glyph name
   of the form \c uniXXXX, where XXXX is the upppercase 4-digit
   hexadecimal unicode of the glpyh.

   You can get a glyph id for a given unicode character
   using \c hpgs_font_get_glyph_id.

   Return -1, if an error occurrs.
 */
const char *hpgs_font_get_glyph_name(hpgs_font *font, unsigned gid)
{
  if (gid > font->post_data.numberOfGlyphs) return 0;

  return font->post_data.names[gid];
}

/*!
   Gets the bounding box of the specified glpyh id.

   You can get a glyph id for a given unicode character
   using \c hpgs_font_get_glyph_id.

   Return -1, if an error occurrs.
 */
int hpgs_font_get_glyph_bbox(hpgs_font *font, hpgs_bbox *bb, unsigned gid)
{
  hpgs_font_glyph_data *glyph_data = find_glyph_gid(font,gid);

  if (!glyph_data)
    return hpgs_set_error(hpgs_i18n("Error reading glyph number %d from file."),(int)gid);

  *bb = glyph_data->bbox;

  return 0;
}

/*!
   Gets the horizontal and vertical advance of the given glyph
   id. The unit of the returned vector is em, the nominal size
   of the letter M of the font. For almost any font one em is actually
   larger than the size of the capital letter M.  
   
   You can get a glyph id for a given unicode character
   using \c hpgs_font_get_glyph_id.

   Return -1, if an error occurrs.
 */
int hpgs_font_get_glyph_metrics(hpgs_font *font, hpgs_point *m, unsigned gid)
{
  if (gid > font->maxp_data.numGlyphs) gid = 0;

  m->x = (double)font->hmtx_data[gid].advanceWidth/(double)font->head_data.unitsPerEm;
  m->y = 0.0;

  return 0;
}

/*!
   Gets the horizontal and vertical kerning correction for the given glyph
   ids. The unit of the returned vector is em, the nominal size
   of the letter M of the font. For almost any font one em is actually
   larger than the size of the capital letter M.  
   
   You can get a glyph id for a given unicode character
   using \c hpgs_font_get_glyph_id.

   Return -1, if an error occurrs.
 */
int hpgs_font_get_kern_metrics(hpgs_font *font, hpgs_point *m, unsigned gid_l, unsigned gid_r)
{
  if (!font->kern_data) { m->y = m->x = 0.0; return 0; }

  m->x = find_kern_value(font,gid_l,gid_r);
  m->y = 0.0;

  return 0;
}

/*!
   Gets the horizontal and vertical advance of the given unicode
   string given in utf8 encoding. The unit of the returned vector is em,
   the nominal size of the letter M of the font. For almost any font
   one em is actually larger than the size of the capital letter M.  
   
   Return -1, if an error occurrs.
 */
int hpgs_font_get_utf8_metrics(hpgs_font *font, hpgs_point *m, const char *str, int strlen)
{
  m->x = 0.0;
  m->y = 0.0;

  if (strlen == 0) return 0;

  const char *s = str;

  int uc = hpgs_next_utf8(&s);
  unsigned gid = cmap_find_unicode(font,uc);

  while (uc != 0)
    {
      m->x += (double)font->hmtx_data[gid].advanceWidth/(double)font->head_data.unitsPerEm;

      if (strlen >= 0 && (s - str) >= strlen) return 0;

      // next glyph
      uc = hpgs_next_utf8(&s);

      // end of string.
      if (uc == 0) return 0;

      unsigned gid_n = cmap_find_unicode(font,uc);

      m->x += find_kern_value(font,gid,gid_n);

      gid = gid_n;
    }

  return 0;
}

// dispatch a 2nd order spline 
static int quad_to(void *ctxt,
                   hpgs_curveto_func_t curveto_func,
                   const hpgs_point *p0,
                   const hpgs_point *p1,
                   const hpgs_point *p2 )
{
  hpgs_point pm0,pm1;

  pm0.x = (1.0/3.0) * p0->x  + (2.0/3.0) * p1->x;
  pm0.y = (1.0/3.0) * p0->y  + (2.0/3.0) * p1->y;

  pm1.x = (1.0/3.0) * p2->x  + (2.0/3.0) * p1->x;
  pm1.y = (1.0/3.0) * p2->y  + (2.0/3.0) * p1->y;

  return curveto_func(ctxt,&pm0,&pm1,p2);
}

static int decompose_glyph_internal(hpgs_font *font,
                                    void *ctxt,
                                    hpgs_moveto_func_t moveto_func,
                                    hpgs_lineto_func_t lineto_func,
                                    hpgs_curveto_func_t curveto_func,
                                    const hpgs_matrix *m,
                                    unsigned gid)
{
  hpgs_font_glyph_data *glyph_data = find_glyph_gid(font,gid);

  if (!glyph_data)
    return hpgs_set_error(hpgs_i18n("Error reading glyph number %d from file."),(int)gid);

  int ret = 0;

  // Output the glpyh outline.
  if (glyph_data->points_sz)
    {
      size_t i;
      hpgs_point last_out = {0.0,0.0};
      hpgs_point start_out = {0.0,0.0};
      hpgs_point ctrl_out = {0.0,0.0};
      hpgs_font_glyph_point *start_p = 0;
      hpgs_font_glyph_point *last_p = 0;

      for (i=0;i<glyph_data->points_sz;++i)
        {
          hpgs_font_glyph_point *point = &glyph_data->points[i];

          switch(point->flags)
            {
            case HPGS_FONT_GLYPH_POINT_START:
              // close subpath.
              if (i>0 && start_p && last_p)
                {
                  switch (last_p->flags)
                    {
                    case HPGS_FONT_GLYPH_POINT_ONCURVE:
                      if (lineto_func(ctxt,&start_out) < 0)
                        return -1;
                      ++ret;
                      break;
                    case HPGS_FONT_GLYPH_POINT_CONTROL:
                      if (quad_to(ctxt,curveto_func,&last_out,&ctrl_out,&start_out) < 0)
                        return -1;
                      ++ret;
                     }
                }
              
              hpgs_matrix_xform(&start_out,m,&point->p);
              if (moveto_func(ctxt,&start_out) < 0)
                return -1;
              start_p = point;
              last_out = start_out;
              break;

            case HPGS_FONT_GLYPH_POINT_CONTROL:
              {
                hpgs_point c_out;
                hpgs_matrix_xform(&c_out,m,&point->p);
                // generate intermediate on curve point from
                // consecutive control points. 

                if (start_p && last_p && last_p->flags == HPGS_FONT_GLYPH_POINT_CONTROL)
                  {
                    hpgs_point out;
                    out.x = (ctrl_out.x + c_out.x) * 0.5;
                    out.y = (ctrl_out.y + c_out.y) * 0.5;

                    if (quad_to(ctxt,curveto_func,&last_out,&ctrl_out,&out) < 0)
                      return -1;
                  
                    last_out = out;
                  }
                ctrl_out = c_out;
              }
              break;

            case HPGS_FONT_GLYPH_POINT_ONCURVE:
              if (start_p && last_p)
                {
                  hpgs_point out;
                  hpgs_matrix_xform(&out,m,&point->p);

                  switch (last_p->flags)
                    {
                    default:
                      // HPGS_FONT_GLYPH_POINT_ONCURVE, HPGS_FONT_GLYPH_POINT_START
                      if (lineto_func(ctxt,&out) < 0)
                        return -1;
                      break;
                    case HPGS_FONT_GLYPH_POINT_CONTROL:
                      if (quad_to(ctxt,curveto_func,&last_out,&ctrl_out,&out) < 0)
                        return -1;
                    }

                  last_out = out;
                }
            }

          last_p = point;
        }
      // close last subpath.
      if (i>0 && start_p && last_p)
        {
          switch (last_p->flags)
            {
            case HPGS_FONT_GLYPH_POINT_ONCURVE:
              if (lineto_func(ctxt,&start_out) < 0)
                return -1;
              ++ret;
              break;
            case HPGS_FONT_GLYPH_POINT_CONTROL:
              if (quad_to(ctxt,curveto_func,&last_out,&ctrl_out,&start_out) < 0)
                return -1;
              ++ret;
            }
        }
    }

  // output referenced glyphs.
  if (glyph_data->refs_sz)
    {
      size_t i;

      for (i=0;i<glyph_data->refs_sz;++i)
        {
          hpgs_matrix mm;
          
          hpgs_matrix_concat(&mm,m,&glyph_data->refs[i].matrix);

          int r = decompose_glyph_internal(font,ctxt,
                                           moveto_func,lineto_func,curveto_func,
                                           &mm,glyph_data->refs[i].gid);

          if (r < 0) return -1;
          ret += r;
        }
    }

  return ret;
}

/*!
   Decomposes the outline of the given glyph id
   using the given moveto/linto/curveto functions and
   the specified transformation matrix.

   You can get a glyph id for a given unicode character
   using \c hpgs_font_get_glyph_id.

   Returns the number of outlines drawn or -1, if an error occurrs.
 */
int hpgs_font_decompose_glyph(hpgs_font *font,
                              void *ctxt,
                              hpgs_moveto_func_t moveto_func,
                              hpgs_lineto_func_t lineto_func,
                              hpgs_curveto_func_t curveto_func,
                              const hpgs_matrix *m,
                              unsigned gid)
{
  return decompose_glyph_internal(font,ctxt,
                                  moveto_func,lineto_func,curveto_func,m,gid);
}

/*!
   Draws the outline of the given glyph id
   to the given device by issuing moveto/linto/curveto/fill  operations.
   
   You can get a glyph id for a given unicode character
   using \c hpgs_font_get_glyph_id.

   Returns -1, if an error occurrs.
 */
int hpgs_font_draw_glyph(hpgs_font *font,
                         hpgs_device *device,
                         const hpgs_matrix *m,
                         unsigned gid)
{
  int r=hpgs_font_decompose_glyph(font,device,
                                  (hpgs_moveto_func_t)device->vtable->moveto,
                                  (hpgs_lineto_func_t)device->vtable->lineto,
                                  (hpgs_curveto_func_t)device->vtable->curveto,
                                  m,gid);
  if (r<0)  return -1;
  if (r==0) return 0;

  return hpgs_fill(device,HPGS_FALSE);
}

/*!
   Decomposes the outline of the given utf8-encoded unicode string
   using the given moveto/linto/curveto/fill functions and
   the specified transformation matrix.

   Returns -1, if an error occurrs.
 */
int hpgs_font_decompose_utf8(hpgs_font *font,
                             void *ctxt,
                             hpgs_moveto_func_t moveto_func,
                             hpgs_lineto_func_t lineto_func,
                             hpgs_curveto_func_t curveto_func,
                             hpgs_fill_func_t fill_func,
                             const hpgs_matrix *m,
                             const char *str, int strlen)
{
  if (strlen == 0) return 0;

  const char *s = str;

  int uc = hpgs_next_utf8(&s);
  uint16_t gid = cmap_find_unicode(font,uc);

  hpgs_matrix mm = *m;

  while (uc != 0)
    {
      int r=decompose_glyph_internal(font,ctxt,
                                     moveto_func,lineto_func,curveto_func,&mm,gid);

      if (r<0) return -1;

      if (r>0 && fill_func(ctxt,HPGS_FALSE))
        return -1;

      if (strlen >= 0 && (s - str) >= strlen) return 0;
      
      double d = (double)font->hmtx_data[gid].advanceWidth/(double)font->head_data.unitsPerEm;

      // next glyph
      uc = hpgs_next_utf8(&s);

      // end of string.
      if (uc == 0) return 0;

      uint16_t gid_n = cmap_find_unicode(font,uc);

      d += find_kern_value(font,gid,gid_n);

      mm.dx += d * mm.mxx;
      mm.dy += d * mm.myx;
      gid = gid_n;
    }

  return 0;
}

/*!
   Draws the outline of the given utf8-encoded unicode string
   to the given device by issuing moveto/linto/curveto/fill  operations.
   
   Returns -1, if an error occurrs.
 */
int hpgs_font_draw_utf8(hpgs_font *font,
                        hpgs_device *device,
                        const hpgs_matrix *m, const char *str, int strlen)
{
  return hpgs_font_decompose_utf8(font,device,
                                  (hpgs_moveto_func_t)device->vtable->moveto,
                                  (hpgs_lineto_func_t)device->vtable->lineto,
                                  (hpgs_curveto_func_t)device->vtable->curveto,
                                  (hpgs_fill_func_t)device->vtable->fill,
                                  m,str,strlen);
}
