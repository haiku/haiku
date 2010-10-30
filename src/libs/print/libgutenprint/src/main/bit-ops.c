/*
 * "$Id: bit-ops.c,v 1.14 2009/06/07 15:21:22 rlk Exp $"
 *
 *   Softweave calculator for Gutenprint.
 *
 *   Copyright 2000 Charles Briscoe-Smith <cpbs@debian.org>
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

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <string.h>
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

void
stp_fold(const unsigned char *line,
	 int single_length,
	 unsigned char *outbuf)
{
  int i;
  memset(outbuf, 0, single_length * 2);
  for (i = 0; i < single_length; i++)
    {
      unsigned char l0 = line[0];
      unsigned char l1 = line[single_length];
      if (l0 || l1)
	{
	  outbuf[0] =		/* B7 A7 B6 A6 B5 A5 B4 A4 */
	    ((l0 & (1 << 7)) >> 1) +
	    ((l0 & (1 << 6)) >> 2) +
	    ((l0 & (1 << 5)) >> 3) +
	    ((l0 & (1 << 4)) >> 4) +
	    ((l1 & (1 << 7)) >> 0) +
	    ((l1 & (1 << 6)) >> 1) +
	    ((l1 & (1 << 5)) >> 2) +
	    ((l1 & (1 << 4)) >> 3);
	  outbuf[1] =		/* B3 A3 B2 A2 B1 A1 B0 A0 */
	    ((l0 & (1 << 3)) << 3) +
	    ((l0 & (1 << 2)) << 2) +
	    ((l0 & (1 << 1)) << 1) +
	    ((l0 & (1 << 0)) << 0) +
	    ((l1 & (1 << 3)) << 4) +
	    ((l1 & (1 << 2)) << 3) +
	    ((l1 & (1 << 1)) << 2) +
	    ((l1 & (1 << 0)) << 1);
	}
      line++;
      outbuf += 2;
    }
}

void
stp_fold_3bit(const unsigned char *line,
                int single_length,
                unsigned char *outbuf)
{
  int i;
  memset(outbuf, 0, single_length * 3);
  for (i = 0; i < single_length; i++)
    {
      unsigned char l0 = line[0];
      unsigned char l1 = line[single_length];
      unsigned char l2 = line[single_length * 2];
      if (l0 || l1 || l2)
	{
	  outbuf[0] =		/* C7 B7 A7 C6 B6 A6 C5 B5  */
	    ((l0 & (1 << 7)) >> 2) |
	    ((l0 & (1 << 6)) >> 4) |
	    ((l1 & (1 << 7)) >> 1) |
	    ((l1 & (1 << 6)) >> 3) |
	    ((l1 & (1 << 5)) >> 5) |
	    ((l2 & (1 << 7)) << 0) |
	    ((l2 & (1 << 6)) >> 2) |
	    ((l2 & (1 << 5)) >> 4);
	  outbuf[1] =		/* A5 C4 B4 A4 C3 B3 A3 C2 */
	    ((l0 & (1 << 5)) << 2) |
	    ((l0 & (1 << 4)) << 0) |
	    ((l0 & (1 << 3)) >> 2) |
	    ((l1 & (1 << 4)) << 1) |
	    ((l1 & (1 << 3)) >> 1) |
	    ((l2 & (1 << 4)) << 2) |
	    ((l2 & (1 << 3)) << 0) |
	    ((l2 & (1 << 2)) >> 2);
	  outbuf[2] =		/* B2 A2 C1 B1 A1 C0 B0 A0 */
	    ((l0 & (1 << 2)) << 4) |
	    ((l0 & (1 << 1)) << 2) |
	    ((l0 & (1 << 0)) << 0) |
	    ((l1 & (1 << 2)) << 5) |
	    ((l1 & (1 << 1)) << 3) |
	    ((l1 & (1 << 0)) << 1) |
	    ((l2 & (1 << 1)) << 4) |
	    ((l2 & (1 << 0)) << 2);
	}
    line++;
    outbuf += 3;
  }
}

