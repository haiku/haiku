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


#if !defined(lint) && !defined(__CODECENTER__)
static char rcs_id[] = "$Id: chikuji.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif

#include	"canna.h"
#include "RK.h"
#include "RKintern.h"

#ifdef MEASURE_TIME
#include	<sys/types.h>
#ifdef WIN
#include	<sys/timeb.h>
#else
/* If you compile with Visual C++, then please comment out the next line. */
#include <sys/times.h>
#endif
#endif /* MEASURE_TIME */


static int restoreChikujiYomi(uiContext d, int old);
static int doesSupportChikuji(void);
static int chikujiSubstYomi(uiContext d);
static int chikuji_restore_yomi(uiContext d);
static int chikuji_subst_yomi(uiContext d);
static int ChikujiTanExtend(uiContext d);
static int ChikujiTanShrink(uiContext d);
static int ChikujiYomiDeletePrevious(uiContext d);
static int ChikujiHenkan(uiContext d);
static int generalNaive(uiContext d, int (*fn )(...));
static int ChikujiHenkanNaive(uiContext d);
static int ChikujiHenkanOrNothing(uiContext d);
static int ChikujiMuhenkan(uiContext d);

extern int yomiInfoLevel, nKouhoBunsetsu, KeepCursorPosition;
extern int defaultContext;
extern KanjiModeRec tankouho_mode, cy_mode, cb_mode;
extern void makeYomiReturnStruct (uiContext);
extern int RkwGetServerVersion (int *, int *);
extern int RkwGetProtocolVersion (int *, int *);

int forceRomajiFlushYomi (uiContext);
void moveToChikujiTanMode (uiContext);
void moveToChikujiYomiMode (uiContext);


inline void
clearHenkanContent(yomiContext yc)
{
  yc->allkouho = 0;
  yc->kouhoCount = yc->curIkouho = 0;
  return;
}

void
clearHenkanContext(yomiContext yc)
{
  if (yc->context >= 0) {
    RkwCloseContext(yc->context);
    yc->context = -1;
  }
  yc->nbunsetsu = yc->curbun = 0;
  clearHenkanContent(yc);
  return;
}

extern int NothingChanged (uiContext);

/*
  restoreChikujiYomi

  °ú¿ô:
    d   : uiContext
    old : Á°¤ÎÊ¸Àá¿ô

  ¤ä¤ë¤³¤È:
   (1) Ê¸Àá¿ô¤¬Áý¤¨¤¿¤éµ¬Äê¿ô°Ê¾å¤ÎÊ¸Àá¤ò³ÎÄê¤µ¤»¤ë¡£
   (2) ¤½¤Î¤È¤­¡¢RkwRemoveBun ¤â¤¹¤ë¡£
   (3) ¤½¤ÎÊ¬¡¢ÆÉ¤ß¤âºï¤ë¡£
   (4) Ê¸Àá¤Ë¤Ê¤Ã¤¿Ê¬¤ÎÆÉ¤ß¤ò¥«¥¦¥ó¥È¡£
 */

