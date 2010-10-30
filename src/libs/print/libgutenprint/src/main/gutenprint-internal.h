/*
 * "$Id: gutenprint-internal.h,v 1.3 2010/08/04 00:33:56 rlk Exp $"
 *
 *   Print plug-in header file for the GIMP.
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
 *
 * Revision History:
 *
 *   See ChangeLog
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifndef GUTENPRINT_INTERNAL_INTERNAL_H
#define GUTENPRINT_INTERNAL_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gutenprint/gutenprint-module.h>

/**
 * Utility functions (internal).
 *
 * @defgroup util_internal util-internal
 * @{
 */

extern void stpi_init_paper(void);
extern void stpi_init_dither(void);
extern void stpi_init_printer(void);
#define BUFFER_FLAG_FLIP_X	0x1
#define BUFFER_FLAG_FLIP_Y	0x2
extern stp_image_t* stpi_buffer_image(stp_image_t* image, unsigned int flags);

#define STPI_ASSERT(x,v)						\
do									\
{									\
  if (stp_get_debug_level() & STP_DBG_ASSERTIONS)			\
    stp_erprintf("DEBUG: Testing assertion %s file %s line %d\n",	\
		 #x, __FILE__, __LINE__);				\
  if (!(x))								\
    {									\
      stp_erprintf("\nERROR: ***Gutenprint %s assertion %s failed!"	\
		   " file %s, line %d.  %s\n", PACKAGE_VERSION,		\
		   #x, __FILE__, __LINE__, "Please report this bug!");	\
      stp_abort();							\
    }									\
} while (0)

/** @} */


#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_INTERNAL_INTERNAL_H */
/*
 * End of "$Id: gutenprint-internal.h,v 1.3 2010/08/04 00:33:56 rlk Exp $".
 */
