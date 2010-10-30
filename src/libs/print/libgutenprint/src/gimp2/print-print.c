/*
 * "$Id: print-print.c,v 1.1 2006/07/04 02:57:59 rlk Exp $"
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

#include "print-intl.h"

void
do_gimp_install_procedure(const char *blurb, const char *help,
			  const char *auth, const char *copy,
			  const char *types, int n_args,
			  GimpParamDef *args)
{
  gimp_install_procedure ((BAD_CONST_CHAR) "file_print_gimp",
			  (BAD_CONST_CHAR) blurb,
			  (BAD_CONST_CHAR) help,
			  (BAD_CONST_CHAR) auth,
			  (BAD_CONST_CHAR) copy,
			  (BAD_CONST_CHAR) VERSION " - " RELEASE_DATE,
			  /* Do not translate the prefix "<Image>" */
			  (BAD_CONST_CHAR) N_("<Image>/File/Print..."),
			  (BAD_CONST_CHAR) types,
			  GIMP_PLUGIN,
			  n_args, 0,
			  args, NULL);
}
			  