void
stp_fold_3bit_323(const unsigned char *line,
		  int single_length,
		  unsigned char *outbuf)
{
  const unsigned char *last= line + single_length;
  memset(outbuf, 0, single_length * 3);
  for (; line < last; line += 3)
    {
      unsigned char A0 = line[0];
      unsigned char B0 = line[single_length];
      unsigned char C0 = line[2*single_length];
      unsigned char A1 = (line < last - 2) ? line[1] : 0;
      unsigned char B1 = (line < last - 2) ? line[single_length + 1] : 0;
      unsigned char C1 = (line < last - 2) ? line[(single_length * 2) + 1] : 0;
      unsigned char A2 = (line < last - 1) ? line[2] : 0;
      unsigned char B2 = (line < last - 1) ? line[single_length + 2] : 0;
      unsigned char C2 = (line < last - 1) ? line[(single_length * 2) + 2] : 0;

      if (A0 || A1 || A2 || B0 || B1 || B2 || C0 || C1 || C2)
	{
	  /* Missing: C0_6, C0_3, C0_0, C1_5, C1_2, C2_7, C2_4, C2_1 ??? */
	  outbuf[0] =		/* C0_7 B0_7 A0_7 B0_6 A0_6 C0_5 B0_5 A0_5 */
	    ((C0 & (1 << 7)) >> 0) |
	    ((B0 & (1 << 7)) >> 1) |
	    ((A0 & (1 << 7)) >> 2) |
	    ((B0 & (1 << 6)) >> 2) |
	    ((A0 & (1 << 6)) >> 3) |
	    ((C0 & (1 << 5)) >> 3) |
	    ((B0 & (1 << 5)) >> 4) |
	    ((A0 & (1 << 5)) >> 5);
	  outbuf[1] =		/* C0_4 B0_4 A0_4 B0_3 A0_3 C0_2 B0_2 A0_2 */
	    ((C0 & (1 << 4)) << 3) |
	    ((B0 & (1 << 4)) << 2) |
	    ((A0 & (1 << 4)) << 1) |
	    ((B0 & (1 << 3)) << 1) |
	    ((A0 & (1 << 3)) << 0) |
	    ((C0 & (1 << 2)) >> 0) |
	    ((B0 & (1 << 2)) >> 1) |
	    ((A0 & (1 << 2)) >> 2);
	  outbuf[2] =		/* C0_1 B0_1 A0_1 B0_0 A0_0 C1_7 B1_7 A1_7 */
	    ((C0 & (1 << 1)) << 6) |
	    ((B0 & (1 << 1)) << 5) |
	    ((A0 & (1 << 1)) << 4) |
	    ((B0 & (1 << 0)) << 4) |
	    ((A0 & (1 << 0)) << 3) |
	    ((C1 & (1 << 7)) >> 5) |
	    ((B1 & (1 << 7)) >> 6) |
	    ((A1 & (1 << 7)) >> 7);
	  outbuf[3] =		/* C1_6 B1_6 A1_6 B1_5 A1_5 C1_4 B1_4 A1_4 */
	    ((C1 & (1 << 6)) << 1) |
	    ((B1 & (1 << 6)) << 0) |
	    ((A1 & (1 << 6)) >> 1) |
	    ((B1 & (1 << 5)) >> 1) |
	    ((A1 & (1 << 5)) >> 2) |
	    ((C1 & (1 << 4)) >> 2) |
	    ((B1 & (1 << 4)) >> 3) |
	    ((A1 & (1 << 4)) >> 4);
	  outbuf[4] =		/* C1_3 B1_3 A1_3 B1_2 A1_2 C1_1 B1_1 A1_1 */
	    ((C1 & (1 << 3)) << 4) |
	    ((B1 & (1 << 3)) << 3) |
	    ((A1 & (1 << 3)) << 2) |
	    ((B1 & (1 << 2)) << 2) |
	    ((A1 & (1 << 2)) << 1) |
	    ((C1 & (1 << 1)) << 1) |
	    ((B1 & (1 << 1)) >> 0) |
	    ((A1 & (1 << 1)) >> 1);
	  outbuf[5] =		/* C1_0 B1_0 A1_0 B2_7 A2_7 C2_6 B2_6 A2_6 */
	    ((C1 & (1 << 0)) << 7) |
	    ((B1 & (1 << 0)) << 6) |
	    ((A1 & (1 << 0)) << 5) |
	    ((B2 & (1 << 7)) >> 3) |
	    ((A2 & (1 << 7)) >> 4) |
	    ((C2 & (1 << 6)) >> 4) |
	    ((B2 & (1 << 6)) >> 5) |
	    ((A2 & (1 << 6)) >> 6);
	  outbuf[6] =		/* C2_5 B2_5 A2_5 B2_4 A2_4 C2_3 B2_3 A2_3 */
	    ((C2 & (1 << 5)) << 2) |
	    ((B2 & (1 << 5)) << 1) |
	    ((A2 & (1 << 5)) << 0) |
	    ((B2 & (1 << 4)) >> 0) |
	    ((A2 & (1 << 4)) >> 1) |
	    ((C2 & (1 << 3)) >> 1) |
	    ((B2 & (1 << 3)) >> 2) |
	    ((A2 & (1 << 3)) >> 3);
	  outbuf[7] =		/* C2_2 B2_2 A2_2 B2_1 A2_1 C2_0 B2_0 A2_0 */
	    ((C2 & (1 << 2)) << 5) |
	    ((B2 & (1 << 2)) << 4) |
	    ((A2 & (1 << 2)) << 3) |
	    ((B2 & (1 << 1)) << 3) |
	    ((A2 & (1 << 1)) << 2) |
	    ((C2 & (1 << 0)) << 2) |
	    ((B2 & (1 << 0)) << 1) |
	    ((A2 & (1 << 0)) << 0);
	}
      outbuf += 8;
    }
}

