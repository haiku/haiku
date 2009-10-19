/* Copyright 1992 NEC Corporation, Tokyo, Japan.
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
////////////////////////////////////////////////////////////////////////
// This source cdde is Modified 1998 by T.Murai for kanBe.
////////////////////////////////////////////////////////////////////////

#if !defined(lint) && !defined(__CODECENTER__)
static char rcsid[]="@(#) 102.1 $Id: RKroma.c 10525 2004-12-23 21:23:50Z korli $";
#endif

#include "canna.h"
// There is Exported Symbols !!
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define S2TOS(s2)	(((unsigned short)(s2)[0]<<8)|(s2)[1])

#ifdef WIN
#define JAPANESE_SORT
#endif

#ifdef JAPANESE_SORT

struct romaRec {
  unsigned char *roma;
  unsigned char bang;
};

static int findRoma(struct RkRxDic *rdic, struct rstat *m, unsigned char c, int n, int flg);
static unsigned char *getKana(struct RkRxDic *rdic, int p, int flags);
static unsigned char *getTSU(struct RkRxDic *rdic, int flags);
static unsigned char *getTemp(struct RkRxDic *rdic, int p);

int
compar(struct romaRec *p, struct romaRec *q)
{	
  unsigned char	*s = p->roma;
  unsigned char	*t = q->roma;

  while ( *s == *t )
    if (*s)
      s++, t++;
    else
      return 0;
  return ((int)*s) - ((int)*t);
}
#endif /* JAPANESE_SORT */

#define ROMDICHEADERLEN 6