static int
restoreChikujiYomi(uiContext d, int old)
{
  yomiContext yc = (yomiContext)d->modec;
  WCHAR_T *s = d->buffer_return, *e = s + d->n_buffer;
  RkStat stat;
  int len, i, j, yomilen, ll = 0, m = 0, n = 0, recalc = 0;

  d->nbytes = 0;
  yomilen = yc->kEndp - yc->cStartp;
  if (yc->nbunsetsu) {
    yc->status |= CHIKUJI_ON_BUNSETSU;
    if (yc->nbunsetsu > old) {
      recalc = 1;
    }
    if (nKouhoBunsetsu) {
      (void)cutOffLeftSide(d, yc, nKouhoBunsetsu - yc->nbunsetsu);
      if (nKouhoBunsetsu < yc->nbunsetsu) {
	n = yc->nbunsetsu - nKouhoBunsetsu;
	if (n > old) {
	  n = old; /* Á°¤Ë¤Þ¤À´Á»ú¤Ë¤Ê¤Ã¤Æ¤¤¤Ê¤«¤Ã¤¿Ê¬¤Þ¤Ç¤Ï³ÎÄê¤µ¤»¤Ê¤¤ */
	}
      }
    }
    if (n > 0) { /* ³ÎÄê¤µ¤»¤ëÊ¸Àá¿ô */

      recalc = 1;
      for (i = 0 ; i < n ; i++) {
	if (RkwGoTo(yc->context, i) < 0 ||
	    (len = RkwGetKanji(yc->context, s, (int)(e - s))) < 0 ||
	    RkwGetStat(yc->context, &stat) == -1) {
	  return -1;
	}
	s += len;

	ll += stat.ylen;
	m += stat.klen;
      }
      d->nbytes = s - d->buffer_return;
      if (s < e) {
	*s++ = (WCHAR_T)'\0';
      }

      if (RkwRemoveBun(yc->context, cannaconf.Gakushu ? 1 : 0) == -1) {
	return -1;
      }

      /* ¤«¤Ê¥Ð¥Ã¥Õ¥¡¤È¤«¤âºï¤ë */
      kPos2rPos(yc, 0, ll, (int *)0, &j);

      if (yomiInfoLevel > 0) {
	d->kanji_status_return->info |= KanjiYomiInfo;
	len = xString(yc->kana_buffer, ll, s, e);
	s += len;
	if (s < e) {
	  *s++ = (WCHAR_T)'\0';
	}
	if (yomiInfoLevel > 1) {
	  len = xString(yc->romaji_buffer, j, s, e);
	  s += len;
	}
	if (s < e) {
	  *s++ = (WCHAR_T)'\0';
	}
      }
      
      removeKana(d, yc, ll, j);

      yc->nbunsetsu -= n;
    }
    if (RkwGoTo(yc->context, yc->nbunsetsu - 1) == -1)
      return(-1);
    yc->curbun = yc->nbunsetsu - 1;
    if (old < yc->curbun) { /* ¤»¤á¤ÆÁ°¤Î¤ä¤Ä¤Î±¦¤Ë¹Ô¤¯ */
      yc->curbun = old;
    }
  }

  if (recalc) {
    yomilen = RkwGetLastYomi(yc->context, d->genbuf, ROMEBUFSIZE);
    if (yomilen == -1) {
      return -1;
    }
    if (yomilen < yc->kEndp) { /* É¬¤º¿¿¤Ç¤Ï¡© */
      kPos2rPos(yc, 0, yc->kEndp - yomilen, (int *)0, &j);
      yc->cStartp = yc->kEndp - yomilen;
      yc->cRStartp = j;
    }
    yc->ys = yc->ye = yc->kEndp;
  }

  if (yc->nbunsetsu) {
    moveToChikujiTanMode(d);
  }
  return(0);
}

static int
doesSupportChikuji(void)
{
  int a, b;

  if (defaultContext == -1) {
    if (KanjiInit() < 0 || defaultContext == -1) {
      jrKanjiError = KanjiInitError();
      return(-1);
    }
  }
  RkwGetProtocolVersion(&a, &b);
  return(a > 1);
}

