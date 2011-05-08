/*
 * "$Id: rastertoprinter.c,v 1.137 2011/04/09 00:19:59 rlk Exp $"
 *
 *   Gutenprint based raster filter for the Common UNIX Printing System.
 *
 *   Copyright 1993-2008 by Mike Sweet.
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
 * Contents:
 *
 *   main()                    - Main entry and processing of driver.
 *   cups_writefunc()          - Write data to a file...
 *   cancel_job()              - Cancel the current job...
 *   Image_get_appname()       - Get the application we are running.
 *   Image_get_row()           - Get one row of the image.
 *   Image_height()            - Return the height of an image.
 *   Image_init()              - Initialize an image.
 *   Image_conclude()          - Close the progress display.
 *   Image_width()             - Return the width of an image.
 */

/*
 * Include necessary headers...
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <cups/cups.h>
#include <cups/ppd.h>
#include <cups/raster.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/times.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include "i18n.h"

/* Solaris with gcc has problems because gcc's limits.h doesn't #define */
/* this */
#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

/*
 * Structure for page raster data...
 */

typedef struct
{
  cups_raster_t		*ras;		/* Raster stream to read from */
  int			page;		/* Current page number */
  int			row;		/* Current row number */
  int			left;
  int			right;
  int			bottom;
  int			top;
  int			width;
  int			height;
  int			left_trim;
  int			right_trim;
  int			top_trim;
  int			bottom_trim;
  int			adjusted_width;
  int			adjusted_height;
  int			last_percent;
  int			shrink_to_fit;
  cups_page_header_t	header;		/* Page header from file */
} cups_image_t;

static void	cups_writefunc(void *file, const char *buf, size_t bytes);
static void	cups_errfunc(void *file, const char *buf, size_t bytes);
static void	cancel_job(int sig);
static const char *Image_get_appname(stp_image_t *image);
static stp_image_status_t Image_get_row(stp_image_t *image,
					unsigned char *data,
					size_t byte_limit, int row);
static int	Image_height(stp_image_t *image);
static int	Image_width(stp_image_t *image);
static void	Image_conclude(stp_image_t *image);
static void	Image_init(stp_image_t *image);

static stp_image_t theImage =
{
  Image_init,
  NULL,				/* reset */
  Image_width,
  Image_height,
  Image_get_row,
  Image_get_appname,
  Image_conclude,
  NULL
};

static volatile stp_image_status_t Image_status = STP_IMAGE_STATUS_OK;
static double total_bytes_printed = 0;
static int print_messages_as_errors = 0;
static int suppress_messages = 0;
static stp_string_list_t *po = NULL;

static void
set_string_parameter(stp_vars_t *v, const char *name, const char *val)
{
  if (! suppress_messages)
    fprintf(stderr, "DEBUG: Gutenprint:   Set special string %s to %s\n", name, val);
  stp_set_string_parameter(v, name, val);
}


static void
set_special_parameter(stp_vars_t *v, const char *name, int choice)
{
  stp_parameter_t desc;
  stp_describe_parameter(v, name, &desc);
  if (desc.p_type == STP_PARAMETER_TYPE_STRING_LIST)
    {
      if (choice < 0)
	{
	  stp_clear_string_parameter(v, name);
	  if (! suppress_messages)
	    fprintf(stderr, "DEBUG: Gutenprint:   Clear special parameter %s\n",
		    name);
	}
      else if (choice >= stp_string_list_count(desc.bounds.str))
	{
	  if (! suppress_messages)
	    stp_i18n_printf(po, _("ERROR: Unable to set Gutenprint option %s "
	                          "(%d > %d)!\n"), name, choice,
				  stp_string_list_count(desc.bounds.str));
	}
      else
	{
	  stp_set_string_parameter
	    (v, name, stp_string_list_param(desc.bounds.str, choice)->name);
	  if (! suppress_messages)
	    fprintf(stderr, "DEBUG: Gutenprint:   Set special parameter %s to choice %d (%s)\n",
		    name, choice,
		    stp_string_list_param(desc.bounds.str, choice)->name);
	}
    }
  else
    {
      if (! suppress_messages)
	fprintf(stderr, "DEBUG: Gutenprint:   Unable to set special %s: not a string\n",
		name);
    }
  stp_parameter_description_destroy(&desc);
}

