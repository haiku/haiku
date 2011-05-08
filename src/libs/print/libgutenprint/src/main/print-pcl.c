/*
 * "$Id: print-pcl.c,v 1.160 2010/12/05 21:38:15 rlk Exp $"
 *
 *   Print plug-in HP PCL driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and
 *      Dave Hill (dave@minnie.demon.co.uk)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include <gutenprint/gutenprint-intl-internal.h>
#include "gutenprint-internal.h"
#include <stdio.h>
#include <string.h>

/* #define DEBUG */
/* #define PCL_DEBUG_DISABLE_BLANKLINE_REMOVAL */

/*
 * Local functions...
 */
static void	pcl_mode0(stp_vars_t *, unsigned char *, int, int);
static void	pcl_mode2(stp_vars_t *, unsigned char *, int, int);

#ifndef MAX
#  define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif /* !MAX */

typedef struct
{
  int do_blank;
  int blank_lines;
  unsigned char *comp_buf;
  void (*writefunc)(stp_vars_t *, unsigned char *, int, int);	/* PCL output function */
  int do_cret;
  int do_cretb;
  int do_6color;
  int height;
  int duplex;
  int tumble;
  int use_crd;
} pcl_privdata_t;

/*
 * Generic define for a name/value set
 */

typedef struct
{
  const char	*pcl_name;
  const char	*pcl_text;
  int		pcl_code;
  int		p0;
  int		p1;
} pcl_t;

static const stp_dotsize_t single_dotsize[] =
{
  { 0x1, 1.0 }
};

static const stp_shade_t photo_dither_shades[] =
{
  { 1.0000, 1, single_dotsize },
  { 0.3333, 1, single_dotsize },
};

/*
 * Media size to PCL media size code table
 *
 * Note, you can force the list of papersizes given in the GUI to be only those
 * supported by defining PCL_NO_CUSTOM_PAPERSIZES
 */

/* #define PCL_NO_CUSTOM_PAPERSIZES */

#define PCL_PAPERSIZE_EXECUTIVE		1
#define PCL_PAPERSIZE_LETTER		2
#define PCL_PAPERSIZE_LEGAL		3
#define PCL_PAPERSIZE_TABLOID		6	/* "Ledger" */
#define PCL_PAPERSIZE_STATEMENT		15	/* Called "Manual" in print-util */
#define PCL_PAPERSIZE_SUPER_B		16	/* Called "13x19" in print-util */
#define PCL_PAPERSIZE_A5		25
#define PCL_PAPERSIZE_A4		26
#define PCL_PAPERSIZE_A3		27
#define PCL_PAPERSIZE_JIS_B5		45
#define PCL_PAPERSIZE_JIS_B4		46
#define PCL_PAPERSIZE_HAGAKI_CARD	71
#define PCL_PAPERSIZE_OUFUKU_CARD	72
#define PCL_PAPERSIZE_A6_CARD		73
#define PCL_PAPERSIZE_4x6		74
#define PCL_PAPERSIZE_5x8		75
#define PCL_PAPERSIZE_3x5		78
#define PCL_PAPERSIZE_MONARCH_ENV	80
#define PCL_PAPERSIZE_COMMERCIAL10_ENV	81
#define PCL_PAPERSIZE_DL_ENV		90
#define PCL_PAPERSIZE_C5_ENV		91
#define PCL_PAPERSIZE_C6_ENV		92
#define PCL_PAPERSIZE_CUSTOM		101	/* Custom size */
#define PCL_PAPERSIZE_INVITATION_ENV	109
#define PCL_PAPERSIZE_JAPANESE_3_ENV	110
#define PCL_PAPERSIZE_JAPANESE_4_ENV	111
#define PCL_PAPERSIZE_KAKU_ENV		113
#define PCL_PAPERSIZE_HP_CARD		114	/* HP Greeting card!?? */

/*
 * This data comes from the HP documentation "Deskjet 1220C and 1120C
 * PCL reference guide 2.0, Nov 1999". NOTE: The first name *must* match
 * those in print-util.c for the lookups to work properly!
 * The long names are not used so they have been removed, the ones in
 * print-util.c are used in the interface.
 */

static const pcl_t pcl_media_sizes[] =
{
    { "Executive", "notused", PCL_PAPERSIZE_EXECUTIVE},			/* US Exec (7.25 x 10.5 in) */
    { "Letter", "notused", PCL_PAPERSIZE_LETTER},			/* US Letter (8.5 x 11 in) */
    { "Legal", "notused", PCL_PAPERSIZE_LEGAL},				/* US Legal (8.5 x 14 in) */
    { "Tabloid", "notused", PCL_PAPERSIZE_TABLOID},			/* US Tabloid (11 x 17 in) */
    { "Statement", "notused", PCL_PAPERSIZE_STATEMENT},			/* US Manual/Statement (5.5 x 8.5 in) */
    { "SuperB", "notused", PCL_PAPERSIZE_SUPER_B},			/* US 13x19/Super B (13 x 19 in) */
    { "A5", "notused", PCL_PAPERSIZE_A5},				/* ISO/JIS A5 (148 x 210 mm) */
    { "A4", "notused", PCL_PAPERSIZE_A4},				/* ISO/JIS A4 (210 x 297 mm) */
    { "A3", "notused", PCL_PAPERSIZE_A3},				/* ISO/JIS A3 (297 x 420 mm) */
    { "B5", "notused", PCL_PAPERSIZE_JIS_B5},				/* JIS B5 (182 x 257 mm). */
    { "B4", "notused", PCL_PAPERSIZE_JIS_B4},				/* JIS B4 (257 x 364 mm). */
    { "w283h420", "notused", PCL_PAPERSIZE_HAGAKI_CARD},		/* Japanese Hagaki Card (100 x 148 mm) */
    { "w420h567", "notused", PCL_PAPERSIZE_OUFUKU_CARD},		/* Japanese Oufuku Card (148 x 200 mm) */
    { "A6", "notused", PCL_PAPERSIZE_A6_CARD},				/* ISO/JIS A6 card */
    { "w288h432", "notused", PCL_PAPERSIZE_4x6},			/* US Index card (4 x 6 in) */
    { "w360h576", "notused", PCL_PAPERSIZE_5x8},			/* US Index card (5 x 8 in) */
    { "w216h360", "notused", PCL_PAPERSIZE_3x5},			/* US Index card (3 x 5 in) */
    { "Monarch", "notused", PCL_PAPERSIZE_MONARCH_ENV},			/* Monarch Envelope (3 7/8 x 7 1/2 in) */
    { "COM10", "notused", PCL_PAPERSIZE_COMMERCIAL10_ENV},		/* US Commercial 10 Envelope (4.125 x 9.5 in) Portrait */
    { "DL", "notused", PCL_PAPERSIZE_DL_ENV},				/* DL envelope (110 x 220 mm) Portrait */
    { "C5", "notused", PCL_PAPERSIZE_C5_ENV},				/* C5 envelope (162 x 229 mm) */
    { "C6", "notused", PCL_PAPERSIZE_C6_ENV},				/* C6 envelope (114 x 162 mm) */
    { "w315h414", "notused", PCL_PAPERSIZE_INVITATION_ENV},		/* US A2 Invitation envelope (4 3/8 x 5 3/4 in) */
    { "w340h666", "notused", PCL_PAPERSIZE_JAPANESE_3_ENV},		/* Japanese Long Envelope #3 (120 x 235 mm) */
    { "w255h581", "notused", PCL_PAPERSIZE_JAPANESE_4_ENV},		/* Japanese Long Envelope #4 (90 x 205 mm) */
    { "w680h941", "notused", PCL_PAPERSIZE_KAKU_ENV},			/* Japanese Kaku Envelope (240 x 332.1 mm) */
/**** MRS: this size not supported by print-util funcs! ****/
    { "w612h792", "notused", PCL_PAPERSIZE_HP_CARD}, 			/* Hp greeting card */
};
#define NUM_PRINTER_PAPER_SIZES	(sizeof(pcl_media_sizes) / sizeof(pcl_t))

/*
 * Media type to code table
 */

#define PCL_PAPERTYPE_PLAIN	0
#define PCL_PAPERTYPE_BOND	1
#define PCL_PAPERTYPE_PREMIUM	2
#define PCL_PAPERTYPE_GLOSSY	3	/* or photo */
#define PCL_PAPERTYPE_TRANS	4
#define PCL_PAPERTYPE_QPHOTO	5	/* Quick dry photo (2000 only) */
#define PCL_PAPERTYPE_QTRANS	6	/* Quick dry transparency (2000 only) */

static const pcl_t pcl_media_types[] =
{
    { "Plain", N_ ("Plain"), PCL_PAPERTYPE_PLAIN},
    { "Bond", N_ ("Bond"), PCL_PAPERTYPE_BOND},
    { "Premium", N_ ("Premium"), PCL_PAPERTYPE_PREMIUM},
    { "Glossy", N_ ("Glossy Photo"), PCL_PAPERTYPE_GLOSSY},
    { "Transparency", N_ ("Transparency"), PCL_PAPERTYPE_TRANS},
    { "GlossyQD", N_ ("Quick-dry Photo"), PCL_PAPERTYPE_QPHOTO},
    { "TransparencyQD", N_ ("Quick-dry Transparency"), PCL_PAPERTYPE_QTRANS},
};
#define NUM_PRINTER_PAPER_TYPES	(sizeof(pcl_media_types) / sizeof(pcl_t))

/*
 * Media feed to code table. There are different names for the same code,
 * so we encode them by adding "lumps" of "PAPERSOURCE_MOD".
 * This is removed later to get back to the main codes.
 */

#define PAPERSOURCE_MOD			16
#define PAPERSOURCE_340_MOD		(PAPERSOURCE_MOD)
#define PAPERSOURCE_DJ_MOD		(PAPERSOURCE_MOD << 1)
#define PAPERSOURCE_ADJ_GUIDE		(PAPERSOURCE_MOD << 2)

#define PCL_PAPERSOURCE_STANDARD	0	/* Don't output code */
#define PCL_PAPERSOURCE_MANUAL		2
#define PCL_PAPERSOURCE_MANUAL_ADJ	(PCL_PAPERSOURCE_MANUAL + PAPERSOURCE_ADJ_GUIDE)
#define PCL_PAPERSOURCE_ENVELOPE	3	/* Not used */

/* LaserJet types */
#define PCL_PAPERSOURCE_LJ_TRAY2	1
#define PCL_PAPERSOURCE_LJ_TRAY3	4
#define PCL_PAPERSOURCE_LJ_TRAY4	5
#define PCL_PAPERSOURCE_LJ_TRAY1	8
#define PCL_PAPERSOURCE_LJ_TRAY1_ADJ	(PCL_PAPERSOURCE_LJ_TRAY1 + PAPERSOURCE_ADJ_GUIDE)
#define PCL_PAPERSOURCE_LJ_TRAY2_ADJ	(PCL_PAPERSOURCE_LJ_TRAY2 + PAPERSOURCE_ADJ_GUIDE)
#define PCL_PAPERSOURCE_LJ_TRAY3_ADJ	(PCL_PAPERSOURCE_LJ_TRAY3 + PAPERSOURCE_ADJ_GUIDE)
#define PCL_PAPERSOURCE_LJ_TRAY4_ADJ	(PCL_PAPERSOURCE_LJ_TRAY4 + PAPERSOURCE_ADJ_GUIDE)

/* Deskjet 340 types */
#define PCL_PAPERSOURCE_340_PCSF	(1 + PAPERSOURCE_340_MOD)
						/* Portable sheet feeder for 340 */
#define PCL_PAPERSOURCE_340_DCSF	(4 + PAPERSOURCE_340_MOD)
						/* Desktop sheet feeder for 340 */

/* Other Deskjet types */
#define PCL_PAPERSOURCE_DJ_TRAY		(1 + PAPERSOURCE_DJ_MOD)
#define PCL_PAPERSOURCE_DJ_TRAY2	(4 + PAPERSOURCE_DJ_MOD)
						/* Tray 2 for 2500 */
#define PCL_PAPERSOURCE_DJ_OPTIONAL	(5 + PAPERSOURCE_DJ_MOD)
						/* Optional source for 2500 */
#define PCL_PAPERSOURCE_DJ_AUTO		(7 + PAPERSOURCE_DJ_MOD)
						/* Autoselect for 2500 */

