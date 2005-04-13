/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -acCgopt -k'1,2,5,9,$' -L ANSI-C -N charmap_hash programs/charmap-kw.gperf  */
/* Copyright (C) 1995, 1996, 1997, 1998, 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper, <drepper@gnu.org>.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <string.h>

#include "locfile-token.h"
struct keyword_t ;

#define TOTAL_KEYWORDS 17
#define MIN_WORD_LENGTH 3
#define MAX_WORD_LENGTH 14
#define MIN_HASH_VALUE 3
#define MAX_HASH_VALUE 35
/* maximum key range = 33, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36, 36, 36, 25, 10,
      15, 20, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36, 36, 10,  0,  0,
       5, 36,  0,  0, 36, 36, 36,  0,  0, 36,
       0, 36,  0, 36,  0, 36, 36,  0, 36, 36,
      36, 36, 36, 36, 36,  0, 36,  0,  0,  0,
      10,  0, 36,  0,  0,  0, 36, 36, 36,  0,
       0,  0,  0,  0,  0,  0,  0,  0, 36, 36,
      25, 36, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
      36, 36, 36, 36, 36, 36
    };
  register int hval = len;

  switch (hval)
    {
      default:
      case 9:
        hval += asso_values[(unsigned char)str[8]];
      case 8:
      case 7:
      case 6:
      case 5:
        hval += asso_values[(unsigned char)str[4]];
      case 4:
      case 3:
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

#ifdef __GNUC__
__inline
#endif
const struct keyword_t *
charmap_hash (register const char *str, register unsigned int len)
{
  static const struct keyword_t wordlist[] =
    {
      {""}, {""}, {""},
      {"END",             tok_end,             0},
      {""},
      {"WIDTH",           tok_width,           0},
      {"escseq",          tok_escseq,          1},
      {"include",         tok_include,         1},
      {""}, {""},
      {"mb_cur_min",      tok_mb_cur_min,      1},
      {"escape_char",     tok_escape_char,     1},
      {"comment_char",    tok_comment_char,    1},
      {"code_set_name",   tok_code_set_name,   1},
      {"WIDTH_VARIABLE",  tok_width_variable,  0},
      {"g1esc",           tok_g1esc,           1},
      {"addset",          tok_addset,          1},
      {"CHARMAP",         tok_charmap,         0},
      {"WIDTH_DEFAULT",   tok_width_default,   0},
      {""},
      {"g2esc",           tok_g2esc,           1},
      {""}, {""}, {""}, {""},
      {"g3esc",           tok_g3esc,           1},
      {""}, {""}, {""}, {""},
      {"g0esc",           tok_g0esc,           1},
      {""}, {""}, {""}, {""},
      {"mb_cur_max",      tok_mb_cur_max,      1}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].name;

          if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
            return &wordlist[key];
        }
    }
  return 0;
}
