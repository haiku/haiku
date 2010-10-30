/*
 * "$Id: xml-curve.c,v 1.7 2008/08/02 15:10:56 rleigh Exp $"
 *
 *   Copyright 2002 Robert Krawitz (rlk@alum.mit.edu)
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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <gutenprint/gutenprint.h>

int main(int argc, char *argv[])
{
  stp_curve_t *curve;

  if (argc != 2)
    {
      fprintf(stderr, "Usage: %s filename.xml\n", argv[0]);
      return 1;
    }

  stp_init();

#ifdef DEBUG
  fprintf(stderr, "stp-xml-parse: reading  `%s'...\n", argv[1]);
#endif

  fprintf(stderr, "Using file: %s\n", argv[1]);
  curve = stp_curve_create_from_file(argv[1]);

  if (curve)
    {
      char *output;
      if ((stp_curve_write(stdout, curve)) == 0)
        fprintf(stderr, "curve successfully created\n");
      else
	fprintf(stderr, "error creating curve\n");
      output = stp_curve_write_string(curve);
      if (output)
	{
	  fprintf(stderr, "%s", output);
	  fprintf(stderr, "curve string successfully created\n");
	  free(output);
	}
      else
	fprintf(stderr, "error creating curve string\n");
      stp_curve_destroy(curve);
    }
  else
    printf("curve is NULL!\n");

  return 0;
}