struct RkRxDic *
RkwOpenRoma(char *romaji)
{
    struct RkRxDic	*rdic;
#ifdef JAPANESE_SORT
    struct romaRec *tmp_rdic;
#endif

    rdic = (struct RkRxDic *)malloc(sizeof(struct RkRxDic));
    if (rdic) {
	int	dic;
	unsigned char	header[ROMDICHEADERLEN];
	unsigned char	*s;
	int	i, sz, open_flags = O_RDONLY;

#ifdef O_BINARY
	open_flags |= O_BINARY;
#endif
	if ( (dic = open((char *)romaji, open_flags)) < 0 ) {
		free(rdic);
		return((struct RkRxDic *)0);
	}
/* magic no shougou */
	if ( read(dic, (char *)header, ROMDICHEADERLEN) != ROMDICHEADERLEN ||
	     (strncmp((char *)header, "RD", 2) &&
	      strncmp((char *)header, "KP", 2)) ) {
		(void)close(dic);
		free(rdic);
		return((struct RkRxDic *)0);
	}
	if ( !strncmp((char *)header, "KP", 2) ) {
	  rdic->dic = RX_KPDIC;
	}
	else {
	  rdic->dic = RX_RXDIC;
	}
	rdic->nr_strsz = S2TOS(header + 2);
	rdic->nr_nkey  = S2TOS(header + 4);
	if (rdic->nr_strsz > 0) {
	  rdic->nr_string =
	    (unsigned char *)malloc((unsigned int)rdic->nr_strsz);

	  if ( !rdic->nr_string ) {
		(void)close(dic);
		free(rdic);
		return((struct RkRxDic *)0);
	  }

	  sz = read(dic, (char *)rdic->nr_string, rdic->nr_strsz);
	  (void)close(dic);
	  if ( sz != rdic->nr_strsz ) {
	    free(rdic->nr_string);
	    free(rdic);
	    return((struct RkRxDic *)0);
	  }
	}
	else {
	  rdic->nr_string = (unsigned char *)0;
	}

	if (rdic->nr_nkey > 0) {
	  rdic->nr_keyaddr =
	    (unsigned char **)calloc((unsigned)rdic->nr_nkey, 
				     sizeof(unsigned char *));
	  if ( !rdic->nr_keyaddr ) {
	    free(rdic->nr_string);
	    free(rdic);
	    return((struct RkRxDic *)0);
	  }
	}
	else {
	  rdic->nr_keyaddr = (unsigned char **)0;
	}

	s = rdic->nr_string;

	/* ¥È¥ê¥¬¡¼Ê¸»ú¤Î¥Ý¥¤¥ó¥¿ */
	if (rdic->dic == RX_KPDIC) { /* KPDIC ¤Ç nr_string ¤¬Ìµ¤¤¤³¤È¤Ï¤Ê¤¤ */
	  rdic->nr_bchars = s;
	  while (*s++)
	    /* EMPTY */
	    ;

	  /* ¥È¥ê¥¬¡¼Ê¸»ú¤¬¤¢¤ë¤Î¤Ê¤é¡¢¥È¥ê¥¬¡¼¥ë¡¼¥ë¤â¤¢¤ë¤Ï¤º */
	  if (*rdic->nr_string && rdic->nr_nkey > 0) {
	    rdic->nr_brules = (unsigned char *)calloc((unsigned)rdic->nr_nkey,
                                                       sizeof(unsigned char));
	  }
	  else {
	    rdic->nr_brules = (unsigned char *)0;
	  }
	}
	else {
	  rdic->nr_brules = (unsigned char *)0;
	}

	/* ¥ë¡¼¥ë¤ÎÆÉ¤ß¹þ¤ß */
	for ( i = 0; i < rdic->nr_nkey; i++ ) {
	    rdic->nr_keyaddr[i] = s;
	    while (*s++)
	      /* EMPTY */
	      ;
	    while (*s++)
	      /* EMPTY */
	      ;
	    if (rdic->dic == RX_KPDIC) {
	      while ( *s > 0x19 ) s++;
	      if (*s) { /* ¥È¥ê¥¬¡¼¥ë¡¼¥ë */
		if (rdic->nr_brules) {
		  rdic->nr_brules[i] = (unsigned char)1;
		}
		*s = (unsigned char)'\0';
	      }
	      s++;
	    }
	}

#ifdef JAPANESE_SORT
	tmp_rdic = (struct romaRec *)calloc((unsigned)rdic->nr_nkey,
                                              sizeof(struct romaRec));
	if (!tmp_rdic) {
          if (rdic->nr_string)
            free(rdic->nr_string);
          if (rdic->nr_keyaddr)
            free(rdic->nr_keyaddr);
	  if (rdic->nr_brules)
            free(rdic->nr_brules);
	  free(rdic);
	  return (struct RkRxDic *)NULL;
	}

        for (i = 0; i < rdic->nr_nkey; i++) {
	  tmp_rdic[i].roma = rdic->nr_keyaddr[i];
	  if (rdic->nr_brules)
	    tmp_rdic[i].bang = rdic->nr_brules[i]; 
	}

        qsort((char *)tmp_rdic, rdic->nr_nkey, sizeof(struct romaRec), 
                (int (*) (const void *, const void *)))compar;

        for (i = 0; i < rdic->nr_nkey; i++) {
	  rdic->nr_keyaddr[i] = tmp_rdic[i].roma;
	  if (rdic->nr_brules)
	    rdic->nr_brules[i]  = tmp_rdic[i].bang; 
	}
	free ((char *)tmp_rdic);
#endif /* JAPANESE_SORT */
    }
    return((struct RkRxDic *)rdic);
}
/* RkCloseRoma
 *	romaji henkan table wo tojiru
 */
void
RkwCloseRoma(struct RkRxDic *rdic)
{
    if ( rdic ) {
        if (rdic->nr_string) free(rdic->nr_string);
        if (rdic->nr_keyaddr) free(rdic->nr_keyaddr);
	if (rdic->nr_brules) free(rdic->nr_brules);
	free(rdic);
    };
}

struct RkRxDic *
RkOpenRoma(char *romaji)
{
  return RkwOpenRoma(romaji);
}

void
RkCloseRoma(struct RkRxDic *rdic)
{
  RkwCloseRoma(rdic);
}

