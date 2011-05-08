/*
 * "$Id: print-raw.c,v 1.42 2010/12/05 21:38:15 rlk Exp $"
 *
 *   Print plug-in RAW driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
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

#ifdef __GNUC__
#define inline __inline__
#endif

typedef struct
{
  const char *output_type;
  int output_channels;
  int rotate_channels;
  const char *name;
} ink_t;

typedef struct raw_printer
{
  int output_bits;
} raw_printer_t;

static const raw_printer_t raw_model_capabilities[] =
{
  {
    16
  },
  {
    8
  },
};

static const ink_t inks[] =
{
  { "RGB", 3, 0, "RGB" },
  { "CMY", 3, 0, "CMY" },
  { "KCMY", 4, 1, "CMYK" },
  { "KCMY", 4, 0, "KCMY" },
  { "Whitescale", 1, 0, "RGBGray" },
  { "Grayscale", 1, 0, "CMYGray" },
};

static const int ink_count = sizeof(inks) / sizeof(ink_t);

static const stp_parameter_t the_parameters[] =
{
  {
    "InkType", N_("Ink Type"), "Color=Yes,Category=Advanced Printer Setup",
    N_("Type of ink in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
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

static stp_parameter_list_t
raw_list_parameters(const stp_vars_t *v)
{
  stp_parameter_list_t *ret = stp_parameter_list_create();
  int i;
  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(the_parameters[i]));
  return ret;
}

static void
raw_parameters(const stp_vars_t *v, const char *name,
	       stp_parameter_t *description)
{
  int		i;
  description->p_type = STP_PARAMETER_TYPE_INVALID;
  if (name == NULL)
    return;

  description->deflt.str = NULL;
  for (i = 0; i < the_parameter_count; i++)
    if (strcmp(name, the_parameters[i].name) == 0)
      {
	stp_fill_parameter_settings(description, &(the_parameters[i]));
	break;
      }
  if (strcmp(name, "InkType") == 0)
    {
      description->bounds.str = stp_string_list_create();
      for (i = 0; i < ink_count; i++)
	stp_string_list_add_string(description->bounds.str,
				  inks[i].name, inks[i].name);
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "PrintingMode") == 0)
    {
      description->bounds.str = stp_string_list_create();
      stp_string_list_add_string
	(description->bounds.str, "Color", _("Color"));
      stp_string_list_add_string
	(description->bounds.str, "BW", _("Black and White"));
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else
    description->is_active = 0;
}

/*
 * 'escp2_imageable_area()' - Return the imageable area of the page.
 */

static void
raw_imageable_area(const stp_vars_t *v,
		   int  *left,
		   int  *right,
		   int  *bottom,
		   int  *top)
{
  *left = 0;
  *top = 0;
  *right = stp_get_page_width(v);
  *bottom = stp_get_page_height(v);
}

static void
raw_limit(const stp_vars_t *v,			/* I */
	    int *width, int *height,
	    int *min_width, int *min_height)
{
  *width = 65535;
  *height = 65535;
  *min_width = 1;
  *min_height =	1;
}

static void
raw_describe_resolution(const stp_vars_t *v, int *x, int *y)
{
  *x = 72;
  *y = 72;
}

static const char *
raw_describe_output(const stp_vars_t *v)
{
  const char *ink_type = stp_get_string_parameter(v, "InkType");
  if (ink_type)
    {
      int i;
      for (i = 0; i < ink_count; i++)
	if (strcmp(ink_type, inks[i].name) == 0)
	    return inks[i].output_type;
    }
  return "RGB";
}

/*
 * 'escp2_print()' - Print an image to an EPSON printer.
 */
