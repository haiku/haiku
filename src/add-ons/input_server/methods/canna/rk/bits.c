/* Copyright 1994 NEC Corporation, Tokyo, Japan.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of NEC
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  NEC Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * NEC CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN 
 * NO EVENT SHALL NEC CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
 * OTHER TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE. 
 */

/************************************************************************/
/* THIS SOURCE CODE IS MODIFIED FOR TKO BY T.MURAI 1997
/************************************************************************/


#if !defined(lint) && !defined(__CODECENTER__)
static char rcsid[]="$Id: bits.c 10525 2004-12-23 21:23:50Z korli $";
#endif
/* LINTLIBRARY */

#include "RKintern.h"
// There is Exported Symbols !!
/************************************************************
 * PackBits
************************************************************/
/*
  ³Ø½¬¥Õ¥¡¥¤¥ëÍÑ¥Ó¥Ã¥ÈÁàºî½èÍý

  ³Ø½¬¥ª¥Õ¥»¥Ã¥È¤Î¾ðÊó¤ò¡¢¤Ç¤­¤ë¤À¤±¾®¤µ¤¤¥Ó¥Ã¥È¤ÎÇÛÎó¤È¤·¤ÆÊÝ»ý¤¹¤ë¡£

    ¸õÊä¿ô     1 2 3 4 5 6 7 8 9 ...    n
    ¥Ó¥Ã¥ÈÉý   2 3 3 4 4 4 4 5 5     log(n) + 1

  RkPackBits ¤Ï unsigned ¤ÎÇÛÎó¤Î¿ô¤ò dst_bits ¤Î dst_offset(¥Ó¥Ã¥È¤Ç
  ¥«¥¦¥ó¥È)¤ÎÀè¤«¤é³ÊÇ¼¤¹¤ë¡£Ã¼¿ô¤¬½Ð¤¿¾ì¹ç¤Ï²¼°Ì¥Ó¥Ã¥È¤«¤é»È¤ï¤ì¤ë¡£

  °ú¿ô
    dst_bits   -- ¥Ó¥Ã¥ÈÇÛÎó¤Ø¤Î¥Ý¥¤¥ó¥¿
    dst_offset -- ¼ÂºÝ¤Ë¥¢¥¯¥»¥¹¤¹¤ë¤È¤³¤í¤Þ¤Ç¤Î¥ª¥Õ¥»¥Ã¥È(¥Ó¥Ã¥È¤Ç¥«¥¦¥ó¥È)
    bit_size   -- ¥Ó¥Ã¥ÈÇÛÎó¤Î£±¤Ä¤ÎÍ×ÁÇ¤Î¥Ó¥Ã¥ÈÉý
    src_ints   -- ³ÊÇ¼¤·¤¿¤¤¿ôÃÍ¤ÎÇÛÎó
    count      -- ³ÊÇ¼¤·¤¿¤¤¿ô

  Ìá¤êÃÍ

 */

long
_RkPackBits(unsigned char *dst_bits, long dst_offset, int bit_size, unsigned *src_ints, int count)
{
  unsigned char	*dstB;
  unsigned		dstQ;
  unsigned		dstCount;
  unsigned		bitMask;
  
  dstB = dst_bits + dst_offset / BIT_UNIT;
  dstCount = (dst_offset % BIT_UNIT);

  /* ÅÓÃæ¤Ê¤Î¤Ç¡¢¼ê¤òÉÕ¤±¤Ê¤¤ÉôÊ¬¤¬¤¢¤ë¤³¤È¤ËÃí°Õ */
  dstQ  = *dstB & ((1 << dstCount) - 1);
  bitMask = (1 << bit_size) - 1;
  while (count-- > 0) {
    dstQ |= (*src_ints++ & bitMask) << dstCount;
    dstCount += bit_size;
    dst_offset += bit_size;
    while (dstCount >= BIT_UNIT) {
      *dstB++ = dstQ & ((1 << BIT_UNIT) - 1);
      dstQ >>= BIT_UNIT;
      dstCount -= BIT_UNIT;
    }
  }
  if (dstCount) {
    *dstB = (*dstB & ~((1 << dstCount) - 1)) | (dstQ & ((1 << dstCount) - 1));
  }
  return dst_offset;
}