#ifndef NO_EXTEND_MENU
int
chikujiInit(uiContext d)
{
  int chikuji_f;
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    

  d->status = 0;
  killmenu(d);

  chikuji_f = doesSupportChikuji();

  if (ToggleChikuji(d, 1) == -1) {
    if(!chikuji_f)
      jrKanjiError = "\245\265\241\274\245\320\244\254\303\340\274\241\274\253"
	"\306\260\312\321\264\271\244\362\245\265\245\335\241\274\245\310"
	"\244\267\244\306\244\244\244\336\244\273\244\363";
                     /* ¥µ¡¼¥Ð¤¬Ãà¼¡¼«Æ°ÊÑ´¹¤ò¥µ¥Ý¡¼¥È¤·¤Æ¤¤¤Þ¤»¤ó */
    else
      jrKanjiError = "\303\340\274\241\274\253\306\260\312\321\264\271\244\313"
	"\300\332\302\330\244\250\244\353\244\263\244\310\244\254\244\307"
	"\244\255\244\336\244\273\244\363";    
                     /* Ãà¼¡¼«Æ°ÊÑ´¹¤ËÀÚÂØ¤¨¤ë¤³¤È¤¬¤Ç¤­¤Þ¤»¤ó */
    makeGLineMessageFromString(d, jrKanjiError);
    currentModeInfo(d);
    return(-1);
  }
  else {
    if(!chikuji_f)
      makeGLineMessageFromString(d, "\245\265\241\274\245\320\244\254\303\340"
	"\274\241\274\253\306\260\312\321\264\271\244\362\245\265\245\335"
	"\241\274\245\310\244\267\244\306\244\244\244\336\244\273\244\363");
                /* ¥µ¡¼¥Ð¤¬Ãà¼¡¼«Æ°ÊÑ´¹¤ò¥µ¥Ý¡¼¥È¤·¤Æ¤¤¤Þ¤»¤ó */
    else
      makeGLineMessageFromString(d, "\303\340\274\241\274\253\306\260\312\321"
	"\264\271\244\313\300\332\302\330\244\250\244\336\244\267\244\277");
                /* Ãà¼¡¼«Æ°ÊÑ´¹¤ËÀÚÂØ¤¨¤Þ¤·¤¿ */
    currentModeInfo(d);
    return 0;
  }
}
#endif /* not NO_EXTEND_MENU */

static int
chikujiSubstYomi(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int n = yc->nbunsetsu, ret;

  if (yc->context == -1) { /* ¡Ö½Ð¤À¤·¤À¤Ã¤¿¤é¡×¤Î°ÕÌ£¤â¤¢¤ë */
    if (confirmContext(d, yc) < 0) {
      return -1;
    }
    if (!doesSupportChikuji()) {
      jrKanjiError = "\245\265\241\274\245\320\244\254\303\340\274\241\274\253"
	"\306\260\312\321\264\271\244\362\245\265\245\335\241\274\245\310"
	"\244\267\244\306\244\244\244\336\244\273\244\363";
                     /* ¥µ¡¼¥Ð¤¬Ãà¼¡¼«Æ°ÊÑ´¹¤ò¥µ¥Ý¡¼¥È¤·¤Æ¤¤¤Þ¤»¤ó */     
      abandonContext(d, yc);
      return(-1);
    }
    if (RkwBgnBun(yc->context, (WCHAR_T *)0, 1,
                    RK_XFER << RK_XFERBITS | RK_KFER) == NG) {
    substError:
      jrKanjiError = "\303\340\274\241\274\253\306\260\312\321\264\271\244\313"
	"\274\272\307\324\244\267\244\336\244\267\244\277";
                     /* Ãà¼¡¼«Æ°ÊÑ´¹¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
      /* °Ê²¼¤Ï²¿¤ò¤ä¤Ã¤Æ¤¤¤ë¤Î¤«¤·¤é¡© */
      if (TanMuhenkan(d) == -1) {
	return -2;
      }
      return(-1);
    }
  }
  yc->nbunsetsu = RkwSubstYomi(yc->context,
			       yc->ys - yc->cStartp, yc->ye - yc->cStartp,
			       yc->kana_buffer + yc->ys, yc->kEndp - yc->ys);
  yc->ys = yc->ye = yc->kEndp;
  if (yc->nbunsetsu < 0 || (ret = restoreChikujiYomi(d, n)) < 0) {
    goto substError;
  }
  return(ret);
}