void
stp_fold_4bit(const unsigned char *line,
                int single_length,
                unsigned char *outbuf)
{
  int i;
  memset(outbuf, 0, single_length * 4);
  for (i = 0; i < single_length; i++)
    {
      unsigned char l0 = line[0];
      unsigned char l1 = line[single_length];
      unsigned char l2 = line[single_length*2];
      unsigned char l3 = line[single_length*3];
      if (l0 || l1 || l2 || l3)
	{
	  outbuf[0] =		/* D7 C7 B7 A7 D6 C6 B6 A6 */
            ((l3 & (1 << 7)) >> 0)|
            ((l2 & (1 << 7)) >> 1)|
            ((l1 & (1 << 7)) >> 2)|
            ((l0 & (1 << 7)) >> 3)|
            ((l3 & (1 << 6)) >> 3)|
            ((l2 & (1 << 6)) >> 4)|
            ((l1 & (1 << 6)) >> 5)|
            ((l0 & (1 << 6)) >> 6);

	  outbuf[1] =		/* D5 C5 B5 A5 D4 C4 B4 A4 */
            ((l3 & (1 << 5)) << 2)|
            ((l2 & (1 << 5)) << 1)|
            ((l1 & (1 << 5)) << 0)|
            ((l0 & (1 << 5)) >> 1)|
            ((l3 & (1 << 4)) >> 1)|
            ((l2 & (1 << 4)) >> 2)|
            ((l1 & (1 << 4)) >> 3)|
            ((l0 & (1 << 4)) >> 4);

	  outbuf[2] =		/* D3 C3 B3 A3 D2 C2 B2 A2 */
            ((l3 & (1 << 3)) << 4)|
            ((l2 & (1 << 3)) << 3)|
            ((l1 & (1 << 3)) << 2)|
            ((l0 & (1 << 3)) << 1)|
            ((l3 & (1 << 2)) << 1)|
            ((l2 & (1 << 2)) << 0)|
            ((l1 & (1 << 2)) >> 1)|
            ((l0 & (1 << 2)) >> 2);

	  outbuf[3] =		/* D1 C1 B1 A1 D0 C0 B0 A0 */
            ((l3 & (1 << 1)) << 6)|
            ((l2 & (1 << 1)) << 5)|
            ((l1 & (1 << 1)) << 4)|
            ((l0 & (1 << 1)) << 3)|
            ((l3 & (1 << 0)) << 3)|
            ((l2 & (1 << 0)) << 2)|
            ((l1 & (1 << 0)) << 1)|
            ((l0 & (1 << 0)) << 0);
	}
      line++;
      outbuf += 4;
    }
}

#define SPLIT_MASK(k, b) (((1 << (b)) - 1) << ((k) * (b)))

#define SPLIT_STEP(k, b, i, o, in, r, inc, rl)	\
do						\
  {						\
    if (in & SPLIT_MASK(k, b))			\
      {						\
	o[r][i] |= SPLIT_MASK(k, b) & in;	\
	r += inc;				\
	if (r >= rl)				\
	  r = 0;				\
      }						\
  } while (0)

