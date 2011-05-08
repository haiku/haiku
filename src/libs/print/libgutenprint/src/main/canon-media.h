/*
 *   Print plug-in CANON BJL driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and
 *      Andy Thaller (thaller@ph.tum.de)
 *   Copyright (c) 2006 Sascha Sommer (saschasommer@freenet.de)
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

/* This file contains the definitions for the possible Media Types
   TODO: Color Correction and Density adjustment 
*/

#ifndef GUTENPRINT_INTERNAL_CANON_MEDIA_H
#define GUTENPRINT_INTERNAL_CANON_MEDIA_H

/* media related structs */


/* media slots */

typedef struct {
  const char* name;
  const char* text;
  unsigned char code;
} canon_slot_t;

typedef struct {
  const char *name;
  short count;
  const canon_slot_t *slots;
} canon_slotlist_t;

#define DECLARE_SLOTS(name)                           \
static const canon_slotlist_t name##_slotlist = {     \
  #name,                                              \
  sizeof(name##_slots) / sizeof(canon_slot_t),        \
  name##_slots                                        \
}


static const canon_slot_t canon_default_slots[] = {
  { "Auto",       N_ ("Auto Sheet Feeder"), 4 },
  { "Manual",     N_ ("Manual with Pause"), 0 },
  { "ManualNP",   N_ ("Manual without Pause"), 1 },
  { "Cassette",   N_ ("Cassette"), 8 },
  { "CD",         N_ ("CD tray"), 10 },
};
DECLARE_SLOTS(canon_default);

/* Gernot: changes 2010-10-02 */
static const canon_slot_t canon_PIXMA_iP4000_slots[] = {
  { "SelectKey",  N_ ("Selected by Paper Select Key"), 0x3 },
  { "Auto",       N_ ("Auto Sheet Feeder"), 0x4 },
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "CD",         N_ ("CD tray"), 0xa },
  { "AutoSwitch", N_ ("Automatic Paper Source Switching"), 0xf },
  { "AllocPaper", N_ ("PaperAllocation"), 0x15 },/*Paper allocation? no idea what this means compared to Continuous*/
};
DECLARE_SLOTS(canon_PIXMA_iP4000);

static const canon_slot_t canon_PIXMA_iP3100_slots[] = {
  { "SelectKey",  N_ ("Selected by Paper Select Key"), 0x3 },
  { "Auto",       N_ ("Auto Sheet Feeder"), 0x4 },
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "CD",         N_ ("CD tray"), 0xa },
  { "AutoSwitch", N_ ("Continuous Autofeed"), 0xf },
};
DECLARE_SLOTS(canon_PIXMA_iP3100);

/* MP170, MP450, MP460, MX300 */
static const canon_slot_t canon_MULTIPASS_MP170_slots[] = {
  { "Auto",       N_ ("Auto Sheet Feeder"), 0x4 },
};
DECLARE_SLOTS(canon_MULTIPASS_MP170);

/* MP250 */
/* iP2700 */
/* iP1900 */
static const canon_slot_t canon_MULTIPASS_MP250_slots[] = {
  { "Rear",       N_ ("Rear tray"), 0x4 },
};
DECLARE_SLOTS(canon_MULTIPASS_MP250);

/* MX7600 */
static const canon_slot_t canon_MULTIPASS_MX7600_slots[] = {
  { "Cassette",       N_ ("Cassette"), 0x8 },
};
DECLARE_SLOTS(canon_MULTIPASS_MX7600);

static const canon_slot_t canon_PIXMA_iX7000_slots[] = {
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "HandFeed",  N_ ("Hand Feeding"), 0x1 },
  { "Rear",       N_ ("Rear tray"), 0x4 },
};
DECLARE_SLOTS(canon_PIXMA_iX7000);

/* iP4500 */
static const canon_slot_t canon_PIXMA_iP4500_slots[] = {
  { "SelectKey",  N_ ("Selected by Paper Select Key"), 0x3 },
  { "Rear",       N_ ("Rear tray"), 0x4 },
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "Continuous", N_ ("Continuous Autofeed (both)"), 0xf }, /* no paper automatic change source*/
  { "CD",         N_ ("CD tray"), 0xa }, /* CD-R tray F */
  { "AutoSwitch", N_ ("Automatic Paper Source Switching"), 15 },/*Paper alloction? no idea what this means compared to Continuous*/
};
DECLARE_SLOTS(canon_PIXMA_iP4500);

