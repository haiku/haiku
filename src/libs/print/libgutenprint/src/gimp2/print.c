/*
 * "$Id: print.c,v 1.8 2006/07/04 02:57:59 rlk Exp $"
 *
 *   Print plug-in for the GIMP.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gutenprintui2/gutenprintui.h>
#include "print_gimp.h"

#include <sys/types.h>
#include <signal.h>
#include <ctype.h>
#include <sys/wait.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "print-intl.h"

/*
 * Local functions...
 */

static void    query           (void);
static void    run             (const char        *name,
                                gint               nparams,
                                const GimpParam   *param,
                                gint              *nreturn_vals,
                                GimpParam        **return_vals);
static int     do_print_dialog (const gchar *proc_name,
                                gint32       image_ID);

/*
 * Globals...
 */

GimpPlugInInfo	PLUG_IN_INFO =		/* Plug-in information */
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static stpui_plist_t gimp_vars;

/*
 * 'main()' - Main entry - just call gimp_main()...
 */

MAIN()

/*
 * 'query()' - Respond to a plug-in query...
 */

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "run_mode",	(BAD_CONST_CHAR) "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,	(BAD_CONST_CHAR) "image",	(BAD_CONST_CHAR) "Input image" },
    { GIMP_PDB_DRAWABLE,	(BAD_CONST_CHAR) "drawable",	(BAD_CONST_CHAR) "Input drawable" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "output_to",	(BAD_CONST_CHAR) "Print command or filename (| to pipe to command)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "driver",	(BAD_CONST_CHAR) "Printer driver short name" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "ppd_file",	(BAD_CONST_CHAR) "PPD file" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "output_type",	(BAD_CONST_CHAR) "Output type (0 = gray, 1 = color)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "resolution",	(BAD_CONST_CHAR) "Resolution (\"300\", \"720\", etc.)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "media_size",	(BAD_CONST_CHAR) "Media size (\"Letter\", \"A4\", etc.)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "media_type",	(BAD_CONST_CHAR) "Media type (\"Plain\", \"Glossy\", etc.)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "media_source",	(BAD_CONST_CHAR) "Media source (\"Tray1\", \"Manual\", etc.)" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "brightness",	(BAD_CONST_CHAR) "Brightness (0-400%)" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "scaling",	(BAD_CONST_CHAR) "Output scaling (0-100%, -PPI)" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "orientation",	(BAD_CONST_CHAR) "Output orientation (-1 = auto, 0 = portrait, 1 = landscape)" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "left",	(BAD_CONST_CHAR) "Left offset (points, -1 = centered)" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "top",		(BAD_CONST_CHAR) "Top offset (points, -1 = centered)" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "gamma",	(BAD_CONST_CHAR) "Output gamma (0.1 - 3.0)" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "contrast",	(BAD_CONST_CHAR) "Contrast" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "cyan",	(BAD_CONST_CHAR) "Cyan level" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "magenta",	(BAD_CONST_CHAR) "Magenta level" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "yellow",	(BAD_CONST_CHAR) "Yellow level" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "linear",	(BAD_CONST_CHAR) "Linear output (0 = normal, 1 = linear)" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "image_type",	(BAD_CONST_CHAR) "Image type (0 = line art, 1 = solid tones, 2 = continuous tone, 3 = monochrome)"},
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "saturation",	(BAD_CONST_CHAR) "Saturation (0-1000%)" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "density",	(BAD_CONST_CHAR) "Density (0-200%)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "ink_type",	(BAD_CONST_CHAR) "Type of ink or cartridge" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "dither_algorithm", (BAD_CONST_CHAR) "Dither algorithm" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "unit",	(BAD_CONST_CHAR) "Unit 0=Inches 1=Metric" },
  };

  static const gchar *blurb = "This plug-in prints images from The GIMP using Gutenprint directly.";
  static const gchar *help  = "Prints images to many printers.";
  static const gchar *auth  = "Michael Sweet <mike@easysw.com> and Robert Krawitz <rlk@alum.mit.edu>";
  static const gchar *copy  = "Copyright 1997-2006 by Michael Sweet and Robert Krawitz";
  static const gchar *types = "RGB*,GRAY*,INDEXED*";

  gimp_plugin_domain_register ((BAD_CONST_CHAR) PACKAGE, (BAD_CONST_CHAR) PACKAGE_LOCALE_DIR);

  do_gimp_install_procedure(blurb, help, auth, copy, types, G_N_ELEMENTS(args),
			    args);
}

static guchar *gimp_thumbnail_data = NULL;

static guchar *
stpui_get_thumbnail_data_function(void *image_ID, gint *width, gint *height,
				  gint *bpp, gint page)
{
  if (gimp_thumbnail_data)
    g_free(gimp_thumbnail_data);
  gimp_thumbnail_data =
    gimp_image_get_thumbnail_data((gint) image_ID, width, height, bpp);
  return gimp_thumbnail_data;
}

/*
 * 'run()' - Run the plug-in...
 */

volatile int SDEBUG = 1;

static void
run (const char        *name,		/* I - Name of print program. */
     gint               nparams,	/* I - Number of parameters passed in */
     const GimpParam   *param,		/* I - Parameter values */
     gint              *nreturn_vals,	/* O - Number of return values */
     GimpParam        **return_vals)	/* O - Return values */
{
  GimpDrawable	*drawable;	/* Drawable for image */
  GimpRunMode	 run_mode;	/* Current run mode */
  GimpParam	*values;	/* Return values */
  gint32         drawable_ID;   /* drawable ID */
  GimpExportReturn export = GIMP_EXPORT_CANCEL;    /* return value of gimp_export_image() */
  gdouble xres, yres;
  char *image_filename;
  stpui_image_t *image;
  gint32 image_ID;
  gint32 base_type;
  if (getenv("STP_DEBUG_STARTUP"))
    while (SDEBUG)
      ;

 /*
  * Initialise libgutenprint
  */

  stp_init();

  stp_set_output_codeset("UTF-8");

#ifdef INIT_I18N_UI
  INIT_I18N_UI();
#else
  /*
   * With GCC and glib 1.2, there will be a warning here about braces in
   * expressions.  Getting rid of it causes more problems than it solves.
   * In particular, turning on -ansi on the command line causes a number of
   * other useful things, such as strcasecmp, popen, and snprintf to go away
   */
  INIT_LOCALE (PACKAGE);
#endif

  stpui_printer_initialize(&gimp_vars);
  /*
   * Initialize parameter data...
   */

  run_mode = (GimpRunMode)param[0].data.d_int32;

  values = g_new (GimpParam, 1);

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals  = values;

  image_ID = param[1].data.d_int32;
  drawable_ID = param[2].data.d_int32;

  image_filename = gimp_image_get_filename (image_ID);
  if (image_filename)
    {
      if (strchr(image_filename, '/'))
	image_filename = strrchr(image_filename, '/') + 1;
      stpui_set_image_filename(image_filename);
      /* g_free(image_filename); */
    }
  else
    stpui_set_image_filename("Untitled");

  /*  eventually export the image */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init ("print", TRUE);
      export = gimp_export_image (&image_ID, &drawable_ID, "Print",
				  (GIMP_EXPORT_CAN_HANDLE_RGB |
				   GIMP_EXPORT_CAN_HANDLE_GRAY |
				   GIMP_EXPORT_CAN_HANDLE_INDEXED |
				   GIMP_EXPORT_CAN_HANDLE_ALPHA));
      if (export == GIMP_EXPORT_CANCEL)
	{
	  *nreturn_vals = 1;
	  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
	  return;
	}
      break;
    default:
      break;
    }

  /*
   * Get drawable...
   */

  drawable = gimp_drawable_get (drawable_ID);
  stpui_set_image_dimensions(drawable->width, drawable->height);
  gimp_image_get_resolution (image_ID, &xres, &yres);
  stpui_set_image_resolution(xres, yres);
  stpui_set_image_channel_depth(8);
  base_type = gimp_image_base_type(image_ID);
  switch (base_type)
    {
    case GIMP_INDEXED:
    case GIMP_RGB:
      stpui_set_image_type("RGB");
      break;
    case GIMP_GRAY:
      stpui_set_image_type("Whitescale");
      break;
    default:
      break;
    }

  image = Image_GimpDrawable_new(drawable, image_ID);
  stp_set_float_parameter(gimp_vars.v, "AppGamma", gimp_gamma());

  /*
   * See how we will run
   */

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*
       * Get information from the dialog...
       */

      if (!do_print_dialog (name, image_ID))
	goto cleanup;
      stpui_plist_copy(&gimp_vars, stpui_get_current_printer());
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*
       * Make sure all the arguments are present...
       */
      if (nparams < 11)
	values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      else
	{
#if 0
	  /* What do we do with old output_to?  Probably best ignore it. */
	  stpui_plist_set_output_to(&gimp_vars, param[3].data.d_string);
#endif
	  stp_set_driver(gimp_vars.v, param[4].data.d_string);
	  stp_set_file_parameter(gimp_vars.v, "PPDFile", param[5].data.d_string);
	  switch (param[6].data.d_int32)
	    {
	    case 0:
	    default:
	      stp_set_string_parameter(gimp_vars.v, "PrintingMode", "BW");
	    case 1:
	      stp_set_string_parameter(gimp_vars.v, "PrintingMode", "Color");
	    }
	  stp_set_string_parameter(gimp_vars.v, "Resolution", param[7].data.d_string);
	  stp_set_string_parameter(gimp_vars.v, "PageSize", param[8].data.d_string);
	  stp_set_string_parameter(gimp_vars.v, "MediaType", param[9].data.d_string);
	  stp_set_string_parameter(gimp_vars.v, "InputSlot", param[10].data.d_string);

          if (nparams > 11)
	    stp_set_float_parameter(gimp_vars.v, "Brightness", param[11].data.d_float);

          if (nparams > 12)
	    gimp_vars.scaling = param[12].data.d_float;

          if (nparams > 13)
	    gimp_vars.orientation = param[13].data.d_int32;

          if (nparams > 14)
            stp_set_left(gimp_vars.v, param[14].data.d_int32);

          if (nparams > 15)
            stp_set_top(gimp_vars.v, param[15].data.d_int32);

          if (nparams > 16)
            stp_set_float_parameter(gimp_vars.v, "Gamma", param[16].data.d_float);

          if (nparams > 17)
	    stp_set_float_parameter(gimp_vars.v, "Contrast", param[17].data.d_float);

          if (nparams > 18)
	    stp_set_float_parameter(gimp_vars.v, "Cyan", param[18].data.d_float);

          if (nparams > 19)
	    stp_set_float_parameter(gimp_vars.v, "Magenta", param[19].data.d_float);

          if (nparams > 20)
	    stp_set_float_parameter(gimp_vars.v, "Yellow", param[20].data.d_float);

          if (nparams > 21)
	    stp_set_string_parameter(gimp_vars.v, "ImageOptimization", param[21].data.d_string);

          if (nparams > 22)
            stp_set_float_parameter(gimp_vars.v, "Saturation", param[23].data.d_float);

          if (nparams > 23)
            stp_set_float_parameter(gimp_vars.v, "Density", param[24].data.d_float);

	  if (nparams > 24)
	    stp_set_string_parameter(gimp_vars.v, "InkType", param[25].data.d_string);

	  if (nparams > 25)
	    stp_set_string_parameter(gimp_vars.v, "DitherAlgorithm",
			      param[26].data.d_string);

          if (nparams > 26)
	    gimp_vars.unit = param[27].data.d_int32;
	}

      break;

    case GIMP_RUN_WITH_LAST_VALS:
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      break;

    default:
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      break;
    }

  if (gimp_thumbnail_data)
    g_free(gimp_thumbnail_data);

  /*
   * Print the image...
   */
  if (values[0].data.d_status == GIMP_PDB_SUCCESS)
    {
      /*
       * Set the tile cache size...
       */

      if (drawable->height > drawable->width)
	gimp_tile_cache_ntiles ((drawable->height + gimp_tile_width () - 1) /
				gimp_tile_width () + 1);
      else
	gimp_tile_cache_ntiles ((drawable->width + gimp_tile_width () - 1) /
				gimp_tile_width () + 1);

      if (! stpui_print(&gimp_vars, image))
	  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

      /*
       * Store data...
       * FIXME! This is broken!
       */

#if 0
      if (run_mode == GIMP_RUN_INTERACTIVE)
	gimp_set_data (PLUG_IN_NAME, vars, sizeof (vars));
#endif
    }

  /*
   * Detach from the drawable...
   */
  gimp_drawable_detach (drawable);

 cleanup:
  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image_ID);
  stp_vars_destroy(gimp_vars.v);
}

/*
 * 'do_print_dialog()' - Pop up the print dialog...
 */

static void
gimp_writefunc(void *file, const char *buf, size_t bytes)
{
  FILE *prn = (FILE *)file;
  fwrite(buf, 1, bytes, prn);
}

static void
gimp_errfunc(void *file, const char *buf, size_t bytes)
{
  char formatbuf[32];
  snprintf(formatbuf, 31, "%%%ds", bytes);
  g_message(formatbuf, buf);
}

static gint
do_print_dialog (const gchar *proc_name,
		 gint32       image_ID)
{
 /*
  * Generate the filename for the current user...
  */
  char *filename = gimp_personal_rc_file ((BAD_CONST_CHAR) "printrc");
  stpui_set_printrc_file(filename);
  g_free(filename);
  if (! getenv("STP_PRINT_MESSAGES_TO_STDERR"))
    stpui_set_errfunc(gimp_errfunc);
  stpui_set_thumbnail_func(stpui_get_thumbnail_data_function);
  stpui_set_thumbnail_data((void *) image_ID);
  return stpui_do_print_dialog();
}
