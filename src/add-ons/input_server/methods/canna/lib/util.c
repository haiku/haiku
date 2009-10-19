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

/************************************************************************/
/* THIS SOURCE CODE IS MODIFIED FOR TKO BY T.MURAI 1997
/************************************************************************/


/* filedef

  util.c -- ¥æ¡¼¥Æ¥£¥ê¥Æ¥£´Ø¿ô¤ò½¸¤á¤¿¡£

  °Ê²¼¤Î´Ø¿ô¤¬¤¢¤ë¡£(ÄÉ²Ã¤·¤¿¿Í¤Ï¤Á¤ã¤ó¤È½ñ¤¤¤È¤¤¤Æ¤è)

  GlineClear         ¥¬¥¤¥É¥é¥¤¥ó¤¬¾Ã¤µ¤ì¤ë¤è¤¦¤Ê¥ê¥¿¡¼¥óÃÍ¤òºî¤ë
  Gline2echostr      ¥¬¥¤¥É¥é¥¤¥ó¤ÇÊÖ¤½¤¦¤È¤·¤¿¤â¤Î¤ò¤½¤Î¾ì¤ÇÊÖ¤¹
  echostrClear       ¤½¤Î¾ì¤¬Á´¤¯¾Ã¤µ¤ì¤ë¤è¤¦¤Ê¥ê¥¿¡¼¥óÃÍ¤òºî¤ë
  checkGLineLen      ¥¬¥¤¥É¥é¥¤¥ó¤ËÉ½¼¨¤·¤­¤ì¤ë¤«¤É¤¦¤«¤Î¥Á¥§¥Ã¥¯
  NothingChanged     ²¿¤âÊÑ²½¤¬¤Ê¤¤¤³¤È¤ò¼¨¤¹¥ê¥¿¡¼¥óÃÍ¤òºî¤ë
  NothingForGLine    ¥¬¥¤¥É¥é¥¤¥ó¤Ë´Ø¤·¤Æ¤Ï²¿¤âÊÑ²½¤¬¤Ê¤¤
  NothingChangedWithBeep
                     NothingChange ¤ò¤·¤Æ¤µ¤é¤Ë¥Ó¡¼¥×²»¤òÌÄ¤é¤¹
  NothingForGLineWithBeep
                     NothingForGLine ¤ò¤·¤Æ¤µ¤é¤Ë¥Ó¡¼¥×²»¤òÌÄ¤é¤¹
  CannaBeep          ¥Ó¡¼¥×²»¤ò¤Ê¤é¤¹¡£
  makeGLineMessage   °ú¿ô¤ÎÊ¸»úÎó¤òGLine¤ËÉ½¼¨¤¹¤ë¤è¤¦¤Ê¥ê¥¿¡¼¥óÃÍ¤òºî¤ë
  makeGLineMessageFromString
  		     °ú¿ô¤ÎeucÊ¸»úÎó¤òGLine¤ËÉ½¼¨¤¹¤ë¤è¤¦¤Ê¥ê¥¿¡¼¥óÃÍ¤òºî¤ë
  setWStrings	     Ê¸»úÇÛÎó¤Î½é´ü²½¤ò¹Ô¤¦
  NoMoreMemory       ¥á¥â¥ê¤¬¤Ê¤¤¤«¤é¥¨¥é¡¼¤À¤è¤È¤¤¤¦¥¨¥é¡¼ÃÍ¤òÊÖ¤¹
  GLineNGReturn      ¥¨¥é¡¼¥á¥Ã¥»¡¼¥¸¤ò¥¬¥¤¥É¥é¥¤¥ó¤Ë°Ü¤¹
  GLineNGReturnFI    °ìÍ÷¥â¡¼¥É¤òÈ´¤±¤Æ GLineNGReturn ¤ò¤¹¤ë¡£
  GLineNGReturnTK    ÅÐÏ¿¥â¡¼¥É¤òÈ´¤±¤Æ GLineNGReturn ¤ò¤¹¤ë¡£
  WStrlen            ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿Ê¸»úÎó¤ÎÄ¹¤µ¤òµá¤á¤ë (cf. strlen)
  WStrcat            ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿Ê¸»úÎó¤ò²Ã¤¨¤ë¡£(cf. strcat)
  WStrcpy            ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿Ê¸»úÎó¤ò¥³¥Ô¡¼¤¹¤ë¡£(cf. strcpy)
  WStrncpy           ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿Ê¸»úÎó¤ò£îÊ¸»ú¥³¥Ô¡¼¤¹¤ë¡£(cf. strncpy)
  WStraddbcpy        ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿Ê¸»úÎó¤ò¶õÇòÊ¸»ú¡¢¥¿¥Ö¡¢¥Ð¥Ã¥¯¥¹¥é¥Ã¥·¥å
                     ¤ÎÁ°¤Ë¥Ð¥Ã¥¯¥¹¥é¥Ã¥·¥å¤òÆþ¤ì¤Ê¤¬¤é¥³¥Ô¡¼¤¹¤ë¡£
  WStrcmp	     ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿Ê¸»úÎó¤òÈæ³Ó¤¹¤ë¡£(cf. strcmp)
  WStrncmp	     ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿Ê¸»úÎó¤ò£îÊ¸»úÈæ³Ó¤¹¤ë¡£(cf. strncmp)
  WWhatGPlain	     ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿£±Ê¸»ú¤ÎÂ°¤¹¤ë¥°¥é¥Õ¥£¥Ã¥¯¥×¥ì¡¼¥ó¤òÊÖ¤¹
  WIsG0              G0¤Î¥ï¥¤¥É¥­¥ã¥é¥¯¥¿Ê¸»ú¤«¡©
  WIsG1              G1¤Î¥ï¥¤¥É¥­¥ã¥é¥¯¥¿Ê¸»ú¤«¡©
  WIsG2              G2¤Î¥ï¥¤¥É¥­¥ã¥é¥¯¥¿Ê¸»ú¤«¡©
  WIsG3              G3¤Î¥ï¥¤¥É¥­¥ã¥é¥¯¥¿Ê¸»ú¤«¡©
  CANNA_mbstowcs     EUC ¤ò¥ï¥¤¥É¥­¥ã¥é¥¯¥¿Ê¸»úÎó¤ËÊÑ´¹
  CNvW2E             ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿¤ò EUC ¤ËÊÑ´¹(¥Á¥§¥Ã¥¯ÉÕ¤­)
  CANNA_wcstombs     ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿¤ò EUC ¤ËÊÑ´¹
  WSfree	     WString¤Ç³ÎÊÝ¤·¤¿ÎÎ°è¤ò³«Êü¤¹¤ë
  WString            EUC ¤ò¥ï¥¤¥É¤ËÊÑ´¹¤·¤Æ malloc ¤Þ¤Ç¤·¤ÆÊÖ¤¹(free ÉÔÍ×)
  WStringOpen        ¾åµ­´Ø¿ô¤Î½é´ü²½½èÍý
  WStringClose       ¾åµ­´Ø¿ô¤Î½ªÎ»½èÍý
  WToupper           °ú¿ô¤ÎÊ¸»ú¤òÂçÊ¸»ú¤Ë¤¹¤ë
  WTolower           °ú¿ô¤ÎÊ¸»ú¤ò¾®Ê¸»ú¤Ë¤¹¤ë
  key2wchar          ¥­¡¼¥Ü¡¼¥ÉÆþÎÏ¤ò¥ï¥¤¥É¥­¥ã¥é¥¯¥¿¤Ë¤¹¤ë¡£
  US2WS              Ushort ¤ò WCHAR_T ¤ËÊÑ´¹¤¹¤ë¡£
  WS2US              WCHAR_T ¤ò Ushort ¤ËÊÑ´¹¤¹¤ë¡£
  confirmContext     yc->context ¤¬»È¤¨¤ë¤â¤Î¤«³ÎÇ§¤¹¤ë 
  makeRkError        Rk ¤Î´Ø¿ô¤Ç¥¨¥é¡¼¤¬¤Ç¤¿¤È¤­¤Î½èÍý¤ò¤¹¤ë¡£
  canna_alert        ¥á¥Ã¥»¡¼¥¸¤ò Gline ¤Ë½ñ¤¤¤Æ key ¤òÂÔ¤Ä¡£

 */

#if !defined(lint) && !defined(__CODECENTER__)
static char rcs_id[] = "@(#) 102.1 $Id: util.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif	/* lint */

#include "canna.h"
#include <errno.h>
#include "RK.h"
#include "RKintern.h"
#include <fcntl.h>

#ifdef HAVE_WCHAR_OPERATION
#include <locale.h>
#if defined(__STDC__) || defined(SVR4)
#include <limits.h>
#endif
#endif

#ifndef MB_LEN_MAX
#define MB_LEN_MAX 5 /* 5 ¤â¹Ô¤«¤Ê¤¤¤À¤í¤¦¤È¤Ï»×¤¦ */
#endif

static int wait_anykey_func(uiContext d, KanjiMode mode, int whattodo, int key, int fnum);

