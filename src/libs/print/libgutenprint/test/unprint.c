/* $Id: unprint.c,v 1.46 2011/04/01 02:20:09 rlk Exp $ */
/*
 * Generate PPM files from printer output
 *
 * Copyright 2000-2001 Eric Sharkey <sharkey@superk.physics.sunysb.edu>
 *                     Andy Thaller <thaller@ph.tum.de>
 *                     Robert Krawitz <rlk@alum.mit.edu>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/util.h>
#include<stdio.h>
#include<stdlib.h>
#ifdef HAVE_LIMITS_H
#include<limits.h>
#endif
#include<string.h>

#ifdef __GNUC__
#define inline __inline__
#endif

typedef enum {
  QT_NONE,
  QT_QUAD,
  QT_MIS
} quadtone_t;

/*
 * Printer state variable.
 */
typedef struct {
  unsigned char unidirectional;
  unsigned char printer_weave;
  int page_management_units; /* dpi */
  int relative_horizontal_units; /* dpi */
  int absolute_horizontal_units; /* dpi, assumed to be >= relative */
  int relative_vertical_units; /* dpi */
  int absolute_vertical_units; /* dpi, assumed to be >= relative */
  int horizontal_spacing;	/* Horizontal dot spacing */
  int top_margin; /* dots */
  int bottom_margin; /* dots */
  int page_height; /* dots */
  int dotsize;
  int bpp; /* bits per pixel */
  int current_color;
  int xposition; /* dots */
  int yposition; /* dots */
  int monomode;
  int nozzle_separation;
  int nozzles;
  int extraskip;
  int got_graphics;
  int left_edge;
  int right_edge;
  int top_edge;
  int bottom_edge;
  quadtone_t quadtone;
} pstate_t;

/* We'd need about a gigabyte of ram to hold a ppm file of an 8.5 x 11
 * 1440 x 720 dpi page.  That's more than I have in my laptop, so, let's
 * play some games to reduce memory.  Allocate each scan line separately,
 * and don't require that the allocated height be full page width.  This
 * way, if we only want to print a 2x2 image, we only need to allocate the
 * ram that we need.  We'll build up the printed image in ram at low
 * color depth, KCMYcm color basis, and then write out the RGB ppm file
 * as output.  This way we never need to have the full data in RAM at any
 * time.  2 bits per color of KCMYcm is half the size of 8 bits per color
 * of RGB.
 *
 * We would like to be able to print in bands, so that we don't have to
 * read the entire page into memory in order to print it.  Unfortunately,
 * we may not know what the left and right margins are until we've read the
 * entire file.  If we can read it in two passes we could do it; use one
 * pass to scan the file looking at the margins, and another pass to
 * actually read in the data.  This optimization may be worthwhile.
 */

#define MAX_INKS 20
typedef struct {
   unsigned char *line[MAX_INKS];
   int startx[MAX_INKS];
   int stopx[MAX_INKS];
} line_type;

typedef unsigned char ppmpixel[3];

unsigned char *buf;
unsigned valid_bufsize;
unsigned char minibuf[256];
unsigned bufsize;
unsigned save_bufsize;
unsigned char ch;
unsigned short sh;
int eject = 0;
int global_counter = 0;
int global_count = 0;
unsigned color_mask = 0xffffffff;

pstate_t pstate;
int unweave;

line_type **page=NULL;

/* Color Codes:
   color    Epson1  Epson2   Sequential
   Black    0       0        0/16
   Magenta  1       1        1/17
   Cyan     2       2        2/18
   Yellow   4       4        3/19
   L.Mag.   17      257      4
   L.Cyan   18      258      5
   L.Black  16      256      6
   D.Yellow 36      516      7
   Red      7       N/A      8
   Blue     8       N/A      9
   P.Black  64      N/A      0
   Gloss    9       N/A      10
   LL.Black 48      768      11
   Orange   10       N/A     12
 */

/* convert either Epson1 or Epson2 color encoding into a sequential encoding */
static int
seqcolor(int c)
{
  switch (c)
    {
    case 0:
      return 0;
    case 1:
      return 1;
    case 2:
      return 2;
    case 4:
      return 3;
    case 17:
    case 257:
      return 4;
    case 18:
    case 258:
      return 5;
    case 16:
    case 256:
      return 6;
    case 36:
    case 516:
      return 7;
    case 7:
      return 8;
    case 8:
      return 9;
    case 9:
      return 10;
    case 48:
    case 768:
      return 11;
    case 10:
      return 12;
    case 64:
      return 16;
    case 65:
      return 17;
    case 66:
      return 18;
    case 68:
      return 19;
    default:
      return 0;
    }
}

extern void merge_line(line_type *p, unsigned char *l, int startl, int stopl,
		       int color);
extern void expand_line (unsigned char *src, unsigned char *dst, int height,
			 int skip, int left_ignore);
extern void write_output (FILE *fp_w, int dontwrite, int allblack);
extern void find_white (unsigned char *buff,int npix, int *left, int *right);
extern int update_page (unsigned char *buff, int buffsize, int m, int n,
			int color, int density);
extern void parse_escp2 (FILE *fp_r);
extern void reverse_bit_order (unsigned char *buff, int n);
extern int rle_decode (unsigned char *inbuf, int n, int max);
extern void parse_canon (FILE *fp_r);

unsigned get_mask_1[] = { 7, 6, 5, 4, 3, 2, 1, 0 };
unsigned get_mask_2[] = { 6, 4, 2, 0 };
unsigned get_mask_4[] = { 4, 0 };

static inline int
get_bits(unsigned char *p, int idx)
{
  /*
   * p is a pointer to a bit stream, ordered MSb first.  Extract the
   * indexth bpp bit width field and return that value.  Ignore byte
   * boundaries.
   */
  int value, b;
  unsigned addr;
  switch (pstate.bpp)
    {
    case 1:
      return (p[idx >> 3] >> (7 - (idx & 7))) & 1;
    case 2:
      b = get_mask_2[idx & 3];
      return (p[idx >> 2] >> b) & 3;
    case 4:
      b = get_mask_4[idx & 1];
      return (p[idx >> 1] >> b) & 0xf;
    case 8:
      return p[idx];
    default:
      addr = (idx * pstate.bpp);
      value = 0;
      for (b = 0; b < pstate.bpp; b++)
	{
	  value += value;
	  value |= (p[(addr + b) >> 3] >> (7 - ((addr + b) & 7))) & 1;
	}
      return(value);
    }
}

static unsigned clr_mask_1[] = { 0xfe, 0xfd, 0xfb, 0xf7,
				 0xef, 0xdf, 0xbf, 0x7f };
static unsigned clr_mask_2[] = { 0xfc, 0, 0xf3, 0, 0xcf, 0, 0x3f, 0 };
static unsigned clr_mask_4[] = { 0xf0, 0, 0, 0, 0xf, 0, 0, 0 };