/* RkMapRoma
 *	key no sentou wo saichou itti hou ni yori,henkan suru
 */
#define	xkey(roma, line, n) 	((roma)->nr_keyaddr[line][n])

struct rstat {
    int	start, end;	/* match sury key no hanni */
};

static 
int	
findRoma(struct RkRxDic *rdic, struct rstat *m, unsigned char c, int n, int flg)
{
    register int	s, e;

    if (flg && 'A' <= c && c <= 'Z') {
      c += 'a' - 'A';
    }
    for(s = m->start; s < m->end; s++) 
	if( c == xkey(rdic, s, n) )
	    break;
    for(e = s; e < m->end; e++) 
	if( c != xkey(rdic, e, n) )
	    break;
    m->start	= s;
    m->end	= e;
    return e - s;
}
static 
unsigned char	*
getKana(struct RkRxDic *rdic, int p, int flags)
{
    register unsigned char	*kana;
    int				klen;
    static unsigned  char	tmp[256];

    for (kana = rdic->nr_keyaddr[p] ; *kana++ ; )
      /* EMPTY */
      ;

    klen = strlen((char *)kana);
    switch(flags&RK_XFERMASK) {
    default: 
	(void)RkCvtNone(tmp, sizeof(tmp), kana, klen);
	return tmp;
    case RK_XFER:
	(void)RkCvtHira(tmp, sizeof(tmp), kana, klen);
	return tmp;
    case RK_HFER:
	(void)RkCvtHan(tmp, sizeof(tmp), kana, klen);
	return tmp;
    case RK_KFER:
	(void)RkCvtKana(tmp, sizeof(tmp), kana, klen);
	return tmp;
    case RK_ZFER:
	(void)RkCvtZen(tmp, sizeof(tmp), kana, klen); 
	return tmp;
    };
}

inline 
unsigned char	*
getRoma(struct RkRxDic *rdic, int p)
{
    return rdic->nr_keyaddr[p];
}

/*ARGSUSED*/
static 
unsigned char	*
getTSU(struct RkRxDic *rdic, int flags)
{
    static unsigned char  hira_tsu[] = {0xa4, 0xc3, 0};
    static unsigned char  kana_tsu[] = {0xa5, 0xc3, 0};
    static unsigned char  han_tsu[] =  {0x8e, 0xaf, 0};

    switch(flags&RK_XFERMASK) {
    default:	  return hira_tsu;
    case RK_HFER: return han_tsu;
    case RK_KFER: return kana_tsu;
    };
}