/* arraydef

  tmpbuf -- ¤Á¤ç¤Ã¤È²¾¤Ë»È¤ï¤ì¤ë¥Ð¥Ã¥Õ¥¡

 */

/*
 * Gline ¤ò¥¯¥ê¥¢¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	¤Ê¤·
 */
void
GlineClear(uiContext d)
{
  d->kanji_status_return->info |= KanjiGLineInfo;
  d->kanji_status_return->gline.line = (WCHAR_T *)NULL;
  d->kanji_status_return->gline.length = 0;
  d->kanji_status_return->gline.revPos = 0;
  d->kanji_status_return->gline.revLen = 0;
}

/* cfuncdef

  Gline2echostr -- °ìÍ÷¹Ô¤ÎÆâÍÆ¤ò¤½¤Î¾ì¤Ë°ÜÆ°

 */

inline void
Gline2echostr(uiContext d)
{
  d->kanji_status_return->echoStr =
    d->kanji_status_return->gline.line;
  d->kanji_status_return->length =
    d->kanji_status_return->gline.length;
  d->kanji_status_return->revPos =
    d->kanji_status_return->gline.revPos;
  d->kanji_status_return->revLen =
    d->kanji_status_return->gline.revLen;
  GlineClear(d);
}

void
echostrClear(uiContext d)
{
  d->kanji_status_return->echoStr = (WCHAR_T *)NULL;
  d->kanji_status_return->length =
    d->kanji_status_return->revPos = d->kanji_status_return->revLen = 0;
}

/* 
 * Ê¸»úÎó¤«¤é¥³¥é¥àÉý¤ò¼è¤Ã¼êÍè¤ë´Ø¿ô
 */

inline
int colwidth(WCHAR_T *s, int len)
{
  int ret = 0;
  WCHAR_T *es = s + len;

  for (; s < es ; s++) {
    switch (WWhatGPlain(*s)) {
    case 0:
    case 2:
      ret ++;
      break;
    case 1:
    case 3:
      ret += 2;
      break;
    }
  }
  return ret;
}


/* cfuncdef

  checkGLineLen -- °ìÍ÷¹Ô¤ËÉ½¼¨¤Ç¤­¤ëÄ¹¤µ¤ò±Û¤¨¤Æ¤¤¤ë¤«¤ò¥Á¥§¥Ã¥¯

  Ä¹¤µ¤¬±Û¤¨¤Æ¤¤¤¿¤é¡¢¥«¡¼¥½¥ëÉôÊ¬¤ËÉ½¼¨¤µ¤ì¤ë¤è¤¦¤Ë¤¹¤ë¡£

 */

int
checkGLineLen(uiContext d)
{
  if (d->kanji_status_return->info & KanjiGLineInfo) {
    if (colwidth(d->kanji_status_return->gline.line,
		 d->kanji_status_return->gline.length) > d->ncolumns) {
      Gline2echostr(d);
      return -1;
    }
  }
  return 0;
}

/* cfuncdef

  NothingChanged -- ÆÉ¤ß¤Ë¤Ä¤¤¤Æ¤Ï²¿¤âÊÑ¤¨¤Ê¤¤¤è¤¦¤Ë¤¹¤ë

 */

int
NothingChanged(uiContext d)
{
  d->kanji_status_return->length = -1; /* ÊÑ¤ï¤é¤Ê¤¤¡£ */
  d->kanji_status_return->revPos 
    = d->kanji_status_return->revLen = 0;
  d->kanji_status_return->info = 0;
  return 0;
}

int NothingForGLine(uiContext d)
{
  d->kanji_status_return->length = -1; /* ÊÑ¤ï¤é¤Ê¤¤¡£ */
  d->kanji_status_return->revPos 
    = d->kanji_status_return->revLen = 0;
  return 0;
}

void
CannaBeep(void)
{
  extern int (*jrBeepFunc) (void);

  if (jrBeepFunc) {
    jrBeepFunc();
  }
}

int
NothingChangedWithBeep(uiContext d)
{
  CannaBeep();
  return NothingChanged(d);
}

int
NothingForGLineWithBeep(uiContext d)
{
  CannaBeep();
  return NothingForGLine(d);
}

#ifdef SOMEONE_USE_THIS
/* Ã¯¤â»È¤Ã¤Æ¤¤¤Ê¤¤¤ß¤¿¤¤¡£ */
Insertable(unsigned char ch)
{
  if ((0x20 <= ch && ch <= 0x7f) || (0xa0 <= ch && ch <= 0xff)) {
    return 1;
  }
  else {
    return 0;
  }
}
#endif /* SOMEONE_USE_THIS */


/*
  extractSimpleYomiString -- yomiContext ¤ÎÆÉ¤ßÉôÊ¬¤À¤±¤ò¼è¤ê½Ð¤¹

  °ú¿ô
     yc  -- yomiContext
     s   -- ¼è¤ê½Ð¤¹Àè¤Î¥¢¥É¥ì¥¹
     e   -- ¤³¤³¤ò±Û¤¨¤Æ¼è¤ê½Ð¤·¤Æ¤Ï¤Ê¤é¤Ê¤¤¡¢¤È¸À¤¦¥¢¥É¥ì¥¹
     sr  -- È¿Å¾ÎÎ°è¤Î³«»Ï°ÌÃÖ¤òÊÖ¤¹¥¢¥É¥ì¥¹
     er  -- È¿Å¾ÎÎ°è¤Î½ªÎ»°ÌÃÖ¤òÊÖ¤¹¥¢¥É¥ì¥¹
     pat -- pointer to an attribute buffer.
     focused -- indicates yc is focused or not
 */

inline int
extractSimpleYomiString(yomiContext yc, WCHAR_T *s, WCHAR_T *e, WCHAR_T **sr, WCHAR_T **er, wcKanjiAttributeInternal *pat, int focused)
{
  int len = yc->kEndp - yc->cStartp;

  if (yc->jishu_kEndp) {
    int len = extractJishuString(yc, s, e, sr, er);
    char target = focused ?
      CANNA_ATTR_TARGET_NOTCONVERTED : CANNA_ATTR_CONVERTED;
    if (pat && pat->sp + len < pat->ep) {
      char *ap = pat->sp, *ep = ap + len;
      char *mp1 = ap + (*sr - s), *mp2 = ap + (*er - s);
      while (ap < mp1) {
	*ap++ = CANNA_ATTR_INPUT;
      }
      while (ap < mp2) {
	*ap++ = target;
      }
      while (ap < ep) {
	*ap++ = CANNA_ATTR_INPUT;
      }
      pat->sp = ap;
    }
    return len;
  }

  if (s + len >= e) {
    len = (int)(e - s);
  }
  WStrncpy(s, yc->kana_buffer + yc->cStartp, len);
  if (pat && pat->sp + len < pat->ep) {
    char *ap = pat->sp, *ep = ap + len;

    if (focused) {
      pat->u.caretpos = (ap - pat->u.attr) + yc->kCurs - yc->cStartp;
      /* ¾åµ­¤Î·×»»¤Î²òÀâ: ¥­¥ã¥ì¥Ã¥È¤Î°ÌÃÖ¤Ï¡¢º£¤«¤é½ñ¤­¹þ¤ß¤ò¤·¤è¤¦¤È
	 ¤·¤Æ¤¤¤ë°ÌÃÖ¤«¤é¤ÎÁêÂÐ¤Ç¡¢·×»»¤·¡¢yc->kCurs - yc->cStartp ¤Î°Ì
	 ÃÖ¤Ç¤¢¤ë¡£ */
    }

    while (ap < ep) {
      *ap++ = CANNA_ATTR_INPUT;
    }
    pat->sp = ap;
  }
  if (cannaconf.ReverseWidely) {
    *sr = s;
    *er = s + yc->kCurs - yc->cStartp;
  }
  else if (yc->kCurs == yc->kEndp && !yc->right) {
    *sr = *er = s + yc->kCurs - yc->cStartp;
  }
  else {
    *sr = s + yc->kCurs - yc->cStartp;
    *er = *sr + 1;
  }
  return len;
}

/*
  extractKanjiString -- yomiContext ¤Î´Á»ú¸õÊä¤ò¼è¤ê½Ð¤¹

  °ú¿ô
     yc  -- yomiContext
     s   -- ¼è¤ê½Ð¤¹Àè¤Î¥¢¥É¥ì¥¹
     e   -- ¤³¤³¤ò±Û¤¨¤Æ¼è¤ê½Ð¤·¤Æ¤Ï¤Ê¤é¤Ê¤¤¡¢¤È¸À¤¦¥¢¥É¥ì¥¹
     b   -- Ê¸Àá¶èÀÚ¤ê¤ò¤¹¤ë¤«¤É¤¦¤«
     sr  -- È¿Å¾ÎÎ°è¤Î³«»Ï°ÌÃÖ¤òÊÖ¤¹¥¢¥É¥ì¥¹
     er  -- È¿Å¾ÎÎ°è¤Î½ªÎ»°ÌÃÖ¤òÊÖ¤¹¥¢¥É¥ì¥¹
     pat -- wcKanjiAttributeInternal structure to store attribute information
     focused -- focus is on this yc.
 */