static int
raw_print(const stp_vars_t *v, stp_image_t *image)
{
  int		model = stp_get_model_id(v);
  int width = stp_get_page_width(v);
  int height = stp_get_page_height(v);
  int		i, j;
  int		y;		/* Looping vars */
  stp_vars_t	*nv = stp_vars_create_copy(v);
  int out_channels;
  unsigned short *final_out = NULL;
  int		status = 1;
  int bytes_per_channel = raw_model_capabilities[model].output_bits / 8;
  int ink_channels = 1;
  int rotate_output = 0;
  const char *ink_type = stp_get_string_parameter(nv, "InkType");
  stp_image_init(image);

  stp_prune_inactive_options(nv);
  if (!stp_verify(nv))
    {
      stp_eprintf(nv, _("Print options not verified; cannot print.\n"));
      stp_vars_destroy(nv);
      stp_image_conclude(image);
      return 0;
    }
  if (width != stp_image_width(image) || height != stp_image_height(image))
    {
      stp_eprintf(nv, _("Image dimensions must match paper dimensions"));
      stp_vars_destroy(nv);
      stp_image_conclude(image);
      return 0;
    }
  if (ink_type)
    {
      for (i = 0; i < ink_count; i++)
	if (strcmp(ink_type, inks[i].name) == 0)
	  {
	    stp_set_string_parameter(nv, "STPIOutputType", inks[i].output_type);
	    ink_channels = inks[i].output_channels;
	    rotate_output = inks[i].rotate_channels;
	    break;
	  }
    }

  stp_set_float_parameter(nv, "Density", 1.0);
  stp_set_boolean_parameter(nv, "SimpleGamma", 1);
  stp_channel_reset(nv);
  for (i = 0; i < ink_channels; i++)
    stp_channel_add(nv, i, 0, 1.0);

  if (bytes_per_channel == 1)
    out_channels = stp_color_init(nv, image, 256);
  else
    out_channels = stp_color_init(nv, image, 65536);

  if (out_channels != ink_channels && out_channels != 1 && ink_channels != 1)
    {
      stp_eprintf(nv, "Internal error!  Output channels or input channels must be 1\n");
      stp_vars_destroy(nv);
      stp_image_conclude(image);
      return 0;
    }

  if (out_channels != ink_channels)
    final_out = stp_malloc(width * ink_channels * 2);

  for (y = 0; y < height; y++)
    {
      unsigned short *out;
      unsigned short *real_out;
      unsigned zero_mask;
      if (stp_color_get_row(nv, image, y, &zero_mask))
	{
	  status = 2;
	  break;
	}
      out = stp_channel_get_input(nv);
      real_out = out;
      if (rotate_output)
	{
	  unsigned short *tmp_out = real_out;
	  for (i = 0; i < width; i++)
	    {
	      unsigned short tmp = tmp_out[0];
	      for (j = 0; j < ink_channels - 1; j++)
		tmp_out[j] = tmp_out[j + 1];
	      tmp_out[ink_channels - 1] = tmp;
	      tmp_out += ink_channels;
	    }
	}
      if (out_channels != ink_channels)
	{
	  real_out = final_out;
	  if (out_channels < ink_channels)
	    {
	      for (i = 0; i < width; i++)
		{
		  for (j = 0; j < ink_channels; j++)
		    final_out[i * ink_channels + j] = out[i];
		}
	    }
	  else
	    {
	      for (i = 0; i < width; i++)
		{
		  int avg = 0;
		  for (j = 0; j < out_channels; j++)
		    avg += out[i * out_channels + j];
		  final_out[i] = avg / out_channels;
		}
	    }
	}
      if (bytes_per_channel == 1)
	{
	  unsigned char *char_out = (unsigned char *) real_out;
	  for (i = 0; i < width * ink_channels; i++)
	    char_out[i] = real_out[i] / 257;
	}
      stp_zfwrite((char *) real_out,
		  width * ink_channels * bytes_per_channel, 1, nv);
    }
  stp_image_conclude(image);
  if (final_out)
    stp_free(final_out);
  stp_vars_destroy(nv);
  return status;
}

static const stp_printfuncs_t print_raw_printfuncs =
{
  raw_list_parameters,
  raw_parameters,
  stp_default_media_size,
  raw_imageable_area,
  raw_imageable_area,
  raw_limit,
  raw_print,
  raw_describe_resolution,
  raw_describe_output,
  stp_verify_printer_params,
  NULL,
  NULL,
  NULL
};




static stp_family_t print_raw_module_data =
  {
    &print_raw_printfuncs,
    NULL
  };


static int
print_raw_module_init(void)
{
  return stp_family_register(print_raw_module_data.printer_list);
}


static int
print_raw_module_exit(void)
{
  return stp_family_unregister(print_raw_module_data.printer_list);
}


/* Module header */
#define stp_module_version print_raw_LTX_stp_module_version
#define stp_module_data print_raw_LTX_stp_module_data

stp_module_version_t stp_module_version = {0, 0};

stp_module_t stp_module_data =
  {
    "raw",
    VERSION,
    "RAW family driver",
    STP_MODULE_CLASS_FAMILY,
    NULL,
    print_raw_module_init,
    print_raw_module_exit,
    (void *) &print_raw_module_data
  };
