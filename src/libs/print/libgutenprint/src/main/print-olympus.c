/*
 * "$Id: print-olympus.c,v 1.97 2010/12/05 21:38:15 rlk Exp $"
 *
 *   Print plug-in DyeSub driver (formerly Olympus driver) for the GIMP.
 *
 *   Copyright 2003 - 2006
 *   Michael Mraka (Michael.Mraka@linux.cz)
 *
 *   The plug-in is based on the code of the RAW plugin for the GIMP of
 *   Michael Sweet (mike@easysw.com) and Robert Krawitz (rlk@alum.mit.edu)
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
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#ifdef __GNUC__
#define inline __inline__
#endif

#define DYESUB_FEATURE_NONE		0x00000000
#define DYESUB_FEATURE_FULL_WIDTH	0x00000001
#define DYESUB_FEATURE_FULL_HEIGHT	0x00000002
#define DYESUB_FEATURE_BLOCK_ALIGN	0x00000004
#define DYESUB_FEATURE_BORDERLESS	0x00000008
#define DYESUB_FEATURE_WHITE_BORDER	0x00000010
#define DYESUB_FEATURE_PLANE_INTERLACE	0x00000020
#define DYESUB_FEATURE_PLANE_LEFTTORIGHT	0x00000040

#define DYESUB_PORTRAIT	0
#define DYESUB_LANDSCAPE	1

#ifndef MIN
#  define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#endif /* !MIN */
#ifndef MAX
#  define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#endif /* !MAX */
#define PX(pt,dpi)	((pt) * (dpi) / 72)
#define PT(px,dpi)	((px) * 72 / (dpi))
#define LIST(list_t, list_name, items_t, items_name) \
	static const list_t list_name = \
	{ \
	  items_name, sizeof(items_name) / sizeof(items_t) \
	}

#define MAX_INK_CHANNELS	3
#define MAX_BYTES_PER_CHANNEL	2
#define SIZE_THRESHOLD		6

typedef struct
{
  const char *output_type;
  int output_channels;
  const char *name;
  const char *channel_order;
} ink_t;

typedef struct {
  const ink_t *item;
  size_t n_items;
} ink_list_t;

typedef struct {
  const char* name;
  int w_dpi;
  int h_dpi;
} dyesub_resolution_t;

typedef struct {
  const dyesub_resolution_t *item;
  size_t n_items;
} dyesub_resolution_list_t;

typedef struct {
  const char* name;
  const char* text;
  int width_pt;
  int height_pt;
  int border_pt_left;
  int border_pt_right;
  int border_pt_top;
  int border_pt_bottom;
  int print_mode;
} dyesub_pagesize_t;

typedef struct {
  const dyesub_pagesize_t *item;
  size_t n_items;
} dyesub_pagesize_list_t;

typedef struct {
  const char* res_name;
  const char* pagesize_name;
  int width_px;
  int height_px;
} dyesub_printsize_t;

typedef struct {
  const dyesub_printsize_t *item;
  size_t n_items;
} dyesub_printsize_list_t;


typedef struct {
  const char *name;
  const char *text;
  const stp_raw_t seq;
} laminate_t;

typedef struct {
  const laminate_t *item;
  size_t n_items;
} laminate_list_t;

#define NPUTC_BUFSIZE (4096)

typedef struct
{
  int w_dpi, h_dpi;
  int w_size, h_size;
  char plane;
  int block_min_w, block_min_h;
  int block_max_w, block_max_h;
  const char* pagesize;
  const laminate_t* laminate;
  int print_mode;
  char nputc_buf[NPUTC_BUFSIZE];
} dyesub_privdata_t;

static dyesub_privdata_t privdata;

typedef struct {
  int out_channels;
  int ink_channels;
  const char *ink_order;
  int bytes_per_out_channel;
  int bytes_per_ink_channel;
  int plane_interlacing;
  char empty_byte;
  unsigned short **image_data;
  int outh_px, outw_px, outt_px, outb_px, outl_px, outr_px;
  int imgh_px, imgw_px;
  int prnh_px, prnw_px, prnt_px, prnb_px, prnl_px, prnr_px;
  int print_mode;	/* portrait or landscape */
  int image_rows;
  int plane_lefttoright;
} dyesub_print_vars_t;

typedef struct /* printer specific parameters */
{
  int model;		/* printer model number from printers.xml*/
  const ink_list_t *inks;
  const dyesub_resolution_list_t *resolution;
  const dyesub_pagesize_list_t *pages;
  const dyesub_printsize_list_t *printsize;
  int block_size;
  int features;		
  void (*printer_init_func)(stp_vars_t *);
  void (*printer_end_func)(stp_vars_t *);
  void (*plane_init_func)(stp_vars_t *);
  void (*plane_end_func)(stp_vars_t *);
  void (*block_init_func)(stp_vars_t *);
  void (*block_end_func)(stp_vars_t *);
  const char *adj_cyan;		/* default color adjustment */
  const char *adj_magenta;
  const char *adj_yellow;
  const laminate_list_t *laminate;
} dyesub_cap_t;


static const dyesub_cap_t* dyesub_get_model_capabilities(int model);
static const laminate_t* dyesub_get_laminate_pattern(stp_vars_t *v);
static void  dyesub_nputc(stp_vars_t *v, char byte, int count);


static const ink_t cmy_inks[] =
{
  { "CMY", 3, "CMY", "\1\2\3" },
};

LIST(ink_list_t, cmy_ink_list, ink_t, cmy_inks);

static const ink_t ymc_inks[] =
{
  { "CMY", 3, "CMY", "\3\2\1" },
};

LIST(ink_list_t, ymc_ink_list, ink_t, ymc_inks);

static const ink_t rgb_inks[] =
{
  { "RGB", 3, "RGB", "\1\2\3" },
};

LIST(ink_list_t, rgb_ink_list, ink_t, rgb_inks);

static const ink_t bgr_inks[] =
{
  { "RGB", 3, "RGB", "\3\2\1" },
};

LIST(ink_list_t, bgr_ink_list, ink_t, bgr_inks);


/* Olympus P-10 */
static const dyesub_resolution_t res_310dpi[] =
{
  { "310x310", 310, 310},
};

LIST(dyesub_resolution_list_t, res_310dpi_list, dyesub_resolution_t, res_310dpi);