int	
RkMapRoma(struct RkRxDic *rdic, unsigned char *dst, int maxdst, unsigned char *src, int maxsrc, int flags, int *status)
{
    register int	i;
    unsigned char	*roma;
    unsigned char	*kana = src;
    int			count = 0;
    int			byte;
    int			found = 1;
    struct rstat *m;
    struct rstat match[256];

    if ( rdic ) {
	m = match;
	m->start = 0;
	m->end = rdic->nr_nkey;
	for (i = 0; (flags & RK_FLUSH) || i < maxsrc;  i++) {
	    m[1] = m[0];
	    m++;
	    switch((i < maxsrc) ? findRoma(rdic, m, src[i], i, 0) : 0) {
	    case	0:
		while (--m > match && xkey(rdic, m->start, m - match))
		  /* EMPTY */
		  ;
		if(m == match) { /* table ni nakatta tokino shori */
		    kana = src;
		    count = (maxsrc <= 0)? 0 : (*src & 0x80)? 2 : 1;
		    if( (flags & RK_SOKON) &&
			(match[1].start < rdic->nr_nkey) &&
			(2 <= maxsrc) &&
			(src[0] == src[1]) &&
			(i == 1)) {
			kana = getTSU(rdic, flags);
		    /* tsu ha jisho ni aru kao wo suru */
			byte = strlen((char *)kana);
		    }
		    else {
			static unsigned char	tmp[256];

			switch(flags&RK_XFERMASK) {
			default: 
			    byte = RkCvtNone(tmp, sizeof(tmp), src, count);
			    break;
			case RK_XFER:
			    byte = RkCvtHira(tmp, sizeof(tmp), src, count);
			    break;
			case RK_HFER:
			    byte = RkCvtHan(tmp, sizeof(tmp), src, count);
			    break;
			case RK_KFER:
			    byte = RkCvtKana(tmp, sizeof(tmp), src, count);
			    break;
			case RK_ZFER:
			    byte = RkCvtZen(tmp, sizeof(tmp), src, count); 
			    break;
			};
			kana = tmp;
			found = -1;
		    };
		}
		else {  /* 'n' nado no shori: saitan no monowo toru */
		    kana = getKana(rdic, m->start, flags);
		    byte = strlen((char *)kana);
		    count = m - match;
		}
		goto done;
	    case	1:	/* determined uniquely */
	    /* key no hou ga nagai baai */
		roma = getRoma(rdic, m->start);
		if ( roma[i + 1] ) 	/* waiting suffix */
		    continue;
		kana = getKana(rdic, m->start, flags);
		byte = strlen((char *)kana);
		count = i + 1;
		goto done;
	    };
	};
	byte = 0;
    }
    else 
	byte = (maxsrc <= 0) ? 0 : (*src & 0x80) ? 2 : 1;
done:
    *status = found*byte;
    if ( byte + 1 <= maxdst ) {
	if ( dst ) {
	    while ( byte-- )
		*dst++ = *kana++;
	    *dst = 0;
	};
    };
    return count;
}

inline 
unsigned char	*
getrawKana(struct RkRxDic *rdic, int p)
{
  register unsigned char	*kana;

  for (kana = rdic->nr_keyaddr[p] ; *kana++ ; )
    /* EMPTY */
    ;

  return kana;
}

static 
unsigned char	*
getTemp(struct RkRxDic *rdic, int p)
{
  register unsigned char	*kana;

  if (rdic->dic != RX_KPDIC) {
    return (unsigned char *)0;
  }
  kana = rdic->nr_keyaddr[p];
  while (*kana++)
    /* EMPTY */
    ;
  while (*kana++)
    /* EMPTY */
    ;

  return kana;
}


