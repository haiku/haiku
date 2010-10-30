/*
 * "$Id: print_gimp.h,v 1.5 2006/07/04 02:57:59 rlk Exp $"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu). and Steve Miller (smiller@rni.net
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
 *
 *
 * Revision History:
 *
 *   See ChangeLog
 */

#ifndef __PRINT_GIMP_H__
#define __PRINT_GIMP_H__

#ifdef __GNUC__
#define inline __inline__
#endif

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <gutenprint/gutenprint.h>
#include <gutenprintui2/gutenprintui.h>

/* How to create an Image wrapping a Gimp drawable */
extern stpui_image_t *Image_GimpDrawable_new(GimpDrawable *drawable, gint32);

extern void do_gimp_install_procedure(const char *blurb, const char *help,
				      const char *auth, const char *copy,
				      const char *types, int n_args,
				      GimpParamDef *args);

/*
 * Work around GIMP library not being const-safe.  This is a very ugly
 * hack, but the excessive warnings generated can mask more serious
 * problems.
 */

#define BAD_CONST_CHAR char *

#endif  /* __PRINT_GIMP_H__ */