static void
print_debug_block(const stp_vars_t *v, const cups_image_t *cups)
{
  stp_parameter_list_t params;
  int nparams;
  int i;
  fprintf(stderr, "DEBUG: Gutenprint: Page data:\n");
  fprintf(stderr, "DEBUG: Gutenprint:   MediaClass = \"%s\"\n", cups->header.MediaClass);
  fprintf(stderr, "DEBUG: Gutenprint:   MediaColor = \"%s\"\n", cups->header.MediaColor);
  fprintf(stderr, "DEBUG: Gutenprint:   MediaType = \"%s\"\n", cups->header.MediaType);
  fprintf(stderr, "DEBUG: Gutenprint:   OutputType = \"%s\"\n", cups->header.OutputType);

  fprintf(stderr, "DEBUG: Gutenprint:   AdvanceDistance = %d\n", cups->header.AdvanceDistance);
  fprintf(stderr, "DEBUG: Gutenprint:   AdvanceMedia = %d\n", cups->header.AdvanceMedia);
  fprintf(stderr, "DEBUG: Gutenprint:   Collate = %d\n", cups->header.Collate);
  fprintf(stderr, "DEBUG: Gutenprint:   CutMedia = %d\n", cups->header.CutMedia);
  fprintf(stderr, "DEBUG: Gutenprint:   Duplex = %d\n", cups->header.Duplex);
  fprintf(stderr, "DEBUG: Gutenprint:   HWResolution = [ %d %d ]\n", cups->header.HWResolution[0],
	  cups->header.HWResolution[1]);
  fprintf(stderr, "DEBUG: Gutenprint:   ImagingBoundingBox = [ %d %d %d %d ]\n",
	  cups->header.ImagingBoundingBox[0], cups->header.ImagingBoundingBox[1],
	  cups->header.ImagingBoundingBox[2], cups->header.ImagingBoundingBox[3]);
  fprintf(stderr, "DEBUG: Gutenprint:   InsertSheet = %d\n", cups->header.InsertSheet);
  fprintf(stderr, "DEBUG: Gutenprint:   Jog = %d\n", cups->header.Jog);
  fprintf(stderr, "DEBUG: Gutenprint:   LeadingEdge = %d\n", cups->header.LeadingEdge);
  fprintf(stderr, "DEBUG: Gutenprint:   Margins = [ %d %d ]\n", cups->header.Margins[0],
	  cups->header.Margins[1]);
  fprintf(stderr, "DEBUG: Gutenprint:   ManualFeed = %d\n", cups->header.ManualFeed);
  fprintf(stderr, "DEBUG: Gutenprint:   MediaPosition = %d\n", cups->header.MediaPosition);
  fprintf(stderr, "DEBUG: Gutenprint:   MediaWeight = %d\n", cups->header.MediaWeight);
  fprintf(stderr, "DEBUG: Gutenprint:   MirrorPrint = %d\n", cups->header.MirrorPrint);
  fprintf(stderr, "DEBUG: Gutenprint:   NegativePrint = %d\n", cups->header.NegativePrint);
  fprintf(stderr, "DEBUG: Gutenprint:   NumCopies = %d\n", cups->header.NumCopies);
  fprintf(stderr, "DEBUG: Gutenprint:   Orientation = %d\n", cups->header.Orientation);
  fprintf(stderr, "DEBUG: Gutenprint:   OutputFaceUp = %d\n", cups->header.OutputFaceUp);
  fprintf(stderr, "DEBUG: Gutenprint:   PageSize = [ %d %d ]\n", cups->header.PageSize[0],
	  cups->header.PageSize[1]);
  fprintf(stderr, "DEBUG: Gutenprint:   Separations = %d\n", cups->header.Separations);
  fprintf(stderr, "DEBUG: Gutenprint:   TraySwitch = %d\n", cups->header.TraySwitch);
  fprintf(stderr, "DEBUG: Gutenprint:   Tumble = %d\n", cups->header.Tumble);
  fprintf(stderr, "DEBUG: Gutenprint:   cupsWidth = %d\n", cups->header.cupsWidth);
  fprintf(stderr, "DEBUG: Gutenprint:   cupsHeight = %d\n", cups->header.cupsHeight);
  fprintf(stderr, "DEBUG: Gutenprint:   cups->width = %d\n", cups->width);
  fprintf(stderr, "DEBUG: Gutenprint:   cups->height = %d\n", cups->height);
  fprintf(stderr, "DEBUG: Gutenprint:   cups->adjusted_width = %d\n", cups->adjusted_width);
  fprintf(stderr, "DEBUG: Gutenprint:   cups->adjusted_height = %d\n", cups->adjusted_height);
  fprintf(stderr, "DEBUG: Gutenprint:   cupsMediaType = %d\n", cups->header.cupsMediaType);
  fprintf(stderr, "DEBUG: Gutenprint:   cupsBitsPerColor = %d\n", cups->header.cupsBitsPerColor);
  fprintf(stderr, "DEBUG: Gutenprint:   cupsBitsPerPixel = %d\n", cups->header.cupsBitsPerPixel);
  fprintf(stderr, "DEBUG: Gutenprint:   cupsBytesPerLine = %d\n", cups->header.cupsBytesPerLine);
  fprintf(stderr, "DEBUG: Gutenprint:   cupsColorOrder = %d\n", cups->header.cupsColorOrder);
  fprintf(stderr, "DEBUG: Gutenprint:   cupsColorSpace = %d\n", cups->header.cupsColorSpace);
  fprintf(stderr, "DEBUG: Gutenprint:   cupsCompression = %d\n", cups->header.cupsCompression);
  fprintf(stderr, "DEBUG: Gutenprint:   cupsRowCount = %d\n", cups->header.cupsRowCount);
  fprintf(stderr, "DEBUG: Gutenprint:   cupsRowFeed = %d\n", cups->header.cupsRowFeed);
  fprintf(stderr, "DEBUG: Gutenprint:   cupsRowStep = %d\n", cups->header.cupsRowStep);
  fprintf(stderr, "DEBUG: Gutenprint:   stp_get_driver(v) |%s|\n", stp_get_driver(v));
  fprintf(stderr, "DEBUG: Gutenprint:   stp_get_left(v) %d\n", stp_get_left(v));
  fprintf(stderr, "DEBUG: Gutenprint:   stp_get_top(v) %d\n", stp_get_top(v));
  fprintf(stderr, "DEBUG: Gutenprint:   stp_get_page_width(v) %d\n", stp_get_page_width(v));
  fprintf(stderr, "DEBUG: Gutenprint:   stp_get_page_height(v) %d\n", stp_get_page_height(v));
  fprintf(stderr, "DEBUG: Gutenprint:   shrink page to fit %d\n", cups->shrink_to_fit);
  params = stp_get_parameter_list(v);
  nparams = stp_parameter_list_count(params);
  for (i = 0; i < nparams; i++)
    {
      const stp_parameter_t *p = stp_parameter_list_param(params, i);
      switch (p->p_type)
	{
	case STP_PARAMETER_TYPE_STRING_LIST:
	  fprintf(stderr, "DEBUG: Gutenprint:   stp_get_string %s(v) |%s| %d\n",
		  p->name, stp_get_string_parameter(v, p->name) ?
		  stp_get_string_parameter(v, p->name) : "NULL",
		  stp_get_string_parameter_active(v, p->name));
	  break;
	case STP_PARAMETER_TYPE_DOUBLE:
	  fprintf(stderr, "DEBUG: Gutenprint:   stp_get_float %s(v) |%.3f| %d\n",
		  p->name, stp_get_float_parameter(v, p->name),
		  stp_get_float_parameter_active(v, p->name));
	  break;
	case STP_PARAMETER_TYPE_DIMENSION:
	  fprintf(stderr, "DEBUG: Gutenprint:   stp_get_dimension %s(v) |%d| %d\n",
		  p->name, stp_get_dimension_parameter(v, p->name),
		  stp_get_dimension_parameter_active(v, p->name));
	  break;
	case STP_PARAMETER_TYPE_INT:
	  fprintf(stderr, "DEBUG: Gutenprint:   stp_get_int %s(v) |%d| %d\n",
		  p->name, stp_get_int_parameter(v, p->name),
		  stp_get_int_parameter_active(v, p->name));
	  break;
	case STP_PARAMETER_TYPE_BOOLEAN:
	  fprintf(stderr, "DEBUG: Gutenprint:   stp_get_boolean %s(v) |%d| %d\n",
		  p->name, stp_get_boolean_parameter(v, p->name),
		  stp_get_boolean_parameter_active(v, p->name));
	  break;
	  /*
	   * We don't handle raw, curve, or filename arguments.
	   */
	default:
	  break;
	}
    }
  fprintf(stderr, "DEBUG: Gutenprint: End page data\n");
  stp_parameter_list_destroy(params);
}

static int
printer_supports_bw(const stp_vars_t *v)
{
  stp_parameter_t desc;
  int status = 0;
  stp_describe_parameter(v, "PrintingMode", &desc);
  if (stp_string_list_is_present(desc.bounds.str, "BW"))
    status = 1;
  stp_parameter_description_destroy(&desc);
  return status;
}