static inline void
set_bits(unsigned char *p,int idx,int value)
{

  /*
   * p is a pointer to a bit stream, ordered MSb first.  Set the
   * indexth bpp bit width field to value value.  Ignore byte
   * boundaries.
   */

  int b;
  switch (pstate.bpp)
    {
    case 1:
      b = (7 - (idx & 7));
      p[idx >> 3] &= clr_mask_1[b];
      p[idx >> 3] |= value << b;
      break;
    case 2:
      b = get_mask_2[idx & 3];
      p[idx >> 2] &= clr_mask_2[b];
      p[idx >> 2] |= value << b;
      break;
    case 4:
      b = get_mask_4[idx & 1];
      p[idx >> 1] &= clr_mask_4[b];
      p[idx >> 1] |= value << b;
      break;
    case 8:
      p[idx] = value;
      break;
    default:
      for (b = pstate.bpp - 1; b >= 0; b--)
	{
	  if (value & 1)
	    p[(idx * pstate.bpp + b) / 8] |=
	      1 << (7 - ((idx * pstate.bpp + b) % 8));
	  else
	    p[(idx * pstate.bpp + b) / 8] &=
	      ~(1 << (7 - ((idx * pstate.bpp + b) % 8)));
	  value/=2;
	}
    }
}

static float ink_colors[MAX_INKS][4] =
/* C(R) M(G) Y(B) K(W) */
{{ 0,   0,   0,   1 },		/* 0  K */
 { 1,    .1, 1,   1 },		/* 1  M */
 {  .1, 1,   1,   1 },		/* 2  C */
 { 1,   1,    .1, 1 },		/* 3  Y */
 { 1,    .7, 1,   1 },		/* 4  m */
 {  .7, 1,   1,   1 },		/* 5  c */
 {  .7,  .7,  .7, 1 },		/* 6  k */
 {  .7,  .7, 0,   1 },		/* 7  dY */
 { 1,   0,   0,   1 },		/* 8  R */
 { 0,   0,   1,   1 },		/* 8  B */
 { 1,   1,   1,   1 },		/* 10 Gloss */
 {  .8,  .8,  .8, 1 },		/* 11 llk */
 {  .9,  .3, 0,   1 },		/* 12 Orange */
 { 0,   0,   0,   1 },		/* 13 K */
 { 0,   0,   0,   1 },		/* 14 K */
 { 0,   0,   0,   1 },		/* 15 K */
 { 0,   0,   0,   1 },		/* 16 K */
 { 1,    .1, 1,   1 },		/* 17 M */
 {  .1, 1,   1,   1 },		/* 18 C */
 { 1,   1,    .1, 1 },		/* 19 Y */
};

static float quadtone_inks[] = { 0.0, .5, .25, .75, .9, .8 };

static float mis_quadtone_inks[] = { 0.0, .25, .75, .5, .55, .85 };

static float bpp_shift[] = { 0, 1, 3, 7, 15, 31, 63, 127, 255 };

static inline void
mix_ink(ppmpixel p, int color, unsigned int amount, float *ink, quadtone_t quadtone)
{
  /* this is pretty crude */

  if (((1 << color) & color_mask) && amount)
    {
      int i;
      float size;

      size = (float) amount / bpp_shift[pstate.bpp];
      switch (quadtone)
	{
	case QT_QUAD:
	  for (i = 0; i < 3; i++)
	    p[i] *= (1 - size) + size * quadtone_inks[color];
	  break;
	case QT_MIS:
	  for (i = 0; i < 3; i++)
	    p[i] *= (1 - size) + size * mis_quadtone_inks[color];
	  break;
	default:
	  for (i = 0; i < 3; i++)
	    p[i] *= (1 - size) + size * ink[i];
	}
    }
}

void
merge_line(line_type *p, unsigned char *l, int startl, int stopl, int color)
{

  int i;
  int temp, shift, height, lvalue, pvalue, oldstop;
  int width, owidth;
  unsigned char *tempp;
  int reversed = 0;
  int need_realloc = 0;

  /*
   * If we have a pixel to the left of anything previously printed,
   * we need to expand our margins to the left.  This is a bit tricky...
   */
  if (startl < p->startx[color])
    {
      temp = p->startx[color];
      p->startx[color] = startl;
      startl = temp;

      temp = p->stopx[color];
      p->stopx[color] = stopl;
      stopl = temp;

      tempp = p->line[color];
      p->line[color] = l;
      l = tempp;
      reversed = 1;
    }
  shift = startl - p->startx[color];
  height = stopl - startl + 1;

  oldstop = p->stopx[color];
  if (stopl > p->stopx[color])
    {
      p->stopx[color] = stopl;
      need_realloc = 1;
    }
  if (need_realloc || reversed)
    {
      width = ((p->stopx[color] - p->startx[color] + 1) * pstate.bpp + 7) / 8;
      owidth = ((oldstop - p->startx[color] + 1) * pstate.bpp + 7) / 8;
      p->line[color] = stp_realloc(p->line[color], width);
      memset((p->line[color] + owidth), 0, (width - owidth));
    }
  /*
   * Can we do an empty line optimization?
   */
  for (i = 0; i < height; i++)
    {
      lvalue = get_bits(l, i);
      if (lvalue)
	{
	  pvalue = get_bits(p->line[color], i + shift);
	  pvalue += lvalue;
	  if (pvalue > (1 << pstate.bpp) - 1)
	    pvalue = (1 << pstate.bpp) - 1;
	  set_bits(p->line[color], i + shift, pvalue);
	}
    }
  stp_free(l);
}

void
expand_line (unsigned char *src, unsigned char *dst, int height, int skip,
	     int left_ignore)
{

  /*
   * src is a pointer to a bit stream which is composed of fields of height
   * bpp starting with the most significant bit of the first byte and
   * proceding from there with no regard to byte boundaries.  For the
   * existing Epson printers, bpp is 1 or 2, which means fields will never
   * cross byte boundaries.  However, if bpp were 3, this would undoubtedly
   * happen.  This routine will make no assumptions about bpp, and handle each
   * bit individually.  It's slow, but, it's the only way that will work in
   * the general case of arbitrary bpp.
   *
   * We want to copy each field from the src to the dst, spacing the fields
   * out every skip fields.  We should ignore the first left_ignore fields
   * pointed to by src, though.
   */

  int i;

  if ((skip == 1) && !(left_ignore * pstate.bpp % 8))
    {
      /* the trivial case, this should be faster */
      memcpy(dst, src + left_ignore * pstate.bpp / 8,
	     (height * pstate.bpp + 7) / 8);
    }
  else
    {
      for (i = 0; i < height; i++)
	set_bits(dst, i * skip, get_bits(src, i + left_ignore));
    }
}

int donothing;