static const pcl_t pcl_media_sources[] =
{
    { "Standard", N_ ("Standard"), PCL_PAPERSOURCE_STANDARD},
    { "Manual", N_ ("Manual"), PCL_PAPERSOURCE_MANUAL},
    { "ManualAdj", N_ ("Manual - Movable Guides"), PCL_PAPERSOURCE_MANUAL_ADJ},
/*  {"Envelope", PCL_PAPERSOURCE_ENVELOPE}, */
    { "MultiPurposeAdj", N_ ("Tray 1 - Movable Guides"), PCL_PAPERSOURCE_LJ_TRAY1_ADJ},
    { "MultiPurpose", N_ ("Tray 1"), PCL_PAPERSOURCE_LJ_TRAY1},
    { "UpperAdj", N_ ("Tray 2 - Movable Guides"), PCL_PAPERSOURCE_LJ_TRAY2_ADJ},
    { "Upper", N_ ("Tray 2"), PCL_PAPERSOURCE_LJ_TRAY2},
    { "LowerAdj", N_ ("Tray 3 - Movable Guides"), PCL_PAPERSOURCE_LJ_TRAY3_ADJ},
    { "Lower", N_ ("Tray 3"), PCL_PAPERSOURCE_LJ_TRAY3},
    { "LargeCapacityAdj", N_ ("Tray 4 - Movable Guides"), PCL_PAPERSOURCE_LJ_TRAY4_ADJ},
    { "LargeCapacity", N_ ("Tray 4"), PCL_PAPERSOURCE_LJ_TRAY4},
    { "Portable", N_ ("Portable Sheet Feeder"), PCL_PAPERSOURCE_340_PCSF},
    { "Desktop", N_ ("Desktop Sheet Feeder"), PCL_PAPERSOURCE_340_DCSF},
    { "Tray", N_ ("Tray"), PCL_PAPERSOURCE_DJ_TRAY},
    { "Tray2", N_ ("Tray 2"), PCL_PAPERSOURCE_DJ_TRAY2},
    { "Optional", N_ ("Optional Source"), PCL_PAPERSOURCE_DJ_OPTIONAL},
    { "Auto", N_ ("Autoselect"), PCL_PAPERSOURCE_DJ_AUTO},
};
#define NUM_PRINTER_PAPER_SOURCES	(sizeof(pcl_media_sources) / sizeof(pcl_t))

#define PCL_RES_150_150		1
#define PCL_RES_300_300		2
#define PCL_RES_600_300		4	/* DJ 600 series */
#define PCL_RES_600_600_MONO	8	/* DJ 600/800/1100/2000 b/w only */
#define PCL_RES_600_600		16	/* DJ 9xx/1220C/PhotoSmart */
#define PCL_RES_1200_600	32	/* DJ 9xx/1220C/PhotoSmart */
#define PCL_RES_2400_600	64	/* DJ 9xx/1220C/PhotoSmart */

static const pcl_t pcl_resolutions[] =
{
    { "150dpi", N_("150x150 DPI"), PCL_RES_150_150, 150, 150},
    { "300dpi", N_("300x300 DPI"), PCL_RES_300_300, 300, 300},
    { "600x300dpi", N_("600x300 DPI"), PCL_RES_600_300, 600, 300},
    { "600mono", N_("600x600 DPI monochrome"), PCL_RES_600_600_MONO, 600, 600},
    { "600dpi", N_("600x600 DPI"), PCL_RES_600_600, 600, 600},
    { "1200x600dpi", N_("1200x600 DPI"), PCL_RES_1200_600, 1200, 600},
    { "2400x600dpi", N_("2400x600 DPI"), PCL_RES_2400_600, 2400, 600},
};
#define NUM_RESOLUTIONS		(sizeof(pcl_resolutions) / sizeof (pcl_t))

static const pcl_t pcl_qualities[] =
{
    { "Draft", N_("Draft"), PCL_RES_150_150, 150, 150},
    { "Standard", N_("Standard"), PCL_RES_300_300, 300, 300},
    { "High", N_("High"), PCL_RES_600_600, 600, 600},
    { "High", N_("Standard"), PCL_RES_600_300, 600, 300},
    { "Photo", N_("Photo"), PCL_RES_1200_600, 1200, 600},
    { "Photo", N_("Photo"), PCL_RES_2400_600, 2400, 600},
};
#define NUM_QUALITIES		(sizeof(pcl_qualities) / sizeof (pcl_t))

typedef struct {
  int top_margin;
  int bottom_margin;
  int left_margin;
  int right_margin;
} margins_t;

/*
 * Printer capability data
 */

typedef struct {
  int model;
  int custom_max_width;
  int custom_max_height;
  int custom_min_width;
  int custom_min_height;
  int resolutions;
  margins_t normal_margins;
  margins_t a4_margins;
  int color_type;		/* 2 print head or one, 2 level or 4 */
  int stp_printer_type;		/* Deskjet/Laserjet and quirks */
/* The paper size, paper type and paper source codes cannot be combined */
  const short *paper_sizes;	/* Paper sizes */
  const short *paper_types;	/* Paper types */
  const short *paper_sources;	/* Paper sources */
  } pcl_cap_t;

#define PCL_COLOR_NONE		0
#define PCL_COLOR_CMY		1	/* One print head */
#define PCL_COLOR_CMYK		2	/* Two print heads */
#define PCL_COLOR_CMYK4		4	/* CRet printing */
#define PCL_COLOR_CMYKcm	8	/* CMY + Photo Cart */
#define PCL_COLOR_CMYK4b	16	/* CRet for HP840c */

#define PCL_PRINTER_LJ		1
#define PCL_PRINTER_DJ		2
#define PCL_PRINTER_NEW_ERG	4	/* use "\033*rC" to end raster graphics,
					   instead of "\033*rB" */
#define PCL_PRINTER_TIFF	8	/* Use TIFF compression */
#define PCL_PRINTER_MEDIATYPE	16	/* Use media type & print quality */
#define PCL_PRINTER_CUSTOM_SIZE	32	/* Custom sizes supported */
#define PCL_PRINTER_BLANKLINE	64	/* Blank line removal supported */
#define PCL_PRINTER_DUPLEX	128	/* Printer can have duplexer */

/*
 * FIXME - the 520 shouldn't be lumped in with the 500 as it supports
 * more paper sizes.
 *
 * The following models use depletion, raster quality and shingling:-
 * 500, 500c, 510, 520, 550c, 560c.
 * The rest use Media Type and Print Quality.
 *
 * This data comes from the HP documentation "Deskjet 1220C and 1120C
 * PCL reference guide 2.0, Nov 1999".
 */

static const short emptylist[] =
{
  -1
};

static const short standard_papersizes[] =
{
  PCL_PAPERSIZE_EXECUTIVE,
  PCL_PAPERSIZE_STATEMENT,
  PCL_PAPERSIZE_LETTER,
  PCL_PAPERSIZE_LEGAL,
  PCL_PAPERSIZE_A4,
  -1,
};

static const short letter_only_papersizes[] =
{
  PCL_PAPERSIZE_LETTER,
  -1
};

static const short letter_a4_papersizes[] =
{
  PCL_PAPERSIZE_A4,
  PCL_PAPERSIZE_LETTER,
  -1
};

static const short dj340_papersizes[] =
{
  PCL_PAPERSIZE_EXECUTIVE,
  PCL_PAPERSIZE_LETTER,
  PCL_PAPERSIZE_LEGAL,
  PCL_PAPERSIZE_A4,
  -1,
};

static const short dj400_papersizes[] =
{
  PCL_PAPERSIZE_EXECUTIVE,
  PCL_PAPERSIZE_LETTER,
  PCL_PAPERSIZE_LEGAL,
  PCL_PAPERSIZE_A4,
  PCL_PAPERSIZE_JIS_B5,
  -1,
};

static const short dj500_papersizes[] =
{
/*    PCL_PAPERSIZE_EXECUTIVE, The 500 doesn't support this, the 520 does */
  PCL_PAPERSIZE_LETTER,
  PCL_PAPERSIZE_LEGAL,
  PCL_PAPERSIZE_A4,
  PCL_PAPERSIZE_COMMERCIAL10_ENV,
/*    PCL_PAPERSIZE_DL_ENV,    The 500 doesn't support this, the 520 does */
  -1,
};

static const short dj540_papersizes[] =
{
  PCL_PAPERSIZE_EXECUTIVE,
  PCL_PAPERSIZE_LETTER,
  PCL_PAPERSIZE_LEGAL,
  PCL_PAPERSIZE_A4,
  PCL_PAPERSIZE_A5,
  PCL_PAPERSIZE_JIS_B5,
  PCL_PAPERSIZE_HAGAKI_CARD,
  PCL_PAPERSIZE_A6_CARD,
  PCL_PAPERSIZE_4x6,
  PCL_PAPERSIZE_5x8,
  PCL_PAPERSIZE_COMMERCIAL10_ENV,
  PCL_PAPERSIZE_DL_ENV,
  PCL_PAPERSIZE_C6_ENV,
  -1,
};

static const short dj600_papersizes[] =
{
  PCL_PAPERSIZE_EXECUTIVE,
  PCL_PAPERSIZE_LETTER,
  PCL_PAPERSIZE_LEGAL,
  PCL_PAPERSIZE_A4,
  PCL_PAPERSIZE_A5,
  PCL_PAPERSIZE_HAGAKI_CARD,
  PCL_PAPERSIZE_A6_CARD,
  PCL_PAPERSIZE_4x6,
  PCL_PAPERSIZE_5x8,
  PCL_PAPERSIZE_COMMERCIAL10_ENV,
  PCL_PAPERSIZE_DL_ENV,
  PCL_PAPERSIZE_C6_ENV,
  PCL_PAPERSIZE_INVITATION_ENV,
  -1,
};

static const short dj1220_papersizes[] =
{
  PCL_PAPERSIZE_EXECUTIVE,
  PCL_PAPERSIZE_LETTER,
  PCL_PAPERSIZE_LEGAL,
  PCL_PAPERSIZE_TABLOID,
  PCL_PAPERSIZE_STATEMENT,
  PCL_PAPERSIZE_SUPER_B,
  PCL_PAPERSIZE_A5,
  PCL_PAPERSIZE_A4,
  PCL_PAPERSIZE_A3,
  PCL_PAPERSIZE_JIS_B5,
  PCL_PAPERSIZE_JIS_B4,
  PCL_PAPERSIZE_HAGAKI_CARD,
  PCL_PAPERSIZE_OUFUKU_CARD,
  PCL_PAPERSIZE_A6_CARD,
  PCL_PAPERSIZE_4x6,
  PCL_PAPERSIZE_5x8,
  PCL_PAPERSIZE_3x5,
  PCL_PAPERSIZE_HP_CARD,
  PCL_PAPERSIZE_MONARCH_ENV,
  PCL_PAPERSIZE_COMMERCIAL10_ENV,
  PCL_PAPERSIZE_DL_ENV,
  PCL_PAPERSIZE_C5_ENV,
  PCL_PAPERSIZE_C6_ENV,
  PCL_PAPERSIZE_INVITATION_ENV,
  PCL_PAPERSIZE_JAPANESE_3_ENV,
  PCL_PAPERSIZE_JAPANESE_4_ENV,
  PCL_PAPERSIZE_KAKU_ENV,
  -1,
};

static const short dj1100_papersizes[] =
{
  PCL_PAPERSIZE_EXECUTIVE,
  PCL_PAPERSIZE_LETTER,
  PCL_PAPERSIZE_LEGAL,
  PCL_PAPERSIZE_TABLOID,
  PCL_PAPERSIZE_STATEMENT,
  PCL_PAPERSIZE_SUPER_B,
  PCL_PAPERSIZE_A5,
  PCL_PAPERSIZE_A4,
  PCL_PAPERSIZE_A3,
  PCL_PAPERSIZE_JIS_B5,
  PCL_PAPERSIZE_JIS_B4,
  PCL_PAPERSIZE_HAGAKI_CARD,
  PCL_PAPERSIZE_A6_CARD,
  PCL_PAPERSIZE_4x6,
  PCL_PAPERSIZE_5x8,
  PCL_PAPERSIZE_COMMERCIAL10_ENV,
  PCL_PAPERSIZE_DL_ENV,
  PCL_PAPERSIZE_C6_ENV,
  PCL_PAPERSIZE_INVITATION_ENV,
  PCL_PAPERSIZE_JAPANESE_3_ENV,
  PCL_PAPERSIZE_JAPANESE_4_ENV,
  PCL_PAPERSIZE_KAKU_ENV,
  -1,
};

