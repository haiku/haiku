/***********************************************************************
 *                                                                     *
 * $Id: hpgsfont.h 307 2006-03-07 08:45:04Z softadm $
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
 * Private header file for ttf font support.                           *
 *                                                                     *
 ***********************************************************************/

#ifndef __HPGS_FONT_H__
#define __HPGS_FONT_H__

#include<hpgsmutex.h>
#include<stdint.h>

HPGS_INTERNAL_API void hpgs_font_init();
HPGS_INTERNAL_API void hpgs_font_cleanup();

// The max. number of ttf tables to store.
#define HPGS_FONT_MAX_TTF_TABLES 32

typedef struct hpgs_font_header_st hpgs_font_header;
typedef struct hpgs_font_dentry_st hpgs_font_dentry;
typedef struct hpgs_font_table_entry_st hpgs_font_table_entry;
typedef struct hpgs_font_head_data_st   hpgs_font_head_data;
typedef struct hpgs_font_hhea_data_st   hpgs_font_hhea_data;
typedef struct hpgs_font_maxp_data_st   hpgs_font_maxp_data;
typedef struct hpgs_font_longHorMetrics_st  hpgs_font_longHorMetrics;
typedef struct hpgs_font_cmap_data_st       hpgs_font_cmap_data;
typedef struct hpgs_font_cmap_code_range_st hpgs_font_cmap_code_range;
typedef struct hpgs_font_post_data_st       hpgs_font_post_data;
typedef struct hpgs_font_glyph_point_st hpgs_font_glyph_point;
typedef struct hpgs_font_glyph_ref_st   hpgs_font_glyph_ref;
typedef struct hpgs_font_glyph_data_st  hpgs_font_glyph_data;
typedef struct hpgs_font_kern_pair_st  hpgs_font_kern_pair;


struct hpgs_font_table_entry_st
{
  uint32_t tag;	/* Table name */
  uint32_t checksum; /* for table */
  uint32_t offset;	/* to start of table in input file */
  uint32_t length;   /* length of table */
};

struct hpgs_font_head_data_st {
  uint32_t version;
  uint32_t fontRevision;
  uint32_t checkSumAdjustment;
  uint32_t magicNumber;
  uint16_t flags;
  uint16_t unitsPerEm;
  //int64_t  created;
  //int64_t  modified;
  int32_t  created_low;
  int32_t  created_high;
  int32_t  modified_low;
  int32_t  modified_high;
  int16_t  xMin;
  int16_t  yMin;
  int16_t  xMax;
  int16_t  yMax;
  uint16_t macStyle;
  int16_t  lowestRecPPEM;
  int16_t  fontDirectionHint;
  int16_t  indexToLocFormat;
  int16_t  glyphDataFormat;
};
  
struct hpgs_font_hhea_data_st {
  uint32_t version;
  int16_t  ascent;
  int16_t  descent;
  int16_t  lineGap;
  uint16_t advanceWidthMax;
  int16_t  minLeftSideBearing;
  int16_t  minRightSideBearing;
  int16_t  xMaxExtent;
  int16_t  caretSlopeRise;
  int16_t  caretSlopeRun;
  int16_t  caretOffset;
  int16_t  reserved1;
  int16_t  reserved2;
  int16_t  reserved3;
  int16_t  reserved4;
  int16_t  metricDataFormat;
  uint16_t numOfLongHorMetrics;
};

struct hpgs_font_maxp_data_st {
  uint32_t version;
  uint16_t numGlyphs;
  uint16_t maxPoints;
  uint16_t maxContours;
  uint16_t maxComponentPoints;
  uint16_t maxComponentContours;
  uint16_t maxZones;
  uint16_t maxTwilightPoints;
  uint16_t maxStorage;
  uint16_t maxFunctionDefs;
  uint16_t maxInstructionDefs;
  uint16_t maxStackElements;
  uint16_t maxSizeOfInstructions;
  uint16_t maxComponentElements;
  uint16_t maxComponentDepth;
};

struct hpgs_font_longHorMetrics_st
{
  uint16_t advanceWidth;
  int16_t  leftSideBearing;
};

struct hpgs_font_cmap_code_range_st
{
  uint16_t startCode;
  uint16_t endCode;
  uint16_t idDelta;
  uint16_t idRangeOffset;
};

struct hpgs_font_cmap_data_st
{
  size_t ranges_sz;
  hpgs_font_cmap_code_range *ranges;

  size_t glyphIndexArray_sz;
  uint16_t *glyphIndexArray;
};

struct hpgs_font_post_data_st
{
  int32_t version; // 0x00020000 for version 2.0 
  int32_t italicAngle; // Italic angle in counter-clockwise degrees from the vertical. Zero for upright text, negative for text that leans to the right (forward).
  int16_t  underlinePosition; // This is the suggested distance of the top of the underline from the baseline (negative values indicate below baseline). 
  int16_t underlineThickness; // Suggested values for the underline thickness.
  uint32_t isFixedPitch; // Set to 0 if the font is proportionally spaced, non-zero if the font is not proportionally spaced (i.e. monospaced).
  uint32_t minMemType42; // Minimum memory usage when an OpenType font is downloaded.
  uint32_t maxMemType42; // Maximum memory usage when an OpenType font is downloaded.
  uint32_t minMemType1; // Minimum memory usage when an OpenType font is downloaded as a Type 1 font.
  uint32_t maxMemType1; // Maximum memory usage when an OpenType font is downloaded as a Type 1 font.
  uint16_t numberOfGlyphs; // Number of glyphs (this should be the same as numGlyphs in 'maxp' table).

