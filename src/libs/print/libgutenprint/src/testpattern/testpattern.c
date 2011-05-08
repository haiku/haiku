/*
 * "$Id: testpattern.c,v 1.59 2011/04/01 01:21:12 rlk Exp $"
 *
 *   Test pattern generator for Gimp-Print
 *
 *   Copyright 2001 Robert Krawitz <rlk@alum.mit.edu>
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
 * This sample program may be used to generate test patterns.  It also
 * serves as an example of how to use the gimp-print API.
 *
 * As the purpose of this program is to allow fine grained control over
 * the output, it uses the raw CMYK output type.  This feeds 16 bits each
 * of CMYK to the driver.  This mode performs no correction on the data;
 * it passes it directly to the dither engine, performing no color,
 * density, gamma, etc. correction.  Most programs will use one of the
 * other modes (RGB, density and gamma corrected 8-bit CMYK, grayscale, or
 * black and white).
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "testpattern.h"
#include <gutenprint/gutenprint-intl.h>
#include <errno.h>

extern int yyparse(void);

static const char *Image_get_appname(stp_image_t *image);
static void Image_conclude(stp_image_t *image);
static void Image_init(stp_image_t *image);
static stp_image_status_t Image_get_row(stp_image_t *image,
					unsigned char *data,
					size_t byte_limit, int row);
static int Image_height(stp_image_t *image);
static void Image_reset(stp_image_t *image);
static int Image_width(stp_image_t *image);
static int Image_is_valid = 0;
static stp_image_t theImage =
{
  Image_init,
  Image_reset,
  Image_width,
  Image_height,
  Image_get_row,
  Image_get_appname,
  Image_conclude,
  NULL
};
stp_vars_t *global_vars = NULL;

double global_levels[STP_CHANNEL_LIMIT];
double global_gammas[STP_CHANNEL_LIMIT];
double global_gamma;
int global_steps;
double global_ink_limit;
int global_noblackline;
int global_printer_width;
int global_printer_height;
int global_band_height;
int global_n_testpatterns;
char *global_printer = NULL;
double global_density;
double global_xtop;
double global_xleft;
size_mode_t global_size_mode;
double global_hsize;
double global_vsize;
const char *global_image_type;
int global_bit_depth;
int global_channel_depth;
int global_invert_data = 0;
int global_use_raw_cmyk;
int global_did_something;
int global_noscale = 0;
int global_suppress_output = 0;
int global_quiet = 0;
int global_fail_verify_ok = 0;
char *global_output = NULL;
FILE *output = NULL;
int write_to_process = 0;
int start_job = 0;
int end_job = 0;
int passes = 0;
int failures = 0;
int skipped = 0;

static testpattern_t *static_testpatterns;

static size_t
c_strlen(const char *s)
{
  return strlen(s);
}

static char *
c_strndup(const char *s, int n)
{
  char *ret;
  if (!s || n < 0)
    {
      ret = stp_malloc(1);
      ret[0] = 0;
      return ret;
    }
  else
    {
      ret = stp_malloc(n + 1);
      memcpy(ret, s, n);
      ret[n] = 0;
      return ret;
    }
}

char *
c_strdup(const char *s)
{
  char *ret;
  if (!s)
    {
      ret = stp_malloc(1);
      ret[0] = 0;
      return ret;
    }
  else
    return c_strndup(s, c_strlen(s));
}

static void
clear_testpattern(testpattern_t *p)
{
  int i;
  for (i = 0; i < STP_CHANNEL_LIMIT; i++)
    {
      p->d.pattern.mins[i] = 0;
      p->d.pattern.vals[i] = 0;
      p->d.pattern.gammas[i] = 1;
      p->d.pattern.levels[i] = 0;
    }
}
  

testpattern_t *
get_next_testpattern(void)
{
  static int internal_n_testpatterns = 0;
  if (global_n_testpatterns == -1)
    {
      static_testpatterns = stp_malloc(sizeof(testpattern_t));
      global_n_testpatterns = 0;
      internal_n_testpatterns = 1;
      clear_testpattern(&(static_testpatterns[0]));
      return &(static_testpatterns[0]);
    }
  else if (global_n_testpatterns + 1 >= internal_n_testpatterns)
    {
      internal_n_testpatterns *= 2;
      static_testpatterns =
	stp_realloc(static_testpatterns,
		    internal_n_testpatterns * sizeof(testpattern_t));
    }
  global_n_testpatterns++;
  clear_testpattern(&(static_testpatterns[global_n_testpatterns]));
  return &(static_testpatterns[global_n_testpatterns]);
}

static void
writefunc(void *file, const char *buf, size_t bytes)
{
  FILE *prn = (FILE *)file;
  if (! file)
    return;
  if (!global_suppress_output || (file == stderr))
    {
      fwrite(buf, 1, bytes, prn);
    }
}

void
close_output(void)
{
  if (output && output != stdout)
    {
      if (write_to_process)
	(void) pclose(output);
      else
	(void) fclose(output);
      output = NULL;
    }
}

static void
initialize_global_parameters(void)
{
  int i;
  for (i = 0; i < STP_CHANNEL_LIMIT; i++)
    {
      global_levels[i] = 1.0;
      global_gammas[i] = 1.0;
    }
  global_gamma = 1.0;
  global_steps = 256;
  global_ink_limit = 1.0;
  global_noblackline = 0;
  global_printer_width = 0;
  global_printer_height = 0;
  global_band_height = 0;
  global_n_testpatterns = -1;
  if (global_printer)
    free(global_printer); /* Allocated with strdup() */
  global_printer = NULL;
  global_density = 1.0;
  global_xtop = 0;
  global_xleft = 0;
  global_hsize = 1.0;
  global_vsize = 1.0;
  global_bit_depth = 16;
  global_channel_depth = 0;
  global_image_type = "CMYK";
  global_use_raw_cmyk = 0;
  global_did_something = 0;
  global_invert_data = 0;
  if (static_testpatterns)
    stp_free(static_testpatterns);
  static_testpatterns = NULL;
  if (global_vars)
    stp_vars_destroy(global_vars);
  global_vars = NULL;
  if (global_printer)
    free(global_printer); /* Allocated with strdup() */
  global_printer = NULL;
  start_job = 0;
  end_job = 0;
}