/* MX850 */
static const canon_slot_t canon_MULTIPASS_MX850_slots[] = {
  { "SelectKey",  N_ ("Selected by Paper Select Key"), 0x3 },
  { "Rear",       N_ ("Rear tray"), 0x4 },
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "Continuous", N_ ("Continuous Autofeed (both)"), 0xf }, /* no paper automatic change source*/
  { "CD",         N_ ("CD tray"), 0xa }, /* CD-R tray F */
};
DECLARE_SLOTS(canon_MULTIPASS_MX850);

static const canon_slot_t canon_MULTIPASS_MP520_slots[] = {
  { "SelectKey",  N_ ("Selected by Paper Select Key"), 0x3 },
  { "Rear",       N_ ("Rear tray"), 0x4 },
  { "Front",   N_ ("Front tray"), 0x8 },
  /* There is also a "plain media only Front" option, but it seems to have same 0x8 when used, no idea whether it should take different values */
};
DECLARE_SLOTS(canon_MULTIPASS_MP520);

/* iP4600, iP4700 */
static const canon_slot_t canon_PIXMA_iP4600_slots[] = {
  { "AutoSwitch", N_ ("Automatic Paper Source Switching"), 0xe },
  { "Rear",       N_ ("Rear tray"), 0x4 },
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "Continuous", N_ ("Continuous Autofeed (both)"), 0xf },
  { "CD",         N_ ("CD tray"), 0xa }
};
DECLARE_SLOTS(canon_PIXMA_iP4600);

/* Pro9000, Pro9500 series */
static const canon_slot_t canon_PIXMA_Pro9000_slots[] = {
  { "Rear",       N_ ("Rear tray"), 0x4 },
  { "Front",   N_ ("Front tray"), 0xb },
  { "CD",         N_ ("CD tray"), 0xa }
};
DECLARE_SLOTS(canon_PIXMA_Pro9000);


/* Gernot --- added so check this*/
static const canon_slot_t canon_PIXMA_MG5100_slots[] = {
  { "AutoSwitch", N_ ("Automatic Paper Source Switching"), 0xe },
  { "Rear",       N_ ("Rear tray"), 0x4 },
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "Continuous", N_ ("Continuous Autofeed (both)"), 0xf },
  { "AllocPaper", N_ ("PaperAllocation"), 0x15 },/*Paper allocation? no idea what this means compared to Continuous*/
};
DECLARE_SLOTS(canon_PIXMA_MG5100);

/* Gernot --- added so check this*/
static const canon_slot_t canon_PIXMA_MG5200_slots[] = {
  { "AutoSwitch", N_ ("Automatic Paper Source Switching"), 0xe },
  { "Rear",       N_ ("Rear tray"), 0x4 },
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "Continuous", N_ ("Continuous Autofeed (both)"), 0xf },
  { "AllocPaper", N_ ("PaperAllocation"), 0x15 },/*Paper allocation? no idea what this means compared to Continuous*/
  { "CD",         N_ ("CD tray"), 0xa }
};
DECLARE_SLOTS(canon_PIXMA_MG5200); /* also MG6100 */


/* media types */

typedef struct {
  const char *name;                        /* Internal Name may not contain spaces */
  const char *text;                        /* Translateable name */
  unsigned char media_code_c;              /* Media Code used for the ESC (c (SetColor) command */
  unsigned char media_code_l;              /* Media Code used for the ESC (l (SetTray) command */
  unsigned char media_code_P;              /* Media Code used for the ESC (P (Unknown) command  */
  double base_density;
  double k_lower_scale;
  double k_upper;
  const char *hue_adjustment;
  const char *lum_adjustment;
  const char *sat_adjustment;
} canon_paper_t;

typedef struct {
  const char *name;
  short count;
  const canon_paper_t *papers;
} canon_paperlist_t;

