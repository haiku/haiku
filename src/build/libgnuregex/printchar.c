/* Extended regular expression matching and search library,
   version 0.12.
   (Implements POSIX draft P10003.2/D11.2, except for
   internationalization features.)

   Copyright (C) 1993 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* (this file was downloaded from
 * https://raw.githubusercontent.com/ge-ne/bibtool/master/regex-0.12/test/printchar.c ,
 * where no copyright header is included).
 */
#ifdef DEBUG

void
printchar (c)
     char c;
{
  if (c < 040 || c >= 0177)
    {
      putchar ('\\');
      putchar (((c >> 6) & 3) + '0');
      putchar (((c >> 3) & 7) + '0');
      putchar ((c & 7) + '0');
    }
  else
    putchar (c);
}

#endif
