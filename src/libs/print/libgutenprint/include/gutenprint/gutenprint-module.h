/*		-*- Mode: C -*-
 *  $Id: gutenprint-module.h,v 1.1 2004/09/17 18:38:01 rleigh Exp $
 *
 *   Gimp-Print module header file
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
 * @file gutenprint/gutenprint-module.h
 * @brief Gutenprint module header.
 * This header includes all of the public headers used by modules.
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifndef GUTENPRINT_GUTENPRINT_MODULE_H
#define GUTENPRINT_GUTENPRINT_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

#define STP_MODULE 1

#include <gutenprint/gutenprint.h>

#include <gutenprint/bit-ops.h>
#include <gutenprint/channel.h>
#include <gutenprint/color.h>
#include <gutenprint/dither.h>
#include <gutenprint/list.h>
#include <gutenprint/module.h>
#include <gutenprint/path.h>
#include <gutenprint/weave.h>
#include <gutenprint/xml.h>

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_MODULE_H */
/*
 * End of $Id: gutenprint-module.h,v 1.1 2004/09/17 18:38:01 rleigh Exp $
 */
