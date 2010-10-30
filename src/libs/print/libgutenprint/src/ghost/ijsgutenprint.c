/*
 *  $Id: ijsgutenprint.c,v 1.23 2010/07/19 11:14:04 rlk Exp $
 *
 *   IJS server for Gutenprint.
 *
 *   Copyright 2001 Robert Krawitz (rlk@alum.mit.edu)
 *
 *   Originally written by Russell Lang, copyright assigned to Robert Krawitz.
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
 *
 * Revision History:
 *
 *   See ChangeLog
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <ijs.h>
#include <ijs_server.h>
#include <errno.h>
#include <gutenprint/gutenprint-intl-internal.h>


static int suppress_messages = 0;
volatile int SDEBUG = 1;
static int job_aborted = 0;

#define STP_DEBUG(x)				\
do						\
{						\
  if (!suppress_messages)			\
    fprintf(stderr, "DEBUG: ");			\
  if (!suppress_messages)			\
    x;						\
} while (0)

typedef struct _GutenprintParamList GutenprintParamList;

struct _GutenprintParamList {
  GutenprintParamList *next;
  char *key;
  char *value;
  int value_size;
};

typedef struct _IMAGE
{
  IjsServerCtx *ctx;
  stp_vars_t *v;
  char *filename;	/* OutputFile */
  int fd;		/* OutputFD + 1 (so that 0 is invalid) */
  int width;		/* pixels */
  int height;		/* pixels */
  int bps;		/* bytes per sample */
  int n_chan;		/* number of channels */
  int xres;		/* dpi */
  int yres;
  int output_type;
  int left_margin;
  int right_margin;
  int top_margin;
  int bottom_margin;
  int monochrome_flag;	/* for monochrome output */
  int row;		/* row number in buffer */
  int row_width;	/* length of a row */
  char *row_buf;	/* buffer for raster */
  double total_bytes;	/* total size of raster */
  double bytes_left;	/* bytes remaining to be read */
  GutenprintParamList *params;
} IMAGE;

static const char DeviceGray[] = "DeviceGray";
static const char DeviceRGB[] = "DeviceRGB";
static const char DeviceCMYK[] = "DeviceCMYK";

static const char *version_id;
static int version_is_ok = 1;
#define VERSION_MISMATCH "\
ERROR: ijsgutenprint: the version of Gutenprint software installed (%s)\n\
ERROR: ijsgutenprint: does not match the PPD file (%s).  If you have upgraded your version\n\
ERROR: ijsgutenprint: of Gutenprint recently, you must reinstall all printer queues.\n\
ERROR: ijsgutenprint: Please refer to your vendor's documentation or the ``foomatic-ppdfile''\n\
ERROR: ijsgutenprint: command for instructions.\n\
ERROR: ijsgutenprint: the version of Gutenprint software installed (%s) does not match the PPD file (%s).\n"

const char *gutenprint_ppd_version = NULL;
static int ppd_mode = 0;	/* Use PPD-style margins */

static stp_string_list_t *option_remap_list = NULL;
static int print_messages_as_errors = 0;

static double page_bytes_printed = 0;
static double total_bytes_printed = 0;

static char *
c_strdup(const char *s)
{
  char *ret = stp_malloc(strlen(s) + 1);
  strcpy(ret, s);
  return ret;
}

static int
image_init(IMAGE *img, IjsPageHeader *ph)
{
  img->width = ph->width;
  img->height = ph->height;
  img->bps = ph->bps;
  img->n_chan = ph->n_chan;
  img->xres = ph->xres;
  img->yres = ph->yres;

  img->row = -1;
  img->row_width = (ph->n_chan * ph->bps * ph->width + 7) >> 3;
  if (img->row_buf)
    stp_free(img->row_buf);
  img->row_buf = (char *)stp_malloc(img->row_width);
  STP_DEBUG(fprintf(stderr, "ijsgutenprint: image_init\n"));
  STP_DEBUG(fprintf(stderr,
		    "ijsgutenprint: ph width %d height %d bps %d n_chan %d xres %f yres %f\n",
		    ph->width, ph->height, ph->bps, ph->n_chan, ph->xres,
		    ph->yres));

  stp_set_string_parameter(img->v, "ChannelBitDepth", "8");
  if ((img->bps == 1) && (img->n_chan == 1) &&
      (strncmp(ph->cs, DeviceGray, strlen(DeviceGray)) == 0))
    {
      STP_DEBUG(fprintf(stderr, "ijsgutenprint: output monochrome\n"));
      stp_set_string_parameter(img->v, "InputImageType", "Whitescale");
      stp_set_string_parameter(img->v, "PrintingMode", "BW");
      stp_set_string_parameter(img->v, "ColorCorrection", "Threshold");
      img->monochrome_flag = 1;
      /* 8-bit greyscale */
    }
  else if (img->bps == 8 || img->bps == 16)
    {
      if (img->bps == 8)
	stp_set_string_parameter(img->v, "ChannelBitDepth", "8");
      else
	stp_set_string_parameter(img->v, "ChannelBitDepth", "16");
      if ((img->n_chan == 1) &&
	  (strncmp(ph->cs, DeviceGray, strlen(DeviceGray)) == 0))
	{
	  STP_DEBUG(fprintf(stderr, "ijsgutenprint: output gray\n"));
	  stp_set_string_parameter(img->v, "InputImageType", "Whitescale");
	  stp_set_string_parameter(img->v, "PrintingMode", "BW");
	  img->monochrome_flag = 0;
	  /* 8/16-bit greyscale */
	}
      else if ((img->n_chan == 3) &&
	       (strncmp(ph->cs, DeviceRGB, strlen(DeviceRGB)) == 0))
	{
	  STP_DEBUG(fprintf(stderr, "ijsgutenprint: output color\n"));
	  stp_set_string_parameter(img->v, "InputImageType", "RGB");
	  stp_set_string_parameter(img->v, "PrintingMode", "Color");
	  img->monochrome_flag = 0;
	  /* 24/48-bit RGB colour */
	}
      else if ((img->n_chan == 4) &&
	       (strncmp(ph->cs, DeviceCMYK, strlen(DeviceCMYK)) == 0))
	{
	  STP_DEBUG(fprintf(stderr, "ijsgutenprint: output CMYK\n"));
	  stp_set_string_parameter(img->v, "InputImageType", "CMYK");
	  stp_set_string_parameter(img->v, "PrintingMode", "Color");
	  img->monochrome_flag = 0;
	  /* 32/64-bit CMYK colour */
	}
    }
  else
    {
      fprintf(stderr, "ERROR: ijsgutenprint: Bad color space: bps %d channels %d space %s\n",
		img->bps, img->n_chan, ph->cs);
      /* unsupported */
      return -1;
    }

  if (img->row_buf == NULL)
    {
      fprintf(stderr, "ERROR: ijsgutenprint: No row buffer\n");
      return -1;
    }

  return 0;
}

static void
image_finish(IMAGE *img)
{
  if (img->row_buf)
    stp_free(img->row_buf);
  img->row_buf = NULL;
}