inline int
extractKanjiString(yomiContext yc, WCHAR_T *s, WCHAR_T *e, int b, WCHAR_T **sr, WCHAR_T **er, wcKanjiAttributeInternal *pat, int focused)
{
  WCHAR_T *ss = s;
  int i, len, nbun;

  nbun = yc->bunlen ? yc->curbun : yc->nbunsetsu;

  for (i = 0 ; i < nbun ; i++) {
    if (i && b && s < e) {
      *s++ = (WCHAR_T)' ';
      if (pat && pat->sp < pat->ep) {
	*pat->sp++ = CANNA_ATTR_CONVERTED;
      }
    }
    RkwGoTo(yc->context, i);
    len = RkwGetKanji(yc->context, s, (int)(e - s));
    if (len < 0) {
      if (errno == EPIPE) {
	jrKanjiPipeError();
      }
      jrKanjiError = "¥«¥ì¥ó¥È¸õÊä¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿";
    }
    else {
      char curattr;
      if (i == yc->curbun && !yc->bunlen && focused) { /* focused */
	*sr = s; *er = s + len;
	curattr = CANNA_ATTR_TARGET_CONVERTED;
      }else{
	curattr = CANNA_ATTR_CONVERTED;
      }
      if (pat && pat->sp + len < pat->ep) {
	char *ap = pat->sp, *ep = ap + len;
	while (ap < ep) {
	  *ap++ = curattr;
	}
	pat->sp = ap;
      }
      s += len;
    }
  }

  if (yc->bunlen) {
    if (i && b && s < e) {
      *s++ = (WCHAR_T)' ';
      if (pat && pat->sp < pat->ep) {
	*pat->sp++ = CANNA_ATTR_CONVERTED;
      }
    }
    len = yc->kEndp - yc->kanjilen;
    if ((int)(e - s) < len) {
      len = (int)(e - s);
    }
    WStrncpy(s, yc->kana_buffer + yc->kanjilen, len);
    if (pat && pat->sp + len < pat->ep) {
      char *ap = pat->sp, *mp = ap + yc->bunlen, *ep = ap + len;
      char target = focused ?
	CANNA_ATTR_TARGET_NOTCONVERTED : CANNA_ATTR_CONVERTED;
      while (ap < mp) {
	*ap++ = target;
      }
      while (ap < ep) {
	*ap++ = CANNA_ATTR_INPUT;
      }
      pat->sp = ap;
    }
    if (b) {
      *er = (*sr = s + yc->bunlen) +
	((yc->kanjilen + yc->bunlen == yc->kEndp) ? 0 : 1);
    }
    else {
      *sr = s; *er = s + yc->bunlen;
    }
    s += len;
  }

  if (s < e) {
    *s = (WCHAR_T)'\0';
  }

  RkwGoTo(yc->context, yc->curbun);
  return (int)(s - ss);
}

/*
  extractYomiString -- yomiContext ¤ÎÊ¸»ú¤ò¼è¤ê½Ð¤¹

  °ú¿ô
     yc  -- yomiContext
     s   -- ¼è¤ê½Ð¤¹Àè¤Î¥¢¥É¥ì¥¹
     e   -- ¤³¤³¤ò±Û¤¨¤Æ¼è¤ê½Ð¤·¤Æ¤Ï¤Ê¤é¤Ê¤¤¡¢¤È¸À¤¦¥¢¥É¥ì¥¹
     b   -- Ê¸Àá¶èÀÚ¤ê¤ò¤¹¤ë¤«¤É¤¦¤«
     sr  -- È¿Å¾ÎÎ°è¤Î³«»Ï°ÌÃÖ¤òÊÖ¤¹¥¢¥É¥ì¥¹
     er  -- È¿Å¾ÎÎ°è¤Î½ªÎ»°ÌÃÖ¤òÊÖ¤¹¥¢¥É¥ì¥¹
     pat -- wcKanjiAttributeInternal structure to store attribute information
     focused -- The yc is now focused.
 */

inline int
extractYomiString(yomiContext yc, WCHAR_T *s, WCHAR_T *e, int b, WCHAR_T **sr, WCHAR_T **er, wcKanjiAttributeInternal *pat, int focused)
{
  int autoconvert = yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE, len;
  WCHAR_T *ss = s;

  if (autoconvert) {
    int OnBunsetsu = ((yc->status & CHIKUJI_ON_BUNSETSU) ||
		      (yc->nbunsetsu && !(yc->status & CHIKUJI_OVERWRAP)));
    len = extractKanjiString(yc, s, e, b, sr, er, pat, focused && OnBunsetsu);
    s += len;
    if (yc->kEndp - yc->cStartp > 0) {
      WCHAR_T *ssr, *eer;

      if (b && len && s < e) {
	*s++ = (WCHAR_T)' ';
	if (pat && pat->sp < pat->ep) {
	  *pat->sp++ = CANNA_ATTR_CONVERTED;
	}
      }
      len = extractSimpleYomiString(yc, s, e, &ssr, &eer, pat,
				    focused && !OnBunsetsu);
/* ºÇ¸å¤Î !OnBunsetsu ¤Ã¤Æ¤È¤³¤í¤Ï¡¢°Ê²¼¤Î¤è¤¦¤Ë¤·¤¿Êý¤¬É½¼¨¤¬¥­¥ã¥ì¥Ã¥È
   ¤Ä¤­¤Ç¡¢È¿Å¾Ê¸Àá¤¬½Ð¤Æ¤â¥­¥ã¥ì¥Ã¥È¤¬¥«¡¼¥½¥ë¥Ý¥¸¥·¥ç¥ó¤Ç¤¢¤ë¤³¤È¤¬¤ï
   ¤«¤ê¤ä¤¹¤¤¤Î¤À¤¬¡¢OVERWRAP ¥Õ¥é¥°¤¬¤Á¤ã¤ó¤È¥â¡¼¥ÉÅù¤È¤ÎÂÐ±þ¤¬¤µ¤ì¤Æ¤¤
   ¤Ê¤¤¤è¤¦¤Ê¤Î¤Ç¡¢¤È¤ê¤¢¤¨¤º¾åµ­¤Î¤Þ¤Þ¤È¤¹¤ë¡£
				    (!yc->nbunsetsu ||
				     (yc->status & CHIKUJI_OVERWRAP)));
 */
      s += len;
      if (!OnBunsetsu) {
	*sr = ssr; *er = eer;
	if (pat && focused) {
	  pat->u.caretpos = pat->sp - pat->u.attr - (s - *sr);
	  /* ¾åµ­¤Î·×»»¤Î²òÀâ: ¥­¥ã¥ì¥Ã¥È°ÌÃÖ¤Ï¡¢º£¸å¥¢¥È¥ê¥Ó¥å¡¼¥È
	     ¤ò½ñ¤­¹þ¤à°ÌÃÖ¤«¤éÌá¤Ã¤¿°ÌÃÖ¤Ë¤¢¤ë¡£¤É¤Î¤¯¤é¤¤Ìá¤ë¤«¤È
	     ¸À¤¦¤È¡¢¼¡¤ËÊ¸»úÎó¤ò½ñ¤­¹þ¤à°ÌÃÖ¤«¤é¡¢È¿Å¾³«»Ï°ÌÃÖ¤Þ¤Ç
	     Ìá¤ëÎÌ¤À¤±Ìá¤ë */
	}
      }
    }
  }
  else if (yc->nbunsetsu) { /* Ã±¸õÊä¥â¡¼¥É */
    len = extractKanjiString(yc, s, e, b, sr, er, pat, focused);
    s += len;
  }
  else {
    len = extractSimpleYomiString(yc, s, e, sr, er, pat, focused);
    s += len;
  }
  if (s < e) {
    *s = (WCHAR_T)'\0';
  }
  return (int)(s - ss);
}

inline
int extractString(WCHAR_T *str, WCHAR_T *s, WCHAR_T *e)
{
  int len;

  len = WStrlen(str);
  if (s + len < e) {
    WStrcpy(s, str);
    return len;
  }
  else {
    WStrncpy(s, str, (int)(e - s));
    return (int)(e - s);
  }
}

/*
  extractTanString -- tanContext ¤ÎÊ¸»ú¤ò¼è¤ê½Ð¤¹

  °ú¿ô
     tan -- tanContext
     s   -- ¼è¤ê½Ð¤¹Àè¤Î¥¢¥É¥ì¥¹
     e   -- ¤³¤³¤ò±Û¤¨¤Æ¼è¤ê½Ð¤·¤Æ¤Ï¤Ê¤é¤Ê¤¤¡¢¤È¸À¤¦¥¢¥É¥ì¥¹
 */

int
extractTanString(tanContext tan, WCHAR_T *s, WCHAR_T *e)
{
  return extractString(tan->kanji, s, e);
}