#define DECLARE_PAPERS(name)                            \
static const canon_paperlist_t name##_paperlist = {     \
  #name,                                                \
  sizeof(name##_papers) / sizeof(canon_paper_t),        \
  name##_papers                                         \
}


/* paperlists for the various printers. The first entry will be the default */

static const canon_paper_t canon_default_papers[] = {
  { "Plain",		N_ ("Plain Paper"),		0x00, 0x00,0x00,0.50, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		0x02, 0x02,0x00,1.00, 1.00, 0.900, 0, 0, 0 },
  { "BackPrint",	N_ ("Back Print Film"),		0x03, 0x03,0x00,1.00, 1.00, 0.900, 0, 0, 0 },
  { "Fabric",		N_ ("Fabric Sheets"),		0x04, 0x04,0x00,0.50, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),		0x08, 0x08,0x00,0.50, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),	0x07, 0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),	0x03, 0x03,0x00,0.50, 0.25, 0.500, 0, 0, 0 },
  { "GlossyFilm",	N_ ("High Gloss Film"),		0x06, 0x06,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),	0x05, 0x05,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "GlossyCard",	N_ ("Glossy Photo Cards"),	0x0a, 0x0a,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),		0x09, 0x09,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "Other",		N_ ("Other"),                   0x00, 0x00,0x00,0.50, 0.25, .5, 0, 0, 0 },
};
DECLARE_PAPERS(canon_default);

static const canon_paper_t canon_PIXMA_iP4000_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "Transparency", 	N_ ("Transparencies"),			0x02,0x02,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Photo Paper Matte"),		0x0a,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "CD",		N_ ("CD"),				0x00,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, 
  /* FIXME media code for c) should be 0x0c for CD but this will restrict CD printing to a single, not well supported, resolution */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble", N_ ("Photopaper Plus Double Sided"),0x10,0x15,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP4000);

static const canon_paper_t canon_PIXMA_iP3100_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble", N_ ("Photopaper Plus Double Sided"),0x10,0x15,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Photo Paper Matte"),		0x0a,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency", 	N_ ("Transparencies"),			0x02,0x02,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  /* FIXME media code for c) should be 0x0c for CD but this will restrict CD printing to a single, not well supported, resolution */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* experiment */
};
DECLARE_PAPERS(canon_PIXMA_iP3100);

static const canon_paper_t canon_PIXMA_iP1000_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,1.00, 1.00, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },/*experiment*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },/*experiment*/
};
DECLARE_PAPERS(canon_PIXMA_iP1000);

static const canon_paper_t canon_PIXMA_iP1200_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,1.00, 1.00, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP1200);

static const canon_paper_t canon_PIXMA_iP1500_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },/* experimental */
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x01,1.00, 1.00, 0.900, 0, 0, 0 },/* experimental */
};
DECLARE_PAPERS(canon_PIXMA_iP1500);

static const canon_paper_t canon_PIXMA_iP2200_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x01,1.00, 1.00, 0.900, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP2200);

static const canon_paper_t canon_PIXMA_iP2600_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP kinumecho */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP2600);

static const canon_paper_t canon_PIXMA_iP6100_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble", N_ ("Photopaper Plus Double Sided"),0x10,0x15,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Photo Paper Matte"),		0x0a,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "CD",		N_ ("CD"),				0x0c,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, 
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency", 	N_ ("Transparencies"),			0x02,0x02,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP6100);

/* iP6600D, Tray C for CD media */
static const canon_paper_t canon_PIXMA_iP6600_papers[] = { /*               k_lower_scale   *hue_adjustment *sat_adjustment */
  /* Name                    Text                               (c   (l   (P   Density    k_upper    *lum_adjustment */
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP6600);

/* iP6700D, Tray C for CD media */
static const canon_paper_t canon_PIXMA_iP6700_papers[] = { /*               k_lower_scale   *hue_adjustment *sat_adjustment */
  /* Name                    Text                               (c   (l   (P   Density    k_upper    *lum_adjustment */
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x16,0x18,0x29,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP6700);