static const short dj1200_papersizes[] =
{
  /* This printer is not mentioned in the Comparison tables,
     so I'll just pick some likely sizes... */
  PCL_PAPERSIZE_EXECUTIVE,
  PCL_PAPERSIZE_LETTER,
  PCL_PAPERSIZE_LEGAL,
  PCL_PAPERSIZE_A5,
  PCL_PAPERSIZE_A4,
  PCL_PAPERSIZE_4x6,
  PCL_PAPERSIZE_5x8,
  -1,
};

static const short dj2000_papersizes[] =
{
  PCL_PAPERSIZE_EXECUTIVE,
  PCL_PAPERSIZE_LETTER,
  PCL_PAPERSIZE_LEGAL,
  PCL_PAPERSIZE_A5,
  PCL_PAPERSIZE_A4,
  PCL_PAPERSIZE_HAGAKI_CARD,
  PCL_PAPERSIZE_A6_CARD,
  PCL_PAPERSIZE_4x6,
  PCL_PAPERSIZE_5x8,
  PCL_PAPERSIZE_3x5,
  PCL_PAPERSIZE_COMMERCIAL10_ENV,
  PCL_PAPERSIZE_DL_ENV,
  PCL_PAPERSIZE_C6_ENV,
  PCL_PAPERSIZE_INVITATION_ENV,
  -1,
};

static const short dj2500_papersizes[] =
{
  PCL_PAPERSIZE_EXECUTIVE,
  PCL_PAPERSIZE_LETTER,
  PCL_PAPERSIZE_LEGAL,
  PCL_PAPERSIZE_TABLOID,
  PCL_PAPERSIZE_STATEMENT,
  PCL_PAPERSIZE_A5,
  PCL_PAPERSIZE_A4,
  PCL_PAPERSIZE_A3,
  PCL_PAPERSIZE_JIS_B5,
  PCL_PAPERSIZE_JIS_B4,
  PCL_PAPERSIZE_HAGAKI_CARD,
  PCL_PAPERSIZE_A6_CARD,
  PCL_PAPERSIZE_4x6,
  PCL_PAPERSIZE_5x8,
  PCL_PAPERSIZE_COMMERCIAL10_ENV,
  PCL_PAPERSIZE_DL_ENV,
  -1,
};

static const short ljsmall_papersizes[] =
{
  PCL_PAPERSIZE_EXECUTIVE,
  PCL_PAPERSIZE_STATEMENT,
  PCL_PAPERSIZE_LETTER,
  PCL_PAPERSIZE_LEGAL,
  PCL_PAPERSIZE_A4,
  PCL_PAPERSIZE_MONARCH_ENV,
  PCL_PAPERSIZE_COMMERCIAL10_ENV,
  PCL_PAPERSIZE_DL_ENV,
  PCL_PAPERSIZE_C5_ENV,
  PCL_PAPERSIZE_C6_ENV,
  -1,
};

static const short ljbig_papersizes[] =
{
  PCL_PAPERSIZE_EXECUTIVE,
  PCL_PAPERSIZE_STATEMENT,
  PCL_PAPERSIZE_LETTER,
  PCL_PAPERSIZE_LEGAL,
  PCL_PAPERSIZE_TABLOID,
  PCL_PAPERSIZE_SUPER_B,
  PCL_PAPERSIZE_A5,
  PCL_PAPERSIZE_A4,
  PCL_PAPERSIZE_A3,
  PCL_PAPERSIZE_JIS_B5,
  PCL_PAPERSIZE_JIS_B4,                /* Guess */
  PCL_PAPERSIZE_4x6,
  PCL_PAPERSIZE_5x8,
  PCL_PAPERSIZE_MONARCH_ENV,
  PCL_PAPERSIZE_COMMERCIAL10_ENV,
  PCL_PAPERSIZE_DL_ENV,
  PCL_PAPERSIZE_C5_ENV,
  PCL_PAPERSIZE_C6_ENV,
  -1,
};

static const short ljtabloid_papersizes[] =
{
  PCL_PAPERSIZE_EXECUTIVE,
  PCL_PAPERSIZE_STATEMENT,
  PCL_PAPERSIZE_LETTER,
  PCL_PAPERSIZE_LEGAL,
  PCL_PAPERSIZE_TABLOID,
  PCL_PAPERSIZE_A5,
  PCL_PAPERSIZE_A4,
  PCL_PAPERSIZE_A3,
  PCL_PAPERSIZE_JIS_B5,
  PCL_PAPERSIZE_JIS_B4,                /* Guess */
  PCL_PAPERSIZE_4x6,
  PCL_PAPERSIZE_5x8,
  PCL_PAPERSIZE_MONARCH_ENV,
  PCL_PAPERSIZE_COMMERCIAL10_ENV,
  PCL_PAPERSIZE_DL_ENV,
  PCL_PAPERSIZE_C5_ENV,
  PCL_PAPERSIZE_C6_ENV,
  -1,
};

static const short basic_papertypes[] =
{
  PCL_PAPERTYPE_PLAIN,
  PCL_PAPERTYPE_BOND,
  PCL_PAPERTYPE_PREMIUM,
  PCL_PAPERTYPE_GLOSSY,
  PCL_PAPERTYPE_TRANS,
  -1,
};

static const short new_papertypes[] =
{
  PCL_PAPERTYPE_PLAIN,
  PCL_PAPERTYPE_BOND,
  PCL_PAPERTYPE_PREMIUM,
  PCL_PAPERTYPE_GLOSSY,
  PCL_PAPERTYPE_TRANS,
  PCL_PAPERTYPE_QPHOTO,
  PCL_PAPERTYPE_QTRANS,
  -1,
};

static const short laserjet_papersources[] =
{
  PCL_PAPERSOURCE_STANDARD,
  PCL_PAPERSOURCE_MANUAL_ADJ,
  PCL_PAPERSOURCE_MANUAL,
  PCL_PAPERSOURCE_LJ_TRAY1_ADJ,
  PCL_PAPERSOURCE_LJ_TRAY1,
  PCL_PAPERSOURCE_LJ_TRAY2_ADJ,
  PCL_PAPERSOURCE_LJ_TRAY2,
  PCL_PAPERSOURCE_LJ_TRAY3_ADJ,
  PCL_PAPERSOURCE_LJ_TRAY3,
  PCL_PAPERSOURCE_LJ_TRAY4_ADJ,
  PCL_PAPERSOURCE_LJ_TRAY4,
  -1,
};

static const short dj340_papersources[] =
{
  PCL_PAPERSOURCE_STANDARD,
  PCL_PAPERSOURCE_MANUAL,
  PCL_PAPERSOURCE_340_PCSF,
  PCL_PAPERSOURCE_340_DCSF,
  -1,
};

static const short dj_papersources[] =
{
  PCL_PAPERSOURCE_STANDARD,
  PCL_PAPERSOURCE_MANUAL,
  PCL_PAPERSOURCE_DJ_TRAY,
  -1,
};

static const short dj2500_papersources[] =
{
  PCL_PAPERSOURCE_STANDARD,
  PCL_PAPERSOURCE_MANUAL,
  PCL_PAPERSOURCE_DJ_AUTO,
  PCL_PAPERSOURCE_DJ_TRAY,
  PCL_PAPERSOURCE_DJ_TRAY2,
  PCL_PAPERSOURCE_DJ_OPTIONAL,
  -1,
};

static const short standard_papersources[] =
{
  PCL_PAPERSOURCE_STANDARD,
  -1
};