static int
do_print(void)
{
  int status = 0;
  stp_vars_t *v;
  const stp_printer_t *the_printer;
  int left, right, top, bottom;
  int x, y;
  int width, height;
  int retval;
  stp_parameter_list_t params;
  int count;
  int i;
  char tmp[32];

  initialize_global_parameters();
  global_vars = stp_vars_create();
  stp_set_outfunc(global_vars, writefunc);
  stp_set_errfunc(global_vars, writefunc);
  stp_set_outdata(global_vars, stdout);
  stp_set_errdata(global_vars, stderr);

  setlocale(LC_ALL, "C");
  retval = yyparse();
  setlocale(LC_ALL, "");
  if (retval)
    return retval + 1;

  if (!global_did_something)
    return 1;

  v = stp_vars_create();
  the_printer = stp_get_printer_by_driver(global_printer);
  if (!the_printer)
    {
      int j;
      fprintf(stderr, "Unknown printer %s\nValid printers are:\n",
	      global_printer);
      for (j = 0; j < stp_printer_model_count(); j++)
	{
	  the_printer = stp_get_printer_by_index(j);
	  fprintf(stderr, "%-16s%s\n", stp_printer_get_driver(the_printer),
		  stp_printer_get_long_name(the_printer));
	}
      return 2;
    }
  if (global_output)
    {
      if (strcmp(global_output, "-") == 0)
	output = stdout;
      else if (strcmp(global_output, "") == 0)
	output = NULL;
      else if (global_output[0] == '|')
	{
	  close_output();
	  write_to_process = 1;
	  output = popen(global_output+1, "w");
	  if (! output)
	    {
	      fprintf(stderr, "popen '%s' failed: %s\n", global_output, strerror(errno));
	      output = NULL;
	    }
	  free(global_output);
	  global_output = NULL;
	}
      else
	{
	  close_output();
	  output = fopen(global_output, "wb");
	  if (! output)
	    {
	      fprintf(stderr, "Create %s failed: %s\n", global_output, strerror(errno));
	      output = NULL;
	    }
	  free(global_output);
	  global_output = NULL;
	}
    }
  stp_set_printer_defaults(v, the_printer);
  stp_set_outfunc(v, writefunc);
  stp_set_errfunc(v, writefunc);
  stp_set_outdata(v, output);
  stp_set_errdata(v, stderr);
  stp_set_string_parameter(v, "InputImageType", global_image_type);
  sprintf(tmp, "%d", global_bit_depth);
  stp_set_string_parameter(v, "ChannelBitDepth", tmp);
  if (strcmp(global_image_type, "Raw") == 0)
    {
      sprintf(tmp, "%d", global_channel_depth);
      stp_set_string_parameter(v, "RawChannels", tmp);
    }
  stp_set_float_parameter(v, "Density", global_density);
  stp_set_string_parameter(v, "Quality", "None");
  stp_set_string_parameter(v, "ImageType", "None");

  params = stp_get_parameter_list(v);
  count = stp_parameter_list_count(params);
  for (i = 0; i < count; i++)
    {
      const stp_parameter_t *p = stp_parameter_list_param(params, i);
      if (p->p_type == STP_PARAMETER_TYPE_STRING_LIST)
	{
	  const char *val = stp_get_string_parameter(global_vars, p->name);
	  if (val && strlen(val) > 0)
	    {
	      stp_set_string_parameter(v, p->name, val);
	    }
	}
      else if (p->p_type == STP_PARAMETER_TYPE_INT &&
	       stp_check_int_parameter(global_vars, p->name, STP_PARAMETER_ACTIVE))
	{
	  int val = stp_get_int_parameter(global_vars, p->name);
	  stp_set_int_parameter(v, p->name, val);
	}
      else if (p->p_type == STP_PARAMETER_TYPE_BOOLEAN &&
	       stp_check_boolean_parameter(global_vars, p->name, STP_PARAMETER_ACTIVE))
	{
	  int val = stp_get_boolean_parameter(global_vars, p->name);
	  stp_set_boolean_parameter(v, p->name, val);
	}
      else if (p->p_type == STP_PARAMETER_TYPE_CURVE &&
	       stp_check_curve_parameter(global_vars, p->name, STP_PARAMETER_ACTIVE))
	{
	  const stp_curve_t *val = stp_get_curve_parameter(global_vars, p->name);
	  stp_set_curve_parameter(v, p->name, val);
	}
      else if (p->p_type == STP_PARAMETER_TYPE_DOUBLE &&
	       stp_check_float_parameter(global_vars, p->name, STP_PARAMETER_ACTIVE))
	{
	  double val = stp_get_float_parameter(global_vars, p->name);
	  stp_set_float_parameter(v, p->name, val);
	}
    }
  stp_set_page_width(v, stp_get_page_width(global_vars));
  stp_set_page_height(v, stp_get_page_height(global_vars));
  stp_parameter_list_destroy(params);
  if (stp_check_string_parameter(v, "PageSize", STP_PARAMETER_ACTIVE) &&
      !strcmp(stp_get_string_parameter(v, "PageSize"), "Auto"))
    {
      stp_parameter_t desc;
      stp_describe_parameter(v, "PageSize", &desc);
      if (desc.p_type == STP_PARAMETER_TYPE_STRING_LIST)
	stp_set_string_parameter(v, "PageSize", desc.deflt.str);
      stp_parameter_description_destroy(&desc);
    }
  stp_set_printer_defaults_soft(v, the_printer);

  stp_get_imageable_area(v, &left, &right, &bottom, &top);
  stp_describe_resolution(v, &x, &y);
  if (x < 0)
    x = 300;
  if (y < 0)
    y = 300;

  width = right - left;
  height = bottom - top;

  switch (global_size_mode)
    {
    case SIZE_PT:
      top += (int) (global_xtop + .5);
      left += (int) (global_xleft + .5);
      width = (int) (global_hsize + .5);
      height = (int) (global_vsize + .5);
      break;
    case SIZE_IN:
      top += (int) ((global_xtop * 72) + .5);
      left += (int) ((global_xleft * 72) + .5);
      width = (int) ((global_hsize * 72) + .5);
      height = (int) ((global_vsize * 72) + .5);
      break;
    case SIZE_MM:
      top += (int) ((global_xtop * 72 / 25.4) + .5);
      left += (int) ((global_xleft * 72 / 25.4) + .5);
      width = (int) ((global_hsize * 72 / 25.4) + .5);
      height = (int) ((global_vsize * 72 / 25.4) + .5);
      break;
    case SIZE_RELATIVE:
    default:
      top += height * global_xtop;
      left += width * global_xleft;
      width *= global_hsize;
      height *= global_vsize;
      break;
    }
  stp_set_width(v, width);
  stp_set_height(v, height);
#if 0
  width = (width / global_steps) * global_steps;
  height = (height / global_n_testpatterns) * global_n_testpatterns;
#endif
  if (global_steps > width)
    global_steps = width;

  global_printer_width = width * x / 72;
  global_printer_height = height * y / 72;

  global_band_height = global_printer_height / global_n_testpatterns;
  if (global_band_height == 0)
    global_band_height = 1;
  stp_set_left(v, left);
  stp_set_top(v, top);

  stp_merge_printvars(v, stp_printer_get_defaults(the_printer));
  if (stp_verify(v))
    {
      if (start_job)
	{
	  stp_start_job(v, &theImage);
	  start_job = 0;
	}
      if (stp_print(v, &theImage) != 1)
	{
	  if (!global_quiet)
	    fputs("FAILED", stderr);
	  failures++;
	  status = 2;
	}
      else
	passes++;
      if (end_job)
	{
	  stp_end_job(v, &theImage);
	  end_job = 0;
	}
    }
  else
    {
      if (! global_fail_verify_ok)
	{
	  if (!global_quiet)
	    fputs("FAILED", stderr);
	  failures++;
	  status = 2;
	}
      else
	{
	  if (!global_quiet)
	    fprintf(stderr, "(skipped)");
	  skipped++;
	}
    }
  if (!global_quiet)
    fputc('\n', stderr);
  stp_vars_destroy(v);
  stp_free(static_testpatterns);
  static_testpatterns = NULL;
  return status;
}