void
stp_split(int length,
	  int bits,
	  int n,
	  const unsigned char *in,
	  int increment,
	  unsigned char **outs)
{
  int row = 0;
  int limit = length * bits;
  int rlimit = n * increment;
  int i;
  for (i = 1; i < n; i++)
    memset(outs[i * increment], 0, limit);

  if (bits == 1)
    {
      for (i = 0; i < limit; i++)
	{
	  unsigned char inbyte = in[i];
	  outs[0][i] = 0;
	  if (inbyte == 0)
	    continue;
	  /* For some reason gcc isn't unrolling this, even with -funroll-loops */
	  SPLIT_STEP(0, 1, i, outs, inbyte, row, increment, rlimit);
	  SPLIT_STEP(1, 1, i, outs, inbyte, row, increment, rlimit);
	  SPLIT_STEP(2, 1, i, outs, inbyte, row, increment, rlimit);
	  SPLIT_STEP(3, 1, i, outs, inbyte, row, increment, rlimit);
	  SPLIT_STEP(4, 1, i, outs, inbyte, row, increment, rlimit);
	  SPLIT_STEP(5, 1, i, outs, inbyte, row, increment, rlimit);
	  SPLIT_STEP(6, 1, i, outs, inbyte, row, increment, rlimit);
	  SPLIT_STEP(7, 1, i, outs, inbyte, row, increment, rlimit);
	}
    }
  else
    {
      for (i = 0; i < limit; i++)
	{
	  unsigned char inbyte = in[i];
	  outs[0][i] = 0;
	  if (inbyte == 0)
	    continue;
	  /* For some reason gcc isn't unrolling this, even with -funroll-loops */
	  SPLIT_STEP(0, 2, i, outs, inbyte, row, increment, rlimit);
	  SPLIT_STEP(1, 2, i, outs, inbyte, row, increment, rlimit);
	  SPLIT_STEP(2, 2, i, outs, inbyte, row, increment, rlimit);
	  SPLIT_STEP(3, 2, i, outs, inbyte, row, increment, rlimit);
	}
    }
}

void
stp_split_2(int length,
	    int bits,
	    const unsigned char *in,
	    unsigned char *outhi,
	    unsigned char *outlo)
{
  unsigned char *outs[2];
  outs[0] = outhi;
  outs[1] = outlo;
  stp_split(length, bits, 2, in, 1, outs);
}

void
stp_split_4(int length,
	    int bits,
	    const unsigned char *in,
	    unsigned char *out0,
	    unsigned char *out1,
	    unsigned char *out2,
	    unsigned char *out3)
{
  unsigned char *outs[4];
  outs[0] = out0;
  outs[1] = out1;
  outs[2] = out2;
  outs[3] = out3;
  stp_split(length, bits, 4, in, 1, outs);
}


static void
stpi_unpack_2_1(int length,
		const unsigned char *in,
		unsigned char **outs)
{
  unsigned char	tempin, bit, temp0, temp1;

  if (length <= 0)
    return;
  for (bit = 128, temp0 = 0, temp1 = 0;
       length > 0;
       length --)
    {
      tempin = *in++;

      if (tempin & 128)
        temp0 |= bit;
      if (tempin & 64)
        temp1 |= bit;
      bit >>= 1;
      if (tempin & 32)
        temp0 |= bit;
      if (tempin & 16)
        temp1 |= bit;
      bit >>= 1;
      if (tempin & 8)
        temp0 |= bit;
      if (tempin & 4)
        temp1 |= bit;
      bit >>= 1;
      if (tempin & 2)
        temp0 |= bit;
      if (tempin & 1)
        temp1 |= bit;

      if (bit > 1)
        bit >>= 1;
      else
      {
        bit     = 128;
	*outs[0]++ = temp0;
	*outs[1]++ = temp1;

	temp0   = 0;
	temp1   = 0;
      }
    }

  if (bit < 128)
    {
      *outs[0]++ = temp0;
      *outs[1]++ = temp1;
    }
}

static void
stpi_unpack_2_2(int length,
		const unsigned char *in,
		unsigned char **outs)
{
  if (length <= 0)
    return;

  for (;length;length --)
    {
      unsigned char ti0, ti1;
      ti0 = in[0];
      ti1 = in[1];

      *outs[0]++  = (ti0 & 0xc0) << 0
	| (ti0 & 0x0c) << 2
	| (ti1 & 0xc0) >> 4
	| (ti1 & 0x0c) >> 2;
      *outs[1]++  = (ti0 & 0x30) << 2
	| (ti0 & 0x03) << 4
	| (ti1 & 0x30) >> 2
	| (ti1 & 0x03) >> 0;
      in += 2;
    }
}