static void
validate_options(stp_vars_t *v, cups_image_t *cups)
{
  stp_parameter_list_t params = stp_get_parameter_list(v);
  int nparams = stp_parameter_list_count(params);
  int i;
  if (! suppress_messages)
    fprintf(stderr, "DEBUG: Gutenprint: Validating options\n");
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
	      if (! suppress_messages)
		{
		  const char *val = stp_get_string_parameter(v, desc.name);
		  fprintf(stderr, "DEBUG: Gutenprint:   Clearing string %s (%s)\n",
			  desc.name, val ? val : "(null)");
		}
	      stp_clear_string_parameter(v, desc.name);
	      if (!desc.read_only && desc.is_mandatory && desc.is_active)
		{
		  if (! suppress_messages)
		    fprintf(stderr, "DEBUG: Gutenprint:   Setting default string %s to %s\n",
			    desc.name, desc.deflt.str ? desc.deflt.str : "(null)");
		  stp_set_string_parameter(v, desc.name, desc.deflt.str);
		  if (strcmp(desc.name, "PageSize") == 0)
		    {
		      const stp_papersize_t *ps =
			stp_get_papersize_by_name(desc.deflt.str);
		      if (ps->width > 0)
			{
			  if (! suppress_messages)
			    fprintf(stderr, "DEBUG: Gutenprint:   Setting page width to %d\n",
				    ps->width);
			  if (ps->width < stp_get_page_width(v))
			    stp_set_page_width(v, ps->width);
			}
		      if (ps->height > 0)
			{
			  if (! suppress_messages)
			    fprintf(stderr, "DEBUG: Gutenprint:   Setting page height to %d\n",
				    ps->height);
			  if (ps->height < stp_get_page_height(v))
			    stp_set_page_height(v, ps->height);
			}
		    }
		}
	    }
	}
      stp_parameter_description_destroy(&desc);
    }
  if (! suppress_messages)
    fprintf(stderr, "DEBUG: Gutenprint: Done validating options\n");
  stp_parameter_list_destroy(params);
}