int
main(int argc, char **argv)
{
  int c;
  int status;
  int global_status = 0;
  while (1)
    {
      c = getopt(argc, argv, "nqy");
      if (c == -1)
	break;
      switch (c)
	{
	case 'n':
	  global_suppress_output = 1;
	  break;
	case 'q':
	  global_quiet = 1;
	  break;
	case 'y':
	  global_fail_verify_ok = 1;
	  break;
	default:
	  break;
	}
    }

  stp_init();
  output = stdout;
  while (1)
    {
      status = do_print();
      if (status == 1)
	break;
      else if (status != 0)
	global_status = 1;
    }
  close_output();
  if (!global_quiet)
    fprintf(stderr, "%d pass, %d fail, %d skipped\n", passes, failures, skipped);
  return global_status;
}

static void
invert_data(unsigned char *data, size_t byte_depth)
{
  int i;
  size_t total_bytes;
  total_bytes = global_printer_width * global_channel_depth * byte_depth;
  for (i = 0; i < total_bytes; i++)
    data[i] = 0xff ^ data[i];
}

/*
 * Emulate templates with macros -- rlk 20031014
 */

#define FILL_BLACK_FUNCTION(T, bits)					\
static void								\
fill_black_##bits(unsigned char *data, size_t len, size_t scount)	\
{									\
  int i;								\
  T *s_data = (T *) data;						\
  unsigned black_val = global_ink_limit * ((1 << bits) - 1);		\
  unsigned blocks = (len / scount) * scount;				\
  unsigned extra = len - blocks;					\
  if (strcmp(global_image_type, "Raw") == 0)				\
    {									\
      for (i = 0; i < (len / scount) * scount; i++)			\
	{								\
	  memset(s_data, 0, sizeof(T) * global_channel_depth);		\
	  s_data[0] = black_val;					\
	  if (global_channel_depth == 3)				\
	    {								\
	      s_data[1] = black_val;					\
	      s_data[2] = black_val;					\
	    }								\
	  else if (global_channel_depth == 5)				\
	    {								\
	      s_data[2] = black_val;					\
	      s_data[4] = black_val;					\
	    }								\
	  s_data += global_channel_depth;				\
	}								\
      memset(s_data, 0xff, sizeof(T) * extra *				\
	     global_channel_depth);					\
    }									\
  else if (strcmp(global_image_type, "RGB") == 0)			\
    {									\
      for (i = 0; i < (len / scount) * scount; i++)			\
	{								\
	  memset(s_data, 0, sizeof(T) * 3);				\
	  s_data += 3;							\
	}								\
      memset(s_data, 0xff, sizeof(T) * extra * 3);			\
    }									\
  else if (strcmp(global_image_type, "CMY") == 0)			\
    {									\
      for (i = 0; i < (len / scount) * scount; i++)			\
	{								\
	  s_data[0] = black_val;					\
	  s_data[1] = black_val;					\
	  s_data[2] = black_val;					\
	  s_data += 3;							\
	}								\
      memset(s_data, 0, sizeof(T) * extra * 3);				\
    }									\
  else if (strcmp(global_image_type, "CMYK") == 0)			\
    {									\
      for (i = 0; i < (len / scount) * scount; i++)			\
	{								\
	  memset(s_data, 0, sizeof(T) * 4);				\
	  s_data[3] = black_val;					\
	  s_data += 4;							\
	}								\
      memset(s_data, 0, sizeof(T) * extra * 4);				\
    }									\
  else if (strcmp(global_image_type, "KCMY") == 0)			\
    {									\
      for (i = 0; i < (len / scount) * scount; i++)			\
	{								\
	  memset(s_data, 0, sizeof(T) * 4);				\
	  s_data[0] = black_val;					\
	  s_data += 4;							\
	}								\
      memset(s_data, 0, sizeof(T) * extra * 4);				\
    }									\
  else if (strcmp(global_image_type, "Grayscale") == 0)			\
    {									\
      for (i = 0; i < (len / scount) * scount; i++)			\
	{								\
	  memset(s_data, 0, sizeof(T) * 1);				\
	  s_data[0] = black_val;					\
	  s_data += 1;							\
	}								\
      memset(s_data, 0, sizeof(T) * extra);				\
    }									\
  else if (strcmp(global_image_type, "Whitescale") == 0)		\
    {									\
      for (i = 0; i < (len / scount) * scount; i++)			\
	{								\
	  memset(s_data, 0, sizeof(T) * 1);				\
	  s_data += 1;							\
	}								\
      memset(s_data, 0xff, sizeof(T) * extra);				\
    }									\
}

