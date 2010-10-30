/*
 *   Print plug-in CANON BJL driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and
 *      Andy Thaller (thaller@ph.tum.de)
 *   Copyright (c) 2006 - 2007 Sascha Sommer (saschasommer@freenet.de)
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

/* This file contains definitions for the various printmodes
*/

#ifndef GUTENPRINT_INTERNAL_CANON_MODES_H
#define GUTENPRINT_INTERNAL_CANON_MODES_H

static const char iP4200_300dpi_lum_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "1.60 1.60 1.60 1.60 1.60 1.60 1.60 1.60 "  /* B */
/* B */  "1.60 1.60 1.60 1.60 1.60 1.60 1.60 1.60 "  /* M */
/* M */  "1.60 1.60 1.55 1.50 1.45 1.40 1.35 1.35 "  /* R */
/* R */  "1.35 1.35 1.35 1.35 1.35 1.35 1.35 1.35 "  /* Y */
/* Y */  "1.35 1.42 1.51 1.58 1.60 1.60 1.60 1.60 "  /* G */
/* G */  "1.60 1.60 1.60 1.60 1.60 1.60 1.60 1.60 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

static const char iP4200_300dpi_draft_lum_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "2.13 2.15 2.20 2.25 2.30 2.35 2.40 2.40 "  /* B */
/* B */  "2.40 2.40 2.35 2.30 2.22 2.10 2.08 1.92 "  /* M */
/* M */  "1.90 1.85 1.80 1.70 1.60 1.55 1.42 1.35 "  /* R */
/* R */  "1.35 1.35 1.35 1.35 1.30 1.34 1.38 1.40 "  /* Y */
/* Y */  "1.40 1.45 1.55 1.68 1.80 1.92 2.02 2.10 "  /* G */
/* G */  "2.10 2.05 1.95 1.90 2.00 2.10 2.11 2.13 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

/* delay settings 
 sometimes the raster data has to be sent 
 | K     |
   | C     |
     | M     |
       | Y     |
 instead of 
 | K
 | C
 | M
 | Y

 following tables can be used to account for this

*/

typedef struct {
    unsigned char color;
    unsigned int delay;
} canon_delay_t;

/* delay settings for the different printmodes  last entry has to be {0,0} */
static const canon_delay_t delay_1440[] = {{'C',112},{'M',224},{'Y',336},{'c',112},{'m',224},{'y',336},{0,0}};
static const canon_delay_t delay_S200[] = {{'C',0x30},{'M',0x50},{'Y',0x70},{0,0}};



/*
 * A printmode is defined by its resolution (xdpi x ydpi), the inkset
 * and the installed printhead.
 *
 * For a hereby defined printmode we specify the density and gamma multipliers
 * and the ink definition with optional adjustments for lum, hue and sat
 *
 */
typedef struct {
  const int xdpi;                      /* horizontal resolution */
  const int ydpi;                      /* vertical resolution   */
  const unsigned int ink_types;        /* the used color channels */
  const char* name;                    /* internal name do not translate, must not contain spaces */
  const char* text;                    /* description */
  const int num_inks;
  const canon_inkset_t *inks;          /* ink definition        */
  const unsigned int flags;
#define MODE_FLAG_WEAVE 0x1            /* this mode requires weaving */
#define MODE_FLAG_EXTENDED_T 0x2       /* this mode requires extended color settings in the esc t) command */
#define MODE_FLAG_CD 0x4               /* this mode can be used to print to cds */
#define MODE_FLAG_PRO 0x8              /* special ink settings for the PIXMA Pro9500 */
#define MODE_FLAG_IP8500 0x10          /* special ink settings for the PIXMA iP8500 */
  const canon_delay_t* delay;          /* delay settings for this printmode */
  const double density;                /* density multiplier    */
  const double gamma;                  /* gamma multiplier      */
  const char *lum_adjustment;          /* optional lum adj.     */
  const char *hue_adjustment;          /* optional hue adj.     */
  const char *sat_adjustment;          /* optional sat adj.     */
  const int quality;                   /* quality (unused for some printers, default is usually 2) */
} canon_mode_t;

#define INKSET(a) sizeof(canon_##a##_inkset)/sizeof(canon_inkset_t),canon_##a##_inkset


typedef struct {
  const char *name;
  const short count;
  const short default_mode;
  const canon_mode_t *modes;
} canon_modelist_t;

#define DECLARE_MODES(name,default_mode)               \
static const canon_modelist_t name##_modelist = {      \
  #name,                                               \
  sizeof(name##_modes) / sizeof(canon_mode_t),         \
  default_mode,                                        \
  name##_modes                                         \
}