void
write_output(FILE *fp_w, int dontwrite, int allblack)
{
  int c, l, p, left, right, first, last, width, height, i;
  unsigned int amount;
  ppmpixel *out_row;
  int oversample = pstate.absolute_horizontal_units /
    pstate.absolute_vertical_units;
  if (oversample == 0)
    oversample = 1;

  fprintf(stderr, "Margins: top: %d bottom: top+%d\n", pstate.top_margin,
          pstate.bottom_margin);

  first = pstate.top_edge;
  last = pstate.bottom_edge;
  left = pstate.left_edge;
  right = pstate.right_edge;
  height = oversample * (last - first + 1);

  fprintf(stderr, "Image from (%d,%d) (%.3fx%.3f) to (%d,%d) (%.3fx%.3f) size (%dx%d) (%.3fx%.3f)\n",
	  left, first - pstate.top_margin,
	  (left / (double) pstate.page_management_units),
	  (first / (double) pstate.page_management_units),
	  right, last - pstate.top_margin,
	  (right / (double) pstate.page_management_units),
	  (last / (double) pstate.page_management_units),
	  right - left + 1, last - first + 1,
	  (right - left + 1) / (double) pstate.page_management_units,
	  (last - first + 1) / (double) pstate.page_management_units);

  width = right - left + 1;
  if (width < 0)
    width=0;

  out_row = stp_malloc(sizeof(ppmpixel) * width);
  fprintf(stderr, "Writing output...\n");

  if (dontwrite)
    return;
  /* write out the PPM header */
  fprintf(fp_w, "P6\n");
  fprintf(fp_w, "%d %d\n", width, height);
  fprintf(fp_w, "255\n");
  for (l = first; l <= last; l++)
    {
      line_type *lt = page[l];
      memset(out_row, ~0, (sizeof(ppmpixel) * width));
      if (lt)
	{
	  for (c = 0; c < MAX_INKS; c++)
	    {
	      int inknum = allblack ? 0 : c;
	      float *ink = ink_colors[inknum];
	      if (lt->line[c])
		{
		  for (p = lt->startx[c]; p <= lt->stopx[c]; p++)
		    {
		      amount = get_bits(lt->line[c], p - lt->startx[c]);
		      mix_ink(out_row[p - left], c, amount, ink,
			      pstate.quadtone);
		    }
		}
	    }
	}
      for (i = 0; i < oversample; i++)
	fwrite(out_row, sizeof(ppmpixel), width, fp_w);
    }
  stp_free(out_row);
}

void
find_white(unsigned char *buff,int npix, int *left, int *right)
{

  /*
   * If a line has white borders on either side, count the number of
   * pixels and fill that info into left and right.
   */

  int i, j, max;
  int words, bytes, bits, extra;

  *left = *right = 0;
  bits = npix * pstate.bpp;
  bytes = bits / 8;
  extra = bits % 8;
  words = bytes / sizeof(int);

  /*
   * First, find the leftmost pixel.  We first identify the word
   * containing the byte, then the byte, and finally the pixel within
   * the byte.  It does seem like this is unnecessarily complex, perhaps?
   */
  max = words;
  for (i = 0; (i < max) && (((int *)buff)[i] == 0); i++)
    ;
  max = (i < words) ? (i + 1) * sizeof(int) : bytes;

  i *= sizeof(int);		/* Convert from ints to bytes */
  for (; (i < max) && (buff[i] == 0); i++)
    ;
  max = (i < bytes) ? 8 : extra;
  for (j = 0; (j < max) && !(buff[i] & (1 << (7 - j))); j++)
    ;
  *left = (i * 8 + j) / pstate.bpp;
  *right = 0;

  /* if left is everything, then right is nothing */
  if (*left == npix)
    return;

  /* right side, this is a little trickier */
  for (i = 0; (i < extra) && !(buff[bytes] & (1 << (i + 8 - extra))); i++)
    ;
  if (i < extra)
    {
      *right = i / pstate.bpp;
      return;
    }
  *right = extra;  /*temporarily store right in bits to avoid rounding error*/

  for (i = 0; (i < bytes % sizeof(int)) && !(buff[bytes - 1 - i]); i++)
    ;
  if (i < bytes % sizeof(int))
    {
      for (j = 0; (j < 8) && !(buff[bytes - 1 - i] & (1 << j)); j++)
	;
      *right = (*right + i * 8 + j) / pstate.bpp;
      return;
    }
  *right += i * 8;

  for (i = 0; (i < words) && !(((int *)buff)[words - 1 - i]); i++)
    ;

  if (i < words)
    {
      *right += i * sizeof(int) * 8;
      for (j = 0;
	   (j < sizeof(int)) && !(buff[(words - i) * sizeof(int) - 1 - j]);
	   j++)
	;
      if (j < sizeof(int))
	{
	  *right += j * 8;
	  max = (words - i) * sizeof(int) - 1 - j;
	  for (j = 0; (j < 8) && !(buff[max] & (1 << j)); j++)
	    ;
	  if (j < 8)
	    {
	      *right = (*right + j) / pstate.bpp;
	      return;
	    }
	}
    }
  fprintf(stderr, "Warning: Reality failure.  The impossible happened.\n");
}

int
update_page(unsigned char *buff, /* I - pixel data               */
	    int buffsize,        /* I - size of buff in bytes     */
	    int m,              /* I - height of area in pixels */
	    int n,              /* I - width of area in pixels  */
	    int color,          /* I - color of pixel data      */
	    int density         /* I - horizontal density in dpi  */
	    )
{
  int y, skip, oldstart, oldstop, mi = 0;
  int left_white, right_white, width;
  unsigned char *oldline;
  int sep;

  if ((n == 0) || (m == 0))
    return(0);  /* shouldn't happen */

  skip = pstate.relative_horizontal_units / density;
  skip *= pstate.extraskip;

  if (skip == 0)
    {
      fprintf(stderr, "Warning!  Attempting to print at %d DPI but units are "
	      "set to %d DPI.\n", density, pstate.relative_horizontal_units);
      return(0);
    }

  if (!page)
    {
      fprintf(stderr,
	      "Warning! Attempting to print before setting up page!\n");
      /*
       * Let's hope that we've at least initialized the printer with
       * with an ESC @ and allocate the default page.  Otherwise, we'll
       * have unpredictable results.  But, that's a pretty acurate statement
       * for a real printer, too!
       */
      page = (line_type **)
	stp_zalloc((pstate.bottom_margin - pstate.top_margin) *
		   sizeof(line_type *));
    }
  if (pstate.printer_weave)
    sep = 1;
  else
    sep = pstate.nozzle_separation;
  for (y=pstate.yposition; y < pstate.yposition + m * sep; y += sep, mi++)
    {
      if (y >= pstate.bottom_margin - pstate.top_margin)
	{
	  fprintf(stderr,
		  "Warning: Unprinter out of unpaper (limit %d, pos %d).\n",
		  pstate.bottom_margin, y);
	  return(1);
	}
      find_white(buff + mi * ((n * pstate.bpp + 7) / 8), n,
		 &left_white, &right_white);
      if (left_white == n)
	continue; /* ignore blank lines */
      if (!(page[y]))
	{
	  page[y] = (line_type *) stp_zalloc(sizeof(line_type));
	  if (y < pstate.top_edge)
	    pstate.top_edge = y;
	  if (y > pstate.bottom_edge)
	    pstate.bottom_edge = y;
	}
      if ((left_white * pstate.bpp < 8) && (skip == 1))
	{
	  left_white=0; /* if it's just a few bits, don't bother cropping */
	}               /* unless we need to expand the line anyway       */
      if (page[y]->line[color])
	{
	  oldline = page[y]->line[color];
	  oldstart = page[y]->startx[color];
	  oldstop = page[y]->stopx[color];
	}
      else
	{
	  oldline = NULL;
	  oldstart = -1;
	  oldstop = -1;
	}
      page[y]->startx[color] = pstate.xposition + left_white * skip;
      page[y]->stopx[color] =pstate.xposition + ((n - 1 - right_white) * skip);
      if (page[y]->startx[color] < pstate.left_edge)
	pstate.left_edge = page[y]->startx[color];
      if (page[y]->stopx[color] > pstate.right_edge)
	pstate.right_edge = page[y]->stopx[color];
      width = page[y]->stopx[color] - page[y]->startx[color];
      page[y]->line[color] =
	stp_zalloc(((width * skip + 1) * pstate.bpp + 7) / 8);
      expand_line(buff + mi * ((n * pstate.bpp + 7) / 8), page[y]->line[color],
		  width+1, skip, left_white);
      if (oldline)
	merge_line(page[y], oldline, oldstart, oldstop, color);
    }
  if (n)
    pstate.xposition += (n - 1) * skip + 1;
  return(0);
}