static double
get_float(const char *str, const char *name, double *pval)
{
  float new_value;
  int status = 0;
  /* Force locale to "C", because decimal numbers coming from the IJS
     client are always with a decimal point, nver with a decimal comma */
  setlocale(LC_ALL, "C");
  if (sscanf(str, "%f", &new_value) == 1)
    *pval = new_value;
  else
    {
      fprintf(stderr, "ERROR: ijsgutenprint: Unable to parse parameter %s=%s (expect a number)\n",
	      name, str);
      status = -1;
    }
  setlocale(LC_ALL, "");
  return status;
}

static int
get_int(const char *str, const char *name, int *pval)
{
  int new_value;
  int status = 0;
  /* Force locale to "C", because decimal numbers sent to the IJS
     client must have a decimal point, nver a decimal comma */
  setlocale(LC_ALL, "C");
  if (sscanf(str, "%d", &new_value) == 1)
    *pval = new_value;
  else
    {
      fprintf(stderr, "ERROR: ijsgutenprint: Unable to parse parameter %s=%s (expect a number)\n",
	      name, str);
      status = -1;
    }
  setlocale(LC_ALL, "");
  return status;
}

static int
parse_wxh_internal(const char *val, int size, double *pw, double *ph)
{
  char buf[256];
  char *tail;
  int i;

  for (i = 0; i < size; i++)
    if (val[i] == 'x')
      break;

  if (i + 1 >= size)
    return IJS_ESYNTAX;

  if (i >= sizeof(buf))
    return IJS_EBUF;

  memcpy (buf, val, i);
  buf[i] = 0;
  *pw = strtod (buf, &tail);
  if (tail == buf)
    return IJS_ESYNTAX;

  if (size - i > sizeof(buf))
    return IJS_EBUF;

  memcpy (buf, val + i + 1, size - i - 1);
  buf[size - i - 1] = 0;
  *ph = strtod (buf, &tail);
  if (tail == buf)
    return IJS_ESYNTAX;

  return 0;
}

/* A C implementation of /^(\d\.+\-eE)+x(\d\.+\-eE)+$/ */
static int
gutenprint_parse_wxh (const char *val, int size, double *pw, double *ph)
{
  /* Force locale to "C", because decimal numbers coming from the IJS
     client are always with a decimal point, nver with a decimal comma */
  int status;
  setlocale(LC_ALL, "C");
  status = parse_wxh_internal(val, size, pw, ph);
  setlocale(LC_ALL, "");
  return status;
}

/**
 * gutenprint_find_key: Search parameter list for key.
 *
 * @key: key to look up
 *
 * Return value: GutenprintParamList entry matching @key, or NULL.
 **/
static GutenprintParamList *
gutenprint_find_key (GutenprintParamList *pl, const char *key)
{
  GutenprintParamList *curs;

  for (curs = pl; curs != NULL; curs = curs->next)
    {
      if (!strcmp (curs->key, key))
	return curs;
    }
  return NULL;
}

static int
gutenprint_status_cb (void *status_cb_data,
		      IjsServerCtx *ctx,
		      IjsJobId job_id)
{
  return 0;
}

static const char *
list_all_parameters(void)
{
  static char *param_string = NULL;
  size_t param_length = 0;
  size_t offset = 0;
  if (param_length == 0)
    {
      stp_string_list_t *sl = stp_string_list_create();
      int printer_count = stp_printer_model_count();
      int i;
      stp_string_list_add_string(sl, "PrintableArea", NULL);
      stp_string_list_add_string(sl, "Dpi", NULL);
      stp_string_list_add_string(sl, "PrintableTopLeft", NULL);
      stp_string_list_add_string(sl, "DeviceManufacturer", NULL);
      stp_string_list_add_string(sl, "DeviceModel", NULL);
      stp_string_list_add_string(sl, "PageImageFormat", NULL);
      stp_string_list_add_string(sl, "OutputFile", NULL);
      stp_string_list_add_string(sl, "OutputFd", NULL);
      stp_string_list_add_string(sl, "PaperSize", NULL);
      stp_string_list_add_string(sl, "MediaName", NULL);
      stp_string_list_add_string(sl, "STP_VERSION", NULL);
      for (i = 0; i < printer_count; i++)
	{
	  const stp_printer_t *printer = stp_get_printer_by_index(i);
	  stp_parameter_list_t params =
	    stp_get_parameter_list(stp_printer_get_defaults(printer));
	  size_t count = stp_parameter_list_count(params);
	  int j;
	  if (strcmp(stp_printer_get_family(printer), "ps") == 0 ||
	      strcmp(stp_printer_get_family(printer), "raw") == 0)
	    continue;
	  for (j = 0; j < count; j++)
	    {
	      const stp_parameter_t *param =
		stp_parameter_list_param(params, j);
	      char *tmp =
		stp_malloc(strlen(param->name) + strlen("STP_") + 1);
	      sprintf(tmp, "STP_%s", param->name);
	      if ((param->p_level < STP_PARAMETER_LEVEL_ADVANCED4) &&
		  (param->p_type != STP_PARAMETER_TYPE_RAW) &&
		  (param->p_type != STP_PARAMETER_TYPE_FILE) &&
		  (!param->read_only) &&
		  (strcmp(param->name, "Resolution") != 0) &&
		  (strcmp(param->name, "PageSize") != 0) &&
		  (!stp_string_list_is_present(sl, tmp)))
		{
		  sprintf(tmp, "STP_%s", param->name);
		  stp_string_list_add_string(sl, tmp, NULL);
		  if ((param->p_type == STP_PARAMETER_TYPE_DOUBLE ||
		       param->p_type == STP_PARAMETER_TYPE_DIMENSION) &&
		      !param->read_only && param->is_active &&
		      !param->is_mandatory)
		    {
		      char *tmp1 =
			stp_malloc(strlen(param->name) + strlen("STP_Enable") + 1);
		      sprintf(tmp1, "STP_Enable%s", param->name);
		      stp_string_list_add_string(sl, tmp1, NULL);
		      stp_free(tmp1);
		    }
		}
	      stp_free(tmp);
	    }
	  stp_parameter_list_destroy(params);
	}
      for (i = 0; i < stp_string_list_count(sl); i++)
	param_length += strlen(stp_string_list_param(sl, i)->name) + 1;
      param_string = stp_malloc(param_length);
      for (i = 0; i < stp_string_list_count(sl); i++)
	{
	  stp_param_string_t *param = stp_string_list_param(sl, i);
	  strcpy(param_string + offset, param->name);
	  offset += strlen(param->name) + 1;
	  param_string[offset - 1] = ',';
	}
      if (offset != param_length)
	{
	  fprintf(stderr, "ERROR: ijsgutenprint: Bad string length %lu != %lu!\n",
		  (unsigned long) offset,
		  (unsigned long) param_length);
	  exit(1);
	}
      param_string[param_length - 1] = '\0';
    }
  return param_string;
}