static const pcl_cap_t pcl_model_capabilities[] =
{
  /* Default/unknown printer - assume laserjet */
  { 0,
    17 * 72 / 2, 14 * 72,		/* Max paper size */
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,	/* Resolutions */
    {12, 12, 18, 18},			/* non-A4 Margins */
    {12, 12, 10, 10},			/* A4 Margins */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ,
    standard_papersizes,
    emptylist,
    emptylist,
  },
  /* DesignJet 230/430 (monochrome ) */
  { 10230,
    36 * 72, 150 * 12 * 72,		/* 150ft in roll mode, 64" in sheet */
    826 * 72 / 100, 583 * 72 / 100,	/* 8.3" wide min in sheet mode */
    PCL_RES_300_300 | PCL_RES_600_600,
    {49, 49, 15, 15},
    {49, 49, 15, 15},
    PCL_COLOR_NONE,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE | PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_NEW_ERG,
    letter_a4_papersizes,
    basic_papertypes,
    standard_papersources,
  },
  /* DesignJet 250C/450C/455CA/488CA */
  /* The "CA" versions have a "software RIP" but are the same hardware */
  { 10250,
    36 * 72, 150 * 12 * 72,		/* 150ft in roll mode, 64" in sheet */
    826 * 72 / 100, 583 * 72 / 100,	/* 8.3" wide min in sheet mode */
    PCL_RES_300_300 | PCL_RES_600_600_MONO,
    {49, 49, 15, 15},
    {49, 49, 15, 15},
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE | PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_NEW_ERG,
    letter_a4_papersizes,
    basic_papertypes,
    standard_papersources,
  },
  /* DesignJet 700 (monochrome) */
  { 10700,
    36 * 72, 150 * 12 * 72,		/* 150ft in roll mode, 64" in sheet */
    826 * 72 / 100, 583 * 72 / 100,	/* 8.3" wide min in sheet mode */
    PCL_RES_300_300 | PCL_RES_600_600,
    {30, 30, 15, 15},		/* These margins are for sheet mode FIX */
    {30, 30, 15, 15},
    PCL_COLOR_NONE,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE | PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_NEW_ERG,
    letter_a4_papersizes,
    basic_papertypes,
    standard_papersources,
  },
  /* DesignJet 750C */
  { 10750,
    36 * 72, 150 * 12 * 72,		/* 150ft in roll mode, 64" in sheet */
    826 * 72 / 100, 583 * 72 / 100,	/* 8.3" wide min in sheet mode */
    PCL_RES_300_300 | PCL_RES_600_600_MONO,
    {30, 30, 15, 15},	/* These margins are for roll mode FIX */
    {30, 30, 15, 15},
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE | PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_NEW_ERG,
    letter_a4_papersizes,
    basic_papertypes,
    standard_papersources,
  },
  /* DesignJet 2000C/2500C (36" wide) */
  { 12500,	/* Deskjet 2500 already has "2500" */
    36 * 72, 150 * 12 * 72,		/* 150ft in roll mode, 64" in sheet */
    826 * 72 / 100, 583 * 72 / 100,	/* 8.3" wide min in sheet mode */
    PCL_RES_300_300 | PCL_RES_600_600_MONO,
    {49, 49, 15, 15},		/* Check/Fix */
    {49, 49, 15, 15},
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE | PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_NEW_ERG,
    letter_a4_papersizes,
    basic_papertypes,
    standard_papersources,
  },
  /* DesignJet 3000C/3500C (54" wide) */
  { 13500,	/* Deskjet 2500 already has "2500" */
    54 * 72, 150 * 12 * 72,		/* 150ft in roll mode, 64" in sheet */
    826 * 72 / 100, 583 * 72 / 100,	/* 8.3" wide min in sheet mode */
    PCL_RES_300_300 | PCL_RES_600_600_MONO,
    {49, 49, 15, 15},		/* Check/Fix */
    {49, 49, 15, 15},
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE | PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_NEW_ERG,
    letter_a4_papersizes,
    basic_papertypes,
    standard_papersources,
  },
  /* Deskjet 340 */
  { 340,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {6, 48, 18, 18},	/* from bpd07933.pdf */
    {6, 48, 10, 11},	/* from bpd07933.pdf */
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    dj340_papersizes,
    basic_papertypes,
    dj340_papersources,
  },
  /* Deskjet 400 */
  { 400,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {7, 41, 18, 18},
    {7, 41, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    dj400_papersizes,
    basic_papertypes,
    emptylist,
  },
  /* Deskjet 500, 520. Lexmark 4076 */
  { 500,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {7, 41, 18, 18},
    {7, 41, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    dj500_papersizes,
    basic_papertypes,
    dj_papersources,
  },
  /* Deskjet 500C */
  { 501,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {7, 33, 18, 18},
    {7, 33, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    dj500_papersizes,
    basic_papertypes,
    dj_papersources,
  },
  /* Deskjet 540C */
  { 540,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {7, 33, 18, 18},
    {7, 33, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    dj540_papersizes,
    basic_papertypes,
    dj_papersources,
  },
  /* Deskjet 550C, 560C */
  { 550,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {3, 33, 18, 18},
    {5, 33, 10, 10},
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
/* The 550/560 support COM10 and DL envelope, but the control codes
   are negative, indicating landscape mode. This needs thinking about! */
    dj340_papersizes,
    basic_papertypes,
    dj_papersources,
  },
  /* Deskjet 600/600C */
  { 600,
    17 * 72 / 2, 14 * 72,
    5 * 72, 583 * 72 / 100,		/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_300 | PCL_RES_600_600_MONO,
    {0, 33, 18, 18},
    {0, 33, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    dj600_papersizes,
    basic_papertypes,
    emptylist,
  },
  /* Deskjet 6xx series */
  { 601,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_300 | PCL_RES_600_600_MONO,
    {0, 33, 18, 18},
    {0, 33, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    dj600_papersizes,
    basic_papertypes,
    emptylist,
  },
  /* Deskjet 69x series (Photo Capable) */
  { 690,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_300 | PCL_RES_600_600,
    {0, 33, 18, 18},
    {0, 33, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMYK | PCL_COLOR_CMYKcm,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    dj600_papersizes,
    basic_papertypes,
    emptylist,
  },
  /* Deskjet 850/855/870/890 (C-RET) */
  { 800,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600_MONO,
    {3, 33, 18, 18},
    {5, 33, 10, 10},
    PCL_COLOR_CMYK | PCL_COLOR_CMYK4,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    dj600_papersizes,
    basic_papertypes,
    emptylist,
  },
  /* Deskjet 810C, 812C, 840C, 842C, 845C, 895C (C-RET) */
  { 840,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_300 | PCL_RES_600_600,
    {0, 33, 18, 18},
    {0, 33, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMYK | PCL_COLOR_CMYK4b,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    dj600_papersizes,
    basic_papertypes,
    emptylist,
  },
  /* Deskjet 900 series, 1220C, PhotoSmart P1000/P1100 */
  { 900,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600 /* | PCL_RES_1200_600 | PCL_RES_2400_600 */,
    {3, 33, 18, 18},
    {5, 33, 10, 10},	/* Oliver Vecernik */
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE | PCL_PRINTER_DUPLEX,
    dj600_papersizes,
    basic_papertypes,
    emptylist,
  },
  /* Deskjet 1220C (or other large format 900) */
  { 901,
    13 * 72, 19 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600 /* | PCL_RES_1200_600 | PCL_RES_2400_600 */,
    {3, 33, 18, 18},
    {5, 33, 10, 10},
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    dj1220_papersizes,
    basic_papertypes,
    emptylist,
  },
  /* Deskjet 1100C, 1120C */
  { 1100,
    13 * 72, 19 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600_MONO,
    {3, 33, 18, 18},
    {5, 33, 10, 10},
    PCL_COLOR_CMYK | PCL_COLOR_CMYK4,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    dj1100_papersizes,
    basic_papertypes,
    dj_papersources,
  },
  /* Deskjet 1200C */
  { 1200,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    dj1200_papersizes,
    basic_papertypes,
    dj_papersources,
  },
  /* Deskjet 1600C */
  { 1600,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    dj1200_papersizes,
    basic_papertypes,
    dj_papersources,
  },
  /* Deskjet 2000 */
  { 2000,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    {0, 35, 18, 18},			/* Michel Goraczko */
    {0, 35, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    dj2000_papersizes,
    new_papertypes,
    dj_papersources,
  },
  /* Deskjet 2500 */
  { 2500,
    13 * 72, 19 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    dj2500_papersizes,
    new_papertypes,
    dj2500_papersources,
  },
  /* LaserJet II series */
  { 2,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ,
    ljsmall_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* LaserJet IIP (TIFF but no blankline) */
  { 21,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_TIFF,
    ljsmall_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* Some laser printers don't have expanded A4 margins */
  { 22,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 18, 18},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ,
    ljsmall_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* PCL-4 with large paper */
  { 23,
    13 * 72, 19 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ,
    ljbig_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* PCL-4 with large paper, no expanded A4 margins */
  { 24,
    13 * 72, 19 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 18, 18},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ,
    ljbig_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* LaserJet III series */
  { 3,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    ljsmall_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* LaserJet III series */
  { 31,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    ljsmall_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* Some laser printers don't have expanded A4 margins */
  { 32,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 18, 18},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    ljsmall_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* PCL-5 with large paper */
  { 33,
    13 * 72, 19 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    ljbig_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* PCL-5 with large paper, no expanded margins */
  { 34,
    13 * 72, 19 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 18, 18},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    ljbig_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* PCL-5 with tabloid paper, no expanded margins */
  { 35,
    118 * 72 / 10, 17 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 18, 18},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    ljtabloid_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* LaserJet 4L */
  { 4,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    ljsmall_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* LaserJet 4V */
  { 5,
    13 * 72, 19 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    ljbig_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* LaserJet 4Si */
  { 51,
    13 * 72, 19 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE |
      PCL_PRINTER_DUPLEX,
    ljbig_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* LaserJet 4 series (except as above), 5 series, 6 series */
  { 6,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE |
      PCL_PRINTER_DUPLEX,
    ljsmall_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* PCL-5c/5e/6/XL with large paper */
  { 61,
    13 * 72, 19 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE |
      PCL_PRINTER_DUPLEX,
    ljbig_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* Some laser printers don't have expanded A4 margins */
  { 62,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    {12, 12, 18, 18},
    {12, 12, 18, 18},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE |
      PCL_PRINTER_DUPLEX,
    ljsmall_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* PCL-5c/5e/6/XL with large paper, no expanded A4 margins */
  { 63,
    13 * 72, 19 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    {12, 12, 18, 18},
    {12, 12, 18, 18},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE |
      PCL_PRINTER_DUPLEX,
    ljbig_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* PCL-5c/5e/6/XL with tabloid paper, no expanded A4 margins */
  { 64,
    118 * 72 / 10, 17 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    {12, 12, 18, 18},
    {12, 12, 18, 18},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE |
      PCL_PRINTER_DUPLEX,
    ljtabloid_papersizes,
    emptylist,
    laserjet_papersources,
  },
  /* LaserJet 5Si */
  { 7,
    13 * 72, 19 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE |
      PCL_PRINTER_DUPLEX,
    ljbig_papersizes,
    emptylist,
    laserjet_papersources,
  },
};


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
    "Quality", N_("Print Quality"), "Color=Yes,Category=Basic Output Adjustment",
    N_("Print Quality"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 0, 0
  },
  {
    "Resolution", N_("Resolution"), "Color=Yes,Category=Basic Printer Setup",
    N_("Resolution of the print"),
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
  {
    "Duplex", N_("Double-Sided Printing"), "Color=No,Category=Basic Printer Setup",
    N_("Duplex/Tumble Setting"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
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

/*
 * Convert a name into it's option value
 */

static int pcl_string_to_val(const char *string,		/* I: String */
                           const pcl_t *options,	/* I: Options */
			   int num_options)		/* I: Num options */
{

  int i;
  int code = -1;

 /*
  * Look up the string in the table and convert to the code.
  */

  for (i=0; i<num_options; i++) {
    if (!strcmp(string, options[i].pcl_name)) {
       code=options[i].pcl_code;
       break;
       }
  }

  stp_deprintf(STP_DBG_PCL, "String: %s, Code: %d\n", string, code);

  return(code);
}

/*
 * Convert a value into it's option name
 */

static const char * pcl_val_to_string(int code,			/* I: Code */
                           const pcl_t *options,	/* I: Options */
			   int num_options)		/* I: Num options */
{

  int i;
  const char *string = NULL;

 /*
  * Look up the code in the table and convert to the string.
  */

  for (i=0; i<num_options; i++) {
    if (code == options[i].pcl_code) {
       string=options[i].pcl_name;
       break;
       }
  }

  stp_deprintf(STP_DBG_PCL, "Code: %d, String: %s\n", code, string);

  return(string);
}

static const char * pcl_val_to_text(int code,			/* I: Code */
                           const pcl_t *options,	/* I: Options */
			   int num_options)		/* I: Num options */
{

  int i;
  const char *string = NULL;

 /*
  * Look up the code in the table and convert to the string.
  */

  for (i=0; i<num_options; i++) {
    if (code == options[i].pcl_code) {
       string=gettext(options[i].pcl_text);
       break;
       }
  }

  stp_deprintf(STP_DBG_PCL, "Code: %d, String: %s\n", code, string);

  return(string);
}

static const double dot_sizes[] = { 0.5, 0.832, 1.0 };
static const double dot_sizes_cret[] = { 1.0, 1.0, 1.0 };

static const stp_dotsize_t variable_dotsizes[] =
{
  { 0x1, 0.5 },
  { 0x2, 0.67 },
  { 0x3, 1.0 }
};

static const stp_shade_t variable_shades[] =
{
  { 0.38, 3, variable_dotsizes },
  { 1.0, 3, variable_dotsizes }
};

/*
 * pcl_get_model_capabilities() - Return struct of model capabilities
 */

static const pcl_cap_t *				/* O: Capabilities */
pcl_get_model_capabilities(int model)	/* I: Model */
{
  int i;
  int models= sizeof(pcl_model_capabilities) / sizeof(pcl_cap_t);
  for (i=0; i<models; i++) {
    if (pcl_model_capabilities[i].model == model) {
      return &(pcl_model_capabilities[i]);
    }
  }
  stp_erprintf("pcl: model %d not found in capabilities list.\n",model);
  return &(pcl_model_capabilities[0]);
}

/*
 * Determine the current resolution based on quality and resolution settings
 */

static void
pcl_describe_resolution(const stp_vars_t *v, int *x, int *y)
{
  int i;
  int model = stp_get_model_id(v);
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  const char *quality;
  const pcl_cap_t *caps = NULL;
  if (resolution)
    {
      for (i = 0; i < NUM_RESOLUTIONS; i++)
	{
	  if (!strcmp(resolution, pcl_resolutions[i].pcl_name))
	    {
	      *x = pcl_resolutions[i].p0;
	      *y = pcl_resolutions[i].p1;
	      return;
	    }
	}
    }
  quality = stp_get_string_parameter(v, "Quality");
  caps = pcl_get_model_capabilities(model);
  if (quality && strcmp(quality, "None") == 0)
    quality = "Standard";
  if (quality)
    {
      for (i = 0; i < NUM_QUALITIES; i++)
	{
	  if ((caps->resolutions & pcl_qualities[i].pcl_code) &&
	      !strcmp(quality, pcl_qualities[i].pcl_name))
	    {
	      *x = pcl_qualities[i].p0;
	      *y = pcl_qualities[i].p1;
	      return;
	    }
	}
    }
  *x = -1;
  *y = -1;
}

/*
 * Convert Media size name into PCL media code for printer
 */

static int pcl_convert_media_size(const char *media_size,	/* I: Media size string */
				  int  model)		/* I: model number */
{

  int i;
  int media_code = 0;
  const pcl_cap_t *caps;

 /*
  * First look up the media size in the table and convert to the code.
  */

  media_code = pcl_string_to_val(media_size, pcl_media_sizes,
                                 NUM_PRINTER_PAPER_SIZES);

  stp_deprintf(STP_DBG_PCL, "Media Size: %s, Code: %d\n", media_size, media_code);

 /*
  * Now see if the printer supports the code found.
  */

  if (media_code != -1) {
    caps = pcl_get_model_capabilities(model);

    for (i=0; (i<NUM_PRINTER_PAPER_SIZES) && (caps->paper_sizes[i] != -1); i++) {
      if (media_code == (int) caps->paper_sizes[i])
        return(media_code);		/* Is supported */
    }

    stp_deprintf(STP_DBG_PCL, "Media Code %d not supported by printer model %d.\n",
      media_code, model);
    return(-1);				/* Not supported */
  }
  else
    return(-1);				/* Not supported */
}


static const stp_param_string_t ink_types[] =
{
  { "CMYK",	N_ ("Color + Black Cartridges") },
  { "Photo",	N_ ("Color + Photo Cartridges") }
};

/*
 * Duplex support - modes available
 * Note that the internal names MUST match those in cups/genppd.c else the
 * PPD files will not be generated correctly
 */

static const stp_param_string_t duplex_types[] =
{
  { "None",		N_ ("Off") },
  { "DuplexNoTumble",	N_ ("Long Edge (Standard)") },
  { "DuplexTumble",	N_ ("Short Edge (Flip)") }
};
#define NUM_DUPLEX (sizeof (duplex_types) / sizeof (stp_param_string_t))

/*
 * 'pcl_papersize_valid()' - Is the paper size valid for this printer.
 */

static int
pcl_papersize_valid(const stp_papersize_t *pt,
		    int model)
{

  const pcl_cap_t *caps = pcl_get_model_capabilities(model);

#ifdef PCL_NO_CUSTOM_PAPERSIZES
  int use_custom = 0;
#else
  int use_custom = ((caps->stp_printer_type & PCL_PRINTER_CUSTOM_SIZE)
                     == PCL_PRINTER_CUSTOM_SIZE);
#endif

  unsigned int pwidth = pt->width;
  unsigned int pheight = pt->height;

/*
 * This function decides whether a paper size is allowed for the
 * current printer. The DeskJet feed mechanisms set a minimum and
 * maximum size for the paper, BUT some of the directly supported
 * media sizes are less than this minimum (eg card and envelopes)
 * So, we allow supported sizes though, but clamp custom sizes
 * to the min and max sizes.
 */

/*
 * Is it a valid name?
 */

  if (strlen(pt->name) <= 0)
    return(0);

/*
 * Is it a recognised supported name?
 */

  if (pcl_convert_media_size(pt->name, model) != -1)
    return(1);

/*
 * If we are not allowed to use custom paper sizes, we are done
 */

  if (use_custom == 0)
    return(0);

/*
 * We are allowed custom paper sizes. Check that the size is within
 * limits.
 */

  if (pwidth <= caps->custom_max_width &&
     pheight <= caps->custom_max_height &&
     (pheight >=  caps->custom_min_height || pheight == 0) &&
     (pwidth >= caps->custom_min_width || pwidth == 0))
    return(1);

  return(0);
}

/*
 * 'pcl_parameters()' - Return the parameter values for the given parameter.
 */

static stp_parameter_list_t
pcl_list_parameters(const stp_vars_t *v)
{
  stp_parameter_list_t *ret = stp_parameter_list_create();
  int i;
  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(the_parameters[i]));
  for (i = 0; i < float_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(float_parameters[i].param));
  return ret;
}

static void
pcl_parameters(const stp_vars_t *v, const char *name,
	       stp_parameter_t *description)
{
  int		model = stp_get_model_id(v);
  int		i;
  const pcl_cap_t *caps;
  description->p_type = STP_PARAMETER_TYPE_INVALID;

  if (name == NULL)
    return;

  stp_deprintf(STP_DBG_PCL, "pcl_parameters(): Name = %s\n", name);

  caps = pcl_get_model_capabilities(model);

  stp_deprintf(STP_DBG_PCL, "Printer model = %d\n", model);
  stp_deprintf(STP_DBG_PCL, "PageWidth = %d, PageHeight = %d\n", caps->custom_max_width, caps->custom_max_height);
  stp_deprintf(STP_DBG_PCL, "MinPageWidth = %d, MinPageHeight = %d\n", caps->custom_min_width, caps->custom_min_height);
  stp_deprintf(STP_DBG_PCL, "Normal Margins: top = %d, bottom = %d, left = %d, right = %d\n",
    caps->normal_margins.top_margin, caps->normal_margins.bottom_margin,
    caps->normal_margins.left_margin, caps->normal_margins.right_margin);
  stp_deprintf(STP_DBG_PCL, "A4 Margins: top = %d, bottom = %d, left = %d, right = %d\n",
    caps->a4_margins.top_margin, caps->a4_margins.bottom_margin,
    caps->a4_margins.left_margin, caps->a4_margins.right_margin);
  stp_deprintf(STP_DBG_PCL, "Resolutions: %d\n", caps->resolutions);
  stp_deprintf(STP_DBG_PCL, "ColorType = %d, PrinterType = %d\n", caps->color_type, caps->stp_printer_type);

  for (i = 0; i < the_parameter_count; i++)
    if (strcmp(name, the_parameters[i].name) == 0)
      {
	stp_fill_parameter_settings(description, &(the_parameters[i]));
	break;
      }
  description->deflt.str = NULL;

  for (i = 0; i < float_parameter_count; i++)
    if (strcmp(name, float_parameters[i].param.name) == 0)
      {
	stp_fill_parameter_settings(description,
				    &(float_parameters[i].param));
	description->deflt.dbl = float_parameters[i].defval;
	description->bounds.dbl.upper = float_parameters[i].max;
	description->bounds.dbl.lower = float_parameters[i].min;
	break;
      }

  if (strcmp(name, "PageSize") == 0)
    {
      int papersizes = stp_known_papersizes();
      description->bounds.str = stp_string_list_create();
      for (i = 0; i < papersizes; i++)
	{
	  const stp_papersize_t *pt = stp_get_papersize_by_index(i);
	  if (strlen(pt->name) > 0 && pcl_papersize_valid(pt, model))
	    stp_string_list_add_string(description->bounds.str,
				       pt->name, gettext(pt->text));
	}
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "MediaType") == 0)
  {
    description->bounds.str = stp_string_list_create();
    if (caps->paper_types[0] != -1)
      {
	for (i=0; (i < NUM_PRINTER_PAPER_TYPES) && (caps->paper_types[i] != -1); i++)
	  stp_string_list_add_string(description->bounds.str,
				    pcl_val_to_string(caps->paper_types[i],
						      pcl_media_types,
						      NUM_PRINTER_PAPER_TYPES),
				    pcl_val_to_text(caps->paper_types[i],
						    pcl_media_types,
						    NUM_PRINTER_PAPER_TYPES));
	description->deflt.str =
	  stp_string_list_param(description->bounds.str, 0)->name;
      }
    else
      description->is_active = 0;
  }
  else if (strcmp(name, "InputSlot") == 0)
  {
    description->bounds.str = stp_string_list_create();
    if (caps->paper_sources[0] != -1)
      {
	for (i=0; (i < NUM_PRINTER_PAPER_SOURCES) && (caps->paper_sources[i] != -1); i++)
	  stp_string_list_add_string(description->bounds.str,
				    pcl_val_to_string(caps->paper_sources[i],
						      pcl_media_sources,
						      NUM_PRINTER_PAPER_SOURCES),
				    pcl_val_to_text(caps->paper_sources[i],
						    pcl_media_sources,
						    NUM_PRINTER_PAPER_SOURCES));
	description->deflt.str =
	  stp_string_list_param(description->bounds.str, 0)->name;
      }
    else
      description->is_active = 0;
  }
  else if (strcmp(name, "Resolution") == 0)
  {
    description->bounds.str = stp_string_list_create();
    stp_string_list_add_string(description->bounds.str, "None", _("Default"));
    description->deflt.str = "None";
    for (i = 0; i < NUM_RESOLUTIONS; i++)
      if (caps->resolutions & pcl_resolutions[i].pcl_code)
	{
	  stp_string_list_add_string
	    (description->bounds.str,
	     pcl_val_to_string(pcl_resolutions[i].pcl_code,
			       pcl_resolutions, NUM_RESOLUTIONS),
	     pcl_val_to_text(pcl_resolutions[i].pcl_code,
			     pcl_resolutions, NUM_RESOLUTIONS));
	}
  }
  else if (strcmp(name, "Quality") == 0)
  {
    int has_standard_quality = 0;
    description->bounds.str = stp_string_list_create();
    stp_string_list_add_string(description->bounds.str, "None",
			       _("Manual Control"));
    for (i = 0; i < NUM_QUALITIES; i++)
      if (caps->resolutions & pcl_qualities[i].pcl_code)
	{
	  const char *qual =
	    pcl_val_to_string(pcl_qualities[i].pcl_code,
			      pcl_qualities, NUM_QUALITIES);
	  if (! stp_string_list_is_present(description->bounds.str, qual))
	    stp_string_list_add_string
	      (description->bounds.str, qual,
	       pcl_val_to_text(pcl_qualities[i].pcl_code,
			       pcl_qualities, NUM_QUALITIES));
	  if (strcmp(qual, "Standard") == 0)
	    has_standard_quality = 1;
	}
    if (has_standard_quality)
      description->deflt.str = "Standard";
    else
      description->deflt.str = "None";
  }
  else if (strcmp(name, "InkType") == 0)
  {
    description->bounds.str = stp_string_list_create();
    if (caps->color_type & PCL_COLOR_CMYKcm)
    {
      description->deflt.str = ink_types[0].name;
      stp_string_list_add_string(description->bounds.str,
			       ink_types[0].name,gettext(ink_types[0].text));
      stp_string_list_add_string(description->bounds.str,
			       ink_types[1].name,gettext(ink_types[1].text));
    }
    else
      description->is_active = 0;
  }
  else if (strcmp(name, "Duplex") == 0)
  {
    int offer_duplex=0;

    description->bounds.str = stp_string_list_create();

/*
 * Don't offer the Duplex/Tumble options if the JobMode parameter is
 * set to "Page" Mode.
 * "Page" mode is set by the Gimp Plugin, which only outputs one page at a
 * time, so Duplex/Tumble is meaningless.
 */

    if (stp_get_string_parameter(v, "JobMode"))
        offer_duplex = strcmp(stp_get_string_parameter(v, "JobMode"), "Page");
    else
     offer_duplex=1;

    if (offer_duplex)
    {
      if (caps->stp_printer_type & PCL_PRINTER_DUPLEX)
      {
        description->deflt.str = duplex_types[0].name;
        for (i=0; i < NUM_DUPLEX; i++)
        {
          stp_string_list_add_string(description->bounds.str,
			       duplex_types[i].name,gettext(duplex_types[i].text));
        }
      }
      else
        description->is_active = 0;	/* Not supported by printer */
    }
    else
      description->is_active = 0;	/* Not in "Job" mode */
  }
  else if (strcmp(name, "CyanDensity") == 0 ||
	   strcmp(name, "MagentaDensity") == 0 ||
	   strcmp(name, "YellowDensity") == 0 ||
	   strcmp(name, "BlackDensity") == 0)
    {
      if (caps->color_type != PCL_COLOR_NONE &&
	  stp_check_string_parameter(v, "PrintingMode", STP_PARAMETER_DEFAULTED) &&
	  strcmp(stp_get_string_parameter(v, "PrintingMode"), "Color") == 0)
	description->is_active = 1;
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "LightCyanTrans") == 0 ||
	   strcmp(name, "LightMagentaTrans") == 0)
    {
      if (caps->color_type & PCL_COLOR_CMYKcm &&
	  stp_check_string_parameter(v, "PrintingMode", STP_PARAMETER_DEFAULTED) &&
	  strcmp(stp_get_string_parameter(v, "PrintingMode"), "Color") == 0)
	description->is_active = 1;
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "InkChannels") == 0)
    {
      if (caps->color_type & PCL_COLOR_CMYKcm)
	description->deflt.integer = 6;
      else if (caps->color_type == PCL_COLOR_NONE)
	description->deflt.integer = 1;
      else
	description->deflt.integer = 4;
      description->bounds.integer.lower = -1;
      description->bounds.integer.upper = -1;
    }
  else if (strcmp(name, "PrintingMode") == 0)
    {
      description->bounds.str = stp_string_list_create();
      if (caps->color_type != PCL_COLOR_NONE)
	stp_string_list_add_string
	  (description->bounds.str, "Color", _("Color"));
      stp_string_list_add_string
	(description->bounds.str, "BW", _("Black and White"));
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
}


/*
 * 'pcl_imageable_area()' - Return the imageable area of the page.
 */
static void
internal_imageable_area(const stp_vars_t *v,     /* I */
			int  use_paper_margins,
			int  *left,	/* O - Left position in points */
			int  *right,	/* O - Right position in points */
			int  *bottom,	/* O - Bottom position in points */
			int  *top)	/* O - Top position in points */
{
  int	width, height;			/* Size of page */
  const pcl_cap_t *caps;		/* Printer caps */
  int	pcl_media_size;			/* Converted media size */
  const char *media_size = stp_get_string_parameter(v, "PageSize");
  const stp_papersize_t *pp = NULL;
  int left_margin = 0;
  int right_margin = 0;
  int bottom_margin = 0;
  int top_margin = 0;

  caps = pcl_get_model_capabilities(stp_get_model_id(v));

  stp_default_media_size(v, &width, &height);

/* If we are using A4 paper, then the margins are different than any
 * other paper size. This is because HP wanted to have the same printable
 * width for A4 as for letter. Go figure.
 */

  if (!media_size)
    media_size = "";
  if (strlen(media_size) == 0 &&
      ((pp = stp_get_papersize_by_size(stp_get_page_height(v),
				       stp_get_page_width(v))) != NULL))
    media_size = pp->name;

  stp_deprintf(STP_DBG_PCL, "pcl_imageable_area(): media_size: '%s'\n",
	       media_size);

  pcl_media_size = pcl_convert_media_size(media_size, stp_get_model_id(v));
  if (media_size)
    pp = stp_get_papersize_by_name(media_size);
  if (pp && use_paper_margins)
    {
      left_margin = pp->left;
      right_margin = pp->right;
      bottom_margin = pp->bottom;
      top_margin = pp->top;
    }

  if (pcl_media_size == PCL_PAPERSIZE_A4)
  {
    left_margin = MAX(left_margin, caps->a4_margins.left_margin);
    right_margin = MAX(right_margin, caps->a4_margins.right_margin);
    top_margin = MAX(top_margin, caps->a4_margins.top_margin);
    bottom_margin = MAX(bottom_margin, caps->a4_margins.bottom_margin);
  }
  else
  {
    left_margin = MAX(left_margin, caps->normal_margins.left_margin);
    right_margin = MAX(right_margin, caps->normal_margins.right_margin);
    top_margin = MAX(top_margin, caps->normal_margins.top_margin);
    bottom_margin = MAX(bottom_margin, caps->normal_margins.bottom_margin);
  }
  *left =	left_margin;
  *right =	width - right_margin;
  *top =	top_margin;
  *bottom =	height - bottom_margin;
}

static void
pcl_imageable_area(const stp_vars_t *v,     /* I */
                   int  *left,		/* O - Left position in points */
                   int  *right,		/* O - Right position in points */
                   int  *bottom,	/* O - Bottom position in points */
                   int  *top)		/* O - Top position in points */
{
  internal_imageable_area(v, 1, left, right, bottom, top);
}

static void
pcl_limit(const stp_vars_t *v,  		/* I */
	  int *width,
	  int *height,
	  int *min_width,
	  int *min_height)
{
  const pcl_cap_t *caps= pcl_get_model_capabilities(stp_get_model_id(v));
  *width =	caps->custom_max_width;
  *height =	caps->custom_max_height;
  *min_width =	caps->custom_min_width;
  *min_height =	caps->custom_min_height;
}

static const char *
pcl_describe_output(const stp_vars_t *v)
{
  int printing_color = 0;
  int model = stp_get_model_id(v);
  const pcl_cap_t *caps = pcl_get_model_capabilities(model);
  const char *print_mode = stp_get_string_parameter(v, "PrintingMode");
  int xdpi, ydpi;

  pcl_describe_resolution(v, &xdpi, &ydpi);
  if (!print_mode || strcmp(print_mode, "Color") == 0)
    printing_color = 1;
  if (((caps->resolutions & PCL_RES_600_600_MONO) == PCL_RES_600_600_MONO) &&
      printing_color && xdpi == 600 && ydpi == 600)
    printing_color = 0;
  if (printing_color)
    {
      if ((caps->color_type & PCL_COLOR_CMY) == PCL_COLOR_CMY)
	return "CMY";
      else
	return "CMYK";
    }
  else
    return "Grayscale";
}

/*
 * 'pcl_print()' - Print an image to an HP printer.
 */

static void
pcl_printfunc(stp_vars_t *v)
{
  pcl_privdata_t *pd = (pcl_privdata_t *) stp_get_component_data(v, "Driver");
  int do_blank = pd->do_blank;
  unsigned char *black = stp_dither_get_channel(v, STP_ECOLOR_K, 0);
  unsigned char *cyan = stp_dither_get_channel(v, STP_ECOLOR_C, 0);
  unsigned char *lcyan = stp_dither_get_channel(v, STP_ECOLOR_C, 1);
  unsigned char *magenta = stp_dither_get_channel(v, STP_ECOLOR_M, 0);
  unsigned char *lmagenta = stp_dither_get_channel(v, STP_ECOLOR_M, 1);
  unsigned char *yellow = stp_dither_get_channel(v, STP_ECOLOR_Y, 0);
  int len_c = stp_dither_get_last_position(v, STP_ECOLOR_C, 0);
  int len_lc = stp_dither_get_last_position(v, STP_ECOLOR_C, 1);
  int len_m = stp_dither_get_last_position(v, STP_ECOLOR_M, 0);
  int len_lm = stp_dither_get_last_position(v, STP_ECOLOR_M, 1);
  int len_y = stp_dither_get_last_position(v, STP_ECOLOR_Y, 0);
  int len_k = stp_dither_get_last_position(v, STP_ECOLOR_K, 0);
  int is_blank = (do_blank && (len_c == -1) && (len_lc == -1) &&
		  (len_m == -1) && (len_lm == -1) && (len_y == -1) &&
		  (len_k == -1));
  int height = pd->height;
  const char    *print_mode = stp_get_string_parameter(v, "PrintingMode");

  if (is_blank && (pd->blank_lines != 0))	/* repeated blank line */
    {
      pd->blank_lines++;
    }
  else				/* Not blank, or is first one */
    {
      if (! is_blank)
	{
	  if (pd->blank_lines > 1)		/* Output accumulated lines */
	    {
	      pd->blank_lines--;		/* correct for one already output */
	      stp_deprintf(STP_DBG_PCL, "Blank Lines = %d\n", pd->blank_lines);
	      stp_zprintf(v, "\033*b%dY", pd->blank_lines);
	      pd->blank_lines=0;
	    }
	  else;
	}
      else
	{
	  pd->blank_lines++;
	}

      if (pd->do_cret)
	{
	  /*
	   * 4-level (CRet) dithers...
	   */
	  if (strcmp(print_mode, "BW") == 0)
	    {
	      (*(pd->writefunc))(v, black + height / 2, height / 2, 0);
	      (*(pd->writefunc))(v, black, height / 2, 1);
	    }
	  else
	    {
	      if(pd->do_cretb)
		{
		  /*	    (*(pd->writefunc))(v, black + height / 2, 0, 0); */
		  (*(pd->writefunc))(v, black, height/2, 0);
		}
	      else
		{
		  (*(pd->writefunc))(v, black + height / 2, height / 2, 0);
		  (*(pd->writefunc))(v, black, height / 2, 0);
		}
	      (*(pd->writefunc))(v, cyan + height / 2, height / 2, 0);
	      (*(pd->writefunc))(v, cyan, height / 2, 0);
	      (*(pd->writefunc))(v, magenta + height / 2, height / 2, 0);
	      (*(pd->writefunc))(v, magenta, height / 2, 0);
	      (*(pd->writefunc))(v, yellow + height / 2, height / 2, 0);
	      if (pd->do_6color)
		{
		  (*(pd->writefunc))(v, yellow, height / 2, 0);
		  (*(pd->writefunc))(v, lcyan + height / 2, height / 2, 0);
		  (*(pd->writefunc))(v, lcyan, height / 2, 0);
		  (*(pd->writefunc))(v, lmagenta + height / 2, height / 2, 0);
		  (*(pd->writefunc))(v, lmagenta, height / 2, 1);		/* Last plane set on light magenta */
		}
	      else
		(*(pd->writefunc))(v, yellow, height / 2, 1);		/* Last plane set on yellow */
	    }
	}
      else
	{
	  /*
	   * Standard 2-level dithers...
	   */

	  if (strcmp(print_mode, "BW") == 0)
	    {
	      (*(pd->writefunc))(v, black, height, 1);
	    }
	  else
	    {
	      if (black != NULL)
		(*(pd->writefunc))(v, black, height, 0);
	      (*(pd->writefunc))(v, cyan, height, 0);
	      (*(pd->writefunc))(v, magenta, height, 0);
	      if (pd->do_6color)
		{
		  (*(pd->writefunc))(v, yellow, height, 0);
		  (*(pd->writefunc))(v, lcyan, height, 0);
		  (*(pd->writefunc))(v, lmagenta, height, 1);		/* Last plane set on light magenta */
		}
	      else
		(*(pd->writefunc))(v, yellow, height, 1);		/* Last plane set on yellow */
	    }
	}
    }
}

static double
get_double_param(stp_vars_t *v, const char *param)
{
  if (param && stp_check_float_parameter(v, param, STP_PARAMETER_ACTIVE))
    return stp_get_float_parameter(v, param);
  else
    return 1.0;
}

static int
pcl_do_print(stp_vars_t *v, stp_image_t *image)
{
  pcl_privdata_t privdata;
  int		status = 1;
  int		model = stp_get_model_id(v);
  const char	*media_size = stp_get_string_parameter(v, "PageSize");
  const char	*media_type = stp_get_string_parameter(v, "MediaType");
  const char	*media_source = stp_get_string_parameter(v, "InputSlot");
  const char	*ink_type = stp_get_string_parameter(v, "InkType");
  const char	*print_mode = stp_get_string_parameter(v, "PrintingMode");
  const char	*duplex_mode = stp_get_string_parameter(v, "Duplex");
  int		page_number = stp_get_int_parameter(v, "PageNumber");
  int		printing_color = 0;
  int		top = stp_get_top(v);
  int		left = stp_get_left(v);
  int		y;		/* Looping vars */
  int		xdpi, ydpi;	/* Resolution */
  unsigned char *black,		/* Black bitmap data */
		*cyan,		/* Cyan bitmap data */
		*magenta,	/* Magenta bitmap data */
		*yellow,	/* Yellow bitmap data */
		*lcyan,		/* Light Cyan bitmap data */
		*lmagenta;	/* Light Magenta bitmap data */
  int		page_width,	/* Width of page */
		page_height,	/* Height of page */
		page_left,
		page_top,
		page_right,
		page_bottom,
		out_width,	/* Width of image on page */
		out_height,	/* Height of image on page */
		out_channels,	/* Output bytes per pixel */
		errdiv,		/* Error dividend */
		errmod,		/* Error modulus */
		errval,		/* Current error value */
		errline,	/* Current raster line */
		errlast;	/* Last raster line loaded */
  unsigned	zero_mask;
  int           image_height,
                image_width;
  const pcl_cap_t *caps;		/* Printer capabilities */
  int		planes = 3;	/* # of output planes */
  int		pcl_media_size; /* PCL media size code */
  const double *dot_sizes_use;
  const stp_papersize_t *pp;
  int		the_top_margin,	/* Corrected top margin */
		the_left_margin;	/* Corrected left margin */
  int		manual_feed_left_adjust = 0;
  int		extra_left_margin = 0;
  stp_curve_t   *lum_adjustment;
  stp_curve_t   *hue_adjustment;
  double density;

  if (!stp_verify(v))
    {
      stp_eprintf(v, "Print options not verified; cannot print.\n");
      return 0;
    }
  if (strcmp(print_mode, "Color") == 0)
    printing_color = 1;

  caps = pcl_get_model_capabilities(model);

 /*
  * Setup a read-only pixel region for the entire image...
  */

  stp_image_init(image);
  image_height = stp_image_height(image);
  image_width = stp_image_width(image);

 /*
  * Figure out the output resolution...
  */

  pcl_describe_resolution(v, &xdpi, &ydpi);

  stp_deprintf(STP_DBG_PCL,"pcl: resolution=%dx%d\n",xdpi,ydpi);
  if (xdpi <= 0 || ydpi <= 0)
    {
      stp_eprintf(v, "No resolution found; cannot print.\n");
      return 0;
    }

 /*
  * Choose the correct color conversion function...
  */
  if (((caps->resolutions & PCL_RES_600_600_MONO) == PCL_RES_600_600_MONO) &&
      printing_color && xdpi == 600 && ydpi == 600)
    {
      stp_eprintf(v, "600x600 resolution only available in MONO\n");
      stp_set_string_parameter(v, "PrintingMode", "BW");
      printing_color = 0;
    }

  privdata.do_cret = (xdpi >= 300 &&
	     ((caps->color_type & PCL_COLOR_CMYK4) == PCL_COLOR_CMYK4));
  privdata.do_cretb = (xdpi >= 600 && ydpi >= 600 &&
	      ((caps->color_type & PCL_COLOR_CMYK4b) == PCL_COLOR_CMYK4b) &&
	      printing_color);
  if (privdata.do_cretb){
    privdata.do_cret = 1;
    dot_sizes_use=dot_sizes_cret;
  }else{
    dot_sizes_use=dot_sizes;
  }

  stp_deprintf(STP_DBG_PCL, "privdata.do_cret = %d\n", privdata.do_cret);
  stp_deprintf(STP_DBG_PCL, "privdata.do_cretb = %d\n", privdata.do_cretb);

  if (ink_type && printing_color)
    privdata.do_6color = (strcmp(ink_type, "Photo") == 0);
  else
    privdata.do_6color = 0;

  stp_deprintf(STP_DBG_PCL, "privdata.do_6color = %d\n", privdata.do_6color);

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

  image_height = stp_image_height(image);
  image_width = stp_image_width(image);

 /*
  * Set media size here because it is needed by the margin calculation code.
  */

  if (!media_size)
    media_size = "";
  if (strlen(media_size) == 0 &&
      ((pp = stp_get_papersize_by_size(stp_get_page_height(v),
				       stp_get_page_width(v))) != NULL))
    media_size = pp->name;

  pcl_media_size = pcl_convert_media_size(media_size, model);

  stp_deprintf(STP_DBG_PCL,"pcl_media_size = %d, media_size = %s\n", pcl_media_size, media_size);

 /*
  * If the media size requested is unknown, try it as a custom size.
  *
  * Warning: The margins may need to be fixed for this!
  */

  if (pcl_media_size == -1) {
    stp_deprintf(STP_DBG_PCL, "Paper size %s is not directly supported by printer.\n",
      media_size);
    stp_deprintf(STP_DBG_PCL, "Trying as custom pagesize (watch the margins!)\n");
    pcl_media_size = PCL_PAPERSIZE_CUSTOM;			/* Custom */
  }

  stp_deprintf(STP_DBG_PCL, "Duplex: %s, Page_Number: %d\n", duplex_mode, page_number);
  privdata.duplex=0;
  privdata.tumble=0;

 /*
  * Duplex
  */

  if (duplex_mode)
    {
      if (caps->stp_printer_type & PCL_PRINTER_DUPLEX)
        {
          if ((strcmp(duplex_mode, "DuplexTumble") == 0) || (strcmp(duplex_mode, "DuplexNoTumble") == 0))
            privdata.duplex=1;
          if ((strcmp(duplex_mode, "DuplexTumble") == 0))
            privdata.tumble=1;
        }
    }

 /*
  * Send PCL initialization commands...
  */

  if ((privdata.duplex == 0) || ((page_number & 1) == 0))
    {
      int pcl_media_type,	/* PCL media type code */
          pcl_media_source;	/* PCL media source code */

      stp_deprintf(STP_DBG_PCL, "Normal init\n");

      if (privdata.do_cretb)
        stp_puts("\033*rbC", v);	/* End raster graphics */
      stp_puts("\033E", v); 				/* PCL reset */
      if (privdata.do_cretb)
        stp_zprintf(v, "\033%%-12345X@PJL ENTER LANGUAGE=PCL3GUI\n");

      stp_puts("\033&l6D\033&k12H",v);		/* 6 lines per inch, 10 chars per inch */
      stp_puts("\033&l0O",v);			/* Portrait */

      stp_zprintf(v, "\033&l%dA", pcl_media_size);	/* Set media size we calculated above */
      stp_zprintf(v, "\033&l%dP", stp_get_page_height(v) / 12);
						/* Length of "forms" in "lines" */
      stp_puts("\033&l0L", v);			/* Turn off perforation skip */
      stp_puts("\033&l0E", v);			/* Reset top margin to 0 */

 /*
  * Convert media source string to the code, if specified.
  */

      if (media_source && strlen(media_source) != 0) {
        pcl_media_source = pcl_string_to_val(media_source, pcl_media_sources,
                             sizeof(pcl_media_sources) / sizeof(pcl_t));

        stp_deprintf(STP_DBG_PCL,"pcl_media_source = %d, media_source = %s\n", pcl_media_source,
               media_source);

        if (pcl_media_source == -1)
          stp_deprintf(STP_DBG_PCL, "Unknown media source %s, ignored.\n", media_source);
        else if (pcl_media_source != PCL_PAPERSOURCE_STANDARD) {

/* Correct the value by taking the modulus */

	  if ((pcl_media_source & PAPERSOURCE_ADJ_GUIDE) ==
	      PAPERSOURCE_ADJ_GUIDE)
	    {
	      manual_feed_left_adjust = 1;
	      stp_deprintf(STP_DBG_PCL, "Adjusting left margin for manual feed.\n");
	    }
          pcl_media_source = pcl_media_source % PAPERSOURCE_MOD;
          stp_zprintf(v, "\033&l%dH", pcl_media_source);
        }
      }

 /*
  * Convert media type string to the code, if specified.
  */

      if (media_type && strlen(media_type) != 0) {
        pcl_media_type = pcl_string_to_val(media_type, pcl_media_types,
                           sizeof(pcl_media_types) / sizeof(pcl_t));

        stp_deprintf(STP_DBG_PCL,"pcl_media_type = %d, media_type = %s\n", pcl_media_type,
               media_type);

        if (pcl_media_type == -1) {
          stp_deprintf(STP_DBG_PCL, "Unknown media type %s, set to PLAIN.\n", media_type);
          pcl_media_type = PCL_PAPERTYPE_PLAIN;
        }

/*
 * The HP812C doesn't like glossy paper being selected when using 600x600
 * C-RET (PhotoRET II). So we use Premium paper instead.
 *
 */

        if (privdata.do_cretb && pcl_media_type == PCL_PAPERTYPE_GLOSSY) {
          stp_deprintf(STP_DBG_PCL, "Media type GLOSSY, set to PREMIUM for PhotoRET II.\n");
          pcl_media_type = PCL_PAPERTYPE_PREMIUM;
        }
      }
      else
        pcl_media_type = PCL_PAPERTYPE_PLAIN;

 /*
  * Set DJ print quality to "best" if resolution >= 300
  */

      if ((xdpi >= 300) && ((caps->stp_printer_type & PCL_PRINTER_DJ) == PCL_PRINTER_DJ))
      {
        if ((caps->stp_printer_type & PCL_PRINTER_MEDIATYPE) == PCL_PRINTER_MEDIATYPE)
        {
          stp_puts("\033*o1M", v);			/* Quality = presentation */
          stp_zprintf(v, "\033&l%dM", pcl_media_type);
        }
        else
        {
          stp_puts("\033*r2Q", v);			/* Quality (high) */
          stp_puts("\033*o2Q", v);			/* Shingling (4 passes) */

 /* Depletion depends on media type and cart type. */

          if ((pcl_media_type == PCL_PAPERTYPE_PLAIN)
           || (pcl_media_type == PCL_PAPERTYPE_BOND)) {
            if ((caps->color_type & PCL_COLOR_CMY) == PCL_COLOR_CMY)
              stp_puts("\033*o2D", v);			/* Depletion 25% */
            else
              stp_puts("\033*o5D", v);			/* Depletion 50% with gamma correction */
            }

          else if ((pcl_media_type == PCL_PAPERTYPE_PREMIUM)
                 || (pcl_media_type == PCL_PAPERTYPE_GLOSSY)
                 || (pcl_media_type == PCL_PAPERTYPE_TRANS))
            stp_puts("\033*o1D", v);			/* Depletion none */
        }
      }

/*
 * Duplex
 */

      if (privdata.duplex)
          stp_zprintf(v,"\033&l%dS", privdata.duplex + privdata.tumble);
      }
    else
      {
        stp_deprintf(STP_DBG_PCL, "Back face init\n");
        stp_puts("\033&a2G", v);
      }

/*
 * See if we need to use the CRD (Configure Raster Data) command, because we're
 * doing something interesting on a DeskJet.
 * (I hate long complicated if statements, the compiler will sort it out!).
 */

  privdata.use_crd = 0;
  if ((caps->stp_printer_type & PCL_PRINTER_DJ) == PCL_PRINTER_DJ)
    {
    if (xdpi != ydpi)				/* Different X and Y Resolutions */
      privdata.use_crd = 1;
    if (privdata.do_cret)			/* Resolution Enhancement */
      privdata.use_crd = 1;
    if (privdata.do_6color)			/* Photo Ink printing */
      privdata.use_crd = 1;
    if (privdata.duplex)			/* Duplexing */
      privdata.use_crd = 1;
    }

  if (privdata.use_crd)
						/* Set resolution using CRD */
  {

/*
 * If duplexing on a CRD printer, we need to use the (re)load media command.
 */

    if (privdata.duplex)
      stp_puts("\033&l-2H",v);			/* Load media */

   /*
    * Send configure image data command with horizontal and
    * vertical resolutions as well as a color count...
    */

    if (printing_color)
      if ((caps->color_type & PCL_COLOR_CMY) == PCL_COLOR_CMY)
        planes = 3;
      else
        if (privdata.do_6color)
          planes = 6;
        else
          planes = 4;
    else
      planes = 1;

    stp_zprintf(v, "\033*g%dW", 2 + (planes * 6));
    stp_putc(2, v);				/* Format 2 (Complex Direct Planar) */
    stp_putc(planes, v);				/* # output planes */

    if (planes != 3) {		/* Black resolution */
      stp_send_command(v, "", "HHH", xdpi, ydpi, (privdata.do_cretb || !privdata.do_cret) ? 2 : 4);
    }

    if (planes != 1) {		/* Cyan, magenta, yellow resolutions */
      stp_send_command(v, "", "HHH", xdpi, ydpi, privdata.do_cret ? 4 : 2);
      stp_send_command(v, "", "HHH", xdpi, ydpi, privdata.do_cret ? 4 : 2);
      stp_send_command(v, "", "HHH", xdpi, ydpi, privdata.do_cret ? 4 : 2);
    }
    if (planes == 6)		/* LC, LM resolutions */
    {
      stp_send_command(v, "", "HHH", xdpi, ydpi, privdata.do_cret ? 4 : 2);
      stp_send_command(v, "", "HHH", xdpi, ydpi, privdata.do_cret ? 4 : 2);
    }
  }
  else
  {
    stp_zprintf(v, "\033*t%dR", xdpi);		/* Simple resolution */
    if (printing_color)
    {
      if ((caps->color_type & PCL_COLOR_CMY) == PCL_COLOR_CMY)
        stp_puts("\033*r-3U", v);		/* Simple CMY color */
      else
        stp_puts("\033*r-4U", v);		/* Simple KCMY color */
    }
  }

  if ((caps->stp_printer_type & PCL_PRINTER_TIFF) == PCL_PRINTER_TIFF &&
      !(stp_get_debug_level() & STP_DBG_NO_COMPRESSION))
  {
    stp_puts("\033*b2M", v);			/* Mode 2 (TIFF) */
  }
  else
  {
    stp_puts("\033*b0M", v);			/* Mode 0 (no compression) */
  }

 /*
  * Convert image size to printer resolution and setup the page for printing...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;

  if (pcl_media_size == PCL_PAPERSIZE_A4)
  {
    the_left_margin = caps->a4_margins.left_margin;
    the_top_margin = caps->a4_margins.top_margin;
  }
  else
  {
    the_left_margin = caps->normal_margins.left_margin;
    the_top_margin = caps->normal_margins.top_margin;
  }

  stp_deprintf(STP_DBG_PCL, "left %d margin %d top %d margin %d width %d height %d\n",
	  left, the_left_margin, top, the_top_margin, out_width, out_height);

  if (manual_feed_left_adjust)
    {
      unsigned wdelta = caps->custom_max_width - stp_get_page_width(v);
      if (wdelta > 0)
	{
	  /*
	   * Why 3?  I would expect it would be 2 here, but it appears
	   * that at least one printer (LJ 1022) actually partially
	   * adjusts the margin itself.  Adjusting the left margin by 1/3
	   * of the difference between the maximum width and the actual
	   * width experimentally yields correct results -- rlk 20081014
	   */
	  stp_deprintf(STP_DBG_PCL,
		       "  Adjusting manual feed left margin by %d\n", wdelta / 3);
	  extra_left_margin += wdelta / 3;
	}
    }

  if (!privdata.do_cretb) {
    stp_zprintf(v, "\033&a%dH", 10 * (left + extra_left_margin));		/* Set left raster position */
    stp_zprintf(v, "\033&a%dV", 10 * (top + the_top_margin));
				/* Set top raster position */
  }
  stp_zprintf(v, "\033*r%dS", out_width);		/* Set raster width */
  stp_zprintf(v, "\033*r%dT", out_height);	/* Set raster height */

  if (privdata.do_cretb)
    {
      /* Move to top left of printed area */
      stp_zprintf(v, "\033*p%dY", (top + the_top_margin)*4); /* Measured in dots. */
      stp_zprintf(v, "\033*p%dX", (left + extra_left_margin)*4);
    }
  stp_puts("\033*r1A", v); 			/* Start GFX */

 /*
  * Allocate memory for the raster data...
  */

  privdata.height = (out_width + 7) / 8;
  if (privdata.do_cret)
    privdata.height *= 2;

  if (!printing_color)
  {
    black   = stp_malloc(privdata.height);
    cyan    = NULL;
    magenta = NULL;
    yellow  = NULL;
    lcyan    = NULL;
    lmagenta = NULL;
  }
  else
  {
    cyan    = stp_malloc(privdata.height);
    magenta = stp_malloc(privdata.height);
    yellow  = stp_malloc(privdata.height);

    if ((caps->color_type & PCL_COLOR_CMY) == PCL_COLOR_CMY)
      black = NULL;
    else
      black = stp_malloc(privdata.height);
    if (privdata.do_6color)
    {
      lcyan    = stp_malloc(privdata.height);
      lmagenta = stp_malloc(privdata.height);
    }
    else
    {
      lcyan    = NULL;
      lmagenta = NULL;
    }
  }

  if (black)
    {
      if (cyan)
	stp_set_string_parameter(v, "STPIOutputType", "KCMY");
      else
	stp_set_string_parameter(v, "STPIOutputType", "Grayscale");
    }
  else
    stp_set_string_parameter(v, "STPIOutputType", "CMY");

/* Allocate buffer for pcl_mode2 tiff compression */

  if ((caps->stp_printer_type & PCL_PRINTER_TIFF) == PCL_PRINTER_TIFF &&
      !(stp_get_debug_level() & STP_DBG_NO_COMPRESSION))
  {
    privdata.comp_buf = stp_malloc((privdata.height + 128 + 7) * 129 / 128);
    privdata.writefunc = pcl_mode2;
  }
  else
  {
    privdata.comp_buf = NULL;
    privdata.writefunc = pcl_mode0;
  }

/* Set up dithering for special printers. */

#if 1		/* Leave alone for now */
  if (!stp_check_float_parameter(v, "GCRLower", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(v, "GCRLower", .3);
  if (!stp_check_float_parameter(v, "GCRUpper", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(v, "GCRUpper", .999);
#endif
  stp_dither_init(v, image, out_width, xdpi, ydpi);

  if (black)
    {
      stp_dither_add_channel(v, black, STP_ECOLOR_K, 0);
      stp_channel_set_black_channel(v, STP_ECOLOR_K);
    }
  if (cyan)
    stp_dither_add_channel(v, cyan, STP_ECOLOR_C, 0);
  if (lcyan)
    stp_dither_add_channel(v, lcyan, STP_ECOLOR_C, 1);
  if (magenta)
    stp_dither_add_channel(v, magenta, STP_ECOLOR_M, 0);
  if (lmagenta)
    stp_dither_add_channel(v, lmagenta, STP_ECOLOR_M, 1);
  if (yellow)
    stp_dither_add_channel(v, yellow, STP_ECOLOR_Y, 0);

/* Ensure that density does not exceed 1.0 */

  if (!stp_check_float_parameter(v, "Density", STP_PARAMETER_DEFAULTED))
    {
      stp_set_float_parameter_active(v, "Density", STP_PARAMETER_ACTIVE);
      stp_set_float_parameter(v, "Density", 1.0);
    }

  stp_deprintf(STP_DBG_PCL, "Density: %f\n", stp_get_float_parameter(v, "Density"));
  if (stp_get_float_parameter(v, "Density") > 1.0)
    stp_set_float_parameter(v, "Density", 1.0);
  density = stp_get_float_parameter(v, "Density");

  if (privdata.do_cret)			/* 4-level printing for 800/1120 */
    {
      if (yellow)
	stp_dither_set_inks_simple(v, STP_ECOLOR_Y, 3, dot_sizes_use, 1.0, 0.08);
      if (black && !privdata.do_cretb)
        stp_dither_set_inks_simple(v, STP_ECOLOR_K, 3, dot_sizes_use, 1.0, 1.0);

      /* Note: no printer I know of does both CRet (4-level) and 6 colour, but
	 what the heck. variable_dither_ranges copied from print-escp2.c */

      if (privdata.do_6color)			/* Photo for 69x */
	{
	  stp_dither_set_inks_full(v, STP_ECOLOR_C, 6, variable_shades, 1.0,
				    0.31 / .5);
	  stp_dither_set_inks_full(v, STP_ECOLOR_M, 6, variable_shades, 1.0,
				    0.61 / .97);
	}
      else
	{
	  if (cyan)
	    stp_dither_set_inks_simple(v, STP_ECOLOR_C, 3, dot_sizes_use, 1.0,
				       0.31 / .5);
	  if (magenta)
	    stp_dither_set_inks_simple(v, STP_ECOLOR_M, 3, dot_sizes_use, 1.0,
				       0.61 / .7);
	}
    }
  else if (privdata.do_6color)
    {
      /* Set light inks for 6 colour printers.
	 Numbers copied from print-escp2.c */
      stp_dither_set_inks_full(v, STP_ECOLOR_C, 2, photo_dither_shades, 1.0,
				0.31 / .5);
      stp_dither_set_inks_full(v, STP_ECOLOR_M, 2, photo_dither_shades, 1.0,
				0.61 / .7);
    }
  if (black)
    stp_channel_set_density_adjustment(v, STP_ECOLOR_K, 0,
				       get_double_param(v, "BlackDensity") *
				       get_double_param(v, "Density"));
  if (cyan)
    stp_channel_set_density_adjustment(v, STP_ECOLOR_C, 0,
				       get_double_param(v, "CyanDensity") *
				       get_double_param(v, "Density"));
  if (magenta)
    stp_channel_set_density_adjustment(v, STP_ECOLOR_M, 0,
				       get_double_param(v, "MagentaDensity") *
				       get_double_param(v, "Density"));
  if (yellow)
    stp_channel_set_density_adjustment(v, STP_ECOLOR_Y, 0,
					get_double_param(v, "YellowDensity") *
					get_double_param(v, "Density"));
  if (lcyan)
    stp_channel_set_density_adjustment
      (v, STP_ECOLOR_C, 1, (get_double_param(v, "CyanDensity") *
			get_double_param(v, "LightCyanTrans") *
			get_double_param(v, "Density")));
  if (lmagenta)
    stp_channel_set_density_adjustment
      (v, STP_ECOLOR_M, 1, (get_double_param(v, "MagentaDensity") *
			get_double_param(v, "LightMagentaTrans") *
			get_double_param(v, "Density")));


  if (!stp_check_curve_parameter(v, "HueMap", STP_PARAMETER_ACTIVE))
    {
      hue_adjustment = stp_curve_create_from_string(standard_hue_adjustment);
      stp_set_curve_parameter(v, "HueMap", hue_adjustment);
      stp_curve_destroy(hue_adjustment);
    }
  if (!stp_check_curve_parameter(v, "LumMap", STP_PARAMETER_ACTIVE))
    {
      lum_adjustment = stp_curve_create_from_string(standard_lum_adjustment);
      stp_curve_destroy(lum_adjustment);
    }

  out_channels = stp_color_init(v, image, 65536);

  errdiv  = image_height / out_height;
  errmod  = image_height % out_height;
  errval  = 0;
  errlast = -1;
  errline  = 0;
  privdata.blank_lines = 0;
#ifndef PCL_DEBUG_DISABLE_BLANKLINE_REMOVAL
  privdata.do_blank = ((caps->stp_printer_type & PCL_PRINTER_BLANKLINE) ==
		       PCL_PRINTER_BLANKLINE);
#else
  privdata.do_blank = 0;
#endif
  stp_allocate_component_data(v, "Driver", NULL, NULL, &privdata);

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
    pcl_printfunc(v);
    stp_deprintf(STP_DBG_PCL,"pcl_print: y = %d, line = %d, val = %d, mod = %d, height = %d\n",
		  y, errline, errval, errmod, out_height);
    errval += errmod;
    errline += errdiv;
    if (errval >= out_height)
    {
      errval -= out_height;
      errline ++;
    }
  }

/* Output trailing blank lines (may not be required?) */

  if (privdata.blank_lines > 1)
  {
    privdata.blank_lines--;		/* correct for one already output */
    stp_deprintf(STP_DBG_PCL, "Blank Lines = %d\n", privdata.blank_lines);
    stp_zprintf(v, "\033*b%dY", privdata.blank_lines);
    privdata.blank_lines=0;
  }

  stp_image_conclude(image);

 /*
  * Cleanup...
  */

  if (black != NULL)
    stp_free(black);
  if (cyan != NULL)
  {
    stp_free(cyan);
    stp_free(magenta);
    stp_free(yellow);
  }
  if (lcyan != NULL)
  {
    stp_free(lcyan);
    stp_free(lmagenta);
  }

  if (privdata.comp_buf != NULL)
    stp_free(privdata.comp_buf);

  if ((caps->stp_printer_type & PCL_PRINTER_NEW_ERG) == PCL_PRINTER_NEW_ERG)
    stp_puts("\033*rC", v);
  else
    stp_puts("\033*rB", v);

  if ((privdata.duplex == 1) && ((page_number & 1) == 0))
      stp_puts("\014", v);		/* Form feed */
  else
    {
      stp_puts("\033&l0H", v);		/* Eject page */
      if (privdata.do_cretb)
        stp_zprintf(v, "\033%%-12345X\n");
      stp_puts("\033E", v); 		/* PCL reset */
    }
  return status;
}

static int
pcl_print(const stp_vars_t *v, stp_image_t *image)
{
  int status;
  stp_vars_t *nv = stp_vars_create_copy(v);
  stp_prune_inactive_options(nv);
  status = pcl_do_print(nv, image);
  stp_vars_destroy(nv);
  return status;
}

static const stp_printfuncs_t print_pcl_printfuncs =
{
  pcl_list_parameters,
  pcl_parameters,
  stp_default_media_size,
  pcl_imageable_area,
  pcl_imageable_area,
  pcl_limit,
  pcl_print,
  pcl_describe_resolution,
  pcl_describe_output,
  stp_verify_printer_params,
  NULL,
  NULL,
  NULL
};


/*
 * 'pcl_mode0()' - Send PCL graphics using mode 0 (no) compression.
 */

static void
pcl_mode0(stp_vars_t *v,		/* I - Print file or command */
          unsigned char *line,		/* I - Output bitmap data */
          int           height,		/* I - Height of bitmap data */
          int           last_plane)	/* I - True if this is the last plane */
{
  stp_zprintf(v, "\033*b%d%c", height, last_plane ? 'W' : 'V');
  stp_zfwrite((const char *) line, height, 1, v);
}


/*
 * 'pcl_mode2()' - Send PCL graphics using mode 2 (TIFF) compression.
 */

static void
pcl_mode2(stp_vars_t *v,		/* I - Print file or command */
          unsigned char *line,		/* I - Output bitmap data */
          int           height,		/* I - Height of bitmap data */
          int           last_plane)	/* I - True if this is the last plane */
{
  pcl_privdata_t *privdata =
    (pcl_privdata_t *) stp_get_component_data(v, "Driver");
  unsigned char *comp_buf = privdata->comp_buf;
  unsigned char	*comp_ptr;		/* Current slot in buffer */

  stp_pack_tiff(v, line, height, comp_buf, &comp_ptr, NULL, NULL);

 /*
  * Send a line of raster graphics...
  */

  stp_zprintf(v, "\033*b%d%c", (int)(comp_ptr - comp_buf), last_plane ? 'W' : 'V');
  stp_zfwrite((const char *)comp_buf, comp_ptr - comp_buf, 1, v);
}


static stp_family_t print_pcl_module_data =
  {
    &print_pcl_printfuncs,
    NULL
  };


static int
print_pcl_module_init(void)
{
  return stp_family_register(print_pcl_module_data.printer_list);
}


static int
print_pcl_module_exit(void)
{
  return stp_family_unregister(print_pcl_module_data.printer_list);
}


/* Module header */
#define stp_module_version print_pcl_LTX_stp_module_version
#define stp_module_data print_pcl_LTX_stp_module_data

stp_module_version_t stp_module_version = {0, 0};

stp_module_t stp_module_data =
  {
    "pcl",
    VERSION,
    "PCL family driver",
    STP_MODULE_CLASS_FAMILY,
    NULL,
    print_pcl_module_init,
    print_pcl_module_exit,
    (void *) &print_pcl_module_data
  };