int
ChikujiSubstYomi(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if ((yc->ys == yc->ye && yc->kEndp == yc->ye)
      || yc->kEndp != yc->kCurs
      || !(yc->kAttr[yc->kEndp - 1] & HENKANSUMI)) {
    /* ¿·¤·¤¤ÆþÎÏ¤¬¤Ê¤«¤Ã¤¿¤ê¡¢
       ºÇ¸å¤Ø¤ÎÆþÎÏ¤¸¤ã¤Ê¤«¤Ã¤¿¤ê¡¢
       ºÇ¸å¤Î»ú¤¬¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¤µ¤ì¤Ä¤¯¤·¤Æ¤¤¤Ê¤«¤Ã¤¿¤ê¤·¤¿¤é */
    return 0; /* ¤Ê¤Ë¤â¤·¤Ê¤¤ */
  }
  return chikujiSubstYomi(d);
}

int
ChikujiTanDeletePrevious(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int i, j, l = 0, flg = 0;
  RkStat stat;

  if (yc->cRStartp < yc->rEndp) { /* ¥í¡¼¥Þ»ú¤¬Æþ¤Ã¤Æ¤¤¤ë */
    flg = 1;
  }
  d->nbytes = 0;

  /* ¤Þ¤º¥«¥ì¥ó¥ÈÊ¸Àá¤«¤é¸å¤í¤òÆÉ¤ß¤ËÌá¤¹ */
  if (forceRomajiFlushYomi(d)) {
    return d->nbytes;
  }
  if (RkwSubstYomi(yc->context, 0, yc->ye - yc->cStartp,
                                    (WCHAR_T *)0, 0) == NG) {
    /* ÆÉ¤ß¤Ç»Ä¤Ã¤Æ¤¤¤ëÊ¬¤òÃà¼¡¤Î¥Ç¡¼¥¿¤«¤é¾Ã¤¹ */
    (void)makeRkError(d, "\306\311\244\337\244\313\314\341\244\271\244\263"
	"\244\310\244\254\244\307\244\255\244\336\244\273\244\363");
                         /* ÆÉ¤ß¤ËÌá¤¹¤³¤È¤¬¤Ç¤­¤Þ¤»¤ó */
    (void)TanMuhenkan(d); /* ¤³¤ìÍ×¤ë¤Î¡© */
    return 0;
  }
  yc->ys = yc->ye = yc->cStartp;
  for (i = yc->nbunsetsu - 1 ; i >= yc->curbun ; i--) {
    /* ¥«¥ì¥ó¥ÈÊ¸Àá¤«¤é¸å¤í¤òÆÉ¤ß¤ËÌá¤¹¤¿¤á¤Î½àÈ÷ */
    if (RkwGoTo(yc->context, i) == NG
	|| RkwGetStat(yc->context, &stat) == NG
	|| RkwStoreYomi(yc->context, (WCHAR_T *)0, 0) == NG) {
      (void)makeRkError(d, "\306\311\244\337\244\313\314\341\244\271\244\263"
	"\244\310\244\254\244\307\244\255\244\336\244\273\244\363");
                           /* ÆÉ¤ß¤ËÌá¤¹¤³¤È¤¬¤Ç¤­¤Þ¤»¤ó */
      TanMuhenkan(d); /* ¤³¤ì¡¢Í×¤ë¤Î¡© */
      return 0;
    }
    l += stat.ylen;
  }
  yc->nbunsetsu = yc->curbun;
  if (l) {
    i = j = 0;
    do {
      ++i;
      if (yc->kAttr[yc->cStartp - i] & SENTOU) {
	for (j++ ;
	     j < yc->cRStartp && !(yc->rAttr[yc->cRStartp - j] & SENTOU) ;) {
	  j++;
	}
      }
    } while (i < l);
    yc->cStartp = (i < yc->cStartp) ? yc->cStartp - i : 0;
    yc->cRStartp = (j < yc->cRStartp) ? yc->cRStartp - j : 0;
  }
  if (KeepCursorPosition && yc->kCurs != yc->kEndp) {
    yc->kRStartp = yc->kCurs = yc->cStartp;
    yc->rStartp = yc->rCurs = yc->cRStartp;
  }
  else {
    yc->kRStartp = yc->kCurs = yc->kEndp;
    yc->rStartp = yc->rCurs = yc->rEndp;
  }
  yc->ys = yc->ye = yc->cStartp;
  clearHenkanContent(yc);
  if (yc->curbun) {
    yc->curbun--;
  }
  yc->status |= CHIKUJI_OVERWRAP;
  moveToChikujiYomiMode(d);
  makeKanjiStatusReturn(d, yc);
  if (flg && cannaconf.chikujiRealBackspace && !KeepCursorPosition) {
    d->more.todo = 1;
    d->more.ch = 0;
    d->more.fnum = CANNA_FN_DeletePrevious;
    /* Æ±¤¸¤³¤È¤ò¤Þ¤¿¤ä¤ë¤Ã¤Æ´¶¤¸¤À¤¾ */
  }
  return 0;
}

