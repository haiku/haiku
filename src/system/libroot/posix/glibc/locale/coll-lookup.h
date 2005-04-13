/* Copyright (C) 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Bruno Haible <haible@clisp.cons.org>, 2000.

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

/* Word tables are accessed by cutting wc in three blocks of bits:
   - the high 32-q-p bits,
   - the next q bits,
   - the next p bits.

	    +------------------+-----+-----+
     wc  =  +      32-q-p      |  q  |  p  |
	    +------------------+-----+-----+

   p and q are variable.  For 16-bit Unicode it is sufficient to
   choose p and q such that q+p <= 16.

   The table contains the following uint32_t words:
   - q+p,
   - s = upper exclusive bound for wc >> (q+p),
   - p,
   - 2^q-1,
   - 2^p-1,
   - 1st-level table: s offsets, pointing into the 2nd-level table,
   - 2nd-level table: k*2^q offsets, pointing into the 3rd-level table,
   - 3rd-level table: j*2^p words, each containing 32 bits of data.
*/

#include <stdint.h>

/* Lookup in a table of int32_t, with default value 0.  */
static inline int32_t
collidx_table_lookup (const char *table, uint32_t wc)
{
  uint32_t shift1 = ((const uint32_t *) table)[0];
  uint32_t index1 = wc >> shift1;
  uint32_t bound = ((const uint32_t *) table)[1];
  if (index1 < bound)
    {
      uint32_t lookup1 = ((const uint32_t *) table)[5 + index1];
      if (lookup1 != 0)
	{
	  uint32_t shift2 = ((const uint32_t *) table)[2];
	  uint32_t mask2 = ((const uint32_t *) table)[3];
	  uint32_t index2 = (wc >> shift2) & mask2;
	  uint32_t lookup2 = ((const uint32_t *)(table + lookup1))[index2];
	  if (lookup2 != 0)
	    {
	      uint32_t mask3 = ((const uint32_t *) table)[4];
	      uint32_t index3 = wc & mask3;
	      int32_t lookup3 = ((const int32_t *)(table + lookup2))[index3];

	      return lookup3;
	    }
	}
    }
  return 0;
}

/* Lookup in a table of uint32_t, with default value 0xffffffff.  */
static inline uint32_t
collseq_table_lookup (const char *table, uint32_t wc)
{
  uint32_t shift1 = ((const uint32_t *) table)[0];
  uint32_t index1 = wc >> shift1;
  uint32_t bound = ((const uint32_t *) table)[1];
  if (index1 < bound)
    {
      uint32_t lookup1 = ((const uint32_t *) table)[5 + index1];
      if (lookup1 != 0)
	{
	  uint32_t shift2 = ((const uint32_t *) table)[2];
	  uint32_t mask2 = ((const uint32_t *) table)[3];
	  uint32_t index2 = (wc >> shift2) & mask2;
	  uint32_t lookup2 = ((const uint32_t *)(table + lookup1))[index2];
	  if (lookup2 != 0)
	    {
	      uint32_t mask3 = ((const uint32_t *) table)[4];
	      uint32_t index3 = wc & mask3;
	      uint32_t lookup3 = ((const uint32_t *)(table + lookup2))[index3];

	      return lookup3;
	    }
	}
    }
  return ~((uint32_t) 0);
}