static const dyesub_pagesize_t p10_page[] =
{
  { "w288h432", "4 x 6", 298, 430, 0, 0, 0, 0, DYESUB_PORTRAIT}, /* 4x6" */
  { "B7", "3.5 x 5", 266, 370, 0, 0, 0, 0, DYESUB_PORTRAIT},	 /* 3.5x5" */
  { "Custom", NULL, 298, 430, 28, 28, 48, 48, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, p10_page_list, dyesub_pagesize_t, p10_page);

static const dyesub_printsize_t p10_printsize[] =
{
  { "310x310", "w288h432", 1280, 1848},
  { "310x310", "B7",  1144,  1591},
  { "310x310", "Custom", 1280, 1848},
};

LIST(dyesub_printsize_list_t, p10_printsize_list, dyesub_printsize_t, p10_printsize);

static void p10_printer_init_func(stp_vars_t *v)
{
  stp_zfwrite("\033R\033M\033S\2\033N\1\033D\1\033Y", 1, 15, v);
  stp_write_raw(&(privdata.laminate->seq), v); /* laminate */
  stp_zfwrite("\033Z\0", 1, 3, v);
}

static void p10_printer_end_func(stp_vars_t *v)
{
  stp_zfwrite("\033P", 1, 2, v);
}

static void p10_block_init_func(stp_vars_t *v)
{
  stp_zprintf(v, "\033T%c", privdata.plane);
  stp_put16_le(privdata.block_min_w, v);
  stp_put16_le(privdata.block_min_h, v);
  stp_put16_le(privdata.block_max_w + 1, v);
  stp_put16_le(privdata.block_max_h + 1, v);
}

static const laminate_t p10_laminate[] =
{
  {"Coated",  N_("Coated"),  {1, "\x00"}},
  {"None",    N_("None"),    {1, "\x02"}},
};

LIST(laminate_list_t, p10_laminate_list, laminate_t, p10_laminate);


/* Olympus P-200 series */
static const dyesub_resolution_t res_320dpi[] =
{
  { "320x320", 320, 320},
};

LIST(dyesub_resolution_list_t, res_320dpi_list, dyesub_resolution_t, res_320dpi);

static const dyesub_pagesize_t p200_page[] =
{
  { "ISOB7", "80x125mm", -1, -1, 16, 17, 33, 33, DYESUB_PORTRAIT},
  { "Custom", NULL, -1, -1, 16, 17, 33, 33, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, p200_page_list, dyesub_pagesize_t, p200_page);

static const dyesub_printsize_t p200_printsize[] =
{
  { "320x320", "ISOB7", 960, 1280},
  { "320x320", "Custom", 960, 1280},
};

LIST(dyesub_printsize_list_t, p200_printsize_list, dyesub_printsize_t, p200_printsize);

static void p200_printer_init_func(stp_vars_t *v)
{
  stp_zfwrite("S000001\0S010001\1", 1, 16, v);
}

static void p200_plane_init_func(stp_vars_t *v)
{
  stp_zprintf(v, "P0%d9999", 3 - privdata.plane+1 );
  stp_put32_be(privdata.w_size * privdata.h_size, v);
}

static void p200_printer_end_func(stp_vars_t *v)
{
  stp_zprintf(v, "P000001\1");
}

static const char p200_adj_any[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"33\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.000000 0.039216 0.078431 0.117647 0.152941 0.192157 0.231373 0.266667\n"
  "0.301961 0.341176 0.376471 0.411765 0.447059 0.482353 0.513725 0.549020\n"
  "0.580392 0.615686 0.647059 0.678431 0.709804 0.741176 0.768627 0.796078\n"
  "0.827451 0.854902 0.878431 0.905882 0.929412 0.949020 0.972549 0.988235\n"
  "1.000000\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";


/* Olympus P-300 series */
static const dyesub_resolution_t p300_res[] =
{
  { "306x306", 306, 306},
  { "153x153", 153, 153},
};

LIST(dyesub_resolution_list_t, p300_res_list, dyesub_resolution_t, p300_res);

static const dyesub_pagesize_t p300_page[] =
{
  { "A6", NULL, -1, -1, 28, 28, 48, 48, DYESUB_PORTRAIT},
  { "Custom", NULL, -1, -1, 28, 28, 48, 48, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, p300_page_list, dyesub_pagesize_t, p300_page);

static const dyesub_printsize_t p300_printsize[] =
{
  { "306x306", "A6", 1024, 1376},
  { "153x153", "A6",  512,  688},
  { "306x306", "Custom", 1024, 1376},
  { "153x153", "Custom", 512, 688},
};

LIST(dyesub_printsize_list_t, p300_printsize_list, dyesub_printsize_t, p300_printsize);

static void p300_printer_init_func(stp_vars_t *v)
{
  stp_zfwrite("\033\033\033C\033N\1\033F\0\1\033MS\xff\xff\xff"
	      "\033Z", 1, 19, v);
  stp_put16_be(privdata.w_dpi, v);
  stp_put16_be(privdata.h_dpi, v);
}

static void p300_plane_end_func(stp_vars_t *v)
{
  const char *c = "CMY";
  stp_zprintf(v, "\033\033\033P%cS", c[privdata.plane-1]);
  stp_deprintf(STP_DBG_DYESUB, "dyesub: p300_plane_end_func: %c\n",
	c[privdata.plane-1]);
}

static void p300_block_init_func(stp_vars_t *v)
{
  const char *c = "CMY";
  stp_zprintf(v, "\033\033\033W%c", c[privdata.plane-1]);
  stp_put16_be(privdata.block_min_h, v);
  stp_put16_be(privdata.block_min_w, v);
  stp_put16_be(privdata.block_max_h, v);
  stp_put16_be(privdata.block_max_w, v);

  stp_deprintf(STP_DBG_DYESUB, "dyesub: p300_block_init_func: %d-%dx%d-%d\n",
	privdata.block_min_w, privdata.block_max_w,
	privdata.block_min_h, privdata.block_max_h);
}

static const char p300_adj_cyan[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.078431 0.211765 0.250980 0.282353 0.309804 0.333333 0.352941 0.368627\n"
  "0.388235 0.403922 0.427451 0.443137 0.458824 0.478431 0.498039 0.513725\n"
  "0.529412 0.545098 0.556863 0.576471 0.592157 0.611765 0.627451 0.647059\n"
  "0.666667 0.682353 0.701961 0.713725 0.725490 0.729412 0.733333 0.737255\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";

static const char p300_adj_magenta[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.047059 0.211765 0.250980 0.278431 0.305882 0.333333 0.349020 0.364706\n"
  "0.380392 0.396078 0.415686 0.435294 0.450980 0.466667 0.482353 0.498039\n"
  "0.513725 0.525490 0.541176 0.556863 0.572549 0.592157 0.611765 0.631373\n"
  "0.650980 0.670588 0.694118 0.705882 0.721569 0.741176 0.745098 0.756863\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";

static const char p300_adj_yellow[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.047059 0.117647 0.203922 0.250980 0.274510 0.301961 0.321569 0.337255\n"
  "0.352941 0.364706 0.380392 0.396078 0.407843 0.423529 0.439216 0.450980\n"
  "0.466667 0.482353 0.498039 0.513725 0.533333 0.552941 0.572549 0.596078\n"
  "0.615686 0.635294 0.650980 0.666667 0.682353 0.690196 0.701961 0.713725\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";


/* Olympus P-400 series */
static const dyesub_resolution_t res_314dpi[] =
{
  { "314x314", 314, 314},
};

LIST(dyesub_resolution_list_t, res_314dpi_list, dyesub_resolution_t, res_314dpi);

static const dyesub_pagesize_t p400_page[] =
{
  { "A4", NULL, -1, -1, 22, 22, 54, 54, DYESUB_PORTRAIT},
  { "c8x10", "A5 wide", -1, -1, 58, 59, 84, 85, DYESUB_PORTRAIT},
  { "C6", "2 Postcards (A4)", -1, -1, 9, 9, 9, 9, DYESUB_PORTRAIT},
  { "Custom", NULL, -1, -1, 22, 22, 54, 54, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, p400_page_list, dyesub_pagesize_t, p400_page);

static const dyesub_printsize_t p400_printsize[] =
{
  { "314x314", "A4", 2400, 3200},
  { "314x314", "c8x10", 2000, 2400},
  { "314x314", "C6", 1328, 1920},
  { "314x314", "Custom", 2400, 3200},
};

LIST(dyesub_printsize_list_t, p400_printsize_list, dyesub_printsize_t, p400_printsize);

static void p400_printer_init_func(stp_vars_t *v)
{
  int wide = (strcmp(privdata.pagesize, "c8x10") == 0
		  || strcmp(privdata.pagesize, "C6") == 0);

  stp_zprintf(v, "\033ZQ"); dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033FP"); dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033ZF");
  stp_putc((wide ? '\x40' : '\x00'), v); dyesub_nputc(v, '\0', 60);
  stp_zprintf(v, "\033ZS");
  if (wide)
    {
      stp_put16_be(privdata.h_size, v);
      stp_put16_be(privdata.w_size, v);
    }
  else
    {
      stp_put16_be(privdata.w_size, v);
      stp_put16_be(privdata.h_size, v);
    }
  dyesub_nputc(v, '\0', 57);
  stp_zprintf(v, "\033ZP"); dyesub_nputc(v, '\0', 61);
}

static void p400_plane_init_func(stp_vars_t *v)
{
  stp_zprintf(v, "\033ZC"); dyesub_nputc(v, '\0', 61);
}

static void p400_plane_end_func(stp_vars_t *v)
{
  stp_zprintf(v, "\033P"); dyesub_nputc(v, '\0', 62);
}

static void p400_block_init_func(stp_vars_t *v)
{
  int wide = (strcmp(privdata.pagesize, "c8x10") == 0
		  || strcmp(privdata.pagesize, "C6") == 0);

  stp_zprintf(v, "\033Z%c", '3' - privdata.plane + 1);
  if (wide)
    {
      stp_put16_be(privdata.h_size - privdata.block_max_h - 1, v);
      stp_put16_be(privdata.w_size - privdata.block_max_w - 1, v);
      stp_put16_be(privdata.block_max_h - privdata.block_min_h + 1, v);
      stp_put16_be(privdata.block_max_w - privdata.block_min_w + 1, v);
    }
  else
    {
      stp_put16_be(privdata.block_min_w, v);
      stp_put16_be(privdata.block_min_h, v);
      stp_put16_be(privdata.block_max_w - privdata.block_min_w + 1, v);
      stp_put16_be(privdata.block_max_h - privdata.block_min_h + 1, v);
    }
  dyesub_nputc(v, '\0', 53);
}

static const char p400_adj_cyan[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.003922 0.031373 0.058824 0.090196 0.125490 0.156863 0.184314 0.219608\n"
  "0.250980 0.278431 0.309804 0.341176 0.376471 0.403922 0.439216 0.470588\n"
  "0.498039 0.517647 0.533333 0.545098 0.564706 0.576471 0.596078 0.615686\n"
  "0.627451 0.647059 0.658824 0.678431 0.690196 0.705882 0.721569 0.737255\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";
  
static const char p400_adj_magenta[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.003922 0.031373 0.062745 0.098039 0.125490 0.156863 0.188235 0.215686\n"
  "0.250980 0.282353 0.309804 0.345098 0.376471 0.407843 0.439216 0.470588\n"
  "0.501961 0.521569 0.549020 0.572549 0.592157 0.619608 0.643137 0.662745\n"
  "0.682353 0.713725 0.737255 0.756863 0.784314 0.807843 0.827451 0.850980\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";
  
static const char p400_adj_yellow[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.003922 0.027451 0.054902 0.090196 0.121569 0.156863 0.184314 0.215686\n"
  "0.250980 0.282353 0.309804 0.345098 0.372549 0.400000 0.435294 0.466667\n"
  "0.498039 0.525490 0.552941 0.580392 0.607843 0.631373 0.658824 0.678431\n"
  "0.698039 0.725490 0.760784 0.784314 0.811765 0.839216 0.866667 0.890196\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";


/* Olympus P-440 series */
static const dyesub_pagesize_t p440_page[] =
{
  { "A4", NULL, -1, -1, 10, 9, 54, 54, DYESUB_PORTRAIT},
  { "c8x10", "A5 wide", -1, -1, 58, 59, 72, 72, DYESUB_PORTRAIT},
  { "C6", "2 Postcards (A4)", -1, -1, 9, 9, 9, 9, DYESUB_PORTRAIT},
  { "w255h581", "A6 wide", -1, -1, 25, 25, 25, 24, DYESUB_PORTRAIT},
  { "Custom", NULL, -1, -1, 22, 22, 54, 54, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, p440_page_list, dyesub_pagesize_t, p440_page);

static const dyesub_printsize_t p440_printsize[] =
{
  { "314x314", "A4", 2508, 3200},
  { "314x314", "c8x10", 2000, 2508},
  { "314x314", "C6", 1328, 1920},
  { "314x314", "w255h581", 892, 2320},
  { "314x314", "Custom", 2508, 3200},
};

LIST(dyesub_printsize_list_t, p440_printsize_list, dyesub_printsize_t, p440_printsize);

static void p440_printer_init_func(stp_vars_t *v)
{
  int wide = ! (strcmp(privdata.pagesize, "A4") == 0
		  || strcmp(privdata.pagesize, "Custom") == 0);

  stp_zprintf(v, "\033FP"); dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033Y");
  stp_write_raw(&(privdata.laminate->seq), v); /* laminate */ 
  dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033FC"); dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033ZF");
  stp_putc((wide ? '\x40' : '\x00'), v); dyesub_nputc(v, '\0', 60);
  stp_zprintf(v, "\033N\1"); dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033ZS");
  if (wide)
    {
      stp_put16_be(privdata.h_size, v);
      stp_put16_be(privdata.w_size, v);
    }
  else
    {
      stp_put16_be(privdata.w_size, v);
      stp_put16_be(privdata.h_size, v);
    }
  dyesub_nputc(v, '\0', 57);
  if (strcmp(privdata.pagesize, "C6") == 0)
    {
      stp_zprintf(v, "\033ZC"); dyesub_nputc(v, '\0', 61);
    }
}

static void p440_printer_end_func(stp_vars_t *v)
{
  stp_zprintf(v, "\033P"); dyesub_nputc(v, '\0', 62);
}

static void p440_block_init_func(stp_vars_t *v)
{
  int wide = ! (strcmp(privdata.pagesize, "A4") == 0
		  || strcmp(privdata.pagesize, "Custom") == 0);

  stp_zprintf(v, "\033ZT");
  if (wide)
    {
      stp_put16_be(privdata.h_size - privdata.block_max_h - 1, v);
      stp_put16_be(privdata.w_size - privdata.block_max_w - 1, v);
      stp_put16_be(privdata.block_max_h - privdata.block_min_h + 1, v);
      stp_put16_be(privdata.block_max_w - privdata.block_min_w + 1, v);
    }
  else
    {
      stp_put16_be(privdata.block_min_w, v);
      stp_put16_be(privdata.block_min_h, v);
      stp_put16_be(privdata.block_max_w - privdata.block_min_w + 1, v);
      stp_put16_be(privdata.block_max_h - privdata.block_min_h + 1, v);
    }
  dyesub_nputc(v, '\0', 53);
}

static void p440_block_end_func(stp_vars_t *v)
{
  int pad = (64 - (((privdata.block_max_w - privdata.block_min_w + 1)
	  * (privdata.block_max_h - privdata.block_min_h + 1) * 3) % 64)) % 64;
  stp_deprintf(STP_DBG_DYESUB,
		  "dyesub: max_x %d min_x %d max_y %d min_y %d\n",
  		  privdata.block_max_w, privdata.block_min_w,
	  	  privdata.block_max_h, privdata.block_min_h);
  stp_deprintf(STP_DBG_DYESUB, "dyesub: olympus-p440 padding=%d\n", pad);
  dyesub_nputc(v, '\0', pad);
}


/* Olympus P-S100 */
static const dyesub_pagesize_t ps100_page[] =
{
  { "w288h432", "4 x 6", 296, 426, 0, 0, 0, 0, DYESUB_PORTRAIT},/* 4x6" */
  { "B7", "3.5 x 5", 264, 366, 0, 0, 0, 0, DYESUB_PORTRAIT},	/* 3.5x5" */
  { "Custom", NULL, 296, 426, 0, 0, 0, 0, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, ps100_page_list, dyesub_pagesize_t, ps100_page);

static const dyesub_printsize_t ps100_printsize[] =
{
  { "306x306", "w288h432", 1254, 1808},
  { "306x306", "B7", 1120, 1554},
  { "306x306", "Custom", 1254, 1808},
};

LIST(dyesub_printsize_list_t, ps100_printsize_list, dyesub_printsize_t, ps100_printsize);

static void ps100_printer_init_func(stp_vars_t *v)
{
  stp_zprintf(v, "\033U"); dyesub_nputc(v, '\0', 62);
  
  /* stp_zprintf(v, "\033ZC"); dyesub_nputc(v, '\0', 61); */
  
  stp_zprintf(v, "\033W"); dyesub_nputc(v, '\0', 62);
  
  stp_zfwrite("\x30\x2e\x00\xa2\x00\xa0\x00\xa0", 1, 8, v);
  stp_put16_be(privdata.h_size, v);	/* paper height (px) */
  stp_put16_be(privdata.w_size, v);	/* paper width (px) */
  dyesub_nputc(v, '\0', 3);
  stp_putc('\1', v);	/* number of copies */
  dyesub_nputc(v, '\0', 8);
  stp_putc('\1', v);
  dyesub_nputc(v, '\0', 15);
  stp_putc('\6', v);
  dyesub_nputc(v, '\0', 23);

  stp_zfwrite("\033ZT\0", 1, 4, v);
  stp_put16_be(0, v);			/* image width offset (px) */
  stp_put16_be(0, v);			/* image height offset (px) */
  stp_put16_be(privdata.w_size, v);	/* image width (px) */
  stp_put16_be(privdata.h_size, v);	/* image height (px) */
  dyesub_nputc(v, '\0', 52);
}

static void ps100_printer_end_func(stp_vars_t *v)
{
  int pad = (64 - (((privdata.block_max_w - privdata.block_min_w + 1)
	  * (privdata.block_max_h - privdata.block_min_h + 1) * 3) % 64)) % 64;
  stp_deprintf(STP_DBG_DYESUB,
		  "dyesub: max_x %d min_x %d max_y %d min_y %d\n",
  		  privdata.block_max_w, privdata.block_min_w,
	  	  privdata.block_max_h, privdata.block_min_h);
  stp_deprintf(STP_DBG_DYESUB, "dyesub: olympus-ps100 padding=%d\n", pad);
  dyesub_nputc(v, '\0', pad);		/* padding to 64B blocks */

  stp_zprintf(v, "\033PY"); dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033u"); dyesub_nputc(v, '\0', 62);
}


/* Canon CP-10 */
static const dyesub_resolution_t res_300dpi[] =
{
  { "300x300", 300, 300},
};

LIST(dyesub_resolution_list_t, res_300dpi_list, dyesub_resolution_t, res_300dpi);

static const dyesub_pagesize_t cp10_page[] =
{
  { "w155h244", "Card 54x86mm", 159, 250, 6, 6, 29, 29, DYESUB_PORTRAIT},
  { "Custom", NULL, -1, -1, 6, 6, 29, 29, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, cp10_page_list, dyesub_pagesize_t, cp10_page);

static const dyesub_printsize_t cp10_printsize[] =
{
  { "300x300", "w155h244", 662, 1040},
  { "300x300", "Custom", 662, 1040},
};

LIST(dyesub_printsize_list_t, cp10_printsize_list, dyesub_printsize_t, cp10_printsize);


/* Canon CP-100 series */
static const dyesub_pagesize_t cpx00_page[] =
{
  { "Postcard", "Postcard 100x148mm", 296, 434, 13, 13, 16, 19, DYESUB_PORTRAIT},
  { "w253h337", "CP_L 89x119mm", 264, 350, 13, 13, 15, 15, DYESUB_PORTRAIT},
  { "w155h244", "Card 54x86mm", 162, 250, 13, 13, 15, 15, DYESUB_LANDSCAPE},
  { "Custom", NULL, 296, 434, 13, 13, 16, 19, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, cpx00_page_list, dyesub_pagesize_t, cpx00_page);

static const dyesub_printsize_t cpx00_printsize[] =
{
  { "300x300", "Postcard", 1232, 1808},
  { "300x300", "w253h337", 1100, 1456},
  { "300x300", "w155h244", 672, 1040},
  { "300x300", "Custom", 1232, 1808},
};

LIST(dyesub_printsize_list_t, cpx00_printsize_list, dyesub_printsize_t, cpx00_printsize);

static void cpx00_printer_init_func(stp_vars_t *v)
{
  char pg = (strcmp(privdata.pagesize, "Postcard") == 0 ? '\1' :
		(strcmp(privdata.pagesize, "w253h337") == 0 ? '\2' :
		(strcmp(privdata.pagesize, "w155h244") == 0 ? 
			(strcmp(stp_get_driver(v),"canon-cp10") == 0 ?
				'\0' : '\3' ) :
		(strcmp(privdata.pagesize, "w283h566") == 0 ? '\4' : 
		 '\1' ))));

  stp_put16_be(0x4000, v);
  stp_putc('\0', v);
  stp_putc(pg, v);
  dyesub_nputc(v, '\0', 8);
}

static void cpx00_plane_init_func(stp_vars_t *v)
{
  stp_put16_be(0x4001, v);
  stp_put16_le(3 - privdata.plane, v);
  stp_put32_le(privdata.w_size * privdata.h_size, v);
  dyesub_nputc(v, '\0', 4);
}

static const char cpx00_adj_cyan[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.000000 0.035294 0.070588 0.101961 0.117647 0.168627 0.180392 0.227451\n"
  "0.258824 0.286275 0.317647 0.341176 0.376471 0.411765 0.427451 0.478431\n"
  "0.505882 0.541176 0.576471 0.611765 0.654902 0.678431 0.705882 0.737255\n"
  "0.764706 0.792157 0.811765 0.839216 0.862745 0.894118 0.909804 0.925490\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";
  
static const char cpx00_adj_magenta[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.011765 0.019608 0.035294 0.047059 0.054902 0.101961 0.133333 0.156863\n"
  "0.192157 0.235294 0.274510 0.321569 0.360784 0.403922 0.443137 0.482353\n"
  "0.521569 0.549020 0.584314 0.619608 0.658824 0.705882 0.749020 0.792157\n"
  "0.831373 0.890196 0.933333 0.964706 0.988235 0.992157 0.992157 0.996078\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";
  
static const char cpx00_adj_yellow[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.003922 0.015686 0.015686 0.023529 0.027451 0.054902 0.094118 0.129412\n"
  "0.180392 0.219608 0.250980 0.286275 0.317647 0.341176 0.388235 0.427451\n"
  "0.470588 0.509804 0.552941 0.596078 0.627451 0.682353 0.768627 0.796078\n"
  "0.890196 0.921569 0.949020 0.968627 0.984314 0.992157 0.992157 1.000000\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";


/* Canon CP-220 series */
static const dyesub_pagesize_t cp220_page[] =
{
  { "Postcard", "Postcard 100x148mm", 296, 434, 13, 13, 16, 19, DYESUB_PORTRAIT},
  { "w253h337", "CP_L 89x119mm", 264, 350, 13, 13, 15, 15, DYESUB_PORTRAIT},
  { "w155h244", "Card 54x86mm", 162, 250, 13, 13, 15, 15, DYESUB_LANDSCAPE},
  { "w283h566", "Wide 100x200mm", 296, 580, 13, 13, 20, 20, DYESUB_PORTRAIT},
  { "Custom", NULL, 296, 434, 13, 13, 16, 19, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, cp220_page_list, dyesub_pagesize_t, cp220_page);

static const dyesub_printsize_t cp220_printsize[] =
{
  { "300x300", "Postcard", 1232, 1808},
  { "300x300", "w253h337", 1100, 1456},
  { "300x300", "w155h244", 672, 1040},
  { "300x300", "w283h566", 1232, 2416},
  { "300x300", "Custom", 1232, 1808},
};

LIST(dyesub_printsize_list_t, cp220_printsize_list, dyesub_printsize_t, cp220_printsize);


/* Canon SELPHY CP-520 */
static void cp520_printer_init_func(stp_vars_t *v)
{
  char pg = (strcmp(privdata.pagesize, "Postcard") == 0 ? '\1' :
		(strcmp(privdata.pagesize, "w253h337") == 0 ? '\2' :
		(strcmp(privdata.pagesize, "w155h244") == 0 ? '\3' :
		(strcmp(privdata.pagesize, "w283h566") == 0 ? '\4' : 
		 '\1' ))));

  stp_put16_be(0x4000, v);
  stp_putc('\0', v);
  stp_putc(pg, v);
  dyesub_nputc(v, '\0', 8); 
  /* The CP520 does not want the printer_init and plane_init command to be sent
     in the same USB-packet so we fill up first USB-Packet  with '\0'. */
  dyesub_nputc(v, '\0', 1012); 
}

static void cp520_plane_init_func(stp_vars_t *v)
{
  stp_put16_be(0x4001, v);
  stp_putc(3 - privdata.plane, v);  /* The CP520 differs from the cp-printer
                                       in that it reqires the plane in the 3rd
                                       byte not in the 4th */
  stp_putc('\0', v);
  stp_put32_le(privdata.w_size * privdata.h_size, v);
  dyesub_nputc(v, '\0', 4);
}


/* Canon SELPHY ES series */
static void es1_printer_init_func(stp_vars_t *v)
{
  char pg = (strcmp(privdata.pagesize, "Postcard") == 0 ? 0x11 :
	     (strcmp(privdata.pagesize, "w253h337") == 0 ? '\2' :
	      (strcmp(privdata.pagesize, "w155h244") == 0 ? '\3' : 0x11 )));

  /*
   differs from cp-220 in that after the 0x4000, the next character
   seems to be 0x01 instead of 0x00, and the P card size code is 
   '0x11' instead of '0x1'
   codes for other paper types are unknown.
  */

  stp_put16_be(0x4000, v);
  stp_putc(0x10, v);
  stp_putc(pg, v);
  dyesub_nputc(v, '\0', 8);
}

static void es1_plane_init_func(stp_vars_t *v)
{
  unsigned char plane = 0;

  switch (privdata.plane) {
  case 3: /* Y */
    plane = 0x01;
    break;
  case 2: /* M */
    plane = 0x03;
    break;
  case 1: /* C */
    plane = 0x07;
    break;
  }

  stp_put16_be(0x4001, v);
  stp_putc(0x1, v);
  stp_putc(plane, v);
  stp_put32_le(privdata.w_size * privdata.h_size, v);
  dyesub_nputc(v, '\0', 4);
}

/* Sony DPP-EX5, DPP-EX7 */
static const dyesub_resolution_t res_403dpi[] =
{
  { "403x403", 403, 403},
};

LIST(dyesub_resolution_list_t, res_403dpi_list, dyesub_resolution_t, res_403dpi);

/* only Postcard pagesize is supported */
static const dyesub_pagesize_t dppex5_page[] =
{
  { "w288h432", "Postcard", PT(1664,403)+1, PT(2466,403)+1, 13, 14, 18, 17,
  							DYESUB_PORTRAIT},
  { "Custom", NULL, PT(1664,403)+1, PT(2466,403)+1, 13, 14, 18, 17,
  							DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, dppex5_page_list, dyesub_pagesize_t, dppex5_page);

static const dyesub_printsize_t dppex5_printsize[] =
{
  { "403x403", "w288h432", 1664, 2466},
  { "403x403", "Custom", 1664, 2466},
};

LIST(dyesub_printsize_list_t, dppex5_printsize_list, dyesub_printsize_t, dppex5_printsize);

static void dppex5_printer_init(stp_vars_t *v)
{
  stp_zfwrite("DPEX\0\0\0\x80", 1, 8, v);
  stp_zfwrite("DPEX\0\0\0\x82", 1, 8, v);
  stp_zfwrite("DPEX\0\0\0\x84", 1, 8, v);
  stp_put32_be(privdata.w_size, v);
  stp_put32_be(privdata.h_size, v);
  stp_zfwrite("S\0o\0n\0y\0 \0D\0P\0P\0-\0E\0X\0\x35\0", 1, 24, v);
  dyesub_nputc(v, '\0', 40);
  stp_zfwrite("\1\4\0\4\xdc\0\x24\0\3\3\1\0\1\0\x82\0", 1, 16, v);
  stp_zfwrite("\xf4\5\xf8\3\x64\0\1\0\x0e\0\x93\1\2\0\1\0", 1, 16, v);
  stp_zfwrite("\x93\1\1\0\0\0", 1, 6, v);
  stp_zfwrite("P\0o\0s\0t\0 \0c\0a\0r\0d\0", 1, 18, v);
  dyesub_nputc(v, '\0', 46);
  stp_zfwrite("\x93\1\x18", 1, 3, v);
  dyesub_nputc(v, '\0', 19);
  stp_zfwrite("\2\0\0\0\3\0\0\0\1\0\0\0\1", 1, 13, v);
  dyesub_nputc(v, '\0', 19);
  stp_zprintf(v, "5EPD");
  dyesub_nputc(v, '\0', 4);
  stp_zfwrite((privdata.laminate->seq).data, 1,
			(privdata.laminate->seq).bytes, v); /*laminate pattern*/
  stp_zfwrite("\0d\0d\0d", 1, 6, v);
  dyesub_nputc(v, '\0', 21);
}

static void dppex5_block_init(stp_vars_t *v)
{
  stp_zfwrite("DPEX\0\0\0\x85", 1, 8, v);
  stp_put32_be((privdata.block_max_w - privdata.block_min_w + 1)
  		* (privdata.block_max_h - privdata.block_min_h + 1) * 3, v);
}

static void dppex5_printer_end(stp_vars_t *v)
{
  stp_zfwrite("DPEX\0\0\0\x83", 1, 8, v);
  stp_zfwrite("DPEX\0\0\0\x81", 1, 8, v);
}

static const laminate_t dppex5_laminate[] =
{
  {"Glossy",  N_("Glossy"),  {1, "\x00"}},
  {"Texture", N_("Texture"), {1, "\x01"}},
};

LIST(laminate_list_t, dppex5_laminate_list, laminate_t, dppex5_laminate);


/* Sony UP-DP10 */
static const dyesub_pagesize_t updp10_page[] =
{
  { "w288h432", "UPC-10P23 (4x6)", -1, -1, 12, 12, 18, 18, DYESUB_LANDSCAPE},
  { "w288h387", "UPC-10P34 (4x5)", -1, 384, 12, 12, 16, 16, DYESUB_LANDSCAPE},
#if 0
  /* We can't have two paper sizes that are the same size --rlk 20080813 */
  { "w288h432", "UPC-10S01 (Sticker)", -1, -1, 12, 12, 18, 18, DYESUB_LANDSCAPE},
#endif
  { "Custom", NULL, -1, -1, 12, 12, 0, 0, DYESUB_LANDSCAPE},
};

LIST(dyesub_pagesize_list_t, updp10_page_list, dyesub_pagesize_t, updp10_page);

static const dyesub_printsize_t updp10_printsize[] =
{
  { "300x300", "w288h432", 1200, 1800},
  { "300x300", "w288h387", 1200, 1600},
  { "300x300", "Custom", 1200, 1800},
};

LIST(dyesub_printsize_list_t, updp10_printsize_list, dyesub_printsize_t, updp10_printsize);

static void updp10_printer_init_func(stp_vars_t *v)
{
  stp_zfwrite("\x98\xff\xff\xff\xff\xff\xff\xff"
	      "\x09\x00\x00\x00\x1b\xee\x00\x00"
	      "\x00\x02\x00\x00\x01\x12\x00\x00"
	      "\x00\x1b\xe1\x00\x00\x00\x0b\x00"
	      "\x00\x04", 1, 34, v);
  stp_zfwrite((privdata.laminate->seq).data, 1,
			(privdata.laminate->seq).bytes, v); /*laminate pattern*/
  stp_zfwrite("\x00\x00\x00\x00", 1, 4, v);
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v);
  stp_zfwrite("\x14\x00\x00\x00\x1b\x15\x00\x00"
	      "\x00\x0d\x00\x00\x00\x00\x00\x07"
	      "\x00\x00\x00\x00", 1, 20, v);
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v);
  stp_put32_le(privdata.w_size*privdata.h_size*3+11, v);
  stp_zfwrite("\x1b\xea\x00\x00\x00\x00", 1, 6, v);
  stp_put32_be(privdata.w_size*privdata.h_size*3, v);
  stp_zfwrite("\x00", 1, 1, v);
}

static void updp10_printer_end_func(stp_vars_t *v)
{
	stp_zfwrite("\xff\xff\xff\xff\x07\x00\x00\x00"
		    "\x1b\x0a\x00\x00\x00\x00\x00\xfd"
		    "\xff\xff\xff\xff\xff\xff\xff"
		    , 1, 23, v);
}

static const laminate_t updp10_laminate[] =
{
  {"Glossy",  N_("Glossy"),  {1, "\x00"}},
  {"Texture", N_("Texture"), {1, "\x08"}},
  {"Matte",   N_("Matte"),   {1, "\x0c"}},
};

LIST(laminate_list_t, updp10_laminate_list, laminate_t, updp10_laminate);

static const char updp10_adj_cyan[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"33\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.113725 0.188235 0.247059 0.286275 0.317647 0.345098 0.368627 0.384314\n"
  "0.400000 0.407843 0.423529 0.439216 0.450980 0.466667 0.482353 0.498039\n"
  "0.509804 0.525490 0.545098 0.560784 0.580392 0.596078 0.619608 0.643137\n"
  "0.662745 0.686275 0.709804 0.729412 0.756863 0.780392 0.811765 0.843137\n"
  "1.000000\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";
  
static const char updp10_adj_magenta[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"33\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.105882 0.211765 0.286275 0.333333 0.364706 0.388235 0.403922 0.415686\n"
  "0.427451 0.439216 0.450980 0.462745 0.478431 0.494118 0.505882 0.521569\n"
  "0.537255 0.552941 0.568627 0.584314 0.600000 0.619608 0.643137 0.662745\n"
  "0.682353 0.709804 0.733333 0.760784 0.792157 0.823529 0.858824 0.890196\n"
  "1.000000\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";
  
static const char updp10_adj_yellow[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"33\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.101961 0.160784 0.196078 0.227451 0.243137 0.254902 0.266667 0.286275\n"
  "0.309804 0.337255 0.368627 0.396078 0.423529 0.443137 0.462745 0.478431\n"
  "0.501961 0.517647 0.537255 0.556863 0.576471 0.596078 0.619608 0.643137\n"
  "0.666667 0.690196 0.709804 0.737255 0.760784 0.780392 0.796078 0.803922\n"
  "1.000000\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";


/* Sony UP-DR100 */
static const dyesub_pagesize_t updr100_page[] =
{
  { "w288h432",	"4x6", 298, 442, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "B7",	"3.5x5", 261, 369, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "w360h504",	"5x7", 369, 514, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "w432h576",	"6x8", 442, 588, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "Custom", NULL, 298, 442, 0, 0, 0, 0, DYESUB_LANDSCAPE},
};

LIST(dyesub_pagesize_list_t, updr100_page_list, dyesub_pagesize_t, updr100_page);

static const dyesub_printsize_t updr100_printsize[] =
{
  { "334x334", "w288h432", 1382, 2048},
  { "334x334", "B7", 1210, 1710},
  { "334x334", "w360h504", 1710, 2380},
  { "334x334", "w432h576", 2048, 2724},
  { "334x334", "Custom", 1382, 2048},
};

LIST(dyesub_printsize_list_t, updr100_printsize_list, dyesub_printsize_t, updr100_printsize);

static void updr100_printer_init_func(stp_vars_t *v)
{
  int dim1 = (privdata.print_mode == DYESUB_LANDSCAPE ?
  		privdata.h_size : privdata.w_size);
  int dim2 = (privdata.print_mode == DYESUB_LANDSCAPE ?
  		privdata.w_size : privdata.h_size);

  stp_zfwrite("UPD8D\x00\x00\x00\x10\x03\x00\x00", 1, 12, v);
  stp_put32_le(dim1, v);
  stp_put32_le(dim2, v);
  stp_zfwrite("\x1e\x00\x03\x00\x01\x00\x4e\x01\x00\x00", 1, 10, v);
  stp_write_raw(&(privdata.laminate->seq), v); /* laminate pattern */
  dyesub_nputc(v, '\0', 13);
  stp_zfwrite("\x01\x00\x01\x00\x03", 1, 5, v);
  dyesub_nputc(v, '\0', 19);
}

static void updr100_printer_end_func(stp_vars_t *v)
{
  stp_zfwrite("UPD8D\x00\x00\x00\x02", 1, 9, v);
  dyesub_nputc(v, '\0', 25);
  stp_zfwrite("\x9d\x02\x00\x04\x00\x00\xc0\xe7"
  	      "\x9d\x02\x54\xe9\x9d\x02\x9d\x71"
	      "\x00\x73\xfa\x71\x00\x73\xf4\xea"
	      "\x9d\x02\xa8\x3e\x00\x73\x9c\xeb\x9d\x02"
	      , 1, 34, v);
}

static const laminate_t updr100_laminate[] =
{
  {"Glossy",  N_("Glossy"),  {1, "\x01"}},
  {"Texture", N_("Texture"), {1, "\x03"}},
  {"Matte",   N_("Matte"),   {1, "\x04"}},
};

LIST(laminate_list_t, updr100_laminate_list, laminate_t, updr100_laminate);


/* Sony UP-DR150 */
static const dyesub_resolution_t res_334dpi[] =
{
  { "334x334", 334, 334},
};

LIST(dyesub_resolution_list_t, res_334dpi_list, dyesub_resolution_t, res_334dpi);

static const dyesub_pagesize_t updr150_page[] =
{
  { "w288h432",	"2UPC-153 (4x6)", 298, 442, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "B7",	"2UPC-154 (3.5x5)", 261, 373, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "w360h504",	"2UPC-155 (5x7)", 373, 514, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "w432h576",	"2UPC-156 (6x8)", 442, 588, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "Custom", NULL, 298, 442, 0, 0, 0, 0, DYESUB_LANDSCAPE},
};

LIST(dyesub_pagesize_list_t, updr150_page_list, dyesub_pagesize_t, updr150_page);

static const dyesub_printsize_t updr150_printsize[] =
{
  { "334x334", "w288h432", 1382, 2048},
  { "334x334", "B7", 1210, 1728},
  { "334x334", "w360h504", 1728, 2380},
  { "334x334", "w432h576", 2048, 2724},
  { "334x334", "Custom", 1382, 2048},
};

LIST(dyesub_printsize_list_t, updr150_printsize_list, dyesub_printsize_t, updr150_printsize);

static void updr150_printer_init_func(stp_vars_t *v)
{
  char pg = '\0';
  int dim1 = (privdata.print_mode == DYESUB_LANDSCAPE ?
  		privdata.w_size : privdata.h_size);
  int dim2 = (privdata.print_mode == DYESUB_LANDSCAPE ?
  		privdata.h_size : privdata.w_size);

  stp_zfwrite("\x6a\xff\xff\xff\xef\xff\xff\xff", 1, 8, v);
  if (strcmp(privdata.pagesize,"B7") == 0)
    pg = '\x01';
  else if (strcmp(privdata.pagesize,"w288h432") == 0)
    pg = '\x02';
  else if (strcmp(privdata.pagesize,"w360h504") == 0)
    pg = '\x03';
  else if (strcmp(privdata.pagesize,"w432h576") == 0)
    pg = '\x04';
  stp_putc(pg, v);

  stp_zfwrite("\x00\x00\x00\xfc\xff\xff\xff"
	      "\xfb\xff\xff\xff\xf4\xff\xff\xff"
	      "\xf5\xff\xff\xff\x01\x00\x00\x00"
	      "\x07\x00\x00\x00\x1b\xe5\x00\x00"
	      "\x00\x08\x00\x08\x00\x00\x00\x00"
	      "\x00\x00\x00\x00\x00\x01\x00\xed"
	      "\xff\xff\xff\x07\x00\x00\x00\x1b"
	      "\xee\x00\x00\x00\x02\x00\x02\x00"
	      "\x00\x00\x00\x01\x07\x00\x00\x00"
	      "\x1b\x15\x00\x00\x00\x0d\x00\x0d"
	      "\x00\x00\x00\x00\x00\x00\x00\x07"
	      "\x00\x00\x00\x00", 1, 91, v);
  stp_put16_be(dim1, v);
  stp_put16_be(dim2, v);
  stp_zfwrite("\xf9\xff\xff\xff\x07\x00\x00\x00"
	      "\x1b\xe1\x00\x00\x00\x0b\x00\x0b"
	      "\x00\x00\x00\x00\x80\x00\x00\x00"
	      "\x00\x00", 1, 26, v);
  stp_put16_be(dim1, v);
  stp_put16_be(dim2, v);
  stp_zfwrite("\xf8\xff\xff\xff\x0b\x00\x00\x00\x1b\xea"
	      "\x00\x00\x00\x00", 1, 14, v);
  stp_put32_be(privdata.w_size*privdata.h_size*3, v);
  stp_zfwrite("\x00", 1, 1, v);
  stp_put32_le(privdata.w_size*privdata.h_size*3, v);
}

static void updr150_printer_end_func(stp_vars_t *v)
{
	stp_zfwrite("\xfc\xff\xff"
		    "\xff\xfa\xff\xff\xff\x07\x00\x00"
		    "\x00\x1b\x0a\x00\x00\x00\x00\x00"
		    "\x07\x00\x00\x00\x1b\x17\x00\x00"
		    "\x00\x00\x00\xf3\xff\xff\xff"
		    , 1, 34, v);
}


/* Fujifilm CX-400 */
static const dyesub_pagesize_t cx400_page[] =
{
  { "w288h432", NULL, 295, 428, 24, 24, 23, 22, DYESUB_PORTRAIT},
  { "w288h387", "4x5 3/8", 295, 386, 24, 24, 23, 23, DYESUB_PORTRAIT},
  { "w288h504", NULL, 295, 513, 24, 24, 23, 22, DYESUB_PORTRAIT},
  { "Custom", NULL, 295, 428, 0, 0, 0, 0, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, cx400_page_list, dyesub_pagesize_t, cx400_page);

static const dyesub_printsize_t cx400_printsize[] =
{
  { "310x310", "w288h387", 1268, 1658},
  { "310x310", "w288h432", 1268, 1842},
  { "310x310", "w288h504", 1268, 2208},
  { "310x310", "Custom", 1268, 1842},
};

LIST(dyesub_printsize_list_t, cx400_printsize_list, dyesub_printsize_t, cx400_printsize);

static void cx400_printer_init_func(stp_vars_t *v)
{
  char pg = '\0';
  const char *pname = "XXXXXX";		  				

  stp_deprintf(STP_DBG_DYESUB,
	"dyesub: fuji driver %s\n", stp_get_driver(v));
  if (strcmp(stp_get_driver(v),"fujifilm-cx400") == 0)
    pname = "NX1000";
  else if (strcmp(stp_get_driver(v),"fujifilm-cx550") == 0)
    pname = "QX200\0";

  stp_zfwrite("FUJIFILM", 1, 8, v);
  stp_zfwrite(pname, 1, 6, v);
  stp_putc('\0', v);
  stp_put16_le(privdata.w_size, v);
  stp_put16_le(privdata.h_size, v);
  if (strcmp(privdata.pagesize,"w288h504") == 0)
    pg = '\x0d';
  else if (strcmp(privdata.pagesize,"w288h432") == 0)
    pg = '\x0c';
  else if (strcmp(privdata.pagesize,"w288h387") == 0)
    pg = '\x0b';
  stp_putc(pg, v);
  stp_zfwrite("\x00\x00\x00\x00\x00\x01\x00\x01\x00\x00\x00\x00"
		  "\x00\x00\x2d\x00\x00\x00\x00", 1, 19, v);
  stp_zfwrite("FUJIFILM", 1, 8, v);
  stp_zfwrite(pname, 1, 6, v);
  stp_putc('\1', v);
}
  

/* Fujifilm NX-500 */
static const dyesub_resolution_t res_306dpi[] =
{
  { "306x306", 306, 306},
};

LIST(dyesub_resolution_list_t, res_306dpi_list, dyesub_resolution_t, res_306dpi);

static const dyesub_pagesize_t nx500_page[] =
{
  { "Postcard", NULL, -1, -1, 21, 21, 29, 29, DYESUB_PORTRAIT},
  { "Custom", NULL, -1, -1, 21, 21, 29, 29, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, nx500_page_list, dyesub_pagesize_t, nx500_page);

static const dyesub_printsize_t nx500_printsize[] =
{
  { "306x306", "Postcard", 1024, 1518},
  { "306x306", "Custom", 1024, 1518},
};

LIST(dyesub_printsize_list_t, nx500_printsize_list, dyesub_printsize_t, nx500_printsize);

static void nx500_printer_init_func(stp_vars_t *v)
{
  stp_zfwrite("INFO-QX-20--MKS\x00\x00\x00M\x00W\00A\x00R\00E", 1, 27, v);
  dyesub_nputc(v, '\0', 21);
  stp_zfwrite("\x80\x00\x02", 1, 3, v);
  dyesub_nputc(v, '\0', 20);
  stp_zfwrite("\x02\x01\x01", 1, 3, v);
  dyesub_nputc(v, '\0', 2);
  stp_put16_le(privdata.h_size, v);
  stp_put16_le(privdata.w_size, v);
  stp_zfwrite("\x00\x02\x00\x70\x2f", 1, 5, v);
  dyesub_nputc(v, '\0', 43);
}
  

/* Kodak Easyshare Dock family */
static const dyesub_pagesize_t kodak_dock_page[] =
{
  { "w288h432", NULL, PT(1248,300)+1, PT(1856,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT}, /* 4x6 */
  { "Custom", NULL, PT(1248,300)+1, PT(1856,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT}, /* 4x6 */
};

LIST(dyesub_pagesize_list_t, kodak_dock_page_list, dyesub_pagesize_t, kodak_dock_page);

static const dyesub_printsize_t kodak_dock_printsize[] =
{
  { "300x300", "w288h432", 1248, 1856},
  { "300x300", "Custom", 1248, 1856},
};

LIST(dyesub_printsize_list_t, kodak_dock_printsize_list, dyesub_printsize_t, kodak_dock_printsize);

static void kodak_dock_printer_init(stp_vars_t *v)
{
  stp_put16_be(0x3000, v);
  dyesub_nputc(v, '\0', 10);
}

static void kodak_dock_plane_init(stp_vars_t *v)
{
  stp_put16_be(0x3001, v);
  stp_put16_le(3 - privdata.plane, v);
  stp_put32_le(privdata.w_size*privdata.h_size, v);
  dyesub_nputc(v, '\0', 4);
}


/* Shinko CHC-S9045 (experimental) */
static const dyesub_pagesize_t shinko_chcs9045_page[] =
{
  { "w288h432",	"4x6", PT(1240,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
  { "B7",	"3.5x5", PT(1088,300)+1, PT(1548,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
  { "w360h504",	"5x7", PT(1548,300)+1, PT(2140,300)+1, 0, 0, 0, 0,
  							DYESUB_PORTRAIT},
  { "w432h576",	"6x9", PT(1844,300)+1, PT(2740,300)+1, 0, 0, 0, 0,
  							DYESUB_PORTRAIT},
  { "w283h425",	"Sticker paper", PT(1092,300)+1, PT(1726,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
  { "Custom",   NULL, PT(1240,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
};

LIST(dyesub_pagesize_list_t, shinko_chcs9045_page_list, dyesub_pagesize_t, shinko_chcs9045_page);

static const dyesub_printsize_t shinko_chcs9045_printsize[] =
{
  { "300x300", "w288h432", 1240, 1844},
  { "300x300", "B7", 1088, 1548},
  { "300x300", "w360h504", 1548, 2140},
  { "300x300", "w432h576", 1844, 2740},
  { "300x300", "w283h425", 1092, 1726},
  { "300x300", "Custom", 1240, 1844},
};

LIST(dyesub_printsize_list_t, shinko_chcs9045_printsize_list, dyesub_printsize_t, shinko_chcs9045_printsize);

static void shinko_chcs9045_printer_init(stp_vars_t *v)
{
  char pg = '\0';
  char sticker = '\0';

  stp_zprintf(v, "\033CHC\n");
  stp_put16_be(1, v);
  stp_put16_be(1, v);
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v);
  if (strcmp(privdata.pagesize,"B7") == 0)
    pg = '\1';
  else if (strcmp(privdata.pagesize,"w360h504") == 0)
    pg = '\3';
  else if (strcmp(privdata.pagesize,"w432h576") == 0)
    pg = '\5';
  else if (strcmp(privdata.pagesize,"w283h425") == 0)
    sticker = '\3';
  stp_putc(pg, v);
  stp_putc('\0', v);
  stp_putc(sticker, v);
  dyesub_nputc(v, '\0', 4338);
}


/* Dai Nippon Printing DS40 */
static const dyesub_pagesize_t dnpds40_dock_page[] =
{
  { "w288h432", "4x6", PT(1920,300)+1, PT(1240,300)+1, 0, 0, 0, 0,
						DYESUB_PORTRAIT},
  { "w432h576", "6x9", PT(1920,300)+1, PT(2740,300)+1, 0, 0, 0, 0,
							DYESUB_PORTRAIT},
  { "A5", "6x8", PT(1920,300)+1, PT(2436,300)+1, 0, 0, 0, 0,
						DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, dnpds40_dock_page_list, dyesub_pagesize_t, dnpds40_dock_page);

static const dyesub_printsize_t dnpds40_dock_printsize[] =
{
  { "300x300", "w288h432", 1920, 1240},
  { "300x300", "w432h576", 1920, 2740},
  { "300x300", "A5", 1920, 2436},
};

LIST(dyesub_printsize_list_t, dnpds40_dock_printsize_list, dyesub_printsize_t, dnpds40_dock_printsize);

static void dnpds40_printer_end(stp_vars_t *v)
{
  stp_zprintf(v, "\033PCNTRL START"); dyesub_nputc(v, ' ', 19);
}

static void dnpds40_plane_init(stp_vars_t *v)
{
  char p = (privdata.plane == 3 ? 'Y' :
	    (privdata.plane == 2 ? 'M' :
	     'C' ));

  long RFSize = (privdata.w_size*privdata.h_size) + 1024 + 54;
  long AdSize = (32 - (RFSize % 32));
  long FSize = RFSize + AdSize;

  stp_zprintf(v, "\033PCNTRL RETENTION       0000000800000000");
  stp_zprintf(v, "\033PIMAGE %cPLANE", p); dyesub_nputc(v, ' ', 10);

  stp_zprintf(v, "0%ld", FSize);
  stp_zprintf(v, "BM");
  stp_put32_le(FSize, v);
  dyesub_nputc(v, '\0', 4);
  stp_put32_le(1088, v);
  stp_put32_le(40, v);
  stp_put32_le(privdata.w_size, v);
  stp_put32_le(privdata.h_size, v);
  stp_put16_le(1, v);
  stp_put16_le(8, v);
  dyesub_nputc(v, '\0', 24);
  dyesub_nputc(v, '\0', 1024);   /*RGB Array*/
  dyesub_nputc(v, '\0', AdSize); /*Locate to 32bit border */
}



/* Dai Nippon Printing DS80 */
static const dyesub_pagesize_t dnpds80_dock_page[] =
{
  { "c8x10", "8x10", PT(2560,300)+1, PT(3036,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "C6", "8x4", PT(2560,300)+1, PT(1236,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},/* 8x4 */
  { "C5", "8x5", PT(2560,300)+1, PT(1536,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "C4", "8x6", PT(2560,300)+1, PT(1836,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "C3", "8x8", PT(2560,300)+1, PT(2436,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "C2", "8x12", PT(2560,300)+1, PT(3636,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "C1", "A4 Length", PT(2560,300)+1, PT(3544,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, dnpds80_dock_page_list, dyesub_pagesize_t, dnpds80_dock_page);

static const dyesub_printsize_t dnpds80_dock_printsize[] =
{
  { "300x300", "c8x10", 2560, 3036},
  { "300x300", "C6", 2560, 1236},
  { "300x300", "C5", 2560, 1536},
  { "300x300", "C4", 2560, 1836},
  { "300x300", "C3", 2560, 2436},
  { "300x300", "C2", 2560, 3636},
  { "300x300", "C1", 2560, 3544},
};

LIST(dyesub_printsize_list_t, dnpds80_dock_printsize_list, dyesub_printsize_t, dnpds80_dock_printsize);

/* Model capabilities */

static const dyesub_cap_t dyesub_model_capabilities[] =
{
  { /* Olympus P-10, P-11 */
    2, 		
    &rgb_ink_list,
    &res_310dpi_list,
    &p10_page_list,
    &p10_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &p10_printer_init_func, &p10_printer_end_func,
    NULL, NULL, 
    &p10_block_init_func, NULL,
    NULL, NULL, NULL,	/* color profile/adjustment is built into printer */
    &p10_laminate_list,
  },
  { /* Olympus P-200 */
    4, 		
    &ymc_ink_list,
    &res_320dpi_list,
    &p200_page_list,
    &p200_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_BLOCK_ALIGN
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &p200_printer_init_func, &p200_printer_end_func,
    &p200_plane_init_func, NULL,
    NULL, NULL,
    p200_adj_any, p200_adj_any, p200_adj_any,
    NULL,
  },
  { /* Olympus P-300 */
    0, 		
    &ymc_ink_list,
    &p300_res_list,
    &p300_page_list,
    &p300_printsize_list,
    16,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_BLOCK_ALIGN
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &p300_printer_init_func, NULL,
    NULL, &p300_plane_end_func,
    &p300_block_init_func, NULL,
    p300_adj_cyan, p300_adj_magenta, p300_adj_yellow,
    NULL,
  },
  { /* Olympus P-400 */
    1,
    &ymc_ink_list,
    &res_314dpi_list,
    &p400_page_list,
    &p400_printsize_list,
    180,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &p400_printer_init_func, NULL,
    &p400_plane_init_func, &p400_plane_end_func,
    &p400_block_init_func, NULL,
    p400_adj_cyan, p400_adj_magenta, p400_adj_yellow,
    NULL,
  },
  { /* Olympus P-440 */
    3,
    &bgr_ink_list,
    &res_314dpi_list,
    &p440_page_list,
    &p440_printsize_list,
    128,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &p440_printer_init_func, &p440_printer_end_func,
    NULL, NULL,
    &p440_block_init_func, &p440_block_end_func,
    NULL, NULL, NULL,	/* color profile/adjustment is built into printer */
    &p10_laminate_list,
  },
  { /* Olympus P-S100 */
    20,
    &bgr_ink_list,
    &res_306dpi_list,
    &ps100_page_list,
    &ps100_printsize_list,
    1808,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &ps100_printer_init_func, &ps100_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL,	/* color profile/adjustment is built into printer */
    NULL,
  },
  { /* Canon CP-10 */
    1002,
    &ymc_ink_list,
    &res_300dpi_list,
    &cp10_page_list,
    &cp10_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &cpx00_printer_init_func, NULL,
    &cpx00_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
    NULL,
  },
  { /* Canon CP-100, CP-200, CP-300 */
    1000,
    &ymc_ink_list,
    &res_300dpi_list,
    &cpx00_page_list,
    &cpx00_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &cpx00_printer_init_func, NULL,
    &cpx00_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
    NULL,
  },
  { /* Canon CP-220, CP-330, SELPHY CP-400, SELPHY CP-500, SELPHY CP-510,
       SELPHY CP-600, SELPHY CP-710, SELPHY CP-720, SELPHY CP-730,
       SELPHY CP-740, SELPHY CP-750 */
    1001,
    &ymc_ink_list,
    &res_300dpi_list,
    &cp220_page_list,
    &cp220_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &cpx00_printer_init_func, NULL,
    &cpx00_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
    NULL,
  },
  { /* Canon CP-520, SELPHY CP-530 */
    1004,
    &ymc_ink_list,
    &res_300dpi_list,
    &cp220_page_list,
    &cp220_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &cp520_printer_init_func, NULL,
    &cp520_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
    NULL,
  },
  { /* Canon SELPHY ES1, ES2, ES20 (!experimental) */
    1003,
    &ymc_ink_list,
    &res_300dpi_list,
    &cpx00_page_list,
    &cpx00_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &es1_printer_init_func, NULL,
    &es1_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
    NULL,
  },
  { /* Sony DPP-EX5, DPP-EX7 */
    2002,
    &rgb_ink_list,
    &res_403dpi_list,
    &dppex5_page_list,
    &dppex5_printsize_list,
    100,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS,
    &dppex5_printer_init, &dppex5_printer_end,
    NULL, NULL,
    &dppex5_block_init, NULL,
    NULL, NULL, NULL,
    &dppex5_laminate_list,
  },
  { /* Sony UP-DP10  */
    2000,
    &cmy_ink_list,
    &res_300dpi_list,
    &updp10_page_list,
    &updp10_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS,
    &updp10_printer_init_func, &updp10_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    updp10_adj_cyan, updp10_adj_magenta, updp10_adj_yellow,
    &updp10_laminate_list,
  },
  { /* Sony UP-DR100 */
    2003,
    &rgb_ink_list,
    &res_334dpi_list,
    &updr100_page_list,
    &updr100_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &updr100_printer_init_func, &updr100_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL, 
    &updr100_laminate_list,
  },
  { /* Sony UP-DR150 */
    2001,
    &rgb_ink_list,
    &res_334dpi_list,
    &updr150_page_list,
    &updr150_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &updr150_printer_init_func, &updr150_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL, 
    NULL,
  },
  { /* Fujifilm Printpix CX-400  */
    3000,
    &rgb_ink_list,
    &res_310dpi_list,
    &cx400_page_list,
    &cx400_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS,
    &cx400_printer_init_func, NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL,	/* color profile/adjustment is built into printer */
    NULL,
  },
  { /* Fujifilm Printpix CX-550  */
    3001,
    &rgb_ink_list,
    &res_310dpi_list,
    &cx400_page_list,
    &cx400_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS,
    &cx400_printer_init_func, NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL,	/* color profile/adjustment is built into printer */
    NULL,
  },
  { /* Fujifilm FinePix NX-500  */
    3002,
    &rgb_ink_list,
    &res_306dpi_list,
    &nx500_page_list,
    &nx500_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &nx500_printer_init_func, NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL,	/* color profile/adjustment is built into printer */
    NULL,
  },
  { /* Kodak Easyshare Dock family */
    4000, 		
    &ymc_ink_list,
    &res_300dpi_list,
    &kodak_dock_page_list,
    &kodak_dock_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &kodak_dock_printer_init, NULL,
    &kodak_dock_plane_init, NULL,
    NULL, NULL,
    NULL, NULL, NULL,
    NULL,
  },
  { /* Shinko CHC-S9045 (experimental) */
    5000, 		
    &rgb_ink_list,
    &res_300dpi_list,
    &shinko_chcs9045_page_list,
    &shinko_chcs9045_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &shinko_chcs9045_printer_init, NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL,
    NULL,
  },
  { /* Dai Nippon Printing DS40 */
    6000,
    &rgb_ink_list,
    &res_300dpi_list,
    &dnpds40_dock_page_list,
    &dnpds40_dock_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_PLANE_LEFTTORIGHT,
    NULL, &dnpds40_printer_end,
    &dnpds40_plane_init, NULL,
    NULL, NULL,
    NULL, NULL, NULL,
    NULL,
  },
  { /* Dai Nippon Printing DS80 */
    6001,
    &rgb_ink_list,
    &res_300dpi_list,
    &dnpds80_dock_page_list,
    &dnpds80_dock_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_PLANE_LEFTTORIGHT,
    NULL, &dnpds40_printer_end,
    &dnpds40_plane_init, NULL,
    NULL, NULL,
    NULL, NULL, NULL,
    NULL,
  },
};

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
    /* TRANSLATORS: Some dye sublimation printers are able to achieve */
    /* better durability of output by covering it with transparent */
    /* laminate surface. This surface can be of different patterns: */
    /* common are matte, glossy or texture. */
    "Laminate", N_("Laminate Pattern"), "Color=Yes,Category=Advanced Printer Setup",
    N_("Laminate Pattern"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 0, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Borderless", N_("Borderless"), "Color=No,Category=Advanced Printer Setup",
    N_("Print without borders"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 0, STP_CHANNEL_NONE, 1, 0
  },
  {
    "PrintingMode", N_("Printing Mode"), "Color=Yes,Category=Core Parameter",
    N_("Printing Output Mode"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
};

static int the_parameter_count =
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
      "CyanDensity", N_("Cyan Balance"), N_("Output Level Adjustment"),
      N_("Adjust the cyan balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 1, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "MagentaDensity", N_("Magenta Balance"), N_("Output Level Adjustment"),
      N_("Adjust the magenta balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 2, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "YellowDensity", N_("Yellow Balance"), N_("Output Level Adjustment"),
      N_("Adjust the yellow balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 3, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "BlackDensity", N_("Black Balance"), N_("Output Level Adjustment"),
      N_("Adjust the black balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 0, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
};    

static const int float_parameter_count =
sizeof(float_parameters) / sizeof(const float_param_t);

static const dyesub_cap_t* dyesub_get_model_capabilities(int model)
{
  int i;
  int models = sizeof(dyesub_model_capabilities) / sizeof(dyesub_cap_t);

  for (i=0; i<models; i++)
    {
      if (dyesub_model_capabilities[i].model == model)
        return &(dyesub_model_capabilities[i]);
    }
  stp_deprintf(STP_DBG_DYESUB,
  	"dyesub: model %d not found in capabilities list.\n", model);
  return &(dyesub_model_capabilities[0]);
}

static const laminate_t* dyesub_get_laminate_pattern(stp_vars_t *v)
{
  const char *lpar = stp_get_string_parameter(v, "Laminate");
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(
		  				stp_get_model_id(v));
  const laminate_list_t *llist = caps->laminate;
  const laminate_t *l = NULL;
  int i;

  for (i = 0; i < llist->n_items; i++)
    {
      l = &(llist->item[i]);
      if (strcmp(l->name, lpar) == 0)
	 break;
    }
  return l;
}
  
static void
dyesub_printsize(const stp_vars_t *v,
		   int  *width,
		   int  *height)
{
  int i;
  const char *page = stp_get_string_parameter(v, "PageSize");
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(
		  				stp_get_model_id(v));
  const dyesub_printsize_list_t *p = caps->printsize;

  for (i = 0; i < p->n_items; i++)
    {
      if (strcmp(p->item[i].res_name,resolution) == 0 &&
          strcmp(p->item[i].pagesize_name,page) == 0)
        {
          *width  = p->item[i].width_px;
	  *height = p->item[i].height_px;
          return;
        }
    }
  stp_erprintf("dyesub_printsize: printsize not found (%s, %s)\n",
	       page, resolution);
}

static int
dyesub_feature(const dyesub_cap_t *caps, int feature)
{
  return ((caps->features & feature) == feature);
}

static stp_parameter_list_t
dyesub_list_parameters(const stp_vars_t *v)
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
dyesub_parameters(const stp_vars_t *v, const char *name,
	       stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(
		  				stp_get_model_id(v));

  description->p_type = STP_PARAMETER_TYPE_INVALID;
  if (name == NULL)
    return;

  description->deflt.str = NULL;
  for (i = 0; i < float_parameter_count; i++)
    if (strcmp(name, float_parameters[i].param.name) == 0)
      {
	stp_fill_parameter_settings(description,
				     &(float_parameters[i].param));
	description->deflt.dbl = float_parameters[i].defval;
	description->bounds.dbl.upper = float_parameters[i].max;
	description->bounds.dbl.lower = float_parameters[i].min;
      }

  for (i = 0; i < the_parameter_count; i++)
    if (strcmp(name, the_parameters[i].name) == 0)
      {
	stp_fill_parameter_settings(description, &(the_parameters[i]));
	break;
      }
  if (strcmp(name, "PageSize") == 0)
    {
      int default_specified = 0;
      const dyesub_pagesize_list_t *p = caps->pages;
      const char* text;

      description->bounds.str = stp_string_list_create();
      for (i = 0; i < p->n_items; i++)
	{
          const stp_papersize_t *pt = stp_get_papersize_by_name(
			  p->item[i].name);

	  text = (p->item[i].text ? p->item[i].text : pt->text);
	  stp_string_list_add_string(description->bounds.str,
			  p->item[i].name, gettext(text));
	  if (! default_specified && pt && pt->width > 0 && pt->height > 0)
	    {
	      description->deflt.str = p->item[i].name;
	      default_specified = 1;
	    }
	}
      if (!default_specified)
	description->deflt.str =
	  stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "MediaType") == 0)
    {
      description->bounds.str = stp_string_list_create();
      description->is_active = 0;
    }
  else if (strcmp(name, "InputSlot") == 0)
    {
      description->bounds.str = stp_string_list_create();
      description->is_active = 0;
    }
  else if (strcmp(name, "Resolution") == 0)
    {
      char res_text[24];
      const dyesub_resolution_list_t *r = caps->resolution;

      description->bounds.str = stp_string_list_create();
      for (i = 0; i < r->n_items; i++)
        {
	  sprintf(res_text, "%s DPI", r->item[i].name);
	  stp_string_list_add_string(description->bounds.str,
		r->item[i].name, gettext(res_text));
	}
      if (r->n_items < 1)
        description->is_active = 0;
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "InkType") == 0)
    {
      description->bounds.str = stp_string_list_create();
      for (i = 0; i < caps->inks->n_items; i++)
	stp_string_list_add_string(description->bounds.str,
			  caps->inks->item[i].name, gettext(caps->inks->item[i].name));
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
      if (caps->inks->n_items < 2)
        description->is_active = 0;
    }
  else if (strcmp(name, "Laminate") == 0)
    {
      description->bounds.str = stp_string_list_create();
      if (caps->laminate)
        {
          const laminate_list_t *llist = caps->laminate;

          for (i = 0; i < llist->n_items; i++)
            {
              const laminate_t *l = &(llist->item[i]);
	      stp_string_list_add_string(description->bounds.str,
			  	l->name, gettext(l->text));
	    }
          description->deflt.str =
	  stp_string_list_param(description->bounds.str, 0)->name;
          description->is_active = 1;
        }
    }
  else if (strcmp(name, "Borderless") == 0)
    {
      if (dyesub_feature(caps, DYESUB_FEATURE_BORDERLESS)) 
        description->is_active = 1;
    }
  else if (strcmp(name, "PrintingMode") == 0)
    {
      description->bounds.str = stp_string_list_create();
      stp_string_list_add_string
	(description->bounds.str, "Color", _("Color"));
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else
    description->is_active = 0;
}


static const dyesub_pagesize_t*
dyesub_current_pagesize(const stp_vars_t *v)
{
  const char *page = stp_get_string_parameter(v, "PageSize");
  const stp_papersize_t *pt = stp_get_papersize_by_name(page);
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(
		  				stp_get_model_id(v));
  const dyesub_pagesize_list_t *p = caps->pages;
  int i;

  for (i = 0; i < p->n_items; i++)
    {
      if (strcmp(p->item[i].name,pt->name) == 0)
          return &(p->item[i]);
    }
  return NULL;
}

static void
dyesub_media_size(const stp_vars_t *v,
		int *width,
		int *height)
{
  const dyesub_pagesize_t *p = dyesub_current_pagesize(v);
  stp_default_media_size(v, width, height);

  if (p && p->width_pt > 0)
    *width = p->width_pt;
  if (p && p->height_pt > 0)
    *height = p->height_pt;
}

static void
dyesub_imageable_area_internal(const stp_vars_t *v,
				int  use_maximum_area,
				int  *left,
				int  *right,
				int  *bottom,
				int  *top,
				int  *print_mode)
{
  int width, height;
  const dyesub_pagesize_t *p = dyesub_current_pagesize(v);
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(
		  				stp_get_model_id(v));

  dyesub_media_size(v, &width, &height);
  if (use_maximum_area
      || (dyesub_feature(caps, DYESUB_FEATURE_BORDERLESS) &&
          stp_get_boolean_parameter(v, "Borderless"))
      || !p)
    {
      *left = 0;
      *top  = 0;
      *right  = width;
      *bottom = height;
    }
  else
    {
      *left = p->border_pt_left;
      *top  = p->border_pt_top;
      *right  = width  - p->border_pt_right;
      *bottom = height - p->border_pt_bottom;
    }
  if (p)
    *print_mode = p->print_mode;
  else
    *print_mode = DYESUB_PORTRAIT;
}

static void
dyesub_imageable_area(const stp_vars_t *v,
		       int  *left,
		       int  *right,
		       int  *bottom,
		       int  *top)
{
  int not_used;
  dyesub_imageable_area_internal(v, 0, left, right, bottom, top, &not_used);
}

static void
dyesub_maximum_imageable_area(const stp_vars_t *v,
			       int  *left,
			       int  *right,
			       int  *bottom,
			       int  *top)
{
  int not_used;
  dyesub_imageable_area_internal(v, 1, left, right, bottom, top, &not_used);
}

static void
dyesub_limit(const stp_vars_t *v,			/* I */
	    int *width, int *height,
	    int *min_width, int *min_height)
{
  *width  = SHRT_MAX;
  *height = SHRT_MAX;
  *min_width  = 1;
  *min_height =	1;
}

static void
dyesub_describe_resolution(const stp_vars_t *v, int *x, int *y)
{
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(
  							stp_get_model_id(v));
  const dyesub_resolution_list_t *r = caps->resolution;
  int i;

  *x = -1;
  *y = -1;
  if (resolution)
    {
      for (i = 0; i < r->n_items; i++)
	{
	  if (strcmp(resolution, r->item[i].name) == 0)
	    {
	      *x = r->item[i].w_dpi;
	      *y = r->item[i].h_dpi;
	      break;
	    }
	}
    }  
  return;
}

static const char *
dyesub_describe_output_internal(const stp_vars_t *v, dyesub_print_vars_t *pv)
{
  const char *ink_type      = stp_get_string_parameter(v, "InkType");
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(
  							stp_get_model_id(v));
  const char *output_type;
  int i;

  pv->ink_channels = 1;
  pv->ink_order = NULL;
  output_type = "CMY";

  if (ink_type)
    {
      for (i = 0; i < caps->inks->n_items; i++)
	if (strcmp(ink_type, caps->inks->item[i].name) == 0)
	  {
	    output_type = caps->inks->item[i].output_type;
	    pv->ink_channels = caps->inks->item[i].output_channels;
	    pv->ink_order = caps->inks->item[i].channel_order;
	    break;
	  }
    }
  
  return output_type;
}

static const char *
dyesub_describe_output(const stp_vars_t *v)
{
  dyesub_print_vars_t ipv;
  return dyesub_describe_output_internal(v, &ipv);
}

static void
dyesub_nputc(stp_vars_t *v, char byte, int count)
{
  if (count == 1)
    stp_putc(byte, v);
  else
    {
      int i;
      char *buf = privdata.nputc_buf;
      int size = count;
      int blocks = size / NPUTC_BUFSIZE;
      int leftover = size % NPUTC_BUFSIZE;
      if (size > NPUTC_BUFSIZE)
	size = NPUTC_BUFSIZE;
      (void) memset(buf, byte, size);
      if (blocks)
	for (i = 0; i < blocks; i++)
	  stp_zfwrite(buf, size, 1, v);
      if (leftover)
	stp_zfwrite(buf, leftover, 1, v);
    }
}

static void
dyesub_swap_ints(int *a, int *b)
{
  int t = *a;
  *a = *b; 
  *b = t;
}

static void
dyesub_adjust_curve(stp_vars_t *v,
		const char *color_adj,
		const char *color_curve)
{
  stp_curve_t   *adjustment = NULL;

  if (color_adj &&
        !stp_check_curve_parameter(v, color_curve, STP_PARAMETER_ACTIVE))
    {
      adjustment = stp_curve_create_from_string(color_adj);
      stp_set_curve_parameter(v, color_curve, adjustment);
      stp_set_curve_parameter_active(v, color_curve, STP_PARAMETER_ACTIVE);
      stp_curve_destroy(adjustment);
    }
}

static void
dyesub_exec(stp_vars_t *v,
		void (*func)(stp_vars_t *),
		const char *debug_string)
{
  if (func)
    {
      stp_deprintf(STP_DBG_DYESUB, "dyesub: %s\n", debug_string);
      (*func)(v);
    }
}

static int
dyesub_interpolate(int oldval, int oldsize, int newsize)
{
  /* 
   * This is simple linear interpolation algorithm.
   * When imagesize <> printsize I need rescale image somehow... :-/ 
   */
  return (int)(oldval * newsize / oldsize);
}

static void
dyesub_free_image(dyesub_print_vars_t *pv, stp_image_t *image)
{
  unsigned short** image_data = pv->image_data;
  int image_px_height = pv->image_rows;
  int i;

  for (i = 0; i< image_px_height; i++)
    if (image_data[i])
      stp_free(image_data[i]);
  if (image_data)
    stp_free(image_data);
}

static unsigned short **
dyesub_read_image(stp_vars_t *v,
		dyesub_print_vars_t *pv,
		stp_image_t *image)
{
  int image_px_width  = stp_image_width(image);
  int image_px_height = stp_image_height(image);
  int row_size = image_px_width * pv->ink_channels * pv->bytes_per_out_channel;
  unsigned short **image_data;
  unsigned int zero_mask;
  int i;

  image_data = stp_zalloc(image_px_height * sizeof(unsigned short *));
  pv->image_rows = 0;
  if (!image_data)
    return NULL;	/* ? out of memory ? */

  for (i = 0; i < image_px_height; i++)
    {
      if (stp_color_get_row(v, image, i, &zero_mask))
        {
	  stp_deprintf(STP_DBG_DYESUB,
	  	"dyesub_read_image: "
		"stp_color_get_row(..., %d, ...) == 0\n", i);
	  dyesub_free_image(pv, image);
	  return NULL;
	}	
      image_data[i] = stp_malloc(row_size);
      pv->image_rows = i+1;
      if (!image_data[i])
        {
	  stp_deprintf(STP_DBG_DYESUB,
	  	"dyesub_read_image: "
		"(image_data[%d] = stp_malloc()) == NULL\n", i);
	  dyesub_free_image(pv, image);
	  return NULL;
	}	
      memcpy(image_data[i], stp_channel_get_output(v), row_size);
    }
  return image_data;
}

static int
dyesub_print_pixel(stp_vars_t *v,
		dyesub_print_vars_t *pv,
		int row,
		int col,
		int plane)
{
  unsigned short ink[MAX_INK_CHANNELS * MAX_BYTES_PER_CHANNEL], *out;
  unsigned char *ink_u8;
  int i, j, b;
  
  if (pv->print_mode == DYESUB_LANDSCAPE)
    { /* "rotate" image */
      dyesub_swap_ints(&col, &row);
      row = (pv->imgw_px - 1) - row;
    }

  out = &(pv->image_data[row][col * pv->out_channels]);

  for (i = 0; i < pv->ink_channels; i++)
    {
      if (pv->out_channels == pv->ink_channels)
        { /* copy out_channel (image) to equiv ink_channel (printer) */
          ink[i] = out[i];
        }
      else if (pv->out_channels < pv->ink_channels)
        { /* several ink_channels (printer) "share" same out_channel (image) */
          ink[i] = out[i * pv->out_channels / pv->ink_channels];
        }
      else /* (pv->out_channels > pv->ink_channels) */
        { /* merge several out_channels (image) into ink_channel (printer) */
          int avg = 0;
          for (j = 0; j < pv->out_channels / pv->ink_channels; j++)
            avg += out[j + i * pv->out_channels / pv->ink_channels];
	  ink[i] = avg * pv->ink_channels / pv->out_channels;
	}
    }
   
  if (pv->bytes_per_ink_channel == 1) /* convert 16bits to 8bit */
    {
      ink_u8 = (unsigned char *) ink;
      for (i = 0; i < pv->ink_channels; i++)
        ink_u8[i] = ink[i] / 257;
    }
	
  if (pv->plane_interlacing)
    stp_zfwrite((char *) ink + plane, pv->bytes_per_ink_channel, 1, v);
  else
/*  stp_zfwrite((char *) ink, pv->bytes_per_ink_channel, pv->ink_channels, v);*/
      /* print inks in right order, eg. RGB  BGR */
      for (b = 0; b < pv->ink_channels; b++)
	stp_zfwrite((char *) ink + (pv->ink_order[b]-1), pv->bytes_per_ink_channel, 1, v);

  return 1;
}

static int
dyesub_print_row(stp_vars_t *v,
		dyesub_print_vars_t *pv,
		int row,
		int plane)
{
  int ret = 0;
  int w, col;
  
  for (w = 0; w < pv->outw_px; w++)
    {
      col = dyesub_interpolate(w, pv->outw_px, pv->imgw_px);
      if (pv->plane_lefttoright)
	ret = dyesub_print_pixel(v, pv, row, pv->imgw_px - col - 1, plane);
      else
	ret = dyesub_print_pixel(v, pv, row, col, plane);
      if (ret > 1)
      	break;
    }
  return ret;
}

static int
dyesub_print_plane(stp_vars_t *v,
		dyesub_print_vars_t *pv,
		const dyesub_cap_t *caps,
		int plane)
{
  int ret = 0;
  int h, row;
  int out_bytes = (pv->plane_interlacing ? 1 : pv->ink_channels)
  					* pv->bytes_per_ink_channel;


  for (h = 0; h <= pv->prnb_px - pv->prnt_px; h++)
    {
      if (h % caps->block_size == 0)
        { /* block init */
	  privdata.block_min_h = h + pv->prnt_px;
	  privdata.block_min_w = pv->prnl_px;
	  privdata.block_max_h = MIN(h + pv->prnt_px + caps->block_size - 1,
	  					pv->prnb_px);
	  privdata.block_max_w = pv->prnr_px;

	  dyesub_exec(v, caps->block_init_func, "caps->block_init");
	}

      if (h + pv->prnt_px < pv->outt_px || h + pv->prnt_px >= pv->outb_px)
        { /* empty part above or below image area */
          dyesub_nputc(v, pv->empty_byte, out_bytes * pv->prnw_px);
	}
      else
        {
	  if (dyesub_feature(caps, DYESUB_FEATURE_FULL_WIDTH)
	  	&& pv->outl_px > 0)
	    { /* empty part left of image area */
              dyesub_nputc(v, pv->empty_byte, out_bytes * pv->outl_px);
	    }

	  row = dyesub_interpolate(h + pv->prnt_px - pv->outt_px,
	  					pv->outh_px, pv->imgh_px);
	  stp_deprintf(STP_DBG_DYESUB,
	  	"dyesub_print_plane: h = %d, row = %d\n", h, row);
	  ret = dyesub_print_row(v, pv, row, plane);

	  if (dyesub_feature(caps, DYESUB_FEATURE_FULL_WIDTH)
	  	&& pv->outr_px < pv->prnw_px)
	    { /* empty part right of image area */
              dyesub_nputc(v, pv->empty_byte, out_bytes
	      				* (pv->prnw_px - pv->outr_px));
	    }
	}

      if (h + pv->prnt_px == privdata.block_max_h)
        { /* block end */
	  dyesub_exec(v, caps->block_end_func, "caps->block_end");
	}
    }
  return ret;
}

/*
 * dyesub_print()
 */
static int
dyesub_do_print(stp_vars_t *v, stp_image_t *image)
{
  int i;
  dyesub_print_vars_t pv;
  int status = 1;

  const int model           = stp_get_model_id(v); 
  const char *ink_type;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(model);
  int max_print_px_width = 0;
  int max_print_px_height = 0;
  int w_dpi, h_dpi;	/* Resolution */

  /* output in 1/72" */
  int out_pt_width  = stp_get_width(v);
  int out_pt_height = stp_get_height(v);
  int out_pt_left   = stp_get_left(v);
  int out_pt_top    = stp_get_top(v);

  /* page in 1/72" */
  int page_pt_width  = stp_get_page_width(v);
  int page_pt_height = stp_get_page_height(v);
  int page_pt_left = 0;
  int page_pt_right = 0;
  int page_pt_top = 0;
  int page_pt_bottom = 0;
  int page_mode;	

  int pl;



  if (!stp_verify(v))
    {
      stp_eprintf(v, _("Print options not verified; cannot print.\n"));
      return 0;
    }
  (void) memset(&pv, 0, sizeof(pv));

  stp_image_init(image);
  pv.imgw_px = stp_image_width(image);
  pv.imgh_px = stp_image_height(image);

  stp_describe_resolution(v, &w_dpi, &h_dpi);
  dyesub_printsize(v, &max_print_px_width, &max_print_px_height);

  privdata.pagesize = stp_get_string_parameter(v, "PageSize");
  if (caps->laminate)
	  privdata.laminate = dyesub_get_laminate_pattern(v);

  dyesub_imageable_area_internal(v, 
  	(dyesub_feature(caps, DYESUB_FEATURE_WHITE_BORDER) ? 1 : 0),
	&page_pt_left, &page_pt_right, &page_pt_bottom, &page_pt_top,
	&page_mode);
  
  pv.prnw_px = MIN(max_print_px_width,
		  	PX(page_pt_right - page_pt_left, w_dpi));
  pv.prnh_px = MIN(max_print_px_height,
			PX(page_pt_bottom - page_pt_top, h_dpi));
  pv.outw_px = PX(out_pt_width, w_dpi);
  pv.outh_px = PX(out_pt_height, h_dpi);


  /* if image size is close enough to output size send out original size */
  if (abs(pv.outw_px - pv.imgw_px) < SIZE_THRESHOLD)
      pv.outw_px  = pv.imgw_px;
  if (abs(pv.outh_px - pv.imgh_px) < SIZE_THRESHOLD)
      pv.outh_px = pv.imgh_px;

  pv.outw_px = MIN(pv.outw_px, pv.prnw_px);
  pv.outh_px = MIN(pv.outh_px, pv.prnh_px);
  pv.outl_px = MIN(PX(out_pt_left - page_pt_left, w_dpi),
			pv.prnw_px - pv.outw_px);
  pv.outt_px = MIN(PX(out_pt_top  - page_pt_top, h_dpi),
			pv.prnh_px - pv.outh_px);
  pv.outr_px = pv.outl_px + pv.outw_px;
  pv.outb_px = pv.outt_px  + pv.outh_px;
  

  stp_deprintf(STP_DBG_DYESUB,
	      "paper (pt)   %d x %d\n"
	      "image (px)   %d x %d\n"
	      "image (pt)   %d x %d\n"
	      "* out (pt)   %d x %d\n"
	      "* out (px)   %d x %d\n"
	      "* left x top (pt) %d x %d\n"
	      "* left x top (px) %d x %d\n"
	      "border (pt) (%d - %d) = %d x (%d - %d) = %d\n"
	      "printable pixels (px)   %d x %d\n"
	      "res (dpi)               %d x %d\n",
	      page_pt_width, page_pt_height,
	      pv.imgw_px, pv.imgh_px,
	      PT(pv.imgw_px, w_dpi), PT(pv.imgh_px, h_dpi),
	      out_pt_width, out_pt_height,
	      pv.outw_px, pv.outh_px,
	      out_pt_left, out_pt_top,
	      pv.outl_px, pv.outt_px,
	      page_pt_right, page_pt_left, page_pt_right - page_pt_left,
	      page_pt_bottom, page_pt_top, page_pt_bottom - page_pt_top,
	      pv.prnw_px, pv.prnh_px,
	      w_dpi, h_dpi
	      );	


  /* FIXME: move this into print_init_drv */
  ink_type = dyesub_describe_output_internal(v, &pv);
  stp_set_string_parameter(v, "STPIOutputType", ink_type);
  stp_channel_reset(v);
  for (i = 0; i < pv.ink_channels; i++)
    stp_channel_add(v, i, 0, 1.0);
  pv.out_channels = stp_color_init(v, image, 65536);
  pv.bytes_per_ink_channel = 1;		/* FIXME: this is printer dependent */
  pv.bytes_per_out_channel = 2;		/* FIXME: this is ??? */
  pv.image_data = dyesub_read_image(v, &pv, image);
  pv.empty_byte = (ink_type &&
 		(strcmp(ink_type, "RGB") == 0 || strcmp(ink_type, "BGR") == 0)
		? '\xff' : '\0');
  pv.plane_interlacing = dyesub_feature(caps, DYESUB_FEATURE_PLANE_INTERLACE);
  pv.plane_lefttoright = dyesub_feature(caps, DYESUB_FEATURE_PLANE_LEFTTORIGHT);
  pv.print_mode = page_mode;
  if (!pv.image_data)
    {
      stp_image_conclude(image);
      return 2;
    }
  /* /FIXME */


  dyesub_adjust_curve(v, caps->adj_cyan, "CyanCurve");
  dyesub_adjust_curve(v, caps->adj_magenta, "MagentaCurve");
  dyesub_adjust_curve(v, caps->adj_yellow, "YellowCurve");
  stp_set_float_parameter(v, "Density", 1.0);

  if (dyesub_feature(caps, DYESUB_FEATURE_FULL_HEIGHT))
    {
      pv.prnt_px = 0;
      pv.prnb_px = pv.prnh_px - 1;
    }
  else if (dyesub_feature(caps, DYESUB_FEATURE_BLOCK_ALIGN))
    {
      pv.prnt_px = pv.outt_px - (pv.outt_px % caps->block_size);
      				/* floor to multiple of block_size */
      pv.prnb_px = (pv.outb_px - 1) + (caps->block_size - 1)
      		- ((pv.outb_px - 1) % caps->block_size);
				/* ceil to multiple of block_size */
    }
  else
    {
      pv.prnt_px = pv.outt_px;
      pv.prnb_px = pv.outb_px - 1;
    }
  
  if (dyesub_feature(caps, DYESUB_FEATURE_FULL_WIDTH))
    {
      pv.prnl_px = 0;
      pv.prnr_px = pv.prnw_px - 1;
    }
  else
    {
      pv.prnl_px = pv.outl_px;
      pv.prnr_px = pv.outr_px;
    }
      
  if (pv.print_mode == DYESUB_LANDSCAPE)
    {
      dyesub_swap_ints(&pv.outh_px, &pv.outw_px);
      dyesub_swap_ints(&pv.outt_px, &pv.outl_px);
      dyesub_swap_ints(&pv.outb_px, &pv.outr_px);
      
      dyesub_swap_ints(&pv.prnh_px, &pv.prnw_px);
      dyesub_swap_ints(&pv.prnt_px, &pv.prnl_px);
      dyesub_swap_ints(&pv.prnb_px, &pv.prnr_px);
      
      dyesub_swap_ints(&pv.imgh_px, &pv.imgw_px);
    }

	/* assign private data *after* swaping image dimensions */
  privdata.w_dpi = w_dpi;
  privdata.h_dpi = h_dpi;
  privdata.w_size = pv.prnw_px;
  privdata.h_size = pv.prnh_px;
  privdata.print_mode = pv.print_mode;

  /* printer init */
  dyesub_exec(v, caps->printer_init_func, "caps->printer_init");

  for (pl = 0; pl < (pv.plane_interlacing ? pv.ink_channels : 1); pl++)
    {
      privdata.plane = pv.ink_order[pl];
      stp_deprintf(STP_DBG_DYESUB, "dyesub: plane %d\n", privdata.plane);

      /* plane init */
      dyesub_exec(v, caps->plane_init_func, "caps->plane_init");
  
      dyesub_print_plane(v, &pv, caps, (int) pv.ink_order[pl] - 1);

      /* plane end */
      dyesub_exec(v, caps->plane_end_func, "caps->plane_end");
    }

  /* printer end */
  dyesub_exec(v, caps->printer_end_func, "caps->printer_end");

  dyesub_free_image(&pv, image);
  stp_image_conclude(image);
  return status;
}

static int
dyesub_print(const stp_vars_t *v, stp_image_t *image)
{
  int status;
  stp_vars_t *nv = stp_vars_create_copy(v);
  stp_prune_inactive_options(nv);
  status = dyesub_do_print(nv, image);
  stp_vars_destroy(nv);
  return status;
}

static const stp_printfuncs_t print_dyesub_printfuncs =
{
  dyesub_list_parameters,
  dyesub_parameters,
  dyesub_media_size,
  dyesub_imageable_area,
  dyesub_maximum_imageable_area,
  dyesub_limit,
  dyesub_print,
  dyesub_describe_resolution,
  dyesub_describe_output,
  stp_verify_printer_params,
  NULL,
  NULL,
  NULL
};




static stp_family_t print_dyesub_module_data =
  {
    &print_dyesub_printfuncs,
    NULL
  };


static int
print_dyesub_module_init(void)
{
  return stp_family_register(print_dyesub_module_data.printer_list);
}


static int
print_dyesub_module_exit(void)
{
  return stp_family_unregister(print_dyesub_module_data.printer_list);
}


/* Module header */
#define stp_module_version print_dyesub_LTX_stp_module_version
#define stp_module_data print_dyesub_LTX_stp_module_data

stp_module_version_t stp_module_version = {0, 0};

stp_module_t stp_module_data =
  {
    "dyesub",
    VERSION,
    "DyeSub family driver",
    STP_MODULE_CLASS_FAMILY,
    NULL,
    print_dyesub_module_init,
    print_dyesub_module_exit,
    (void *) &print_dyesub_module_data
  };

