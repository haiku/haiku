/* Print --version and bug-reporting information in a consistent format.
   Copyright (C) 1999, 2003, 2005, 2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Jim Meyering. */

#ifndef VERSION_ETC_H
# define VERSION_ETC_H 1

# include <stdarg.h>
# include <stdio.h>

extern const char version_etc_copyright[];

extern void version_etc_va (FILE *stream,
			    const char *command_name, const char *package,
			    const char *version, va_list authors);

extern void version_etc (FILE *stream,
			 const char *command_name, const char *package,
			 const char *version,
			 /* const char *author1, ...*/ ...);

extern void emit_bug_reporting_address (void);

#endif /* VERSION_ETC_H */
