
/*
 * "$Id: print-lexmark.c,v 1.160 2010/12/05 21:38:14 rlk Exp $"
 *
 *   Print plug-in Lexmark driver for the GIMP.
 *
 *   Copyright 2000 Richard Wisenoecker (richard.wisenoecker@gmx.at) and
 *      Alwin Stolk (p.a.stolk@tmx.nl)
 *
 *   The plug-in is based on the code of the CANON BJL plugin for the GIMP
 *   of Michael Sweet (mike@easysw.com) and Robert Krawitz (rlk@alum.mit.edu).
 *
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

/*
 * !!! IMPORTANT !!!  Some short information: Border and page offsets
 * are defined in 1/72 DPI. This mean, that the parameter defined at
 * lexmark_cap_t which defines positions are in 1/72 DPI. At
 * lexmark_print the unit will be changed dependent on the printer,
 * according to the value defined at lexmark_cap_t.x_raster_res and
 * lexmark_cap_t.y_raster_res. These two parameters are specifing the
 * resolution used for positioning the printer head (it is not the
 * resolution used for printing!).
 */

/* TODO-LIST
 *
 *   * implement the left border
 *
 */

/*#define DEBUG 1*/
#define USEEPSEWAVE 1

#ifdef __GNUC__
#define inline __inline__
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include <gutenprint/gutenprint-intl-internal.h>
#include "gutenprint-internal.h"
#include <string.h>
#ifdef DEBUG
#include <stdio.h>
#endif

#define STP_ECOLOR_LC 4
#define STP_ECOLOR_LM 5
#define STP_ECOLOR_LY 6

#define false 0
#define true  1

#define max(a, b) ((a > b) ? (a) : (b))
#define INCH(x)		(72 * x)

static const stp_dotsize_t single_dotsize[] =
{
  { 0x1, 1.0 }
};

static const stp_shade_t photo_dither_shades[] =
{
  { 1.0000, 1, single_dotsize },
  { 0.3333, 1, single_dotsize },
};


typedef enum Lex_model { m_lex7500,   m_z52=10052, m_z42=10042, m_3200=3200 } Lex_model;

#define NCHANNELS (7)

typedef union {			/* Offsets from the start of each line */
  unsigned long v[NCHANNELS];		/* (really pass) */
  struct {     /* IMPORTANT: order corresponds to STP_ECOLOR_* */
    unsigned long k;
    unsigned long c;
    unsigned long m;
    unsigned long y;
    unsigned long C;
    unsigned long M;
    unsigned long Y;
  } p;
} lexmark_lineoff_t;

typedef union {			/* Base pointers for each pass */
  unsigned char *v[NCHANNELS];
  struct {     /* IMPORTANT: order corresponds to STP_ECOLOR_* */
    unsigned char *k;
    unsigned char *c;
    unsigned char *m;
    unsigned char *y;
    unsigned char *C;
    unsigned char *M;
    unsigned char *Y;
  } p;
} lexmark_linebufs_t;



#ifdef DEBUG
typedef struct testdata {
  FILE *ifile;
  int x, y, cols, deep;
  char colchar[16];
  char *input_line;
} testdata;

const stp_vars_t **dbgfileprn;
int  lex_show_lcount, lex_show_length;

const stp_vars_t *lex_open_tmp_file();
const stp_vars_t *lex_write_tmp_file(const stp_vars_t *ofile, void *data,int length);
static void testprint(testdata *td);
static void readtestprintline(testdata *td, lexmark_linebufs_t *linebufs);
#endif

static void
flush_pass(stp_vars_t *v, int passno, int vertical_subpass);

/*** resolution specific parameters */
#define DPI300   0
#define DPI600   1
#define DPI1200  2
#define DPI2400  3
#define DPItest  4

#define V_NOZZLE_MASK 0x3
#define H_NOZZLE_MASK 0xc
#define NOZZLE_MASK   0xf

#define PRINT_MODE_300   0x100
#define PRINT_MODE_600   0x200
#define PRINT_MODE_1200  0x300
#define PRINT_MODE_2400  0x400

#define COLOR_MODE_K      0x1000
#define COLOR_MODE_C      0x2000
#define COLOR_MODE_Y      0x4000
#define COLOR_MODE_M      0x8000
#define COLOR_MODE_LC    0x10000
#define COLOR_MODE_LY    0x20000
#define COLOR_MODE_LM    0x40000
#define COLOR_MODE_CMYK   (COLOR_MODE_C | COLOR_MODE_M | COLOR_MODE_Y | COLOR_MODE_K)
#define COLOR_MODE_CMY    (COLOR_MODE_C | COLOR_MODE_M | COLOR_MODE_Y)
#define COLOR_MODE_CcMcYK (COLOR_MODE_C | COLOR_MODE_LC | COLOR_MODE_M | COLOR_MODE_LM | COLOR_MODE_Y | COLOR_MODE_K)
#define COLOR_MODE_CcMcY  (COLOR_MODE_C | COLOR_MODE_LC | COLOR_MODE_M | COLOR_MODE_LM | COLOR_MODE_Y)

#define COLOR_MODE_MASK  0x7f000
#define PRINT_MODE_MASK    0xf00
#define COLOR_MODE_PHOTO (COLOR_MODE_LC | COLOR_MODE_LM)

#define BWR      0
#define BWL      1
#define CR       2
#define CL       3


static const char standard_sat_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* B */
/* B */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* M */
/* M */  "1.00 0.95 0.90 0.90 0.90 0.90 0.90 0.90 "  /* R */
/* R */  "0.90 0.95 0.95 1.00 1.00 1.00 1.00 1.00 "  /* Y */
/* Y */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* G */
/* G */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

static const char standard_lum_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "0.65 0.67 0.70 0.72 0.77 0.80 0.82 0.85 "  /* B */
/* B */  "0.87 0.86 0.82 0.79 0.79 0.82 0.85 0.88 "  /* M */
/* M */  "0.92 0.95 0.96 0.97 0.97 0.97 0.96 0.96 "  /* R */
/* R */  "0.96 0.97 0.97 0.98 0.99 1.00 1.00 1.00 "  /* Y */
/* Y */  "1.00 0.97 0.95 0.94 0.93 0.92 0.90 0.86 "  /* G */
/* G */  "0.79 0.76 0.71 0.68 0.68 0.68 0.68 0.66 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

static const char standard_hue_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"-6\" upper-bound=\"6\">\n"
/* C */  "0.00 0.06 0.10 0.10 0.06 -.01 -.09 -.17 "  /* B */
/* B */  "-.25 -.33 -.38 -.38 -.36 -.34 -.34 -.34 "  /* M */
/* M */  "-.34 -.34 -.36 -.40 -.50 -.40 -.30 -.20 "  /* R */
/* R */  "-.12 -.07 -.04 -.02 0.00 0.00 0.00 0.00 "  /* Y */
/* Y */  "0.00 0.00 0.00 -.05 -.10 -.15 -.22 -.24 "  /* G */
/* G */  "-.26 -.30 -.33 -.28 -.25 -.20 -.13 -.06 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";


/* Codes for possible ink-tank combinations.
 * Each combo is represented by the colors that can be used with
 * the installed ink-tank(s)
 * Combinations of the codes represent the combinations allowed for a model
 */
#define LEXMARK_INK_K           1
#define LEXMARK_INK_CMY         2
#define LEXMARK_INK_CMYK        4
#define LEXMARK_INK_CcMmYK      8
#define LEXMARK_INK_CcMmYy     16
#define LEXMARK_INK_CcMmYyK    32

#define LEXMARK_INK_BLACK_MASK (LEXMARK_INK_K|LEXMARK_INK_CMYK|\
                              LEXMARK_INK_CcMmYK|LEXMARK_INK_CcMmYyK)

#define LEXMARK_INK_PHOTO_MASK (LEXMARK_INK_CcMmYy|LEXMARK_INK_CcMmYK|\
                              LEXMARK_INK_CcMmYyK)

/* document feeding */
#define LEXMARK_SLOT_ASF1    1
#define LEXMARK_SLOT_ASF2    2
#define LEXMARK_SLOT_MAN1    4
#define LEXMARK_SLOT_MAN2    8

/* model peculiarities */
#define LEXMARK_CAP_DMT       1<<0    /* Drop Modulation Technology */
#define LEXMARK_CAP_MSB_FIRST 1<<1    /* how to send data           */
#define LEXMARK_CAP_CMD61     1<<2    /* uses command #0x61         */
#define LEXMARK_CAP_CMD6d     1<<3    /* uses command #0x6d         */
#define LEXMARK_CAP_CMD70     1<<4    /* uses command #0x70         */
#define LEXMARK_CAP_CMD72     1<<5    /* uses command #0x72         */


static const int lr_shift_color[10] = { 9, 18, 2*18 }; /* vertical distance between ever 2nd  inkjet (related to resolution) */
static const int lr_shift_black[10] = { 9, 18, 2*18 }; /* vertical distance between ever 2nd  inkjet (related to resolution) */