/* modelist for every printer,modes ordered by ascending resolution ink_type and quality */
static const canon_mode_t canon_BJC_30_modes[] = {
  {  180, 180,CANON_INK_K,"180x180dpi",N_("180x180 DPI"),INKSET(1_K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K,"360x360dpi",N_("360x360 DPI"),INKSET(1_K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  720, 360,CANON_INK_K,"720x360dpi",N_("720x360 DPI"),INKSET(1_K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_30,0);


static const canon_mode_t canon_BJC_85_modes[] = {
  {  360, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
              "360x360dpi",N_("360x360 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
              "360x360dmt",N_("360x360 DPI DMT"),INKSET(6_C4M4Y4K4c4m4),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  720, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
              "720x360dpi",N_("720x360 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_85,0);


/* we treat the printers that can either print in K or CMY as CMYK printers here by assigning a CMYK inkset */
static const canon_mode_t canon_BJC_210_modes[] = {
  {   90,  90,CANON_INK_K | CANON_INK_CMY,"90x90dpi",N_("90x90 DPI"),INKSET(4_C2M2Y2K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  180, 180,CANON_INK_K | CANON_INK_CMY,"180x180dpi",N_("180x180 DPI"),INKSET(4_C2M2Y2K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K | CANON_INK_CMY,"360x360dpi",N_("360x360 DPI"),INKSET(4_C2M2Y2K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  720, 360,CANON_INK_K | CANON_INK_CMY,"720x360dpi",N_("720x360 DPI"),INKSET(4_C2M2Y2K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_210,0);


static const canon_mode_t canon_BJC_240_modes[] = {
  {   90,  90,CANON_INK_K | CANON_INK_CMY,"90x90dpi",N_("90x90 DPI"),INKSET(4_C2M2Y2K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  180, 180,CANON_INK_K | CANON_INK_CMY,"180x180dpi",N_("180x180 DPI"),INKSET(4_C2M2Y2K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K | CANON_INK_CMY,"360x360dpi",N_("360x360 DPI"),INKSET(4_C2M2Y2K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K | CANON_INK_CMY,"360x360dmt",N_("360x360 DMT"),INKSET(4_C4M4Y4K4),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  720, 360,CANON_INK_K | CANON_INK_CMY,"720x360dpi",N_("720x360 DPI"),INKSET(4_C2M2Y2K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_240,0);


static const canon_mode_t canon_BJC_2000_modes[] = {
  {  180, 180,CANON_INK_CMYK,"180x180dpi",N_("180x180 DPI"),INKSET(4_C2M2Y2K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_CMYK,"360x360dpi",N_("360x360 DPI"),INKSET(4_C2M2Y2K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_2000,0);


static const canon_mode_t canon_BJC_3000_modes[] = {
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dpi",N_("360x360 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dmt",N_("360x360 DPI DMT"),INKSET(6_C4M4Y4K4c4m4),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  720, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"720x720dpi",N_("720x720 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  { 1440, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"1440x720dpi",N_("1440x720 DPI"),INKSET(6_C2M2Y2K2c2m2),0,delay_1440,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_3000,0);


static const canon_mode_t canon_BJC_4300_modes[] = {
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dpi",N_("360x360 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dmt",N_("360x360 DPI DMT"),INKSET(6_C4M4Y4K4c4m4),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  720, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"720x720dpi",N_("720x720 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  { 1440, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"1440x720dpi",N_("1440x720 DPI"),INKSET(6_C2M2Y2K2c2m2),0,delay_1440,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_4300,0);



static const canon_mode_t canon_BJC_4400_modes[] = {
  {  360, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
              "360x360dpi",N_("360x360 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  720, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
              "720x360dpi",N_("720x360 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_4400,0);


static const canon_mode_t canon_BJC_5500_modes[] = {
  {  180, 180,CANON_INK_CMYK | CANON_INK_CcMmYK,"180x180dpi",N_("180x180 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dpi",N_("360x360 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_5500,0);


static const canon_mode_t canon_BJC_6000_modes[] = {
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dpi",N_("360x360 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.8,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dmt",N_("360x360 DPI DMT"),INKSET(6_C4M4Y4K4c4m4),0,NULL,1.8,1.0,NULL,NULL,NULL,2},
  {  720, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"720x720dpi",N_("720x720 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  { 1440, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"1440x720dpi",N_("1440x720 DPI"),INKSET(6_C2M2Y2K2c2m2),0,delay_1440,0.5,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_6000,0);


static const canon_mode_t canon_BJC_7000_modes[] = {
  {  300, 300,CANON_INK_CMYK | CANON_INK_CcMmYyK,"300x300dpi",N_("300x300 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,3.5,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.8,1.0,NULL,NULL,NULL,2},
  { 1200, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"1200x600dpi",N_("1200x600 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_7000,0);

static const canon_mode_t canon_BJC_i560_modes[] = {
  {  300, 300,CANON_INK_CMYK | CANON_INK_CcMmYyK,"300x300dpi",N_("300x300 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,3.5,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.8,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_i560,0);

static const canon_mode_t canon_BJC_7100_modes[] = {
  {  300, 300,CANON_INK_CMYK | CANON_INK_CcMmYyK,"300x300dpi",N_("300x300 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  { 1200, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"1200x600dpi",N_("1200x600 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_7100,0);

static const canon_mode_t canon_BJC_i80_modes[] = {
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_i80,0);


static const canon_mode_t canon_BJC_8200_modes[] = {
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  { 1200,1200,CANON_INK_CMYK,"1200x1200dpi",N_("1200x1200 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_8200,0);


static const canon_mode_t canon_BJC_8500_modes[] = {
  {  300, 300,CANON_INK_CMYK | CANON_INK_CcMmYK,"300x300dpi",N_("300x300 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK | CANON_INK_CcMmYK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_8500,0);


static const canon_mode_t canon_S200_modes[] = {
  {  360, 360,CANON_INK_CMYK | CANON_INK_CMY | CANON_INK_K,
              "360x360dpi",N_("360x360 DPI"),INKSET(4_C2M2Y2K2),0,delay_S200,2.0,1.0,NULL,NULL,NULL,2},
  {  720, 720,CANON_INK_CMYK | CANON_INK_CMY | CANON_INK_K,
              "720x720dpi",N_("720x720 DPI"),INKSET(4_C2M2Y2K2),MODE_FLAG_WEAVE,delay_S200,1.0,1.0,NULL,NULL,NULL,2},
  { 1440, 720,CANON_INK_CMYK | CANON_INK_CMY | CANON_INK_K,
              "1440x720dpi",N_("1440x720 DPI"),INKSET(4_C2M2Y2K2),MODE_FLAG_WEAVE,delay_S200,0.5,1.0,NULL,NULL,NULL,2},
  { 1440,1440,CANON_INK_CMYK | CANON_INK_CMY | CANON_INK_K,
              "1440x1440dpi",N_("1440x1440 DPI"),INKSET(4_C2M2Y2K2),MODE_FLAG_WEAVE,delay_S200,0.3,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_S200,0);

static const canon_mode_t canon_S500_modes[] = {
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C3M3Y2K2),MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
/*  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2),MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, */
};
DECLARE_MODES(canon_S500,2);

static const canon_mode_t canon_PIXMA_iP2000_modes[] = {
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP2000,0);


static const canon_mode_t canon_PIXMA_iP3000_modes[] = {
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2_c),MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP3000,1);


static const canon_mode_t canon_PIXMA_iP4000_modes[] = {
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(4_C2M2Y2K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2k3_c),MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
/*  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4k4),MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL},*/ /* this mode is used for CD printing, K is ignored by the printer then, the seperation between the small and large dot inks needs more work */
/*  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_superphoto",N_("600x600 DPI Superphoto"),INKSET(9_C8M8Y8c16m16k8),MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL}, */
};
DECLARE_MODES(canon_PIXMA_iP4000,2);


static const canon_mode_t canon_PIXMA_iP4200_modes[] = {
  /* Q0 - fastest mode (in windows driver it's Q5, printer uses 50% of ink ( I think )) */
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,iP4200_300dpi_draft_lum_adjustment,NULL,NULL,0},
  /* Q1 - normal 300x300 mode (in windows driver it's Q4 - normal darkness of printout ) */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,iP4200_300dpi_lum_adjustment,NULL,NULL,1},
  /* Q2 - standard mode for this driver (in windows driver it's Q3) */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2k3_c),MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
 /* {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4k4),MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL}, */
};
DECLARE_MODES(canon_PIXMA_iP4200,2);

static const canon_mode_t canon_PIXMA_iP6000_modes[] = {
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y3K3),MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP6000,0);

static const canon_mode_t canon_PIXMA_iP6700_modes[] = {
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(19_C3M3Y3k3),MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP6700,0);


static const canon_mode_t canon_MULTIPASS_MP150_modes[] = {
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP150,0);

static const canon_mode_t canon_PIXMA_iP5300_modes[] = {
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2k3_c),MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP5300,0);

static const canon_mode_t canon_MULTIPASS_MP830_modes[] = {
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(4_C2M2Y2K2),0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP830,0);

static const canon_mode_t canon_MULTIPASS_MP970_modes[] = {
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(7_C4M4Y4c4m4k4K4),MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_MULTIPASS_MP970,0);

static const canon_mode_t canon_PIXMA_iX5000_modes[] = {
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2_c),MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iX5000,0);

static const canon_mode_t canon_PIXMA_Pro9500_modes[] = {
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(11_C2M2Y2K2),MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_Pro9500,0);

static const canon_mode_t canon_PIXMA_iP8500_modes[] = {
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(11_C2M2Y2K2),MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP8500,0);

#endif