/*
  extractTanYomi -- tanContext ¤ÎÊ¸»ú¤ò¼è¤ê½Ð¤¹

  °ú¿ô
     tan -- tanContext
     s   -- ¼è¤ê½Ð¤¹Àè¤Î¥¢¥É¥ì¥¹
     e   -- ¤³¤³¤ò±Û¤¨¤Æ¼è¤ê½Ð¤·¤Æ¤Ï¤Ê¤é¤Ê¤¤¡¢¤È¸À¤¦¥¢¥É¥ì¥¹
 */

int
extractTanYomi(tanContext tan, WCHAR_T *s, WCHAR_T *e)
{
  return extractString(tan->yomi, s, e);
}

/*
  extractTanRomaji -- tanContext ¤ÎÊ¸»ú¤ò¼è¤ê½Ð¤¹

  °ú¿ô
     tan -- tanContext
     s   -- ¼è¤ê½Ð¤¹Àè¤Î¥¢¥É¥ì¥¹
     e   -- ¤³¤³¤ò±Û¤¨¤Æ¼è¤ê½Ð¤·¤Æ¤Ï¤Ê¤é¤Ê¤¤¡¢¤È¸À¤¦¥¢¥É¥ì¥¹
 */

int
extractTanRomaji(tanContext tan, WCHAR_T *s, WCHAR_T *e)
{
  return extractString(tan->roma, s, e);
}

void
makeKanjiStatusReturn(uiContext d, yomiContext yc)
{
  int len;
  WCHAR_T *s = d->genbuf, *e = s + ROMEBUFSIZE, *sr, *er, *sk, *ek;
  tanContext tan = (tanContext)yc;
  long truecaret = -1;

  if (d->attr) {
    d->attr->sp = d->attr->u.attr;
    d->attr->ep = d->attr->u.attr + d->attr->len;
  }

  /* ºÇ½é¤ÏÊÑ´¹¤µ¤ì¤Æ¤¤¤ëÉôÊ¬¤ò¼è¤ê½Ð¤¹ */
  while (tan->left) {
    tan = tan->left;
  }

  while (tan) {
    if (d->attr) d->attr->u.caretpos = -1;
    switch (tan->id) {
    case TAN_CONTEXT:
      len = extractTanString(tan, s, e);
      sk = s; ek = s + len;
      if (d->attr &&
	  d->attr->sp + len < d->attr->ep) {
	char *ap = d->attr->sp, *ep = ap + len;
	char curattr =
	  ((mode_context)tan == (mode_context)yc) ?
	    CANNA_ATTR_TARGET_CONVERTED : CANNA_ATTR_CONVERTED;
	for (; ap < ep ; ap++) {
	  *ap = curattr;
	}
	d->attr->sp = ap;
      }
      break;
    case YOMI_CONTEXT:
      len = extractYomiString((yomiContext)tan, s, e, cannaconf.BunsetsuKugiri,
			      &sk, &ek, d->attr,
			      (mode_context)tan == (mode_context)yc);
      break;
    default:
      break;
    }

    if ((mode_context)tan == (mode_context)yc) {
      sr = sk;
      er = ek;
      if (d->attr) truecaret = d->attr->u.caretpos;
    }
    s += len;
    tan = tan->right;
    if (cannaconf.BunsetsuKugiri && tan && s < e) {
      *s++ = (WCHAR_T)' ';
      if (d->attr && d->attr->sp < d->attr->ep) {
	*d->attr->sp++ = CANNA_ATTR_CONVERTED;
      }
    }
  }
  
  if (s < e) {
    *s = (WCHAR_T)'\0';
  }

  d->kanji_status_return->length = (int)(s - d->genbuf);
  d->kanji_status_return->echoStr = d->genbuf;
  d->kanji_status_return->revPos = (int)(sr - d->genbuf);
  d->kanji_status_return->revLen = (int)(er - sr);
  if (d->attr) {
    d->attr->u.caretpos = truecaret;
    if (d->kanji_status_return->length < d->attr->len) {
      d->attr->u.attr[d->kanji_status_return->length] = '\0';
    }
    d->kanji_status_return->info |= KanjiAttributeInfo;
  }
}

#ifdef WIN
#define MESSBUFSIZE 128
#else
#define MESSBUFSIZE 256
#endif

/*
 * ¥ê¥Ð¡¼¥¹¤Ê¤·¤Î¥á¥Ã¥»¡¼¥¸¤ò¥¬¥¤¥É¥é¥¤¥ó¤ËÉ½¼¨¤¹¤ë
 * ¼¡¤ÎÆþÎÏ¤¬¤¢¤Ã¤¿¤È¤­¤Ë¾Ã¤¨¤ë¤è¤¦¤Ë¥Õ¥é¥°¤òÀßÄê¤¹¤ë
 */
void
makeGLineMessage(uiContext d, WCHAR_T *msg, int sz)
{
  static WCHAR_T messbuf[MESSBUFSIZE];
  int len = sz < MESSBUFSIZE ? sz : MESSBUFSIZE - 1;

  WStrncpy(messbuf, msg, len);
  messbuf[len] = (WCHAR_T)0;
  d->kanji_status_return->gline.line = messbuf;
  d->kanji_status_return->gline.length = len;
  d->kanji_status_return->gline.revPos = 0;
  d->kanji_status_return->gline.revLen = 0;
  d->kanji_status_return->info |= KanjiGLineInfo;

  d->flags &= ~PCG_RECOGNIZED;
  d->flags |= PLEASE_CLEAR_GLINE;
  checkGLineLen(d);
}

void
makeGLineMessageFromString(uiContext d, char *msg)
{
  int len;

  len = MBstowcs(d->genbuf, msg, ROMEBUFSIZE);
  makeGLineMessage(d, d->genbuf, len);
}

int setWStrings(WCHAR_T **ws, char **s, int sz)
{
  int f = sz;

  for (; (f && sz) || (!f && *s); ws++, s++, sz--) {
    *ws = WString(*s);
    if (!*ws) {
      return NG;
    }
  }
  return 0;
}

#ifdef DEBUG
dbg_msg(char *fmt, int x, int y, int z)
{
  if (iroha_debug) {
    fprintf(stderr, fmt, x, y, z);
  }
}

checkModec(uiContext d)
{
  coreContext c;
  struct callback *cb;
  int depth = 0, cbDepth = 0;
  int callbacks = 0;

  for (c = (coreContext)d->modec ; c ; c = (coreContext)c->next)
    depth++;
  for (cb = d->cb ; cb ; cb = cb->next) {
    int i;

    cbDepth++;
    for (i = 0 ; i < 4 ; i++) {
      callbacks <<= 1;
      if (cb->func[i]) {
	callbacks++;
      }
    }
  }
  if (depth != cbDepth) {
    fprintf(stderr, "¢£¢£¢£¢£¢£¡ª¡ª¡ª¿¼¤µ¤¬°ã¤¦¤¾¡ª¡ª¡ª¢£¢£¢£¢£¢£\n");
  }
  debug_message("\242\243\40\277\274\244\265: d->modec:%d d->cb:%d callbacks:0x%08x ", 
		depth, cbDepth, callbacks);
                /* ¢£ ¿¼¤µ */
  debug_message("EXIT_CALLBACK = 0x%x\n", d->cb->func[EXIT_CALLBACK],0,0);
  {
    extern KanjiModeRec yomi_mode;
    if (d->current_mode == &yomi_mode) {
      yomiContext yc = (yomiContext)d->modec;
      if (yc->kana_buffer[yc->kEndp]) {
        fprintf(stderr, "¢£¢£¢£¢£¢£ ¥«¥Ê¥Ð¥Ã¥Õ¥¡¤Ë¥´¥ß¤¬Æþ¤Ã¤Æ¤¤¤ë¤¾¡ª\n");
      }
    }
  }
}

static char pbufstr[] = " o|do?b%";

