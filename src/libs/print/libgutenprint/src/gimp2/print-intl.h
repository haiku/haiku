/*
 * "$Id: print-intl.h,v 1.3 2004/09/17 18:38:13 rleigh Exp $"
 *
 *   I18N header file for the GIMP2 Print plugin.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and Michael Natterer (mitch@gimp.org)
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
#ifndef GIMP2_PRINT_INTL_H
#define GIMP2_PRINT_INTL_H

#include <glib.h>
#include <gutenprint/gutenprint-intl.h>

#define INIT_LOCALE(domain)   					\
do								\
{								\
        gtk_set_locale ();					\
        setlocale (LC_NUMERIC, "C");				\
        bindtextdomain (domain, PACKAGE_LOCALE_DIR);		\
        textdomain (domain);					\
} while (0)

#endif /* GIMP2_PRINT_INTL_H */