FILL_BLACK_FUNCTION(unsigned short, 16)
FILL_BLACK_FUNCTION(unsigned char, 8)

static void
fill_black(unsigned char *data, size_t len, size_t scount, size_t bytes)
{
  switch (bytes)
    {
    case 1:
      fill_black_8(data, len, scount);
      break;
    case 2:
      fill_black_16(data, len, scount);
      break;
    }
  if (global_invert_data)
    invert_data(data, bytes);
}

#define FILL_WHITE_FUNCTION(T, bits)					\
static void								\
fill_white_##bits(unsigned char *data, size_t len, size_t scount)	\
{									\
  int i;								\
  T *s_data = (T *) data;						\
  if (strcmp(global_image_type, "Raw") == 0)				\
    {									\
      for (i = 0; i < len; i++)						\
	{								\
	  memset(s_data, 0, sizeof(T) * global_channel_depth);		\
	  s_data += global_channel_depth;				\
	}								\
    }									\
  else if (strcmp(global_image_type, "RGB") == 0)			\
    {									\
      for (i = 0; i < len; i++)						\
	{								\
	  memset(s_data, 0xff, sizeof(T) * 3);				\
	  s_data += 3;							\
	}								\
    }									\
  else if (strcmp(global_image_type, "CMY") == 0)			\
    {									\
      for (i = 0; i < len; i++)						\
	{								\
	  memset(s_data, 0, sizeof(T) * 3);				\
	  s_data += 3;							\
	}								\
    }									\
  else if (strcmp(global_image_type, "CMYK") == 0)			\
    {									\
      for (i = 0; i < len; i++)						\
	{								\
	  memset(s_data, 0, sizeof(T) * 4);				\
	  s_data += 4;							\
	}								\
    }									\
  else if (strcmp(global_image_type, "KCMY") == 0)			\
    {									\
      for (i = 0; i < len; i++)						\
	{								\
	  memset(s_data, 0, sizeof(T) * 4);				\
	  s_data += 4;							\
	}								\
    }									\
  else if (strcmp(global_image_type, "Grayscale") == 0)			\
    {									\
      for (i = 0; i < len; i++)						\
	{								\
	  memset(s_data, 0, sizeof(T) * 1);				\
	  s_data += 1;							\
	}								\
    }									\
  else if (strcmp(global_image_type, "Whitescale") == 0)		\
    {									\
      for (i = 0; i < len; i++)						\
	{								\
	  memset(s_data, 0xff, sizeof(T) * 1);				\
	  s_data += 1;							\
	}								\
    }									\
}