/*
  chikuji_restore_yomi

    cStartp, cRStartp ¤òÄ´À°¤¹¤ë
 */

static int
chikuji_restore_yomi(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int l, j;

  if ((l = RkwGetLastYomi(yc->context, d->genbuf, ROMEBUFSIZE)) == -1) {
    return makeRkError(d, "\314\244\267\350\312\270\300\341\244\362\274\350"
	"\244\352\275\320\244\273\244\336\244\273\244\363\244\307\244\267"
	"\244\277");
                          /* Ì¤·èÊ¸Àá¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿ */
  }
  if (l != yc->kEndp - yc->cStartp) { /* ÊÑ¤ï¤Ã¤¿¤é */
    kPos2rPos(yc, 0, yc->kEndp - l, (int *)0, &j);
    yc->cStartp = yc->kEndp - l;
    yc->cRStartp = j;
  }

  yc->ys = yc->ye = yc->cStartp;
  return 0;
}

static int
chikuji_subst_yomi(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int l, n = yc->nbunsetsu;

  /* ÆÉ¤ß¤òÁ´Éô¿©¤ï¤»¤ë */
  l = RkwSubstYomi(yc->context, yc->ys - yc->cStartp, yc->ye - yc->cStartp,
		   yc->kana_buffer + yc->ys, yc->kEndp - yc->ys);
  yc->ys = yc->ye = yc->kEndp;
  if (l == -1) {
    jrKanjiError = "\312\321\264\271\244\313\274\272\307\324\244\267\244\336"
	"\244\267\244\277";
                   /* ÊÑ´¹¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    (void)TanMuhenkan(d);
    return -1;
  }
  yc->nbunsetsu = l;
  if (l > n) {
    yc->curbun = n; /* Á°¤ÎÊ¸Àá¤¬¥«¥ì¥ó¥È */
  }
  return chikuji_restore_yomi(d);
}