static void
stpi_unpack_4_1(int length,
		 const unsigned char *in,
		 unsigned char **outs)
{
  unsigned char	tempin, bit, temp0, temp1, temp2, temp3;

  if (length <= 0)
    return;
  for (bit = 128, temp0 = 0, temp1 = 0, temp2 = 0, temp3 = 0;
       length > 0;
       length --)
    {
      tempin = *in++;

      if (tempin & 128)
        temp0 |= bit;
      if (tempin & 64)
        temp1 |= bit;
      if (tempin & 32)
        temp2 |= bit;
      if (tempin & 16)
        temp3 |= bit;
      bit >>= 1;
      if (tempin & 8)
        temp0 |= bit;
      if (tempin & 4)
        temp1 |= bit;
      if (tempin & 2)
        temp2 |= bit;
      if (tempin & 1)
        temp3 |= bit;

      if (bit > 1)
        bit >>= 1;
      else
      {
        bit     = 128;
	*outs[0]++ = temp0;
	*outs[1]++ = temp1;
	*outs[2]++ = temp2;
	*outs[3]++ = temp3;

	temp0   = 0;
	temp1   = 0;
	temp2   = 0;
	temp3   = 0;
      }
    }

  if (bit < 128)
    {
      *outs[0]++ = temp0;
      *outs[1]++ = temp1;
      *outs[2]++ = temp2;
      *outs[3]++ = temp3;
    }
}

static void
stpi_unpack_4_2(int length,
		 const unsigned char *in,
		 unsigned char **outs)
{
  unsigned char	tempin,
		shift,
		temp0,
		temp1,
		temp2,
		temp3;

  length *= 2;

  for (shift = 0, temp0 = 0, temp1 = 0, temp2 = 0, temp3 = 0;
       length > 0;
       length --)
    {
     /*
      * Note - we can't use (tempin & N) >> (shift - M) since negative
      * right-shifts are not always implemented.
      */

      tempin = *in++;

      if (tempin & 192)
        temp0 |= (tempin & 192) >> shift;
      if (tempin & 48)
        temp1 |= ((tempin & 48) << 2) >> shift;
      if (tempin & 12)
        temp2 |= ((tempin & 12) << 4) >> shift;
      if (tempin & 3)
        temp3 |= ((tempin & 3) << 6) >> shift;

      if (shift < 6)
        shift += 2;
      else
      {
        shift   = 0;
	*outs[0]++ = temp0;
	*outs[1]++ = temp1;
	*outs[2]++ = temp2;
	*outs[3]++ = temp3;

	temp0   = 0;
	temp1   = 0;
	temp2   = 0;
	temp3   = 0;
      }
    }

  if (shift)
    {
      *outs[0]++ = temp0;
      *outs[1]++ = temp1;
      *outs[2]++ = temp2;
      *outs[3]++ = temp3;
    }
}

static void
stpi_unpack_8_1(int length,
		const unsigned char *in,
		unsigned char **outs)
{
  unsigned char	tempin, bit, temp0, temp1, temp2, temp3, temp4, temp5, temp6,
    temp7;

  if (length <= 0)
    return;

  for (bit = 128, temp0 = 0, temp1 = 0, temp2 = 0,
       temp3 = 0, temp4 = 0, temp5 = 0, temp6 = 0, temp7 = 0;
       length > 0;
       length --)
    {
      tempin = *in++;

      if (tempin & 128)
        temp0 |= bit;
      if (tempin & 64)
        temp1 |= bit;
      if (tempin & 32)
        temp2 |= bit;
      if (tempin & 16)
        temp3 |= bit;
      if (tempin & 8)
        temp4 |= bit;
      if (tempin & 4)
        temp5 |= bit;
      if (tempin & 2)
        temp6 |= bit;
      if (tempin & 1)
        temp7 |= bit;

      if (bit > 1)
        bit >>= 1;
      else
      {
        bit     = 128;
	*outs[0]++ = temp0;
	*outs[1]++ = temp1;
	*outs[2]++ = temp2;
	*outs[3]++ = temp3;
	*outs[4]++ = temp4;
	*outs[5]++ = temp5;
	*outs[6]++ = temp6;
	*outs[7]++ = temp7;

	temp0   = 0;
	temp1   = 0;
	temp2   = 0;
	temp3   = 0;
	temp4   = 0;
	temp5   = 0;
	temp6   = 0;
	temp7   = 0;
      }
    }

  if (bit < 128)
    {
      *outs[0]++ = temp0;
      *outs[1]++ = temp1;
      *outs[2]++ = temp2;
      *outs[3]++ = temp3;
      *outs[4]++ = temp4;
      *outs[5]++ = temp5;
      *outs[6]++ = temp6;
      *outs[7]++ = temp7;
    }
}

