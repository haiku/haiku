/***********************************************************************
 *                                                                     *
 * $Id: hpgs.c 390 2007-03-19 15:56:42Z softadm $
 *                                                                     *
 * hpgs - HPGl Script, a hpgl/2 interpreter, which uses a Postscript   *
 *        API for rendering a scene and thus renders to a variety of   *
 *        devices and fileformats.                                     *
 *                                                                     *
 * (C) 2004-2006 EV-i Informationstechnologie GmbH  http://www.ev-i.at *
 *                                                                     *
 * Author: Wolfgang Glas                                               *
 *                                                                     *
 *  hpgs is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU Lesser General Public          *
 * License as published by the Free Software Foundation; either        *
 * version 2.1 of the License, or (at your option) any later version.  *
 *                                                                     *
 * hpgs is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   *
 * Lesser General Public License for more details.                     *
 *                                                                     *
 * You should have received a copy of the GNU Lesser General Public    *
 * License along with this library; if not, write to the               *
 * Free Software  Foundation, Inc., 59 Temple Place, Suite 330,        *
 * Boston, MA  02111-1307  USA                                         *
 *                                                                     *
 ***********************************************************************
 *                                                                     *
 * The main program.                                                   *
 *                                                                     *
 ***********************************************************************/

#include <hpgs.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <locale.h>

#if defined ( __MINGW32__ ) || defined ( _MSC_VER )
#include<malloc.h>
#else
#include<alloca.h>
#endif

/*! \mainpage The HPGS documentation
 
    \section intro_sec Introduction
 

    HPGS has been developed by
    <A HREF="http://www.ev-i.at">EV-i Informationstechnologie GmbH</A>
    and has been published under the
    <A HREF="http://www.fsf.org/licenses/lgpl.html">LGPL</A>.

    For Details visit the <A HREF="http://hpgs.berlios.de">project's homepage</A>.

    \section install_sec Installation
 
     See the file INSTALL of the distribution.

    \section invoke_sec Invoking hpgs
   
    \verbinclude hpgs-args.txt

    \section api_sec The hpgs API.

    HPGS is designed to be highly modular and may be used inside
    C or C++ programs for either displaying HPGL files or
    drawing a vector graphics sceneery to an image.

    Refer to the \ref reader, \ref device, \ref paint_device and
    \ref image modules in order to get an impression what the hpgs API
    can do for you.
 */


#ifdef WIN32

#include <windows.h>

static char *get_aux_path(void)
{
  const char *key = "SOFTWARE\\ev-i\\hpgs";
  DWORD len;
  HKEY hkey;
  char *ret;
  const char *prg;

  if ((RegOpenKeyEx(HKEY_CURRENT_USER,
		    key, 0, KEY_READ, &hkey) == ERROR_SUCCESS ||
       RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		    key, 0, KEY_READ, &hkey) == ERROR_SUCCESS    )&&
      RegQueryValueEx(hkey,"prefix",NULL,NULL,NULL,&len) == ERROR_SUCCESS)
    {
      ret = malloc(len+2);

      if (RegQueryValueEx(hkey,"prefix",NULL,NULL,ret,&len) != ERROR_SUCCESS)
	{ free (ret); ret = 0; }

      RegCloseKey(hkey);

      return ret;
    }

  prg = getenv("PROGRAMFILES");

  if (prg)
    return hpgs_sprintf_malloc("%s\\EV-i",prg);
  
  return strdup("C:\\Programme\\EV-i");
}
#else

#ifndef HPGS_DEFAULT_PREFIX
#define HPGS_DEFAULT_PREFIX "/usr/local"
#endif

static char *get_aux_path(void)
{
  char *p=getenv("EV_I_PREFIX");

  if (!p)
    return strdup(HPGS_DEFAULT_PREFIX);

  return strdup(p);
}
#endif

static int usage (void)
{
  fprintf (stderr,hpgs_i18n(
	   "\n"
	   " hpgs v%s - HPGl Script, a hpgl/2 interpreter/renderer.\n"
	   "\n"
	   " (C) 2004-2006 ev-i Informationstechnologie GmbH (http://www.ev-i.at)\n"
	   "     published under the LGPL (http://www.fsf.org/licenses/lgpl.html).\n"
	   "\n"
	   "Type `hpgs --help' for a detailed description of cmd line options.\n")
	   ,HPGS_VERSION  );

  hpgs_cleanup();

  return 0;
}