FILL_WHITE_FUNCTION(unsigned short, 16)
FILL_WHITE_FUNCTION(unsigned char, 8)

static void
fill_white(unsigned char *data, size_t len, size_t scount, size_t bytes)
{
  switch (bytes)
    {
    case 1:
      fill_white_8(data, len, scount);
      break;
    case 2:
      fill_white_16(data, len, scount);
      break;
    }
  if (global_invert_data)
    invert_data(data, bytes);
}

#define FILL_GRID_FUNCTION(T, bits)					\
static void								\
fill_grid_##bits(unsigned char *data, size_t len, size_t scount,	\
		 testpattern_t *p)					\
{									\
  int i;								\
  int xlen = (len / scount) * scount;					\
  int errdiv = (p->d.grid.ticks) / (xlen - 1);				\
  int errmod = (p->d.grid.ticks) % (xlen - 1);				\
  int errval  = 0;							\
  int errlast = -1;							\
  int errline  = 0;							\
  T *s_data = (T *) data;						\
  unsigned multiplier = (1 << bits) - 1;				\
									\
  for (i = 0; i < xlen; i++)						\
    {									\
      if (errline != errlast)						\
	{								\
	  errlast = errline;						\
	  s_data[0] = multiplier;					\
	}								\
      else								\
	s_data[0] = 0;							\
      errval += errmod;							\
      errline += errdiv;						\
      if (errval >= xlen - 1)						\
	{								\
	  errval -= xlen - 1;						\
	  errline++;							\
	}								\
      s_data += global_channel_depth;					\
    }									\
}

