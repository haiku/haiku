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

/* This file contains the capabilities of the various canon printers
*/

#ifndef GUTENPRINT_INTERNAL_CANON_PRINTERS_H
#define GUTENPRINT_INTERNAL_CANON_PRINTERS_H


typedef struct canon_caps {
  const char* name;   /* model name */
  int model_id;       /* model ID code for use in commands */
  int max_width;      /* maximum printable paper size */
  int max_height;
  int border_left;    /* left margin, points */
  int border_right;   /* right margin, points */
  int border_top;     /* absolute top margin, points */
  int border_bottom;  /* absolute bottom margin, points */
  int raster_lines_per_block; /* number of raster lines in every F) command */
  const canon_slotlist_t* slotlist; /*available paperslots */
  unsigned long features;  /* special bjl settings */
  unsigned char ESC_r_arg; /* argument used for the ESC (r command during init */
  const char** control_cmdlist;
  const canon_modelist_t* modelist;
  const canon_paperlist_t* paperlist;
  const char *lum_adjustment;
  const char *hue_adjustment;
  const char *sat_adjustment;
  const char *channel_order; /* (in gutenprint notation) 0123 => KCMY,  1230 => CMYK etc. */
} canon_cap_t;

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

static const char iP4200_sat_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* B */
/* B */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* M */
/* M */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* R */
/* R */  "1.00 1.00 1.05 1.10 1.20 1.26 1.34 1.41 "  /* Y */
/* Y */  "1.38 1.32 1.24 1.15 1.08 1.00 1.00 1.00 "  /* G */
/* G */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

static const char iP4200_lum_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "0.52 0.56 0.61 0.67 0.79 0.86 0.91 0.98 "  /* B */
/* B */  "0.97 0.87 0.84 0.81 0.78 0.76 0.74 0.73 "  /* M */
/* M */  "0.74 0.76 0.78 0.83 0.86 0.90 0.98 1.04 "  /* R */
/* R */  "1.04 1.04 1.04 1.04 1.03 1.03 1.03 1.02 "  /* Y */
/* Y */  "1.02 0.97 0.92 0.88 0.83 0.78 0.74 0.71 "  /* G */
/* G */  "0.70 0.62 0.59 0.53 0.48 0.52 0.53 0.51 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

static const char iP4200_hue_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"-6\" upper-bound=\"6\">\n"
/* C */  "0.00 -.20 -.30 -.40 -.40 -.30 -.20 0.00 "  /* B */
/* B */  "0.00 -.04 -.01 0.08 0.14 0.16 0.09 0.00 "  /* M */
/* M */  "0.00 0.00 -.05 -.07 -.10 -.08 -.06 0.00 "  /* R */
/* R */  "0.00 0.04 0.08 0.10 0.13 0.10 0.07 0.00 "  /* Y */
/* Y */  "0.00 -.11 -.18 -.23 -.30 -.37 -.46 -.54 "  /* G */
/* G */  "-.53 -.52 -.57 -.50 -.41 -.25 -.17 0.00 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

static const char* control_cmd_ackshort[] = {
  "AckTime=Short",
  NULL
};

static const char* control_cmd_PIXMA_iP4000[] = {
/*"SetTime=20060722092503", */         /*what is this for?*/
  "SetSilent=OFF",
  "PEdgeDetection=ON",
  "SelectParallel=ECP",
  NULL
};

static const char* control_cmd_PIXMA_iP4200[] = {
/*"SetTime=20060722092503", */         /*original driver sends current time, is it needed?*/
  "SetSilent=OFF",
  "PEdgeDetection=ON",
  NULL
};

static const char* control_cmd_MULTIPASS_MP150[] = {
  "AckTime=Short",
  "MediaDetection=ON",
  NULL
};

static const char* control_cmd_PIXMA_iP2700[] = {
/*"SetTime=20060722092503", */         /*what is this for?*/
  "SetSilent=OFF",
  NULL
};