int	
RkMapPhonogram(struct RkRxDic *rdic, unsigned char *dst, int maxdst, unsigned char *src, int srclen, unsigned key, int flags, int *used_len_return, int *dst_len_return, int *tmp_len_return, int *rule_id_inout)
{
  register int	i;
  unsigned char	*roma, *temp;
  unsigned char	*kana = src;
  int			count = 0;
  int			byte;
  int			found = 1;
  int templen, lastrule;
  struct rstat *m;
  struct rstat match[256];
  
  if ( rdic ) {
    if (rdic->dic == RX_KPDIC 
	&& rule_id_inout && (lastrule = *rule_id_inout)) {
      if (!key) {
	if (rdic->nr_brules && rdic->nr_brules[lastrule] &&
	    !(flags & RK_FLUSH)) {
	  /* ¤â¤·¡¢! ¤¬ÉÕ¤¤¤Æ¤¤¤¿¾ì¹ç¤Ë¤ÏÂè£³¥Õ¥£¡¼¥ë¥É¤Ë½ñ¤«¤ì¤Æ¤¤¤ë 
             Ê¸»ú¤Ç»Ï¤Þ¤ë¥ë¡¼¥ë¤¬¤¢¤ë¤È²¾ÁÛÅª¤Ë¹Í¤¨¤é¤ì¤ë¤ï¤±¤Ç¤¢¤ë¤« 
             ¤é key ¤¬Í¿¤¨¤é¤ì¤Æ¤¤¤Ê¤¤¤Î¤Ç¤¢¤ì¤ÐÍ¿¤¨¤é¤ì¤¿Ê¸»úÎó¤¬Ã»¤«
             ¤¹¤®¤ë¤¿¤á¤Ê¤ó¤È¤â¤Ç¤­¤Ê¤¤¤è¤·¤Î¥ê¥¿¡¼¥óÃÍ¤òÊÖ¤¹¡£ */
	  /* RK_FLUSH ¤ÏÄ´¤Ù¤ë¤Ù¤­¤«¤É¤¦¤«Çº¤à¤È¤³¤í */
	  byte = count = 0;
	  templen = 0;
	  found = 0;
	  goto done;
	}
      }else{
	lastrule--;
	if (lastrule < rdic->nr_nkey && rdic->nr_brules) {
	  if (rdic->nr_brules[lastrule]) {
	    unsigned char *p;

	    for (p = rdic->nr_bchars ; *p ; p++) {
	      if (key == *p) {
		unsigned char *origin = getTemp(rdic, lastrule), *ret;
		int dstlen = 0, tmplen;

		ret = dst;
		for (i = 0 ; i < maxdst && *origin ; i++) {
		  origin++;
		}
		if (i + 1 == srclen) {
		  /* ¥Ð¥Ã¥¯¥È¥é¥Ã¥¯¤ò¤¹¤ë */
		  origin = rdic->nr_keyaddr[lastrule];

		  for (i = 0 ; i < maxdst && *origin ; i++) {
		    *dst++ = *origin++;
		  }
		  tmplen = ++i;
		  if (i < maxdst) {
		    *dst++ = key;
		    *dst = (unsigned char)0;
		  }
		  if (used_len_return) *used_len_return = srclen;
		  if (*ret & 0x80) { /* very dependent on Japanese EUC */
		    if (*ret == 0x8f) {
		      dstlen++;
		    }
		    dstlen++;
		  }
		  dstlen++;
		  if (dst_len_return) *dst_len_return = dstlen;
		  if (tmp_len_return) *tmp_len_return = tmplen - dstlen;
		  *rule_id_inout = 0;
		  goto return_found;
		}
	      }
	    }
	  }
	}
      }
    }
    m = match;
    m->start = 0;
    m->end = rdic->nr_nkey;
    for (i = 0; (flags & RK_FLUSH) || i < srclen;  i++) {
      m[1] = m[0];
      m++;
      switch((i < srclen) ?
	     findRoma(rdic, m, src[i], i, flags & RK_IGNORECASE) : 0) {
      case	0:
	while (--m > match && xkey(rdic, m->start, m - match))
	  /* EMPTY */
	  ;
	if(m == match) { /* ¥Æ¡¼¥Ö¥ë¤Ë¤Ê¤«¤Ã¤¿»þ¤Î½èÍý */
	  count = (*src & 0x80) ? 2 : 1;
	  if (srclen < count) {
	    count = 0;
	  }
	  if( (rdic->dic == RX_RXDIC) && /* tt ¤ÎµßºÑ(µì¼­½ñÍÑ) */
	     (flags & RK_SOKON) &&
	     (match[1].start < rdic->nr_nkey) &&
	     (2 <= srclen) &&
	     (src[0] == src[1]) &&
	     (i == 1)) {
	    kana = getTSU(rdic, flags);
	    /* tsu ha jisho ni aru kao wo suru */
	    byte = strlen((char *)kana);
	    templen = 0;
	    if (rule_id_inout) *rule_id_inout = 0;
	  }
	  else { /* £±Ê¸»úÊÑ´¹¤µ¤ì¤¿¤³¤È¤Ë¤¹¤ë */
	    byte = count;
	    templen = 0;
	    kana = src;
	    found = 0;
	  }
	}
	else {  /* 'n' ¤Ê¤É¤Î½èÍý: ºÇÃ»¤Î¤â¤Î¤ò¼è¤ë */
	  kana = getrawKana(rdic, m->start);
	  byte = strlen((char *)kana);
	  temp = getTemp(rdic, m->start);
	  templen = temp ? strlen((char *)temp) : 0;
	  count = m - match;
	  if (rule_id_inout) {
	    if (byte == 0 && templen > 0) {
	      *rule_id_inout = m->start + 1;
	    }
	    else {
	      *rule_id_inout = 0;
	    }
	  }
	}
	goto done;
      case	1: /* ÅÓÃæ¤Ç¤É¤ó¤Ô¤·¤ã¤¬¸«¤Ä¤«¤Ã¤¿ */
	/* key no hou ga nagai baai */
	roma = getRoma(rdic, m->start);
	if ( roma[i + 1] ) 	/* waiting suffix */
	  continue;
	kana = getrawKana(rdic, m->start);
	byte = strlen((char *)kana);
	temp = getTemp(rdic, m->start);
	templen = temp ? strlen((char *)temp) : 0;
	count = i + 1;
	if (rule_id_inout) {
	  if (byte == 0 && templen > 0) {
	    *rule_id_inout = m->start + 1;
	  }
	  else {
	    *rule_id_inout = 0;
	  }
	}
	goto done;
      }
    }
    byte = count = 0;
    templen = 0;
  }
  else {
    byte = (*src & 0x80) ? 2 : 1;
    if (srclen < byte) {
      byte = 0;
    }
    count = byte;
    kana = src;
    templen = 0;
    found = 0;
  }
 done:
  
  if (dst_len_return) {
    *dst_len_return = byte;
  }
  if (used_len_return) {
    *used_len_return = count;
  }
  if (tmp_len_return) {
    *tmp_len_return = templen;
  }
  if ( byte < maxdst ) {
    if ( dst ) {
      int ii;
      for (ii = 0 ; ii < byte ; ii++)
	*dst++ = *kana++;
      *dst = 0;
    }
    if (byte + templen < maxdst) {
      if (dst) {
	while (templen--) {
	  *dst++ = *temp++;
	}
	*dst = 0;
      }
    }
  }
 return_found:
  return found;
}

