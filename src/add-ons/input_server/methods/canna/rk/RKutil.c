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
static char rcsid[]="@(#)$Id: RKutil.c 10525 2004-12-23 21:23:50Z korli $ $Author: korli $ $Revision: 1.1 $ $Data$";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "RKintern.h"
// There is Exported Symbols !!

static char	*Hdrtag[] = {
  HD_TAG_MAG,
  HD_TAG_VER,
  HD_TAG_TIME,
  HD_TAG_REC,
  HD_TAG_CAN,
  HD_TAG_L2P,
  HD_TAG_L2C,
  HD_TAG_PAG,
  HD_TAG_LND,
  HD_TAG_SND,
  HD_TAG_SIZ,
  HD_TAG_HSZ,
  HD_TAG_DROF,
  HD_TAG_PGOF,
  HD_TAG_DMNM,
  HD_TAG_CODM,
  HD_TAG_LANG,
  HD_TAG_WWID,
  HD_TAG_WTYP,
  HD_TAG_COPY,
  HD_TAG_NOTE,
  HD_TAG_TYPE,
  0,
};

static char	essential_tag[] = {
    HD_TIME,
    HD_DMNM,
    HD_LANG,
    HD_WWID,
    HD_WTYP,
    HD_TYPE,
};

static	unsigned char	localbuffer[RK_MAX_HDRSIZ];

int
uslen(WCHAR_T *us)
{
  WCHAR_T *ous = us;
  
  if (!us)
    return 0;
  while (*us & RK_WMASK)
    us++;
  return (us - ous);
}

void
usncopy(WCHAR_T *dst, WCHAR_T *src, int len)
{
  while (len-- > 0 && (*dst++ = *src++)) /* EMPTY */;
}

unsigned char *
ustoeuc(WCHAR_T *src, int srclen, unsigned char *dest, int destlen)
{
    if (!src || !dest || !srclen || !destlen)
	return dest;
    while (*src && --srclen >= 0 && --destlen >= 0) {
	if (us_iscodeG0(*src)) {
	    *dest++ = (unsigned char)*src++;
	} else if (us_iscodeG2(*src)) {
	    *dest++ = RK_SS2;
	    *dest++ = (unsigned char)*src++;
	    destlen--;
	} else if (destlen > 2) {
	  if (us_iscodeG3(*src)) {
	    *dest++ = RK_SS3;
	  }
	  *dest++ = (unsigned char)(*src >> 8);
	  *dest++ = (unsigned char)(*src++ | 0x80);
	  destlen--;
	};
    };
    *dest = (unsigned char)0;
    return dest;
}

WCHAR_T *
euctous(unsigned char *src, int srclen, WCHAR_T *dest, int destlen)
{
  WCHAR_T	*a = dest;
    
  if (!src || !dest || !srclen || !destlen)
    return(a);
  while (*src && (srclen-- > 0) && (destlen-- > 0)) {
    if (!(*src & 0x80) ) {
      *dest++ = (WCHAR_T)*src++;
    } else if (srclen-- > 0) {
      if (*src == RK_SS2) {
	src++;
	*dest++ = (WCHAR_T)(0x0080 | (*src++ & 0x7f));
      } else if ((*src == RK_SS3) && (srclen-- > 0)) {
	src++;
	*dest++ = (WCHAR_T)(0x8000 | ((src[0] & 0x7f) << 8) | (src[1] & (0x7f)));
	src += 2;
      } else {
	*dest++ = (WCHAR_T)(0x8080 | ((src[0] & 0x7f) << 8) | (src[1] & 0x7f));
	src += 2;
      }
    } else {
      break;
    }
  }
  if (destlen-- > 0)
    *dest = (WCHAR_T)0;
  return dest;
}

#ifndef WIN
static FILE	*logfile = (FILE *)0;
#endif