static int
gutenprint_list_cb (void *list_cb_data,
	      IjsServerCtx *ctx,
	      IjsJobId job_id,
	      char *val_buf,
	      int val_size)
{
  const char *param_list = list_all_parameters();
  int size = strlen (param_list);
  STP_DEBUG(fprintf(stderr, "ijsgutenprint: gutenprint_list_cb: %s\n", param_list));

  if (size > val_size)
    return IJS_EBUF;

  memcpy (val_buf, param_list, size);
  return size;
}

static int
gutenprint_enum_cb (void *enum_cb_data,
	      IjsServerCtx *ctx,
	      IjsJobId job_id,
	      const char *key,
	      char *val_buf,
	      int val_size)
{
  const char *val = NULL;
  STP_DEBUG(fprintf(stderr, "ijsgutenprint: gutenprint_enum_cb: key=%s\n", key));
  if (!strcmp (key, "ColorSpace"))
    val = "DeviceRGB,DeviceGray,DeviceCMYK";
  else if (!strcmp (key, "DeviceManufacturer"))
    val = "Gutenprint";
  else if (!strcmp (key, "DeviceModel"))
    val = "gutenprint";
  else if (!strcmp (key, "PageImageFormat"))
    val = "Raster";
  else if (!strcmp (key, "BitsPerSample"))
    val = "8,16";
  else if (!strcmp (key, "ByteSex"))
    {
#if __BYTE_ORDER == __LITTLE_ENDIAN
      val="little-endian";
#else
      val="big-endian";
#endif
    }

  if (val == NULL)
    return IJS_EUNKPARAM;
  else
    {
      int size = strlen (val);

      if (size > val_size)
	return IJS_EBUF;
      memcpy (val_buf, val, size);
      return size;
    }
}

static int
gutenprint_get_cb (void *get_cb_data,
	     IjsServerCtx *ctx,
	     IjsJobId job_id,
	     const char *key,
	     char *val_buf,
	     int val_size)
{
  IMAGE *img = (IMAGE *)get_cb_data;
  stp_vars_t *v = img->v;
  const stp_printer_t *printer = stp_get_printer(v);
  GutenprintParamList *pl = img->params;
  GutenprintParamList *curs;
  const char *val = NULL;
  char buf[256];

  STP_DEBUG(fprintf(stderr, "ijsgutenprint: gutenprint_get_cb: %s\n", key));
  if (!printer)
    {
      if (strlen(stp_get_driver(v)) == 0)
	fprintf(stderr, "ERROR: ijsgutenprint: Printer must be specified with -sDeviceModel\n");
      else
	fprintf(stderr, "ERROR: ijsgutenprint: Printer %s is not a known model\n",
		stp_get_driver(v));
      return IJS_EUNKPARAM;
    }
  curs = gutenprint_find_key (pl, key);
  if (curs != NULL)
    {
      if (curs->value_size > val_size)
	return IJS_EBUF;
      memcpy (val_buf, curs->value, curs->value_size);
      return curs->value_size;
    }

  if (!strcmp(key, "PrintableArea"))
    {
      int l, r, b, t;
      int h, w;
      if (ppd_mode)
	{
	  stp_get_media_size(v, &w, &h);
	  stp_get_maximum_imageable_area(v, &l, &r, &b, &t);
	  if (l < 0)
	    l = 0;
	  if (r > w)
	    r = w;
	  if (t < 0)
	    t = 0;
	  if (b > h)
	    b = h;
	}
      else
	stp_get_imageable_area(v, &l, &r, &b, &t);
      

      h = b - t;
      w = r - l;
      /* Force locale to "C", because decimal numbers sent to the IJS
	 client must have a decimal point, nver a decimal comma */
      setlocale(LC_ALL, "C");
      sprintf(buf, "%gx%g", (double) w / 72.0, (double) h / 72.0);
      setlocale(LC_ALL, "");
      STP_DEBUG(fprintf(stderr, "ijsgutenprint: PrintableArea %d %d %s\n", h, w, buf));
      val = buf;
    }
  else if (!strcmp(key, "Dpi"))
    {
      int x, y;
      stp_describe_resolution(v, &x, &y);
      /* Force locale to "C", because decimal numbers sent to the IJS
	 client must have a decimal point, nver a decimal comma */
      setlocale(LC_ALL, "C");
      sprintf(buf, "%d", x);
      setlocale(LC_ALL, "");
      STP_DEBUG(fprintf(stderr, "ijsgutenprint: Dpi %d %d (%d) %s\n", x, y, x, buf));
      val = buf;
    }
  else if (!strcmp(key, "PrintableTopLeft"))
    {
      int l, r, b, t;
      int h, w;
      stp_get_media_size(v, &w, &h);
      if (ppd_mode)
	{
	  stp_get_maximum_imageable_area(v, &l, &r, &b, &t);
	  if (l < 0)
	    l = 0;
	  if (r > w)
	    r = w;
	  if (t < 0)
	    t = 0;
	  if (b > h)
	    b = h;
	}
      else
	stp_get_imageable_area(v, &l, &r, &b, &t);
      /* Force locale to "C", because decimal numbers sent to the IJS
	 client must have a decimal point, nver a decimal comma */
      setlocale(LC_ALL, "C");
      sprintf(buf, "%gx%g", (double) l / 72.0, (double) t / 72.0);
      setlocale(LC_ALL, "");
      STP_DEBUG(fprintf(stderr, "ijsgutenprint: PrintableTopLeft %d %d %s\n", t, l, buf));
      val = buf;
    }
  else if (!strcmp (key, "DeviceManufacturer"))
    val = "Gutenprint";
  else if (!strcmp (key, "DeviceModel"))
    val = stp_get_driver(img->v);
  else if (!strcmp (key, "PageImageFormat"))
    val = "Raster";

  if (val == NULL)
    return IJS_EUNKPARAM;
  else
    {
      int size = strlen (val);

      if (size > val_size)
	return IJS_EBUF;
      memcpy (val_buf, val, size);
      return size;
    }
}

static void
print_debug_setcb(const char *key, const char *value, int value_size)
{
  fprintf (stderr, "ijsgutenprint: gutenprint_set_cb: %s='", key);
  fwrite (value, 1, value_size, stderr);
  fputs ("'\n", stderr);
}    