FILL_GRID_FUNCTION(unsigned short, 16)
FILL_GRID_FUNCTION(unsigned char, 8)

static void
fill_grid(unsigned char *data, size_t len, size_t scount,
	  testpattern_t *p, size_t bytes)
{
  switch (bytes)
    {
    case 1:
      fill_grid_8(data, len, scount, p);
      break;
    case 2:
      fill_grid_16(data, len, scount, p);
      break;
    }
}

#define FILL_COLORS_EXTENDED_FUNCTION(T, bits)				\
static void								\
fill_colors_extended_##bits(unsigned char *data, size_t len,		\
			    size_t scount, testpattern_t *p)		\
{									\
  double mins[STP_CHANNEL_LIMIT];					\
  double vals[STP_CHANNEL_LIMIT];					\
  double gammas[STP_CHANNEL_LIMIT];					\
  int i;								\
  int j;								\
  int k;								\
  int pixels;								\
  int channel_limit =							\
    global_channel_depth <= 7 ? 7 : global_channel_depth;		\
  T *s_data = (T *) data;						\
  unsigned multiplier = global_noscale ? 1 : (1 << bits) - 1;		\
									\
  for (j = 0; j < channel_limit; j++)					\
    {									\
      mins[j] = p->d.pattern.mins[j] == -2 ?				\
	global_levels[j] : p->d.pattern.mins[j];			\
      vals[j] = p->d.pattern.vals[j] == -2 ?				\
	global_levels[j] : p->d.pattern.vals[j];			\
      gammas[j] =							\
	p->d.pattern.gammas[j] * global_gamma * global_gammas[j];	\
      vals[j] -= mins[j];						\
    }									\
  if (scount > len)							\
    scount = len;							\
  pixels = len / scount;						\
  for (i = 0; i < scount; i++)						\
    {									\
      double where = (double) i / ((double) scount - 1);		\
      double val = where;						\
      double xvals[STP_CHANNEL_LIMIT];					\
									\
      for (j = 0; j < channel_limit; j++)				\
	{								\
	  xvals[j] = mins[j] + val * vals[j];				\
	  xvals[j] = pow(xvals[j], gammas[j]);				\
	  xvals[j] *= global_ink_limit * multiplier;			\
	  xvals[j] += 0.9;						\
	}								\
      for (k = 0; k < pixels; k++)					\
	{								\
	  switch (global_channel_depth)					\
	    {								\
	    case 1:							\
	      s_data[0] = xvals[0];					\
	      break;							\
	    case 2:							\
	      s_data[0] = xvals[0];					\
	      s_data[1] = xvals[4];					\
	      break;							\
	    case 3:							\
	      s_data[0] = xvals[1];					\
	      s_data[1] = xvals[2];					\
	      s_data[2] = xvals[3];					\
	      break;							\
	    case 4:							\
	      s_data[0] = xvals[0];					\
	      s_data[1] = xvals[1];					\
	      s_data[2] = xvals[2];					\
	      s_data[3] = xvals[3];					\
	      break;							\
	    case 5:							\
	      s_data[0] = xvals[1];					\
	      s_data[1] = xvals[5];					\
	      s_data[2] = xvals[2];					\
	      s_data[3] = xvals[6];					\
	      s_data[4] = xvals[3];					\
	      break;							\
	    case 6:							\
	      s_data[0] = xvals[0];					\
	      s_data[1] = xvals[1];					\
	      s_data[2] = xvals[5];					\
	      s_data[3] = xvals[2];					\
	      s_data[4] = xvals[6];					\
	      s_data[5] = xvals[3];					\
	      break;							\
	    case 7:							\
	      s_data[0] = xvals[0];					\
	      s_data[1] = xvals[4];					\
	      s_data[2] = xvals[1];					\
	      s_data[3] = xvals[5];					\
	      s_data[4] = xvals[2];					\
	      s_data[5] = xvals[6];					\
	      s_data[6] = xvals[3];					\
	      break;							\
	    default:							\
	      for (j = 0; j < global_channel_depth; j++)		\
		s_data[j] = xvals[j];					\
	    }								\
	  s_data += global_channel_depth;				\
	}								\
    }									\
}

