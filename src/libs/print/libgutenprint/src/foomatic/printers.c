/*
 * "$Id: printers.c,v 1.14 2010/08/07 02:30:38 rlk Exp $"
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

int
main(int argc, char **argv)
{
  int i;
  int status = 0;

  stp_init();
  for (i = 0; i < stp_printer_model_count(); i++)
    {
      const stp_printer_t *p = stp_get_printer_by_index(i);
      const char *driver = stp_printer_get_driver(p);
      const char *long_name = stp_printer_get_long_name(p);
      const char *manufacturer = stp_printer_get_manufacturer(p);
      const char *family = stp_printer_get_family(p);
      const char *foomatic_id = stp_printer_get_foomatic_id(p);

      if (foomatic_id)
	{
	  printf("if (defined($foomap{'%s'})) { print STDERR \"\\n*** Duplicate printer %s\"; $errors++; } ",
		 driver, driver);
	  printf("if (defined($mapfoo{'%s'})) { print STDERR \"\\n*** Duplicate foomatic ID %s\"; $errors++; } ",
		 foomatic_id, foomatic_id);
	  printf("$printer_name{'%s'} = '%s';", driver, long_name);
	  printf("$printer_make{'%s'} = '%s';", driver, manufacturer);
	  printf("$printer_family{'%s'} = '%s';", driver, family);
	  printf("$foomap{'%s'} = '%s';", driver, foomatic_id);
	  printf("$mapfoo{'%s'} = '%s';", foomatic_id, driver);
	  printf("push (@{$mapstp{'%s'}}, 'printer/%s');", driver, foomatic_id);
	  printf("push @printer_list, '%s';\n", driver);
	}
      else if (strcmp(family, "raw") != 0 && strcmp(family, "ps") != 0)
	{
	  fprintf(stderr, "No foomatic ID for printer %s!\n", driver);
	  status = 1;
	}

    }
  return status;
}