#define get1(error)							\
do									\
{									\
  if (!(global_count = fread(&ch, 1, 1, fp_r)))				\
    {									\
      fprintf(stderr, "%s at %d (%x), read %d",				\
	      error, global_counter, global_counter, global_count);	\
      eject = 1;							\
      continue;								\
    }									\
  else									\
    global_counter += global_count;					\
} while (0)

#define get2(error)							\
do									\
{									\
  if (!(global_count = fread(minibuf, 1, 2, fp_r)))			\
    {									\
      fprintf(stderr, "%s at %d (%x), read %d",				\
	      error, global_counter, global_counter, global_count);	\
      eject = 1;							\
      continue;								\
    }									\
  else									\
    {									\
      global_counter += global_count;					\
      sh = minibuf[0] + minibuf[1] * 256;				\
    }									\
} while (0)

#define getn(n,error)							\
do									\
{									\
  if (!(global_count = fread(buf, 1, n, fp_r)))				\
    {									\
      fprintf(stderr, "%s at %d (%x), read %d",				\
	      error, global_counter, global_counter, global_count);	\
      eject = 1;							\
      continue;								\
    }									\
  else									\
    global_counter += global_count;					\
} while (0)

#define getnoff(n,offset,error)						\
do									\
{									\
  if (!(global_count = fread(buf + offset, 1, n, fp_r)))		\
    {									\
      fprintf(stderr, "%s at %d (%x), read %d",				\
	      error, global_counter, global_counter, global_count);	\
      eject = 1;							\
      continue;								\
    }									\
  else									\
    global_counter += global_count;					\
} while (0)

static void
parse_escp2_data(FILE *fp_r)
{
  int i, m = 0, n = 0, c = 0;
  int currentcolor = 0;
  int density = 0;
  int bandsize;
  switch (ch)
    {
    case 'i':
      get1("Error reading color.\n");
      currentcolor = seqcolor(ch);
      get1("Error reading compression mode!\n");
      c = ch;
      get1("Error reading bpp!\n");
      if (ch != pstate.bpp)
	{
	  fprintf(stderr, "Warning! Color depth altered by ESC i.\n");
	  pstate.bpp=ch;
	}
      if (pstate.bpp > 2)
	fprintf(stderr, "Warning! Excessively deep color detected.\n");
      if (pstate.bpp == 0)
	fprintf(stderr, "Warning! Zero bit pixel depth detected.\n");
      get2("Error reading number of horizontal dots!\n");
      n = (unsigned) sh * 8 / pstate.bpp;
      get2("Error reading number of vertical dots!\n");
      m = (unsigned) sh;
      density = pstate.horizontal_spacing;
      break;
    case '.':
      get1("Error reading compression mode!\n");
      c=ch;
      if (c > 2)
	{
	  fprintf(stderr,"Warning!  Unknown compression mode.\n");
	  break;
	}
      get1("Error reading vertical density!\n");
      /* What should we do with the vertical density here??? */
      get1("Error reading horizontal density!\n");
      density=3600/ch;
      get1("Error reading number of vertical dots!\n");
      m=ch;
      get2("Error reading number of horizontal dots!\n");
      n=sh;
      currentcolor=pstate.current_color;
      break;
    }
  bandsize = m * ((n * pstate.bpp + 7) / 8);
  if (valid_bufsize < bandsize)
    {
      buf = stp_realloc(buf, bandsize);
      valid_bufsize = bandsize;
    }
  switch (c)
    {
    case 0:  /* uncompressed */
      bufsize = bandsize;
      getn(bufsize,"Error reading raster data!\n");
      update_page(buf, bufsize, m, n, currentcolor, density);
      break;
    case 1:  /* run length encoding */
      i = 0;
      while (!eject && (i < bandsize))
	{
	  get1("Error reading global_counter!\n");
	  if (ch < 128)
	    {
	      bufsize = ch + 1;
	      getnoff(bufsize, i, "Error reading RLE raster data!\n");
	    }
	  else
	    {
	      bufsize = 257 - (unsigned int) ch;
	      get1("Error reading compressed RLE raster data!\n");
	      memset(buf + i, ch, bufsize);
	    }
	  i += bufsize;
	}
      if (i != bandsize)
	{
	  fprintf(stderr, "Error decoding RLE data.\n");
	  fprintf(stderr, "Total bufsize %d, expected %d\n",
		  i, bandsize);
	  eject = 1;
	}
      else
	update_page(buf, i, m, n, currentcolor, density);
      break;
    case 2: /* TIFF compression */
      fprintf(stderr, "TIFF mode not yet supported!\n");
      /* FIXME: write TIFF stuff */
      break;
    default: /* unknown */
      fprintf(stderr, "Unknown compression mode %d.\n", c);
      break;
    }
}