showRomeStruct(unsigned int dpy, unsigned int win)
{
  uiContext d, keyToContext();
  extern defaultContext;
  static int n = 0;
  int i;
  char buf[1024];
  
  n++;
  fprintf(stderr, "\n¡Ú¥Ç¥Ð¥°¥á¥Ã¥»¡¼¥¸(%d)¡Û\n", n);
  d = keyToContext((unsigned int)dpy, (unsigned int)win);
  fprintf(stderr, "buffer(0x%x), bytes(%d)\n",
	  d->buffer_return, d->n_buffer);
  fprintf(stderr, "nbytes(%d), ch(0x%x)\n", d->nbytes, d->ch);
  fprintf(stderr, "¥â¡¼¥É: %d\n", ((coreContext)d->modec)->minorMode);
  /* ¥³¥ó¥Æ¥¯¥¹¥È */
  fprintf(stderr, "¥³¥ó¥Æ¥¯¥¹¥È(%d)\n", d->contextCache);
  fprintf(stderr, "¥Ç¥Õ¥©¥ë¥È¥³¥ó¥Æ¥¯¥¹¥È(%d)\n", defaultContext);

  /* ¥í¡¼¥Þ»ú¤«¤Ê´ØÏ¢ */
  if (((coreContext)d->modec)->id == YOMI_CONTEXT) {
    yomiContext yc = (yomiContext)d->modec;

    fprintf(stderr, "r:       Start(%d), Cursor(%d), End(%d)\n",
	    yc->rStartp, yc->rCurs, yc->rEndp);
    fprintf(stderr, "k: Ì¤ÊÑ´¹Start(%d), Cursor(%d), End(%d)\n",
	    yc->kRStartp, yc->kCurs, yc->kEndp);
    WStrncpy(buf, yc->romaji_buffer, yc->rEndp);
    buf[yc->rEndp] = '\0';
    fprintf(stderr, "romaji_buffer(%s)\n", buf);
    fprintf(stderr, "romaji_attrib(");
    for (i = 0 ; i <= yc->rEndp ; i++) {
      fprintf(stderr, "%1x", yc->rAttr[i]);
    }
    fprintf(stderr, ")\n");
    fprintf(stderr, "romaji_pointr(");
    for (i = 0 ; i <= yc->rEndp ; i++) {
      int n = 0;
      if (i == yc->rStartp)
	n |= 1;
      if (i == yc->rCurs)
	n |= 2;
      if (i == yc->rEndp)
	n |= 4;
      fprintf(stderr, "%c", pbufstr[n]);
    }
    fprintf(stderr, ")\n");
    WStrncpy(buf, yc->kana_buffer, yc->kEndp);
    buf[yc->kEndp] = '\0';
    fprintf(stderr, "kana_buffer(%s)\n", buf);
    fprintf(stderr, "kana_attrib(");
    for (i = 0 ; i <= yc->kEndp ; i++) {
      fprintf(stderr, "%1x", yc->kAttr[i]);
    }
    fprintf(stderr, ")\n");
    fprintf(stderr, "kana_pointr(");
    for (i = 0 ; i <= yc->kEndp ; i++) {
      int n = 0;
      if (i == yc->kRStartp)
	n |= 1;
      if (i == yc->kCurs)
	n |= 2;
      if (i == yc->kEndp)
	n |= 4;
      fprintf(stderr, "%c", pbufstr[n]);
    }
    fprintf(stderr, ")\n");
    fprintf(stderr, "\n");
  }
/*  RkPrintDic(0, "kon"); */
}
#endif /* DEBUG */

extern char *jrKanjiError;

int NoMoreMemory(void)
{
  jrKanjiError = "\245\341\245\342\245\352\244\254\311\324\302\255\244\267\244\306\244\244\244\336\244\271\241\243";
                /* ¥á¥â¥ê¤¬ÉÔÂ­¤·¤Æ¤¤¤Þ¤¹¡£ */
  return NG;
}

int GLineNGReturn(uiContext d)
{
  int len;
  len = MBstowcs(d->genbuf, jrKanjiError, ROMEBUFSIZE);
  makeGLineMessage(d, d->genbuf, len);
  currentModeInfo(d);

  return(0);
}

int GLineNGReturnFI(uiContext d)
{
  popForIchiranMode(d);
  popCallback(d);
  GLineNGReturn(d);
  return(0);
}

#ifdef WIN

specialfunc(uiContext d, int fn)
{
  static WCHAR_T fnbuf[2];

  fnbuf[0] = fn;
  d->kanji_status_return->echoStr = fnbuf;
  d->kanji_status_return->length = 0;
  d->kanji_status_return->info = KanjiSpecialFuncInfo | KanjiEmptyInfo;
  return 0;
}

#endif /* WIN */

#ifndef NO_EXTEND_MENU

int GLineNGReturnTK(uiContext d)
{
  extern void popTourokuMode (uiContext);
 
  popCallback(d);
  GLineNGReturn(d);
  return(0);
}

#endif /* NO_EXTEND_MENU */

#ifdef USE_COPY_ATTRIBUTE
copyAttribute(BYTE *dest, BYTE *src, int n)
{
  if (dest > src && dest < src + n) {
    dest += n;
    src += n;
    while (n-- > 0) {
      *--dest = *--src;
    }
  }
  else {
    while (n-- > 0) {
      *dest++ = *src++;
    }
  }
}
#endif

#ifdef DEBUG_ALLOC
int fail_malloc = 0;

#undef malloc

char *
debug_malloc(int n)
{
  if (fail_malloc)
    return (char *)0;
  else
    return malloc(n);
}
#endif /* DEBUG_ALLOC */

/*
 * ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿¥ª¥Ú¥ì¡¼¥·¥ç¥ó
 *
 */

static int wchar_type; /* ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿¤Î¥¿¥¤¥×(²¼¤ò¸«¤è) */

#define CANNA_WCTYPE_16 0  /* 16¥Ó¥Ã¥ÈÉ½¸½ */
#define CANNA_WCTYPE_32 1  /* 32¥Ó¥Ã¥ÈÉ½¸½ */
#define CANNA_WCTYPE_OT 99 /* ¤½¤ÎÂ¾¤ÎÉ½¸½ */

/*
 WCinit() -- ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿¤È¤·¤Æ¤É¤ì¤¬»È¤ï¤ì¤Æ¤¤¤ë¤«¤ò³ÎÇ§¤¹¤ë

        ¤³¤Î´Ø¿ô¤¬¸Æ¤Ó½Ð¤µ¤ì¤ë¤Þ¤¨¤Ë setlocale ¤¬¤Ê¤µ¤ì¤Æ¤¤¤Ê¤±¤ì¤Ð¤Ê¤é¤Ê¤¤
 */

#define TYPE16A 0x0000a4a2
#define TYPE32A 0x30001222

int
WCinit(void)
{
#if defined(HAVE_WCHAR_OPERATION) && !defined(WIN)
  extern int locale_insufficient;
  WCHAR_T wc[24];
  char *a = "\244\242"; /* ¤¢ */ /* 0xa4a2 */

  locale_insufficient = 0;
  if (mbstowcs(wc, a, sizeof(wc) / sizeof(WCHAR_T)) != 1) {
    /* Â¿Ê¬ setlocale ¤¬¤Ê¤µ¤ì¤Æ¤¤¤Ê¤¤ */
    setlocale(LC_CTYPE, "");
    if (mbstowcs(wc, a, sizeof(wc) / sizeof(WCHAR_T)) != 1) {
      setlocale(LC_CTYPE, JAPANESE_LOCALE);
      if (mbstowcs(wc, a, sizeof(wc) / sizeof(WCHAR_T)) != 1) {
	locale_insufficient = 1;
	return -1;
      }
    }
  }
  switch (wc[0]) {
  case TYPE16A:
    wchar_type = CANNA_WCTYPE_16;
    break;
  case TYPE32A:
    wchar_type = CANNA_WCTYPE_32;
    break;
  default:
    wchar_type = CANNA_WCTYPE_OT;
    break;
  }
#else /* !HAVE_WCHAR_OPERATION || WIN */
# ifdef WCHAR16

  wchar_type = CANNA_WCTYPE_16;

# else /* !WCHAR16 */

  if (sizeof(WCHAR_T) == 2) {
    /* NOTREACHED */
    wchar_type = CANNA_WCTYPE_16;
  }
  else {
    /* NOTREACHED */
    wchar_type = CANNA_WCTYPE_32;
  }

# endif /* !WCHAR16 */
#endif /* !HAVE_WCHAR_OPERATION || WIN */

  return 0;
}


int
WStrlen(WCHAR_T *ws)
{
  int res = 0;
  while (*ws++) {
    res++;
  }
  return res;
}

WCHAR_T *
WStrcpy(WCHAR_T *ws1, WCHAR_T *ws2)
{
WCHAR_T *ws;
int len;

	ws = ws2;
	while (*ws) {
		ws++;
	}

	len =  ws - ws2;
	memmove(ws1, ws2, len * sizeof(WCHAR_T));
	ws1[len] = (WCHAR_T)0;
	return ws1;
}

WCHAR_T *
WStrncpy(WCHAR_T *ws1, WCHAR_T *ws2, int cnt)
{
	if  (!ws2){
		return((WCHAR_T *)0);
	}else{
		return (WCHAR_T *)memmove(ws1, ws2, cnt * sizeof(WCHAR_T));
	}
}

WCHAR_T *
WStraddbcpy(WCHAR_T *ws1, WCHAR_T *ws2, int cnt)
{
  WCHAR_T *strp = ws1, *endp = ws1 + cnt - 1;

  while (*ws2 != (WCHAR_T)'\0' && ws1 < endp) {
    if (*ws2 == (WCHAR_T)' ' || *ws2 == (WCHAR_T)'\t' || *ws2 == (WCHAR_T)'\\')
      *ws1++ = (WCHAR_T)'\\';
    *ws1++ = *ws2++;
  }
  if (ws1 == endp) {
    ws1--;
  }
  *ws1 = (WCHAR_T)'\0';
  return(strp);
}