/* PIXMA Pro9000 */
static const canon_paper_t canon_PIXMA_Pro9000_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0.78, 0.25, 0.500, 0, 0, 0 },/*PPsuper*/
  { "PhotoPlusGloss2",	N_ ("Photo Paper Plus Glossy II"), 	0x0b,0x11,0x32,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGgold*/
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },/*PPgloss*/
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 },/*PPkinumecho*/
  { "PhotopaperPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0.78, 0.25, 0.500, 0, 0, 0 },/*PPpro*/
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x09,0x0d,0x34,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGpro*/
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),	        0x09,0x11,0x33,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGproPlat*/
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x20,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },/*check l*/
  /* special papers */
  { "Boardpaper",	N_ ("Board Paper"),		        0x18,0x1d,0x2e,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Canvas",	        N_ ("Canvas"),		                0x19,0x1e,0x2d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x16,0x1b,0x29,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPremiumMatte",N_ ("Fine Art Premium Matte"),	0x15,0x1a,0x2c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtMuseumEtching",N_ ("Fine Art Museum Etching"),      0x14,0x19,0x31,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_Pro9000);

/* PIXMA Pro9000 Mk.II */
static const canon_paper_t canon_PIXMA_Pro9000mk2_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGgold*/
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },/*PPgloss*/
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 },/*PP kinumecho*/
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGpro*/
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),	        0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGproPlat*/
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  /* special papers */
  { "Boardpaper",	N_ ("Board Paper"),		        0x18,0x1d,0x2e,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Canvas",	        N_ ("Canvas"),		                0x19,0x1e,0x2d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x16,0x1b,0x29,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPremiumMatte",N_ ("Fine Art Premium Matte"),	0x15,0x1a,0x2c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtMuseumEtching",N_ ("Fine Art Museum Etching"),      0x14,0x19,0x31,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_Pro9000mk2);

/* PIXMA Pro9500 */
/* added Envelope, glossy photo paper, coated, T-shirt, PhotopaperOther---must check printjobs if I forgot these ones */
static const canon_paper_t canon_PIXMA_Pro9500_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGgold*/
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },/* check */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 },/*PP kinumecho*/
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGpro*/
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),	        0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGproPlat*/
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },/* check */
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },/* check */
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },/* check */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },/* check */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  /* special papers */
  { "Boardpaper",	N_ ("Board Paper"),		        0x18,0x1d,0x2e,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Canvas",	        N_ ("Canvas"),		                0x19,0x1e,0x2d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x16,0x1b,0x29,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPremiumMatte",N_ ("Fine Art Premium Matte"),	0x15,0x1a,0x2c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtMuseumEtching",N_ ("Fine Art Museum Etching"),	0x14,0x19,0x31,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_Pro9500);

/* PIXMA Pro9500 Mk.II */
/* added Envelope, glossy photo paper, coated, T-shirt, PhotopaperOther---must check printjobs if I forgot these ones */
static const canon_paper_t canon_PIXMA_Pro9500mk2_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGgold*/
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },/* check */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 },/*PP kinumecho*/
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },/* check */
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },/* check */
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },/* check */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },/* check */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  /* special papers */
  { "Boardpaper",	N_ ("Board Paper"),		        0x18,0x1d,0x2e,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Canvas",	        N_ ("Canvas"),		                0x19,0x1e,0x2d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x16,0x1b,0x29,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPremiumMatte",N_ ("Fine Art Premium Matte"),	0x15,0x1a,0x2c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtMuseumEtching",N_ ("Fine Art Museum Etching"),	0x14,0x19,0x31,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_Pro9500mk2);

/* Gernot: added ---- note: ESC ( P code not used at all yet, check print-canon.c */
/* MP150 so far */
static const canon_paper_t canon_MULTIPASS_MP150_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MP150);

/* Gernot: added ---- note: ESC ( P code not used at all yet, check print-canon.c */
/* MP170, MP450, MP460 */
static const canon_paper_t canon_MULTIPASS_MP170_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MP170);

/* MX300, MX310, MX700 */
static const canon_paper_t canon_MULTIPASS_MX300_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotopaperPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MX300);

/* MX330 */
static const canon_paper_t canon_MULTIPASS_MX330_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGpro */
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),	        0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGproPlat */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP kinumecho */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MX330);