static const stp_parameter_t the_parameters[] =
{
  {
    "PageSize", N_("Page Size"), "Color=No,Category=Basic Printer Setup",
    N_("Size of the paper being printed to"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "MediaType", N_("Media Type"), "Color=Yes,Category=Basic Printer Setup",
    N_("Type of media (plain paper, photo paper, etc.)"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "InputSlot", N_("Media Source"), "Color=No,Category=Basic Printer Setup",
    N_("Source (input slot) of the media"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Resolution", N_("Resolution"), "Color=Yes,Category=Basic Printer Setup",
    N_("Resolution and quality of the print"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "InkType", N_("Ink Type"), "Color=Yes,Category=Advanced Printer Setup",
    N_("Type of ink in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "InkChannels", N_("Ink Channels"), "Color=Yes,Category=Advanced Printer Functionality",
    N_("Ink Channels"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_INTERNAL, 0, 0, STP_CHANNEL_NONE, 0, 0
  },
  {
    "PrintingMode", N_("Printing Mode"), "Color=Yes,Category=Core Parameter",
    N_("Printing Output Mode"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
};

static const int the_parameter_count =
sizeof(the_parameters) / sizeof(const stp_parameter_t);

typedef struct
{
  const stp_parameter_t param;
  double min;
  double max;
  double defval;
  int color_only;
} float_param_t;

static const float_param_t float_parameters[] =
{
  {
    {
      "CyanDensity", N_("Cyan Density"), "Color=Yes,Category=Output Level Adjustment",
      N_("Adjust the cyan density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 1, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "MagentaDensity", N_("Magenta Density"), "Color=Yes,Category=Output Level Adjustment",
      N_("Adjust the magenta density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 2, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "YellowDensity", N_("Yellow Density"), "Color=Yes,Category=Output Level Adjustment",
      N_("Adjust the yellow density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 3, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "BlackDensity", N_("Black Density"), "Color=Yes,Category=Output Level Adjustment",
      N_("Adjust the black density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 0, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "LightCyanTrans", N_("Light Cyan Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Light Cyan Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "LightMagentaTrans", N_("Light Magenta Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Light Magenta Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
};    

static const int float_parameter_count =
sizeof(float_parameters) / sizeof(const float_param_t);

/* returns the offset of the first jet when printing in the other direction */
static int get_lr_shift(int mode)
{

  const int *ptr_lr_shift;

      /* K could only be present if black is printed only. */
  if((mode & COLOR_MODE_K) == (mode & COLOR_MODE_MASK)) {
    ptr_lr_shift = lr_shift_black;
  } else {
    ptr_lr_shift = lr_shift_color;
  }

      switch(mode & PRINT_MODE_MASK) 	{
	case PRINT_MODE_300:
	  return ptr_lr_shift[0];
	  break;
	case PRINT_MODE_600:
	  return ptr_lr_shift[1];
	  break;
	case PRINT_MODE_1200:
	  return ptr_lr_shift[2];
	  break;
	case PRINT_MODE_2400:
	  return ptr_lr_shift[2];
	  break;
      }
      return 0;
}


/*
 * head offsets for z52:
 *
 *      black       black          color         photo
 *    cartridge   cartridge      cartridge     cartridge
 *      mode I     mode II
 *
 *              v                 +-----+ --    +-----+ ---
 * --- +-----+ ---             v  |     | ^     |     |  ^
 *  ^  |     | --- +-----+ --- -- |  C  | 64    | LC  |  |
 *  |  |     |  ^  |     |  ^  40 |     | v  v  |     |  |
 *  |  |     |     |     |  |  -- +-----+ -- -- +-----+  |
 *  |  |     |     |     |  |  ^             28          |
 *  |  |     |     |     |  |     +-----+ -- -- +-----+  |
 *     |     |     |     |  |     |     | ^  ^  |     |  |
 * 208 |  K  |     |  K  | 192    |  M  | 64    | LM  | 240
 *     |     |     |     |  |     |     | v  v  |     |  |
 *  |  |     |     |     |  |     +-----+ -- -- +-----+  |
 *  |  |     |     |     |  |  v             28          |
 *  |  |     |     |     |  |  -- +-----+ -- -- +-----+  |
 *  |  |     |     |     |  v  40 |     | ^  ^  |     |  |
 *  v  |     |     +-----+ --- -- |  Y  | 64    |  K  |  |
 * --- +-----+                 ^  |     | v     |     |  v
 *                                +-----+ --    +-----+ ---
 *
 */

static const int head_offset_cmyk[] =
{70, 368, 184, 0, 368, 184, 0};  /* k, m, c, y, M, C, Y */
/* the head_offset_cmy is needed because the dithering code is going into troubles if there is an offset different from 0 for the unused black color */
static const int head_offset_cmy[] =
{0, 368, 184, 0, 368, 184, 0};  /* k, m, c, y, M, C, Y */
static const int head_offset_cCmMyk[] =
{0, 368, 184, 0, 368, 184, 0};  /* k, m, c, y, M, C, Y */




/**************************************************************************/
/**** Data structures which are describing printer specific parameters ****/

/* resolution specific parameters (substructure of lexmark_cap_t) */
typedef struct {
  const char *name;
  const char *text;
  int hres;
  int vres;
  int softweave;
  int vertical_passes;
  int vertical_oversample;
  int unidirectional;      /* print bi/unidirectional */
  int resid;               /* resolution id */
} lexmark_res_t;

#define LEXM_RES_COUNT 30
typedef lexmark_res_t lexmark_res_t_array[LEXM_RES_COUNT];


/* ink type parameters (substructure of lexmark_cap_t) */
typedef struct {
  int ncolors;
  unsigned int used_colors; /* specifies the head colors to be used (e.g. COLOR_MODE_K */
  unsigned int pass_length; /* avaliable jets for one color */
  int v_top_head_offset;    /* offset from top, wehere the first jet will be found */
  int h_catridge_offset;    /* horizontal offset of cartridges */
  int h_direction_offset;   /* Offset when printing in the other direction */
  const int *head_offset;   /* specifies the offset of head colors */
} lexmark_inkparam_t;

typedef struct
{
  const char *name;
  const char *text;
  lexmark_inkparam_t ink_parameter[2];
} lexmark_inkname_t;


/* main structure which describes all printer specific parameters */
typedef struct {
  Lex_model model;    /* printer model */
  int max_paper_width;  /* maximum printable paper size in 1/72 inch */
  int max_paper_height;
  int min_paper_width;  /* Maximum paper width, in points */
  int min_paper_height; /* Maximum paper height, in points */
  int max_xdpi;
  int max_ydpi;
  int max_quality;
  int border_left;    /* unit is 72 DPI */
  int border_right;
  int border_top;
  int border_bottom;
  int inks;           /* installable cartridges (LEXMARK_INK_*) */
  int slots;          /* available paperslots */
  int features;       /* special bjl settings */
  /*** printer internal parameters ***/
  /* the unit of the following parameters is identical with max phys unit of the printer */
  int offset_left_border;      /* Offset to the left paper border (== border_left=0) */
  int offset_top_border;       /* Offset to the top paper border (== border_top=0) */
  int x_raster_res;            /* horizontal resolution for positioning of the printer head in DPI */
  int y_raster_res;            /* vertical   resolution for positioning of the printer head in DPI */
  const lexmark_res_t_array *res_parameters; /* resolution specific parameters; last entry has resid = -1 */
  const lexmark_inkname_t *ink_types;  /* type of supported inks */
  const char *lum_adjustment;
  const char *hue_adjustment;
  const char *sat_adjustment;
} lexmark_cap_t;


/*****************************************************************/
/**** initialize printer specific data structures ****/

/*
 * z52 specific parameters
 */
#define LX_Z52_300_DPI  1
#define LX_Z52_600_DPI  3
#define LX_Z52_1200_DPI 4
#define LX_Z52_2400_DPI 5

#define LX_Z52_COLOR_PRINT 0
#define LX_Z52_BLACK_PRINT 1

#define LX_PSHIFT                   0x13
#define LX_Z52_COLOR_MODE_POS       0x9
#define LX_Z52_RESOLUTION_POS       0x7
#define LX_Z52_PRINT_DIRECTION_POS  0x8

/*static const int IDX_Z52ID =2;*/
static const int IDX_SEQLEN=3;

/*
   head:
     1 .. black,
     0 .. color

   resolution:
     1 .. 300 dpi (for black ?)
     2 .. like 1
     3 .. 600 dpi (color&black)
     4 .. 1200 dpi
     5 .. ? like 1
*/

#define LXM_Z52_HEADERSIZE 34
static const unsigned char outbufHeader_z52[LXM_Z52_HEADERSIZE]=
{
  0x1B,0x2A,0x24,0x00,0x00,0xFF,0xFF,         /* number of packets ----     vvvvvvvvv */
  0x01,0x01,0x01,0x1a,0x03,0x01,              /* 0x7-0xc: resolution, direction, head */
  0x03,0x60,                                  /* 0xd-0xe HE */
  0x04,0xe0,                                  /* 0xf-0x10  HS vertical pos */
  0x19,0x5c,                                  /* 0x11-0x12 */
  0x0,0x0,                                    /* 0x13-0x14  VO between packges*/
  0x0,0x80,                                   /* 0x15-0x16 */
  0x0,0x0,0x0,0x0,0x1,0x2,0x0,0x0,0x0,0x0,0x0 /* 0x17-0x21 */
};

#define LXM_Z42_HEADERSIZE 34
static const unsigned char outbufHeader_z42[LXM_Z42_HEADERSIZE]=
{
  0x1B,0x2A,0x24,0x00,0x00,0x00,0x00,
  0x01,0x01,0x01,0x18,0x00,0x01,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00
};


static const lexmark_res_t_array lexmark_reslist_z52 =  /* LEXM_RES_COUNT entries are allowed !! */
{
  /*     name                                                    hres vres softw v_pass overs unidir resid */
  { "300x600dpi",     N_ ("300 DPI x 600 DPI"),	                 300,  600,  0,    1,    1,    0,    DPI300 },
  { "600dpi",	      N_ ("600 DPI"),		      	         600,  600,  0,    1,    1,    0,    DPI600 },
  { "600hq",	      N_ ("600 DPI high quality"),	         600,  600,  1,    4,    1,    0,    DPI600 },
  { "600uni",	      N_ ("600 DPI Unidirectional"),	         600,  600,  0,    1,    1,    1,    DPI600 },
  { "1200dpi",	      N_ ("1200 DPI"),		                1200, 1200,  1,    1,    1,    0,    DPI1200},
  { "1200hq",	      N_ ("1200 DPI high quality"),             1200, 1200,  1,    1,    1,    0,    DPI300 },
  { "1200hq2",	      N_ ("1200 DPI highest quality"),          1200, 1200,  1,    1,    1,    0,    DPI600 },
  { "1200uni",	      N_ ("1200 DPI  Unidirectional"),          1200, 1200,  0,    1,    1,    1,    DPI1200},
  { "2400x1200dpi",   N_ ("2400 DPI x 1200 DPI"),	        2400, 1200,  1,    1,    1,    0,    DPI1200},
  { "2400x1200hq",    N_ ("2400 DPI x 1200 DPI high quality"),  2400, 1200,  1,    1,    1,    0,    DPI600 },
  { "2400x1200hq2",   N_ ("2400 DPI x 1200 DPI highest quality"),2400, 1200,  1,    1,    1,    0,    DPI300},
#ifdef DEBUG
  { "testprint",      N_ ("test print"),                        1200, 1200,  1,    1,    1,    0,    DPItest},
#endif
  { "",			"", 0, 0, 0, 0, 0, -1 }
};


static const lexmark_inkname_t ink_types_z52[] =
{
  /*   output_type   ncolors used_colors   pass_length  v_top_head_offset
   *                                                        h_catridge_offset
   *                                                           h_direction_offset
   *                                                               head_offset */
  { "CMYK",     N_("Four Color Standard"),
    {{ 1, COLOR_MODE_K,        208, 324, 0, 10, head_offset_cmyk },
     { 4, COLOR_MODE_CMYK,   192/3,   0, 0, 10, head_offset_cmyk }}},
  { "RGB",      N_("Three Color Composite"),
    {{ 1, COLOR_MODE_K,        208, 324, 0, 10, head_offset_cmyk },  /* we ignor CMY, use black */
     { 4, COLOR_MODE_CMY,    192/3,   0, 0, 10, head_offset_cmy }}},
  { "PhotoCMYK", N_("Six Color Photo"),
    {{ 1, COLOR_MODE_K,      192/3,   0, 0, 10, head_offset_cCmMyk },
     { 6, COLOR_MODE_CcMcYK, 192/3,   0, 0, 10, head_offset_cCmMyk }}},
  { "PhotoCMY", N_("Five Color Photo Composite"),
    {{ 1, COLOR_MODE_K,        208, 324, 0, 10, head_offset_cCmMyk },
     { 5, COLOR_MODE_CcMcY,  192/3,   0, 0, 10, head_offset_cCmMyk }}}, /* we ignor CMY, use black */
  { "Gray",     N_("Black"),
    {{ 1, COLOR_MODE_K,        208, 324, 0, 10, head_offset_cmyk },
     { 1, COLOR_MODE_K,        208, 324, 0, 10, head_offset_cmyk }}},
  { NULL, NULL }
};



/*
 * 3200 sepecific stuff
 */
#define LXM3200_LEFTOFFS 6254
#define LXM3200_RIGHTOFFS (LXM3200_LEFTOFFS-2120)

static int lxm3200_headpos = 0;
static int lxm3200_linetoeject = 0;

#define LXM_3200_HEADERSIZE 24
static const char outbufHeader_3200[LXM_3200_HEADERSIZE] =
{
  0x1b, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x1b, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x1b, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static inline int
lexmark_calc_3200_checksum(unsigned char *data)
{
  int ck, i;

  ck = 0;
  for(i=1; i<7; i++)ck += data[i];

  return(ck & 255);
}


static const lexmark_res_t_array lexmark_reslist_3200 =   /* LEXM_RES_COUNT entries are allowed !! */
{
  /*     name                                                    hres vres softw v_pass overs unidir resid */
  { "300x600dpi",     N_ ("300 DPI x 600 DPI"),	                 300,  600,  0,    1,    1,    0,    DPI300 },
  { "600dpi",	      N_ ("600 DPI"),		      	         600,  600,  0,    1,    1,    0,    DPI600 },
  { "600hq",	      N_ ("600 DPI high quality"),	         600,  600,  1,    4,    1,    0,    DPI600 },
  { "600uni",	      N_ ("600 DPI Unidirectional"),	         600,  600,  0,    1,    1,    1,    DPI600 },
  { "1200dpi",	      N_ ("1200 DPI"),		                1200, 1200,  1,    1,    1,    0,    DPI1200},
  { "1200hq",	      N_ ("1200 DPI high quality"),             1200, 1200,  1,    1,    1,    0,    DPI300 },
  { "1200hq2",	      N_ ("1200 DPI highest quality"),          1200, 1200,  1,    1,    1,    0,    DPI600 },
  { "1200uni",	      N_ ("1200 DPI  Unidirectional"),          1200, 1200,  0,    1,    1,    1,    DPI1200},
  { "",			"", 0, 0, 0, 0, 0, -1 }
};


static const lexmark_inkname_t ink_types_3200[] =
{
  /*   output_type   ncolors used_colors   pass_length  v_top_head_offset
   *                                                        h_catridge_offset
   *                                                           h_direction_offset
   *                                                               head_offset */
  { "CMYK",     N_("Four Color Standard"),
    {{ 1, COLOR_MODE_K,        208,  20, 0, 12, head_offset_cmyk },
     { 4, COLOR_MODE_CMYK,   192/3,   0, 0, 12, head_offset_cmyk }}},
  { "RGB",      N_("Three Color Composite"),
    {{ 1, COLOR_MODE_K,        208,  20, 0, 12, head_offset_cmyk },  /* we ignor CMY, use black */
     { 4, COLOR_MODE_CMY,    192/3,   0, 0, 12, head_offset_cmy }}},
  { "PhotoCMYK", N_("Six Color Photo"),
    {{ 1, COLOR_MODE_K,      192/3,   0, 0, 12, head_offset_cCmMyk },
     { 6, COLOR_MODE_CcMcYK, 192/3,   0, 0, 12, head_offset_cCmMyk }}},
  { "PhotoCMY", N_("Five Color Photo Composite"),
    {{ 1, COLOR_MODE_K,        208,  20, 0, 12, head_offset_cCmMyk }, /* we ignor CMY, use black */
     { 5, COLOR_MODE_CcMcY,  192/3,   0, 0, 12, head_offset_cCmMyk }}},
  { NULL, NULL }
};





/* main structure */
static const lexmark_cap_t lexmark_model_capabilities[] =
{
  /* default settings for unkown models */

  {   (Lex_model)-1, 8*72,11*72,180,180,20,20,20,20, LEXMARK_INK_K, LEXMARK_SLOT_ASF1, 0 },

  /* tested models */

  { /* Lexmark z52 */
    m_z52,
    618, 936,         /* max paper size *//* 8.58" x 13 " */
    INCH(2), INCH(4), /* min paper size */
    2400, 1200, 2, /* max resolution */
    0, 0, 5, 15, /* 15 36 border l,r,t,b    unit is 1/72 DPI */
    LEXMARK_INK_CMY | LEXMARK_INK_CMYK | LEXMARK_INK_CcMmYK,
    LEXMARK_SLOT_ASF1 | LEXMARK_SLOT_MAN1,
    LEXMARK_CAP_DMT,
    /*** printer internal parameters ***/
    20,        /* real left paper border */
    123,       /* real top paper border */
    2400,      /* horizontal resolution of 2400 dpi for positioning */
    1200,      /* use a vertical resolution of 1200 dpi for positioning */
    &lexmark_reslist_z52,  /* resolution specific parameters of z52 */
    ink_types_z52,  /* supported inks */
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  { /* Lexmark z42 */
    m_z42,
    618, 936,         /* max paper size *//* 8.58" x 13 " */
    INCH(2), INCH(4), /* min paper size */
    2400, 1200, 2, /* max resolution */
    0, 0, 5, 41, /* border l,r,t,b    unit is 1/72 DPI */
    LEXMARK_INK_CMY | LEXMARK_INK_CMYK | LEXMARK_INK_CcMmYK,
    LEXMARK_SLOT_ASF1 | LEXMARK_SLOT_MAN1,
    LEXMARK_CAP_DMT,
    /*** printer internal parameters ***/
    20,        /* real left paper border */
    123,       /* real top paper border */
    2400,      /* horizontal resolution of 2400 dpi for positioning */
    1200,      /* use a vertical resolution of 1200 dpi for positioning */
    &lexmark_reslist_z52,  /* resolution specific parameters of z52 */
    ink_types_z52,  /* supported inks */
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  { /* Lexmark 3200 */
    m_3200,
    618, 936,      /* 8.58" x 13 " */
    INCH(2), INCH(4), /* min paper size */
    1200, 1200, 2,
    11, 9, 10, 18,
    LEXMARK_INK_CMYK | LEXMARK_INK_CcMmYK,
    LEXMARK_SLOT_ASF1 | LEXMARK_SLOT_MAN1,
    LEXMARK_CAP_DMT,
    /*** printer internal parameters ***/
    0,         /* real left paper border */
    300,       /* real top paper border */
    1200,      /* horizontal resolution of ?? dpi for positioning */
    1200,      /* use a vertical resolution of 1200 dpi for positioning */
    &lexmark_reslist_3200,  /* resolution specific parameters of 3200 */
    ink_types_3200,  /* supported inks */
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  { /*  */
    m_lex7500,
    618, 936,      /* 8.58" x 13 " */
    INCH(2), INCH(4), /* min paper size */
    2400, 1200, 2,
    11, 9, 10, 18,
    LEXMARK_INK_CMY | LEXMARK_INK_CMYK | LEXMARK_INK_CcMmYK,
    LEXMARK_SLOT_ASF1 | LEXMARK_SLOT_MAN1,
    LEXMARK_CAP_DMT,
    /*** printer internal parameters ***/
    0,         /* real left paper border */
    300,       /* real top paper border */
    1200,      /* horizontal resolutio of ??? dpi for positioning */
    1200,      /* use a vertical resolution of 1200 dpi for positioning */
    &lexmark_reslist_3200,  /* resolution specific parameters of ?? */
    ink_types_3200,  /* supported inks */
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
};





typedef struct lexm_privdata_weave {
  const lexmark_inkparam_t *ink_parameter;
  int           bidirectional; /* tells us if we are allowed to print bidirectional */
  int           direction;     /* stores the last direction or print head */
  int		hoffset;
  int model;
  int width;
  int ydpi;
  int xdpi;
  int physical_xdpi;
  int last_pass_offset;
  int jets;
  int bitwidth;
  int ncolors;
  int horizontal_weave;
  unsigned char *outbuf;
} lexm_privdata_weave;


/*
 * internal functions
 */
static int model_to_index(int model)
{
  int i;
  int models= sizeof(lexmark_model_capabilities) / sizeof(lexmark_cap_t);
  for (i=0; i<models; i++) {
    if (lexmark_model_capabilities[i].model == model) {
      return i;
    }
  }
  return -1;
}


static const lexmark_cap_t *
lexmark_get_model_capabilities(int model)
{
  int i = model_to_index(model);

  if (i != -1) {
    return &(lexmark_model_capabilities[i]);
  }
  stp_deprintf(STP_DBG_LEXMARK,
	       "lexmark: model %d not found in capabilities list.\n",model);
  return &(lexmark_model_capabilities[0]);
}



typedef struct
{
  const char *name;
  const char *text;
  int paper_feed_sequence;
  int platen_gap;
  double base_density;
  double k_lower_scale;
  double k_upper;
  double cyan;
  double magenta;
  double yellow;
  double p_cyan;
  double p_magenta;
  double p_yellow;
  double saturation;
  double gamma;
  int feed_adjustment;
  int vacuum_intensity;
  int paper_thickness;
  const char *hue_adjustment;
  const char *lum_adjustment;
  const char *sat_adjustment;
} paper_t;



static const paper_t lexmark_paper_list[] =
{
  { "Plain", N_("Plain Paper"),
    1, 0, 0.80, .1, .5, 1.0, 1.0, 1.0, .9, 1.05, 1.15,
    1, 1.0, 0x6b, 0x1a, 0x01,
    standard_hue_adjustment, standard_lum_adjustment, standard_sat_adjustment},
  { "GlossyFilm", N_("Glossy Film"),
    3, 0, 1.00 ,1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6d, 0x00, 0x01,
    standard_hue_adjustment, standard_lum_adjustment, standard_sat_adjustment},
  { "Transparency", N_("Transparencies"),
    3, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x6d, 0x00, 0x02,
    standard_hue_adjustment, standard_lum_adjustment, standard_sat_adjustment},
  { "Envelope", N_("Envelopes"),
    4, 0, 0.80, .125, .5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01,
    standard_hue_adjustment, standard_lum_adjustment, standard_sat_adjustment},
  { "Matte", N_("Matte Paper"),
    7, 0, 0.85, 1.0, .999, 1.05, .9, 1.05, .9, 1.0, 1.1,
    1, 1.0, 0x00, 0x00, 0x02,
    standard_hue_adjustment, standard_sat_adjustment, standard_sat_adjustment},
  { "Inkjet", N_("Inkjet Paper"),
    7, 0, 0.85, .25, .6, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01,
    standard_hue_adjustment, standard_lum_adjustment, standard_sat_adjustment},
  { "Coated", N_("Photo Quality Inkjet Paper"),
    7, 0, 1.00, 1.0, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01,
    standard_hue_adjustment, standard_lum_adjustment, standard_sat_adjustment},
  { "Photo", N_("Photo Paper"),
    8, 0, 1.00, 1.0, .9, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x67, 0x00, 0x02,
    standard_hue_adjustment, standard_lum_adjustment, standard_sat_adjustment},
  { "GlossyPhoto", N_("Premium Glossy Photo Paper"),
    8, 0, 1.10, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.03, 1.0,
    1, 1.0, 0x80, 0x00, 0x02,
    standard_hue_adjustment, standard_lum_adjustment, standard_sat_adjustment},
  { "Luster", N_("Premium Luster Photo Paper"),
    8, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x80, 0x00, 0x02,
    standard_hue_adjustment, standard_lum_adjustment, standard_sat_adjustment},
  { "GlossyPaper", N_("Photo Quality Glossy Paper"),
    6, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x6b, 0x1a, 0x01,
    standard_hue_adjustment, standard_lum_adjustment, standard_sat_adjustment},
  { "Ilford", N_("Ilford Heavy Paper"),
    8, 0, .85, .5, 1.35, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x80, 0x00, 0x02,
    standard_hue_adjustment, standard_lum_adjustment, standard_sat_adjustment},
  { "Other", N_("Other"),
    0, 0, 0.80, 0.125, .5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01,
    standard_hue_adjustment, standard_lum_adjustment, standard_sat_adjustment},
};

static const int paper_type_count = sizeof(lexmark_paper_list) / sizeof(paper_t);

static const lexmark_inkname_t *
lexmark_get_ink_type(const char *name, int printing_color, const lexmark_cap_t * caps)
{
  int i = 0;
  const lexmark_inkname_t *ink_type = caps->ink_types;

  if (name)
    for (i=0; ((ink_type[i].name != NULL) &&
	       (strcmp(name, ink_type[i].name)  != 0)); i++) ;
  return &(ink_type[i]);

}

static const lexmark_inkparam_t *
lexmark_get_ink_parameter(const char *name, int printing_color, const lexmark_cap_t * caps, const stp_vars_t *nv)
{
  const lexmark_inkname_t *ink_type = lexmark_get_ink_type(name, printing_color, caps);

  if (ink_type->name == NULL) {
    return (NULL); /* not found ! */
  }

  return &(ink_type->ink_parameter[printing_color]);
}


static const paper_t *
get_media_type(const char *name, const lexmark_cap_t * caps)
{
  int i;
  if (name)
    {
      for (i = 0; i < paper_type_count; i++)
	{
	  if (!strcmp(name, lexmark_paper_list[i].name))
	    return &(lexmark_paper_list[i]);
	}
    }
  return NULL;
}

static int
lexmark_source_type(const char *name, const lexmark_cap_t * caps)
{
  if (name)
    {
      if (!strcmp(name,"Auto"))    return 4;
      if (!strcmp(name,"Manual"))    return 0;
      if (!strcmp(name,"ManualNP")) return 1;
    }

  stp_deprintf(STP_DBG_LEXMARK,
	       "lexmark: Unknown source type '%s' - reverting to auto\n",name);

  return 4;
}



/*******************************
lexmark_head_offset
*******************************/
static const lexmark_lineoff_t *
lexmark_head_offset(int ydpi,                       /* i */
		    const char *ink_type,           /* i */
		    const lexmark_cap_t * caps,     /* i */
		    const lexmark_inkparam_t *ink_parameter, /* i */
		    lexmark_lineoff_t *lineoff_buffer)  /* o */
{
  int i;

  stp_deprintf(STP_DBG_LEXMARK, "  sizie %d,  size_v %d, size_v[0] %d\n", (int)sizeof(*lineoff_buffer), (int)sizeof(lineoff_buffer->v), (int)sizeof(lineoff_buffer->v[0]));
  memcpy(lineoff_buffer, ink_parameter->head_offset, sizeof(*lineoff_buffer));

  for (i=0; i < (sizeof(lineoff_buffer->v) / sizeof(lineoff_buffer->v[0])); i++) {
    lineoff_buffer->v[i] /= (caps->y_raster_res / ydpi);
  }
  return (lineoff_buffer);
}


#if 0
/*******************************
lexmark_size_type
*******************************/
/* This method is actually not used.
   Is there a possibility to set such value ???????????? */
static unsigned char
lexmark_size_type(const stp_vars_t *v, const lexmark_cap_t * caps)
{
  const stp_papersize_t *pp = stp_get_papersize_by_size(stp_get_page_height(v),
							stp_get_page_width(v));
  if (pp)
    {
      const char *name = pp->name;
      /* built ins: */
      if (!strcmp(name,"A5"))		return 0x01;
      if (!strcmp(name,"A4"))		return 0x03;
      if (!strcmp(name,"B5"))		return 0x08;
      if (!strcmp(name,"Letter"))	return 0x0d;
      if (!strcmp(name,"Legal"))	return 0x0f;
      if (!strcmp(name,"COM10"))	return 0x16;
      if (!strcmp(name,"DL"))		return 0x17;
      if (!strcmp(name,"LetterExtra"))	return 0x2a;
      if (!strcmp(name,"A4Extra"))	return 0x2b;
      if (!strcmp(name,"w288h144"))	return 0x2d;
      /* custom */

      stp_dprintf(STP_DBG_LEXMARK, v, "lexmark: Unknown paper size '%s' - using custom\n",name);
    } else {
      stp_dprintf(STP_DBG_LEXMARK, v, "lexmark: Couldn't look up paper size %dx%d - "
	      "using custom\n",stp_get_page_height(v), stp_get_page_width(v));
    }
  return 0;
}
#endif


static int lexmark_get_phys_resolution_vertical(int model)
{
  return 600;
}

#if 0
static int lexmark_get_phys_resolution_horizontal(int model)
{
  return 1200;
}
#endif

static const lexmark_res_t
*lexmark_get_resolution_para(int model, const char *resolution)
{
  const lexmark_cap_t * caps= lexmark_get_model_capabilities(model);

  const lexmark_res_t *res = *(caps->res_parameters); /* get the resolution specific parameters of printer */

  if (resolution)
    {
      while (res->hres)
	{
	  if ((res->vres <= caps->max_ydpi) && (caps->max_ydpi != -1) &&
	      (res->hres <= caps->max_xdpi) && (caps->max_xdpi != -1) &&
	      (!strcmp(resolution, res->name)))
	    {
	      return res;
	    }
	  res++;
	}
    }
  stp_deprintf(STP_DBG_LEXMARK, "lexmark_get_resolution_para: resolution not found (%s)\n", resolution);
  return NULL;
}


static int
lexmark_print_bidirectional(int model, const char *resolution)
{
  const lexmark_res_t *res_para = lexmark_get_resolution_para(model, resolution);
  return !res_para->unidirectional;
}

static const char *
lexmark_lum_adjustment(const lexmark_cap_t * caps, const stp_vars_t *v)
{
  return (caps->lum_adjustment);
}

static const char *
lexmark_hue_adjustment(const lexmark_cap_t * caps, const stp_vars_t *v)
{
  return (caps->hue_adjustment);
}

static const char *
lexmark_sat_adjustment(const lexmark_cap_t * caps, const stp_vars_t *v)
{
  return (caps->sat_adjustment);
}


static void
lexmark_describe_resolution(const stp_vars_t *v, int *x, int *y)
{
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  const lexmark_res_t *res =
    lexmark_get_resolution_para(stp_get_model_id(v), resolution);

  if (res)
    {
      *x = res->hres;
      *y = res->vres;
      return;
    }
  *x = -1;
  *y = -1;
}


static stp_param_string_t media_sources[] =
{
  { "Auto",		N_("Auto Sheet Feeder") },
  { "Manual",		N_("Manual with Pause") },
  { "ManualNP",		N_("Manual without Pause") }
};


/*
 * 'lexmark_parameters()' - Return the parameter values for the given parameter.
 */

static stp_parameter_list_t
lexmark_list_parameters(const stp_vars_t *v)
{
  stp_parameter_list_t *ret = stp_parameter_list_create();
  int i;
  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(the_parameters[i]));
  for (i = 0; i < float_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(float_parameters[i].param));
  return ret;
}

static const char *
lexmark_describe_output(const stp_vars_t *v)
{
  int printing_color = 0;
  int model = stp_get_model_id(v);
  const lexmark_cap_t *caps = lexmark_get_model_capabilities(model);
  const char *print_mode = stp_get_string_parameter(v, "PrintingMode");
  const char *ink_type = stp_get_string_parameter(v, "InkType");
  const lexmark_inkparam_t *ink_parameter;

  if (!print_mode || strcmp(print_mode, "Color") == 0)
    printing_color = 1;

  ink_parameter = lexmark_get_ink_parameter(ink_type, printing_color, caps, v);

  if (!ink_parameter || ink_parameter->used_colors == COLOR_MODE_K ||
      caps->inks == LEXMARK_INK_K || !printing_color)
    return "Grayscale";
  else if (!(ink_parameter->used_colors & COLOR_MODE_K))
    return "CMY";
  else
    return "CMYK";
}

static void
lexmark_parameters(const stp_vars_t *v, const char *name,
		   stp_parameter_t *description)
{
  int		i;

  const lexmark_cap_t * caps= lexmark_get_model_capabilities(stp_get_model_id(v));
  description->p_type = STP_PARAMETER_TYPE_INVALID;

  if (name == NULL)
    return;

  for (i = 0; i < float_parameter_count; i++)
    if (strcmp(name, float_parameters[i].param.name) == 0)
      {
	stp_fill_parameter_settings(description,
				    &(float_parameters[i].param));
	description->deflt.dbl = float_parameters[i].defval;
	description->bounds.dbl.upper = float_parameters[i].max;
	description->bounds.dbl.lower = float_parameters[i].min;
	return;
      }

  for (i = 0; i < the_parameter_count; i++)
    if (strcmp(name, the_parameters[i].name) == 0)
      {
	stp_fill_parameter_settings(description, &(the_parameters[i]));
	break;
      }

  if (strcmp(name, "PageSize") == 0)
  {
    unsigned int height_limit, width_limit;
    unsigned int min_height_limit, min_width_limit;
    int papersizes = stp_known_papersizes();
    description->bounds.str = stp_string_list_create();

    width_limit  = caps->max_paper_width;
    height_limit = caps->max_paper_height;
    min_width_limit  = caps->min_paper_width;
    min_height_limit = caps->min_paper_height;

    for (i = 0; i < papersizes; i++) {
      const stp_papersize_t *pt = stp_get_papersize_by_index(i);
      if (strlen(pt->name) > 0 &&
	  pt->width <= width_limit && pt->height <= height_limit &&
	  (pt->height >= min_height_limit || pt->height == 0) &&
	  (pt->width >= min_width_limit || pt->width == 0))
	{
	  if (stp_string_list_count(description->bounds.str) == 0)
	    description->deflt.str = pt->name;
	  stp_string_list_add_string(description->bounds.str,
				     pt->name, gettext(pt->text));
	}
    }
  }
  else if (strcmp(name, "Resolution") == 0)
  {
    const lexmark_res_t *res;
    description->bounds.str = stp_string_list_create();

    res =  *(caps->res_parameters); /* get resolution specific parameters of printer */

    /* check for allowed resolutions */
    while (res->hres)
      {
	if (stp_string_list_count(description->bounds.str) == 0)
	  description->deflt.str = res->name;
	stp_string_list_add_string(description->bounds.str,
				  res->name, gettext(res->text));
	res++;
      }
  }
  else if (strcmp(name, "InkType") == 0)
  {
    description->bounds.str = stp_string_list_create();
    description->deflt.str = caps->ink_types[0].name;
    for (i = 0; caps->ink_types[i].name != NULL; i++)
      stp_string_list_add_string(description->bounds.str,
			       caps->ink_types[i].name,
			       gettext(caps->ink_types[i].text));
  }
  else if (strcmp(name, "MediaType") == 0)
  {
    description->bounds.str = stp_string_list_create();
    description->deflt.str = lexmark_paper_list[0].name;
    for (i = 0; i < paper_type_count; i++)
      stp_string_list_add_string(description->bounds.str,
			       lexmark_paper_list[i].name,
			       gettext(lexmark_paper_list[i].text));
  }
  else if (strcmp(name, "InputSlot") == 0)
  {
    description->bounds.str = stp_string_list_create();
    description->deflt.str = media_sources[0].name;
    for (i = 0; i < sizeof(media_sources) / sizeof(stp_param_string_t); i++)
      stp_string_list_add_string(description->bounds.str,
			       media_sources[i].name,
			       gettext(media_sources[i].name));
  }
  else if (strcmp(name, "InkChannels") == 0)
    {
      if (caps->inks & LEXMARK_INK_CcMmYyK)
	description->deflt.integer = 7;
      else if (caps->inks & LEXMARK_INK_CcMmYK)
	description->deflt.integer = 6;
      else if (caps->inks & LEXMARK_INK_CMYK)
	description->deflt.integer = 4;
      else if (caps->inks & LEXMARK_INK_CMY)
	description->deflt.integer = 3;
      else
	description->deflt.integer = 1;
      description->bounds.integer.lower = -1;
      description->bounds.integer.upper = -1;
    }
  else if (strcmp(name, "PrintingMode") == 0)
    {
      description->bounds.str = stp_string_list_create();
      if (caps->inks != LEXMARK_INK_K)
	stp_string_list_add_string
	  (description->bounds.str, "Color", _("Color"));
      stp_string_list_add_string
	(description->bounds.str, "BW", _("Black and White"));
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
}

/*
 * 'lexmark_imageable_area()' - Return the imageable area of the page.
 */

static void
internal_imageable_area(const stp_vars_t *v,   /* I */
			int  use_paper_margins,
			int  *left,	/* O - Left position in points */
			int  *right,	/* O - Right position in points */
			int  *bottom,	/* O - Bottom position in points */
			int  *top)	/* O - Top position in points */
{
  int	width, length;			/* Size of page */
  int left_margin = 0;
  int right_margin = 0;
  int bottom_margin = 0;
  int top_margin = 0;
  const char *media_size = stp_get_string_parameter(v, "PageSize");
  const stp_papersize_t *pt = NULL;
  const lexmark_cap_t *caps =
    lexmark_get_model_capabilities(stp_get_model_id(v));


  if (media_size && use_paper_margins)
    pt = stp_get_papersize_by_name(media_size);

  stp_default_media_size(v, &width, &length);
  if (pt)
    {
      left_margin = pt->left;
      right_margin = pt->right;
      bottom_margin = pt->bottom;
      top_margin = pt->top;
    }
  left_margin = max(left_margin, caps->border_left);
  right_margin = max(right_margin, caps->border_right);
  top_margin = max(top_margin, caps->border_top);
  bottom_margin = max(bottom_margin, caps->border_bottom);

  *left =	left_margin;
  *right =	width - right_margin;
  *top =	top_margin;
  *bottom =	length - bottom_margin;
}

static void
lexmark_imageable_area(const stp_vars_t *v,   /* I */
		       int  *left,	/* O - Left position in points */
		       int  *right,	/* O - Right position in points */
		       int  *bottom,	/* O - Bottom position in points */
		       int  *top)	/* O - Top position in points */
{
  internal_imageable_area(v, 1, left, right, bottom, top);
}

static void
lexmark_limit(const stp_vars_t *v,  		/* I */
	      int *width,
	      int *height,
	      int *min_width,
	      int *min_height)
{
  const lexmark_cap_t * caps= lexmark_get_model_capabilities(stp_get_model_id(v));
  *width =	caps->max_paper_width;
  *height =	caps->max_paper_height;
  *min_width =  caps->min_paper_width;
  *min_height = caps->min_paper_height;
}



static int
lexmark_init_printer(const stp_vars_t *v, const lexmark_cap_t * caps,
		     int printing_color,
		     const char *source_str,
		     int xdpi, int ydpi,
		     int page_width, int page_height,
		     int top, int left,
		     int use_dmt)
{

  /* because the details of the header sequence are not known, we simply write it as one image. */

#define LXM_Z52_STARTSIZE 0x35
  /* 300 dpi */
  unsigned char startHeader_z52[LXM_Z52_STARTSIZE]={0x1b,0x2a,0x81,0x00,0x1c,0x56,0x49,0x00,
					   0x01,0x00,0x2c,0x01,0x00,0x00,0x60,0x09,
					   0xe4,0x0c,0x01,0x00,0x34,0x00,0x00,0x00,
					   0x08,0x00,0x08,0x00,0x1b,0x2a,0x07,0x76,
					   0x01,0x1b,0x2a,0x07,0x73,0x30,0x1b,0x2a,
					   0x6d,0x00,0x14,0x01,0xf4,0x02,0x00,0x01,
					   0xf0,0x1b,0x2a,0x07,0x63};

#define LXM_Z42_STARTSIZE 0x30
  /* 600 dpi */
  unsigned char startHeader_z42[LXM_Z42_STARTSIZE]={0x1B,0x2A,0x81,0x00,0x1C,0x50,0x41,0x00,
					   0x01,0x00,0x58,0x02,0x04,0x00,0xC0,0x12,
					   0xC8,0x19,0x02,0x00,0x50,0x00,0x14,0x00,
					   0x07,0x00,0x08,0x00,0x1B,0x2A,0x07,0x73,
					   0x30,0x1B,0x2A,0x6D,0x00,0x14,0x01,0xC0,
					   0x02,0x00,0x01,0xBE,0x1B,0x2A,0x07,0x63};

  #define ESC2a "\033\052"



#define LXM_3200_STARTSIZE 32

  unsigned char startHeader_3200[LXM_3200_STARTSIZE] =
  {
    0x1b, 0x2a, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x1b, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33,
    0x1b, 0x30, 0x80, 0x0C, 0x02, 0x00, 0x00, 0xbe,
    0x1b, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21
  };

  /* write init sequence */
  switch(caps->model)
	{
		case m_z52:
			stp_zfwrite((const char *) startHeader_z52,
				    LXM_Z52_STARTSIZE,1,v);
#ifdef DEBUG
			lex_write_tmp_file(dbgfileprn, (void *)startHeader_z52, LXM_Z52_STARTSIZE);
#endif
		case m_z42:
			stp_zfwrite((const char *) startHeader_z42,
				    LXM_Z42_STARTSIZE,1,v);
#ifdef DEBUG
			lex_write_tmp_file(dbgfileprn, (void *)startHeader_z42, LXM_Z42_STARTSIZE);
#endif
			break;

		case m_3200:
			stp_zfwrite((const char *) startHeader_3200,
				    LXM_3200_STARTSIZE, 1, v);
			break;

		default:
			stp_erprintf("Unknown printer !! %i\n", caps->model);
			return 0;
  }



  /*
  stp_dprintf(STP_DBG_LEXMARK, v, "lexmark: printable size = %dx%d (%dx%d) %02x%02x %02x%02x\n",
	  page_width,page_height,printable_width,printable_length,
	  arg_70_1,arg_70_2,arg_70_3,arg_70_4);
  */
  return 1;
}

static void lexmark_deinit_printer(const stp_vars_t *v, const lexmark_cap_t * caps)
{

	switch(caps->model)	{
		case m_z52:
		{
			char buffer[40];

			memcpy(buffer, ESC2a, 2);
			buffer[2] = 0x7;
			buffer[3] = 0x65;

			stp_dprintf(STP_DBG_LEXMARK, v, "lexmark: <<eject page.>> %x %x %x %x\n", buffer[0],  buffer[1], buffer[2], buffer[3]);
#ifdef DEBUG
			lex_write_tmp_file(dbgfileprn, (void *)&(buffer[0]), 4);
#endif
			/* eject page */
			stp_zfwrite(buffer, 1, 4, v);
		}
		break;

		case m_z42:
		{
			unsigned char buffer[12] = {0x1B,0x2A,0x07,0x65,0x1B,0x2A,0x82,0x00,0x00,0x00,0x00,0xAC};
			stp_dprintf(STP_DBG_LEXMARK, v, "lexmark: <<eject page.>>\n");
#ifdef DEBUG
			lex_write_tmp_file(dbgfileprn, (void *)&(buffer[0]), 12);
#endif
			/* eject page */
			stp_zfwrite((char *)buffer, 1, 12, v);
		}
		break;

		case m_3200:
		{
		  unsigned char buffer[24] =
		  {
		    0x1b, 0x22, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
		    0x1b, 0x31, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
		    0x1b, 0x33, 0x10, 0x00, 0x00, 0x00, 0x00, 0x33
		  };

			stp_dprintf(STP_DBG_LEXMARK, v, "Headpos: %d\n", lxm3200_headpos);

			lxm3200_linetoeject += 2400;
			buffer[3] = lxm3200_linetoeject >> 8;
			buffer[4] = lxm3200_linetoeject & 0xff;
			buffer[7] = lexmark_calc_3200_checksum(&buffer[0]);
			buffer[11] = lxm3200_headpos >> 8;
			buffer[12] = lxm3200_headpos & 0xff;
			buffer[15] = lexmark_calc_3200_checksum(&buffer[8]);

			stp_zfwrite((const char *)buffer, 24, 1, v);
		}
		break;

		case m_lex7500:
			break;
	}

}


/* paper_shift() -- shift paper in printer -- units are unknown :-)
 */
static void paper_shift(const stp_vars_t *v, int offset, const lexmark_cap_t * caps)
{
	switch(caps->model)	{
		case m_z52:
		case m_z42:
		{
			unsigned char buf[5] = {0x1b, 0x2a, 0x3, 0x0, 0x0};
			if(offset == 0)return;
			buf[3] = (unsigned char)(offset >> 8);
			buf[4] = (unsigned char)(offset & 0xFF);
			stp_zfwrite((const char *)buf, 1, 5, v);
#ifdef DEBUG
			lex_write_tmp_file(dbgfileprn, (void *)buf, 5);
#endif
		}
		break;

		case m_3200:
		{
			unsigned char buf[8] = {0x1b, 0x23, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00};
			if(offset == 0)return;
			lxm3200_linetoeject -= offset;
			buf[3] = (unsigned char)(offset >> 8);
			buf[4] = (unsigned char)(offset & 0xff);
			buf[7] = lexmark_calc_3200_checksum(buf);
			stp_zfwrite((const char *)buf, 1, 8, v);
		}
		break;

		case m_lex7500:
			break;
	}

	stp_dprintf(STP_DBG_LEXMARK, v, "Lines to eject: %d\n", lxm3200_linetoeject);
}

/*
 * 'advance_buffer()' - Move (num) lines of length (len) down one line
 *                      and sets first line to 0s
 *                      accepts NULL pointers as buf
 *                  !!! buf must contain more than (num) lines !!!
 *                      also sets first line to 0s if num<1
 */
#if 0
static void
lexmark_advance_buffer(unsigned char *buf, int len, int num)
{
  if (!buf || !len) return;
  if (num>0) memmove(buf+len,buf,len*num);
  memset(buf,0,len);
}
#endif

static double
get_double_param(stp_vars_t *v, const char *param)
{
  if (param && stp_check_float_parameter(v, param, STP_PARAMETER_ACTIVE))
    return stp_get_float_parameter(v, param);
  else
    return 1.0;
}

/**********************************************************
 * lexmark_print() - Print an image to a LEXMARK printer.
 **********************************************************/
/* This method should not be printer dependent (mybe it is because of nozzle count and other things) */
/* The method will set the printing method depending on the selected printer.
   It will define the colors to be used and the resolution.
   Additionally the pass_length will be defined.
   The method lexmark_write() is responsible to handle the received lines
   in a correct way.
*/
static int
lexmark_do_print(stp_vars_t *v, stp_image_t *image)
{
  int		status = 1;
  int		y;		/* Looping vars */
  int		xdpi, ydpi;	/* Resolution */
  int		n;		/* Output number */
  int page_width,	/* Width of page */
    page_height,	/* Length of page */
    page_left,
    page_top,
    page_right,
    page_bottom,
    page_true_height,	/* True length of page */
    out_width,	/* Width of image on page in pixles */
    out_height,	/* Length of image on page */
    out_channels,	/* Output bytes per pixel */
    length,		/* Length of raster data in bytes*/
    buf_length,     /* Length of raster data buffer (dmt) */
    errdiv,		/* Error dividend */
    errmod,		/* Error modulus */
    errval,		/* Current error value */
    errline,	/* Current raster line */
    errlast;	/* Last raster line loaded */
  unsigned      zero_mask;
  int           image_height,
                image_width;
  int           use_dmt = 0;
  int pass_length=0;              /* count of inkjets for one pass */
  int add_top_offset=0;              /* additional top offset */
  int printMode = 0;
    int source;
  /* Lexmark do not have differnet pixel sizes. We have to correct the density according the print resolution. */
  double  densityDivisor;            /* This parameter is will adapt the density according the resolution */
  double k_lower, k_upper;
  int  physical_xdpi = 0;
  int  physical_ydpi = 0;
  int i;

  stp_curve_t *lum_adjustment = NULL;
  stp_curve_t *hue_adjustment = NULL;
  stp_curve_t *sat_adjustment = NULL;

  /* weave parameters */
  lexmark_linebufs_t cols;
  int  nozzle_separation;
  int  horizontal_passes;
  int  ncolors;
  lexm_privdata_weave privdata;

  lexmark_lineoff_t lineoff_buffer;  /* holds the line offsets of each color */
#ifdef DEBUG
  testdata td;
#endif


  int		model         = stp_get_model_id(v);
  const char	*resolution   = stp_get_string_parameter(v, "Resolution");
  const char	*media_type   = stp_get_string_parameter(v, "MediaType");
  const char	*media_source = stp_get_string_parameter(v, "InputSlot");
  const char    *print_mode = stp_get_string_parameter(v, "PrintingMode");
  int printing_color = 0;
  const char	*ink_type     = stp_get_string_parameter(v, "InkType");
  int		top = stp_get_top(v);
  int		left = stp_get_left(v);

  const lexmark_cap_t * caps= lexmark_get_model_capabilities(model);
  const lexmark_res_t *res_para_ptr =
    lexmark_get_resolution_para(model, resolution);
  const paper_t *media = get_media_type(media_type,caps);
  const lexmark_inkparam_t *ink_parameter;

  stp_prune_inactive_options(v);

#ifdef DEBUG
  dbgfileprn = lex_open_tmp_file(); /* open file with xx */
#endif

  if (!stp_verify(v))
    {
      stp_eprintf(v, "Print options not verified; cannot print.\n");
      return 0;
    }
  if (strcmp(print_mode, "Color") == 0)
    printing_color = 1;

  ink_parameter = lexmark_get_ink_parameter(ink_type, printing_color, caps, v);

  if (ink_parameter == NULL)
    {
      stp_eprintf(v, "Illegal Ink Type specified; cannot print.\n");
      return 0;
    }

  stp_image_init(image);

  source= lexmark_source_type(media_source,caps);

  /* force grayscale if image is grayscale
   *                 or single black cartridge installed
   */

  if ((ink_parameter->used_colors == COLOR_MODE_K) ||
      (caps->inks == LEXMARK_INK_K))
    {
      printing_color = 0;
      stp_set_string_parameter(v, "PrintingMode", "BW");
    }

  /*
   * Choose the correct color conversion function...
   */


  ncolors = ink_parameter->ncolors;
  printMode = ink_parameter->used_colors;
  pass_length = ink_parameter->pass_length;
  add_top_offset = ink_parameter->v_top_head_offset;


  /*
   * Figure out the output resolution...
   */

  stp_describe_resolution(v, &xdpi, &ydpi);
  stp_dprintf(STP_DBG_LEXMARK, v, "lexmark: resolution=%dx%d\n",xdpi,ydpi);

  switch (res_para_ptr->resid) {
  case DPI300:
    physical_xdpi = 300;
    physical_ydpi = lexmark_get_phys_resolution_vertical(model);
    break;
  case DPI600:
    physical_xdpi = 600;
    physical_ydpi = lexmark_get_phys_resolution_vertical(model);
    break;
  case DPI1200:
  case DPItest:
    physical_xdpi = 1200;
    physical_ydpi = lexmark_get_phys_resolution_vertical(model);
    break;
  default:
    return 0;
    break;
  }
  /* adapt the density */
  densityDivisor = ((xdpi / 300)*(ydpi/ 600));

#ifdef DEBUG
  if (res_para_ptr->resid == DPItest) {
    stp_dprintf(STP_DBG_LEXMARK, v, "Start test print1\n");
    doTestPrint = 1;
  }
#endif

  if ((printMode & COLOR_MODE_PHOTO) == COLOR_MODE_PHOTO) {
    /* in case of photo mode we have to go a bit ligther */
densityDivisor /= 1.2;
  }

  nozzle_separation = ydpi / physical_ydpi;

  horizontal_passes = xdpi / physical_xdpi;
  stp_dprintf(STP_DBG_LEXMARK, v, "lexmark: horizontal_passes %i, xdpi %i, physical_xdpi %i\n",
	       horizontal_passes, xdpi, physical_xdpi);




  if (!strcmp(resolution+(strlen(resolution)-3),"DMT") &&
      (caps->features & LEXMARK_CAP_DMT)) {
    use_dmt= 1;
    stp_dprintf(STP_DBG_LEXMARK, v, "lexmark: using drop modulation technology\n");
  }

  /*
  * Compute the output size...
  */

  out_width = stp_get_width(v);
  out_height = stp_get_height(v);

  internal_imageable_area(v, 0, &page_left, &page_right,
			  &page_bottom, &page_top);
  left -= page_left;
  top -= page_top;
  page_width = page_right - page_left;
  page_height = page_bottom - page_top;

  stp_dprintf(STP_DBG_LEXMARK, v, "page_right %d, page_left %d, page_top %d, page_bottom %d, left %d, top %d\n",page_right, page_left, page_top, page_bottom,left, top);

  image_height = stp_image_height(image);
  image_width = stp_image_width(image);

  stp_default_media_size(v, &n, &page_true_height);
  lxm3200_linetoeject = (page_true_height * 1200) / 72;


  if (!lexmark_init_printer(v, caps, printing_color,
			    media_source,
			    xdpi, ydpi, page_width, page_height,
			    top,left,use_dmt))
    return 0;

  /*
  * Convert image size to printer resolution...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;


  stp_dprintf(STP_DBG_LEXMARK, v, "border: left %d, x_raster_res %d, offser_left %d\n", left, caps->x_raster_res, caps->offset_left_border);

  left = ((caps->x_raster_res * left) / 72) + caps->offset_left_border;

  stp_dprintf(STP_DBG_LEXMARK, v, "border: left %d\n", left);



#ifdef DEBUG
  if (doTestPrint == 1) {
    stp_dprintf(STP_DBG_LEXMARK, v, "Start test print\n");
    testprint(&td);
    out_width = td.x;
    out_height = td.y;
    if (td.cols != 7) {
    printMode = COLOR_MODE_K | COLOR_MODE_M | COLOR_MODE_C | COLOR_MODE_Y;
    } else {
    printMode = COLOR_MODE_K | COLOR_MODE_M | COLOR_MODE_C | COLOR_MODE_Y | COLOR_MODE_LM | COLOR_MODE_LC;
    }
  }
#endif

 /*
  * Allocate memory for the raster data...
  */

  length = (out_width + 7) / 8;



  if (use_dmt) {
    /*    buf_length= length*2; */
    buf_length= length;
  } else {
    buf_length= length;
  }

  stp_dprintf(STP_DBG_LEXMARK, v, "lexmark: buflength is %d!\n",buf_length);


  /* Now we know the color which are used, let's get the memory for every color image */
  cols.p.k = NULL;
  cols.p.c = NULL;
  cols.p.y = NULL;
  cols.p.m = NULL;
  cols.p.C = NULL;
  cols.p.M = NULL;
  cols.p.Y = NULL;


  if ((printMode & COLOR_MODE_C) == COLOR_MODE_C) {
    cols.p.c = stp_zalloc(buf_length+10);
  }
  if ((printMode & COLOR_MODE_Y) == COLOR_MODE_Y) {
    cols.p.y = stp_zalloc(buf_length+10);
  }
  if ((printMode & COLOR_MODE_M) == COLOR_MODE_M) {
    cols.p.m = stp_zalloc(buf_length+10);
  }
  if ((printMode & COLOR_MODE_K) == COLOR_MODE_K) {
    cols.p.k = stp_zalloc(buf_length+10);
  }
  if ((printMode & COLOR_MODE_LC) == COLOR_MODE_LC) {
    cols.p.C = stp_zalloc(buf_length+10);
  }
  if ((printMode & COLOR_MODE_LY) == COLOR_MODE_LY) {
    cols.p.Y = stp_zalloc(buf_length+10);
  }
  if ((printMode & COLOR_MODE_LM) == COLOR_MODE_LM) {
    cols.p.M = stp_zalloc(buf_length+10);
  }

  if (cols.p.k)
    {
      if (cols.p.c)
	stp_set_string_parameter(v, "STPIOutputType", "KCMY");
      else
	stp_set_string_parameter(v, "STPIOutputType", "Grayscale");
    }
  else
    stp_set_string_parameter(v, "STPIOutputType", "CMY");

  stp_dprintf(STP_DBG_LEXMARK, v, "lexmark: driver will use colors %c%c%c%c%c%c%c\n",
	      (cols.p.c) ? 'c' : ' ',
	      (cols.p.C) ? 'C' : ' ',
	      (cols.p.m) ? 'm' : ' ',
	      (cols.p.M) ? 'M' : ' ',
	      (cols.p.y) ? 'y' : ' ',
	      (cols.p.Y) ? 'Y' : ' ',
	      (cols.p.k) ? 'k' : ' ');

  /* initialize soft weaveing */
  privdata.ink_parameter = ink_parameter;
  privdata.bidirectional = lexmark_print_bidirectional(model, resolution);
  privdata.outbuf = stp_malloc((((((pass_length/8)*11))+40) * out_width)+2000);
  privdata.direction = 0;
  stp_allocate_component_data(v, "Driver", NULL, NULL, &privdata);
  /*  lxm_nozzles_used = 1;*/

  stp_initialize_weave(v,
		       pass_length, /* jets */
		       nozzle_separation, /* separation */
		       horizontal_passes, /* h overample */
		       res_para_ptr->vertical_passes, /* v passes */
		       res_para_ptr->vertical_oversample, /* v oversample */
		       ncolors, /* colors */
		       1, /* bits/pixel */
		       out_width, /* line width */
		       out_height,
		       ((top * ydpi) / 72)+(((caps->offset_top_border+add_top_offset)*ydpi)
					    /caps->y_raster_res),
		       (page_height * ydpi) / 72,
		       (const int *) lexmark_head_offset(ydpi, ink_type, caps, ink_parameter, &lineoff_buffer),
		       STP_WEAVE_ZIGZAG, /* weave_strategy */
		       flush_pass,
		       stp_fill_uncompressed,  /* fill_start */
		       stp_pack_uncompressed,  /* pack */
		       stp_compute_uncompressed_linewidth);  /* compute_linewidth */
  privdata.last_pass_offset = 0;
  privdata.jets = pass_length;
  privdata.ncolors = ncolors;
  privdata.horizontal_weave = horizontal_passes;


  if (!stp_check_float_parameter(v, "Density", STP_PARAMETER_DEFAULTED))
    {
      stp_set_float_parameter_active(v, "Density", STP_PARAMETER_ACTIVE);
      stp_set_float_parameter(v, "Density", 1.0);
    }

  stp_dprintf(STP_DBG_LEXMARK, v, "density is %f\n",
	      stp_get_float_parameter(v, "Density"));

  stp_dprintf(STP_DBG_LEXMARK, v, "density is %f and will be changed to %f  (%f)\n",
		stp_get_float_parameter(v, "Density"),
		stp_get_float_parameter(v, "Density") / densityDivisor,
		densityDivisor);

  /* Lexmark do not have differnet pixel sizes. We have to correct the density according the print resolution. */
  stp_scale_float_parameter(v, "Density", 1.0 / densityDivisor);


  /*
   * Compute the LUT.  For now, it's 8 bit, but that may eventually
   * sometimes change.
   */
  if (ncolors > 4)
    k_lower = .5;
  else
    k_lower = .25;

  if (media)
    {
      stp_scale_float_parameter(v, "Density", media->base_density);
      stp_scale_float_parameter(v, "Cyan", media->p_cyan);
      stp_scale_float_parameter(v, "Magenta", media->p_magenta);
      stp_scale_float_parameter(v, "Yellow", media->p_yellow);
      k_lower *= media->k_lower_scale;
      k_upper  = media->k_upper;
    }
  else
    {
      stp_scale_float_parameter(v, "Density", .8);
      k_lower *= .1;
      k_upper = .5;
    }
  if (stp_get_float_parameter(v, "Density") > 1.0)
    stp_set_float_parameter(v, "Density", 1.0);

  stp_dprintf(STP_DBG_LEXMARK, v, "density is %f\n",stp_get_float_parameter(v, "Density"));


  if (!stp_check_float_parameter(v, "GCRLower", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(v, "GCRLower", k_lower);
  if (!stp_check_float_parameter(v, "GCRUpper", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(v, "GCRUpper", k_upper);
  stp_dither_init(v, image, out_width, xdpi, ydpi);

	/*
	  stpi_dither_set_black_lower(dither, .8 / ((1 << (use_dmt+1)) - 1));*/
  /*stpi_dither_set_black_levels(dither, 0.5, 0.5, 0.5);
    stpi_dither_set_black_lower(dither, 0.4);*/
  /*
    if (use_glossy_film)
  */
  if (cols.p.k)
    {
      stp_dither_add_channel(v, cols.p.k, STP_ECOLOR_K, 0);
      stp_channel_set_black_channel(v, STP_ECOLOR_K);
    }
  if (cols.p.c)
    stp_dither_add_channel(v, cols.p.c, STP_ECOLOR_C, 0);
  if (cols.p.C)
    stp_dither_add_channel(v, cols.p.C, STP_ECOLOR_C, 1);
  if (cols.p.m)
    stp_dither_add_channel(v, cols.p.m, STP_ECOLOR_M, 0);
  if (cols.p.M)
    stp_dither_add_channel(v, cols.p.M, STP_ECOLOR_M, 1);
  if (cols.p.y)
    stp_dither_add_channel(v, cols.p.y, STP_ECOLOR_Y, 0);
  if (cols.p.Y)
    stp_dither_add_channel(v, cols.p.Y, STP_ECOLOR_Y, 1);

  if (!use_dmt) {
    if (cols.p.C)
      {
	stp_dither_set_inks_full(v, STP_ECOLOR_C, 2, photo_dither_shades, 1.0,
				  0.31 / .5);
      }
    if (cols.p.M)
      {
	stp_dither_set_inks_full(v, STP_ECOLOR_M, 2, photo_dither_shades, 1.0,
				  0.61 / .97);
      }
    if (cols.p.Y)
      {
	stp_dither_set_inks_full(v, STP_ECOLOR_Y, 2, photo_dither_shades, 1.0,
				  0.08);
      }
  }

  stp_channel_set_density_adjustment(v, STP_ECOLOR_K, 0,
				     get_double_param(v, "BlackDensity") *
				     get_double_param(v, "Density"));
  stp_channel_set_density_adjustment(v, STP_ECOLOR_C, 0,
				     get_double_param(v, "CyanDensity") *
				     get_double_param(v, "Density"));
  stp_channel_set_density_adjustment(v, STP_ECOLOR_M, 0,
				     get_double_param(v, "MagentaDensity") *
				     get_double_param(v, "Density"));
  stp_channel_set_density_adjustment(v, STP_ECOLOR_Y, 0,
				     get_double_param(v, "YellowDensity") *
				     get_double_param(v, "Density"));
  if (!use_dmt) {
    if (cols.p.C)
      {
	stp_channel_set_density_adjustment
	  (v, STP_ECOLOR_C, 1, (get_double_param(v, "CyanDensity") *
			    get_double_param(v, "LightCyanTrans") *
			    get_double_param(v, "Density")));
      }
    if (cols.p.M)
      {
	stp_channel_set_density_adjustment
	  (v, STP_ECOLOR_M, 1, (get_double_param(v, "MagentaDensity") *
			    get_double_param(v, "LightMagentaTrans") *
			    get_double_param(v, "Density")));
      }
    if (cols.p.Y)
      {
	stp_channel_set_density_adjustment
	  (v, STP_ECOLOR_Y, 1, (get_double_param(v, "YellowDensity") *
			    get_double_param(v, "LightYellowTrans") *
			    get_double_param(v, "Density")));
      }
  }

  /*
   * Output the page...
  */

  if (!stp_check_curve_parameter(v, "HueMap", STP_PARAMETER_ACTIVE) &&
      media->hue_adjustment)
    {
      hue_adjustment = stp_read_and_compose_curves
	(lexmark_hue_adjustment(caps, v),
	 media ? media->hue_adjustment : NULL, STP_CURVE_COMPOSE_ADD, 384);
      stp_set_curve_parameter(v, "HueMap", hue_adjustment);
      stp_curve_destroy(hue_adjustment);
    }
  if (!stp_check_curve_parameter(v, "LumMap", STP_PARAMETER_ACTIVE) &&
      media->lum_adjustment)
    {
      lum_adjustment = stp_read_and_compose_curves
	(lexmark_lum_adjustment(caps, v),
	 media ? media->lum_adjustment : NULL, STP_CURVE_COMPOSE_MULTIPLY, 384);
      stp_set_curve_parameter(v, "LumMap", lum_adjustment);
      stp_curve_destroy(lum_adjustment);
    }
  if (!stp_check_curve_parameter(v, "SatMap", STP_PARAMETER_ACTIVE) &&
      media->sat_adjustment)
    {
      sat_adjustment = stp_read_and_compose_curves
	(lexmark_sat_adjustment(caps, v),
	 media ? media->sat_adjustment : NULL, STP_CURVE_COMPOSE_MULTIPLY, 384);
      stp_set_curve_parameter(v, "SatMap", sat_adjustment);
      stp_curve_destroy(sat_adjustment);
    }

  out_channels = stp_color_init(v, image, 65536);

  /* calculate the memory we need for one line of the printer image (hopefully we are right) */
  stp_dprintf(STP_DBG_LEXMARK, v, "---------- buffer mem size = %d\n", (((((pass_length/8)*11)/10)+40) * out_width)+200);

  errdiv  = image_height / out_height;
  errmod  = image_height % out_height;
  errval  = 0;
  errlast = -1;
  errline  = 0;

  privdata.hoffset = left;
  privdata.ydpi = ydpi;
  privdata.model = model;
  privdata.width = out_width;
  privdata.xdpi = xdpi;
  privdata.physical_xdpi = physical_xdpi;
  privdata.bitwidth = 1;

  for (y = 0; y < out_height; y ++)
    {
      int duplicate_line = 1;

      if (errline != errlast)
	{
	  errlast = errline;
	  duplicate_line = 0;
	  if (stp_color_get_row(v, image, errline, &zero_mask))
	    {
	      status = 2;
	      break;
	    }
	}
      stp_dither(v, y, duplicate_line, zero_mask, NULL);
      stp_write_weave(v, (unsigned char **)cols.v);

      errval += errmod;
      errline += errdiv;
      if (errval >= out_height)
	{
	  errval -= out_height;
	  errline ++;
	}
    }
  stp_image_conclude(image);

  stp_flush_all(v);

  lexmark_deinit_printer(v, caps);

  /*
  * Cleanup...
  */
  if (privdata.outbuf != NULL) {
    stp_free(privdata.outbuf);/* !!!!!!!!!!!!!! */
  }

  for (i = 0; i < NCHANNELS; i++)
    if (cols.v[i])
      stp_free(cols.v[i]);

#ifdef DEBUG
  lex_tmp_file_deinit(dbgfileprn);
#endif

  return status;
}

static int
lexmark_print(const stp_vars_t *v, stp_image_t *image)
{
  int status;
  stp_vars_t *nv = stp_vars_create_copy(v);
  stp_prune_inactive_options(nv);
  status = lexmark_do_print(nv, image);
  stp_vars_destroy(nv);
  return status;
}

static const stp_printfuncs_t print_lexmark_printfuncs =
{
  lexmark_list_parameters,
  lexmark_parameters,
  stp_default_media_size,
  lexmark_imageable_area,
  lexmark_imageable_area,
  lexmark_limit,
  lexmark_print,
  lexmark_describe_resolution,
  lexmark_describe_output,
  stp_verify_printer_params,
  NULL,
  NULL,
  NULL
};



/* lexmark_init_line
   This method is printer type dependent code.

   This method initializes the line to be printed. It will set
   the printer specific initialization which has to be done bofor
   the pixels of the image could be printed.
*/
static unsigned char *
lexmark_init_line(int mode, unsigned char *prnBuf,
		  int pass_length,
		  int offset,    /* offset from left in 1/"x_raster_res" DIP (printer resolution)*/
		  int width, int direction,
		  const lexmark_inkparam_t *ink_parameter,
		  const lexmark_cap_t *   caps	        /* I - Printer model */
		  )
{
  int pos1 = 0;
  int pos2 = 0;
  int abspos, disp;
  int hend = 0;
  int header_size = 0;


  /*  stp_erprintf("#### width %d, length %d, pass_length %d\n", width, length, pass_length);*/
  /* first, we wirte the line header */
  switch(caps->model)  {
  case m_z52:
  case m_z42:
    if (caps->model == m_z52)
      {
	header_size = LXM_Z52_HEADERSIZE;
	memcpy(prnBuf, outbufHeader_z52, header_size);
      }
    if (caps->model == m_z42)
      {
	header_size = LXM_Z42_HEADERSIZE;
	memcpy(prnBuf, outbufHeader_z42, LXM_Z42_HEADERSIZE);
      }

    /* K could only be present if black is printed only. */
    if ((mode & COLOR_MODE_K) || (mode & (COLOR_MODE_K | COLOR_MODE_LC | COLOR_MODE_LM))) {
      stp_deprintf(STP_DBG_LEXMARK, "set  photo/black catridge \n");
      prnBuf[LX_Z52_COLOR_MODE_POS] = LX_Z52_BLACK_PRINT;

      if (direction) {
      } else {
	offset += ink_parameter->h_direction_offset;
      }
    } else {
      stp_deprintf(STP_DBG_LEXMARK, "set color cartridge \n");
      prnBuf[LX_Z52_COLOR_MODE_POS] = LX_Z52_COLOR_PRINT;

      if (direction) {
	offset += ink_parameter->h_catridge_offset;
      } else {
	offset += ink_parameter->h_catridge_offset + ink_parameter->h_direction_offset;
      }
    }

    switch (mode & PRINT_MODE_MASK) {
    case PRINT_MODE_300:
      prnBuf[LX_Z52_RESOLUTION_POS] = LX_Z52_300_DPI;
      break;
    case PRINT_MODE_600:
      prnBuf[LX_Z52_RESOLUTION_POS] = LX_Z52_600_DPI;
      break;
    case PRINT_MODE_1200:
      prnBuf[LX_Z52_RESOLUTION_POS] = LX_Z52_1200_DPI;
      break;
    case PRINT_MODE_2400:
      prnBuf[LX_Z52_RESOLUTION_POS] = LX_Z52_2400_DPI;
      break;
    }


    if (direction) {
      prnBuf[LX_Z52_PRINT_DIRECTION_POS] = 1;
    } else {
      prnBuf[LX_Z52_PRINT_DIRECTION_POS] = 2;
    }

    /* set package count */
    prnBuf[13] = (unsigned char)((width) >> 8);
    prnBuf[14] = (unsigned char)((width) & 0xFF);
    /* set horizontal offset */
    prnBuf[15] =(unsigned char)(offset >> 8);
    prnBuf[16] =(unsigned char)(offset & 0xFF);

    if (caps->model == m_z42) {
	switch(mode & PRINT_MODE_MASK) {
	case PRINT_MODE_300:
		hend = (width-1)*(2400/300);
		break;
	case PRINT_MODE_600:
		hend = (width-1)*(2400/600);
		break;
	case PRINT_MODE_1200:
		hend = (width-1)*(2400/1200);
		break;
	case PRINT_MODE_2400:
		hend = (width-1)*(2400/2400);
		break;
	}
	hend += offset;
	prnBuf[17] = (unsigned char)(hend >> 8);
        prnBuf[18] = (unsigned char)(hend & 0xFF);

 	prnBuf[10] = (pass_length==208 ? 0x1A : 0x18);
    }

    return prnBuf + header_size;  /* return the position where the pixels have to be written */
    break;
    case m_3200:
      memcpy(prnBuf, outbufHeader_3200, LXM_3200_HEADERSIZE);

      offset = (offset - 60) * 4;

      /* K could only be present if black is printed only. */
      if((mode & COLOR_MODE_K) ||
	 (mode & (COLOR_MODE_K | COLOR_MODE_LC | COLOR_MODE_LM)))
	{
	  disp = LXM3200_LEFTOFFS;
	  prnBuf[2] = 0x00;
	}
      else
	{
	  disp = LXM3200_RIGHTOFFS;
	  prnBuf[2] = 0x80;
	}

      if(pass_length == 208)
	{
	  prnBuf[2] |= 0x10;
	}

      switch(mode & PRINT_MODE_MASK) 	{
	case PRINT_MODE_300:
	  prnBuf[2] |= 0x20;
	  pos1 = offset + disp;
	  pos2 = offset + (width * 4) + disp;
	  break;

	case PRINT_MODE_600:
	  prnBuf[2] |= 0x00;
	  pos1 = offset + disp;
	  pos2 = offset + (width * 2) + disp;
	  break;

	case PRINT_MODE_1200:
	  prnBuf[2] |= 0x40;
	  pos1 = offset + disp;
	  pos2 = (offset + width) + disp;
	  break;
	}

      if(direction)
	prnBuf[2] |= 0x01;
      else
	prnBuf[2] |= 0x00;

      /* set package count */
      prnBuf[3] = (unsigned char)((width) >> 8);
      prnBuf[4] = (unsigned char)((width) & 0xff);

      /* set horizontal offset */
      prnBuf[21] = (unsigned char)(pos1 >> 8);
      prnBuf[22] = (unsigned char)(pos1 & 0xFF);

      abspos = ((((pos2 - 3600) >> 3) & 0xfff0) + 9);
      prnBuf[5] = (abspos-lxm3200_headpos) >> 8;
      prnBuf[6] = (abspos-lxm3200_headpos) & 0xff;

      lxm3200_headpos = abspos;

      if(LXM3200_RIGHTOFFS > 4816)
	abspos = (((LXM3200_RIGHTOFFS - 4800) >> 3) & 0xfff0);
      else
	abspos = (((LXM3200_RIGHTOFFS - 3600) >> 3) & 0xfff0);

      prnBuf[11] = (lxm3200_headpos-abspos) >> 8;
      prnBuf[12] = (lxm3200_headpos-abspos) & 0xff;

      lxm3200_headpos = abspos;

      prnBuf[7] = (unsigned char)lexmark_calc_3200_checksum(&prnBuf[0]);
      prnBuf[15] = (unsigned char)lexmark_calc_3200_checksum(&prnBuf[8]);
      prnBuf[23] = (unsigned char)lexmark_calc_3200_checksum(&prnBuf[16]);

      /* return the position where the pixels have to be written */
      return prnBuf + LXM_3200_HEADERSIZE;
      break;

  case m_lex7500:
    stp_erprintf("Lexmark 7500 not supported !\n");
    return NULL;
    break;
  }
  return NULL;
}


typedef struct Lexmark_head_colors {
  int v_start;
  unsigned char *line;
  int head_nozzle_start;
  int head_nozzle_end;
  int used_jets;
} Lexmark_head_colors;

/* lexmark_write
   This method is has NO printer type dependent code.
   This method writes a single line of the print. The line consits of "pass_length"
   pixel lines (pixels, which could be printed with one pass by the printer.
*/


static int
lexmark_write(const stp_vars_t *v,		/* I - Print file or command */
	      unsigned char *prnBuf,      /* mem block to buffer output */
	      int *paperShift,
	      int direction,
	      int pass_length,       /* num of inks to print */
	      const lexmark_cap_t *   caps,	        /* I - Printer model */
	      const lexmark_inkparam_t *ink_parameter,
	      int xdpi,
	      int yCount,
	      Lexmark_head_colors *head_colors,
	      int           length,	/* I - Length of bitmap data in bytes */
	      int           mode,	/* I - Which color */
	      int           ydpi,		/* I - Vertical resolution */
	      int           width,	/* I - Printed width in pixles */
	      int           offset, 	/* I - Offset from left side in lexmark_cap_t.x_raster_res DPI */
	      int           dmt)
{
  unsigned char *tbits=NULL, *p=NULL;
  int clen;
  int x;  /* actual vertical position */
  int y;  /* actual horizontal position */
  int dy; /* horiz. inkjet posintion */
  int x1;
  unsigned short pixelline;  /* byte to be written */
  unsigned int valid_bytes; /* bit list which tells the present bytes */
  int xStart=0; /* count start for horizontal line */
  int xEnd=0;
  int xIter=0;  /* count direction for horizontal line */
  int anyCol=0;
  int colIndex;
  int rwidth; /* real with used at printing (includes shift between even & odd nozzles) */
  /* stp_dprintf(STP_DBG_LEXMARK, v, "<%c>",("CMYKcmy"[coloridx])); */
  stp_dprintf(STP_DBG_LEXMARK, v, "pass length %d\n", pass_length);


  /* first, we check the length of the line an cut it if necessary. */
  if ((((width*caps->x_raster_res)/xdpi)+offset) > ((caps->max_paper_width*caps->x_raster_res)/72)) {
    /* line too long !! Cut the line */
   stp_dprintf(STP_DBG_LEXMARK, v, "!! Line too long !! reduce it from %d", width);
    width = ((((caps->max_paper_width*caps->x_raster_res)/72) - offset)*xdpi)/caps->x_raster_res;
   stp_dprintf(STP_DBG_LEXMARK, v, " down to %d\n", width);
  }


  /* we have to write the initial sequence for a line */

  stp_dprintf(STP_DBG_LEXMARK, v, "lexmark: printer line initialized.\n");

  if (direction) {
    /* left to right */
    xStart = -get_lr_shift(mode);
    xEnd = width-1;
    xIter = 1;
    rwidth = xEnd - xStart;
  } else {
    /* right to left ! */
    xStart = width-1;
    xEnd = -get_lr_shift(mode);
    rwidth = xStart - xEnd;
    xIter = -1;
  }

  p = lexmark_init_line(mode, prnBuf, pass_length, offset, rwidth,
			direction,  /* direction */
			ink_parameter, caps);


  stp_dprintf(STP_DBG_LEXMARK, v, "lexmark: xStart %d, xEnd %d, xIter %d.\n", xStart, xEnd, xIter);

  /* now we can start to write the pixels */
  yCount = 2;


  for (x=xStart; x != xEnd; x+=xIter) {
    int  anyDots=0; /* tells us if there was any dot to print */

       switch(caps->model)	{
	case m_z52:
	  tbits = p;
	  *(p++) = 0x3F;
	  tbits[1] = 0; /* here will be nice bitmap */
	  p++;
	  break;

	case m_3200:
	case m_z42:
	  tbits = p;
	  p += 4;
	  break;

	case m_lex7500:
	  break;
	}


    pixelline =0;     /* here we store 16 pixels */
    valid_bytes = 0;  /* for every valid word (16 bits) a corresponding bit will be set to 1. */

    anyDots =0;
    x1 = x+get_lr_shift(mode);

    for (colIndex=0; colIndex < 3; colIndex++) {
      for (dy=head_colors[colIndex].head_nozzle_start,y=head_colors[colIndex].v_start*yCount;
	   (dy < head_colors[colIndex].head_nozzle_end);
	   y+=yCount, dy++) { /* we start counting with 1 !!!! */
	if (head_colors[colIndex].line != NULL) {
	  pixelline = pixelline << 1;
	  if ((x >= 0) &&
	      ((dy - head_colors[colIndex].head_nozzle_start) < (head_colors[colIndex].used_jets/2)))
	    pixelline = pixelline | ((head_colors[colIndex].line[(y*length)+(x/8)] >> (7-(x%8))) & 0x1);
	  pixelline = pixelline << 1;
	  if ((x1 < width) &&
	      (((dy - head_colors[colIndex].head_nozzle_start)+1) < (head_colors[colIndex].used_jets/2)))
	    pixelline = pixelline | ((head_colors[colIndex].line[(((yCount>>1)+y)*length)+ (x1/8)] >> (7-(x1%8))) & 0x1);

	} else {
	  pixelline = pixelline << 2;
	}
	switch(caps->model)		{
	case m_z52:
	  if ((dy%8) == 7) {
	    /* we have two bytes, write them */
	    anyDots |= pixelline;
	    if (pixelline) {
	      /* we have some dots */
	      valid_bytes = valid_bytes >> 1;
	      *((p++)) = (unsigned char)(pixelline >> 8);
	      *((p++)) = (unsigned char)(pixelline & 0xff);
	    } else {
	      /* there are no dots ! */
	      valid_bytes = valid_bytes >> 1;
	      valid_bytes |= 0x1000;
	    }
	    pixelline =0;
	  }
	  break;

	case m_3200:
	case m_z42:
	  if((dy % 4) == 3)
	    {
	      anyDots |= pixelline;
	      valid_bytes <<= 1;

	      if(pixelline)
		*(p++) = (unsigned char)(pixelline & 0xff);
	      else
		valid_bytes |= 0x01;

	      pixelline = 0;
	    }
	  break;

	case m_lex7500:
	  break;
	}
      }
    }

    switch(caps->model)	{
    case m_z52:
      if (pass_length != 208) {
	valid_bytes = valid_bytes >> 1;
	valid_bytes |= 0x1000;
      }
      tbits[0] = 0x20 | ((unsigned char)((valid_bytes >> 8) & 0x1f));
      tbits[1] = (unsigned char)(valid_bytes & 0xff);
      break;

    case m_z42:
      if ((p-tbits) & 1) *(p++)=0; /* z42 packets always have even length */
      /* fall through */
    case m_3200:
      tbits[0] = 0x80 | ((unsigned char)((valid_bytes >> 24) & 0x1f));
      tbits[1] = (unsigned char)((valid_bytes >> 16) & 0xff);
      tbits[2] = (unsigned char)((valid_bytes >> 8) & 0xff);
      tbits[3] = (unsigned char)(valid_bytes & 0xff);
      break;

    case m_lex7500:
      break;
    }


    if (anyDots) {
      anyCol = 1;
    } else {
      /* there are no dots, make empy package */
      /*     stp_dprintf(STP_DBG_LEXMARK, v, "-- empty col %i\n", x); */
    }
  }

  stp_dprintf(STP_DBG_LEXMARK, v, "lexmark: 4\n");

  clen=((unsigned char *)p)-prnBuf;

  switch(caps->model)    {
    case m_z52:
    case m_z42:
  prnBuf[IDX_SEQLEN]  =(unsigned char)(clen >> 24);
  prnBuf[IDX_SEQLEN+1]  =(unsigned char)(clen >> 16);
  prnBuf[IDX_SEQLEN+2]  =(unsigned char)(clen >> 8);
  prnBuf[IDX_SEQLEN+3]=(unsigned char)(clen & 0xFF);
  break;

  case m_3200:
    prnBuf[18] = (unsigned char)((clen - LXM_3200_HEADERSIZE) >> 16);
    prnBuf[19] = (unsigned char)((clen - LXM_3200_HEADERSIZE) >> 8);
    prnBuf[20] = (unsigned char)((clen - LXM_3200_HEADERSIZE) & 0xff);
    prnBuf[23] = (unsigned char)lexmark_calc_3200_checksum(&prnBuf[16]);
    break;

  default:
    break;
  }

  if (anyCol) {
    /* fist, move the paper */
    paper_shift(v, (*paperShift), caps);
    *paperShift=0;

    /* now we write the image line */
    stp_zfwrite((const char *)prnBuf,1,clen,v);
#ifdef DEBUG
    lex_write_tmp_file(dbgfileprn, (void *)prnBuf,clen);
#endif
    stp_dprintf(STP_DBG_LEXMARK, v, "lexmark: line written.\n");
    return 1;
  } else {
    stp_dprintf(STP_DBG_LEXMARK, v, "-- empty line\n");
    return 0;
  }

  /* Send a line of raster graphics... */
}



#ifdef DEBUG
const stp_vars_t *lex_open_tmp_file() {
  int i;
  const stp_vars_t *ofile;
  char tmpstr[256];

      stp_erprintf(" create file !\n");
  for (i=0, sprintf(tmpstr, "/tmp/xx%d.prn", i), ofile = fopen(tmpstr, "r");
       ofile != NULL;
       i++, sprintf(tmpstr, "/tmp/xx%d.prn", i), ofile = fopen(tmpstr, "r")) {
    if (ofile != NULL)
      {
	fclose(ofile);
      }
  }
      stp_erprintf("Create file %s !\n", tmpstr);
  ofile = fopen(tmpstr, "wb");
  STPI_ASSERT(ofile);
  return ofile;
}

void lex_tmp_file_deinit(const stp_vars_t *file) {
  stp_erprintf("Close file %lx\n", file);
  fclose(file);
}

const stp_vars_t *lex_write_tmp_file(const stp_vars_t *ofile, void *data,int length) {
  fwrite(data, 1, length, ofile);
}


#endif


static void
flush_pass(stp_vars_t *v, int passno, int vertical_subpass)
{
  stp_lineoff_t        *lineoffs   = stp_get_lineoffsets_by_pass(v, passno);
  stp_lineactive_t     *lineactive = stp_get_lineactive_by_pass(v, passno);
  const stp_linebufs_t *bufs       = stp_get_linebases_by_pass(v, passno);
  stp_pass_t           *pass       = stp_get_pass_by_pass(v, passno);
  stp_linecount_t      *linecount  = stp_get_linecount_by_pass(v, passno);
  lexm_privdata_weave *pd =
    (lexm_privdata_weave *) stp_get_component_data(v, "Driver");
  int width = pd->width;
  int hoffset = pd->hoffset;
  int model = pd->model;
  int xdpi = pd->xdpi;
  int ydpi = pd->ydpi;
  int physical_xdpi = pd->physical_xdpi;
  int lwidth = (width + (pd->horizontal_weave - 1)) / pd->horizontal_weave;
  int microoffset = vertical_subpass & (pd->horizontal_weave - 1);

  int prn_mode;
  int j; /* color counter */
  const lexmark_cap_t * caps= lexmark_get_model_capabilities(model);
  int paperShift;
  Lexmark_head_colors head_colors[3]={{0, NULL,     0,  64/2, 64},
				      {0, NULL,  64/2, 128/2, 64},
				      {0, NULL, 128/2, 192/2, 64}};


  stp_dprintf(STP_DBG_LEXMARK, v, "Lexmark: flush_pass, here we are !\n");
  stp_dprintf(STP_DBG_LEXMARK, v, "  passno %d, pd->ncolors %d, width %d, lwidth %d, bitwidth %d\n",
	       passno, pd->ncolors, width, lwidth, pd->bitwidth);
  stp_dprintf(STP_DBG_LEXMARK, v, "microoffset %d, vertical_subpass %d, pd->horizontal_weave %d\n", microoffset,vertical_subpass, pd->horizontal_weave);

  stp_dprintf(STP_DBG_LEXMARK, v, "Lexmark: last_pass_offset %d, logicalpassstart %d\n", pd->last_pass_offset, pass->logicalpassstart);
  stp_dprintf(STP_DBG_LEXMARK, v, "Lexmark: vertical adapt: caps->y_raster_res %d, ydpi %d,  \n", caps->y_raster_res, ydpi);

  switch (physical_xdpi) {
  case 300:
    prn_mode = PRINT_MODE_300;
    break;
  case 600:
    prn_mode = PRINT_MODE_600;
    break;
  case 1200:
    prn_mode = PRINT_MODE_1200;
    break;
  default:
    stp_dprintf(STP_DBG_LEXMARK, v, "Eror: Unsupported phys resolution (%d)\n", physical_xdpi);
    return;
    break;
  }
  /* calculate paper shift and adapt actual resoution to physical positioning resolution */
  paperShift = (pass->logicalpassstart - pd->last_pass_offset) * (caps->y_raster_res/ydpi);
  for (j = 0; j < pd->ncolors; j++)
    stp_dprintf(STP_DBG_LEXMARK, v, "Color %d: active %d line %p jets %d offset %ld\n",
		j, lineactive[0].v[j], bufs[0].v[j], linecount[0].v[j],
		lineoffs[0].v[j]);

      /*** do we have to print something with the color cartridge ? ***/
      if ((STP_ECOLOR_C < pd->ncolors) && (lineactive[0].v[STP_ECOLOR_C] > 0))
	{
	  head_colors[0].line = bufs[0].v[STP_ECOLOR_C];
	  head_colors[0].used_jets = linecount[0].v[STP_ECOLOR_C];
	}
      else
	{
	  head_colors[0].line = NULL;
	  head_colors[0].used_jets = 0;
	}

      if ((STP_ECOLOR_M < pd->ncolors) && (lineactive[0].v[STP_ECOLOR_M] > 0))
	{
	  head_colors[1].line = bufs[0].v[STP_ECOLOR_M];
	  head_colors[1].used_jets = linecount[0].v[STP_ECOLOR_M];
	}
      else
	{
	  head_colors[1].line = 0;
	  head_colors[1].used_jets = 0;
	}

      if ((STP_ECOLOR_Y < pd->ncolors) && (lineactive[0].v[STP_ECOLOR_Y] > 0))
	{
	  head_colors[2].line = bufs[0].v[STP_ECOLOR_Y];
	  head_colors[2].used_jets = linecount[0].v[STP_ECOLOR_Y];
	}
      else
	{
	  head_colors[2].line = 0;
	  head_colors[2].used_jets = 0;
	}

      if ((head_colors[0].line != 0) || (head_colors[1].line != 0) || (head_colors[2].line != 0)) {



	stp_dprintf(STP_DBG_LEXMARK, v, "lexmark_write: lwidth %d\n", lwidth);
	lexmark_write(v,		/* I - Print file or command */
		      pd->outbuf,/*unsigned char *prnBuf,   mem block to buffer output */
		      &paperShift,           /* int *paperShift, */
		      pd->direction,                     /* int direction, */
		      pd->jets,       /* num of inks to print */
		      caps,                  /* const lexmark_cap_t *   caps,	    I - Printer model */
		      pd->ink_parameter,
		      xdpi,                  /* int xresolution, */
		      2,                     /* yCount,*/
		      head_colors,           /* Lexmark_head_colors *head_colors, */
		      (lwidth+7)/8, /* length,	 I - Length of bitmap data of one line in bytes */
		      prn_mode | COLOR_MODE_C | COLOR_MODE_Y | COLOR_MODE_M,       /* mode,	 I - Which color */
		      ydpi,                  /* ydpi,	 I - Vertical resolution */
		      lwidth,      /* width, 	 I - Printed width in pixles*/
		      hoffset+microoffset,   /* offset  I - Offset from left side in x_raster_res DPI */
		      0                      /* dmt */);
	if (pd->bidirectional)
	  pd->direction = (pd->direction +1) & 1;
      }


      /*** do we have to print somthing with black or photo cartridge ? ***/
      /* we print with the photo or black cartidge */

    if (pd->jets != 208)
      {
	/* we have photo or black cartridge */
	if ((STP_ECOLOR_LC < pd->ncolors) && (lineactive[0].v[STP_ECOLOR_LC] > 0))
	  {
	    head_colors[0].line = bufs[0].v[STP_ECOLOR_LC];
	    head_colors[0].used_jets = linecount[0].v[STP_ECOLOR_LC];
	  }
	else
	  {
	    head_colors[0].line = 0;
	    head_colors[0].used_jets = 0;
	  }

	    if ((STP_ECOLOR_LM < pd->ncolors) && (lineactive[0].v[STP_ECOLOR_LM] > 0))
	  {
	    head_colors[1].line = bufs[0].v[STP_ECOLOR_LM];
	    head_colors[1].used_jets = linecount[0].v[STP_ECOLOR_LM];
	  }
	else
	  {
	    head_colors[1].line = 0;
	    head_colors[1].used_jets = 0;
	  }

	    if ((STP_ECOLOR_K < pd->ncolors) && (lineactive[0].v[STP_ECOLOR_K] > 0))
	  {
	    head_colors[2].line = bufs[0].v[STP_ECOLOR_K];
	    head_colors[2].used_jets = linecount[0].v[STP_ECOLOR_K];
	  }
	else
	  {
	    head_colors[2].line = 0;
	    head_colors[2].used_jets = 0;
	  }
      }
    else
      {
	if ((STP_ECOLOR_K < pd->ncolors) && (lineactive[0].v[STP_ECOLOR_K] > 0))
	  {
	    /* we have black cartridge; we have to print with all 208 jets at once */
	    head_colors[0].line = bufs[0].v[STP_ECOLOR_K];
	    head_colors[0].used_jets = linecount[0].v[STP_ECOLOR_K];
	    head_colors[0].head_nozzle_start = 0;
	    head_colors[0].head_nozzle_end = pd->jets/2;
	    head_colors[2].line = NULL;
	    head_colors[2].used_jets = 0;
	    head_colors[2].head_nozzle_start = 0;
	    head_colors[2].head_nozzle_end = 0;
	    head_colors[1].line = NULL;
	    head_colors[1].used_jets = 0;
	    head_colors[1].head_nozzle_start = 0;
	    head_colors[1].head_nozzle_end = 0;
	  }
	else
	  {
	    head_colors[2].line = NULL;
	    head_colors[2].used_jets = 0;
	    head_colors[2].head_nozzle_start = 0;
	    head_colors[2].head_nozzle_end = 0;
	    head_colors[1].line = NULL;
	    head_colors[1].used_jets = 0;
	    head_colors[1].head_nozzle_start = 0;
	    head_colors[1].head_nozzle_end = 0;
	    head_colors[0].line = NULL;
	    head_colors[0].used_jets = 0;
	    head_colors[0].head_nozzle_start = 0;
	    head_colors[0].head_nozzle_end = 0;
	  }
      }

     if ((head_colors[0].line != 0) || (head_colors[1].line != 0) || (head_colors[2].line != 0)) {

    lexmark_write(v,		/* I - Print file or command */
		  pd->outbuf,/*unsigned char *prnBuf,   mem block to buffer output */
		  &paperShift,           /* int *paperShift, */
		  pd->direction,             /* int direction, */
		  pd->jets,              /* num of inks to print */
		  caps,                  /* const lexmark_cap_t *   caps,     I - Printer model */
		  pd->ink_parameter,
		  xdpi,                  /* int xresolution, */
		  2,                     /* yCount,*/
		  head_colors,           /* Lexmark_head_colors *head_colors, */
		  (lwidth+7)/8, /* length,	 I - Length of bitmap data of one line in bytes */
		  prn_mode | COLOR_MODE_LC | COLOR_MODE_LM | COLOR_MODE_K,       /* mode,	 I - Which color */
		  ydpi,                  /* ydpi,	 I - Vertical resolution */
		  lwidth,      /* width, 	 I - Printed width in pixles*/
		  hoffset+microoffset,   /* offset  I - Offset from left side in x_raster_res DPI */
		  0                      /* dmt */);
    if (pd->bidirectional)
      {
	pd->direction = (pd->direction +1) & 1;
      }
    }
  /* store paper position in respect if there was a paper shift */
  pd->last_pass_offset = pass->logicalpassstart - (paperShift / (caps->y_raster_res/ydpi));

  for (j = 0; j < pd->ncolors; j++)
    {
      lineoffs[0].v[j]  = 0;
      linecount[0].v[j] = 0;
    }

  stp_deprintf(STP_DBG_LEXMARK, "lexmark_write finished\n");

}





#ifdef DEBUG

static void testprint(testdata *td)
{
  int icol, i;
  char dummy1[256], dummy2[256];
  lexmark_linebufs_t linebufs;

  /* init */
  for (i=0; i < (sizeof(linebufs.v)/sizeof(linebufs.v[0])); i++) {
    linebufs.v[i] = NULL;
  }

  /*let's go */
  td->ifile = fopen("/t1.ppm", "rb");
  if (td->ifile != NULL) {
    /* find "{" */
    fscanf(td->ifile, "%[^{]{%[^\"]\"%d %d %d %d\",", dummy1, dummy2, &(td->x), &(td->y), &(td->cols), &(td->deep));
    td->cols -= 1; /* we reduce it by one because fist color will be ignored */
    td->input_line = (char *)stp_malloc(td->x+10);
    stp_erprintf("<%s> <%s>\n", dummy1, dummy2);
    stp_erprintf("%d %d %d %d\n", td->x, td->y, td->cols, td->deep);
    if (td->cols > 16) {
      stp_erprintf("too many colors !!\n");
      return;
    }

    /* read the colors */
    fscanf(td->ifile, "%[^\"]\"%c	c %[^\"]\",", dummy1, dummy2, dummy2); /* jump over first color */
    for (icol=0; icol < td->cols; icol++) {  /* we ignor the first color. It is "no dot". */
      fscanf(td->ifile, "%[^\"]\"%c	c %[^\"]\",", dummy1, &(td->colchar[icol]), dummy2);
      stp_erprintf("colchar %d <%c>\n", i, td->colchar[icol]);
    }


    if (td->cols > 5) {
      td->cols = 7;
      for (icol=0; icol < td->cols; icol++) {  /* we ignor the first color. It is "no dot". */
	linebufs.v[icol] = (char *)stp_malloc((td->x+7)/8); /* allocate the color */
      }
    } else if (td->cols > 4) {
      td->cols = 5;
      for (icol=0; icol < td->cols; icol++) {  /* we ignor the first color. It is "no dot". */
	linebufs.v[icol] = (char *)stp_malloc((td->x+7)/8); /* allocate the color */
      }
    } else {
      td->cols = 1;
      linebufs.v[0] = (char *)stp_malloc((td->x+7)/8); /* allocate the color */
    }
  } else {
    stp_erprintf("can't open file !\n");
  }
}


static void readtestprintline(testdata *td, lexmark_linebufs_t *linebufs)
{
  char dummy1[256];
  int icol, ix;

  stp_erprintf("start readtestprintline\n");
  for (icol=0; icol < 7; icol++) {
    if (linebufs->v[icol] != NULL) {
      memset(linebufs->v[icol], 0, (td->x+7)/8);  /* clean line */
    }
  }
  stp_erprintf("1 readtestprintline cols %d\n", td->cols);


  fscanf(td->ifile, "%[^\"]\"%[^\"]\",", dummy1, td->input_line);
  for (icol=0; icol < td->cols; icol++) {
   for (ix=0; ix < td->x; ix++) {
      if (td->input_line[ix] == td->colchar[icol]) {
	/* set the dot */
	if (icol != 0) {
	  linebufs->v[icol-1][ix/8] |= 1 << (ix%8);
	} else {
	  /* this is specific, we set ymc */
	  linebufs->p.y[ix/8] |= 1 << (ix%8);
	  linebufs->p.m[ix/8] |= 1 << (ix%8);
	  linebufs->p.c[ix/8] |= 1 << (ix%8);
	}
      }
    }
  }
  /* stp_erprintf("pixchar  <%s><%s>\n",dummy1, td->input_line);*/
}

#endif


static stp_family_t print_lexmark_module_data =
  {
    &print_lexmark_printfuncs,
    NULL
  };

static int
print_lexmark_module_init(void)
{
  return stp_family_register(print_lexmark_module_data.printer_list);
}


static int
print_lexmark_module_exit(void)
{
  return stp_family_unregister(print_lexmark_module_data.printer_list);
}


/* Module header */
#define stp_module_version print_lexmark_LTX_stp_module_version
#define stp_module_data print_lexmark_LTX_stp_module_data

stp_module_version_t stp_module_version = {0, 0};

stp_module_t stp_module_data =
  {
    "lexmark",
    VERSION,
    "Lexmark family driver",
    STP_MODULE_CLASS_FAMILY,
    NULL,
    print_lexmark_module_init,
    print_lexmark_module_exit,
    (void *) &print_lexmark_module_data
  };