WCHAR_T *
WStrcat(WCHAR_T *ws1, WCHAR_T *ws2)
{
  WCHAR_T *ws;

  ws = ws1;
  while (*ws) {
    ws++;
  }
  WStrcpy(ws, ws2);
  return ws1;
}

int
WStrcmp(WCHAR_T *w1, WCHAR_T *w2)
{
  while (*w1 && *w1 == *w2) {
    w1++;
    w2++;
  }
  return(*w1 - *w2);
}

int
WStrncmp(WCHAR_T *w1, WCHAR_T *w2, int n)
{
  if (n == 0) return(0);
  while (--n && *w1 && *w1 == *w2) {
    w1++;
    w2++;
  }
  return *w1 - *w2;
}

/* WWhatGPlain -- ¤É¤Î¥°¥é¥Õ¥£¥Ã¥¯¥×¥ì¡¼¥ó¤ÎÊ¸»ú¤«¡©

   Ìá¤êÃÍ:
     0 : G0 ASCII
     1 : G1 ´Á»ú(JISX0208)
     2 : G2 È¾³Ñ¥«¥¿¥«¥Ê(JISX0201)
     3 : G3 ³°»ú(Êä½õ´Á»ú JISX0212)
 */

int
WWhatGPlain(WCHAR_T wc)
{
  static char plain[4] = {0, 2, 3, 1};

  switch (wchar_type) {
  case CANNA_WCTYPE_16:
    switch (((unsigned long)wc) & 0x8080) {
    case 0x0000:
      return 0;
    case 0x8080:
      return 1;
    case 0x0080:
      return 2;
    case 0x8000:
      return 3;
    }
    break;
  case CANNA_WCTYPE_32:
    return plain[(((unsigned long)wc) >> 28) & 3];
  default:
    return 0; /* ¤É¤¦¤·¤è¤¦ */
  }
  /* NOTREACHED */
}

int
WIsG0(WCHAR_T wc)
{
  return (WWhatGPlain(wc) == 0);
}

int
WIsG1(WCHAR_T wc)
{
  return (WWhatGPlain(wc) == 1);
}

int
WIsG2(WCHAR_T wc)
{
  return (WWhatGPlain(wc) == 2);
}

int
WIsG3(WCHAR_T wc)
{
  return (WWhatGPlain(wc) == 3);
}

#ifndef HAVE_WCHAR_OPERATION
int
CANNA_mbstowcs(WCHAR_T *dest, char *src, int destlen)
{
  register int i, j;
  register unsigned ec;

    for (i = 0, j = 0 ;
	 (ec = (unsigned)(unsigned char)src[i]) != 0 && j < destlen ; i++) {
      if (ec & 0x80) {
	switch (ec) {
	case 0x8e: /* SS2 */
	  dest[j++] = (WCHAR_T)(0x80 | ((unsigned)src[++i] & 0x7f));
	  break;
	case 0x8f: /* SS3 */
	  dest[j++] = (WCHAR_T)(0x8000
				| (((unsigned)src[i + 1] & 0x7f) << 8)
				| ((unsigned)src[i + 2] & 0x7f));
	  i += 2;
	  break;
	default:
	  dest[j++] = (WCHAR_T)(0x8080 | (((unsigned)src[i] & 0x7f) << 8)
				| ((unsigned)src[i + 1] & 0x7f));
	  i++;
	  break;
	}
      }else{
	dest[j++] = (WCHAR_T)ec;
      }
    }
    if (j < destlen)
      dest[j] = (WCHAR_T)0;
    return j;
}

#endif /* HAVE_WCHAR_OPERATION */

int
CNvW2E(WCHAR_T *src, int srclen, char *dest, int destlen)
{
	int i, j;

//  case CANNA_WCTYPE_16:
    for (i = 0, j = 0 ; i < srclen && j + 2 < destlen ; i++) {
      WCHAR_T wc = src[i];
      switch (wc & 0x8080) {
      case 0:
	/* ASCII */
	dest[j++] = (char)((unsigned)wc & 0x7f);
	break;
      case 0x0080:
	/* È¾³Ñ¥«¥Ê */
	dest[j++] = (char)0x8e; /* SS2 */
	dest[j++] = (char)(((unsigned)wc & 0x7f) | 0x80);
	break;
      case 0x8000:
	/* ³°»ú */
	dest[j++] = (char)0x8f; /* SS3 */
	dest[j++] = (char)((((unsigned)wc & 0x7f00) >> 8) | 0x80);
	dest[j++] = (char)(((unsigned)wc & 0x7f) | 0x80);
	break;
      case 0x8080:
	/* ´Á»ú */
	dest[j++] = (char)((((unsigned)wc & 0x7f00) >> 8) | 0x80);
	dest[j++] = (char)(((unsigned)wc & 0x7f) | 0x80);
	break;
      }
    }
    dest[j] = (char)0;
    return j;
}

#ifndef HAVE_WCHAR_OPERATION
int
CANNA_wcstombs(char *dest, WCHAR_T *src, int destlen)
{
  return CNvW2E(src, WStrlen(src), dest, destlen);
}

#endif /* HAVE_WCHAR_OPERATION */


/* cfuncdef

  WString -- EUC¤«¤é¥ï¥¤¥É¥­¥ã¥é¥¯¥¿¤Ø¤Î¥Þ¥Ã¥Ô¥ó¥°¤ª¤è¤Ó malloc

  WString ¤Ï°ú¿ô¤ÎÊ¸»úÎó¤ò¥ï¥¤¥É¥­¥ã¥é¥¯¥¿¤ËÊÑ´¹¤·¡¢¤½¤ÎÊ¸»úÎó¤¬¼ý¤Þ¤ë
  ¤À¤±¤Î¥á¥â¥ê¤ò malloc ¤·¡¢¤½¤ÎÊ¸»úÎó¤òÇ¼¤áÊÖ¤¹¡£

  ÍøÍÑ¼Ô¤Ï¤³¤Î´Ø¿ô¤ÇÆÀ¤¿¥Ý¥¤¥ó¥¿¤ò free ¤¹¤ëÉ¬Í×¤Ï¤¢¤Þ¤ê¤Ê¤¤¡£

  ¤¹¤Ê¤ï¤Á¡¢¤³¤Î´Ø¿ô¤ÇÆÀ¤¿¥á¥â¥ê¤Ï¸å¤Ç WStringClose ¤ò¸Æ¤Ó½Ð¤·¤¿¤È¤­¤Ë
  free ¤µ¤ì¤ë¡£

  ¤½¤¦¤¤¤¦»ö¾ð¤Ê¤Î¤Ç¤³¤Î´Ø¿ô¤òÉÑÈË¤Ë¸Æ¤Ó½Ð¤·¤Æ¤Ï¤¤¤±¤Ê¤¤¡£º£¤Þ¤ÇEUC¤Ç
  ½é´üÄêµÁ¤Ç¤­¤Æ¤¤¤¿Ê¸»úÎó¤Ê¤É¤ËÎ±¤á¤ë¤Ù¤­¤Ç¤¢¤ë¡£

  ¤³¤Îµ¡Ç½¤ò»È¤¦¿Í¤ÏºÇ½é¤Ë WStringOpen ¤ò¸Æ¤Ó½Ð¤µ¤Ê¤±¤ì¤Ð¤Ê¤é¤Ê¤¤¤¬¡¢
  ¥æ¡¼¥¶¥¤¥ó¥¿¥Õ¥§¡¼¥¹¥é¥¤¥Ö¥é¥ê¤Ç¤Ï¥·¥¹¥Æ¥à¤¬¼«Æ°Åª¤ËÆÉ¤ó¤Ç¤¯¤ì¤ë¤Î
  ¤Ç¤½¤ÎÉ¬Í×¤Ï¤Ê¤¤¡£

 */ 

static WCHAR_T **wsmemories = (WCHAR_T **)NULL;
static int nwsmemories = 0;

#define WSBLOCKSIZE 128

int
WStringOpen(void)
{
  return 0;
}

WCHAR_T *
WString(char *s)
{
  int i, len;
  WCHAR_T *temp, **wm;

  if (wsmemories == (WCHAR_T **)NULL) {
    nwsmemories = WSBLOCKSIZE;
    if (!(wsmemories = (WCHAR_T **)calloc(nwsmemories, sizeof(WCHAR_T *))))
      return((WCHAR_T *)0) ;
    /* calloc ¤µ¤ì¤¿¥á¥â¥ê¤Ï¥¯¥ê¥¢¤µ¤ì¤Æ¤¤¤ë */
  }

  for (i = 0 ; i < nwsmemories && wsmemories[i] ;) {
    i++;
  }

  if (i == nwsmemories) { /* »È¤¤ÀÚ¤Ã¤¿¤Î¤ÇÁý¤ä¤¹ */
    if (!(wm = (WCHAR_T **)realloc(wsmemories,
				 (nwsmemories + WSBLOCKSIZE) 
				 * sizeof(WCHAR_T *))))
      return((WCHAR_T *)0);
    wsmemories = wm;
    for (; i < nwsmemories + WSBLOCKSIZE ; i++)
      wsmemories[i] = (WCHAR_T *)0;
    i = nwsmemories;
    nwsmemories += WSBLOCKSIZE;
  }

  /* ¤È¤ê¤¢¤¨¤ºÂç¤­¤¯¤È¤Ã¤Æ¤ª¤¤¤Æ¡¢¤½¤Î¥µ¥¤¥º¤ò¸«¤ÆÃúÅÙ¤Î¥µ¥¤¥º¤Ë
     Ä¾¤·¤ÆÊÖ¤¹ */

  len = strlen(s);
  if (!(temp = (WCHAR_T *)malloc((len + 1) * WCHARSIZE)))
    return((WCHAR_T *)0);
  len = MBstowcs(temp, s, len + 1);
  if (!(wsmemories[i] = (WCHAR_T *)malloc((len + 1) * WCHARSIZE))) {
    free(temp);
    return((WCHAR_T *) 0);
  }
  WStrncpy(wsmemories[i], temp, len);
  wsmemories[i][len] = (WCHAR_T)0;
  free(temp);
  return(wsmemories[i]);
}