static int
gutenprint_set_cb (void *set_cb_data, IjsServerCtx *ctx, IjsJobId jobid,
	     const char *key, const char *value, int value_size)
{
  int code = 0;
  char vbuf[256];
  int i;
  double z = 0;
  IMAGE *img = (IMAGE *)set_cb_data;
  STP_DEBUG(print_debug_setcb(key, value, value_size));
  if (value_size > sizeof(vbuf)-1)
    return -1;
  memset(vbuf, 0, sizeof(vbuf));
  memcpy(vbuf, value, value_size);

  if (strcmp(key, "OutputFile") == 0)
    {
      if (img->filename)
	stp_free(img->filename);
      img->filename = c_strdup(vbuf);
    }
  else if (strcmp(key, "OutputFD") == 0)
    {
      /* Force locale to "C", because decimal numbers sent to the IJS
	 client must have a decimal point, nver a decimal comma */
      setlocale(LC_ALL, "C");
      img->fd = atoi(vbuf) + 1;
      setlocale(LC_ALL, "");
    }
  else if (strcmp(key, "DeviceManufacturer") == 0)
    ;				/* We don't care who makes it */
  else if (strcmp(key, "DeviceModel") == 0)
    {
      const stp_printer_t *printer = stp_get_printer_by_driver(vbuf);
      stp_set_driver(img->v, vbuf);
      if (printer &&
	  strcmp(stp_printer_get_family(printer), "ps") != 0 &&
	  strcmp(stp_printer_get_family(printer), "raw") != 0)
        {
	  stp_set_printer_defaults(img->v, printer);
          /* Reset JobMode to "Job" */
          stp_set_string_parameter(img->v, "JobMode", "Job");
        }
      else
	{
	  fprintf(stderr, "ERROR: ijsgutenprint: unknown DeviceModel %s\n", vbuf);
	  code = IJS_ERANGE;
	}
    }
  else if (strcmp(key, "TopLeft") == 0)
    {
      int l, r, b, t, pw, ph;
      double w, h;
      stp_get_media_size(img->v, &pw, &ph);
      if (ppd_mode)
	{
	  stp_get_maximum_imageable_area(img->v, &l, &r, &b, &t);
	  STP_DEBUG(fprintf(stderr, "ijsgutenprint: l %d r %d t %d b %d pw %d ph %d\n",
			    l, r, t, b, pw, ph));
	  if (l < 0)
	    l = 0;
	  if (r > pw)
	    r = pw;
	  if (t < 0)
	    t = 0;
	  if (b > ph)
	    b = ph;
	}
      else
	stp_get_imageable_area(img->v, &l, &r, &b, &t);
      STP_DEBUG(fprintf(stderr, "ijsgutenprint ppd_mode %d top left: %s\n",
			ppd_mode, vbuf));
      STP_DEBUG(fprintf(stderr, "ijsgutenprint: l %d r %d t %d b %d pw %d ph %d\n",
			l, r, t, b, pw, ph));
      code = gutenprint_parse_wxh(vbuf, strlen(vbuf), &w, &h);
      if (code == 0)
	{
	  int al = (w * 72) + .5;
	  int ah = (h * 72) + .5;
	  STP_DEBUG(fprintf(stderr, "ijsgutenprint: left top %f %f %d %d %s\n",
			    w * 72, h * 72, al, ah, vbuf));
	  if (al >= 0)
	    stp_set_left(img->v, al);
	  if (ah >= 0)
	    stp_set_top(img->v, ah);
	  stp_set_width(img->v, r - l);
	  stp_set_height(img->v, b - t);
	}
      else
	fprintf(stderr, "ERROR: ijsgutenprint: cannot parse TopLeft %s\n", vbuf);
    }
  else if (strcmp(key, "PaperSize") == 0)
    {
      double w, h;
      code = gutenprint_parse_wxh(vbuf, strlen(vbuf), &w, &h);
      if (code == 0)
	{
	  const stp_papersize_t *p;
	  w *= 72;
	  h *= 72;
	  STP_DEBUG(fprintf(stderr, "ijsgutenprint: paper size %f %f %s\n", w, h, vbuf));
	  stp_set_page_width(img->v, w);
	  stp_set_page_height(img->v, h);
	  if ((p = stp_get_papersize_by_size_exact(h, w)) != NULL)
	    {
	      STP_DEBUG(fprintf(stderr, "ijsgutenprint: Found page size %s\n", p->name));
	      stp_set_string_parameter(img->v, "PageSize", p->name);
	    }
	  else
	    STP_DEBUG(fprintf(stderr, "ijsgutenprint: No matching paper size found\n"));
	}
      else
	fprintf(stderr, "ERROR: ijsgutenprint: cannot parse PaperSize %s\n", vbuf);
    }

/*
 * Duplex & Tumble. The PS: values come from the PostScript document, the
 * others come from the command line. However, the PS: values seem to get
 * fed back again as non PS: values after the command line is processed.
 * The net effect is that the command line is always overridden by the
 * values from the document.
 */

  else if ((strcmp (key, "Duplex") == 0) || (strcmp (key, "PS:Duplex") == 0))
    {
      stp_set_string_parameter(img->v, "x_Duplex", vbuf);
    }
  else if ((strcmp (key, "Tumble") == 0) || (strcmp (key, "PS:Tumble") == 0))
    {
       stp_set_string_parameter(img->v, "x_Tumble", vbuf);
    }
  else if (strcmp(key, "STP_VERSION") == 0)
    {
      ppd_mode = 1;
      if (strcmp(vbuf, version_id) != 0)
	{
	  fprintf(stderr, VERSION_MISMATCH,
		  version_id, vbuf, version_id, vbuf);
	  version_is_ok = 0;
	  gutenprint_ppd_version = c_strdup(vbuf);
	  code = IJS_ERANGE;
	}
    } 
  else if (strncmp(key, "STP_OPT_REMAP_", strlen("STP_OPT_REMAP_")) == 0)
    {
      const char *xkey = key + strlen("STP_OPT_REMAP_");
      char *buf1 = stp_malloc(strlen("STP_OPT_") + strlen(xkey) + 1);
      char *buf2 = c_strdup(vbuf);
      strcpy(buf1, "STP_OPT_");
      strcpy(buf1 + strlen("STP_OPT_"), xkey);
      stp_string_list_add_string(option_remap_list, buf1, buf2);
      stp_free(buf1);
      stp_free(buf2);
    }
  else if (strncmp(key, "STP_", 4) == 0)
    {
      stp_curve_t *curve;
      stp_parameter_t desc;
      const char *xkey = key + 4;
      stp_param_string_t *pstr = stp_string_list_find(option_remap_list, key);
      if (pstr)
	{
	  xkey = pstr->text;
	  STP_DEBUG(fprintf(stderr, "ijsgutenprint: remapping %s to %s\n",
			    key, xkey));
	}

      stp_describe_parameter(img->v, xkey, &desc);
      switch (desc.p_type)
	{
	case STP_PARAMETER_TYPE_STRING_LIST:
	  stp_set_string_parameter(img->v, xkey, vbuf);
	  break;
	case STP_PARAMETER_TYPE_FILE:
	  stp_set_file_parameter(img->v, xkey, vbuf);
	  break;
	case STP_PARAMETER_TYPE_CURVE:
	  curve = stp_curve_create_from_string(vbuf);
	  if (curve)
	    {
	      stp_set_curve_parameter(img->v, xkey, curve);
	      stp_curve_destroy(curve);
	    }
	  else
	    fprintf(stderr, "ERROR: ijsgutenprint: cannot parse curve %s\n", vbuf);
	  break;
	case STP_PARAMETER_TYPE_DOUBLE:
	  code = get_float(vbuf, xkey, &z);
	  if (code == 0)
	    stp_set_float_parameter(img->v, xkey, z);
	  else
	    fprintf(stderr, "ERROR: ijsgutenprint: cannot parse %s float %s\n", xkey, vbuf);
	  break;
	case STP_PARAMETER_TYPE_INT:
	  code = get_int(vbuf, xkey, &i);
	  if (code == 0)
	    stp_set_int_parameter(img->v, xkey, i);
	  else
	    fprintf(stderr, "ERROR: ijsgutenprint: cannot parse %s int %s\n", xkey, vbuf);
	  break;
	case STP_PARAMETER_TYPE_DIMENSION:
	  code = get_int(vbuf, xkey, &i);
	  if (code == 0)
	    stp_set_dimension_parameter(img->v, xkey, i);
	  else
	    fprintf(stderr, "ERROR: ijsgutenprint: cannot parse %s dimension %s\n", xkey, vbuf);
	  break;
	case STP_PARAMETER_TYPE_BOOLEAN:
	  if (strcmp(vbuf, "False") == 0 ||
	      strcmp(vbuf, "false") == 0 ||
	      strcmp(vbuf, "FALSE") == 0 ||
	      strcmp(vbuf, "0") == 0)
	    stp_set_boolean_parameter(img->v, xkey, 0);
	  else if (strcmp(vbuf, "True") == 0 ||
		   strcmp(vbuf, "true") == 0 ||
		   strcmp(vbuf, "TRUE") == 0 ||
		   strcmp(vbuf, "1") == 0)
	    stp_set_boolean_parameter(img->v, xkey, 1);
	  else
	    fprintf(stderr, "ERROR: ijsgutenprint: cannot parse %s boolean %s\n", xkey, vbuf);
	  break;
	default:
	  if (strncmp(xkey, "Enable", strlen("Enable")) == 0)
	    {
	      STP_DEBUG(fprintf(stderr,
				"ijsgutenprint: Setting dummy enable parameter %s %s\n",
				xkey, vbuf));
	      stp_set_string_parameter(img->v, xkey, vbuf);
	    }
	  else
	    fprintf(stderr, "ERROR: ijsgutenprint: Bad parameter %s %d\n", key, desc.p_type);
	}
      stp_parameter_description_destroy(&desc);
    }

  if (code == 0)
    {
      GutenprintParamList *pl = gutenprint_find_key (img->params, key);

      if (pl == NULL)
	{
	  pl = (GutenprintParamList *)stp_malloc (sizeof (GutenprintParamList));
	  pl->next = img->params;
	  pl->key = stp_malloc (strlen(key) + 1);
	  memcpy (pl->key, key, strlen(key) + 1);
	  img->params = pl;
	}
      else
	{
	  stp_free (pl->value);
	}
      pl->value = stp_malloc (value_size);
      memcpy (pl->value, value, value_size);
      pl->value_size = value_size;
    }
  else
    fprintf(stderr, "ERROR: ijsgutenprint: bad key code %d\n", code);

  return code;
}

