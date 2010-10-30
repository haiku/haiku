/*
 * "$Id: printer_options.c,v 1.5 2008/06/07 22:47:35 rlk Exp $"
 *
 *   Dump the per-printer options for the OpenPrinting database
 *
 *   Copyright 2000 Robert Krawitz (rlk@alum.mit.edu)
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
#include <gutenprint/gutenprint-intl.h>

int
main(int argc, char **argv)
{
  int i, j, k;
  int first_arg = 1;
  stp_string_list_t *printer_list = NULL;
  stp_parameter_level_t max_level = STP_PARAMETER_LEVEL_ADVANCED4;
  if (argc > 1 && !strcmp(argv[1], "-s"))
    {
      max_level = STP_PARAMETER_LEVEL_BASIC;
      first_arg++;
    }

  stp_init();
  
  if (argc > first_arg)
    {
      printer_list = stp_string_list_create();
      for (i = 1; i < argc; i++)
	stp_string_list_add_string(printer_list, argv[i], argv[i]);
    }
  for (i = 0; i < stp_printer_model_count(); i++)
    {
      stp_parameter_list_t params;
      int nparams;
      stp_parameter_t desc;
      const stp_printer_t *printer = stp_get_printer_by_index(i);
      const char *driver = stp_printer_get_driver(printer);
      const char *family = stp_printer_get_family(printer);
      stp_vars_t *pv = stp_vars_create_copy(stp_printer_get_defaults(printer));
      int tcount = 0;
      size_t count;
      int printer_is_color = 0;
      if (strcmp(family, "ps") == 0 || strcmp(family, "raw") == 0)
	continue;
      if (printer_list && !stp_string_list_is_present(printer_list, driver))
	continue;

      /* Set Job Mode to "Job" as this enables the Duplex option */
      stp_set_string_parameter(pv, "JobMode", "Job");

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
	     stp_printer_get_long_name(printer));

      printf("$families{'%s'} = '%s';\n", driver, family);
      printf("$models{'%s'} = '%d';\n", driver, stp_get_model_id(pv));

      params = stp_get_parameter_list(pv);
      nparams = stp_parameter_list_count(params);

      for (k = 0; k < nparams; k++)
	{
	  const stp_parameter_t *p = stp_parameter_list_param(params, k);
	  if (p->read_only ||
	      (p->p_level > max_level && strcmp(p->name, "Resolution") != 0) ||
	      (p->p_class != STP_PARAMETER_CLASS_OUTPUT &&
	       p->p_class != STP_PARAMETER_CLASS_CORE &&
	       p->p_class != STP_PARAMETER_CLASS_FEATURE))
	    continue;
	  count = 0;
	  stp_describe_parameter(pv, p->name, &desc);
	  if (desc.is_active)
	    {
	      printf("$longnames{'%s'}{'%s'} = '%s';\n",
		     driver, desc.name, desc.text);
	      printf("$param_classes{'%s'}{'%s'} = %d;\n",
		     driver, desc.name, desc.p_class);
	      printf("$param_types{'%s'}{'%s'} = %d;\n",
		     driver, desc.name, desc.p_type);
	      printf("$param_levels{'%s'}{'%s'} = %d;\n",
		     driver, desc.name, desc.p_level);
	      if (desc.p_type == STP_PARAMETER_TYPE_STRING_LIST)
		{
		  count = stp_string_list_count(desc.bounds.str);
		  if (count > 0)
		    {
		      if (desc.is_mandatory)
			{
			  printf("$defaults{'%s'}{'%s'} = '%s';\n",
				 driver, desc.name, desc.deflt.str);
			}
		      else
			{
			  printf("$defaults{'%s'}{'%s'} = '%s';\n",
				 driver, desc.name, "None");
			  printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
				 driver, desc.name, "None", "None");
			}
		      for (j = 0; j < count; j++)
			{
			  const stp_param_string_t *param =
			    stp_string_list_param(desc.bounds.str, j);
			  printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
				 driver, desc.name, param->name, param->text);
			  if (strcmp(desc.name, "Resolution") == 0)
			    {
			      int x, y;
			      stp_set_string_parameter(pv, "Resolution",
						       param->name);
			      stp_describe_resolution(pv, &x, &y);
			      if (x > 0 && y > 0)
				{
				  printf("$stpdata{'%s'}{'%s'}{'%s'} = '%d';\n",
					 driver, "x_resolution", param->name, x);
				  printf("$stpdata{'%s'}{'%s'}{'%s'} = '%d';\n",
					 driver, "y_resolution", param->name, y);
				}
			    }
			}
		    }
		}
	      else if (desc.p_type == STP_PARAMETER_TYPE_BOOLEAN)
		{
		  if (desc.is_mandatory)
		    {
		      printf("$defaults{'%s'}{'%s'} = '%d';\n",
			     driver, desc.name, desc.deflt.boolean);
		    }
		  else
		    {
		      printf("$defaults{'%s'}{'%s'} = '%s';\n",
			     driver, desc.name, "None");
		      printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
			     driver, desc.name, "None", "None");
		    }
		    
		  printf("$stpdata{'%s'}{'%s'}{'False'} = 'False';\n",
			 driver, desc.name);
		  printf("$stpdata{'%s'}{'%s'}{'True'} = 'True';\n",
			 driver, desc.name);
		}
	      else if (desc.p_type == STP_PARAMETER_TYPE_DOUBLE)
		{
		  if (desc.bounds.dbl.lower <= desc.deflt.dbl &&
		      desc.bounds.dbl.upper >= desc.deflt.dbl)
		    {
		      printf("$stp_float_values{'%s'}{'MINVAL'}{'%s'} = %.3f;\n",
			     driver, desc.name, desc.bounds.dbl.lower);
		      printf("$stp_float_values{'%s'}{'MAXVAL'}{'%s'} = %.3f;\n",
			     driver, desc.name, desc.bounds.dbl.upper);
		      printf("$stp_float_values{'%s'}{'DEFVAL'}{'%s'} = %.3f;\n",
			     driver, desc.name, desc.deflt.dbl);
		      /* printf("$stp_float_values{'%s'}{'LONG_NAME'}{'%s'} = '%s';\n",
			 driver, desc.name, gettext(desc.text)); */
		      printf("$stp_float_values{'%s'}{'CATEGORY'}{'%s'} = '%s';\n",
			     driver, desc.name, gettext(desc.category));
		      printf("$stp_float_values{'%s'}{'HELP'}{'%s'} = q(%s);\n",
			     driver, desc.name, (desc.help ? gettext(desc.help) : "''"));
		      printf("$stp_float_values{'%s'}{'MANDATORY'}{'%s'} = q(%d);\n",
			     driver, desc.name, desc.is_mandatory);
		    }
		}
	      else if (desc.p_type == STP_PARAMETER_TYPE_INT)
		{
		  if (desc.bounds.integer.lower <= desc.deflt.integer &&
		      desc.bounds.integer.upper >= desc.deflt.integer)
		    {
		      printf("$stp_int_values{'%s'}{'MINVAL'}{'%s'} = %d;\n",
			     driver, desc.name, desc.bounds.integer.lower);
		      printf("$stp_int_values{'%s'}{'MAXVAL'}{'%s'} = %d;\n",
			     driver, desc.name, desc.bounds.integer.upper);
		      printf("$stp_int_values{'%s'}{'DEFVAL'}{'%s'} = %d;\n",
			     driver, desc.name, desc.deflt.integer);
		      /* printf("$stp_int_values{'%s'}{'LONG_NAME'}{'%s'} = '%s';\n",
			 driver, desc.name, gettext(desc.text)); */
		      printf("$stp_int_values{'%s'}{'CATEGORY'}{'%s'} = '%s';\n",
			     driver, desc.name, gettext(desc.category));
		      printf("$stp_int_values{'%s'}{'HELP'}{'%s'} = q(%s);\n",
			     driver, desc.name, (desc.help ? gettext(desc.help) : "''"));
		      printf("$stp_int_values{'%s'}{'MANDATORY'}{'%s'} = q(%d);\n",
			     driver, desc.name, desc.is_mandatory);
		    }
		}
	      else if (desc.p_type == STP_PARAMETER_TYPE_DIMENSION)
		{
		  if (desc.bounds.dimension.lower <= desc.deflt.dimension &&
		      desc.bounds.dimension.upper >= desc.deflt.dimension)
		    {
		      printf("$stp_dimension_values{'%s'}{'MINVAL'}{'%s'} = %d;\n",
			     driver, desc.name, desc.bounds.dimension.lower);
		      printf("$stp_dimension_values{'%s'}{'MAXVAL'}{'%s'} = %d;\n",
			     driver, desc.name, desc.bounds.dimension.upper);
		      printf("$stp_dimension_values{'%s'}{'DEFVAL'}{'%s'} = %d;\n",
			     driver, desc.name, desc.deflt.dimension);
		      /* printf("$stp_dimension_values{'%s'}{'LONG_NAME'}{'%s'} = '%s';\n",
			 driver, desc.name, gettext(desc.text)); */
		      printf("$stp_dimension_values{'%s'}{'CATEGORY'}{'%s'} = '%s';\n",
			     driver, desc.name, gettext(desc.category));
		      printf("$stp_dimension_values{'%s'}{'HELP'}{'%s'} = q(%s);\n",
			     driver, desc.name, (desc.help ? gettext(desc.help) : "''"));
		      printf("$stp_dimension_values{'%s'}{'MANDATORY'}{'%s'} = q(%d);\n",
			     driver, desc.name, desc.is_mandatory);
		    }
		}
	      tcount += count;
	    }
	  stp_parameter_description_destroy(&desc);
	}
      stp_parameter_list_destroy(params);
      if (tcount > 0)
	{
	  if (printer_is_color)
	    {
	      printf("$defaults{'%s'}{'%s'} = '%s';\n",
		     driver, "Color", "Color");
	      printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
		     driver, "Color", "Color", "Color");
	      printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
		     driver, "Color", "RawCMYK", "Raw CMYK");
	    }
	  else
	    printf("$defaults{'%s'}{'%s'} = '%s';\n",
		   driver, "Color", "Grayscale");
	  printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
		 driver, "Color", "Grayscale", "Gray Scale");
	  printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
		 driver, "Color", "BlackAndWhite", "Black and White");
	}
      stp_vars_destroy(pv);
    }
  return 0;
}