void
WStringClose(void)
{
  int i;

  for (i = 0 ; i < nwsmemories ; i++)
    if (wsmemories[i])
      free(wsmemories[i]);
  free(wsmemories);
  wsmemories = (WCHAR_T **)0;
  nwsmemories = 0;
}

int WSfree(WCHAR_T *s)
{
  int	i;
  WCHAR_T **t;

  for (t = wsmemories, i = nwsmemories; s != *t && i;) {
    t++;
    i--;
  }
  if (s != *t)
    return(-1);
  free(*t);
  *t = (WCHAR_T *) 0;
  return(0);
}

/* 
 generalReplace -- ¥«¥Ê¥Ð¥Ã¥Õ¥¡¤Ë¤â¥í¡¼¥Þ»ú¥Ð¥Ã¥Õ¥¡¤Ë¤â»È¤¨¤ëÃÖ´¹¥ë¡¼¥Á¥ó

  ¤³¤ÎÃÖ´¹¥ë¡¼¥Á¥ó¤ÏÊ¸»úÎó¤Î¥á¥â¥ê¾å¤ÎÃÖ´¹¤ò¹Ô¤¦¤¿¤á¤Î¥é¥¤¥Ö¥é¥ê¥ë¡¼¥Á
  ¥ó¤Ç¤¢¤ë¡£¥á¥â¥ê¾å¤ËÊ¸»úÎó¤òÊÝ»ý¤·¤Æ¤ª¤¯»ÅÁÈ¤ß¤Ï¼¡¤Î¤è¤¦¤Ë¤Ê¤Ã¤Æ¤¤¤ë
  ¤â¤Î¤È¤¹¤ë¡£

    ¡¦Ê¸»úÎóÍÑ¤Î¥Ð¥Ã¥Õ¥¡
    ¡¦Ê¸»ú¤ÎÂ°À­ÍÑ¤Î¥Ð¥Ã¥Õ¥¡
    ¡¦¥«¡¼¥½¥ë(¥¤¥ó¥Ç¥Ã¥¯¥¹(¥Ý¥¤¥ó¥¿¤Ç¤Ï¤Ê¤¤))
    ¡¦Ê¸»úÎó¤Î½ª¤ï¤ê¤ò»Ø¤¹¥¤¥ó¥Ç¥Ã¥¯¥¹
    ¡¦É¬¤º¥«¡¼¥½¥ë¤è¤êº¸¤ËÂ¸ºß¤¹¤ë¥¤¥ó¥Ç¥Ã¥¯¥¹(Ì¤ÊÑ´¹Ê¸»ú¤Ø¤Î¥¤¥ó¥Ç¥Ã
      ¥¯¥¹¤Ë»È¤Ã¤¿¤ê¤¹¤ë)

  ¾åµ­¤Ë¼¨¤µ¤ì¤ë¥Ð¥Ã¥Õ¥¡¾å¤Î¥«¡¼¥½¥ë¤ÎÁ°¤«¸å¤í¤Î»ØÄê¤µ¤ì¤¿Ä¹¤µ¤ÎÊ¸»úÎó
  ¤òÊÌ¤Ë»ØÄê¤µ¤ì¤ëÊ¸»úÎó¤ÇÃÖ¤­´¹¤¨¤ë½èÍý¤ò¤¹¤ë¡£

  ¥È¡¼¥¿¥ë¤Î¥Ð¥¤¥È¿ô¤¬ÊÑ²½¤¹¤ë¾ì¹ç¤ÏÊ¸»úÎó¤Î½ª¤ï¤ê¤ò»Ø¤¹¥¤¥ó¥Ç¥Ã¥¯¥¹¤Î
  ÃÍ¤âÊÑ²½¤µ¤»¤ë¡£¤Þ¤¿¡¢¥«¡¼¥½¥ë¤ÎÁ°¤ÎÉôÊ¬¤ËÂÐ¤·¤ÆÊ¸»úÎó¤ÎÃÖ´¹¤ò¹Ô¤¦¾ì
  ¹ç¤Ë¤Ï¥«¡¼¥½¥ë¥Ý¥¸¥·¥ç¥ó¤ÎÃÍ¤âÊÑ²½¤µ¤»¤ë¡£¥«¡¼¥½¥ë¤òÊÑ²½¤µ¤»¤¿·ë²Ì¡¢
  Ì¤ÊÑ´¹Ê¸»úÅù¤Ø¤Î¥¤¥ó¥Ç¥Ã¥¯¥¹¤è¤ê¤â¾®¤µ¤¯¤Ê¤Ã¤¿¾ì¹ç¤Ë¤Ï¡¢Ì¤ÊÑ´¹Ê¸»úÅù
  ¤Ø¤Î¥¤¥ó¥Ç¥Ã¥¯¥¹¤ÎÃÍ¤ò¥«¡¼¥½¥ë¤ÎÃÍ¤Ë¹ç¤ï¤»¤Æ¾®¤µ¤¯¤¹¤ë¡£

  ¤³¤Î´Ø¿ô¤ÎºÇ½ª°ú¿ô¤Ë¤Ï¿·¤¿¤ËÁÞÆþ¤¹¤ëÊ¸»úÎó¤ÎÂ°À­¤Ë´Ø¤¹¤ë¥Ò¥ó¥È¤¬µ­½Ò 
  ¤Ç¤­¤ë¡£¿·¤¿¤ËÁÞÆþ¤µ¤ì¤ëÊ¸»úÎó¤Î³ÆÊ¸»ú¤ËÂÐ¤·¤Æ¡¢¥Ò¥ó¥È¤ÇÍ¿¤¨¤é¤ì¤¿ÃÍ
  ¼«¿È¤¬Â°À­ÃÍ¤È¤·¤Æ³ÊÇ¼¤µ¤ì¤ë¡£

  ¡Ú°ú¿ô¡Û
     buf      ¥Ð¥Ã¥Õ¥¡¤Ø¤Î¥Ý¥¤¥ó¥¿
     attr     Â°À­¥Ð¥Ã¥Õ¥¡¤Ø¤Î¥Ý¥¤¥ó¥¿
     startp   ¥Ð¥Ã¥Õ¥¡¤ÎÌ¤³ÎÄêÊ¸»úÎó¤Ê¤É¤Ø¤Î¥¤¥ó¥Ç¥Ã¥¯¥¹¤ò¼ý¤á¤Æ¤¤¤ëÊÑ
              ¿ô¤Ø¤Î¥Ý¥¤¥ó¥¿
     cursor   ¥«¡¼¥½¥ë°ÌÃÖ¤ò¼ý¤á¤Æ¤¤¤ëÊÑ¿ô¤Ø¤Î¥Ý¥¤¥ó¥¿
     endp     Ê¸»úÎó¤ÎºÇ½ª°ÌÃÖ¤ò»Ø¤·¼¨¤·¤Æ¤¤¤ëÊÑ¿ô¤Ø¤Î¥Ý¥¤¥ó¥¿

     bytes    ²¿¥Ð¥¤¥ÈÃÖ´¹¤¹¤ë¤«¡©Éé¤Î¿ô¤¬»ØÄê¤µ¤ì¤ë¤È¥«¡¼¥½¥ë¤ÎÁ°¤ÎÉô
              Ê¬¤Î |bytes| Ê¬¤ÎÊ¸»úÎó¤¬ÃÖ´¹¤ÎÂÐ¾Ý¤È¤Ê¤ê¡¢Àµ¤Î¿ô¤¬»ØÄê
              ¤µ¤ì¤ë¤È¥«¡¼¥½¥ë¤Î¸å¤í¤ÎÉôÊ¬¤Î bytes Ê¬¤ÎÊ¸»úÎó¤¬ÂÐ¾Ý¤È
              ¤Ê¤ë¡£
     rplastr  ¿·¤·¤¯ÃÖ¤¯Ê¸»úÎó¤Ø¤Î¥Ý¥¤¥ó¥¿
     len      ¿·¤·¤¯ÃÖ¤¯Ê¸»úÎó¤ÎÄ¹¤µ
     attrmask ¿·¤·¤¯ÃÖ¤¯Ê¸»úÎó¤ÎÂ°À­¤Î¥Ò¥ó¥È

  ¼ÂºÝ¤Ë¤Ï¤³¤Î´Ø¿ô¤òÄ¾ÀÜ¤Ë»È¤ï¤º¤Ë¡¢bytes, rplastr, len, attrmask ¤À¤±
  ¤òÍ¿¤¨¤ë¤À¤±¤Ç¤¹¤à¥Þ¥¯¥í¡¢kanaReplace, romajiReplace ¤ò»È¤¦¤Î¤¬ÎÉ¤¤¡£
*/