FILL_COLORS_EXTENDED_FUNCTION(unsigned short, 16)
FILL_COLORS_EXTENDED_FUNCTION(unsigned char, 8)

static void
fill_colors_extended(unsigned char *data, size_t len, size_t scount,
		     testpattern_t *p, size_t bytes)
{
  switch (bytes)
    {
    case 1:
      fill_colors_extended_8(data, len, scount, p);
      break;
    case 2:
      fill_colors_extended_16(data, len, scount, p);
      break;
    }
}

#define FILL_COLORS_FUNCTION(T, bits)					\
static void								\
fill_colors_##bits(unsigned char *data, size_t len, size_t scount,	\
		   testpattern_t *p)					\
{									\
  double mins[4];							\
  double vals[4];							\
  double gammas[4];							\
  double levels[4];							\
  double lower = p->d.pattern.lower;					\
  double upper = p->d.pattern.upper;					\
  int i;								\
  int j;								\
  int pixels;								\
  T *s_data = (T *) data;						\
  unsigned multiplier = global_noscale ? 1 : (1 << bits) - 1;		\
									\
  vals[0] = p->d.pattern.vals[0];					\
  mins[0] = p->d.pattern.mins[0];					\
									\
  for (j = 1; j < 4; j++)						\
    {									\
      vals[j] = p->d.pattern.vals[j] == -2 ? global_levels[j] :		\
	p->d.pattern.vals[j];						\
      mins[j] = p->d.pattern.mins[j] == -2 ? global_levels[j] :		\
	p->d.pattern.mins[j];						\
      levels[j] = p->d.pattern.levels[j] ==				\
	-2 ? global_levels[j] : p->d.pattern.levels[j];			\
    }									\
  for (j = 0; j < 4; j++)						\
    {									\
      gammas[j] =							\
	p->d.pattern.gammas[j] * global_gamma * global_gammas[j];	\
      vals[j] -= mins[j];						\
    }									\
									\
  if (scount > len)							\
    scount = len;							\
  pixels = len / scount;						\
  for (i = 0; i < scount; i++)						\
    {									\
      int k;								\
      double where = (double) i / ((double) scount - 1);		\
      double cmyv;							\
      double kv;							\
      double val = where;						\
      double xvals[4];							\
      for (j = 0; j < 4; j++)						\
	{								\
	  if (j > 0)							\
	    xvals[j] = mins[j] + val * vals[j];				\
	  else								\
	    xvals[j] = mins[j] + vals[j];				\
	  xvals[j] = pow(xvals[j], gammas[j]);				\
	}								\
									\
      if (where <= lower)						\
	kv = 0;								\
      else if (where > upper)						\
	kv = where;							\
      else								\
	kv = (where - lower) * upper / (upper - lower);			\
      cmyv = vals[0] * (where - kv);					\
      xvals[0] *= kv;							\
      for (j = 1; j < 4; j++)						\
	xvals[j] += cmyv * levels[j];					\
      for (j = 0; j < 4; j++)						\
	{								\
	  if (xvals[j] > 1)						\
	    xvals[j] = 1;						\
	  xvals[j] *= global_ink_limit * multiplier;			\
	  xvals[j] += 0.9;						\
	}								\
      for (k = 0; k < pixels; k++)					\
	{								\
	  switch (global_channel_depth)					\
	    {								\
	    case 0:							\
	      for (j = 0; j < 4; j++)					\
		s_data[j] = xvals[(j + 1) % 4];				\
	      s_data += 4;						\
	      break;							\
	    case 1:							\
	      s_data[0] = xvals[0];					\
	      break;							\
	    case 2:							\
	      s_data[0] = xvals[0];					\
	      s_data[1] = 0;						\
	      break;							\
	    case 3:							\
	      for (j = 1; j < 4; j++)					\
		s_data[j - 1] = xvals[j];				\
	      break;							\
	    case 4:							\
	      for (j = 0; j < 4; j++)					\
		s_data[j] = xvals[j];					\
	      break;							\
	    case 6:							\
	      s_data[0] = xvals[0];					\
	      s_data[1] = xvals[1];					\
	      s_data[2] = 0;						\
	      s_data[3] = xvals[2];					\
	      s_data[4] = 0;						\
	      s_data[5] = xvals[3];					\
	      break;							\
	    case 7:							\
	      for (j = 0; j < 4; j++)					\
		s_data[j * 2] = xvals[j];				\
	      for (j = 1; j < 6; j += 2)				\
		s_data[j] = 0;						\
	      break;							\
	    }								\
	  s_data += global_channel_depth;				\
	}								\
    }									\
}

