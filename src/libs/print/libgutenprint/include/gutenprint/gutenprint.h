/*		-*- Mode: C -*-
 *  $Id: gutenprint.h,v 1.1 2004/09/17 18:38:01 rleigh Exp $
 *
 *   Gimp-Print header file
 *
 *   Copyright 1997-2002 Michael Sweet (mike@easysw.com) and
 *      Robert Krawitz (rlk@alum.mit.edu)
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
 * Revision History:
 *
 *   See ChangeLog
 */

/**
 * @file gutenprint/gutenprint.h
 * @brief Gutenprint master header.
 * This header includes all of the public headers.
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifndef GUTENPRINT_H
#define GUTENPRINT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>     /* For size_t */
#include <stdio.h>	/* For FILE */

#include <gutenprint/array.h>
#include <gutenprint/curve.h>
#include <gutenprint/gutenprint-version.h>
#include <gutenprint/image.h>
#include <gutenprint/paper.h>
#include <gutenprint/printers.h>
#include <gutenprint/sequence.h>
#include <gutenprint/string-list.h>
#include <gutenprint/util.h>
#include <gutenprint/vars.h>

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_H */
/*
 * End of $Id: gutenprint.h,v 1.1 2004/09/17 18:38:01 rleigh Exp $
 */