void
generalReplace(WCHAR_T *buf, BYTE *attr, int *startp, int *cursor, int *endp, int bytes, WCHAR_T *rplastr, int len, int attrmask) 
{ 
  int idou, begin, end, i; 
  int cursorMove;

  if (bytes > 0) {
    cursorMove = 0;
    begin = *cursor;
    end = *endp;
  }
  else {
    bytes = -bytes;
    cursorMove = 1;
    begin = *cursor - bytes;
    end = *endp;
  }

  idou = len - bytes;

  moveStrings(buf, attr, begin + bytes, end, idou);
  *endp += idou;
  if (cursorMove) {
    *cursor += idou;
    if (*cursor < *startp)
      *startp = *cursor;
  }

  WStrncpy(buf + begin, rplastr, len);
  for (i = 0 ; i < len ; i++) {
    attr[begin + i] = attrmask;
  }
/*  if (len)
    attr[begin] |= attrmask; */
}

int WToupper(WCHAR_T w)
{
  if ('a' <= w && w <= 'z')
    return((WCHAR_T) (w - 'a' + 'A'));
  else
    return(w);
}

int WTolower(WCHAR_T w)
{
  if ('A' <= w && w <= 'Z') {
    return (WCHAR_T)(w - 'A' + 'a');
  }
  else {
    return w;
  }
}

/*
  ¥­¡¼¤ò wchar ¤ÎÊ¸»ú¤ËÊÑ´¹¤¹¤ë¡£

  °ú¿ô:
      key         ÆþÎÏ¤µ¤ì¤¿¥­¡¼
      check       WCHAR_T ¤ËÊÑ´¹¤µ¤ì¤¿¤«¤É¤¦¤«¤ò³ÊÇ¼¤¹¤ë¤¿¤á¤ÎÊÑ¿ô¤Î¥¢¥É¥ì¥¹
  ÊÖÃÍ:
      ´Ø¿ô¤ÎÊÖÃÍ  ÊÑ´¹¤µ¤ì¤¿ WCHAR_T ¤ÎÊ¸»ú
      check       ¤¦¤Þ¤¯ÊÑ´¹¤Ç¤­¤¿¤«¤É¤¦¤«
  Ãí°Õ:
      check ¤ÏÉ¬¤ºÍ­¸ú¤ÊÊÑ¿ô¤Î¥¢¥É¥ì¥¹¤ò¥Ý¥¤¥ó¥È¤¹¤ë¤³¤È¡£
      check ¤Î¥Ý¥¤¥ó¥ÈÀè¤ÎÍ­¸úÀ­¤Ï key2wchar ¤Ç¤Ï¥Á¥§¥Ã¥¯¤·¤Ê¤¤¡£
 */

WCHAR_T
key2wchar(int key, int *check)
{
  *check = 1; /* Success as default */
  if (161 <= key && key <= 223) { /* ¥«¥¿¥«¥Ê¤ÎÈÏ°Ï¤À¤Ã¤¿¤é */
    char xxxx[4];
    WCHAR_T yyyy[4];
    int nchars;

    xxxx[0] = (char)0x8e; /* SS2 */
    xxxx[1] = (char)key;
    xxxx[2] = '\0';
    nchars = MBstowcs(yyyy, xxxx, 4);
    if (nchars != 1) {
      *check = 0;
      return 0; /* ¥¨¥é¡¼ */
    }
    return yyyy[0];
  }
  else {
    return (WCHAR_T)key;
  }
}

int
confirmContext(uiContext d, yomiContext yc)
{
  extern int defaultContext;

  if (yc->context < 0) {
    if (d->contextCache >= 0) {
      yc->context = d->contextCache;
      d->contextCache = -1;
    }
    else {
      if (defaultContext == -1) {
	if (KanjiInit() < 0 || defaultContext == -1) {
	  jrKanjiError = KanjiInitError();
	  return -1;
	}
      }
      yc->context = RkwDuplicateContext(defaultContext);
      if (yc->context < 0) {
	if (errno == EPIPE) {
	  jrKanjiPipeError();
	}
	jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\313\274\272\307\324\244\267\244\336\244\267\244\277";
                      /* ¤«¤Ê´Á»úÊÑ´¹¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
	return -1;
      }
    }
  }
  return yc->context;
}

int
abandonContext(uiContext d, yomiContext yc)
{
  extern int defaultContext;

  if (yc->context >= 0) {
    if (d->contextCache >= 0) {
      RkwCloseContext(yc->context);
    }
    else {
      d->contextCache = yc->context;
    }
    yc->context = -1;
  }
  return 0;
}

int
makeRkError(uiContext d, char *str)
{
  if (errno == EPIPE) {
    jrKanjiPipeError();
  }
  jrKanjiError = str;
  makeGLineMessageFromString(d, jrKanjiError);
  return -1;
}

/* °Ê²¼¥á¥Ã¥»¡¼¥¸¤ò gline ¤Ë½Ð¤¹¤¿¤á¤Î»ÅÁÈ¤ß */

inline
int ProcAnyKey(uiContext d)
{
  coreContext cc = (coreContext)d->modec;

  d->current_mode = cc->prevMode;
  d->modec = cc->next;
  freeCoreContext(cc);

  d->status = EXIT_CALLBACK;
  return 0;
}

static int wait_anykey_func (uiContext, KanjiMode, int, int, int);

static
int wait_anykey_func(uiContext d, KanjiMode mode, int whattodo, int key, int fnum)
/* ARGSUSED */
{
  switch (whattodo) {
  case KEY_CALL:
    return ProcAnyKey(d);
  case KEY_CHECK:
    return 1;
  case KEY_SET:
    return 0;
  }
  /* NOTREACHED */
}

static KanjiModeRec canna_message_mode = {
  wait_anykey_func,
  0, 0, 0,
};

inline void
cannaMessageMode(uiContext d, canna_callback_t cnt)
{
  coreContext cc;
  extern coreContext newCoreContext (void);


  cc = newCoreContext();
  if (cc == 0) {
    NothingChangedWithBeep(d);
    return;
  }
  cc->prevMode = d->current_mode;
  cc->next = d->modec;
  cc->majorMode = d->majorMode;
  cc->minorMode = d->minorMode;
  if (pushCallback(d, d->modec, NO_CALLBACK, cnt,
                     NO_CALLBACK, NO_CALLBACK) == (struct callback *)0) {
    freeCoreContext(cc);
    NothingChangedWithBeep(d);
    return;
  }
  d->modec = (mode_context)cc;
  d->current_mode = &canna_message_mode;
  return;
}

/*
  canna_alert(d, message, cnt) -- ¥á¥Ã¥»¡¼¥¸¤ò gline ¤Ë½Ð¤¹

  ²¿¤«¥­¡¼¤¬ÆþÎÏ¤µ¤ì¤¿¤é cnt ¤È¸À¤¦´Ø¿ô¤ò¸Æ¤Ó½Ð¤¹¡£

  °ú¿ô:
    d        UI Context
    message  ¥á¥Ã¥»¡¼¥¸
    cnt      ¼¡¤Î½èÍý¤ò¹Ô¤¦´Ø¿ô

  cnt ¤Ç¤Ï popCallback(d) ¤ò¤ä¤é¤Ê¤±¤ì¤Ð¤Ê¤é¤Ê¤¤¤³¤È¤ËÃí°Õ¡ª 

 */

int canna_alert(uiContext d, char *message, canna_callback_t cnt)
{
  d->nbytes = 0;

  makeGLineMessageFromString(d, message);
  cannaMessageMode(d, cnt);
  return 0;
}

char *
KanjiInitError(void)
{
  extern int standalone;

  if (standalone) {
    return "\244\253\244\312\264\301\273\372\312\321\264\271\244\307\244\255\244\336\244\273\244\363";
    /* "¤«¤Ê´Á»úÊÑ´¹¤Ç¤­¤Þ¤»¤ó" */
  }
  else {
    return "\244\253\244\312\264\301\273\372\312\321\264\271\245\265"
      "\241\274\245\320\244\310\304\314\277\256\244\307\244\255\244\336"
	"\244\273\244\363";              
    /* "¤«¤Ê´Á»úÊÑ´¹¥µ¡¼¥Ð¤ÈÄÌ¿®¤Ç¤­¤Þ¤»¤ó" */
  }
}