static void
parse_escp2_extended(FILE *fp_r)
{
  int unit_base;
  int i;

  get1("Corrupt file.  Incomplete extended command.\n");
  if (eject)
    return;
  get2("Corrupt file.  Error reading buffer size.\n");
  bufsize = sh;
  getn(bufsize, "Corrupt file.  Error reading command payload.\n");
  /* fprintf(stderr,"Command %X bufsize %d.\n",ch,bufsize); */
  switch (ch)
    {
    case 'R':
      if (bufsize == 8 && memcmp(buf, "\0REMOTE1", 8) == 0)
	{
	  int rc1 = 0, rc2 = 0;
	  /* Remote mode 1 */
	  do
	    {
	      get1("Corrupt file.  Error in remote mode.\n");
	      rc1 = ch;
	      get1("Corrupt file.  Error reading remote mode command.\n");
	      rc2 = ch;
	      get2("Corrupt file.  Error reading remote mode command size.\n");
	      bufsize = sh;
	      if (bufsize)
		getn(bufsize, "Corrupt file.  Error reading remote mode command parameters.\n");
	      if (rc1 == 0x1b && rc2 == 0) /* ignore quietly */
		;
	      else
		fprintf(stderr,
			"Remote mode command `%c%c' ignored.\n",
			rc1,rc2);
	    }
	  while (!eject && !(rc1 == 0x1b && rc2 == 0));
	}
      else
	{
/*
	  fprintf(stderr,"Warning!  Commands in unrecognised remote mode %s ignored.\n", buf);
*/
	  do
	    {
	      while((!eject) && (ch!=0x1b))
		get1("Error in remote mode.\n");
	      get1("Error reading remote mode terminator\n");
	    }
	  while ((!eject) && (ch != 0));
	}
      break;
    case 'G': /* select graphics mode */
      /* FIXME: this is supposed to have more side effects */
      pstate.printer_weave = 0;
      pstate.dotsize = 0;
      pstate.bpp = 1;
      break;
    case 'U': /* set page units */
      switch (bufsize)
	{
	case 1:
	  pstate.page_management_units =
	    pstate.absolute_horizontal_units =
	    pstate.relative_horizontal_units =
	    pstate.relative_vertical_units =
	    pstate.horizontal_spacing =
	    pstate.absolute_vertical_units = 3600 / buf[0];
	  if (pstate.page_management_units < 720)
	    pstate.extraskip = 1;
	  fprintf(stderr, "Setting units to 1/%d\n",
		  pstate.absolute_horizontal_units);
	  break;
	case 5:
	  unit_base = buf[4] * 256 + buf[3];
	  pstate.extraskip=1;
	  pstate.page_management_units= unit_base / buf[0];
	  pstate.relative_vertical_units =
	    pstate.absolute_vertical_units = unit_base/buf[1];
	  pstate.relative_horizontal_units =
	    pstate.horizontal_spacing =
	    pstate.absolute_horizontal_units = unit_base / buf[2];
	  fprintf(stderr, "Setting page management units to 1/%d\n",
		  pstate.page_management_units);
	  fprintf(stderr, "Setting vertical units to 1/%d\n",
		  pstate.relative_vertical_units);
	  fprintf(stderr, "Setting horizontal units to 1/%d\n",
		  pstate.relative_horizontal_units);
	  break;
	}
      break;
    case 'i': /* set printer weave mode */
      if (bufsize != 1)
	fprintf(stderr,"Malformed printer weave setting command.\n");
      else
	pstate.printer_weave = buf[0] % 0x30;
      break;
    case 'e': /* set dot size */
      if ((bufsize != 2) || (buf[0] != 0))
	fprintf(stderr,"Malformed dotsize setting command.\n");
      else if (pstate.got_graphics)
	fprintf(stderr,"Changing dotsize while printing not supported.\n");
      else
	{
	  pstate.dotsize = buf[1];
	  if (pstate.dotsize > 0x10)
	    pstate.bpp = 2;
	  else
	    pstate.bpp = 1;
	}
      fprintf(stderr, "Setting dot size to 0x%x (bits %d)\n",
	      pstate.dotsize, pstate.bpp);
      break;
    case 'c': /* set page format */
      if (page)
	{
	  fprintf(stderr,"Setting the page format in the middle of printing a page is not supported.\n");
	  break;
	}
      switch (bufsize)
	{
	case 4:
	  pstate.top_margin = buf[1] * 256 + buf[0];
	  pstate.bottom_margin = buf[3] * 256 + buf[2];
	  break;
	case 8:
	  pstate.top_margin = (buf[3] << 24) +
	    (buf[2] << 16) + (buf[1] << 8) + buf[0];
	  pstate.bottom_margin = (buf[7] << 24) +
	    (buf[6] << 16) + (buf[5] << 8) + buf[4];
	  break;
	default:
	  fprintf(stderr,"Malformed page format.  Ignored.\n");
	}
      pstate.yposition = 0;
      if (pstate.top_margin + pstate.bottom_margin > pstate.page_height)
	pstate.page_height = pstate.top_margin + pstate.bottom_margin;
      fprintf(stderr, "Setting top margin to %d (%.3f)\n",
	      pstate.top_margin,
	      (double) pstate.top_margin / pstate.page_management_units);
      fprintf(stderr, "Setting bottom margin to %d (%.3f)\n",
	      pstate.bottom_margin,
	      (double) pstate.bottom_margin / pstate.page_management_units);
      page = (line_type **)
	stp_zalloc((pstate.bottom_margin - pstate.top_margin) *
		   sizeof(line_type *));
      break;
    case 'V': /* set absolute vertical position */
      i = 0;
      switch (bufsize)
	{
	case 4:
	  i = (buf[2] << 16) + (buf[3]<<24);
	  /* FALLTHROUGH */
	case 2:
	  i += (buf[0]) + (256 * buf[1]);
	  if (pstate.top_margin + i * (pstate.relative_vertical_units /
				       pstate.absolute_vertical_units) >=
	      pstate.yposition)
	    pstate.yposition = i * (pstate.relative_vertical_units /
				    pstate.absolute_vertical_units) +
	      pstate.top_margin;
	  else
	    fprintf(stderr, "Warning: Setting Y position in negative direction ignored\n");
	  break;
	default:
	  fprintf(stderr, "Malformed absolute vertical position set.\n");
	}
      if (pstate.yposition > pstate.bottom_margin - pstate.top_margin)
	{
	  fprintf(stderr,
		  "Warning! Printer head moved past bottom margin.  Dumping output and exiting.\n");
	  eject = 1;
	}
      break;
    case 'v': /* set relative vertical position */
      i = 0;
      switch (bufsize)
	{
	case 4:
	  i = (buf[2] << 16) + (buf[3] << 24);
	  /* FALLTHROUGH */
	case 2:
	  i += (buf[0]) + (256 * buf[1]);
	  if (unweave)
	    i = pstate.nozzles;
	  pstate.yposition+=i;
	  break;
	default:
	  fprintf(stderr,"Malformed relative vertical position set.\n");
	}
      if (pstate.yposition > pstate.bottom_margin - pstate.top_margin)
	{
	  fprintf(stderr,"Warning! Printer head moved past bottom margin.  Dumping output and exiting.\n");
	  eject = 1;
	}
      break;
    case 'K':
      if (bufsize!=2)
	fprintf(stderr,"Malformed monochrome/color mode selection.\n");
      else if (buf[0])
	fprintf(stderr,"Non-zero first byte in monochrome selection command. Ignored.\n");
      else if (buf[0] > 0x02)
	fprintf(stderr,"Unknown color mode 0x%X.\n",buf[1]);
      else
	pstate.monomode = buf[1];

      break;
    case 's':		/* Set print speed */
    case 'm':		/* Set paper type */
      break;
    case 'S': /* set paper dimensions */
      switch (bufsize)
	{
	case 4:
	  i = (buf[1] << 16) + buf[0];
	  fprintf(stderr, "Setting paper width to %d (%.3f)\n", i,
		  (double) i / pstate.page_management_units);
	  i = (buf[3] << 16) + buf[2];
	  fprintf(stderr, "Setting paper height to %d (%.3f)\n", i,
		  (double) i / pstate.page_management_units);
	  break;
	case 8:
	  i = (buf[3] << 24) + (buf[2] << 16) + (buf[1] << 8) + buf[0];
	  fprintf(stderr, "Setting paper width to %d (%.3f)\n", i,
		  (double) i / pstate.page_management_units);
	  i = (buf[7] << 24) + (buf[6] << 16) + (buf[5] << 8) + buf[4];
	  fprintf(stderr, "Setting paper height to %d (%.3f)\n", i,
		  (double) i / pstate.page_management_units);
	  break;
	default:
	  fprintf(stderr, "Invalid set paper dimensions command.\n");
	}
      break;
    case 'D':
      if (bufsize != 4)
	fprintf(stderr, "Malformed set resolution request.\n");
      else
	{
	  int res_base = (256 * buf[1]) + buf[0];
	  pstate.nozzle_separation =
	    pstate.absolute_vertical_units / (res_base / buf[2]);
	  pstate.horizontal_spacing = res_base / buf[3];
	  fprintf(stderr, "Setting nozzle separation to %d\n",
		  pstate.nozzle_separation);
	  fprintf(stderr, "Setting vertical spacing to 1/%d\n",
		  res_base / buf[2]);
	  fprintf(stderr, "Setting horizontal spacing to 1/%d\n",
		  pstate.horizontal_spacing);
	}
      break;
    case 'r': /* select color */
      if (bufsize!=2)
	fprintf(stderr,"Malformed color selection request.\n");
      else
	{
	  sh = 256 * buf[0] + buf[1];
	  if ((buf[1] > 4) || (buf[1] == 3) || (buf[0] > 1) ||
	      (buf[0] && (buf[1]==0)))
	    fprintf(stderr,"Invalid color 0x%X.\n",sh);
	  else
	    pstate.current_color = seqcolor(sh);
	}
      break;
    case '\\': /* set relative horizontal position */
    case '/':
      i = (buf[3] << 8) + buf[2];
      if (pstate.xposition + i < 0)
	{
	  fprintf(stderr,"Warning! Attempt to move to -X region ignored.\n");
	  fprintf(stderr,"   Command:  ESC ( %c %X %X %X %X  Original "
		  "position: %d\n",
		  ch, buf[0], buf[1], buf[2], buf[3], pstate.xposition);
	}
      else  /* FIXME: Where is the right margin??? */
	pstate.xposition+=i;
      break;
    case '$': /* set absolute horizontal position */
      i = (buf[3] << 24) + (buf[2] << 16) + (buf[1] << 8) + buf[0];
      pstate.xposition = i * (pstate.relative_horizontal_units /
			      pstate.absolute_horizontal_units);
      break;
    case 'C': /* set page height */
      switch (bufsize)
	{
	case 2:
	  i = (buf[1] << 8) | buf[0];
	  fprintf(stderr, "Setting page height to %d (%.3f)\n", i,
		  (double) i / pstate.page_management_units);
	  break;
	case 4:
	  i = (buf[3] << 24) + (buf[2] << 16) + (buf[1] << 8) + buf[0];
	  fprintf(stderr, "Setting page height to %d (%.3f)\n", i,
		  (double) i / pstate.page_management_units);
	  break;
	default:
	  fprintf(stderr, "Invalid set page height command.\n");
	}
      break;
    default:
      fprintf(stderr,"Warning: Unknown command ESC ( 0x%X at 0x%08X.\n",
	      ch, global_counter - 5 - bufsize);
    }
}