static int
ChikujiTanExtend(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int i;

  d->nbytes = 0;
  yc->kouhoCount = 0;

  if (yc->ys < yc->kEndp || yc->ye != yc->kEndp) {
    i = yc->curbun; /* ¤È¤Ã¤È¤¯ */
    if (chikuji_subst_yomi(d) == -1) {
      makeGLineMessageFromString(d, jrKanjiError);
      return TanMuhenkan(d);
    }
    if (RkwGoTo(yc->context, i) == -1) {
      (void)makeRkError(d, "\312\270\300\341\244\316\260\334\306\260\244\313"
	"\274\272\307\324\244\267\244\336\244\267\244\277");
                           /* Ê¸Àá¤Î°ÜÆ°¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
      return TanMuhenkan(d);
    }
    yc->curbun = i;
  }
  if ((yc->nbunsetsu = RkwEnlarge(yc->context)) <= 0) {
    (void)makeRkError(d, "\312\270\300\341\244\316\263\310\302\347\244\313"
	"\274\272\307\324\244\267\244\336\244\267\244\277");
                         /* Ê¸Àá¤Î³ÈÂç¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    return TanMuhenkan(d);
  }
  if (chikuji_restore_yomi(d) == NG) {
    return TanMuhenkan(d);
  }
  yc->status |= CHIKUJI_OVERWRAP;
  makeKanjiStatusReturn(d, yc);
  return d->nbytes;
}


static int
ChikujiTanShrink(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  RkStat stat;
  int i;

  d->nbytes = 0;
  yc->kouhoCount = 0;
  if (yc->ys < yc->kEndp || yc->ye != yc->kEndp) {
    i = yc->curbun;
    if (chikuji_subst_yomi(d) == -1) {
      makeGLineMessageFromString(d, jrKanjiError);
      return TanMuhenkan(d);
    }
    if (RkwGoTo(yc->context, i) == -1) {
      (void)makeRkError(d, "\312\270\300\341\244\316\275\314\276\256\244\313"
	"\274\272\307\324\244\267\244\336\244\267\244\277");
                           /* Ê¸Àá¤Î½Ì¾®¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
      return TanMuhenkan(d);
    }
    yc->curbun = i;
  }

  if (RkwGetStat(yc->context, &stat) < 0 || stat.ylen == 1) {
    /* ¤³¤ì°Ê¾åÃ»¤¯¤Ç¤­¤ë¤«¤É¤¦¤«³ÎÇ§¡£Í×¤ë¡© */
    return NothingForGLine(d);
  }
  yc->nbunsetsu = RkwShorten(yc->context);
  if (yc->nbunsetsu <= 0) { /* 0 ¤Ã¤Æ¤³¤È¤¢¤ó¤Î¤«¤Ê¤¢¡© */
    (void)makeRkError(d, "\312\270\300\341\244\316\275\314\276\256\244\313"
	"\274\272\307\324\244\267\244\336\244\267\244\277");
                         /* Ê¸Àá¤Î½Ì¾®¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    return TanMuhenkan(d);
  }
  if (chikuji_restore_yomi(d) == NG) {
    return TanMuhenkan(d);
  }
  yc->status |= CHIKUJI_OVERWRAP;
  makeKanjiStatusReturn(d, yc);
  return d->nbytes;
}


static int
ChikujiYomiDeletePrevious(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  RkStat stat;
  int i, j, l = 0;

  d->nbytes = 0;
  if (!(yc->cStartp < yc->kCurs)) { /* ÆÉ¤ß¤¬¤Ê¤¤¤Ê¤é */
    if (!yc->nbunsetsu) { /* Ê¸Àá¤â¤Ê¤¤ */
      return NothingChanged(d);
    }
    else {
      if (RkwSubstYomi(yc->context, 0, yc->ye - yc->cStartp,
                                        (WCHAR_T *)0, 0) == NG) {
	(void)makeRkError(d, "\306\311\244\337\244\313\314\341\244\271\244\263"
	"\244\310\244\254\244\307\244\255\244\336\244\273\244\363");
                             /* ÆÉ¤ß¤ËÌá¤¹¤³¤È¤¬¤Ç¤­¤Þ¤»¤ó */
	(void)TanMuhenkan(d);
	return 0;
      }
      yc->ys = yc->ye = yc->cStartp;
      yc->curbun = yc->nbunsetsu - 1; /* ¤Ò¤È¤ÄÆÉ¤ß¤ËÌá¤¹ */
      for (i = yc->nbunsetsu - 1; i >= yc->curbun; i--) {
	if (RkwGoTo(yc->context, i) == NG ||
	    RkwGetStat(yc->context, &stat) == NG ||
	    RkwStoreYomi(yc->context, (WCHAR_T *)0, 0) == NG) {
	  return makeRkError(d, "\306\311\244\337\244\313\314\341\244\271"
	"\244\263\244\310\244\254\244\307\244\255\244\336\244\273\244\363");
                                /* ÆÉ¤ß¤ËÌá¤¹¤³¤È¤¬¤Ç¤­¤Þ¤»¤ó */
	}
	l += stat.ylen;
	yc->nbunsetsu--;
      }
      i = j = 0;
      do {
	++i;
	if (yc->kAttr[yc->cStartp - i] & SENTOU) {
	  for (j++ ;
	       j < yc->cRStartp && !(yc->rAttr[yc->cRStartp - j] & SENTOU) ;) {
	    j++;
	  }
	}
      } while (i < l);
      yc->kCurs = yc->kRStartp = yc->cStartp;
      yc->rCurs = yc->rStartp = yc->cRStartp;
      yc->cStartp = (i < yc->cStartp) ? yc->cStartp - i : 0;
      yc->cRStartp = (j < yc->cRStartp) ? yc->cRStartp - j : 0;
      yc->ys = yc->ye = yc->cStartp;
      clearHenkanContent(yc);
      if (yc->curbun) {
	yc->curbun--;
      }

      makeKanjiStatusReturn(d, yc);
      return 0;
    }
  }
  if (yc->kCurs - 1 < yc->ys) {
    yc->ys = yc->kCurs - 1;
  }
  if (yc->ys < 0) {
    yc->ys = 0;
  }
  KanaDeletePrevious(d);
  yc->status |= CHIKUJI_OVERWRAP;
  if (yc->kCurs <= yc->cStartp && yc->kEndp <= yc->cStartp && yc->nbunsetsu) {
    /* Ê¸Àá¤Ï¤¢¤ë¤Î¤ËÆÉ¤ß¤¬¤Ê¤¤¤Ê¤é */
    if (RkwGoTo(yc->context, yc->nbunsetsu - 1) == -1) {
      return makeRkError(d, "\312\270\300\341\244\316\260\334\306\260\244\313"
	"\274\272\307\324\244\267\244\336\244\267\244\277");
                            /* Ê¸Àá¤Î°ÜÆ°¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    }
    yc->kouhoCount = 0;
    yc->curbun = yc->nbunsetsu - 1;
    moveToChikujiTanMode(d);
    makeKanjiStatusReturn(d, yc);
  }
  else {
    moveToChikujiYomiMode(d);
    makeYomiReturnStruct(d);
    if (yc->kEndp <= yc->cStartp && !yc->nbunsetsu) {
      /* ²¿¤Ë¤â¤Ê¤¤ */
      d->current_mode = yc->curMode = yc->myEmptyMode;
      d->kanji_status_return->info |= KanjiEmptyInfo;
    }
  }
  return 0;
}


static int
ChikujiHenkan(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int n, tmp, idx;

  if (!yc->nbunsetsu && yc->rEndp == /* yc->cRStartp(== 0) + */ 1 &&
      (yc->kAttr[0] & SUPKEY) &&
      (idx = findSup(yc->romaji_buffer[0]))) {
    return selectKeysup(d, yc, idx - 1);
  }
  if (!doesSupportChikuji()) {
    jrKanjiError = "\245\265\241\274\245\320\244\254\303\340\274\241\274\253"
	"\306\260\312\321\264\271\244\362\245\265\245\335\241\274\245\310"
	"\244\267\244\306\244\244\244\336\244\273\244\363";
                   /* ¥µ¡¼¥Ð¤¬Ãà¼¡¼«Æ°ÊÑ´¹¤ò¥µ¥Ý¡¼¥È¤·¤Æ¤¤¤Þ¤»¤ó */     
    makeGLineMessageFromString(d, jrKanjiError);
    makeKanjiStatusReturn(d, yc);
    d->nbytes = 0;
    return 0;
  }
  if (yc->status & CHIKUJI_ON_BUNSETSU) {
    tmp = yc->curbun;
  }
  else {
    tmp = yc->nbunsetsu;
  }
  d->nbytes = 0;
  if (yc->kCurs < yc->ys) {
    yc->ys = yc->kCurs;
  }
  if (forceRomajiFlushYomi(d)) {
    return d->nbytes;
  }

  if (containUnconvertedKey(yc)) {
    if (yc->cmark < yc->cStartp) {
      yc->cmark = yc->cStartp;
    }
    YomiMark(d);
    yc->ys = yc->pmark;
    if (forceRomajiFlushYomi(d)) {
      return d->nbytes;
    }
  }

  yc->kRStartp = yc->kCurs = yc->kEndp;
  yc->rStartp  = yc->rCurs = yc->rEndp;

  if (yc->cStartp < yc->kEndp) { /* ÆÉ¤ß¤¬¤¢¤ì¤Ð */
    yc->kCurs = yc->kEndp;
    if (chikujiSubstYomi(d) < 0) {
      makeGLineMessageFromString(d, jrKanjiError);
      TanMuhenkan(d);
      return 0;
    }
    n = RkwFlushYomi(yc->context);
    if (n == -1) {
      (void)makeRkError(d, "\312\321\264\271\244\313\274\272\307\324\244\267"
	"\244\336\244\267\244\277");
                           /* ÊÑ´¹¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
      (void)TanMuhenkan(d);
      return 0;
    }
    yc->cStartp = yc->kEndp;
    yc->cRStartp = yc->rEndp;
    yc->kouhoCount = 1;
    yc->status |= CHIKUJI_ON_BUNSETSU;
    if (n > yc->nbunsetsu) {
      yc->curbun = yc->nbunsetsu;
      yc->nbunsetsu = n;
    }
  }
  if (RkwGoTo(yc->context, tmp) == -1) {
    makeRkError(d, "\303\340\274\241\312\321\264\271\244\313\274\272\307\324"
	"\244\267\244\336\244\267\244\277");
                   /* Ãà¼¡ÊÑ´¹¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    return 0;
  }
  yc->curbun = tmp;
  yc->ys = yc->ye = yc->cStartp;
  d->current_mode = yc->curMode = &tankouho_mode;
  yc->minorMode = CANNA_MODE_TankouhoMode;
  if (cannaconf.kouho_threshold > 0 &&
      yc->kouhoCount >= cannaconf.kouho_threshold) {
    return tanKouhoIchiran(d, 0);
  }
  currentModeInfo(d);
  makeKanjiStatusReturn(d, yc);
  return d->nbytes;
}

void
moveToChikujiTanMode(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  yc->status |= CHIKUJI_ON_BUNSETSU;
  yc->minorMode = CANNA_MODE_ChikujiTanMode;
  d->current_mode = yc->curMode = &cb_mode;
  currentModeInfo(d);
}

void
moveToChikujiYomiMode(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  yc->status &= ~CHIKUJI_ON_BUNSETSU;
  d->current_mode = yc->curMode = &cy_mode;
  EmptyBaseModeInfo(d, yc);
}

static int
generalNaive(uiContext d, int (*fn )(...))
{
  if ((((yomiContext)d->modec)->generalFlags) &
      (CANNA_YOMI_HANKAKU | CANNA_YOMI_ROMAJI | CANNA_YOMI_BASE_HANKAKU)) {
    return (*fn)(d);
  }
  else {
    return ChikujiHenkan(d);
  }
}

static int
ChikujiHenkanNaive(uiContext d)
{
  return generalNaive(d, (int(*)(...))YomiInsert);
}


static int
ChikujiHenkanOrNothing(uiContext d)
{
  return generalNaive(d, (int(*)(...))NothingChanged);
}


static int
ChikujiMuhenkan(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->nbunsetsu) {
    return TanMuhenkan(d);
  }
  else if (yc->left || yc->right) {
    removeCurrentBunsetsu(d, (tanContext)yc);
    yc = (yomiContext)d->modec;
  }
  else {
    RomajiClearYomi(d);
    d->current_mode = yc->curMode = yc->myEmptyMode;
    d->kanji_status_return->info |= KanjiEmptyInfo;
  }
  makeKanjiStatusReturn(d, yc);
  return 0;
}

#include "chikujimap.h"
