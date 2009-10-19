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
static char rcs_id[] = "@(#) 102.1 $Id: bunsetsu.c 10525 2004-12-23 21:23:50Z korli $";
#endif	/* lint */

#include <errno.h>
#include "canna.h"
#include "RK.h"
#include "RKintern.h"

extern int BunsetsuKugiri;

static char *e_message[] = {
#ifdef WIN
  /* 0*/"\312\270\300\341\244\316\260\334\306\260\244\313\274\272\307\324\244\267\244\336\244\267\244\277",
  /* 1*/"\264\301\273\372\244\316\306\311\244\337\244\362\274\350\244\352\275\320\244\273\244\336\244\273\244\363\244\307\244\267\244\277",
  /* 2*/"\312\270\300\341\244\316\260\334\306\260\244\313\274\272\307\324\244\267\244\336\244\267\244\277",
  /* 3*/"\264\301\273\372\244\316\306\311\244\337\244\362\274\350\244\352\275\320\244\273\244\336\244\273\244\363\244\307\244\267\244\277",
  /* 4*/"\244\253\244\312\264\301\273\372\312\321\264\271\244\313\274\272\307\324\244\267\244\336\244\267\244\277",
#else
  /* 0*/"Ê¸Àá¤Î°ÜÆ°¤Ë¼ºÇÔ¤·¤Þ¤·¤¿",
  /* 1*/"´Á»ú¤ÎÆÉ¤ß¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿",
  /* 2*/"Ê¸Àá¤Î°ÜÆ°¤Ë¼ºÇÔ¤·¤Þ¤·¤¿",
  /* 3*/"´Á»ú¤ÎÆÉ¤ß¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿",
  /* 4*/"¤«¤Ê´Á»úÊÑ´¹¤Ë¼ºÇÔ¤·¤Þ¤·¤¿",
#endif
};

int
enterAdjustMode(uiContext d, yomiContext yc)
{
  extern KanjiModeRec bunsetsu_mode;
  int i, n = 0;
  RkStat rst;

  for (i = 0 ; i < yc->curbun ; i++) {
    if (RkwGoTo(yc->context, i) == -1) {
      return makeRkError(d, e_message[0]);
    }
    if (RkwGetStat(yc->context, &rst) == -1) {
      return makeRkError(d, e_message[1]);
    }
    n += rst.ylen;
  }
  yc->kanjilen = n;
  /* ¥«¥ì¥ó¥ÈÊ¸Àá¤ÎÆÉ¤ß¤ÎÄ¹¤µ¤ò¼è¤ê½Ð¤¹ */
  if (RkwGoTo(yc->context, yc->curbun) == -1) {
    return makeRkError(d, e_message[2]);
  }
  if (RkwGetStat(yc->context, &rst) == -1) {
    return makeRkError(d, e_message[3]);
  }
  yc->bunlen = rst.ylen;

  yc->tanMode = yc->curMode;
  yc->tanMinorMode = yc->minorMode;
  yc->minorMode = CANNA_MODE_AdjustBunsetsuMode;
  d->current_mode = yc->curMode = &bunsetsu_mode;
  return 0;
}

int
leaveAdjustMode(uiContext d, yomiContext yc)
{
  extern KanjiModeRec bunsetsu_mode;

  yc->bunlen = yc->kanjilen = 0;
  yc->minorMode = yc->tanMinorMode;
  d->current_mode = yc->curMode = yc->tanMode;
  return 0;
}


static int
BunFullExtend(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  yc->bunlen = yc->kEndp - yc->kanjilen;
  makeKanjiStatusReturn(d, yc);
  return 0;
}

static int
BunFullShrink(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  yc->bunlen = 1;
  makeKanjiStatusReturn(d, yc);
  return 0;
}

static int
BunExtend(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->kanjilen + yc->bunlen < yc->kEndp) {
    /* ¤Þ¤À¿­¤Ð¤»¤ë */

    yc->bunlen++;
    makeKanjiStatusReturn(d, yc);
    return 0;
  }
  else if (cannaconf.CursorWrap) {
    return BunFullShrink(d);
  }
  (void)NothingChangedWithBeep(d);
  return 0;
}

static int
BunShrink(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->bunlen > 0) {
    /* ¤Þ¤À½Ì¤Þ¤ë */
    int newlen = yc->bunlen;

    newlen--;
    if (newlen > 0) {
      yc->bunlen = newlen;
      makeKanjiStatusReturn(d, yc);
      return 0;
    }
    else if (cannaconf.CursorWrap) {
      return BunFullExtend(d);
    }
  }
  (void)NothingChangedWithBeep(d);
  return 0;
}

static int
BunHenkan(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  yc->nbunsetsu = RkwResize(yc->context, yc->bunlen);
  leaveAdjustMode(d, yc);
  if (yc->nbunsetsu < 0) {
    makeRkError(d, e_message[4]);
    yc->nbunsetsu = 1/* dummy */;
    return TanMuhenkan(d);
  }
  makeKanjiStatusReturn(d, yc);
  currentModeInfo(d);
  return 0;
}

static int
BunQuit(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  leaveAdjustMode(d, yc);
  makeKanjiStatusReturn(d, yc);
  currentModeInfo(d);
  return 0;
}

static int
BunSelfInsert(uiContext d)
{
  d->nbytes = BunQuit(d);
  d->more.todo = 1;
  d->more.ch = d->ch;
  d->more.fnum = CANNA_FN_FunctionalInsert;
  return d->nbytes;
}

static int
BunQuotedInsert(uiContext d)
{
  d->nbytes = BunQuit(d);
  d->more.todo = 1;
  d->more.ch = d->ch;
  d->more.fnum = CANNA_FN_QuotedInsert;
  return d->nbytes;
}

static int
BunKillToEOL(uiContext d)
{
  d->nbytes = BunQuit(d);
  d->more.todo = 1;
  d->more.ch = d->ch;
  d->more.fnum = CANNA_FN_KillToEndOfLine;
  return d->nbytes;
}

#include "bunmap.h"

/* »Ä¤Ã¤Æ¤¤¤ë¤ª»Å»ö

 ¡¦Ãà¼¡¼«Æ°ÊÑ´¹Ãæ¤ÎÊ¸Àá¿­½Ì¥â¡¼¥É
 */