static void
stpi_unpack_8_2(int length,
		const unsigned char *in,
		unsigned char **outs)
{
  unsigned char	tempin,
		shift,
		temp0,
		temp1,
		temp2,
		temp3,
		temp4,
		temp5,
		temp6,
		temp7;


  for (shift = 0, temp0 = 0, temp1 = 0,
       temp2 = 0, temp3 = 0, temp4 = 0, temp5 = 0, temp6 = 0, temp7 = 0;
       length > 0;
       length --)
    {
     /*
      * Note - we can't use (tempin & N) >> (shift - M) since negative
      * right-shifts are not always implemented.
      */

      tempin = *in++;

      if (tempin & 192)
        temp0 |= (tempin & 192) >> shift;
      if (tempin & 48)
        temp1 |= ((tempin & 48) << 2) >> shift;
      if (tempin & 12)
        temp2 |= ((tempin & 12) << 4) >> shift;
      if (tempin & 3)
        temp3 |= ((tempin & 3) << 6) >> shift;

      tempin = *in++;

      if (tempin & 192)
        temp4 |= (tempin & 192) >> shift;
      if (tempin & 48)
        temp5 |= ((tempin & 48) << 2) >> shift;
      if (tempin & 12)
        temp6 |= ((tempin & 12) << 4) >> shift;
      if (tempin & 3)
        temp7 |= ((tempin & 3) << 6) >> shift;

      if (shift < 6)
        shift += 2;
      else
      {
        shift   = 0;
	*outs[0]++ = temp0;
	*outs[1]++ = temp1;
	*outs[2]++ = temp2;
	*outs[3]++ = temp3;
	*outs[4]++ = temp4;
	*outs[5]++ = temp5;
	*outs[6]++ = temp6;
	*outs[7]++ = temp7;

	temp0   = 0;
	temp1   = 0;
	temp2   = 0;
	temp3   = 0;
	temp4   = 0;
	temp5   = 0;
	temp6   = 0;
	temp7   = 0;
      }
    }

  if (shift)
    {
      *outs[0]++ = temp0;
      *outs[1]++ = temp1;
      *outs[2]++ = temp2;
      *outs[3]++ = temp3;
      *outs[4]++ = temp4;
      *outs[5]++ = temp5;
      *outs[6]++ = temp6;
      *outs[7]++ = temp7;
    }
}

static void
stpi_unpack_16_1(int length,
		 const unsigned char *in,
		 unsigned char **outs)
{
  unsigned char	tempin, bit;
  unsigned char temp[16];
  int j;

  if (length <= 0)
    return;

  memset(temp, 0, 16);

  for (bit = 128; length > 0; length--)
    {
      tempin = *in++;

      if (tempin & 128)
        temp[0] |= bit;
      if (tempin & 64)
        temp[1] |= bit;
      if (tempin & 32)
        temp[2] |= bit;
      if (tempin & 16)
        temp[3] |= bit;
      if (tempin & 8)
        temp[4] |= bit;
      if (tempin & 4)
        temp[5] |= bit;
      if (tempin & 2)
        temp[6] |= bit;
      if (tempin & 1)
        temp[7] |= bit;

      tempin = *in++;

      if (tempin & 128)
	temp[8] |= bit;
      if (tempin & 64)
	temp[9] |= bit;
      if (tempin & 32)
	temp[10] |= bit;
      if (tempin & 16)
	temp[11] |= bit;
      if (tempin & 8)
	temp[12] |= bit;
      if (tempin & 4)
	temp[13] |= bit;
      if (tempin & 2)
	temp[14] |= bit;
      if (tempin & 1)
	temp[15] |= bit;

      if (bit > 1)
        bit >>= 1;
      else
	{
	  bit     = 128;
	  for (j = 0; j < 16; j++)
	    *outs[j]++ = temp[j];

	  memset(temp, 0, 16);
	}
    }

  if (bit < 128)
    for (j = 0; j < 16; j++)
      *outs[j]++ = temp[j];
}

