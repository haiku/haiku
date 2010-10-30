/*
 * "$Id: path.h,v 1.2 2008/06/01 14:41:03 rlk Exp $"
 *
 *   libgimpprint path functions header
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and Michael Natterer (mitch@gimp.org)
 *   Copyright 2002 Roger Leigh (rleigh@debian.org)
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

/**
 * @file gutenprint/path.h
 * @brief Simple directory path functions.
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifndef GUTENPRINT_PATH_H
#define GUTENPRINT_PATH_H

#ifdef __cplusplus
extern "C" {
#endif


extern stp_list_t *stp_path_search(stp_list_t *dirlist,
				   const char *suffix);

extern void stp_path_split(stp_list_t *list,
			   const char *path);

extern stp_list_t *stpi_data_path(void);

extern stp_list_t *stpi_list_files_on_data_path(const char *name);

extern char *stpi_path_merge(const char *path, const char *file);


#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_PATH_H */
/*
 * End of "$Id: path.h,v 1.2 2008/06/01 14:41:03 rlk Exp $".
 */