  const char **names; // pointer to the glyph names.
  const char *name_data; // the buffer holding the actual data.
};

#define HPGS_FONT_GLYPH_POINT_START    1
#define HPGS_FONT_GLYPH_POINT_ONCURVE  2
#define HPGS_FONT_GLYPH_POINT_CONTROL  3

struct hpgs_font_glyph_point_st
{
  hpgs_point p;
  int flags; //  HPGS_FONT_GLYPH_POINT_*
};

struct hpgs_font_glyph_ref_st
{
  hpgs_matrix matrix; // transformation matrix
  uint16_t    gid;    // the references glyph id.
};

struct hpgs_font_glyph_data_st
{
  hpgs_bbox bbox;

  size_t points_sz;
  hpgs_font_glyph_point *points;

  size_t refs_sz;
  size_t refs_alloc_sz;
  hpgs_font_glyph_ref *refs;

  size_t begin_kern_pair; /*!< The index of the first kern pair where this glyph id the left one. */
  size_t end_kern_pair;/*!< The index of the first kern pair after \c begin_kern_pair where this glyph id is not the left one. */
};

/*!
    \brief A ttf kerning pair
*/
struct hpgs_font_kern_pair_st
{
  uint16_t left_gid;  /*!< The left glyph ID. */
  uint16_t right_gid; /*!< The rightt glyph ID. */

  double value; /*!< The correction of the advynce width in em. */
};


/*! \brief A truetype font header.

   This structure holds all the header information of a truetype font
   and is used when scanning a fon directory as well as a part of an 
   open font file.
*/
struct hpgs_font_header_st
{
  int32_t version;	/*!< 0x00010000 */
  uint16_t numtab;      /*!< The number truetype tables in this file. */
  uint16_t searchRange;	/*!< (Max power of 2 <= numtab) *16 */
  uint16_t entrySel;	/*!< Log2(Max power of 2 <= numtab ) */
  uint16_t rangeShift;	/*!< numtab*16 - searchRange */

  hpgs_font_table_entry tables[HPGS_FONT_MAX_TTF_TABLES]; /*!< The truetype table entries. */

  int name_table; /*!< The position of the name table. */
  int head_table; /*!< The position of the head table. */
  int hhea_table; /*!< The position of the hhea table. */
  int maxp_table; /*!< The position of the maxp table. */
  int hmtx_table; /*!< The position of the hmtx table. */
  int loca_table; /*!< The position of the loca table. */
  int cmap_table; /*!< The position of the cmap table. */
  int post_table; /*!< The position of the post table. */
  int glyf_table; /*!< The position of the glyf table. */
  int kern_table; /*!< The position of the kern table. */
};

/*! \brief A truetype font.

   This structure holds all details for operating on a truetype font
   file. The font file is kept open by the font object during it's
   lifetime, since we operate on the font file like on a database.
*/
struct hpgs_font_st
{
  hpgs_istream *is; /*!< The input stream to operate on. */

  // the file header.
  hpgs_font_header header;

  hpgs_font_head_data head_data; /*!< The head table. */
  hpgs_font_hhea_data hhea_data; /*!< The hhea table. */
  hpgs_font_maxp_data maxp_data; /*!< The maxp table. */

  hpgs_font_longHorMetrics *hmtx_data;
  /*!< The hmtx table. This array has a dimension of maxp_data.numGlyphs. */

  uint32_t *loca_data; /*!< The loca table. */
  size_t loca_data_sz; /*!< The size of the loca table. */

  hpgs_font_cmap_data cmap_data; /*!< The cmap table. */
  hpgs_font_post_data post_data; /*!< The post table. */

  hpgs_font_kern_pair *kern_data;
  size_t kern_data_sz; /*!< The size of the kern table. */

  int *glyph_cache_positions;
  /*!< The position of an individual glyph in \c glyph_cache or -1, if the glyph has not been cached yet.
       This array has a dimension of maxp_data.numGlyphs
   */

  size_t n_cached_glyphs; /*!< The number of actually cached glyphs. */
  hpgs_font_glyph_data *glyph_cache; /*!< The cached glyphs. This array has a dimension of maxp_data.numGlyphs. */

  hpgs_mutex_t mutex; /*!< The mutex to protec glyph access and reference counting. */
  size_t nref; /*!< The reference count. */
};

/*! \brief A truetype font directory entry.

   This structure holds an entry in the font directory.
*/
struct hpgs_font_dentry_st {
  
  char *font_name; /*!< The utf8-encoded name of the font. */
  char *filename; /*!< The filename of the font. */

  hpgs_font *font; /*!< The font object, if this font is cached. */
};

#endif