static void
stpi_unpack_16_2(int length,
		 const unsigned char *in,
		 unsigned char **outs)
{
  unsigned char	tempin, shift;
  unsigned char temp[16];
  int j;

  if (length <= 0)
    return;

  memset(temp, 0, 16);

  for (shift = 0; length > 0; length--)
    {
      /*
       * Note - we can't use (tempin & N) >> (shift - M) since negative
       * right-shifts are not always implemented.
       */

      tempin = *in++;

      if (tempin & 192)
        temp[0] |= (tempin & 192) >> shift;
      if (tempin & 48)
        temp[1] |= ((tempin & 48) << 2) >> shift;
      if (tempin & 12)
        temp[2] |= ((tempin & 12) << 4) >> shift;
      if (tempin & 3)
        temp[3] |= ((tempin & 3) << 6) >> shift;

      tempin = *in++;

      if (tempin & 192)
        temp[4] |= (tempin & 192) >> shift;
      if (tempin & 48)
        temp[5] |= ((tempin & 48) << 2) >> shift;
      if (tempin & 12)
        temp[6] |= ((tempin & 12) << 4) >> shift;
      if (tempin & 3)
        temp[7] |= ((tempin & 3) << 6) >> shift;

      if (length-- > 0)
	{
	  tempin = *in++;

	  if (tempin & 192)
	    temp[8] |= (tempin & 192) >> shift;
	  if (tempin & 48)
	    temp[9] |= ((tempin & 48) << 2) >> shift;
	  if (tempin & 12)
	    temp[10] |= ((tempin & 12) << 4) >> shift;
	  if (tempin & 3)
	    temp[11] |= ((tempin & 3) << 6) >> shift;

	  tempin = *in++;

	  if (tempin & 192)
	    temp[12] |= (tempin & 192) >> shift;
	  if (tempin & 48)
	    temp[13] |= ((tempin & 48) << 2) >> shift;
	  if (tempin & 12)
	    temp[14] |= ((tempin & 12) << 4) >> shift;
	  if (tempin & 3)
	    temp[15] |= ((tempin & 3) << 6) >> shift;
	}

      if (shift < 6)
        shift += 2;
      else
	{
	  shift   = 0;
	  for (j = 0; j < 16; j++)
	    *outs[j]++ = temp[j];

	  memset(temp, 0, 16);
	}
    }

  if (shift)
    for (j = 0; j < 16; j++)
      *outs[j]++ = temp[j];
}

void
stp_unpack(int length,
	   int bits,
	   int n,
	   const unsigned char *in,
	   unsigned char **outs)
{
  unsigned char **touts;
  int i;
  if (n < 2)
    return;
  touts = stp_malloc(sizeof(unsigned char *) * n);
  for (i = 0; i < n; i++)
    touts[i] = outs[i];
  if (bits == 1)
    switch (n)
      {
      case 2:
	stpi_unpack_2_1(length, in, touts);
	break;
      case 4:
	stpi_unpack_4_1(length, in, touts);
	break;
      case 8:
	stpi_unpack_8_1(length, in, touts);
	break;
      case 16:
	stpi_unpack_16_1(length, in, touts);
	break;
      }
  else
    switch (n)
      {
      case 2:
	stpi_unpack_2_2(length, in, touts);
	break;
      case 4:
	stpi_unpack_4_2(length, in, touts);
	break;
      case 8:
	stpi_unpack_8_2(length, in, touts);
	break;
      case 16:
	stpi_unpack_16_2(length, in, touts);
	break;
      }
  stp_free(touts);
}

void
stp_unpack_2(int length,
	     int bits,
	     const unsigned char *in,
	     unsigned char *outhi,
	     unsigned char *outlo)
{
  unsigned char *outs[2];
  outs[0] = outhi;
  outs[1] = outlo;
  stp_unpack(length, bits, 2, in, outs);
}

void
stp_unpack_4(int length,
	     int bits,
	     const unsigned char *in,
	     unsigned char *out0,
	     unsigned char *out1,
	     unsigned char *out2,
	     unsigned char *out3)
{
  unsigned char *outs[4];
  outs[0] = out0;
  outs[1] = out1;
  outs[2] = out2;
  outs[3] = out3;
  stp_unpack(length, bits, 4, in, outs);
}

void
stp_unpack_8(int length,
	     int bits,
	     const unsigned char *in,
	     unsigned char *out0,
	     unsigned char *out1,
	     unsigned char *out2,
	     unsigned char *out3,
	     unsigned char *out4,
	     unsigned char *out5,
	     unsigned char *out6,
	     unsigned char *out7)
{
  unsigned char *outs[8];
  outs[0] = out0;
  outs[1] = out1;
  outs[2] = out2;
  outs[3] = out3;
  outs[4] = out4;
  outs[5] = out5;
  outs[6] = out6;
  outs[7] = out7;
  stp_unpack(length, bits, 8, in, outs);
}