static const char iP4500_channel_order[STP_NCOLORS] = {1,2,3,0}; /* CMYK */

static const canon_cap_t canon_model_capabilities[] =
{
  /* the first printer is used as default in case something has gone wrong in printers.xml */
  { /* Canon MULTIPASS MP830 */
    "PIXMA MP830", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4000_slotlist,
    /* CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px,0,control_cmd_MULTIPASS_MP150, */  /*features */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_P|CANON_CAP_I,0x64,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MP830_modelist,
    /* &canon_PIXMA_iP4000_paperlist, */
    &canon_MULTIPASS_MP600_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon MULTIPASS MP800 */
    "PIXMA MP800", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_P|CANON_CAP_I,0x64,control_cmd_PIXMA_iP4200,  /*features */
    &canon_MULTIPASS_MP800_modelist,
    &canon_MULTIPASS_MP600_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon MULTIPASS MP810 */
    "PIXMA MP810", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_P|CANON_CAP_I,0x64,control_cmd_PIXMA_iP4200,  /*features */
    &canon_MULTIPASS_MP810_modelist,
    &canon_MULTIPASS_MP600_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA MP950 */
    "PIXMA MP950", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I,0x64,control_cmd_PIXMA_iP4200,  /* features */
    &canon_MULTIPASS_MP950_modelist,
    &canon_PIXMA_iP4000_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA MP960 */
    "PIXMA MP960", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4600_slotlist, /* changed from iP4000 */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I,0x64,control_cmd_PIXMA_iP4200,  /* features */
    &canon_MULTIPASS_MP960_modelist,
    &canon_PIXMA_iP4600_paperlist, /* changed from iP4000 */
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP970 */
    "PIXMA MP970", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_I,0x64,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MP970_modelist,
    &canon_PIXMA_iP4000_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA MP980 */
    "PIXMA MP980", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4600_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_I,0x64,control_cmd_PIXMA_iP4200,  /*features */
    &canon_MULTIPASS_MP980_modelist,
    &canon_MULTIPASS_MP980_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA MP990 --- uses XML */
    "PIXMA MP990", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4600_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML,0x64,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MP990_modelist,
    &canon_MULTIPASS_MP990_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },

  /* ******************************** */
  /*                                  */
  /* tested and color-adjusted models */
  /*                                  */
  /* ******************************** */




  /* ************************************ */
  /*                                      */
  /* tested models w/out color-adjustment */
  /*                                      */
  /* ************************************ */

  { /* Canon S200x *//* heads: BC-24 */
    "S200", 3,
    618, 936,       /* 8.58" x 13 " */
    10, 10, 9, 20,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD1 | CANON_CAP_rr,0x61,NULL,
    &canon_S200_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },

  { /* Canon BJC S300 */
    "S300", 3,
    842, 17*72,
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD1 | CANON_CAP_r,0x61,control_cmd_ackshort,
    &canon_BJC_8500_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },

  { /* Canon  BJ 30   *//* heads: BC-10 */
    "30", 1,
    9.5*72, 14*72,
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD0 | CANON_CAP_a,0,NULL,
    &canon_BJC_30_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon  BJC 85  *//* heads: BC-20 BC-21 BC-22 */
    "85", 1,
    9.5*72, 14*72,
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD0 | CANON_CAP_a,0,NULL,
    &canon_BJC_85_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },

  { /* Canon BJC 4300 *//* heads: BC-20 BC-21 BC-22 BC-29 */
    "4300", 1,
    618, 936,      /* 8.58" x 13 " */
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD0,0,NULL,
    &canon_BJC_4300_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },

  { /* Canon BJC 4400 *//* heads: BC-20 BC-21 BC-22 BC-29 */
    "4400", 1,
    9.5*72, 14*72,
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD0 | CANON_CAP_a,0,NULL,
    &canon_BJC_4400_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },

  { /* Canon BJC 6000 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    "6000", 3,
    618, 936,      /* 8.58" x 13 " */
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD1,0,control_cmd_ackshort,
    &canon_BJC_6000_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },

  { /* Canon BJC 6200 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    "6200", 3,
    618, 936,      /* 8.58" x 13 " */
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD1,0,control_cmd_ackshort,
    &canon_BJC_6000_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },

  { /* Canon BJC 6500 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    "6500", 3,
    842, 17*72,
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD1,0,NULL,
    &canon_BJC_6000_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 8200 *//* heads: BC-50 */
    "8200", 3,
    842, 17*72,
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD1 | CANON_CAP_r,0x61,control_cmd_ackshort,
    &canon_BJC_8200_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon S500 */
    "S500", 3,
    842, 17*72,
    10, 10, 15, 15,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD0 | CANON_CAP_r | CANON_CAP_p,0x61,control_cmd_ackshort,
    &canon_S500_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },


  /* *************** */
  /*                 */
  /* untested models */
  /*                 */
  /* *************** */

  { /* Canon BJC 210 *//* heads: BC-02 BC-05 BC-06 */
    "210", 1,
    618, 936,      /* 8.58" x 13 " */
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD0,0,NULL,
    &canon_BJC_210_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 240 *//* heads: BC-02 BC-05 BC-06 */
    "240", 1,
    618, 936,      /* 8.58" x 13 " */
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD0,0,NULL,
    &canon_BJC_240_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 250 *//* heads: BC-02 BC-05 BC-06 */
    "250", 1,
    618, 936,      /* 8.58" x 13 " */
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD0,0,NULL,
    &canon_BJC_240_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 1000 *//* heads: BC-02 BC-05 BC-06 */
    "1000", 1,
    842, 17*72,
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD0 | CANON_CAP_a,0,NULL,
    &canon_BJC_240_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 2000 *//* heads: BC-20 BC-21 BC-22 BC-29 */
    "2000", 1,
    842, 17*72,
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD0 | CANON_CAP_a,0,NULL,
    &canon_BJC_2000_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 3000 *//* heads: BC-30 BC-33 BC-34 */
    "3000", 3,
    842, 17*72,
    10, 10, 9, 15,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD0 | CANON_CAP_a | CANON_CAP_p,0,NULL, /*FIX? should have _r? */
    &canon_BJC_3000_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 6100 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    "6100", 3,
    842, 17*72,
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD1 | CANON_CAP_a | CANON_CAP_r,0x61,NULL,
    &canon_BJC_3000_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 7000 *//* heads: BC-60/BC-61 BC-60/BC-62   ??????? */
    "7000", 3,
    842, 17*72,
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD1,0,NULL,
    &canon_BJC_7000_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC i560 */
    "i560", 3,
    842, 17*72,
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD1,0,NULL,
    &canon_BJC_i560_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 7100 *//* heads: BC-60/BC-61 BC-60/BC-62   ??????? */
    "7100", 3,
    842, 17*72,
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD0,0,NULL,
    &canon_BJC_7100_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC i80 *//* heads: BC-60/BC-61 BC-60/BC-62   ??????? */
    "i80", 3,
    842, 17*72,
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD0,0,NULL,
    &canon_BJC_i80_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },

  /*****************************/
  /*                           */
  /*  extremely fuzzy models   */
  /* (might never work at all) */
  /*                           */
  /*****************************/

  { /* Canon BJC 5100 *//* heads: BC-20 BC-21 BC-22 BC-23 BC-29 */
    "5100", 1,
    17*72, 22*72,
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD0,0,NULL,
    &canon_BJC_3000_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 5500 *//* heads: BC-20 BC-21 BC-29 */
    "5500", 1,
    22*72, 34*72,
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD0 | CANON_CAP_a,0,NULL,
    &canon_BJC_5500_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 6500 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    "6500", 3,
    17*72, 22*72,
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD1 | CANON_CAP_a,0,NULL,
    &canon_BJC_3000_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 8500 *//* heads: BC-80/BC-81 BC-82/BC-81 */
    "8500", 3,
    17*72, 22*72,
    11, 9, 10, 18,
    8,
    &canon_default_slotlist,
    CANON_CAP_STD0,0,NULL,
    &canon_BJC_8500_modelist,
    &canon_default_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP1000 */
    "PIXMA iP1000", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP170_slotlist, /* auto sheet feeder only */
    CANON_CAP_STD0,0,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP1000_modelist,
    &canon_PIXMA_iP1000_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP1200 --- iP1300 same */
    "PIXMA iP1200", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP170_slotlist, /* auto sheet feeder only */
    CANON_CAP_STD0,0,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP1200_modelist,
    &canon_PIXMA_iP1200_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP1500 */
    "PIXMA iP1500", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP170_slotlist, /* auto sheet feeder only */
    CANON_CAP_STD0,0,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP1500_modelist,
    &canon_PIXMA_iP1500_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP1600 */
    "PIXMA iP1600", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP170_slotlist, /* auto sheet feeder only */
    CANON_CAP_STD0,0,control_cmd_PIXMA_iP4000,  /*features */
    &canon_MULTIPASS_MP170_modelist,
    &canon_PIXMA_iP1500_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP1900 */
    "PIXMA iP1900", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP250_slotlist, /* rear only */
    CANON_CAP_STD0,0,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP1900_modelist,
    &canon_PIXMA_iP1900_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP2000 */
    "PIXMA iP2000", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px,0x61,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP2000_modelist,
    &canon_PIXMA_iP4000_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP2200 */
    "PIXMA iP2200", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP170_slotlist, /* auto sheet feeder only */
    CANON_CAP_STD0,0,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP1900_modelist,
    &canon_PIXMA_iP2200_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP2500 */
    "PIXMA iP2500", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP170_slotlist, /* auto sheet feeder only */
    CANON_CAP_STD0,0,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP1900_modelist,
    &canon_PIXMA_iP2200_paperlist,/* OHP experimental */
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP2600 */
    "PIXMA iP2600", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP170_slotlist, /* auto sheet feeder only */
    CANON_CAP_STD0,0,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP1900_modelist,
    &canon_PIXMA_iP2600_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP2700 */
    "PIXMA iP2700", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP250_slotlist, /* rear only */
    CANON_CAP_STD0|CANON_CAP_P|CANON_CAP_I|CANON_CAP_px|CANON_CAP_T|CANON_CAP_XML,0,control_cmd_PIXMA_iP2700,  /*features */
    &canon_PIXMA_iP2700_modelist,
    &canon_PIXMA_iP2700_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP3000 */
    "PIXMA iP3000", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px,0x64,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP3000_modelist,
    &canon_PIXMA_iP4000_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP3100 */
    "PIXMA iP3100", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP3100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px,0x64,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP3100_modelist,
    &canon_PIXMA_iP3100_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP4000 */
    "PIXMA iP4000", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px /*|CANON_CAP_I*/,0x64,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP4000_modelist,
    &canon_PIXMA_iP4000_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* PIXMA MP740 (== iP4000 without duplex) */
    "PIXMA MP740", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_r|CANON_CAP_px /*,|CANON_CAP_I*/,0x64,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP4000_modelist,
    &canon_PIXMA_iP4000_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP5300, MP610 */
    "PIXMA iP5300", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_I,0x64,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP5300_modelist,
    /*&canon_PIXMA_iP4000_paperlist,*/
    &canon_MULTIPASS_MP600_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP4600 */
    "PIXMA iP4600", 3,          /*model, model_id*/
    8.5*72, 26.625*72, /* max paper width and height */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    16,
    &canon_PIXMA_iP4600_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_I,0x61,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_PIXMA_iP5300_modelist,
    &canon_PIXMA_iP4600_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP4500 CD-R tray F */
    "PIXMA iP4500", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4500_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I,0x64,control_cmd_MULTIPASS_MP150,  /* features */
    &canon_PIXMA_iP4500_modelist, /* changed from iP5300 */
    &canon_PIXMA_iP4500_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP4700 CD-R tray G */
    "PIXMA iP4700", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4600_slotlist, /* there is some ESC (r 0x65 command for CD tray also, not sure why */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_I|CANON_CAP_XML,0x64,control_cmd_MULTIPASS_MP150,  /* features */
    &canon_PIXMA_iP4700_modelist,
    &canon_PIXMA_iP4700_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP4200 */
    "PIXMA iP4200", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P,0x64,control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP4200_modelist,
    &canon_PIXMA_iP4000_paperlist,
    iP4200_lum_adjustment,
    iP4200_hue_adjustment,
    iP4200_sat_adjustment,
    NULL
  },
  { /* Canon PIXMA iP4800 CD-R tray G */
    "PIXMA iP4800", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4600_slotlist, /* there is some ESC (r 0x68 command for CD tray also, not sure why */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_I|CANON_CAP_XML,0x64,control_cmd_MULTIPASS_MP150,  /* features */
    &canon_PIXMA_iP4700_modelist,/* same for iP4800 */
    &canon_PIXMA_iP4700_paperlist,/* same for iP4800 */
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP6000 */
    "PIXMA iP6000", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px,0x64,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP6000_modelist,
    &canon_PIXMA_iP4000_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP6100 */
    "PIXMA iP6100", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px,0x64,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP6000_modelist,
    &canon_PIXMA_iP6100_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP6600 */
    "PIXMA iP6600", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_P|CANON_CAP_r|CANON_CAP_px,0x64,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP6700_modelist,
    &canon_PIXMA_iP6600_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP6700 */
    "PIXMA iP6700", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_P|CANON_CAP_r|CANON_CAP_px,0x64,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP6700_modelist,
    &canon_PIXMA_iP6700_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iX5000 */
    "PIXMA iX5000", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP170_slotlist,
    /* CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_I,0x61,control_cmd_PIXMA_iP4000, */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I,0,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iX5000_modelist,
    &canon_PIXMA_iP1500_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iX6500 --- with XML */
    "PIXMA iX6500", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP250_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML,0,control_cmd_PIXMA_iP4000,  /*features */
    &canon_MULTIPASS_MP540_modelist,
    &canon_MULTIPASS_MX880_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iX7000 --- with XML */
    "PIXMA iX7000", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iX7000_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_r|CANON_CAP_P|CANON_CAP_XML,0x64,control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iX7000_modelist,
    &canon_PIXMA_iX7000_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA MP510 --- like MP520 but without PPGgold paper support */
    "PIXMA MP510", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP520_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I,0,control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP520_modelist,
    &canon_MULTIPASS_MP520_paperlist, /* Windows driver lacks PPGgold, but let us try anyway */
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA MP520 */
    "PIXMA MP520", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP520_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I,0,control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP520_modelist,
    &canon_MULTIPASS_MP520_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA MP540 */
    "PIXMA MP540", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_MG5100_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_r,0x64,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MP540_modelist,
    &canon_MULTIPASS_MX330_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA MP550 ---wth XML */
    "PIXMA MP550", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_MG5100_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_r|CANON_CAP_XML,0x64,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MP540_modelist, /* seems to have same modes */
    &canon_MULTIPASS_MP250_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA MP600 */
    "PIXMA MP600", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MX850_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_I,0x64,control_cmd_PIXMA_iP4000,  /*features */
    &canon_MULTIPASS_MP520_modelist,
    &canon_MULTIPASS_MP600_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA MP630 */
    "PIXMA MP630", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_MG5200_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_r,0x64,control_cmd_PIXMA_iP4200,  /*features */
    &canon_MULTIPASS_MP640_modelist,
    &canon_MULTIPASS_MP630_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA MP640 */
    "PIXMA MP640", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_MG5200_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_r,0x64,control_cmd_PIXMA_iP4200,  /*features */
    &canon_MULTIPASS_MP640_modelist,
    &canon_MULTIPASS_MP640_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA Pro9000 */
    "PIXMA Pro9000", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_Pro9000_slotlist,
    CANON_CAP_STD0|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P,0x64,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_Pro9000_modelist,
    &canon_PIXMA_Pro9000_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA Pro9000 Mk.II */
    "PIXMA Pro9002", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_Pro9000_slotlist,
    CANON_CAP_STD0|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P,0x64,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_Pro9000mk2_modelist,
    &canon_PIXMA_Pro9000mk2_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA Pro9500 */
    "PIXMA Pro9500", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_Pro9000_slotlist,
    CANON_CAP_STD0|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P,0x64,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_Pro9500_modelist,
    &canon_PIXMA_Pro9500_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA Pro9500 Mk.II */
    "PIXMA Pro9502", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_Pro9000_slotlist,
    CANON_CAP_STD0|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P,0x64,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_Pro9500mk2_modelist,
    &canon_PIXMA_Pro9500mk2_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP8500 */
    "PIXMA iP8500", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_iP4000_slotlist,
    /*CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px,0x61,control_cmd_PIXMA_iP4000,*/  /*features */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P,0x64,control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP8500_modelist,
    &canon_PIXMA_iP4000_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon MULTIPASS MP150 */
    "PIXMA MP150", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    /* &canon_PIXMA_iP4000_slotlist, */
    &canon_MULTIPASS_MP170_slotlist, /* these models only have an auto sheet feeder also uses CAP_T */
    /* CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_T|CANON_CAP_I,0x61,control_cmd_MULTIPASS_MP150, */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I,0,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MP150_modelist,
    /* &canon_PIXMA_iP4000_paperlist, */
    &canon_MULTIPASS_MP150_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon MULTIPASS MP170 */
    "PIXMA MP170", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP170_slotlist, /* these models only have an auto sheet feeder also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I,0,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MP170_modelist,
    &canon_MULTIPASS_MP170_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon MULTIPASS MP450 */
    "PIXMA MP450", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP170_slotlist, /* these models only have an auto sheet feeder also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I,0,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MP170_modelist,
    &canon_MULTIPASS_MP170_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon MULTIPASS MP480 --- no XML */
    "PIXMA MP480", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP250_slotlist, /* only rear tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I,0,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MP480_modelist,
    &canon_MULTIPASS_MP480_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon MULTIPASS MP493: new model after MP480(?) --- with XML */
    "PIXMA MP493", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP250_slotlist, /* only rear tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_XML,0,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MP480_modelist,
    &canon_MULTIPASS_MP493_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon MULTIPASS MP495 identical to MP280 it seems --- with XML */
    "PIXMA MP495", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP250_slotlist, /* only rear tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_XML,0,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MP280_modelist,
    &canon_MULTIPASS_MP280_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon MULTIPASS MP250 */
    "PIXMA MP250", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP250_slotlist, /* these models only have a rear tray. Also uses CAP_T  */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_XML,0x61,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MP250_modelist,
    &canon_MULTIPASS_MP250_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon MULTIPASS MP270 */
    "PIXMA MP270", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP250_slotlist, /* these models only have a rear tray. Also uses CAP_T  */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_XML,0,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MP250_modelist,
    &canon_MULTIPASS_MP250_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon MULTIPASS MP280 */
    "PIXMA MP280", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP250_slotlist, /* these models only have a rear tray.  */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_r|CANON_CAP_P|CANON_CAP_I|CANON_CAP_T|CANON_CAP_XML,0,control_cmd_MULTIPASS_MP150,  /*features  Also uses CAP_T. MP280 no duplex, no 0x61 */
    &canon_MULTIPASS_MP280_modelist,
    &canon_MULTIPASS_MP280_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA MG5100 */
    "PIXMA MG5100", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_MG5100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML,0,control_cmd_MULTIPASS_MP150,  /* features */
    &canon_PIXMA_MG5100_modelist,
    &canon_PIXMA_MG5100_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG5200 --- like MG5100, plus CD tray */
    "PIXMA MG5200", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_MG5200_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML,0x64,control_cmd_MULTIPASS_MP150,  /* features */
    &canon_PIXMA_MG5200_modelist,/* there is some ESC (r 0x68 command for CD tray also, not sure why */
    &canon_PIXMA_MG5200_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG6100 */
    "PIXMA MG6100", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_MG5200_slotlist, /* same as MG5200 */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML,0x64,control_cmd_MULTIPASS_MP150,  /* features */
    &canon_PIXMA_MG6100_modelist,/* there is some ESC (r 0x68 command for CD tray also, not sure why */
    &canon_PIXMA_MG6100_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG8100 */
    "PIXMA MG8100", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_MG5200_slotlist, /* same as MG5200 */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML,0x64,control_cmd_MULTIPASS_MP150,  /* features */
    &canon_PIXMA_MG8100_modelist,/* there is some ESC (r 0x68 command for CD tray also, not sure why */
    &canon_PIXMA_MG6100_paperlist, /* same as MG6100 */
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  /* MX series */
  { /* Canon MULTIPASS MX300 --- MX310 is the same */
    "PIXMA MX300", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP170_slotlist, /* these models only have an auto sheet feeder also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I,0,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MX300_modelist,
    &canon_MULTIPASS_MX300_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX330 */
    "PIXMA MX330", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP170_slotlist, /* these models only have an auto sheet feeder also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I,0,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MX330_modelist,
    &canon_MULTIPASS_MX330_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX340 */
    "PIXMA MX340", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP170_slotlist, /* these models only have an auto sheet feeder also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I,0,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MX340_modelist,
    &canon_MULTIPASS_MX340_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX360 -- with XML */
    "PIXMA MX360", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP250_slotlist, /* only rear tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_XML,0,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MX360_modelist,
    &canon_MULTIPASS_MX360_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX420 --- with XML */
    "PIXMA MX420", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP250_slotlist, /* only rear tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_XML,0,control_cmd_MULTIPASS_MP150,  /*features */
    &canon_MULTIPASS_MP480_modelist,
    &canon_MULTIPASS_MX420_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon MULTIPASS MX700 */
    "PIXMA MX700", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MP520_slotlist, /* front, rear, button, and front/plain-only */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I,0,control_cmd_PIXMA_iP4000, /*features*/
    &canon_MULTIPASS_MP520_modelist,
    &canon_MULTIPASS_MX300_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX850 */
    "PIXMA MX850", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MX850_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I,0x64,control_cmd_PIXMA_iP4000, /*features*/
    &canon_PIXMA_iP4500_modelist, /* same inksets as iP4500 */
    &canon_MULTIPASS_MX850_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX860 */
    "PIXMA MX860", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_MG5100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I,0x64,control_cmd_PIXMA_iP4000, /*features*/
    &canon_MULTIPASS_MX860_modelist,
    &canon_MULTIPASS_MX330_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX870 --- with XML */
    "PIXMA MX870", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_MG5100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_XML,0x64,control_cmd_MULTIPASS_MP150, /*features*/
    &canon_MULTIPASS_MX870_modelist,
    &canon_MULTIPASS_MX340_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX880 --- with XML */
    "PIXMA MX880", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_PIXMA_MG5100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_XML,0,control_cmd_MULTIPASS_MP150, /*features*/
    &canon_MULTIPASS_MP540_modelist,
    &canon_MULTIPASS_MX880_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX7600 */
    "PIXMA MX7600", 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    10, 10, 15, 15,    /*border_left, border_right, border_top, border_bottom */
    8,
    &canon_MULTIPASS_MX7600_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I,0x64,control_cmd_PIXMA_iP4000, /*features*/
    &canon_MULTIPASS_MX7600_modelist,
    &canon_MULTIPASS_MX7600_paperlist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
};

#endif