/* MX340 --- MX350 is the same */
static const canon_paper_t canon_MULTIPASS_MX340_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGpro */
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),	        0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGproPlat */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP kinumecho */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0.78, 0.25, 0.500, 0, 0, 0 }, /* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0.78, 0.25, 0.500, 0, 0, 0 }, /* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MX340);

/* MX360 --- MX410 is the same */
static const canon_paper_t canon_MULTIPASS_MX360_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),	        0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGproPlat */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP kinumecho */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0.78, 0.25, 0.500, 0, 0, 0 }, /* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0.78, 0.25, 0.500, 0, 0, 0 }, /* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MX360);

/* MX420 */
static const canon_paper_t canon_MULTIPASS_MX420_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),	        0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGproPlat */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0.78, 0.25, 0.500, 0, 0, 0 }, /* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0.78, 0.25, 0.500, 0, 0, 0 }, /* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MX420);

/* MX850 */
static const canon_paper_t canon_MULTIPASS_MX850_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotopaperPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoSemiGloss",N_ ("Photo Paper Semi-gloss"),	        0x0b,0x11,0x2a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP kinumecho new !! */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },/*check c,l*/
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },/*check c,l*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MX850);

static const canon_paper_t canon_MULTIPASS_MX880_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 }, /* plain */
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),	        0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGproPlat */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP kinumecho */
  { "GlossyPhoto",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPG */
  { "MattePhoto",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP matte */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 }, /* env */
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0.78, 0.25, 0.500, 0, 0, 0 }, /* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 }, /* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0.78, 0.25, 0.500, 0, 0, 0 }, /* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 }, /* hagaki*/
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 }, /* hi res paper */
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 }, /* T-shirt */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP other */
};
DECLARE_PAPERS(canon_MULTIPASS_MX880);

/* MX7600 */
static const canon_paper_t canon_MULTIPASS_MX7600_papers[] = { /*               k_lower_scale   *hue_adjustment *sat_adjustment */
  /* Name                    Text                               (c   (l   (P   Density    k_upper    *lum_adjustment */
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*OK*/
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "MattePhoto",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },/*check c,l*/
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP kinumecho */
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },/*check c*/
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0.78, 0.25, 0.500, 0, 0, 0 },/*check*/
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x16,0x18,0x29,0.78, 0.25, 0.500, 0, 0, 0 },/*check*/
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },/*check c*/
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },/*check variations*/
};
DECLARE_PAPERS(canon_MULTIPASS_MX7600);

/* iX7000 */
static const canon_paper_t canon_PIXMA_iX7000_papers[] = { /*               k_lower_scale   *hue_adjustment *sat_adjustment */
  /* Name                    Text                               (c   (l   (P   Density    k_upper    *lum_adjustment */
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),	        0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGproPlat */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 },/* PP kinumecho */
  { "MattePhoto",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x13,0x18,0x29,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iX7000);

/* Gernot: added ---- note: ESC ( P code not used at all yet, check print-canon.c */
/* MP250 */
static const canon_paper_t canon_MULTIPASS_MP250_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 }, /* plain */
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGpro */
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),	        0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGproPlat */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP kinumecho */
  { "GlossyPhoto",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPG */
  { "MattePhoto",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP matte */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 }, /* hi res paper */
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0.78, 0.25, 0.500, 0, 0, 0 }, /* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 }, /* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0.78, 0.25, 0.500, 0, 0, 0 }, /* Canon photo hagaki*/
  { "ProPhotoHagakiP",  N_ ("Hagaki P (Pro Photo)"),		0x1f,0x25,0x37,0.78, 0.25, 0.500, 0, 0, 0 }, /* pro photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 }, /* hagaki*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 }, /* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 }, /* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP other */
};
DECLARE_PAPERS(canon_MULTIPASS_MP250);