static int help (void)
{
  fprintf (stderr,hpgs_i18n(
	   "\n"
	   " hpgs v%s - HPGl Script, a hpgl/2 interpreter/renderer.\n"
	   "\n"
	   " (C) 2004-2006 ev-i Informationstechnologie GmbH (http://www.ev-i.at)\n"
	   "     published under the LGPL (http://www.fsf.org/licenses/lgpl.html).\n"
	   "\n"
	   "usage: hpgs [-i] [-v] [-q] [-d <dev>] [options...] [-o <out>] <in1> <in2>...\n"
	   "       -i ... Ignore plotsize from PS and determine\n"
	   "              the plotsize from the file content.\n"
	   "       --linewidth-size ... Account for linewidths in the plotsize\n"
	   "              calculation for -i (default behaviour).\n"
	   "       --no-linewidth-size ... Ignore linewidths in the plotsize\n"
	   "              calculation for -i.\n"
	   "       -v ... Increase verbosity.\n"
	   "       -q ... Decrease verbosity, be quiet.\n"
	   "       -o <out> ... specifiy the output filename. If not specified, write\n"
	   "                    to stdout.\n"
	   "       -m ... Switch to multipage mode, if a single file is given on the.\n"
	   "              command line. If not specified, only the first page of the input\n"
           "              file is processed.\n"
	   "       -w ... Correct linewidth by given factor (default: 1).\n"
	   "       -d ... Specifiy the output device. The default is the\n"
	   "              native 'eps' device, which writes a simple eps file.\n"
	   "              See section `Supported devices' below.\n"
	   "       -a ... Use antialiasing when rendering to a png image.\n"
	   "       --thin-alpha=<a>\n"
           "              Specifiy the minimal alpha value for thin lines.\n"
	   "              allowed values lie in the interval [0.001,1.0] (default: 0.25).\n"
	   "       -I ... Specifiy image interpolation.\n"
	   "       --rop3 ... Use ROP3 raster operations for rendering on devices\n"
	   "              with ROP3 capabilities (default).\n"
	   "       --no-rop3 ... Suppress usage of ROP3 raster operations.\n"
	   "       -c ... Specifiy the compression level for png writing.\n"
	   "       -r ... Specifiy the device resolution in dpi.\n"
	   "              If no y-resolution is given, the y-resolution\n"
	   "              is set to the same value as the x-resolution.\n"
	   "       -p ... Specifiy the maximum pixel size of the device.\n"
	   "              This is in fact an alternate method to specify the\n"
	   "              resolution of the device. If -p is given, the device\n"
	   "              resolution is caclulated in a way, that the plot fits\n"
	   "              into the given rectangle.\n"
	   "              If no y-size is given, the size y-size\n"
	   "              is set to the same value as the x-size.\n"
	   "       -s<papersize> ... Specifiy the plot size in pt (1/72 inch).\n"
	   "              If -s is not given, the size is calculated from\n"
	   "              the contents of the hpgl file.\n"
           "              The <papersize> may be either one of the standard\n"
           "              paper sizes A0,A1,A2,A3 or A4, their landscape versions\n"
           "              A0l,A1l,A2l,A3l or A4l or a size in the format <width>x<height>\n"
           "              where <width> and <height> specify a physical length in one of the\n"
           "              formats <l>,<l>mm,<l>cm,<l>inch or <l>pt. <l> is a floating point\n"
           "              number, the default unit is PostScript pt (1/72 inch).\n"
	   "       --paper=<papersize>\n"
           "              Specify a paper size of the output device. All plots\n"
           "              are scaled to this paper size respecting the given border.\n"
           "              If no fixed paper size is given, each page in the output\n"
           "              file has an individual size adapted to the size of the HPGL\n"
           "              content of the page.\n"
           "              The format of <papersize> is decribed above for -s.\n"
	   "       --border=<border>\n"
           "              Specify a border for placing plots on output pages.\n"
	   "       --rotation=<angle>\n"
           "              Specify an angle for placing plots on output pages.\n"
	   "       --stamp=<stamp>\n"
           "              Specify a string, which is printed in the background.\n"
	   "       --stamp-encoding=<enc>\n"
           "              Specify an the name of the encoding of the string, which\n"
	   "              is given in --stamp. Default is iso-8859-1.\n"
	   "       --stamp-size=<pt>\n"
           "              Specify the size of the stamp specified with --stamp\n"
	   "              in point. Default value: 500pt.\n"
	   "       --dump-png=<filename>\n"
           "              Specify a filename for dumping inline PCL images in.\n"
           "              PNG format. The actual filenames written are:\n"
           "                 <filename>0001.png\n"
           "                 <filename>0002.png\n"
           "                  .....\n"
           "              If <filename> contains an extension '.png' the counter\n"
           "              is put between the basename and the '.png' extension.\n"
	   "       --help\n"
           "              Displays this usage information.\n"
	   "       --version\n"
           "              Displays the version information.\n"
	   "\n"
	   "Supported devices (option -d):\n"
	   "\n"
	   "       bbox    ... writes a PostScript BoundingBox to stdout.\n"
	   "       ps      ... writes a multipage PostScript file.\n"
	   "       eps     ... writes an eps files.\n"
	   "       png_256 ... writes an indexed 8bpp png image.\n"
	   "       png_gray... writes a grayscale 8bpp png image.\n"
	   "       png_rgb ... writes a rgb 24bpp png image.\n"
	   "       png_gray_alpha ... writes a grayscale 16bpp png image\n"
	   "                          with an alpha channel.\n"
	   "       png_rgb_alpha ... writes a rgb 32bpp png image\n"
	   "                         with an alpha channel.\n"
	   "       cairo_png ... writes a rgb 32bpp png image using\n"
	   "                     the cairo library (experimental).\n")
	   ,HPGS_VERSION );

  hpgs_cleanup();

  return 0;
}

