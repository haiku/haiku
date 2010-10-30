/*
 * "$Id: util.h,v 1.9 2010/08/04 00:33:55 rlk Exp $"
 *
 *   libgimpprint utility and miscellaneous functions.
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

/**
 * @file gutenprint/util.h
 * @brief Utility functions.
 */

#ifndef GUTENPRINT_UTIL_H
#define GUTENPRINT_UTIL_H

#include <gutenprint/curve.h>
#include <gutenprint/vars.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__GNUC__) && !defined(__attribute__)
#  define __attribute__(x)
#endif /* !__GNUC__ && !__attribute__ */

/**
 * Utility functions.
 *
 * @defgroup util util
 * @{
 */

/**
 * Initialise libgimpprint.
 * This function must be called prior to any other use of the library.
 * It is responsible for loading modules and XML data and initialising
 * internal data structures.
 * @returns 0 on success, 1 on failure.
 */
extern int stp_init(void);

/**
 * Set the output encoding.  This function sets the encoding that all
 * strings translated by gettext are output in.  It is a wrapper
 * around the gettext bind_textdomain_codeset() function.
 * @param codeset the standard name of the encoding, which must be
 * usable with iconv_open().  For example, "US-ASCII" or "UTF-8".  If
 * NULL, the currently-selected codeset will be returned (or NULL if
 * no codeset has been selected yet).
 * @returns a string containing the selected codeset, or NULL on
 * failure (errno is set accordingly).
 */
extern const char *stp_set_output_codeset(const char *codeset);

extern stp_curve_t *stp_read_and_compose_curves(const char *s1, const char *s2,
						stp_curve_compose_t comp,
						size_t piecewise_point_count);
extern void stp_abort(void);

/*
 * Remove inactive and unclaimed options from the list
 */
extern void stp_prune_inactive_options(stp_vars_t *v);


extern void stp_zprintf(const stp_vars_t *v, const char *format, ...)
       __attribute__((format(__printf__, 2, 3)));

extern void stp_zfwrite(const char *buf, size_t bytes, size_t nitems,
			const stp_vars_t *v);

extern void stp_write_raw(const stp_raw_t *raw, const stp_vars_t *v);

extern void stp_putc(int ch, const stp_vars_t *v);
extern void stp_put16_le(unsigned short sh, const stp_vars_t *v);
extern void stp_put16_be(unsigned short sh, const stp_vars_t *v);
extern void stp_put32_le(unsigned int sh, const stp_vars_t *v);
extern void stp_put32_be(unsigned int sh, const stp_vars_t *v);
extern void stp_puts(const char *s, const stp_vars_t *v);
extern void stp_putraw(const stp_raw_t *r, const stp_vars_t *v);
extern void stp_send_command(const stp_vars_t *v, const char *command,
			     const char *format, ...);

extern void stp_erputc(int ch);

extern void stp_eprintf(const stp_vars_t *v, const char *format, ...)
       __attribute__((format(__printf__, 2, 3)));
extern void stp_erprintf(const char *format, ...)
       __attribute__((format(__printf__, 1, 2)));
extern void stp_asprintf(char **strp, const char *format, ...)
       __attribute__((format(__printf__, 2, 3)));
extern void stp_catprintf(char **strp, const char *format, ...)
       __attribute__((format(__printf__, 2, 3)));

#define STP_DBG_LUT 		0x1
#define STP_DBG_COLORFUNC	0x2
#define STP_DBG_INK		0x4
#define STP_DBG_PS		0x8
#define STP_DBG_PCL		0x10
#define STP_DBG_ESCP2		0x20
#define STP_DBG_CANON		0x40
#define STP_DBG_LEXMARK	        0x80
#define STP_DBG_WEAVE_PARAMS	0x100
#define STP_DBG_ROWS		0x200
#define STP_DBG_MARK_FILE	0x400
#define STP_DBG_LIST		0x800
#define STP_DBG_MODULE		0x1000
#define STP_DBG_PATH		0x2000
#define STP_DBG_PAPER		0x4000
#define STP_DBG_PRINTERS	0x8000
#define STP_DBG_XML		0x10000
#define STP_DBG_VARS		0x20000
#define STP_DBG_DYESUB		0x40000
#define STP_DBG_CURVE		0x80000
#define STP_DBG_CURVE_ERRORS	0x100000
#define STP_DBG_PPD		0x200000
#define STP_DBG_NO_COMPRESSION	0x400000
#define STP_DBG_ASSERTIONS	0x800000

extern unsigned long stp_get_debug_level(void);
extern void stp_dprintf(unsigned long level, const stp_vars_t *v,
			const char *format, ...)
       __attribute__((format(__printf__, 3, 4)));
extern void stp_deprintf(unsigned long level, const char *format, ...)
       __attribute__((format(__printf__, 2, 3)));
extern void stp_init_debug_messages(stp_vars_t *v);
extern void stp_flush_debug_messages(stp_vars_t *v);


extern void *stp_malloc (size_t);
extern void *stp_zalloc (size_t);
extern void *stp_realloc (void *ptr, size_t);
extern void stp_free(void *ptr);

#define STP_SAFE_FREE(x)			\
do						\
{						\
  if ((x))					\
    stp_free((char *)(x));			\
  ((x)) = NULL;					\
} while (0)

extern size_t stp_strlen(const char *s);
extern char *stp_strndup(const char *s, int n);
extern char *stp_strdup(const char *s);

/**
 * Get the library version string (x.y.z)
 * @returns a pointer to the version name of the package, which must not
 * be modified or freed.
 */
extern const char *stp_get_version(void);

/**
 * Get the library release version string (x.y)
 * @returns a pointer to the release name of the package, which must not
 * be modified or freed.
 */
extern const char *stp_get_release_version(void);

/** @} */

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_UTIL_H */
/*
 * End of "$Id: util.h,v 1.9 2010/08/04 00:33:55 rlk Exp $".
 */