/* Gernot: added ---- note: ESC ( P code not used at all yet, check print-canon.c */
/* MP280 */
static const canon_paper_t canon_MULTIPASS_MP280_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 }, /* plain */
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),	        0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGproPlat */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP kinumecho */
  { "GlossyPhoto",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPG */
  { "MattePhoto",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP matte */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 }, /* hi res paper */
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0.78, 0.25, 0.500, 0, 0, 0 }, /* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 }, /* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0.78, 0.25, 0.500, 0, 0, 0 }, /* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 }, /* hagaki*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 }, /* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 }, /* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP other */
};
DECLARE_PAPERS(canon_MULTIPASS_MP280);

/* Gernot: added ---- note: ESC ( P code not used at all yet, check print-canon.c */
/* MP480 */
static const canon_paper_t canon_MULTIPASS_MP480_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 }, /* plain */
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGpro */
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),	        0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGproPlat */
  { "GlossyPhoto",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPG */
  { "MattePhoto",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP matte */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP other */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 }, /* hi res paper */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 }, /* inkjet hagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 }, /* hagaki*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 }, /* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 }, /* env */
};
DECLARE_PAPERS(canon_MULTIPASS_MP480);

static const canon_paper_t canon_MULTIPASS_MP493_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 }, /* plain */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGpro */
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),	        0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGproPlat */
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "GlossyPhoto",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPG */
  { "MattePhoto",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP matte */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP kinumecho */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP other */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 }, /* hi res paper */
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 }, /* T-shirt */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 }, /* inkjet hagaki */
  { "ProPhotoHagakiO",  N_ ("Hagaki Pro Photo"),		0x1f,0x25,0x37,0.78, 0.25, 0.500, 0, 0, 0 }, /* Pro photo hagaki*/
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0.78, 0.25, 0.500, 0, 0, 0 }, /* Canon photo hagaki*/
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0.78, 0.25, 0.500, 0, 0, 0 }, /* all hagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 }, /* hagaki*/
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 }, /* env */
};
DECLARE_PAPERS(canon_MULTIPASS_MP493);

/* MP520 */
static const canon_paper_t canon_MULTIPASS_MP520_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 }, /* plain */
  { "PhotopaperPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPpro*/
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x1d,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPsuper */
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPsuperDS */
  { "GlossyPhoto",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPgloss */
  { "MattePhoto",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPmatte */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPother */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 }, /* HiRes */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 }, /* inkjetHagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 }, /* hagaki*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 }, /* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 }, /* env */
};
DECLARE_PAPERS(canon_MULTIPASS_MP520);

/* iP1900 series */
static const canon_paper_t canon_PIXMA_iP1900_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 }, /* plain */
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGpro */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP kinumecho */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 }, /* inkjet hagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 }, /* hagaki*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 }, /* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 }, /* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP other */
};
DECLARE_PAPERS(canon_PIXMA_iP1900);

/* Gernot: added ---- note: ESC ( P code not used at all yet, check print-canon.c */
/* MP600 */
static const canon_paper_t canon_MULTIPASS_MP600_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 }, /* plain */
  { "PhotopaperPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (recommended)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (others)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MP600);

/* MP640 */
static const canon_paper_t canon_MULTIPASS_MP630_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 }, /* plain */
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGpro */
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),	        0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGproPlat */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP kinumecho */
  { "GlossyPhoto",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPG */
  { "MattePhoto",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP matte */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 }, /* hi res paper */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 }, /* inkjet hagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 }, /* hagaki*/
  { "DiscCompat",	N_ ("Printable Disc (recommended)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (others)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 }, /* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 }, /* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP other */
};
DECLARE_PAPERS(canon_MULTIPASS_MP630);

/* MP640 */
static const canon_paper_t canon_MULTIPASS_MP640_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 }, /* plain */
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGpro */
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),	        0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGproPlat */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP kinumecho */
  { "GlossyPhoto",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPG */
  { "MattePhoto",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP matte */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 }, /* hi res paper */
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0.78, 0.25, 0.500, 0, 0, 0 }, /* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 }, /* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0.78, 0.25, 0.500, 0, 0, 0 }, /* Canon photo hagaki*/
  { "ProPhotoHagakiP",  N_ ("Hagaki P (Pro Photo)"),		0x1f,0x25,0x37,0.78, 0.25, 0.500, 0, 0, 0 }, /* pro photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 }, /* hagaki*/
  { "DiscCompat",	N_ ("Printable Disc (recommended)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (others)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 }, /* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 }, /* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP other */
};
DECLARE_PAPERS(canon_MULTIPASS_MP640);

/* Gernot: added ---- note: ESC ( P code not used at all yet, check print-canon.c */
/* iP2700 series */
static const canon_paper_t canon_PIXMA_iP2700_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 }, /* plain */
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGpro */
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),	        0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGproPlat */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP kinumecho */
  { "GlossyPhoto",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPG */
  { "MattePhoto",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP matte */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 }, /* hi res paper */
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0.78, 0.25, 0.500, 0, 0, 0 }, /* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 }, /* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0.78, 0.25, 0.500, 0, 0, 0 }, /* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 }, /* hagaki*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 }, /* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 }, /* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP other */
};
DECLARE_PAPERS(canon_PIXMA_iP2700);