/**********************************************************/

static void
gutenprint_errfunc(void *file, const char *buf, size_t bytes)
{
  size_t next_nl = 0;
  size_t where = 0;
  FILE *prn = (FILE *)file;
  while (where < bytes)
    {
      if (print_messages_as_errors)
	fputs("ERROR: Gutenprint: ", prn);
      else
	fputs("DEBUG: Gutenprint internal: ", prn);
      while (next_nl < bytes)
	{
	  if (buf[next_nl++] == '\n')
	    break;
	}
      fwrite(buf + where, 1, next_nl - where, prn);
      where = next_nl;
    }
}

static void
gutenprint_outfunc(void *data, const char *buffer, size_t bytes)
{
  page_bytes_printed += bytes;
  total_bytes_printed += bytes;
  if ((data != NULL) && (buffer != NULL) && (bytes != 0))
    fwrite(buffer, 1, bytes, (FILE *)data);
}

/**********************************************************/
/* stp_image_t functions */

static int
gutenprint_image_width(stp_image_t *image)
{
  IMAGE *img = (IMAGE *)(image->rep);
  STP_DEBUG(fprintf(stderr, "ijsgutenprint: image width %d\n", img->width));
  return img->width;
}

static int
gutenprint_image_height(stp_image_t *image)
{
  IMAGE *img = (IMAGE *)(image->rep);
  STP_DEBUG(fprintf(stderr, "ijsgutenprint: image height %d (%d)\n",
		    img->height, img->height * img->xres / img->yres));
  return img->height * img->xres / img->yres;
}

static void
throwaway_data(int amount, IMAGE *img)
{
  char trash[4096];	/* Throwaway */
  int block_count = amount / 4096;
  int leftover = amount % 4096;
  while (block_count > 0)
    {
      ijs_server_get_data(img->ctx, trash, 4096);
      block_count--;
    }
  if (leftover)
    ijs_server_get_data(img->ctx, trash, leftover);
}

static int
image_next_row(IMAGE *img)
{
  int status = 0;
  double n_bytes = img->bytes_left;
  if (img->bytes_left)
    {

      if (n_bytes > img->row_width)
	n_bytes = img->row_width;
#ifdef VERBOSE
      STP_DEBUG(fprintf(stderr, "ijsgutenprint: %.0f bytes left, reading %.d, on row %d\n",
			img->bytes_left, (int) n_bytes, img->row));
#endif
      throwaway_data(img->left_margin, img);
      status = ijs_server_get_data(img->ctx, img->row_buf, (int) n_bytes);
      if (status)
	{
	  STP_DEBUG(fprintf(stderr, "ERROR: ijsgutenprint: page aborted (%d) at line %d!\n",
			    status, img->row));
	  job_aborted = 1;
	  return status;
	}
      else
	{
	  img->row++;
	  img->bytes_left -= (n_bytes + img->right_margin + img->left_margin);
	}
      throwaway_data(img->right_margin, img);
    }
  else
    return 1;	/* Done */
  return status;
}

static stp_image_status_t
gutenprint_image_get_row(stp_image_t *image, unsigned char *data, size_t byte_limit,
		   int row)
{
  IMAGE *img = (IMAGE *)(image->rep);
  int physical_row = row * img->yres / img->xres;

  if ((physical_row < 0) || (physical_row >= img->height))
    return STP_IMAGE_STATUS_ABORT;

  /* Read until we reach the requested row. */
  while (physical_row > img->row)
    {
      if (image_next_row(img))
	return STP_IMAGE_STATUS_ABORT;
    }

  if (physical_row == img->row)
    {
      unsigned i, j, length;
      switch (img->bps)
	{
	case 16:
	case 8:
	  memcpy(data, img->row_buf, img->row_width);
	  break;
	case 1:
	  length = img->width / 8;
	  for (i = 0; i < length; i++)
	    for (j = 128; j > 0; j >>= 1)
	      {
		if (img->row_buf[i] & j)
		  data[0] = 255;
		else
		  data[0] = 0;
		data++;
	      }
	  length = img->width % 8;
	  for (j = 128; j > 1 << (7 - length); j >>= 1)
	    {
	      if (img->row_buf[i] & j)
		data[0] = 255;
	      else
		data[0] = 0;
	      data++;
	    }
	  break;
	default:
	  return STP_IMAGE_STATUS_ABORT;
	}
    }
  else
    return STP_IMAGE_STATUS_ABORT;
  return STP_IMAGE_STATUS_OK;
}


static const char *
gutenprint_image_get_appname(stp_image_t *image)
{
  return "ijsgutenprint";
}