static void
parse_escp2_command(FILE *fp_r)
{
  get1("Corrupt file.  No command found.\n");
  switch (ch)
    {
    case 1: /* Magic EJL stuff to get USB port working */
      fprintf(stderr,"Ignoring EJL commands.\n");
      do
	{
	  get1("Error reading EJL commands.\n");
	}
      while (!eject && ch != 0x1b);
      if (eject)
	break;
      get1("Expect esc-NULL to close EJL command.\n");
      if (ch != 0x40)
	fprintf(stderr, "Expect esc-NULL to close EJL command.\n");
      break;
    case '@': /* initialize printer */
      if (page)
	eject = 1;
      else
	{
	  pstate.unidirectional = 0;
	  pstate.printer_weave = 0;
	  pstate.dotsize = 0;
	  pstate.bpp = 1;
	  pstate.page_management_units = 360;
	  pstate.relative_horizontal_units = 180;
	  pstate.absolute_horizontal_units = 60;
	  pstate.relative_vertical_units = 360;
	  pstate.absolute_vertical_units = 360;
	  pstate.top_margin = 120;
	  pstate.bottom_margin =
	    pstate.page_height = 22 * 360; /* 22 inches is default ??? */
	  pstate.monomode = 0;
	  pstate.left_edge = INT_MAX;
	  pstate.right_edge = 0;
	  pstate.top_edge = INT_MAX;
	  pstate.bottom_edge = 0;
	}
      break;
    case 'U': /* turn unidirectional mode on/off */
      get1("Error reading unidirectionality.\n");
      if ((ch <= 2) || ((ch >= 0x30) && (ch <= 0x32)))
	pstate.unidirectional=ch;
      break;
    case 'i': /* transfer raster image */
    case '.':
      pstate.got_graphics = 1;
      parse_escp2_data(fp_r);
      break;
    case '\\': /* set relative horizontal position */
      get2("Error reading relative horizontal position.\n");
      if (pstate.xposition + (signed short)sh < 0)
	{
	  fprintf(stderr, "Warning! Move off left of region ignored.\n");
	  fprintf(stderr, "   Command:  ESC %c %X %X   "
		  "Original Position: %d\n",
		  ch, minibuf[0], minibuf[1], pstate.xposition);
	}
      else  /* FIXME: Where is the right margin??? */
	pstate.xposition += (signed short)sh;
      break;
    case '$': /* set absolute horizontal position */
      get2("Error reading absolute horizontal position.\n");
      pstate.xposition = sh * (pstate.relative_horizontal_units /
			       pstate.absolute_horizontal_units);
      break;
    case 0x0:			/* Exit remote mode */
      get2("Error exiting remote mode.\n");
      break;
    case 0x6: /* flush buffers */
      /* Woosh.  Consider them flushed. */
      break;
    case 0x19: /* control paper loading */
      get1("Error reading paper control byte.\n");
      /* paper? */
      break;
    case 'r': /* select printing color */
      get1("Error reading color.\n");
      if ((ch <= 4) && (ch != 3))
	pstate.current_color = seqcolor(ch);
      else
	fprintf(stderr, "Invalid color %d.\n", ch);
      break;
    case '(': /* commands with a payload */
      parse_escp2_extended(fp_r);
      break;
    default:
      fprintf(stderr,"Warning: Unknown command ESC 0x%X at 0x%08X.\n",ch,global_counter-2);
    }
}

void
parse_escp2(FILE *fp_r)
{
  global_counter = 0;

  while ((!eject) && (fread(&ch, 1, 1, fp_r)))
    {
      global_counter++;
      switch (ch)
	{
	case 0xd:		/* carriage return */
	  pstate.xposition = 0;
	  break;
	case 0xc:		/* form feed */
	  eject = 1;
	  break;
	case 0x0:
	  break;
	case 0x1b:		/* Command! */
	  parse_escp2_command(fp_r);
	  break;
	default:
	  fprintf(stderr,
		  "Corrupt file?  No ESC found.  Found: %02X at 0x%08X\n",
		  ch, global_counter-1);
	  break;
	}
    }
}


/* 'reverse_bit_order'
 *
 * reverse the bit order in an array of bytes - does not reverse byte order!
 */
void
reverse_bit_order(unsigned char *buff, int n)
{
  int i;
  unsigned char a;
  if (!n) return; /* nothing to do */

  for (i= 0; i<n; i++) {
    a= buff[i];
    buff[i]=
      (a & 0x01) << 7 |
      (a & 0x02) << 5 |
      (a & 0x04) << 3 |
      (a & 0x08) << 1 |
      (a & 0x10) >> 1 |
      (a & 0x20) >> 3 |
      (a & 0x40) >> 5 |
      (a & 0x80) >> 7;
  }
}