/* RkCvtRoma
 */
int	
RkCvtRoma(struct RkRxDic *rdic, unsigned char *dst, int maxdst, unsigned char *src, int maxsrc, unsigned flags)
{
    register unsigned char	*d = dst;
    register unsigned char	*s = src;
    register unsigned char	*S = src + maxsrc;
    int count = 0;
    unsigned xp = 0;
    unsigned char key;
#ifndef WIN
    unsigned char xxxx[64], yyyy[64];
#else
    unsigned char *xxxx, *yyyy;
    xxxx = (unsigned char *)malloc(64);
    yyyy = (unsigned char *)malloc(64);
    if (!xxxx || !yyyy) {
      if (xxxx) {
	free(xxxx);
      }
      if (yyyy) {
	free(yyyy);
      }
      return count;
    }
#endif

    if (!(maxdst <= 0 || maxsrc < 0)) {
      while ( s < S ) {
	int ulen, dlen, tlen, rule = 0;
	unsigned dontflush = RK_FLUSH;
      
	key = xxxx[xp++] = *s++;
      flush:
	do {
	  RkMapPhonogram(rdic, d, maxdst, xxxx, xp, (unsigned)key,
			 flags & ~dontflush, &ulen, &dlen, &tlen, &rule);
	
	  if ( dlen + 1 <= maxdst ) {
	    maxdst -= dlen; count += dlen;
	    if ( dst ) {
	      d += dlen;
	      (void)strncpy((char *)yyyy, (char *)d, tlen);
	    }
	  }
	
	  if (ulen < (int)xp) {
	    strncpy((char *)yyyy + tlen, (char *)xxxx + ulen, xp - ulen);
	  }
	  strncpy((char *)xxxx, (char *)yyyy, tlen + xp - ulen);
	  xp = tlen + xp - ulen;
	  key = 0;
	} while (ulen > 0);
	if (s == S && dontflush) {
	  dontflush = 0;
	  goto flush;
	}
      }
    }
#ifdef WIN
    free(yyyy);
    free(xxxx);
#endif
    return count;
}