/*
  UnpackBits

  RkUnpackBits ¤Ï dst_bits ¤Î dst_offset(¥Ó¥Ã¥È¤Ç¥«¥¦¥ó¥È)¤Ë³ÊÇ¼¤µ¤ì¤Æ
  ¤¤¤ë¥Ó¥Ã¥È¤ÎÇÛÎó¤ò unsigned ¤ÎÇÛÎó¤Ë¼è¤ê½Ð¤¹¡£offset ¤ËÃ¼¿ô¤¬½Ð¤¿¾ì
  ¹ç¤Ï²¼°Ì¥Ó¥Ã¥È¤«¤é»È¤ï¤ì¤ë¡£

  °ú¿ô
    dst_ints   -- ¼è¤ê½Ð¤·¤¿¿ôÃÍ¤ò³ÊÇ¼¤¹¤ëÇÛÎó¤Ø¤Î¥Ý¥¤¥ó¥¿
    src_bits   -- ¥Ó¥Ã¥ÈÇÛÎó¤Ø¤Î¥Ý¥¤¥ó¥¿
    src_offset -- ¼ÂºÝ¤Ë³ÊÇ¼¤¹¤ë¤È¤³¤í¤Þ¤Ç¤Î¥ª¥Õ¥»¥Ã¥È(¥Ó¥Ã¥È¤Ç¥«¥¦¥ó¥È)
    bit_size   -- ¥Ó¥Ã¥ÈÇÛÎó¤Î£±¤Ä¤ÎÍ×ÁÇ¤Î¥Ó¥Ã¥ÈÉý
    count      -- ¼è¤ê½Ð¤·¤¿¤¤¿ô

  Ìá¤êÃÍ

 */

long
_RkUnpackBits(unsigned *dst_ints, unsigned char *src_bits, long src_offset, int bit_size, int count)
{
  unsigned char	*srcB;
  unsigned		srcQ;
  unsigned		srcCount;
  unsigned		bitMask;
  
  srcB = src_bits + src_offset / BIT_UNIT;
  srcCount = BIT_UNIT - (src_offset % BIT_UNIT);
  srcQ  = *srcB++ >> (src_offset % BIT_UNIT);
  bitMask = (1 << bit_size) - 1;
  while (count-- > 0) {
    while (srcCount < (unsigned)bit_size) {
      srcQ |= (*srcB++ << srcCount);
      srcCount += BIT_UNIT;
    }
    *dst_ints++ = srcQ & bitMask;
    srcQ >>= bit_size;
    srcCount -= bit_size;
    src_offset += bit_size;
  }
  return src_offset;
}

/*
  CopyBits

  RkCopyBits ¤Ï src_bits ¤Î src_offset ¤Ë³ÊÇ¼¤µ¤ì¤Æ¤¤¤ë¥Ó¥Ã¥ÈÇÛÎó¤ò 
  count ¸Ä¤À¤± dst_bits ¤Î dst_offset¤Ë°ÜÆ°¤µ¤»¤ë¡£

  °ú¿ô
    dst_bits   -- °ÜÆ°Àè¥Ó¥Ã¥ÈÇÛÎó¤Ø¤Î¥Ý¥¤¥ó¥¿
    dst_offset -- ¼ÂºÝ¤Ë³ÊÇ¼¤¹¤ë¤È¤³¤í¤Þ¤Ç¤Î¥ª¥Õ¥»¥Ã¥È(¥Ó¥Ã¥È¤Ç¥«¥¦¥ó¥È)
    bit_size   -- ¥Ó¥Ã¥ÈÇÛÎó¤Î£±¤Ä¤ÎÍ×ÁÇ¤Î¥Ó¥Ã¥ÈÉý
    src_bits   -- °ÜÆ°¸µ¥Ó¥Ã¥ÈÇÛÎó¤Ø¤Î¥Ý¥¤¥ó¥¿
    src_offset -- ¼è¤ê½Ð¤¹¤È¤³¤í¤Þ¤Ç¤Î¥ª¥Õ¥»¥Ã¥È(¥Ó¥Ã¥È¤Ç¥«¥¦¥ó¥È)
    count      -- °ÜÆ°¤·¤¿¤¤¿ô

  Ìá¤êÃÍ

 */

long
_RkCopyBits(unsigned char *dst_bits, long dst_offset, int bit_size, unsigned char *src_bits, long src_offset, int count)
{
  unsigned char	*dstB;
  unsigned		dstQ;
  unsigned		dstCount;
  unsigned char	*srcB;
  unsigned		srcQ;
  unsigned		srcCount;
  unsigned		bitMask;
  unsigned		bits;
  
  dstB = dst_bits + dst_offset / BIT_UNIT;
  dstCount = (dst_offset % BIT_UNIT);
  dstQ  = *dstB & ((1 << dstCount) - 1);
  srcB = src_bits + src_offset / BIT_UNIT;
  srcCount = BIT_UNIT - (src_offset % BIT_UNIT);
  srcQ  = *srcB++ >> (src_offset % BIT_UNIT);
  bitMask = (1 << bit_size) - 1;
  while (count-- > 0) {
    /* unpack */
    while (srcCount < (unsigned)bit_size) {
      srcQ |= (*srcB++ << srcCount);
      srcCount += BIT_UNIT;
    }
    bits = srcQ & bitMask;
    srcQ >>= bit_size;
    srcCount -= bit_size;
    src_offset += bit_size;
    /* pack */
    dstQ |= bits << dstCount;
    dstCount += bit_size;
    dst_offset += bit_size;
    while (dstCount >= BIT_UNIT) {
      *dstB++ = dstQ & ((1 << BIT_UNIT) - 1);
      dstQ >>= BIT_UNIT;
      dstCount -= BIT_UNIT;
    }
  }
  if (dstCount) {
    *dstB = (*dstB & ~((1 << dstCount) - 1)) | (dstQ & ((1 << dstCount) - 1));
  }
  return dst_offset;
}