/* 'rle_decode'
 *
 * run-length-decodes a given buffer of height "n"
 * and stores the result in the same buffer
 * not exceeding a size of "max" bytes.
 */
int
rle_decode(unsigned char *inbuf, int n, int max)
{
  unsigned char outbuf[1440*3];
  signed char *ib= (signed char *)inbuf;
  signed char cnt;
  int num;
  int i= 0, j;
  int o= 0;

#ifdef DEBUG_RLE
  fprintf(stderr,"input: %d\n",n);
#endif
  if (n<=0) return 0;
  if (max>1440*3) max= 1440*3; /* FIXME: this can be done much better! */

  while (i<n && o<max) {
    cnt= ib[i];
    if (cnt<0) {
      /* cnt identical bytes */
      /* fprintf(stderr,"rle 0x%02x = %4d = %4d\n",cnt&0xff,cnt,1-cnt); */
      num= 1-cnt;
      /* fprintf (stderr,"+%6d ",num); */
      for (j=0; j<num && o+j<max; j++) outbuf[o+j]= inbuf[i+1];
      o+= num;
      i+= 2;
    } else {
      /* cnt individual bytes */
      /* fprintf(stderr,"raw 0x%02x = %4d = %4d\n",cnt&0xff,cnt,cnt + 1); */
      num= cnt+1;
      /* fprintf (stderr,"*%6d ",num); */
      for (j=0; j<num && o+j<max; j++) outbuf[o+j]= inbuf[i+j+1];
      o+= num;
      i+= num+1;
    }
  }
  if (o>=max) {
    fprintf(stderr,"Warning: rle decompression exceeds output buffer.\n");
    return 0;
  }
  /* copy decompressed data to inbuf: */
  memset(inbuf,0,max-1);
  memcpy(inbuf,outbuf,o);
#ifdef DEBUG_RLE
   fprintf(stderr,"output: %d\n",o);
#endif
  return o;
}

void
parse_canon(FILE *fp_r)
{

  int m=0;
  int currentcolor,currentbpp,density,l_eject;
  int cmdcounter;
  int delay_c=0, delay_m=0, delay_y=0, delay_C=0,
    delay_M=0, delay_Y=0, delay_K=0, currentdelay=0;

  global_counter=0;

  page= 0;
  l_eject=pstate.got_graphics=currentbpp=currentcolor=density=0;
  while ((!l_eject)&&(fread(&ch,1,1,fp_r))){
    global_counter++;
   if (ch==0xd) { /* carriage return */
     pstate.xposition=0;
#ifdef DEBUG_CANON
     fprintf(stderr,"<  ");
#endif
     continue;
   }
   if (ch==0xc) { /* form feed */
     l_eject=1;
     continue;
   }
   if (ch=='B') {
     fgets((char *)buf,sizeof(buf),fp_r);
     global_counter+= strlen((char *)buf);
     if (!strncmp((char *)buf,"JLSTART",7)) {
       while (strncmp((char *)buf,"BJLEND",6)) {
	 fgets((char *)buf,sizeof(buf),fp_r);
	 global_counter+= strlen((char *)buf);
	 fprintf(stderr,"got BJL-plaintext-command %s",buf);
       }
     } else {
       fprintf(stderr,"Error: expected BJLSTART but got B%s",buf);
     }
     global_counter= ftell(fp_r);
     continue;
   }
   if (ch!=0x1b) {
     fprintf(stderr,"Corrupt file?  No ESC found.  Found: %02X at 0x%08X\n",
	     ch,global_counter-1);
     continue;
   }
   get1("Corrupt file.  No command found.\n");
   /* fprintf(stderr,"Got a %X.\n",ch); */
   switch (ch) {
   case '[': /* 0x5b initialize printer */
     get1("Error reading CEM-code.\n");
     cmdcounter= global_counter;
     get2("Error reading CEM-data size.\n");
     getn(sh,"Error reading CEM-data.\n");

     if (ch=='K') /* 0x4b */ {
       if (sh!=2 || buf[0]!=0x00 ) {
	 fprintf(stderr,"Error initializing printer with ESC [ K\n");
	 l_eject=1;
	 continue;
       }
       if (page) {
	 l_eject=1;
	 continue;
       } else {
	 pstate.unidirectional=0;
	 pstate.printer_weave=0;
	 pstate.dotsize=0;
	 pstate.bpp=1;
	 pstate.page_management_units=360;
	 pstate.relative_horizontal_units=180;
	 pstate.absolute_horizontal_units=60;
	 pstate.relative_vertical_units=360;
	 pstate.absolute_vertical_units=360;
	 pstate.top_margin=120;
	 pstate.bottom_margin=
	   pstate.page_height=22*360; /* 22 inches is default ??? */
	 pstate.monomode=0;
	 pstate.xposition= 0;
	 pstate.yposition= 0;
	 pstate.left_edge = INT_MAX;
	 pstate.right_edge = 0;
	 pstate.top_edge = INT_MAX;
	 pstate.bottom_edge = 0;
	 fprintf(stderr,"canon: init printer\n");
       }
     } else {
       fprintf(stderr,"Warning: Unknown command ESC %c 0x%X at 0x%08X.\n",
	       0x5b,ch,global_counter);
     }
     break;

   case '@': /* 0x40 */
     l_eject=1;
     break;

   case '(': /* 0x28 */
     get1("Corrupt file.  Incomplete extended command.\n");
     cmdcounter= global_counter;
     get2("Corrupt file.  Error reading buffer size.\n");
     bufsize=sh;
     getn(bufsize,"Corrupt file.  Error reading data buffer.\n");

     switch(ch) {
/* Color Codes:
   color    Epson1  Epson2   Sequential
   Black    0       0        0 K
   Magenta  1       1        1 M
   Cyan     2       2        2 C
   Yellow   4       4        3 Y
   L.Mag.   17      257      4 m
   L.Cyan   18      258      5 c
   L.Yellow NA      NA       6 y
 */
     case 'A': /* 0x41 - transfer graphics data */
       switch (*buf) {
       case 'K': currentcolor= 0; currentdelay= delay_K; break;
       case 'M': currentcolor= 1; currentdelay= delay_M; break;
       case 'C': currentcolor= 2; currentdelay= delay_C; break;
       case 'Y': currentcolor= 3; currentdelay= delay_Y; break;
       case 'm': currentcolor= 4; currentdelay= delay_m; break;
       case 'c': currentcolor= 5; currentdelay= delay_c; break;
       case 'y': currentcolor= 6; currentdelay= delay_y; break;
       default:
	 fprintf(stderr,"Error: unsupported color type 0x%02x.\n",*buf);
	 /* exit(-1); */
       }
       pstate.current_color= currentcolor;
       m= rle_decode(buf+1,bufsize-1,sizeof(buf)-1);
       /* reverse_bit_order(buf+1,m); */
       pstate.yposition+= currentdelay;
       if (m) update_page(buf+1,m,1,(m*8)/pstate.bpp,currentcolor,
			  pstate.absolute_horizontal_units);
       pstate.yposition-= currentdelay;
#ifdef DEBUG_CANON
       fprintf(stderr,"%c:%d>%d  ",*buf,sh-1,m);
#endif
       break;
     case 'a': /* 0x61 - turn something on/off */
       break;
     case 'b': /* 0x62 - turn something else on/off */
       break;
     case 'c': /* 0x63 - some information about the print job */
       break;
     case 'd': /* 0x64 - set resolution */
       if (page) {
	 fprintf(stderr,"Setting the page format in the middle of printing "
		 "a page is not supported.\n");
	 exit(-1);
       }
       pstate.relative_vertical_units=
	 pstate.absolute_vertical_units=
	 buf[1]+256*buf[0];
       pstate.relative_horizontal_units=
	 pstate.absolute_horizontal_units=
	 buf[3]+256*buf[2];
       pstate.bottom_margin= pstate.relative_vertical_units* 22;
       /* FIXME: replace with real page height */
       fprintf(stderr,"canon: res is %d x %d dpi\n",
	       pstate.relative_horizontal_units,
	       pstate.relative_vertical_units);

       page= (line_type **) stp_zalloc(pstate.bottom_margin *
				       sizeof(line_type *));
       break;
     case 'e': /* 0x65 - vertical head movement */
       pstate.yposition+= (buf[1]+256*buf[0]);
#ifdef DEBUG_CANON
       fprintf(stderr,"\n");
#endif
       break;
     case 'l': /* 0x6c - some more information about the print job*/
       break;
     case 'm': /* 0x6d - used printheads and other things */
       break;
     case 'p': /* 0x70 - set printable area */
       break;
     case 'q': /* 0x71 - turn yet something else on/off */
       break;
     case 't': /* 0x74 - contains bpp and line delaying*/
       pstate.bpp= buf[0];
       fprintf(stderr,"canon: using %d bpp\n",pstate.bpp);
       if (buf[1]&0x04) {
	 delay_y= 0;
	 delay_m= 0;
	 delay_c= 0;
	 delay_Y= 0;
	 delay_M= delay_Y+112;
	 delay_C= delay_M+112;
	 delay_K= delay_C+112;
	 fprintf(stderr,"canon: using line delay code\n");
       }
       break;

     default:
       fprintf(stderr,"Warning: Unknown command ESC ( 0x%X at 0x%08X.\n",
	       ch,global_counter);
     }
     break;

   default:
     fprintf(stderr,"Warning: Unknown command ESC 0x%X at 0x%08X.\n",
	     ch,global_counter-2);
   }
 }
}