/* Gernot: added ---- note: ESC ( P code not used at all yet, check print-canon.c */
/* iP4500 */
static const canon_paper_t canon_PIXMA_iP4500_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 }, /* plain */
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotopaperPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (recommended)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (others)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP4500);

static const canon_paper_t canon_PIXMA_iP4600_papers[] = {    /*                  k_lower_scale   *hue_adjustment *sat_adjustment */
  /* Name                    Text                               (c   (l   (P   Density    k_upper    *lum_adjustment */
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2", 	N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),		0x1f,0x25,0x34,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),		0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPhoto",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "MattePhoto",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HighResolution",	N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirtTransfer",	N_ ("T-Shirt Transfer"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Other",		N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 }
};
DECLARE_PAPERS(canon_PIXMA_iP4600);

static const canon_paper_t canon_PIXMA_iP4700_papers[] = {    /*                  k_lower_scale   *hue_adjustment *sat_adjustment */
  /* Name                    Text                               (c   (l   (P   Density    k_upper    *lum_adjustment */
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2", 	N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),		0x1f,0x25,0x34,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),		0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPhoto",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "MattePhoto",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HighResolution",	N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "OtherPhotoHagakiO",N_ ("Hagaki O (Other Photo)"),		0x1f,0x25,0x37,0.78, 0.25, 0.500, 0, 0, 0 },/* Other photo hagaki*/
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirtTransfer",	N_ ("T-Shirt Transfer"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Other",		N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 }
};
DECLARE_PAPERS(canon_PIXMA_iP4700);

static const canon_paper_t canon_MULTIPASS_MP960_papers[] = { /*                  k_lower_scale   *hue_adjustment *sat_adjustment */
  /* Name                    Text                               (c   (l   (P   Density    k_upper    *lum_adjustment */
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*OK*/
  { "GlossyPro",	N_ ("Photo Paper Pro"),		        0x09,0x0d,0x1a,1.00, 1.00, 0.999, 0, 0, 0 },/*check*/
  { "PhotoSuper",       N_ ("Photo Paper Super"),	        0x1b,0x11,0x1d,0.78, 0.25, 0.500, 0, 0, 0 },/*check*/
  { "GlossyPhoto",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },/*check c*/
  { "MattePhoto",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },/*check c,l*/
  { "HighResolution",	N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },/*OK*/
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },/*check c*/
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },/*check c*/
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },/*check c,l*/
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },/*check c,l*/
  { "TShirtTransfer",	N_ ("T-Shirt Transfer"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },/*check c*/
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },/*check variations*/
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0.78, 0.25, 0.500, 0, 0, 0 },/*check*/
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x16,0x18,0x29,0.78, 0.25, 0.500, 0, 0, 0 },/*check*/
  { "PhotopaperPlusDouble", N_ ("Photopaper Plus Double Sided"),0x10,0x15,0x25,0.78, 0.25, 0.500, 0, 0, 0 },/*check c,l*/
  { "Other",		N_ ("Other Coated Photo Paper"),	0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 } /*coated, OK*/
};
DECLARE_PAPERS(canon_MULTIPASS_MP960);