static stp_vars_t *
initialize_page(cups_image_t *cups, const stp_vars_t *default_settings,
		const char *page_size_name)
{
  int tmp_left, tmp_right, tmp_top, tmp_bottom, tmp_width, tmp_height;
  stp_vars_t *v = stp_vars_create_copy(default_settings);

  if (! suppress_messages)
    fprintf(stderr, "DEBUG: Gutenprint: Initialize page\n");

  stp_set_outfunc(v, cups_writefunc);
  stp_set_errfunc(v, cups_errfunc);
  stp_set_outdata(v, stdout);
  stp_set_errdata(v, stderr);

  if (cups->header.cupsBitsPerColor == 16)
    set_string_parameter(v, "ChannelBitDepth", "16");
  else
    set_string_parameter(v, "ChannelBitDepth", "8");
  switch (cups->header.cupsColorSpace)
    {
    case CUPS_CSPACE_W :
      /* DyeSub photo printers don't support black & white ink! */
      if (printer_supports_bw(v))
	set_string_parameter(v, "PrintingMode", "BW");
      set_string_parameter(v, "InputImageType", "Whitescale");
      break;
    case CUPS_CSPACE_K :
      /* DyeSub photo printers don't support black & white ink! */
      if (printer_supports_bw(v))
	set_string_parameter(v, "PrintingMode", "BW");
      set_string_parameter(v, "InputImageType", "Grayscale");
      break;
    case CUPS_CSPACE_RGB :
      set_string_parameter(v, "PrintingMode", "Color");
      set_string_parameter(v, "InputImageType", "RGB");
      break;
    case CUPS_CSPACE_CMY :
      set_string_parameter(v, "PrintingMode", "Color");
      set_string_parameter(v, "InputImageType", "CMY");
      break;
    case CUPS_CSPACE_CMYK :
      set_string_parameter(v, "PrintingMode", "Color");
      set_string_parameter(v, "InputImageType", "CMYK");
      break;
    case CUPS_CSPACE_KCMY :
      set_string_parameter(v, "PrintingMode", "Color");
      set_string_parameter(v, "InputImageType", "KCMY");
      break;
    default :
      stp_i18n_printf(po, _("ERROR: Gutenprint detected a bad colorspace "
                            "(%d)!\n"), cups->header.cupsColorSpace);
      break;
    }

  set_special_parameter(v, "Resolution", cups->header.cupsCompression - 1);

  set_special_parameter(v, "Quality", cups->header.cupsRowFeed - 1);

  if (cups->header.MediaClass && strlen(cups->header.MediaClass) > 0)
    set_string_parameter(v, "InputSlot", cups->header.MediaClass);

  if (cups->header.MediaType && strlen(cups->header.MediaType) > 0)
    set_string_parameter(v, "MediaType", cups->header.MediaType);

  if (! suppress_messages)
    fprintf(stderr, "DEBUG: Gutenprint:   PageSize = %dx%d\n", cups->header.PageSize[0],
	    cups->header.PageSize[1]);

  if (page_size_name)
    {
      if (strcmp(page_size_name, "Custom") == 0)
	{
	  if (!suppress_messages)
	    fprintf(stderr, "DEBUG: Gutenprint:   Using custom page size for (%d, %d)\n",
		    cups->header.PageSize[1], cups->header.PageSize[0]);
	  stp_set_page_width(v, cups->header.PageSize[0]);
	  stp_set_page_height(v, cups->header.PageSize[1]);
	}
      else if (stp_get_papersize_by_name(page_size_name))
	{
	  int width, height;
	  if (!suppress_messages)
	    fprintf(stderr, "DEBUG: Gutenprint:   Using page size %s with (%d, %d)\n",
		    page_size_name, cups->header.PageSize[1], cups->header.PageSize[0]);
	  set_string_parameter(v, "PageSize", page_size_name);
	  stp_get_media_size(v, &width, &height);
	  if (width > 0)
	    stp_set_page_width(v, width);
	  else
	    stp_set_page_width(v, cups->header.PageSize[0]);
	  if (height > 0)
	    stp_set_page_height(v, height);
	  else
	    stp_set_page_height(v, cups->header.PageSize[1]);
	}
      else
	{
	  if (!suppress_messages)
	    fprintf(stderr, "DEBUG: Gutenprint:   Can't find page size %s with (%d, %d), using custom page size\n",
		    page_size_name, cups->header.PageSize[1], cups->header.PageSize[0]);
	  stp_set_page_width(v, cups->header.PageSize[0]);
	  stp_set_page_height(v, cups->header.PageSize[1]);
	}
    }
  else
    {
      if (! suppress_messages)
	fprintf(stderr, "DEBUG: Gutenprint:   No named media size for (%d, %d)\n",
		cups->header.PageSize[1], cups->header.PageSize[0]);
      stp_set_page_width(v, cups->header.PageSize[0]);
      stp_set_page_height(v, cups->header.PageSize[1]);
    }

 /*
  * Duplex
  * Note that the names MUST match those in the printer driver(s)
  */

  if (cups->header.Duplex != 0)
    {
      if (cups->header.Tumble != 0)
        set_string_parameter(v, "Duplex", "DuplexTumble");
      else
        set_string_parameter(v, "Duplex", "DuplexNoTumble");
    }

  cups->shrink_to_fit = stp_get_int_parameter(v, "CUPSShrinkPage");

  set_string_parameter(v, "JobMode", "Job");
  validate_options(v, cups);
  stp_get_media_size(v, &(cups->width), &(cups->height));
  stp_get_maximum_imageable_area(v, &tmp_left, &tmp_right,
				 &tmp_bottom, &tmp_top);
  stp_get_imageable_area(v, &(cups->left), &(cups->right),
			 &(cups->bottom), &(cups->top));
  if (! suppress_messages)
    {
      fprintf(stderr, "DEBUG:   Gutenprint: limits w %d l %d r %d  h %d t %d b %d\n",
	      cups->width, cups->left, cups->right, cups->height, cups->top, cups->bottom);
      fprintf(stderr, "DEBUG:   Gutenprint: max limits l %d r %d t %d b %d\n",
	      tmp_left, tmp_right, tmp_top, tmp_bottom);
    }

  if (tmp_left < 0)
    tmp_left = 0;
  if (tmp_top < 0)
    tmp_top = 0;
  if (tmp_right > tmp_left + cups->width)
    tmp_right = cups->width;
  if (tmp_bottom > tmp_top + cups->height)
    tmp_bottom = cups->height;
  tmp_width = cups->right - cups->left;
  tmp_height = cups->bottom - cups->top;
  if (tmp_left < cups->left)
    {
      if (cups->shrink_to_fit != 1)
	{
	  cups->left_trim = cups->left - tmp_left;
	  tmp_left = cups->left;
	}
      else
	cups->left_trim = 0;
      if (! suppress_messages)
	fprintf(stderr, "DEBUG:   Gutenprint: left margin %d\n", cups->left_trim);
    }
  else
    {
      cups->left_trim = 0;
      if (! suppress_messages)
	fprintf(stderr, "DEBUG: Gutenprint:   Adjusting left margin from %d to %d\n",
		cups->left, tmp_left);
      cups->left = tmp_left;
    }
  if (tmp_right > cups->right)
    {
      if (cups->shrink_to_fit != 1)
	{
	  cups->right_trim = tmp_right - cups->right;
	  tmp_right = cups->right;
	}
      else
	cups->right_trim = 0;
      if (! suppress_messages)
	fprintf(stderr, "DEBUG: Gutenprint:   right margin %d\n", cups->right_trim);
    }
  else
    {
      cups->right_trim = 0;
      if (! suppress_messages)
	fprintf(stderr, "DEBUG: Gutenprint:   Adjusting right margin from %d to %d\n",
		cups->right, tmp_right);
      cups->right = tmp_right;
    }
  if (tmp_top < cups->top)
    {
      if (cups->shrink_to_fit != 1)
	{
	  cups->top_trim = cups->top - tmp_top;
	  tmp_top = cups->top;
	}
      else
	cups->top_trim = 0;
      if (! suppress_messages)
	fprintf(stderr, "DEBUG: Gutenprint:   top margin %d\n", cups->top_trim);
    }
  else
    {
      cups->top_trim = 0;
      if (! suppress_messages)
	fprintf(stderr, "DEBUG: Gutenprint:   Adjusting top margin from %d to %d\n",
		cups->top, tmp_top);
      cups->top = tmp_top;
    }
  if (tmp_bottom > cups->bottom)
    {
      if (cups->shrink_to_fit != 1)
	{
	  cups->bottom_trim = tmp_bottom - cups->bottom;
	  tmp_bottom = cups->bottom;
	}
      else
	cups->bottom_trim = 0;
      if (! suppress_messages)
	fprintf(stderr, "DEBUG: Gutenprint:   bottom margin %d\n", cups->bottom_trim);
    }
  else
    {
      cups->bottom_trim = 0;
      if (! suppress_messages)
	fprintf(stderr, "DEBUG: Gutenprint:   Adjusting bottom margin from %d to %d\n",
		cups->bottom, tmp_bottom);
      cups->bottom = tmp_bottom;
    }

  if (cups->shrink_to_fit == 2)
    {
      int t_left, t_right, t_bottom, t_top;
      stp_get_imageable_area(v, &(t_left), &(t_right), &(t_bottom), &(t_top));
      stp_set_width(v, t_right - t_left);
      stp_set_height(v, t_bottom - t_top);
      stp_set_left(v, t_left);
      stp_set_top(v, t_top);
    }
  else
    {
      stp_set_width(v, cups->right - cups->left);
      stp_set_height(v, cups->bottom - cups->top);
      stp_set_left(v, cups->left);
      stp_set_top(v, cups->top);
    }

  cups->right = cups->width - cups->right;
  if (cups->shrink_to_fit == 1)
    cups->width = tmp_right - tmp_left;
  else
    cups->width = cups->width - cups->left - cups->right;
  cups->width = cups->header.HWResolution[0] * cups->width / 72;
  cups->left = cups->header.HWResolution[0] * cups->left / 72;
  cups->right = cups->header.HWResolution[0] * cups->right / 72;
  cups->left_trim = cups->header.HWResolution[0] * cups->left_trim / 72;
  cups->right_trim = cups->header.HWResolution[0] * cups->right_trim / 72;
  cups->adjusted_width = cups->width;
  if (cups->adjusted_width > cups->header.cupsWidth)
    cups->adjusted_width = cups->header.cupsWidth;

  cups->bottom = cups->height - cups->bottom;
  if (cups->shrink_to_fit == 1)
    cups->height = tmp_bottom - tmp_top;
  else
    cups->height = cups->height - cups->top - cups->bottom;
  cups->height = cups->header.HWResolution[1] * cups->height / 72;
  cups->top = cups->header.HWResolution[1] * cups->top / 72;
  cups->bottom = cups->header.HWResolution[1] * cups->bottom / 72;
  cups->top_trim = cups->header.HWResolution[1] * cups->top_trim / 72;
  cups->bottom_trim = cups->header.HWResolution[1] * cups->bottom_trim / 72;
  cups->adjusted_height = cups->height;
  if (cups->adjusted_height > cups->header.cupsHeight)
    cups->adjusted_height = cups->header.cupsHeight;
  if (! suppress_messages)
    fprintf(stderr, "DEBUG: Gutenprint:   CUPS settings w %d (%d) l %d r %d  h %d (%d) t %d b %d\n",
	    cups->width, cups->adjusted_width, cups->left, cups->right,
	    cups->height, cups->adjusted_height, cups->top, cups->bottom);

  if (! suppress_messages)
    fprintf(stderr, "DEBUG: Gutenprint: End initialize page\n");
  return v;
}

static void
purge_excess_data(cups_image_t *cups)
{
  char *buffer = stp_malloc(cups->header.cupsBytesPerLine);
  if (buffer)
    {
      if (! suppress_messages)
	fprintf(stderr, "DEBUG: Gutenprint: Purging %d rows\n",
		cups->header.cupsHeight - cups->row);
      while (cups->row < cups->header.cupsHeight)
	{
	  cupsRasterReadPixels(cups->ras, (unsigned char *)buffer,
			       cups->header.cupsBytesPerLine);
	  cups->row ++;
	}
    }
  stp_free(buffer);
}