int
main(int argc,char *argv[])
{

  int arg;
  char *s;
  char *UNPRINT;
  FILE *fp_r, *fp_w;
  int force_extraskip = -1;
  int no_output = 0;
  int all_black = 0;

  unweave = 0;
  pstate.nozzle_separation = 6;
  fp_r = fp_w = NULL;
  for (arg = 1; arg < argc; arg++)
    {
      if (argv[arg][0] == '-')
	{
	  switch (argv[arg][1])
	    {
	    case 0:
	      if (fp_r)
		fp_w = stdout;
	      else
		fp_r = stdin;
	      break;
	    case 'h':
	      fprintf(stderr, "Usage: %s [-m mask] [-n nozzle_sep] [-s extra] [-b] [-q] [-Q] [-M] [-u] [in [out]]\n", argv[0]);
	      fprintf(stderr, "        -m mask       Color mask to unprint\n");
	      fprintf(stderr, "        -n nozzle_sep Nozzle separation in vertical units for old printers\n");
	      fprintf(stderr, "        -s extra      Extra feed requirement for old printers (typically 1)\n");
	      fprintf(stderr, "        -b            Unprint everything in black\n");
	      fprintf(stderr, "        -q            Don't produce output\n");
	      fprintf(stderr, "        -Q            Assume quadtone inks\n");
	      fprintf(stderr, "        -M            Assume MIS quadtone inks\n");
	      fprintf(stderr, "        -u            Unweave\n");
	      return 1;
	    case 'm':
	      if (argv[arg][2])
		{
		  s = argv[arg] + 2;
		}
	      else
		{
		  if (argc <= arg + 1)
		    {
		      fprintf(stderr, "Missing color mask\n");
		      exit(-1);
		    }
		  else
		    {
		      s = argv[++arg];
		    }
		}
	      if (!sscanf(s, "%x", &color_mask))
		{
		  fprintf(stderr,"Error parsing mask\n");
		  exit(-1);
		}
	      break;
	    case 'n':
	      if (argv[arg][2])
		{
		  s = argv[arg] + 2;
		}
	      else
		{
		  if (argc <= arg + 1)
		    {
		      fprintf(stderr, "Missing nozzle separation\n");
		      exit(-1);
		    }
		  else
		    {
		      s = argv[++arg];
		    }
		}
	      if (!sscanf(s, "%d", &pstate.nozzle_separation))
		{
		  fprintf(stderr,"Error parsing nozzle separation\n");
		  exit(-1);
		}
	      break;
	    case 's':
	      if (argv[arg][2])
		{
		  s = argv[arg] + 2;
		}
	      else
		{
		  if (argc <= arg + 1)
		    {
		      fprintf(stderr,"Missing extra skip\n");
		      exit(-1);
		    }
		  else
		    {
		      s = argv[++arg];
		    }
		}
	      if (!sscanf(s, "%d", &force_extraskip))
		{
		  fprintf(stderr, "Error parsing extra skip\n");
		  exit(-1);
		}
	      break;
	    case 'b':
	      all_black = 1;
	      break;
	    case 'q':
	      no_output = 1;
	      break;
	    case 'Q':
	      pstate.quadtone = QT_QUAD;
	      break;
	    case 'M':
	      pstate.quadtone = QT_MIS;
	      break;
	    case 'u':
	      unweave = 1;
	      break;
	    }
	}
      else
	{
	  if (fp_r)
	    {
	      if (!(fp_w = fopen(argv[arg],"w")))
		{
		  perror("Error opening ouput file");
		  exit(-1);
		}
	    }
	  else
	    {
	      if (!(fp_r = fopen(argv[arg],"r")))
		{
		  perror("Error opening input file");
		  exit(-1);
		}
	    }
	}
    }
  if (!fp_r)
    fp_r = stdin;
  if (!fp_w)
    fp_w = stdout;

  if (unweave) {
    pstate.nozzle_separation = 1;
  }
  pstate.nozzles = 96;
  buf = stp_malloc(256 * 256);
  valid_bufsize = 256 * 256;

  UNPRINT = getenv("UNPRINT");
  if ((UNPRINT)&&(!strcmp(UNPRINT,"canon")))
    {
      if (force_extraskip > 0)
	pstate.extraskip = force_extraskip;
      else
	pstate.extraskip = 1;
      parse_canon(fp_r);
    }
  else
    {
      if (force_extraskip > 0)
	pstate.extraskip = force_extraskip;
      else
	pstate.extraskip = 2;
      parse_escp2(fp_r);
    }
  fprintf(stderr,"Done reading.\n");
  write_output(fp_w, no_output, all_black);
  fclose(fp_w);
  fprintf(stderr,"Image dump complete.\n");

  return(0);
}