void
stp_unpack_16(int length,
	      int bits,
	      const unsigned char *in,
	      unsigned char *out0,
	      unsigned char *out1,
	      unsigned char *out2,
	      unsigned char *out3,
	      unsigned char *out4,
	      unsigned char *out5,
	      unsigned char *out6,
	      unsigned char *out7,
	      unsigned char *out8,
	      unsigned char *out9,
	      unsigned char *out10,
	      unsigned char *out11,
	      unsigned char *out12,
	      unsigned char *out13,
	      unsigned char *out14,
	      unsigned char *out15)
{
  unsigned char *outs[16];
  outs[0] = out0;
  outs[1] = out1;
  outs[2] = out2;
  outs[3] = out3;
  outs[4] = out4;
  outs[5] = out5;
  outs[6] = out6;
  outs[7] = out7;
  outs[8] = out8;
  outs[9] = out9;
  outs[10] = out10;
  outs[11] = out11;
  outs[12] = out12;
  outs[13] = out13;
  outs[14] = out14;
  outs[15] = out15;
  stp_unpack(length, bits, 16, in, outs);
}

static void
find_first_and_last(const unsigned char *line, int length,
		    int *first, int *last)
{
  int i;
  int found_first = 0;
  if (!first || !last)
    return;
  *first = 0;
  *last = 0;
  for (i = 0; i < length; i++)
    {
      if (line[i] == 0)
	{
	  if (!found_first)
	    (*first)++;
	}
      else
	{
	  *last = i;
	  found_first = 1;
	}
    }
}

int
stp_pack_uncompressed(stp_vars_t *v,
		      const unsigned char *line,
		      int length,
		      unsigned char *comp_buf,
		      unsigned char **comp_ptr,
		      int *first,
		      int *last)
{
  find_first_and_last(line, length, first, last);
  memcpy(comp_buf, line, length);
  *comp_ptr = comp_buf + length;
  if (first && last && *first > *last)
    return 0;
  else
    return 1;
}

int
stp_pack_tiff(stp_vars_t *v,
	      const unsigned char *line,
	      int length,
	      unsigned char *comp_buf,
	      unsigned char **comp_ptr,
	      int *first,
	      int *last)
{
  const unsigned char *start;		/* Start of compressed data */
  unsigned char repeat;			/* Repeating char */
  int count;			/* Count of compressed bytes */
  int tcount;			/* Temporary count < 128 */
  register const unsigned char *xline = line;
  register int xlength = length;
  find_first_and_last(line, length, first, last);

  /*
   * Compress using TIFF "packbits" run-length encoding...
   */

  (*comp_ptr) = comp_buf;

  while (xlength > 0)
    {
      /*
       * Get a run of non-repeated chars...
       */

      start  = xline;
      xline   += 2;
      xlength -= 2;

      while (xlength > 0 && (xline[-2] != xline[-1] || xline[-1] != xline[0]))
	{
	  xline ++;
	  xlength --;
	}

      xline   -= 2;
      xlength += 2;

      /*
       * Output the non-repeated sequences (max 128 at a time).
       */

      count = xline - start;
      while (count > 0)
	{
	  tcount = count > 128 ? 128 : count;

	  (*comp_ptr)[0] = tcount - 1;
	  memcpy((*comp_ptr) + 1, start, tcount);

	  (*comp_ptr) += tcount + 1;
	  start    += tcount;
	  count    -= tcount;
	}

      if (xlength <= 0)
	break;

      /*
       * Find the repeated sequences...
       */

      start  = xline;
      repeat = xline[0];

      xline ++;
      xlength --;

      if (xlength > 0)
	{
	  int ylength = xlength;
	  while (ylength && *xline == repeat)
	    {
	      xline ++;
	      ylength --;
	    }
	  xlength = ylength;
	}

      /*
       * Output the repeated sequences (max 128 at a time).
       */

      count = xline - start;
      while (count > 0)
	{
	  tcount = count > 128 ? 128 : count;

	  (*comp_ptr)[0] = 1 - tcount;
	  (*comp_ptr)[1] = repeat;

	  (*comp_ptr) += 2;
	  count    -= tcount;
	}
    }
  if (first && last && *first > *last)
    return 0;
  else
    return 1;
}