static void
set_all_options(stp_vars_t *v, cups_option_t *options, int num_options,
		ppd_file_t *ppd)
{
  stp_parameter_list_t params = stp_get_parameter_list(v);
  int nparams = stp_parameter_list_count(params);
  int i;
  const char *val;		/* CUPS option value */
  ppd_option_t *ppd_option;
  if (! suppress_messages)
    fprintf(stderr, "DEBUG: Gutenprint: Set options:\n");
  val = cupsGetOption("StpiShrinkOutput", num_options, options);
  if (!val)
    {
      ppd_option = ppdFindOption(ppd, "StpiShrinkOutput");
      if (ppd_option)
	val = ppd_option->defchoice;
    }
  if (val)
    {
      if (!strcasecmp(val, "crop"))
	stp_set_int_parameter(v, "CUPSShrinkPage", 0);
      else if (!strcasecmp(val, "expand"))
	stp_set_int_parameter(v, "CUPSShrinkPage", 2);
      else
	stp_set_int_parameter(v, "CUPSShrinkPage", 1);
    }
  else
    stp_set_int_parameter(v, "CUPSShrinkPage", 1);
  for (i = 0; i < nparams; i++)
    {
      const stp_parameter_t *param = stp_parameter_list_param(params, i);
      stp_parameter_t desc;
      char *ppd_option_name = stp_malloc(strlen(param->name) + 8);	/* StpFineFOO\0 */

      stp_describe_parameter(v, param->name, &desc);
      if (desc.p_type == STP_PARAMETER_TYPE_DOUBLE)
	{
	  sprintf(ppd_option_name, "Stp%s", desc.name);
	  val = cupsGetOption(ppd_option_name, num_options, options);
	  if (!val)
	    {
	      ppd_option = ppdFindOption(ppd, ppd_option_name);
	      if (ppd_option)
		val = ppd_option->defchoice;
	    }
	  if (val && !strncasecmp(val, "Custom.", 7))
	    {
	      double dval = atof(val + 7);

	      if (! suppress_messages)
		fprintf(stderr, "DEBUG: Gutenprint:   Set float %s to %f\n",
			desc.name, dval);
	      if (dval > desc.bounds.dbl.upper)
		dval = desc.bounds.dbl.upper;
	      stp_set_float_parameter(v, desc.name, dval);
            }
	  else if (val && strlen(val) > 0 && strcmp(val, "None") != 0)
	    {
	      double fine_val = 0;
	      if (strchr(val, (int) '.'))
		{
		  fine_val = atof(val);
		  if (! suppress_messages)
		    fprintf(stderr, "DEBUG: Gutenprint:   Set float %s to %f (%s)\n",
			    desc.name, fine_val, val);
		}
	      else
		{
		  double coarse_val = atof(val) * 0.001;
		  sprintf(ppd_option_name, "StpFine%s", desc.name);
		  val = cupsGetOption(ppd_option_name, num_options, options);
		  if (!val)
		    {
		      ppd_option = ppdFindOption(ppd, ppd_option_name);
		      if (ppd_option)
			val = ppd_option->defchoice;
		    }
		  if (val && strlen(val) > 0 && strcmp(val, "None") != 0)
		    fine_val = atof(val) * 0.001;
		  if (! suppress_messages)
		    fprintf(stderr, "DEBUG: Gutenprint:   Set float %s to %f + %f\n",
			    desc.name, coarse_val, fine_val);
		  fine_val += coarse_val;
		}
	      if (fine_val > desc.bounds.dbl.upper)
		fine_val = desc.bounds.dbl.upper;
	      if (fine_val < desc.bounds.dbl.lower)
		fine_val = desc.bounds.dbl.lower;
	      stp_set_float_parameter(v, desc.name, fine_val);
	    }
	}
      else
	{
	  sprintf(ppd_option_name, "Stp%s", desc.name);
	  val = cupsGetOption(ppd_option_name, num_options, options);
	  if (!val)
	    {
	      ppd_option = ppdFindOption(ppd, ppd_option_name);
	      if (ppd_option)
		val = ppd_option->defchoice;
	    }
	  if (val && ((strlen(val) > 0 && strcmp(val, "None") != 0) ||
		      (desc.p_type == STP_PARAMETER_TYPE_STRING_LIST)))
	    {
	      switch (desc.p_type)
		{
		case STP_PARAMETER_TYPE_STRING_LIST:
		  if (! suppress_messages)
		    fprintf(stderr, "DEBUG: Gutenprint:   Set string %s to %s\n",
			    desc.name, val);
		  set_string_parameter(v, desc.name, val);
		  break;
		case STP_PARAMETER_TYPE_INT:
                  if (!strncasecmp(val, "Custom.", 7))
		    val += 7;

		  if (! suppress_messages)
		    fprintf(stderr, "DEBUG: Gutenprint:   Set int %s to %s (%d)\n",
			    desc.name, val, atoi(val));
		  stp_set_int_parameter(v, desc.name, atoi(val));
		  break;
		case STP_PARAMETER_TYPE_DIMENSION:
                  if (!strncasecmp(val, "Custom.", 7))
		    val += 7;

		  if (! suppress_messages)
		    fprintf(stderr, "DEBUG: Gutenprint:   Set dimension %s to %s (%d)\n",
			    desc.name, val, atoi(val));

		  stp_set_dimension_parameter(v, desc.name, atoi(val));
		  break;
		case STP_PARAMETER_TYPE_BOOLEAN:
		  if (! suppress_messages)
		    fprintf(stderr, "DEBUG: Gutenprint:   Set bool %s to %s (%d)\n",
			    desc.name, val, strcasecmp(val, "true") == 0 ? 1 : 0);
		  stp_set_boolean_parameter
		    (v, desc.name, strcasecmp(val, "true") == 0 ? 1 : 0);
		  break;
		case STP_PARAMETER_TYPE_CURVE: /* figure this out later... */
		case STP_PARAMETER_TYPE_FILE: /* Probably not, security hole */
		case STP_PARAMETER_TYPE_RAW: /* figure this out later, too */
		  if (! suppress_messages)
		    fprintf(stderr, "DEBUG: Gutenprint:   Ignoring option %s %s type %d\n",
			    desc.name, val, desc.p_type);
		  break;
		default:
		  break;
		}
	    }
	  else if (val)
	    {
	      if (! suppress_messages)
		fprintf(stderr, "DEBUG: Gutenprint:   Not setting %s to '%s'\n",
			desc.name, val);
	    }
	  else
	    {
	      if (! suppress_messages)
		fprintf(stderr, "DEBUG: Gutenprint:   Not setting %s to (null)\n",
			desc.name);
	    }
	}
      stp_parameter_description_destroy(&desc);
      stp_free(ppd_option_name);
    }
  if (! suppress_messages)
    fprintf(stderr, "DEBUG: Gutenprint: End options\n");
  stp_parameter_list_destroy(params);
}

/*
 * 'main()' - Main entry and processing of driver.
 */