static int get_double_arg(const char *arg, double *val)
{
  char * endptr = (char*)arg;
  *val = strtod(arg,&endptr);
  return (*endptr || endptr == arg) ? -1 : 0;
}

static int get_double_pair(const char *arg, double *xval, double *yval)
{
  char * endptr = (char*)arg;
  *xval = strtod(arg,&endptr);

  if (endptr == arg) return -1;
  if (*endptr == '\0') { *yval = *xval; return 0; }
  if (*endptr != 'x') return -1;

  const char *yarg=endptr+1;

  *yval = strtod(yarg,&endptr);
  return (*endptr || endptr == yarg) ? -1 : 0;
}

static int get_int_arg(const char *arg, int *val)
{
  char * endptr = (char*)arg;
  *val = strtol(arg,&endptr,10);
  return (*endptr) ? -1 : 0;
}

static int get_int_pair(const char *arg, int *xval, int *yval)
{
  char * endptr = (char*)arg;
  *xval = strtod(arg,&endptr);

  if (*endptr == '\0') { *yval = *xval; return 0; }
  if (*endptr != 'x') return -1;

  const char *yarg=endptr+1;

  *yval = strtod(yarg,&endptr);
  return (*endptr || endptr == yarg) ? -1 : 0;
}

// helper function for supporting old device-specific options, which
// are now global ones.
static int get_dev_arg_value(const char *dev_name,int dev_name_len,
                             const char *opt, const char *argv[],
                             const char **value, int *narg)
{
  char *opt_name = alloca(4+dev_name_len+strlen(opt));

  // contruct string --<dev_name><opt>
  opt_name[1] = opt_name[0] = '-';
  memcpy(opt_name+2,dev_name,dev_name_len);
  strcpy(&opt_name[2+dev_name_len],opt);
  
  return  hpgs_get_arg_value(opt_name,argv,value,narg);
}

/*
   The main program analyses the cmd line argument and delegates the work.
*/

#define HPGS_MAX_PLUGIN_ARGS 32

