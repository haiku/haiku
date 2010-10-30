/*
 * "$Id: printer_margins.c,v 1.21 2007/05/06 19:38:10 rlk Exp $"
 *
 *   Dump the per-printer margins for the OpenPrinting database
 *
 *   Copyright 2000, 2003 Robert Krawitz (rlk@alum.mit.edu) and
 *                        Till Kamppeter (till.kamppeter@gmail.com)
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
#include <stdio.h>
#include <string.h>
#include <gutenprint/gutenprint.h>

int
main(int argc, char **argv) {
  int i, k;
  int use_all_page_sizes = 1;
  if (argc > 1 && !strcmp(argv[1], "-s"))
    use_all_page_sizes = 0;

  stp_init();
  for (i = 0; i < stp_printer_model_count(); i++) {
    const stp_printer_t *p = stp_get_printer_by_index(i);
    const char *driver = stp_printer_get_driver(p);
    const char *family = stp_printer_get_family(p);
    stp_vars_t *pv = 
      stp_vars_create_copy(stp_printer_get_defaults(p));
    stp_parameter_t desc;
    int num_opts;
    int printer_is_color = 0;
    const stp_param_string_t *opt;
    int width, height, bottom, left, top, right;
    if (strcmp(family, "ps") == 0 || strcmp(family, "raw") == 0)
      continue;
    stp_describe_parameter(pv, "PrintingMode", &desc);
    if (stp_string_list_is_present(desc.bounds.str, "Color"))
      printer_is_color = 1;
    stp_parameter_description_destroy(&desc);
    if (printer_is_color)
      stp_set_string_parameter(pv, "PrintingMode", "Color");
    else
      stp_set_string_parameter(pv, "PrintingMode", "BW");
    stp_set_string_parameter(pv, "ChannelBitDepth", "8");
    printf("# Printer model %s, long name `%s'\n", driver,
	   stp_printer_get_long_name(p));
    stp_describe_parameter(pv, "PageSize", &desc);
    printf("$defaults{'%s'}{'PageSize'} = '%s';\n",
	   driver, desc.deflt.str);
    num_opts = stp_string_list_count(desc.bounds.str);
    
    for (k = 0; k < num_opts; k++) {
      const stp_papersize_t *papersize;
      opt = stp_string_list_param(desc.bounds.str, k);
      papersize = stp_get_papersize_by_name(opt->name);
      
      if (!papersize) {
	printf("Unable to lookup size %s!\n", opt->name);
	continue;
      }
      if (!use_all_page_sizes && num_opts >= 10 &&
	  (papersize->paper_unit == PAPERSIZE_ENGLISH_EXTENDED ||
	   papersize->paper_unit == PAPERSIZE_METRIC_EXTENDED))
	continue;
      
      width  = papersize->width;
      height = papersize->height;
      
      stp_set_string_parameter(pv, "PageSize", opt->name);
      
      stp_get_media_size(pv, &width, &height);
      stp_get_maximum_imageable_area(pv, &left, &right, &bottom, &top);

      if (left < 0)
	left = 0;
      if (right > width)
	right = width;
      if (bottom > height)
	bottom = height;
      if (top < 0)
	top = 0;
      
      bottom = height - bottom;
      top    = height - top;

      if (strcmp(opt->name, "Custom") == 0) {
	/* Use relative values for the custom size */
	right = width - right;
	top = height - top;
	width = 0;
	height = 0;
      }

      printf("$stpdata{'%s'}{'PageSize'}{'%s'} = '%s';",
	     driver, opt->name, opt->text);
      printf("$imageableareas{'%s'}{'%s'} = {",
	     driver, opt->name);
      printf("  'left' => '%d',", left);
      printf("  'right' => '%d',", right);
      printf("  'top' => '%d',", top);
      printf("  'bottom' => '%d',", bottom);
      printf("  'width' => '%d',", width);
      printf("  'height' => '%d'", height);
      printf("};\n");
    }
    stp_parameter_description_destroy(&desc);
    stp_vars_destroy(pv);
  }
  return 0;
}