static const canon_paper_t canon_MULTIPASS_MP980_papers[] = {    /*  k_lower_scale   *hue_adjustment *sat_adjustment */
  /* Name                    Text                               (c   (l   (P   Density    k_upper    *lum_adjustment */
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2", 	N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),		0x1f,0x25,0x34,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),		0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPhoto",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "MattePhoto",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x13,0x18,0x29,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirtTransfer",	N_ ("T-Shirt Transfer"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Other",		N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MP980);

static const canon_paper_t canon_MULTIPASS_MP990_papers[] = {    /*  k_lower_scale   *hue_adjustment *sat_adjustment */
  /* Name                    Text                               (c   (l   (P   Density    k_upper    *lum_adjustment */
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2", 	N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),		0x1f,0x25,0x34,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),		0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPhoto",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "MattePhoto",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Other",		N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirtTransfer",	N_ ("T-Shirt Transfer"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0.78, 0.25, 0.500, 0, 0, 0 },/*check*/
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x16,0x18,0x29,0.78, 0.25, 0.500, 0, 0, 0 },/*check*/
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HighResolution",	N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "OtherPhotoHagakiO",N_ ("Hagaki O (Other Photo)"),		0x1f,0x25,0x37,0.78, 0.25, 0.500, 0, 0, 0 },/* Other photo hagaki*/
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MP990);

/* Gernot: added ---- note: ESC ( P code not used at all yet, check print-canon.c */
/* MG5100 series */
static const canon_paper_t canon_PIXMA_MG5100_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 }, /* plain */
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),		0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP kinumecho */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0.78, 0.25, 0.500, 0, 0, 0 }, /* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 }, /* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0.78, 0.25, 0.500, 0, 0, 0 }, /* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 }, /* hagaki*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 }, /* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 }, /* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP other */
};
DECLARE_PAPERS(canon_PIXMA_MG5100);

/* Gernot: added ---- note: ESC ( P code not used at all yet, check print-canon.c */
/* MG5200 series */
static const canon_paper_t canon_PIXMA_MG5200_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 }, /* plain */
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),		0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP kinumecho */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0.78, 0.25, 0.500, 0, 0, 0 }, /* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 }, /* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0.78, 0.25, 0.500, 0, 0, 0 }, /* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 }, /* hagaki*/
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },/*check c,l*/
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },/*check c,l*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 }, /* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 }, /* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP other */
};
DECLARE_PAPERS(canon_PIXMA_MG5200);

/* Gernot: added ---- note: ESC ( P code not used at all yet, check print-canon.c */
/* MG6100 series */
static const canon_paper_t canon_PIXMA_MG6100_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 }, /* plain */
  { "PhotoPlusGLoss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0.78, 0.25, 0.500, 0, 0, 0 }, /* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Platinum"),		0x1e,0x24,0x33,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP kinumecho */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0.78, 0.25, 0.500, 0, 0, 0 }, /* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0.78, 0.25, 0.500, 0, 0, 0 }, /* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0.78, 0.25, 0.500, 0, 0, 0 }, /* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0.78, 0.25, 0.500, 0, 0, 0 }, /* hagaki*/
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0.78, 0.25, 0.500, 0, 0, 0 },/*check c,l*/
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0.78, 0.25, 0.500, 0, 0, 0 },/*check c,l*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0.78, 0.25, 0.500, 0, 0, 0 }, /* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0.78, 0.25, 0.500, 0, 0, 0 }, /* env */
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0.78, 0.25, 0.500, 0, 0, 0 },/*check*/
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x16,0x18,0x29,0.78, 0.25, 0.500, 0, 0, 0 },/*check*/
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0.78, 0.25, 0.500, 0, 0, 0 }, /* PP other */
};
DECLARE_PAPERS(canon_PIXMA_MG6100);

#endif

/*
Plain:            Plain Paper, Canon High Resolution Paper;
Super High Gloss: Photo Paper Pro Platinum;
Glossy:           Photo Paper Plus Glossy II, Photo Paper Glossy;
Semi-Gloss:       Photo Paper Plus Semi-Gloss;
Matte:            Canon Matte Photo Paper;
Fine Art:         Canon Fine Art Paper "Photo Rag";
Envelope:         U.S.# 10 Envelope
*/