int					/* O - Exit status */
main(int  argc,				/* I - Number of command-line arguments */
     char *argv[])			/* I - Command-line arguments */
{
  int			fd;		/* File descriptor */
  cups_image_t		cups;		/* CUPS image */
  const char		*ppdfile;	/* PPD environment variable */
  ppd_file_t		*ppd;		/* PPD file */
  ppd_size_t		*size;
  const stp_printer_t	*printer;	/* Printer driver */
  int			num_options;	/* Number of CUPS options */
  cups_option_t		*options;	/* CUPS options */
  stp_vars_t		*v = NULL;
  stp_vars_t		*default_settings;
  int			initialized_job = 0;
  const char            *version_id;
  const char            *release_version_id;
  struct tms		tms;
  clock_t		clk;
  long			clocks_per_sec;
  struct timeval	t1, t2;
  struct timezone	tz;
  char			*page_size_name = NULL;


 /*
  * Don't buffer error/status messages...
  */

  setbuf(stderr, NULL);

  if (getenv("STP_SUPPRESS_MESSAGES"))
    suppress_messages = 1;

 /*
  * Initialize libgutenprint
  */

  po = stp_i18n_load(getenv("LANG"));

  theImage.rep = &cups;

  (void) gettimeofday(&t1, &tz);
  stp_init();
  version_id = stp_get_version();
  release_version_id = stp_get_release_version();
  default_settings = stp_vars_create();

 /*
  * Check for valid arguments...
  */
  if (argc < 6 || argc > 7)
  {
   /*
    * We don't have the correct number of arguments; write an error message
    * and return.
    */

    stp_i18n_printf(po, _("Usage: rastertoprinter job-id user title copies "
                          "options [file]\n"));
    return (1);
  }

  if (! suppress_messages)
    {
      fprintf(stderr, "DEBUG: Gutenprint %s Starting\n", version_id);
      fprintf(stderr, "DEBUG: Gutenprint command line: %s '%s' '%s' '%s' '%s' %s%s%s%s\n",
	      argv[0], argv[1], argv[2], argv[3], argv[4], "<args>",
	      argc >= 7 ? " '" : "",
	      argc >= 7 ? argv[6] : "",
	      argc >= 7 ? "'" : "");
    }

 /*
  * Get the PPD file...
  */

  if ((ppdfile = getenv("PPD")) == NULL)
  {
    stp_i18n_printf(po, _("ERROR: No PPD file, unable to continue!\n"));
    return (1);
  }
  if (! suppress_messages)
    fprintf(stderr, "DEBUG: Gutenprint using PPD file %s\n", ppdfile);

  if ((ppd = ppdOpenFile(ppdfile)) == NULL)
  {
    stp_i18n_printf(po, _("ERROR: Gutenprint was unable to load PPD file "
                          "\"%s\"!\n"), ppdfile);
    return (1);
  }

  if (ppd->modelname == NULL)
  {
    stp_i18n_printf(po, _("ERROR: Gutenprint did not find a ModelName "
                          "attribute in PPD file \"%s\"!\n"), ppdfile);
    ppdClose(ppd);
    return (1);
  }

  if (ppd->nickname == NULL)
  {
    stp_i18n_printf(po, _("ERROR: Gutenprint did not find a NickName attribute "
                          "in PPD file \"%s\"!\n"), ppdfile);
    ppdClose(ppd);
    return (1);
  }
  else if (strlen(ppd->nickname) <
	   strlen(ppd->modelname) + strlen(CUPS_PPD_NICKNAME_STRING) + 3)
  {
    stp_i18n_printf(po, _("ERROR: Gutenprint found a corrupted NickName "
                          "attribute in PPD file \"%s\"!\n"), ppdfile);
    ppdClose(ppd);
    return (1);
  }
  else if (strcmp(ppd->nickname + strlen(ppd->modelname) +
		  strlen(CUPS_PPD_NICKNAME_STRING), version_id) != 0 &&
	   (strlen(ppd->nickname + strlen(ppd->modelname) +
		   strlen(CUPS_PPD_NICKNAME_STRING)) < strlen(version_id) ||
	    !((strncmp(ppd->nickname + strlen(ppd->modelname) +
		      strlen(CUPS_PPD_NICKNAME_STRING), version_id,
		      strlen(version_id)) == 0) &&
	      *(ppd->nickname + strlen(ppd->modelname) +
		strlen(CUPS_PPD_NICKNAME_STRING)) != ' ')))
  {
    stp_i18n_printf(po, _("ERROR: The PPD version (%s) is not compatible with "
                          "Gutenprint %s.\n"),
	            ppd->nickname+strlen(ppd->modelname)+strlen(CUPS_PPD_NICKNAME_STRING),
	            version_id);
    fprintf(stderr, "DEBUG: Gutenprint: If you have upgraded your version of Gutenprint\n");
    fprintf(stderr, "DEBUG: Gutenprint: recently, you must reinstall all printer queues.\n");
    fprintf(stderr, "DEBUG: Gutenprint: If the previous installed version of Gutenprint\n");
    fprintf(stderr, "DEBUG: Gutenprint: was 4.3.19 or higher, you can use the `cups-genppdupdate.%s'\n", release_version_id);
    fprintf(stderr, "DEBUG: Gutenprint: program to do this; if the previous installed version\n");
    fprintf(stderr, "DEBUG: Gutenprint: was older, you can use the Modify Printer command via\n");
    fprintf(stderr, "DEBUG: Gutenprint: the CUPS web interface: http://localhost:631/printers.\n");
    ppdClose(ppd);
    return 1;
  }

 /*
  * Get the STP options, if any...
  */

  num_options = cupsParseOptions(argv[5], 0, &options);
  ppdMarkDefaults(ppd);
  cupsMarkOptions(ppd, num_options, options);
  size = ppdPageSize(ppd, NULL);

  if (size->name)
    page_size_name = stp_strdup(size->name);

  if (! suppress_messages)
    fprintf(stderr, "DEBUG: Gutenprint: CUPS option count is %d (%d bytes)\n",
	    num_options, (int)strlen(argv[5]));

  if (num_options > 0)
    {
      int i;
      for (i = 0; i < num_options; i++)
	if (! suppress_messages)
	  fprintf(stderr, "DEBUG: Gutenprint: CUPS option %d %s = %s\n",
		  i, options[i].name, options[i].value);
    }

 /*
  * Figure out which driver to use...
  */

  printer = stp_get_printer_by_driver(ppd->modelname);
  if (!printer)
    printer = stp_get_printer_by_long_name(ppd->modelname);

  if (printer == NULL)
    {
      stp_i18n_printf(po, _("ERROR: Unable to find Gutenprint driver named "
                            "\"%s\"!\n"), ppd->modelname);
      ppdClose(ppd);
      return (1);
    }
  if (! suppress_messages)
    fprintf(stderr, "DEBUG: Gutenprint: Driver %s\n", ppd->modelname);

 /*
  * Open the page stream...
  */

  if (argc == 7)
  {
    if ((fd = open(argv[6], O_RDONLY)) == -1)
    {
      stp_i18n_printf(po, _("ERROR: Gutenprint was unable to open raster file "
                            "\"%s\" - %s"), argv[6], strerror(errno));
      sleep(1);
      return (1);
    }
  }
  else
    fd = 0;
  if (! suppress_messages)
    fprintf(stderr, "DEBUG: Gutenprint: Using fd %d\n", fd);

  stp_set_printer_defaults(default_settings, printer);
  stp_set_float_parameter(default_settings, "AppGamma", 1.0);
  set_all_options(default_settings, options, num_options, ppd);
  stp_merge_printvars(default_settings, stp_printer_get_defaults(printer));
  ppdClose(ppd);

  cups.ras = cupsRasterOpen(fd, CUPS_RASTER_READ);

 /*
  * Process pages as needed...
  */

  cups.page = 0;

  if (! suppress_messages)
    fprintf(stderr, "DEBUG: Gutenprint: About to start printing loop.\n");

  /*
   * Read the first page header, which we need in order to set up
   * the page.
   */
  signal(SIGTERM, cancel_job);
  while (cupsRasterReadHeader(cups.ras, &cups.header))
    {
      /*
       * We don't know how many pages we're going to print, and
       * we need to call stp_end_job at the completion of the job.
       * Therefore, we need to keep v in scope after the termination
       * of the loop to permit calling stp_end_job then.  Therefore,
       * we have to free the previous page's stp_vars_t at the start
       * of the loop.
       */
      if (v)
	stp_vars_destroy(v);

      /*
       * Setup printer driver variables...
       */
      if (! suppress_messages)
	{
	  fprintf(stderr, "DEBUG: Gutenprint: Printing page %d\n", cups.page + 1);
	  fprintf(stderr, "PAGE: %d 1\n", cups.page + 1);
	}
      v = initialize_page(&cups, default_settings, page_size_name);
      stp_set_int_parameter(v, "PageNumber", cups.page);
      cups.row = 0;
      if (! suppress_messages)
	print_debug_block(v, &cups);
      print_messages_as_errors = 1;
      if (!stp_verify(v))
	{
	  fprintf(stderr, "DEBUG: Gutenprint: Options failed to verify.\n");
	  fprintf(stderr, "DEBUG: Gutenprint: Make sure that you are using ESP Ghostscript rather\n");
	  fprintf(stderr, "DEBUG: Gutenprint: than GNU or AFPL Ghostscript with CUPS.\n");
	  fprintf(stderr, "DEBUG: Gutenprint: If this is not the cause, set LogLevel to debug to identify the problem.\n");
	  goto cups_abort;
	}

      if (!initialized_job)
	{
	  stp_start_job(v, &theImage);
	  initialized_job = 1;
	}

      if (!stp_print(v, &theImage))
	  goto cups_abort;
      print_messages_as_errors = 0;

      fflush(stdout);

      /*
       * Purge any remaining bitmap data...
       */
      if (cups.row < cups.header.cupsHeight)
	purge_excess_data(&cups);
      if (! suppress_messages)
	fprintf(stderr, "DEBUG: Gutenprint: Done printing page %d\n", cups.page + 1);
      cups.page ++;
    }
  if (v)
    {
      if (! suppress_messages)
	fprintf(stderr, "DEBUG: Gutenprint: Ending job\n");
      stp_end_job(v, &theImage);
      fflush(stdout);
      stp_vars_destroy(v);
    }
  cupsRasterClose(cups.ras);
  clk = times(&tms);
  (void) gettimeofday(&t2, &tz);
  clocks_per_sec = sysconf(_SC_CLK_TCK);
  fprintf(stderr, "DEBUG: Gutenprint: Printed total %.0f bytes\n",
	  total_bytes_printed);
  fprintf(stderr, "DEBUG: Gutenprint: Used %.3f seconds user, %.3f seconds system, %.3f seconds elapsed\n",
	  (double) tms.tms_utime / clocks_per_sec,
	  (double) tms.tms_stime / clocks_per_sec,
	  (double) (t2.tv_sec - t1.tv_sec) +
	  ((double) (t2.tv_usec - t1.tv_usec)) / 1000000.0);
  stp_vars_destroy(default_settings);
  if (page_size_name)
    stp_free(page_size_name);
  if (fd != 0)
    close(fd);
  return 0;

cups_abort:
  if (v)
    {
      stp_end_job(v, &theImage);
      fflush(stdout);
      stp_vars_destroy(v);
    }
  cupsRasterClose(cups.ras);
  clk = times(&tms);
  (void) gettimeofday(&t2, &tz);
  clocks_per_sec = sysconf(_SC_CLK_TCK);
  fprintf(stderr, "DEBUG: Gutenprint: Printed total %.0f bytes\n",
	  total_bytes_printed);
  fprintf(stderr, "DEBUG: Gutenprint: Used %.3f seconds user, %.3f seconds system, %.3f seconds elapsed\n",
	  (double) tms.tms_utime / clocks_per_sec,
	  (double) tms.tms_stime / clocks_per_sec,
	  (double) (t2.tv_sec - t1.tv_sec) +
	  ((double) (t2.tv_usec - t1.tv_usec)) / 1000000.0);
  stp_i18n_printf(po, _("ERROR: Invalid Gutenprint driver settings!\n"));
  stp_vars_destroy(default_settings);
  if (page_size_name)
    stp_free(page_size_name);
  if (fd != 0)
    close(fd);
  return 1;
}