/*
  _RkSetBitNum

  _RkSetBitNum ¤Ï bit ÇÛÎó¤Î offset °ÌÃÖ¤«¤é n ÈÖÌÜ¤ÎÃÍ¤È¤·¤Æ val ¤ò³Ê
  Ç¼¤¹¤ë´Ø¿ô¤Ç¤¢¤ë¡£

  °ú¿ô
    dst_bits   -- ¥Ó¥Ã¥ÈÇÛÎó¤Ø¤Î¥Ý¥¤¥ó¥¿
    dst_offset -- ¼ÂºÝ¤Ë³ÊÇ¼¤¹¤ë¤È¤³¤í¤Þ¤Ç¤Î¥ª¥Õ¥»¥Ã¥È(¥Ó¥Ã¥È¤Ç¥«¥¦¥ó¥È)
    bit_size   -- ¥Ó¥Ã¥ÈÇÛÎó¤Î£±¤Ä¤ÎÍ×ÁÇ¤Î¥Ó¥Ã¥ÈÉý
    n          -- ÀèÆ¬¤«¤é²¿ÈÖÌÜ¤ÎÍ×ÁÇ¤«¤òÍ¿¤¨¤ë¡£
    val        -- ³ÊÇ¼¤¹¤ëÃÍ¤òÍ¿¤¨¤ë¡£

  Ìá¤êÃÍ

 */

int
_RkSetBitNum(unsigned char *dst_bits, unsigned long dst_offset, int bit_size, int n, int val)
{
  unsigned char	*dstB;
  unsigned dstQ, dstCount, bitMask;

  dst_offset += bit_size * n;

  dstB = dst_bits + dst_offset / BIT_UNIT;
  dstCount = (dst_offset % BIT_UNIT);

  /* ÅÓÃæ¤Ê¤Î¤Ç¡¢¼ê¤òÉÕ¤±¤Ê¤¤ÉôÊ¬¤¬¤¢¤ë¤³¤È¤ËÃí°Õ */
  dstQ  = *dstB & ((1 << dstCount) - 1);
  bitMask = (1 << bit_size) - 1;

  dstQ |= (val & bitMask) << dstCount;
  dstCount += bit_size;
  dst_offset += bit_size;
  while (dstCount >= BIT_UNIT) {
    *dstB++ = dstQ & ((1 << BIT_UNIT) - 1);
    dstQ >>= BIT_UNIT;
    dstCount -= BIT_UNIT;
  }
  if (dstCount) {
    *dstB = (*dstB & ~((1 << dstCount) - 1)) | (dstQ & ((1 << dstCount) - 1));
  }
  return dst_offset;
}

int
_RkCalcFqSize(int n)
{
  return n*(_RkCalcLog2(n) + 1);
}

#ifdef __BITS_DEBUG__
#include <stdio.h>
_RkPrintPackedBits(unsigned char *bits, int offset, int bit_size, int count)
{
    fprintf(stderr, "%d <", count);
    while ( count-- > 0 ) {
	unsigned w;

        offset = _RkUnpackBits(&w, bits, offset, bit_size, 1);
        fprintf(stderr, " %d", w/2);
    };
    fprintf(stderr, ">\n");
}

int 
_RkCalcLog2(int n)
{
  int	lg2;
  
  n--;
  for (lg2 = 0; n > 0; lg2++)
    n >>= 1;
  return(lg2);
}

main(void)
{
  int		 offset;
  int		 bit_size;
  int		 size;
  unsigned char bits[1024*8];
  unsigned char Bits[1024*8];
  int	c, i;
  int		ec;
  int	o;
  
  /* create test run */
  for ( size = 1; size <= 32; size++ ) {
    bit_size = _RkCalcLog2(size) + 1;
    printf("#%4d/%2d\t", size, bit_size);
    /* pack 'em all */
    o = 0;
    for ( i = 0; i < size; i++ )
      o = _RkPackBits(Bits, o, bit_size, &i, 1);
    printf("PK ");
    for ( i = 0; i < (bit_size*size+7)/8; i++ )
      printf(" %02x", Bits[i]);
    printf("\n");
    
    
    for ( offset = 0; offset < 16; offset++ ) {
      /* copybits */
      o = _RkCopyBits(bits, offset, bit_size, Bits, 0, size);
      printf("%d ", offset);
      for ( i = 0; i < (o + 7)/8; i++ )
	printf(" %02x", bits[i]);
      printf("\n");
      
      /* unpack 'em again */
      ec = 0;
      o = offset;
      for ( i = 0; i < size; i++ ) {
	unsigned w;
	  
	o = _RkUnpackBits(&w, bits, o, bit_size, 1);
	if ( w != i )
	  ec++;
      };
      if ( ec )
	printf(" %d", offset);
      else
	printf(".");
    };
    printf("\n");
  };
}
#endif