void
_Rkpanic(char *fmt, int p, int q, int r)
/* VARARGS2 */
{
#ifndef WIN
  char	msg[RK_LINE_BMAX];
  
  (void)sprintf(msg, fmt, p, q, r);
  (void)fprintf(logfile ? logfile : stderr, "%s\n", msg);
  (void)fflush(logfile);
#endif
  /* The following exit() must be removed.  1996.6.5 kon */
  exit(1);
}

int
_RkCalcUnlog2(int x)
{
  return((1 << x) - 1);
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

WCHAR_T
uniqAlnum(WCHAR_T c)
{
  return((0xa3a0 < c && c < 0xa3ff) ? (WCHAR_T)(c & 0x7f) : c);
}

void
_RkClearHeader(struct HD *hd)
{
	if (hd) {
		for (int i = 0; i < HD_MAXTAG; i++) {
			if (hd->flag[i] > 0) {
				free(hd->data[i].ptr);
			}
		}
	}
}

int
_RkReadHeader(int fd, struct HD *hd, off_t off_from_top)
{
  unsigned char	*src;
  unsigned long	len, off;
  int		i, tmpres;
  long hdrsize;
#ifdef WIN
  DWORD readsize;
#endif

  for (i = 0; i < HD_MAXTAG; i++) {
    hd->data[i].var = 0;
    hd->flag[i] = 0;
  }
  /* ¼¡¤Î off_from_top ¤Î·×»»¤¬¤¦¤µ¤ó¤¯¤µ¤¤¤¾ */
  tmpres = lseek(fd, off_from_top, 0);
  if (tmpres < 0) {
    RkSetErrno(RK_ERRNO_EACCES);
    goto read_err;
  }

  hdrsize = read(fd, (char *)localbuffer, RK_MAX_HDRSIZ);
  if (hdrsize <= ((long) 0)) {
    RkSetErrno(RK_ERRNO_EACCES);
    goto read_err;
  }
  for (src = localbuffer; src < (localbuffer + hdrsize);) {
    if (isEndTag(src))
      break;
    for (i = 0; i < HD_MAXTAG; i++) {
      if (!strncmp((char *)src, Hdrtag[i],  HD_TAGSIZ))
	break;
    }
    if (i == HD_MAXTAG)
      goto read_err;
    src += HD_TAGSIZ;
    len = bst4_to_l(src);
    src += HD_TAGSIZ;
    off = bst4_to_l(src);
    src += HD_TAGSIZ;
    if (hd->flag[i] != 0)
      goto read_err;
    if (len == 0) {
      hd->flag[i] = -1;
      hd->data[i].var = off;
    } else {
      hd->flag[i] = len;
      if (!(hd->data[i].ptr = (unsigned char *)malloc((size_t) (len + 1)))) {
	RkSetErrno(RK_ERRNO_NOMEM);
	goto read_err;
      }
      if (off < (unsigned long)hdrsize) {
	memcpy(hd->data[i].ptr, localbuffer + off, (size_t) len);
      } else {
	tmpres = lseek(fd, off_from_top + off, 0);
	if (tmpres < 0) {
	  RkSetErrno(RK_ERRNO_EACCES);
	  goto read_err;
	}
	tmpres = read(fd, (char *)hd->data[i].ptr, (unsigned)len);
	if (tmpres != (int)len) {
	  RkSetErrno(RK_ERRNO_EACCES);
	  goto read_err;
	}
      }
      hd->data[i].ptr[len] = 0;
    }
  }
  if (hd->data[HD_MAG].var != (long)bst4_to_l("CDIC")) {
    goto read_err;
  }
  return 0;
 read_err:
  for (i = 0; i < HD_MAXTAG; i++) {
    if (hd->flag[i] > 0)
      free(hd->data[i].ptr);
    hd->flag[i] = 0;
    hd->data[i].var = 0;
  }
  return -1;
}

unsigned char *
_RkCreateHeader(struct HD *hd, unsigned *size)
{
  unsigned char	*tagdst, *datadst, *ptr;
  int		i, j;
  unsigned long	len, off;

  if (!hd)
    return 0;
  tagdst = localbuffer;
  datadst = localbuffer + HD_MAXTAG * HD_MIN_TAGSIZ;
  datadst += sizeof(long);
  for (i = 0; i < HD_MAXTAG; i++) {
    for (j = 0; j < sizeof(essential_tag); j++) {
      if (essential_tag[j] == i) {
	break;
      }
    }
    memcpy(tagdst, Hdrtag[i], HD_TAGSIZ);
    tagdst += HD_TAGSIZ;
    if (hd->flag[i] == -1) {
      len = 0;
      off = hd->data[i].var;
    } else if (hd->flag[i] > 0) {
      len = hd->flag[i];
      off = datadst - localbuffer;
      memcpy(datadst, hd->data[i].ptr, (size_t) len);
      datadst += len;
    } else {
      len = 0;
      off = 0;
    }
    l_to_bst4(len, tagdst); tagdst += HD_TAGSIZ;
    l_to_bst4(off, tagdst); tagdst += HD_TAGSIZ;
  }
  *tagdst++ = 0; *tagdst++ = 0; *tagdst++ = 0; *tagdst++ = 0;
  *size = datadst - localbuffer;
  if (!(ptr = (unsigned char *)malloc(*size))) {
    return 0;
  }
  memcpy(ptr, localbuffer, *size);
  return ptr;
}

unsigned long
_RkGetTick(int mode)
{
  static unsigned long time = 10000;
  return(mode ? time++ : time);
}

int
set_hdr_var(struct HD *hd, int n, unsigned long var)
{
    if (!hd)
	return -1;
    hd->data[n].var = var;
    hd->flag[n] = -1;
    return 0;
}

int
_RkGetLink(struct ND *dic, long pgno, unsigned long off, unsigned long *lvo, unsigned long *csn)
{
  struct NP	*pg = dic->pgs + pgno;
  unsigned char	*p;
  unsigned	i;

  for (i = 0, p = pg->buf + 14 + 4 * pg->ndsz; i < pg->lnksz; i++, p += 5) {
    if (thisPWO(p) == off) {
      *lvo = pg->lvo + thisLVO(p);
      *csn = pg->csn + thisCSN(p);
      return(0);
    }
  }
  return(-1);
}

unsigned long
_RkGetOffset(struct ND *dic, unsigned char *pos)
{
  struct NP	*pg;
  unsigned char	*p;
  unsigned	i;
  unsigned long	lvo;

  for (i = 0; i < dic->ttlpg; i++) {
    if (dic->pgs[i].buf) {
      if (dic->pgs[i].buf < pos && pos < dic->pgs[i].buf + dic->pgsz)
	break;
    }
  }
  if (i == dic->ttlpg) {
    return(0);
  }
  pg = dic->pgs + i;
  for (i = 0, p = pg->buf + 14 + 4 * pg->ndsz; i < pg->lnksz; i++, p += 5) {
    if ((unsigned long) (pos - pg->buf) == thisPWO(p)) {
      lvo = pg->lvo + thisLVO(p);
      return(lvo);
    }
  }
  _Rkpanic("Cannot get Offset", 0, 0, 0);
}

int
HowManyChars(WCHAR_T *yomi, int len)
{
  int chlen, bytelen;

  for (chlen = 0, bytelen = 0; bytelen < len; chlen++) {
    WCHAR_T ch = yomi[chlen];
    
    if (us_iscodeG0(ch))
      bytelen++;
    else if (us_iscodeG3(ch))
      bytelen += 3;
    else
      bytelen += 2;
  }
  return(chlen);
}

int
HowManyBytes(WCHAR_T *yomi, int len)
{
  int chlen, bytelen;

  for (chlen = 0, bytelen = 0; chlen < len; chlen++) {
    WCHAR_T ch = yomi[chlen];

    if (us_iscodeG0(ch))
      bytelen++;
    else if (us_iscodeG3(ch))
      bytelen += 3;
    else {
      bytelen += 2;
    }
  }
  return(bytelen);
}