/*
 * 'cups_writefunc()' - Write data to a file...
 */

static void
cups_writefunc(void *file, const char *buf, size_t bytes)
{
  FILE *prn = (FILE *)file;
  total_bytes_printed += bytes;
  fwrite(buf, 1, bytes, prn);
}

static void
cups_errfunc(void *file, const char *buf, size_t bytes)
{
  size_t next_nl = 0;
  size_t where = 0;
  FILE *prn = (FILE *)file;
  while (where < bytes)
    {
      if (bytes - where > 6 && strncmp(buf, "ERROR:", 6) == 0)
	{
	  fputs("ERROR: Gutenprint error:", prn);
	  buf += 6;
	}
      else if (print_messages_as_errors)
	fputs("ERROR: Gutenprint error: ", prn);
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


/*
 * 'cancel_job()' - Cancel the current job...
 */

static void
cancel_job(int sig)			/* I - Signal */
{
  (void)sig;
  Image_status = STP_IMAGE_STATUS_ABORT;
}

/*
 * 'Image_get_appname()' - Get the application we are running.
 */

static const char *				/* O - Application name */
Image_get_appname(stp_image_t *image)		/* I - Image */
{
  (void)image;

  return ("CUPS driver based on Gutenprint");
}


/*
 * 'Image_get_row()' - Get one row of the image.
 */

static void
throwaway_data(int amount, cups_image_t *cups)
{
  unsigned char trash[4096];	/* Throwaway */
  int block_count = amount / 4096;
  int leftover = amount % 4096;
  while (block_count > 0)
    {
      cupsRasterReadPixels(cups->ras, trash, 4096);
      block_count--;
    }
  if (leftover)
    cupsRasterReadPixels(cups->ras, trash, leftover);
}

static stp_image_status_t
Image_get_row(stp_image_t   *image,	/* I - Image */
	      unsigned char *data,	/* O - Row */
	      size_t	    byte_limit,	/* I - how many bytes in data */
	      int           row)	/* I - Row number (unused) */
{
  cups_image_t	*cups;			/* CUPS image */
  int		i;			/* Looping var */
  int 		bytes_per_line;
  int		margin;
  stp_image_status_t tmp_image_status = Image_status;
  unsigned char *orig = data;           /* Temporary pointer */
  static int warned = 0;                /* Error warning printed? */
  int new_percent;
  int left_margin, right_margin;

  if ((cups = (cups_image_t *)(image->rep)) == NULL)
    {
      stp_i18n_printf(po, _("ERROR: Gutenprint image is not initialized!  "
                            "Please report this bug to "
			    "gimp-print-devel@lists.sourceforge.net\n"));
      return STP_IMAGE_STATUS_ABORT;
    }
  bytes_per_line =
    ((cups->adjusted_width * cups->header.cupsBitsPerPixel) + CHAR_BIT - 1) /
    CHAR_BIT;

  left_margin = ((cups->left_trim * cups->header.cupsBitsPerPixel) + CHAR_BIT - 1) /
    CHAR_BIT;
  right_margin = ((cups->right_trim * cups->header.cupsBitsPerPixel) + CHAR_BIT - 1) /
    CHAR_BIT;
  margin = cups->header.cupsBytesPerLine - left_margin - bytes_per_line -
    right_margin;

  if (cups->row < cups->header.cupsHeight)
  {
    if (! suppress_messages)
      fprintf(stderr, "DEBUG2: Gutenprint: Reading %d %d\n",
	      bytes_per_line, cups->row);
    while (cups->row <= row && cups->row < cups->header.cupsHeight)
      {
	if (left_margin > 0)
	  {
	    if (! suppress_messages)
	      fprintf(stderr, "DEBUG2: Gutenprint: Tossing left %d (%d)\n",
		      left_margin, cups->left_trim);
	    throwaway_data(left_margin, cups);
	  }
	cupsRasterReadPixels(cups->ras, data, bytes_per_line);
	cups->row ++;
	if (margin + right_margin > 0)
	  {
	    if (!suppress_messages)
	      fprintf(stderr, "DEBUG2: Gutenprint: Tossing right %d (%d) + %d\n",
		      right_margin, cups->right_trim, margin);
	    throwaway_data(margin + right_margin, cups);
	  }
      }
  }
  else
    {
      switch (cups->header.cupsColorSpace)
	{
	case CUPS_CSPACE_K:
	case CUPS_CSPACE_CMYK:
	case CUPS_CSPACE_KCMY:
	case CUPS_CSPACE_CMY:
	  memset(data, 0, bytes_per_line);
	  break;
	case CUPS_CSPACE_RGB:
	case CUPS_CSPACE_W:
	  memset(data, ((1 << CHAR_BIT) - 1), bytes_per_line);
	  break;
	default:
	  stp_i18n_printf(po, _("ERROR: Gutenprint detected a bad colorspace "
	                        "(%d)!\n"), cups->header.cupsColorSpace);
	  return STP_IMAGE_STATUS_ABORT;
	}
    }

  /*
   * This exists to print non-ADSC input which has messed up the job
   * input, such as that generated by psnup.  The output is barely
   * legible, but it's better than the garbage output otherwise.
   */
  data = orig;
  if (cups->header.cupsBitsPerPixel == 1)
    {
      if (warned == 0)
	{
	  fputs(_("WARNING: Gutenprint detected a bad color depth (1).  "
		  "Output quality is degraded.  Are you using psnup or "
		  "non-ADSC PostScript?\n"), stderr);
	  warned = 1;
	}
      for (i = cups->adjusted_width - 1; i >= 0; i--)
	{
	  if ( (data[i/8] >> (7 - i%8)) &0x1)
	    data[i]=255;
	  else
	    data[i]=0;
	}
    }

  new_percent = (int) (100.0 * cups->row / cups->header.cupsHeight);
  if (new_percent > cups->last_percent)
    {
      if (! suppress_messages)
	{
	  stp_i18n_printf(po, _("INFO: Printing page %d, %d%%\n"),
			  cups->page + 1, new_percent);
	  fprintf(stderr, "ATTR: job-media-progress=%d\n", new_percent);
	}
      cups->last_percent = new_percent;
    }

  if (tmp_image_status != STP_IMAGE_STATUS_OK)
    {
      if (! suppress_messages)
	fprintf(stderr, "DEBUG: Gutenprint: Image status %d\n", tmp_image_status);
    }
  return tmp_image_status;
}


/*
 * 'Image_height()' - Return the height of an image.
 */

static int				/* O - Height in pixels */
Image_height(stp_image_t *image)	/* I - Image */
{
  cups_image_t	*cups;		/* CUPS image */


  if ((cups = (cups_image_t *)(image->rep)) == NULL)
    return (0);

  if (! suppress_messages)
    fprintf(stderr, "DEBUG: Gutenprint: Image_height %d\n", cups->adjusted_height);
  return (cups->adjusted_height);
}


/*
 * 'Image_init()' - Initialize an image.
 */

static void
Image_init(stp_image_t *image)		/* I - Image */
{
  cups_image_t	*cups;		/* CUPS image */

  if ((cups = (cups_image_t *)(image->rep)) == NULL)
    return;
  cups->last_percent = 0;

  if (! suppress_messages)
    stp_i18n_printf(po, _("INFO: Starting page %d...\n"), cups->page + 1);
  /* cups->page + 1 because users expect 1-based counting */
}

/*
 * 'Image_progress_conclude()' - Close the progress display.
 */

static void
Image_conclude(stp_image_t *image)	/* I - Image */
{
  cups_image_t	*cups;		/* CUPS image */


  if ((cups = (cups_image_t *)(image->rep)) == NULL)
    return;

  if (! suppress_messages)
    stp_i18n_printf(po, _("INFO: Finished page %d...\n"), cups->page + 1);
}

/*
 * 'Image_width()' - Return the width of an image.
 */

static int				/* O - Width in pixels */
Image_width(stp_image_t *image)	/* I - Image */
{
  cups_image_t	*cups;		/* CUPS image */


  if ((cups = (cups_image_t *)(image->rep)) == NULL)
    return (0);

  if (! suppress_messages)
    fprintf(stderr, "DEBUG: Gutenprint: Image_width %d\n", cups->adjusted_width);
  return (cups->adjusted_width);
}


/*
 * End of "$Id: rastertoprinter.c,v 1.137 2011/04/09 00:19:59 rlk Exp $".
 */
