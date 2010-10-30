/*		-*- Mode: C -*-
 *  $Id: gutenprint-version.h.in,v 1.1 2004/09/17 18:38:01 rleigh Exp $
 *
 *   Version of Gimp-print
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
 *
 * Revision History:
 *
 *   See ChangeLog
 */

/**
 * @file gutenprint-version.h
 * @brief Version functions.
 */

#ifndef GUTENPRINT_VERSION_H
#define GUTENPRINT_VERSION_H

/**
 * Version information.  Version information may be used to check the
 * library version at compile-time, using macros, or at run-time,
 * using constants.
 *
 * @defgroup version version
 * @{
 */


/*
 * Autogen-time versioning.
 */
#define STP_MAJOR_VERSION       (5)
#define STP_MINOR_VERSION       (2)
#define STP_MICRO_VERSION       (6)
#define STP_CURRENT_INTERFACE   (2)
#define STP_BINARY_AGE          (0)
#define STP_INTERFACE_AGE       (7)

#define STP_CHECK_VERSION(major,minor,micro)	\
  (STP_MAJOR_VERSION >  (major) ||		\
  (STP_MAJOR_VERSION == (major) &&		\
   STP_MINOR_VERSION > (minor)) ||		\
  (STP_MAJOR_VERSION == (major) &&		\
   STP_MINOR_VERSION == (minor) &&		\
   STP_MICRO_VERSION >= (micro)))

/** The library major version number. */
extern const unsigned int stp_major_version;
/** The library minor version number. */
extern const unsigned int stp_minor_version;
/** The library micro version number. */
extern const unsigned int stp_micro_version;
/** The library ABI revision number (number of incompatible revisions). */
extern const unsigned int stp_current_interface;
/** The library ABI binary age number (number of forward-compatible revisions). */
extern const unsigned int stp_binary_age;
/** The library ABI interface age number (number of revisions of this ABI). */
extern const unsigned int stp_interface_age;

/**
 * Check whether the library provides the requested version.
 * @param required_major the minimum major revision.
 * @param required_minor the minimum minor revision.
 * @param required_micro the minimum micro revision.
 * @returns NULL if the version matches, or else a description of the
 * error if the library is too old or too new.
 */
extern const char *stp_check_version(unsigned int required_major,
				     unsigned int required_minor,
				     unsigned int required_micro);

  /** @} */

#endif /* GUTENPRINT_VERSION_H */
