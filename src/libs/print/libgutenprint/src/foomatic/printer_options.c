/*
 * "$Id: printer_options.c,v 1.52 2007/05/06 19:38:10 rlk Exp $"
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
  stp_parameter_level_t max_level = STP_PARAMETER_LEVEL_ADVANCED4;
  stp_string_list_t *params_seen;
  if (argc > 1 && !strcmp(argv[1], "-s"))
    max_level = STP_PARAMETER_LEVEL_BASIC;

  stp_init();
  params_seen = stp_string_list_create();
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

      params = stp_get_parameter_list(pv);
      nparams = stp_parameter_list_count(params);

      for (k = 0; k < nparams; k++)
	{
	  const stp_parameter_t *p = stp_parameter_list_param(params, k);
	  if (p->read_only ||
	      (p->p_level > max_level && strcmp(p->name, "Resolution") != 0) ||
	      (p->p_class != STP_PARAMETER_CLASS_OUTPUT &&
	       p->p_class != STP_PARAMETER_CLASS_FEATURE))
	    continue;
	  count = 0;
	  stp_describe_parameter(pv, p->name, &desc);
	  if (desc.is_active)
	    {
	      char buf[1024];
	      sprintf(buf, "STP_%s", desc.name);
	      if (!stp_string_list_find(params_seen, buf))
		{
		  stp_string_list_add_string(params_seen, buf, buf);
		  if ((desc.p_type == STP_PARAMETER_TYPE_DOUBLE ||
		       desc.p_type == STP_PARAMETER_TYPE_DIMENSION ||
		       desc.p_type == STP_PARAMETER_TYPE_INT) &&
		      !desc.is_mandatory)
		    {
		      sprintf(buf, "STP_Enable%s", desc.name);
		      if (!stp_string_list_find(params_seen, buf))
			{
			  stp_string_list_add_string(params_seen, buf, buf);
			  /*
			   * Create a dummy option that enables or disables
			   * the option as appropriate.  The long name ends in
			   * enable, rather than starts with enable, because
			   * CUPS has this nasty habit of sorting options
			   * alphabetically rather than leaving them in the
			   * order listed.  This ensures that the enable
			   * option is adjacent to the value it controls.
			   */
			  printf("$longnames{'STP_Enable%s'} = '%s Enable';",
				 desc.name, desc.text);
			  printf("$param_classes{'STP_Enable%s'} = %d;",
				 desc.name, desc.p_class);
			  printf("$param_levels{'STP_Enable%s'} = %d;",
				 desc.name, desc.p_level);
			  printf("$longnames{'STP_%s'} = '%s Value';",
				 desc.name, desc.text);
			}
		    }
		  else
		    printf("$longnames{'STP_%s'} = '%s';",
			   desc.name, desc.text);
		  printf("$param_classes{'STP_%s'} = %d;",
			 desc.name, desc.p_class);
		  printf("$param_levels{'STP_%s'} = %d;",
			 desc.name, desc.p_level);
		}
	      if ((desc.p_type == STP_PARAMETER_TYPE_DOUBLE ||
		   desc.p_type == STP_PARAMETER_TYPE_DIMENSION ||
		   desc.p_type == STP_PARAMETER_TYPE_INT) &&
		  !desc.is_mandatory)
		{
		  printf("$defaults{'%s'}{'STP_Enable%s'} = 'Disabled';",
			 driver, desc.name);
		  printf("$stpdata{'%s'}{'STP_Enable%s'}{'Disabled'} = 'Disabled';",
			 driver, desc.name);
		  printf("$stpdata{'%s'}{'STP_Enable%s'}{'Enabled'} = 'Enabled';",
			 driver, desc.name);
		}
	      if (desc.p_type == STP_PARAMETER_TYPE_STRING_LIST)
		{
		  count = stp_string_list_count(desc.bounds.str);
		  if (count > 0)
		    {
		      printf("{ $stpdata{'%s'}{'STP_%s'} = {};",
			     driver, desc.name);
		      printf("my $tmp = $stpdata{'%s'}{'STP_%s'};",
			     driver, desc.name);
		      if (strcmp(desc.name, "Resolution") == 0)
			{
			  printf("$stpdata{'%s'}{'x_resolution'} = {};",
				 driver);
			  printf("my $x_t = $stpdata{'%s'}{'x_resolution'};",
				 driver);
			  printf("$stpdata{'%s'}{'y_resolution'} = {};",
				 driver);
			  printf("my $y_t = $stpdata{'%s'}{'y_resolution'};",
				 driver);
			}
		      if (desc.is_mandatory)
			{
			  printf("$defaults{'%s'}{'STP_%s'} = '%s';",
				 driver, desc.name, desc.deflt.str);
			}
		      else
			{
			  printf("$defaults{'%s'}{'STP_%s'} = '%s';",
				 driver, desc.name, "None");
			  printf("$$tmp{'%s'} = '%s';", "None", "None");
			}
		      for (j = 0; j < count; j++)
			{
			  const stp_param_string_t *param =
			    stp_string_list_param(desc.bounds.str, j);
			  printf("$$tmp{'%s'} = '%s';",
				 param->name, param->text);
			  if (strcmp(desc.name, "Resolution") == 0)
			    {
			      int x, y;
			      stp_set_string_parameter(pv, "Resolution",
						       param->name);
			      stp_describe_resolution(pv, &x, &y);
			      if (x > 0 && y > 0)
				{
				  printf("$$x_t{'%s'} = '%d';",param->name, x);
				  printf("$$y_t{'%s'} = '%d';",param->name, y);
				}
			    }
			}
		      printf("}\n");
		    }
		}
	      else if (desc.p_type == STP_PARAMETER_TYPE_BOOLEAN)
		{
		  if (desc.is_mandatory)
		    {
		      printf("$defaults{'%s'}{'STP_%s'} = '%d';",
			     driver, desc.name, desc.deflt.boolean);
		    }
		  else
		    {
		      printf("$defaults{'%s'}{'STP_%s'} = '%s';",
			     driver, desc.name, "None");
		      printf("$stpdata{'%s'}{'STP_%s'}{'%s'} = '%s';",
			     driver, desc.name, "None", "None");
		    }
		    
		  printf("$stpdata{'%s'}{'STP_%s'}{'False'} = 'False';",
			 driver, desc.name);
		  printf("$stpdata{'%s'}{'STP_%s'}{'True'} = 'True';\n",
			 driver, desc.name);
		}
	      else if (desc.p_type == STP_PARAMETER_TYPE_DOUBLE)
		{
		  if (desc.bounds.dbl.lower <= desc.deflt.dbl &&
		      desc.bounds.dbl.upper >= desc.deflt.dbl)
		    {
		      printf("{ $stp_float_values{'%s'}{'STP_%s'} = {};",
			     driver, desc.name);
		      printf("my $tmp = $stp_float_values{'%s'}{'STP_%s'};",
			     driver, desc.name);
		      printf("$$tmp{'MINVAL'} = %.3f;",
			     desc.bounds.dbl.lower);
		      printf("$$tmp{'MAXVAL'} = %.3f;",
			     desc.bounds.dbl.upper);
		      printf("$$tmp{'DEFVAL'} = %.3f;",
			     desc.deflt.dbl);
		      /* printf("$$tmp{'LONG_NAME'} = '%s';",
			 gettext(desc.text)); */
		      printf("$$tmp{'CATEGORY'} = '%s';",
			     gettext(desc.category));
		      printf("$$tmp{'HELP'} = q(%s);",
			     (desc.help ? gettext(desc.help) : "''"));
		      printf("$$tmp{'MANDATORY'} = q(%d);",
			     desc.is_mandatory);
		      printf("}\n");
		    }
		}
	      else if (desc.p_type == STP_PARAMETER_TYPE_INT)
		{
		  if (desc.bounds.integer.lower <= desc.deflt.integer &&
		      desc.bounds.integer.upper >= desc.deflt.integer)
		    {
		      printf("{ $stp_int_values{'%s'}{'STP_%s'} = {};",
			     driver, desc.name);
		      printf("my $tmp = $stp_int_values{'%s'}{'STP_%s'};",
			     driver, desc.name);
		      printf("$$tmp{'MINVAL'} = %d;",
			     desc.bounds.integer.lower);
		      printf("$$tmp{'MAXVAL'} = %d;",
			     desc.bounds.integer.upper);
		      printf("$$tmp{'DEFVAL'} = %d;",
			     desc.deflt.integer);
		      /* printf("$$tmp{'LONG_NAME'} = '%s';",
			 gettext(desc.text)); */
		      printf("$$tmp{'CATEGORY'} = '%s';",
			     gettext(desc.category));
		      printf("$$tmp{'HELP'} = q(%s);",
			     (desc.help ? gettext(desc.help) : "''"));
		      printf("$$tmp{'MANDATORY'} = q(%d);",
			     desc.is_mandatory);
		      printf("}\n");
		    }
		}
	      else if (desc.p_type == STP_PARAMETER_TYPE_DIMENSION)
		{
		  if (desc.bounds.dimension.lower <= desc.deflt.dimension &&
		      desc.bounds.dimension.upper >= desc.deflt.dimension)
		    {
		      printf("{ $stp_dimension_values{'%s'}{'STP_%s'} = {};",
			     driver, desc.name);
		      printf("my $tmp = $stp_dimension_values{'%s'}{'STP_%s'};",
			     driver, desc.name);
		      printf("$$tmp{'MINVAL'} = %d;",
			     desc.bounds.dimension.lower);
		      printf("$$tmp{'MAXVAL'} = %d;",
			     desc.bounds.dimension.upper);
		      printf("$$tmp{'DEFVAL'} = %d;",
			     desc.deflt.dimension);
		      /* printf("$$tmp{'LONG_NAME'} = '%s';",
			 gettext(desc.text)); */
		      printf("$$tmp{'CATEGORY'} = '%s';",
			     gettext(desc.category));
		      printf("$$tmp{'HELP'} = q(%s);",
			     (desc.help ? gettext(desc.help) : "''"));
		      printf("$$tmp{'MANDATORY'} = q(%d);",
			     desc.is_mandatory);
		      printf("}\n");
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
	      printf("$defaults{'%s'}{'%s'} = '%s';",
		     driver, "Color", "Color");
	      printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';",
		     driver, "Color", "Color", "Color");
	      printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
		     driver, "Color", "RawCMYK", "Raw CMYK");
	    }
	  else
	    printf("$defaults{'%s'}{'%s'} = '%s';",
		   driver, "Color", "Grayscale");
	  printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';",
		 driver, "Color", "Grayscale", "Gray Scale");
	  printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
		 driver, "Color", "BlackAndWhite", "Black and White");
	}
      stp_vars_destroy(pv);
    }
  return 0;
}