/**********************************************************/

static const char *
safe_get_string_parameter(const stp_vars_t *v, const char *param)
{
  const char *val = stp_get_string_parameter(v, param);
  if (val)
    return val;
  else
    return "NULL";
}

static void
stp_dbg(const char *msg, const stp_vars_t *v)
{
  stp_parameter_list_t params = stp_get_parameter_list(v);
  int count = stp_parameter_list_count(params);
  int i;
  if (suppress_messages)
    return;
  fprintf(stderr, "DEBUG: %s\n", msg);
  fprintf(stderr, "DEBUG: ijsgutenprint: Settings: Model %s\n", stp_get_driver(v));
  for (i = 0; i < count; i++)
    {
      const stp_parameter_t *p = stp_parameter_list_param(params, i);
      switch (p->p_type)
	{
	case STP_PARAMETER_TYPE_DOUBLE:
	  if (stp_check_float_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
	    fprintf(stderr, "DEBUG: ijsgutenprint: Settings: %s %f\n",
		    p->name, stp_get_float_parameter(v, p->name));
	  break;
	case STP_PARAMETER_TYPE_INT:
	  if (stp_check_int_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
	    fprintf(stderr, "DEBUG: ijsgutenprint: Settings: %s %d\n",
		    p->name, stp_get_int_parameter(v, p->name));
	  break;
	case STP_PARAMETER_TYPE_DIMENSION:
	  if (stp_check_dimension_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
	    fprintf(stderr, "DEBUG: ijsgutenprint: Settings: %s %d\n",
		    p->name, stp_get_dimension_parameter(v, p->name));
	  break;
	case STP_PARAMETER_TYPE_BOOLEAN:
	  if (stp_check_boolean_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
	    fprintf(stderr, "DEBUG: ijsgutenprint: Settings: %s %s\n",
		    p->name,
		    stp_get_boolean_parameter(v, p->name) ? "true" : "false");
	  break;
	case STP_PARAMETER_TYPE_STRING_LIST:
	  if (stp_check_string_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
	    fprintf(stderr, "DEBUG: ijsgutenprint: Settings: %s %s\n",
		    p->name, safe_get_string_parameter(v, p->name));
	  break;
	case STP_PARAMETER_TYPE_CURVE:
	  if (stp_check_curve_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
	    {
	      char *curve =
		stp_curve_write_string(stp_get_curve_parameter(v, p->name));
	      fprintf(stderr, "DEBUG: ijsgutenprint: Settings: %s %s\n",
		      p->name, curve);
	      stp_free(curve);
	    }
	  break;
	default:
	  break;
	}
    }
  stp_parameter_list_destroy(params);
}

static void
purge_unused_float_parameters(stp_vars_t *v)
{
  int i;
  stp_parameter_list_t params = stp_get_parameter_list(v);
  size_t count = stp_parameter_list_count(params);
  STP_DEBUG(fprintf(stderr, "ijsgutenprint: Purging unused floating point parameters\n"));
  for (i = 0; i < count; i++)
    {
      const stp_parameter_t *param = stp_parameter_list_param(params, i);
      if (param->p_type == STP_PARAMETER_TYPE_DOUBLE &&
	  !param->read_only && param->is_active && !param->is_mandatory)
	{
	  size_t bytes = strlen(param->name) + strlen("Enable") + 1;
	  char *tmp = stp_malloc(bytes);
	  const char *value;
	  sprintf(tmp, "Enable%s", param->name);
	  STP_DEBUG(fprintf(stderr, "ijsgutenprint:   Looking for parameter %s\n", tmp));
	  value = stp_get_string_parameter(v, tmp);
	  if (value)
	    {
	      STP_DEBUG(fprintf(stderr, "ijsgutenprint:   Found %s: %s\n", tmp, value));
	      if (strcmp(value, "Disabled") == 0)
		{
		  STP_DEBUG(fprintf(stderr, "ijsgutenprint:     Clearing %s\n", param->name));
		  stp_clear_float_parameter(v, param->name);
		}
	    }
	  stp_free(tmp);
	}
      if (param->p_type == STP_PARAMETER_TYPE_DIMENSION &&
	  !param->read_only && param->is_active && !param->is_mandatory)
	{
	  size_t bytes = strlen(param->name) + strlen("Enable") + 1;
	  char *tmp = stp_malloc(bytes);
	  const char *value;
	  sprintf(tmp, "Enable%s", param->name);
	  STP_DEBUG(fprintf(stderr, "ijsgutenprint:   Looking for parameter %s\n", tmp));
	  value = stp_get_string_parameter(v, tmp);
	  if (value)
	    {
	      STP_DEBUG(fprintf(stderr, "ijsgutenprint:     Found %s: %s\n", tmp, value));
	      if (strcmp(value, "Disabled") == 0)
		{
		  STP_DEBUG(fprintf(stderr, "ijsgutenprint:     Clearing %s\n", param->name));
		  stp_clear_dimension_parameter(v, param->name);
		}
	    }
	  stp_free(tmp);
	}
    }
  stp_parameter_list_destroy(params);
}

static void
validate_options(stp_image_t *image)
{
  IMAGE *im = (IMAGE *) (image->rep);
  stp_vars_t *v = im->v;
  stp_parameter_list_t params = stp_get_parameter_list(v);
  int nparams = stp_parameter_list_count(params);
  int i;
  for (i = 0; i < nparams; i++)
    {
      const stp_parameter_t *param = stp_parameter_list_param(params, i);
      stp_parameter_t desc;
      stp_describe_parameter(v, param->name, &desc);
      if (desc.p_type == STP_PARAMETER_TYPE_STRING_LIST)
	{
	  if (!stp_string_list_is_present
	      (desc.bounds.str, stp_get_string_parameter(v, desc.name)))
	    {
	      STP_DEBUG(fprintf(stderr, "ijsgutenprint: clearing string %s (%s)\n",
				desc.name, safe_get_string_parameter(v, desc.name)));
	      stp_clear_string_parameter(v, desc.name);
	      if (!desc.read_only && desc.is_mandatory && desc.is_active)
		{
		  STP_DEBUG(fprintf(stderr, "ijsgutenprint: setting default string %s to %s\n",
				    desc.name, desc.deflt.str ? desc.deflt.str : "(null)"));
		  stp_set_string_parameter(v, desc.name, desc.deflt.str);
		  if (strcmp(desc.name, "PageSize") == 0)
		    {
		      const stp_papersize_t *ps =
			stp_get_papersize_by_name(desc.deflt.str);
		      if (ps->width > 0)
			{
			  STP_DEBUG(fprintf(stderr, "ijsgutenprint: setting page width to %d\n",
					    ps->width));
			  if (ps->width < stp_get_page_width(v))
			    stp_set_page_width(v, ps->width);
			  if (ps->width < stp_get_left(v) + stp_get_width(v))
			    {
#if 0
			      if (im->width < ps->width)
				im->width = ps->width;
#endif
			      STP_DEBUG(fprintf(stderr, "ijsgutenprint: setting width to %d\n",
						ps->width - stp_get_left(v)));
			      stp_set_width(v, ps->width - stp_get_left(v));
			    }
			}
		      if (ps->height > 0)
			{
			  STP_DEBUG(fprintf(stderr, "ijsgutenprint: setting page height to %d\n",
					    ps->height));
			  if (ps->height < stp_get_page_height(v))
			    stp_set_page_height(v, ps->height);
			  if (ps->height < stp_get_top(v) + stp_get_height(v))
			    {
#if 0
			      if (im->height < ps->height)
				im->height = ps->height;
#endif
			      STP_DEBUG(fprintf(stderr, "ijsgutenprint: setting height to %d\n",
						ps->height - stp_get_top(v)));
			      stp_set_height(v, ps->height - stp_get_top(v));
			    }
			}
		    }
		}
	    }
	}
      stp_parameter_description_destroy(&desc);
    }
  stp_parameter_list_destroy(params);
}  

  

int
main (int argc, char **argv)
{
  IjsPageHeader ph;
  int status;
  int page = 0;
  IMAGE img;
  stp_image_t si;
  const stp_printer_t *printer = NULL;
  FILE *f = NULL;
  int l, t, r, b, w, h;
  int width, height;

  if (getenv("STP_SUPPRESS_MESSAGES"))
    suppress_messages = 1;

  if (getenv("STP_DEBUG_STARTUP"))
    while (SDEBUG)
      ;
    
  memset(&img, 0, sizeof(img));

  stp_init();
  version_id = stp_get_version();
  option_remap_list = stp_string_list_create();

  img.ctx = ijs_server_init();
  if (img.ctx == NULL)
    return 1;

  img.v = stp_vars_create();
  if (img.v == NULL)
    {
      ijs_server_done(img.ctx);
      return 1;
    }
  stp_set_top(img.v, 0);
  stp_set_left(img.v, 0);

  /* Error messages to stderr. */
  stp_set_errfunc(img.v, gutenprint_errfunc);
  stp_set_errdata(img.v, stderr);

  /* Printer data goes to file f, but we haven't opened it yet. */
  stp_set_outfunc(img.v, gutenprint_outfunc);
  stp_set_outdata(img.v, NULL);

  memset(&si, 0, sizeof(si));
  si.width = gutenprint_image_width;
  si.height = gutenprint_image_height;
  si.get_row = gutenprint_image_get_row;
  si.get_appname = gutenprint_image_get_appname;
  si.rep = &img;

  ijs_server_install_status_cb (img.ctx, gutenprint_status_cb, &img);
  ijs_server_install_list_cb (img.ctx, gutenprint_list_cb, &img);
  ijs_server_install_enum_cb (img.ctx, gutenprint_enum_cb, &img);
  ijs_server_install_get_cb (img.ctx, gutenprint_get_cb, &img);
  ijs_server_install_set_cb(img.ctx, gutenprint_set_cb, &img);

  stp_dbg("ijsgutenprint: about to start\n", img.v);

  STP_DEBUG(fprintf(stderr, "ijsgutenprint: About to get page header\n"));
  status = ijs_server_get_page_header(img.ctx, &ph);
  while (status == 0)
    {
      stp_vars_t *old_v = NULL;
      STP_DEBUG(fprintf(stderr, "ijsgutenprint: got page header, %d x %d\n",
			ph.width, ph.height));
      stp_dbg("ijsgutenprint: have page header\n", img.v);

      status = image_init(&img, &ph);
      if (status)
	{
	  fprintf(stderr, "ERROR: ijsgutenprint: image_init failed %d\n", status);
	  break;
	}

      if (page == 0)
	{
	  if (img.fd)
	    {
	      f = fdopen(img.fd - 1, "wb");
	      if (!f)
		{
		  fprintf(stderr, "ERROR: ijsgutenprint: Unable to open file descriptor: %s\n",
			  strerror(errno));
		  status = -1;
		  break;
		}
	    }
	  else if (img.filename && strlen(img.filename) > 0)
	    {
	      f = fopen(img.filename, "wb");
	      if (!f)
		{
		  status = -1;
		  fprintf(stderr, "ERROR: ijsgutenprint: Unable to open %s: %s\n", img.filename,
			  strerror(errno));
		  break;
		}
	    }

	  /* Printer data to file */
	  stp_set_outdata(img.v, f);
	}

      printer = stp_get_printer(img.v);
      if (printer == NULL)
	{
	  fprintf(stderr, "ERROR: ijsgutenprint: Unknown printer %s\n",
		  stp_get_driver(img.v));
	  status = -1;
	  break;
	}
      purge_unused_float_parameters(img.v);
      stp_merge_printvars(img.v, stp_printer_get_defaults(printer));


      img.total_bytes = (double) ((ph.n_chan * ph.bps * ph.width + 7) >> 3)
	* (double) ph.height;
      img.bytes_left = img.total_bytes;

      stp_set_float_parameter(img.v, "AppGamma", 1.0);
      stp_get_media_size(img.v, &w, &h);
      stp_get_imageable_area(img.v, &l, &r, &b, &t);
      STP_DEBUG(fprintf(stderr, "ijsgutenprint: chan %d bps %d image w %d %d h %d %d\n",
			ph.n_chan, ph.bps, stp_get_width(img.v), img.width,
			stp_get_height(img.v), img.height));
      if (ppd_mode)
	{
	  int lt, rt, bt, tt;

	  stp_get_maximum_imageable_area(img.v, &lt, &rt, &bt, &tt);
	  STP_DEBUG(fprintf(stderr, "ijsgutenprint: w %d h %d l %d %d t %d %d r %d %d b %d %d\n",
			    w, h, l, lt, t, tt, r, rt, b, bt));
	  if (lt < 0)
	    lt = 0;
	  if (tt < 0)
	    tt = 0;
	  if (rt > w)
	    rt = w;
	  if (bt > h)
	    bt = h;
	  if (l < 0)
	    l = 0;
	  if (t < 0)
	    t = 0;
	  if (r > w + l)
	    r = w + l;
	  if (b > h + t)
	    b = h + t;
	  STP_DEBUG(fprintf(stderr, "ijsgutenprint: w %d h %d l %d %d t %d %d r %d %d b %d %d\n",
			    w, h, l, lt, t, tt, r, rt, b, bt));
	  if (lt < l)
	    {
	      STP_DEBUG(fprintf(stderr, "ijsgutenprint: l %d, lt %d\n", l, lt));
	      img.left_margin = (l - lt) * ph.xres * ph.n_chan * ph.bps / 8 / 72;
	      img.width -= (l - lt) * ph.xres / 72;
	      STP_DEBUG(fprintf(stderr, "ijsgutenprint: chan %d bps %d image w %d %d h %d %d\n",
				ph.n_chan, ph.bps, stp_get_width(img.v), img.width,
				stp_get_height(img.v), img.height));
	    }
	  else
	    img.left_margin = 0;
	  stp_set_left(img.v, l);
	  if (tt < t)
	    {
	      STP_DEBUG(fprintf(stderr, "ijsgutenprint: t %d, tt %d\n", t, tt));
	      img.top_margin = (t - tt) * ph.yres * ph.n_chan * ph.bps / 8 / 72;
	      img.height -= (t - tt) * ph.yres / 72;
	      STP_DEBUG(fprintf(stderr, "ijsgutenprint: chan %d bps %d image w %d %d h %d %d\n",
				ph.n_chan, ph.bps, stp_get_width(img.v), img.width,
				stp_get_height(img.v), img.height));
	    }
	  else
	    img.top_margin = 0;
	  stp_set_top(img.v, t);
	  if (rt > r)
	    {
	      STP_DEBUG(fprintf(stderr, "ijsgutenprint: r %d, rt %d\n", r, rt));
	      img.right_margin = (rt - r) * ph.xres * ph.n_chan * ph.bps / 8 / 72;
	      img.width -= (rt - r) * ph.xres / 72;
	      STP_DEBUG(fprintf(stderr, "ijsgutenprint: chan %d bps %d image w %d %d h %d %d\n",
				ph.n_chan, ph.bps, stp_get_width(img.v), img.width,
				stp_get_height(img.v), img.height));
	    }
	  else
	    img.right_margin = 0;
	  if (bt > b)
	    {
	      STP_DEBUG(fprintf(stderr, "ijsgutenprint: b %d, bt %d\n", b, bt));
	      img.bottom_margin = (bt - b) * ph.yres * ph.n_chan * ph.bps / 8 / 72;
	      img.height -= (bt - b) * ph.yres / 72;
	      STP_DEBUG(fprintf(stderr, "ijsgutenprint: chan %d bps %d image w %d %d h %d %d\n",
				ph.n_chan, ph.bps, stp_get_width(img.v), img.width,
				stp_get_height(img.v), img.height));
	    }
	  else
	    img.bottom_margin = 0;
	}
      if (l < 0)
	width = r;
      else
	width = r - l;
      stp_set_width(img.v, width);
      if (t < 0)
	height = b;
      else
	height = b - t;
      img.row_width -= img.left_margin;
      img.row_width -= img.right_margin;
      stp_set_height(img.v, height);
      stp_set_int_parameter(img.v, "PageNumber", page);
      STP_DEBUG(fprintf(stderr, "ijsgutenprint: w %d h %d l %d r %d t %d b %d\n",
			width, height, l, r, t, b));
      STP_DEBUG(fprintf(stderr, "ijsgutenprint: chan %d bps %d image w %d %d h %d %d\n",
			ph.n_chan, ph.bps, stp_get_width(img.v), img.width,
			stp_get_height(img.v), img.height));
      STP_DEBUG(fprintf(stderr, "ijsgutenprint: margins l %d r %d t %d b %d row width %d\n",
			img.left_margin, img.right_margin,
			img.top_margin, img.bottom_margin,
			img.row_width));

/* 
 * Fix up the duplex/tumble settings stored in the "x_" parameters
 * If Duplex is "true" then look at "Tumble". If duplex is not "true" or "false"
 * then just take it (e.g. Duplex=DuplexNoTumble).
 */
      STP_DEBUG(fprintf(stderr, "ijsgutenprint: x_Duplex=%s\n", safe_get_string_parameter(img.v, "x_Duplex")));
      STP_DEBUG(fprintf(stderr, "ijsgutenprint: x_Tumble=%s\n", safe_get_string_parameter(img.v, "x_Tumble")));

      if (stp_get_string_parameter(img.v, "x_Duplex"))
        {
          if (strcmp(stp_get_string_parameter(img.v, "x_Duplex"), "false") == 0)
            stp_set_string_parameter(img.v, "Duplex", "None");
          else if (strcmp(stp_get_string_parameter(img.v, "x_Duplex"), "true") == 0)
            {
              if (stp_get_string_parameter(img.v, "x_Tumble"))
              {
                if (strcmp(stp_get_string_parameter(img.v, "x_Tumble"), "false") == 0)
                  stp_set_string_parameter(img.v, "Duplex", "DuplexNoTumble");
                else
                  stp_set_string_parameter(img.v, "Duplex", "DuplexTumble");
              }
            else	/* Tumble missing, assume false */
              stp_set_string_parameter(img.v, "Duplex", "DuplexNoTumble");
            }
          else	/* Not true or false */
            stp_set_string_parameter(img.v, "Duplex", stp_get_string_parameter(img.v, "x_Duplex"));
        }

/* can I destroy the unused parameters? */

      STP_DEBUG(fprintf(stderr, "ijsgutenprint: Duplex=%s\n", safe_get_string_parameter(img.v, "Duplex")));

      validate_options(&si);
      stp_dbg("ijsgutenprint: about to print", img.v);
      STP_DEBUG(fprintf(stderr, "ijsgutenprint: w %d h %d l %d t %d\n",
			stp_get_width(img.v), stp_get_height(img.v),
			stp_get_left(img.v), stp_get_top(img.v)));
      STP_DEBUG(fprintf(stderr, "ijsgutenprint: start printing page %d\n", page));
      print_messages_as_errors = 1;
      if (!version_is_ok)
	{
	  fprintf(stderr, VERSION_MISMATCH, version_id,
		  gutenprint_ppd_version, version_id, gutenprint_ppd_version);
	  status = IJS_ERANGE;
	  break;
	}
      else if (stp_verify(img.v))
	{
	  page_bytes_printed = 0;
	  if (page == 0)
	    stp_start_job(img.v, &si);
	  stp_print(img.v, &si);
	  STP_DEBUG(fprintf(stderr, "ijsgutenprint: printed page %d, %.0f bytes\n",
			    page, page_bytes_printed));
	  old_v = stp_vars_create_copy(img.v);
	}
      else
	{
	  fprintf(stderr, "ERROR: ijsgutenprint: Bad parameters; cannot continue!\n");
	  status = IJS_ERANGE;
	  break;
	}
      if (job_aborted)
	{
	  STP_DEBUG(fprintf(stderr, "ijsgutenprint: aborting job\n"));
	  status = 1;
	}
      else
	{
	  STP_DEBUG(fprintf(stderr, "ijsgutenprint: done printing page %d\n", page));

	  while (img.bytes_left)
	    {
	      status = image_next_row(&img);
	      if (status)
		{
		  fprintf(stderr, "ERROR: ijsgutenprint: Get next row failed at %.0f\n",
			  img.bytes_left);
		  break;
		}
	    }

	  image_finish(&img);
	  status = ijs_server_get_page_header(img.ctx, &ph);
	}
      if (status > 0)
	{
	  fprintf(stderr, "INFO: ijsgutenprint Ready to print.\n");
	  stp_end_job(old_v, &si);
	}
      else
	{
	  stp_vars_destroy(old_v);
	  page++;
	}
    }
  if (f)
    {
      fclose(f);
    }

  if (status > 0)
    status = 0; /* normal exit */

  ijs_server_done(img.ctx);

  STP_DEBUG(fprintf (stderr, "ijsgutenprint: printed total %.0f bytes\n",
		     total_bytes_printed));
  STP_DEBUG(fprintf (stderr, "ijsgutenprint: server exiting with status %d\n", status));
  return status;
}