FILL_COLORS_FUNCTION(unsigned short, 16)
FILL_COLORS_FUNCTION(unsigned char, 8)

static void
fill_colors(unsigned char *data, size_t len, size_t scount,
	    testpattern_t *p, size_t bytes)
{
  if (strcmp(global_image_type, "RGB") == 0)
    fill_colors_extended(data, len, scount, p, bytes);
  else
    {
      switch (bytes)
	{
	case 1:
	  fill_colors_8(data, len, scount, p);
	  break;
	case 2:
	  fill_colors_16(data, len, scount, p);
	  break;
	}
    }
}

extern FILE *yyin;

static void
fill_pattern(testpattern_t *p, unsigned char *data, size_t width,
	     size_t s_count, size_t image_depth, size_t byte_depth)
{
  memset(data, 0, global_printer_width * image_depth * byte_depth);
  switch (p->type)
    {
    case E_PATTERN:
      fill_colors(data, width, s_count, p, byte_depth);
      break;
    case E_XPATTERN:
      fill_colors_extended(data, width, s_count, p, byte_depth);
      break;
    case E_GRID:
      fill_grid(data, width, s_count, p, byte_depth);
      break;
    default:
      break;
    }
}


static stp_image_status_t
Image_get_row(stp_image_t *image, unsigned char *data,
	      size_t byte_limit, int row)
{
  int depth = global_channel_depth;
  if (! Image_is_valid)
    {
      fputs("Calling Image_get_row with invalid image!\n", stderr);
      abort();
    }
  if (static_testpatterns[0].type == E_IMAGE)
    {
      testpattern_t *t = &(static_testpatterns[0]);
      int total_read = fread(data, 1, t->d.image.x * depth * global_bit_depth / 8,
			     yyin);
      if (total_read != t->d.image.x * depth * global_bit_depth / 8)
	{
	  fputs("Read failed!\n", stderr);
	  return STP_IMAGE_STATUS_ABORT;
	}
      if (!global_quiet)
	fputc('.', stderr);
    }
  else
    {
      static int previous_band = -1;
      static int printed_blackline = 0;
      int band = row / global_band_height;
      if (row == global_printer_height - 1 && ! global_noblackline)
	fill_black(data, global_printer_width, global_steps,
		   global_bit_depth / 8);
      else if (band >= global_n_testpatterns)
	fill_white(data, global_printer_width, global_steps,
		   global_bit_depth / 8);
      else
	{
	  if (band != previous_band)
	    {
	      if (! global_noblackline && printed_blackline == 0)
		{
		  fill_black(data, global_printer_width, global_steps,
			     global_bit_depth / 8);
		  printed_blackline = 1;
		  return STP_IMAGE_STATUS_OK;
		}
	      else
		{
		  previous_band = band;
		  printed_blackline = 0;
		  if (! global_quiet)
		    fputc('.', stderr);
		}
	    }
	  fill_pattern(&(static_testpatterns[band]), data,
		       global_printer_width, global_steps, depth,
		       global_bit_depth / 8);
	}
    }
  return STP_IMAGE_STATUS_OK;
}

static int
Image_width(stp_image_t *image)
{
  if (! Image_is_valid)
    {
      fputs("Calling Image_width with invalid image!\n", stderr);
      abort();
    }
  if (static_testpatterns[0].type == E_IMAGE)
    return static_testpatterns[0].d.image.x;
  else
    return global_printer_width;
}

static int
Image_height(stp_image_t *image)
{
  if (! Image_is_valid)
    {
      fputs("Calling Image_height with invalid image!\n", stderr);
      abort();
    }
  if (static_testpatterns[0].type == E_IMAGE)
    return static_testpatterns[0].d.image.y;
  else
    return global_printer_height;
}

static void
Image_init(stp_image_t *image)
{
  if (Image_is_valid)
    {
      fputs("Calling Image_init with already valid image!\n", stderr);
      abort();
    }
  Image_is_valid = 1;
 /* dummy function */
}

static void
Image_reset(stp_image_t *image)
{
  if (!Image_is_valid)
    {
      fputs("Calling Image_reset with invalid image!\n", stderr);
      abort();
    }
 /* dummy function */
}

static void
Image_conclude(stp_image_t *image)
{
  if (! Image_is_valid)
    {
      fputs("Calling Image_conclude with invalid image!\n", stderr);
      abort();
    }
  Image_is_valid = 0;
}

static const char *
Image_get_appname(stp_image_t *image)
{
  if (! Image_is_valid)
    {
      fprintf(stderr, "Calling Image_get_appname with invalid image!\n");
      abort();
    }
  return "Test Pattern";
}
