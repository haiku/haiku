/*
 * "$Id: print-version.c,v 1.8 2005/04/10 23:15:16 rlk Exp $"
 *
 *   Print plug-in driver utility functions for the GIMP.
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
 * compile on generic platforms that don't support glib, gimp, etc.
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>

const unsigned int stp_major_version = STP_MAJOR_VERSION;
const unsigned int stp_minor_version = STP_MINOR_VERSION;
const unsigned int stp_micro_version = STP_MICRO_VERSION;
const unsigned int stp_current_interface = STP_CURRENT_INTERFACE;
const unsigned int stp_binary_age = STP_BINARY_AGE;
const unsigned int stp_interface_age = STP_INTERFACE_AGE;


const char *
stp_check_version (unsigned int required_major,
		   unsigned int required_minor, unsigned int required_micro)
{
  if (required_major > STP_MAJOR_VERSION)
    return "Gutenprint version too old (major mismatch)";
  if (required_major < STP_MAJOR_VERSION)
    return "Gutenprint version too new (major mismatch)";
  if (required_minor > STP_MINOR_VERSION)
    return "Gutenprint version too old (minor mismatch)";
  if (required_minor < STP_MINOR_VERSION)
    return "Gutenprint version too new (minor mismatch)";
  if (required_micro < STP_MICRO_VERSION - STP_BINARY_AGE)
    return "Gutenprint version too new (micro mismatch)";
  if (required_micro > STP_MICRO_VERSION)
    return "Gutenprint version too old (micro mismatch)";
  return NULL;
}

const char *
stp_get_version(void)
{
  static const char *version_id = VERSION;
  return version_id;
}

const char *
stp_get_release_version(void)
{
  static const char *release_version_id = GUTENPRINT_RELEASE_VERSION;
  return release_version_id;
}