int main(int argc, const char *argv[])
{
  const char *dev_name="eps";
  int dev_name_len = 3;
  const char *out_fn=0;
  const char *stamp=0;
  const char *stamp_enc=0;
  const char *png_dump_fn=0;
  int ifile;
  char *aux_path;
  double x_dpi=72.0;
  double y_dpi=72.0;
  hpgs_bbox bbox= { 0.0, 0.0, 0.0, 0.0 };
  double paper_angle = 0.0;
  double paper_border = 0.0;
  double paper_width = 0.0;
  double paper_height = 0.0;
  double lw_factor=-1.0;
  double thin_alpha=0.25;
  double stamp_size=500.0;
  int x_px_size=0;
  int y_px_size=0;
  hpgs_bool multipage=HPGS_FALSE;
  hpgs_bool ignore_ps=HPGS_FALSE;
  hpgs_bool do_linewidth=HPGS_TRUE;
  hpgs_bool do_rop3=HPGS_TRUE;
  int verbosity=1;
  int compression=6;
  hpgs_bool antialias=HPGS_FALSE;
  int image_interpolation=0;
  hpgs_device *size_dev = 0;
  hpgs_device *plot_dev = 0;
  hpgs_istream *in = 0;
  hpgs_reader  *reader = 0;
  hpgs_png_image *image = 0;
  const char *plugin_argv[HPGS_MAX_PLUGIN_ARGS];
  int plugin_argc = 0;
  int ret = 1;

#ifdef LC_MESSAGES
  setlocale(LC_MESSAGES,"");
#endif
  setlocale(LC_CTYPE,"");

  aux_path = get_aux_path();

  if (!aux_path)
    {
      fprintf(stderr,hpgs_i18n("Error getting installation path: %s.\n"),
	      strerror(errno));
      return 1;
    }
  
  hpgs_init(aux_path);
  free(aux_path);

  memset(plugin_argv,0,sizeof(plugin_argv));

  ++argv;
  --argc;

  while (argc>0)
    {
      int narg = 1;
      const char *value;

      if (strcmp(argv[0],"--") == 0)
	{
	  ++argv;
	  --argc;
	  break;
	}
      else       if (strcmp(argv[0],"-v") == 0)
	{
	  ++verbosity;
	}
      else if (strcmp(argv[0],"-q") == 0 || strcmp(argv[0],"-dQUIET") == 0)
	{
	  --verbosity;
	}
      else if (strcmp(argv[0],"-i") == 0)
	{
	  ignore_ps=HPGS_TRUE;
	}
      else if (strcmp(argv[0],"-m") == 0)
	{
	  multipage=HPGS_TRUE;
	}
      else if (hpgs_get_arg_value("-d",argv,&value,&narg))
	{
	  dev_name = value;
          dev_name_len = strlen(dev_name);

          if (plugin_argc)
	    {
	      fprintf(stderr,hpgs_i18n("Error: Device options specified before -d argument.\n"));
	      return usage();
	    }
	}
      else if (hpgs_get_arg_value("--stamp=",argv,&value,&narg))
	{
	  stamp = value;
	}
      else if (hpgs_get_arg_value("--stamp-encoding=",argv,&value,&narg))
	{
	  stamp_enc = value;
	}
      else if (hpgs_get_arg_value("--stamp-size=",argv,&value,&narg))
	{
          if (hpgs_parse_length(value,&stamp_size))
	    {
	      fprintf(stderr,hpgs_i18n("Error: --stamp-size= must be followed by a valid length.\n"));
	      return usage();
	    }

	  if (stamp_size < 20.0 || stamp_size > 2000.0)
	    {
	      fprintf(stderr,hpgs_i18n("Error: stamp-size must lie in the interval [20,2000]pt.\n"));
	      return usage();
	    }
	}
      else if (hpgs_get_arg_value("--dump-png=",argv,&value,&narg))
	{
          png_dump_fn = value;
	}
       else if (hpgs_get_arg_value("-o",argv,&value,&narg))
	{
	  out_fn = value;
	}
      else if (strcmp(argv[0],"-a") == 0)
	{
          antialias = HPGS_TRUE;
	}
      else if (hpgs_get_arg_value("-I",argv,&value,&narg))
	{
	  if (get_int_arg(value,&image_interpolation))
	    {
	      if (narg == 1)
		{
		  fprintf(stderr,hpgs_i18n("Error: -I must be followed by an integer number.\n"));
		  return usage();
		}
	      else
		{
		  image_interpolation= 1;
		  narg = 1;
		}
	    }

	  if (image_interpolation < 0 || image_interpolation > 1)
	    {
	      fprintf(stderr,hpgs_i18n("Error: The argument to -I must be 0 or 1.\n"));
	      return usage();
	    }
	}
      else if (hpgs_get_arg_value("-c",argv,&value,&narg))
	{
	  if (get_int_arg(value,&compression))
	    {
	      fprintf(stderr,hpgs_i18n("Error: -c must be followed by an integer number.\n"));
	      return usage();
	    }
	  
	  if (compression < 1 || compression > 9)
	    {
	      fprintf(stderr,hpgs_i18n("Error: compression factor must lie in the interval [1,9].\n"));
	      return usage();
	    }
	}
      else if (hpgs_get_arg_value("-w",argv,&value,&narg))
	{
	  if (get_double_arg(value,&lw_factor))
	    {
	      fprintf(stderr,hpgs_i18n("Error: -w must be followed by a floating point number.\n"));
	      return usage();
	    }
	  
	  if (lw_factor < 0.0 || lw_factor > 10.0)
	    {
	      fprintf(stderr,hpgs_i18n("Error: Linewidth factor must lie in the interval [0.0,10.0].\n"));
	      return usage();
	    }
	}
      else if (hpgs_get_arg_value("--thin-alpha=",argv,&value,&narg))
	{
	  if (get_double_arg(value,&thin_alpha))
	    {
	      fprintf(stderr,hpgs_i18n("Error: --thin-alpha= must be followed by a floating point number.\n"));
	      return usage();
	    }
	  
	  if (thin_alpha < 0.01 || thin_alpha > 10.0)
	    {
	      fprintf(stderr,hpgs_i18n("Error: Thin alpha must lie in the interval [0.01,10.0].\n"));
	      return usage();
	    }
	}
      else if (hpgs_get_arg_value("-r",argv,&value,&narg))
	{
	  if (get_double_pair(value,&x_dpi,&y_dpi))
	    {
	      fprintf(stderr,hpgs_i18n("Error: -r must be followed by <res> or <xres>x<yres>.\n"));
	      return usage();
            }

          if (x_dpi < 5.0 || y_dpi < 5.0)
            {
              fprintf(stderr,hpgs_i18n("Error: Resolutions must be at least 5 dpi.\n"));
              return usage();
            }
	}
      else if (hpgs_get_arg_value("-p",argv,&value,&narg))
	{
	  if (get_int_pair(value,&x_px_size,&y_px_size))
	    {
	      fprintf(stderr,hpgs_i18n("Error: -p must be followed by <sz> or <xsz>x<ysz>.\n"));
	      return usage();
            }

          if (y_px_size < 20 ||  x_px_size < 20)
            {
              fprintf(stderr,hpgs_i18n("Error: Pixel sizes must be at least 20px.\n"));
              return usage();
            }
	}
      else if (hpgs_get_arg_value("-s",argv,&value,&narg))
	{
          double x_size,y_size;

          if (hpgs_parse_papersize(value,&x_size, &y_size)<0)
            {
	      fprintf(stderr,hpgs_i18n("Error: -s must be followed by a valid paper size.\n"));
	      return usage();
            }

          if (x_size < 72.0 || y_size < 72.0)
            {
              fprintf(stderr,hpgs_i18n("Error: The plot size must be at least 72 pt.\n"));
              return usage();
            }

          bbox.urx = x_size;
          bbox.ury = y_size;
	}
      else if (hpgs_get_arg_value("--paper=",argv,&value,&narg))
	{
          if (hpgs_parse_papersize(value,&paper_width,&paper_height)<0)
            {
	      fprintf(stderr,hpgs_i18n("Error: --paper= must be followed by a valid paper size.\n"));
	      return usage();
            }

          if (paper_width < 72.0 || paper_height < 72.0)
            {
              fprintf(stderr,hpgs_i18n("Error: The paper size must be at least 72 pt.\n"));
              return usage();
            }
	}
      else if (hpgs_get_arg_value("--border=",argv,&value,&narg) ||
               // Allow old device-specific border parameters.
               get_dev_arg_value(dev_name,dev_name_len,"-border=",argv,&value,&narg)     )
	{
          if (hpgs_parse_length(value,&paper_border)<0)
            {
	      fprintf(stderr,hpgs_i18n("Error: --border= must be followed by a valid length.\n"));
	      return usage();
            }

          if (paper_border < 0.0 || paper_border > 144.0)
            {
              fprintf(stderr,hpgs_i18n("Error: The page border must lie in the interval [0,144] pt.\n"));
              return usage();
            }
	}
      else if (hpgs_get_arg_value("--rotation=",argv,&value,&narg) ||
               // Allow old device-specific rotation parameters.
               get_dev_arg_value(dev_name,dev_name_len,"-rotation=",argv,&value,&narg)       )
	{
          if (hpgs_parse_length(value,&paper_angle)<0)
            {
	      fprintf(stderr,hpgs_i18n("Error: --angle= must be followed by a valid angle.\n"));
	      return usage();
            }
	}
      else if (strcmp(argv[0],"--linewidth-size") == 0)
	{
	  do_linewidth = HPGS_TRUE;
	}
      else if (strcmp(argv[0],"--no-linewidth-size") == 0)
  	{
	  do_linewidth = HPGS_FALSE;
	}
      else if (strcmp(argv[0],"--rop3") == 0)
	{
	  do_rop3 = HPGS_TRUE;
	}
      else if (strcmp(argv[0],"--no-rop3") == 0)
	{
	  do_rop3 = HPGS_FALSE;
	}
      else if (strcmp(argv[0],"--help") == 0)
	{
	  return help();
	}
      else if (strcmp(argv[0],"--version") == 0)
	{
	  usage();
	  return 0;
	}
      else if (strncmp(argv[0],"--",2) == 0 &&
               strncmp(argv[0]+2,dev_name,dev_name_len) == 0 &&
               argv[0][dev_name_len+2] == '-')
	{
          int l=strlen(argv[0]);

          if (plugin_argc >= HPGS_MAX_PLUGIN_ARGS-1)
	    {
	      fprintf(stderr,hpgs_i18n("Error: Number of plugin-args exceeds maximum of %d.\n"),
                      HPGS_MAX_PLUGIN_ARGS-1);
	      return usage();
	    }

          plugin_argv[plugin_argc] = argv[0]+dev_name_len+2;
          ++plugin_argc;

          if (argv[0][l-1] == '=')
            {
              if (plugin_argc >= HPGS_MAX_PLUGIN_ARGS-1)
                {
                  fprintf(stderr,hpgs_i18n("Error: Number of plugin-args exceed maximum of %d.\n"),
                          HPGS_MAX_PLUGIN_ARGS-1);
                  usage();
                }

              plugin_argv[plugin_argc] = argv[2];
              ++plugin_argc;

              narg=2;
            }
	}
      else
	{
	  if (argv[0][0] == '-')
	    {
	      fprintf(stderr,hpgs_i18n("Error: Unknown option %s given.\n"),argv[0]);
	      return usage();
	    }

          break;
	}

      argv+=narg;
      argc-=narg;
    }
  
  if (argc < 1)
    {
      fprintf(stderr,hpgs_i18n("Error: No input file given.\n"));
      return usage();
    }

  if (argc > 1 && !multipage)
    {
      multipage = HPGS_TRUE;
    }

  // adjust default linewidth factor.
  if (lw_factor < 0.0)
    {
      // This is the best choice for our basic, non-antialiased renderer,
      // which rounds up pixel line widths.
      if (antialias == 0 && strncmp(dev_name,"png_",4) == 0)
        lw_factor = 0.5;
      else
        lw_factor = 1.0;
    }

  size_dev = (hpgs_device*)hpgs_new_plotsize_device(ignore_ps,do_linewidth);

  if (!size_dev)
    {
      fprintf(stderr,hpgs_i18n("Error: Cannot create plotsize device.\n"));
      goto cleanup;
    }

  in = hpgs_new_file_istream(argv[0]);

  if (!in)
    {
      fprintf(stderr,hpgs_i18n("Error: Cannot open input file %s: %s\n"),
	      argv[0],strerror(errno));
      hpgs_device_destroy((hpgs_device*)size_dev);
      goto cleanup;
    }

  reader = hpgs_new_reader(in,size_dev,
			   multipage,verbosity);

  if (!reader)
    {
      fprintf(stderr,hpgs_i18n("Error: Cannot create hpgl reader: %s\n"),strerror(errno));
      goto cleanup;
    }
 
  hpgs_reader_set_lw_factor(reader,lw_factor);

  // determine plot size, if not specified on the cmd line
  if (bbox.urx < 72.0 || bbox.ury < 72.0)
    {
      // read in multiple pages.
      for (ifile = 1; ifile < argc; ++ifile)
        {
          if (hpgs_read(reader,HPGS_FALSE))
            {
              fprintf(stderr,hpgs_i18n("Error: Cannot determine plot size of file %s: %s\n"),
                      argv[ifile-1],hpgs_get_error());
              goto cleanup;
            }

          in = hpgs_new_file_istream(argv[ifile]);

          if (!in)
            {
              fprintf(stderr,hpgs_i18n("Error: Cannot open input file %s: %s\n"),
                      argv[ifile],strerror(errno));
              goto cleanup;
            }

          hpgs_reader_attach(reader,in);
        }
      
      if (hpgs_read(reader,HPGS_TRUE))
	{
	  fprintf(stderr,hpgs_i18n("Error: Cannot determine plot size of file %s: %s\n"),
		  argv[ifile-1],hpgs_get_error());
	  goto cleanup;
	}

      // get bounding box of first page.
      // (will return overall boundingbox, if in singlepage mode.)
      if (hpgs_getplotsize(size_dev,1,&bbox)<0)
	{
	  fprintf(stderr,hpgs_i18n("Error: Cannot determine plotsize: %s\n"),
		  hpgs_get_error());
	  goto cleanup;
	}

      if (hpgs_bbox_isempty(&bbox))
	{
	  fprintf(stderr,hpgs_i18n("Error: Empty bounding:  %g %g %g %g.\n"),
		  bbox.llx,bbox.lly,bbox.urx,bbox.ury);
	  goto cleanup;
	}

      if (verbosity >= 1)
	fprintf(stderr,"BoundingBox: %g %g %g %g.\n",bbox.llx,bbox.lly,bbox.urx,bbox.ury);
    }

  // set the appropriate page placement.
  if (paper_width > 0.0 && paper_height > 0.0)
    {
      hpgs_reader_set_fixed_page(reader,&bbox,
                                 paper_width,paper_height,
                                 paper_border,paper_angle );
    }
  else
    {
      paper_width = 200.0 * 72.0;
      paper_height = 200.0 * 72.0;

      hpgs_reader_set_dynamic_page(reader,&bbox,
                                   paper_width,paper_height,
                                   paper_border,paper_angle );
    }

  if (strcmp(dev_name,"bbox") == 0)
    {
      int i;
      FILE *out = out_fn ? fopen(out_fn,"wb") : stdout;

      if (!out)
	{
	  fprintf(stderr,hpgs_i18n("Error: Cannot open output file <%s>.\n"),out_fn);
	  goto cleanup;
	}

      for (i=0;i<10000;++i)
        {
          int r = hpgs_getplotsize(size_dev,i,&bbox);

          if (r < 0)
            {
              fprintf(stderr,hpgs_i18n("Error: Cannot determine plotsize: %s\n"),
                      hpgs_get_error());
              fclose(out);
              goto cleanup;
            }

          if (r) break;

          if (i>0)
            {
              fprintf(out,"%%%%Page: %d %d.\n",i,i);
              fprintf(out,"%%%%PageBoundingBox: %d %d %d %d.\n",
                      (int)floor(bbox.llx),(int)floor(bbox.lly),
                      (int)ceil(bbox.urx),(int)ceil(bbox.ury));
              fprintf(out,"%%%%PageHiResBoundingBox: %g %g %g %g.\n",bbox.llx,bbox.lly,bbox.urx,bbox.ury);
            }
          else
            {
              fprintf(out,"%%%%BoundingBox: %d %d %d %d.\n",
                      (int)floor(bbox.llx),(int)floor(bbox.lly),
                      (int)ceil(bbox.urx),(int)ceil(bbox.ury));
              fprintf(out,"%%%%HiResBoundingBox: %g %g %g %g.\n",bbox.llx,bbox.lly,bbox.urx,bbox.ury);
            }
        }

      if (out != stdout) fclose(out);

      ret=0;
      goto cleanup;
    }
  else if (strcmp(dev_name,"eps") == 0)
    {
      plot_dev =
	(hpgs_device*)hpgs_new_eps_device(out_fn,&bbox,do_rop3);

      if (!plot_dev)
	{
	  fprintf(stderr,hpgs_i18n("Error: Cannot create eps device.\n"));
	  goto cleanup;
	}
    }
  else if (strcmp(dev_name,"ps") == 0)
    {
      plot_dev =
	(hpgs_device*)hpgs_new_ps_device(out_fn,&bbox,do_rop3);

      if (!plot_dev)
	{
	  fprintf(stderr,hpgs_i18n("Error: Cannot create postscript device.\n"));
	  goto cleanup;
	}
    }
  else if (strncmp(dev_name,"png_",4) == 0)
    {
      int depth = 8;
      int palette = 0;
      hpgs_paint_device *pdv;

      if (strcmp(dev_name+4,"gray") == 0)
	depth = 8;
      else if (strcmp(dev_name+4,"gray_alpha") == 0)
	depth = 16;
      else if (strcmp(dev_name+4,"rgb") == 0)
	depth = 24;
      else if (strcmp(dev_name+4,"rgb_alpha") == 0)
	depth = 32;
      else if (strcmp(dev_name+4,"256") == 0)
	palette = 1;
      else
	{
	  fprintf(stderr,hpgs_i18n("Error: png device %s in not supported.\n"),
		  dev_name);
	  goto cleanup;
	}

      if (x_px_size >= 20 && y_px_size >= 20)
	{
          // get overall bounding box, if pixel size is specified.
          // This way no page image exceeds the given size and
          // all images have the same resolution.
          hpgs_bbox bb;

          if (hpgs_getplotsize(size_dev,0,&bb)<0)
            {
              fprintf(stderr,hpgs_i18n("Error: Cannot determine overall plotsize: %s\n"),
                      hpgs_get_error());
              goto cleanup;
            }

	  x_dpi = 72.0 * x_px_size / (bb.urx-bb.llx);
	  y_dpi = 72.0 * y_px_size / (bb.ury-bb.lly);

	  if (x_dpi > y_dpi)
	    {
	      x_dpi = y_dpi;
	      x_px_size = x_dpi * (bb.urx-bb.llx) / 72.0;
	    }
	  else
	    {
	      y_dpi = x_dpi;
	      y_px_size = y_dpi * (bb.ury-bb.lly) / 72.0;
	    }
	}
      else
	{
          // initialize the pixel size from the first page.
	  x_px_size = x_dpi * (bbox.urx-bbox.llx) / 72.0;
	  y_px_size = y_dpi * (bbox.ury-bbox.lly) / 72.0;
	}

      image = hpgs_new_png_image(x_px_size,y_px_size,depth,palette,do_rop3);
	
      if (!image)
	{
	  fprintf(stderr,hpgs_i18n("Error creating %dx%dx%d sized png image: %s.\n"),
		  x_px_size,y_px_size,depth,strerror(errno));
	  goto cleanup;
	}

      hpgs_png_image_set_compression(image,compression);

      pdv = hpgs_new_paint_device((hpgs_image*)image,
                                  out_fn,&bbox,
                                  antialias);
      if (!pdv)
	{
	  fprintf(stderr,hpgs_i18n("Error: Cannot create paint device.\n"));
	  goto cleanup;
	}

      hpgs_paint_device_set_image_interpolation(pdv,image_interpolation);
      hpgs_paint_device_set_thin_alpha(pdv,thin_alpha);
      // set the resolution of the image, although
      // hpgs_new_paint_device has done this before.
      // The reason is, that we know the resolution with more precision
      // than hpgs_new_paint_device.
      hpgs_image_set_resolution((hpgs_image*)image,x_dpi,y_dpi);

      plot_dev = (hpgs_device *)pdv;
    }
  else
    {
      if (x_px_size >= 20 && y_px_size >= 20)
	{
          // calculate resolution from overall bounding box,
          // if pixel size is specified.
          // This way no page image exceeds the given size and
          // all images have the same resolution.
          hpgs_bbox bb;

          if (hpgs_getplotsize(size_dev,0,&bb)<0)
            {
              fprintf(stderr,hpgs_i18n("Error: Cannot determine overall plotsize: %s\n"),
                      hpgs_get_error());
              goto cleanup;
            }

	  x_dpi = 72.0 * x_px_size / (bb.urx-bb.llx);
	  y_dpi = 72.0 * y_px_size / (bb.ury-bb.lly);

	  if (x_dpi > y_dpi)
	    x_dpi = y_dpi;
	  else
	    y_dpi = x_dpi;
	}

      void *page_asset_ctxt = 0;
      hpgs_reader_asset_func_t page_asset_func = 0;
      void *frame_asset_ctxt = 0;
      hpgs_reader_asset_func_t frame_asset_func = 0;

      if (hpgs_new_plugin_device(&plot_dev,
                                 &page_asset_ctxt,
                                 &page_asset_func,
                                 &frame_asset_ctxt,
                                 &frame_asset_func,
                                 dev_name,out_fn,&bbox,
                                 x_dpi,y_dpi,do_rop3,
                                 plugin_argc,plugin_argv))
	{
	  fprintf(stderr,hpgs_i18n("Error: Cannot create plugin device: %s\n"),
                  hpgs_get_error());
	  goto cleanup;
	}

      if (page_asset_func)
        hpgs_reader_set_page_asset_func(reader,page_asset_ctxt,page_asset_func);

      if (frame_asset_func)
        hpgs_reader_set_frame_asset_func(reader,frame_asset_ctxt,frame_asset_func);
    }

  if (!plot_dev)
    {
      fprintf (stderr,hpgs_i18n("Error: invalid plot device name %s specified.\n"),dev_name);
      goto cleanup;
    }

  if (png_dump_fn && hpgs_reader_set_png_dump(reader,png_dump_fn))
    {
      fprintf(stderr,hpgs_i18n("Error: Cannot set png_dump filename to reader: %s\n"),
	      hpgs_get_error());
      goto cleanup;
    }

  if (stamp && hpgs_device_stamp(plot_dev,&bbox,stamp,stamp_enc,stamp_size))
    {
      fprintf(stderr,hpgs_i18n("Error: Cannot stamp plot: %s\n"),
	      hpgs_get_error());
      goto cleanup;
    }

  if (hpgs_reader_imbue(reader,plot_dev))
    {
      fprintf(stderr,hpgs_i18n("Error: Cannot imbue plot device to reader: %s\n"),
	      hpgs_get_error());
      goto cleanup;
    }

  // re-open first file, if we have more than one input file.
  if (argc > 1)
    {
      in = hpgs_new_file_istream(argv[0]);

      if (!in)
        {
          fprintf(stderr,hpgs_i18n("Error: Cannot open input file %s: %s\n"),
                  argv[0],strerror(errno));
          goto cleanup;
        }
      hpgs_reader_attach(reader,in);
    }

  // read in multiple pages.
  for (ifile = 1; ifile < argc; ++ifile)
    {
      if (hpgs_read(reader,HPGS_FALSE))
        {
          fprintf(stderr,hpgs_i18n("Error: Cannot process plot file %s: %s\n"),
                  argv[ifile-1],hpgs_get_error());
          goto cleanup;
        }
      
      in = hpgs_new_file_istream(argv[ifile]);

      if (!in)
        {
          fprintf(stderr,hpgs_i18n("Error: Cannot open input file %s: %s\n"),
                  argv[ifile],strerror(errno));
          goto cleanup;
        }
      
      hpgs_reader_attach(reader,in);
    }

  if (hpgs_read(reader,HPGS_TRUE))
    {
      fprintf(stderr,hpgs_i18n("Error: Cannot process plot file %s: %s\n"),
	      argv[ifile-1],hpgs_get_error());
      goto cleanup;
    }

  if (verbosity >= 2)
    fprintf(stderr,hpgs_i18n("Success.\n"));

  ret = 0;

 cleanup:
  if (reader)
    hpgs_destroy_reader(reader);

  hpgs_cleanup();

  return ret;
}
