/*
 * "$Id: gutenprint-intl-internal.h,v 1.4 2008/08/13 07:35:51 easysw Exp $"
 *
 *   I18N header file for the gimp-print.
 *
 *   Copyright 1997-2008 Michael Sweet (mike@easysw.com),
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

/**
 * @file gutenprint/gutenprint-intl-internal.h
 * @brief Internationalisation functions.
 */

#ifndef GUTENPRINT_INTL_INTERNAL_H
#define GUTENPRINT_INTL_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Internationalisation functions are used to localise Gimp-Print by
   * translating strings into the user's native language.
   *
   * The macros defined in this header are convenience wrappers around
   * the gettext functions provided by libintl library (or directly by
   * libc on GNU systems).  They differ from the normal intl functions
   * in that the textdomain is fixed, for use by functions internal to
   * Gimp-Print.  This header should not be included by source files
   * outside the gimp-print source tree.
   *
   * @defgroup intl_internal intl-internal
   * @{
   */

#ifdef INCLUDE_LOCALE_H
INCLUDE_LOCALE_H
#else
#include <locale.h>
#endif

#if defined(ENABLE_NLS) && !defined(DISABLE_NLS)
#  include <libintl.h>
/** Translate String. */
#  define _(String) dgettext (PACKAGE, String)
#  undef gettext
/** Translate String. */
#  define gettext(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
/** Mark String for translation, but don't translate it right now. */
#      define N_(String) gettext_noop (String)
#  else
/** Mark String for translation, but don't translate it right now. */
#        define N_(String) (String)
#  endif
#else /* ifndef ENABLE_NLS */
/* Stubs that do something close enough.  */
#    define textdomain(String) (String)
#    define gettext(String) (String)
#    define dgettext(Domain,Message) (Message)
#    define dcgettext(Domain,Message,Type) (Message)
#    define bindtextdomain(Domain,Directory) (Domain)
#    define _(String) (String)
#    define N_(String) (String)
#endif

  /** @} */

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_INTL_INTERNAL_H */
