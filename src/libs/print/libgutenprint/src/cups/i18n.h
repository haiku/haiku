/*
 * "$Id: i18n.h,v 1.3 2009/04/11 19:05:12 rlk Exp $"
 *
 *   Internationalization definitions for CUPS drivers.
 *
 *   Copyright 2008 Michael Sweet (mike@easysw.com)
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

#include <gutenprint/gutenprint.h>
#include <gutenprint/string-list.h>


/*
 * Macro for localizing driver messages...
 */

#define _(x) x


/*
 * Prototypes...
 */

extern const stp_string_list_t	*stp_i18n_load(const char *locale);
extern const char		*stp_i18n_lookup(const stp_string_list_t *po,
				                 const char *message);
extern void			stp_i18n_printf(const stp_string_list_t *po,
				                const char *message, ...);


/*
 * End of "$Id: i18n.h,v 1.3 2009/04/11 19:05:12 rlk Exp $".
 */
