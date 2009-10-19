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
static char rcs_id[] = "@(#) 102.1 $Id: empty.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif /* lint */

#include "canna.h"
#include "patchlevel.h"
#include "RK.h"
#include "RKintern.h"

extern struct RkRxDic *romajidic, *englishdic;

static int inEmptySelfInsert(uiContext d);
static int EmptySelfInsert(uiContext d);
static int EmptyYomiInsert(uiContext d);
static int EmptyQuotedInsert(uiContext d);
static int AlphaSelfInsert(uiContext d);
static int AlphaNop(uiContext d);
static int EmptyQuit(uiContext d);
static int EmptyKakutei(uiContext d);
static int EmptyDeletePrevious(uiContext d);
static int UserMode(uiContext d, extraFunc *estruct);
static int UserSelect(uiContext d, extraFunc *estruct);
static int UserMenu(uiContext d, extraFunc *estruct);
static int ProcExtraFunc(uiContext d, int fnum);
static int HenkanRegion(uiContext d);
static int PhonoEdit(uiContext d);
static int DicEdit(uiContext d);
static int Configure(uiContext d);
static int renbunInit(uiContext d);
static int showVersion(uiContext d);
static int showServer(uiContext d);
static int showGakushu(uiContext d);
static int showInitFile(uiContext d);
static int showRomkanaFile(uiContext d);
static int dicSync(uiContext d);

extern KanjiModeRec yomi_mode, cy_mode;

/* EmptySelfInsert -- ¼«Ê¬¼«¿È¤ò³ÎÄêÊ¸»úÎó¤È¤·¤ÆÊÖ¤¹´Ø¿ô¡£
 * 
 */

static int
inEmptySelfInsert(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int res = 0;

  d->kanji_status_return->info |= KanjiThroughInfo | KanjiEmptyInfo;
  if (!(yc->generalFlags & CANNA_YOMI_END_IF_KAKUTEI)) {
    res = d->nbytes;
  }
  /* else { ³ÎÄê¥Ç¡¼¥¿¤À¤±¤òÂÔ¤Ã¤Æ¤¤¤ë¿Í¤Ë¤Ï³ÎÄê¥Ç¡¼¥¿¤òÅÏ¤µ¤Ê¤¤ } */

  return res;
}


static int
EmptySelfInsert(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int res = inEmptySelfInsert(d);

/* Ã±¸ìÅÐÏ¿¤Î¤È¤­¤Ë yomi mode ¤Î³ÎÄê¥­¡¼¤¬ empty mode ¤Ç¤Ï³ÎÄê¥­¡¼¤Ç¤Ê
   ¤«¤Ã¤¿¤ê¤¹¤ë¤È¡¢¤½¤Î¥­¡¼¤Î²¡²¼¤Ç»à¤ó¤Ç¤·¤Þ¤Ã¤¿¤ê¤¹¤ë¤Î¤ÎµßºÑ¡£yomi
   mode ¤Î¾å¤Ë yomi mode ¤¬¾è¤Ã¤Æ¤¤¤ë¤Î¤ÏÃ±¸ìÅÐÏ¿¤Î»þ¤°¤é¤¤¤À¤í¤¦¤È¸À
   ¤¦¤³¤È¤ÇÈ½ÃÇ¤ÎºàÎÁ¤Ë¤·¤Æ¤¤¤ë¡£ËÜÅö¤Ï¤³¤ó¤Ê¤³¤È¤ä¤ê¤¿¤¯¤Ê¤¤¡£ */

  if (yc->next && yc->next->id == YOMI_CONTEXT &&
      yomi_mode.keytbl[d->buffer_return[0]] == CANNA_FN_Kakutei) {
    d->status = EXIT_CALLBACK;
    if (d->cb->func[EXIT_CALLBACK] != NO_CALLBACK) {
      d->kanji_status_return->info &= ~KanjiThroughInfo; /* »Å»ö¤·¤¿ */
      popYomiMode(d);
    }
  }
  return res;
}

/* EmptyYomiInsert -- ¡û¥â¡¼¥É¤Ë°Ü¹Ô¤·¡¢ÆÉ¤ß¤òÆþÎÏ¤¹¤ë´Ø¿ô
 *
 */


static int
EmptyYomiInsert(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  d->current_mode = (yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) ?
    &cy_mode : &yomi_mode;
  RomajiClearYomi(d);
  return YomiInsert(d); /* ¥³¡¼¥ë¥Ð¥Ã¥¯¤Î¥Á¥§¥Ã¥¯¤Ï YomiInsert ¤Ç¤µ¤ì¤ë */
}

/* EmptyQuotedInsert -- ¼¡¤Î°ì»ú¤¬¤É¤Î¤è¤¦¤ÊÊ¸»ú¤Ç¤â¥¹¥ë¡¼¤ÇÄÌ¤¹´Ø¿ô¡£
 *
 */

/* 
  Empty ¥â¡¼¥É¤Ç¤Î quotedInset ¤Ï ^Q ¤Î¤è¤¦¤ÊÊ¸»ú¤¬°ì²ó Emacs ¤Ê¤É¤ÎÊý
  ¤ËÄÌ¤Ã¤Æ¤·¤Þ¤¨¤Ð¥Þ¥Ã¥×¤¬ÊÖ¤é¤ì¤Æ¤·¤Þ¤¦¤Î¤Ç¡¢¥«¥Ê´Á»úÊÑ´¹¤ÎÊý¤Ç²¿¤«¤ò
  ¤¹¤ë¤Ê¤ó¤Æ¤³¤È¤ÏÉ¬Í×¤Ê¤¤¤Î¤Ç¤Ï¤Ê¤¤¤Î¤«¤Ê¤¡¡£
 */

static int
EmptyQuotedInsert(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  d->current_mode = (yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) ?
    &cy_mode : &yomi_mode;
  return YomiQuotedInsert(d);
}

/* 
  AlphaSelfInsert -- ¼«Ê¬¼«¿È¤ò³ÎÄêÊ¸»úÎó¤È¤·¤ÆÊÖ¤¹´Ø¿ô¡£
 */

static int
AlphaSelfInsert(uiContext d)
{
  unsigned kanap = (unsigned)d->ch;

  d->kanji_status_return->length = 0;
  d->kanji_status_return->info |= KanjiEmptyInfo;
  d->kanji_status_return->info |= KanjiThroughInfo;
  if ( d->nbytes != 1 || kanap <= 0xa0 || 0xe0 <= kanap ) {
    return d->nbytes;
  }
  else { /* ²¾Ì¾¥­¡¼ÆþÎÏ¤Î¾ì¹ç */
    if (d->n_buffer > 1) {
      return 1;
    }
    else {
      return 0;
    }
  }
}

static int
AlphaNop(uiContext d)
{
  /* currentModeInfo ¤Ç¥â¡¼¥É¾ðÊó¤¬É¬¤ºÊÖ¤ë¤è¤¦¤Ë¥À¥ß¡¼¤Î¥â¡¼¥É¤òÆþ¤ì¤Æ¤ª¤¯ */
  d->majorMode = d->minorMode = CANNA_MODE_KigoMode;
  currentModeInfo(d);
  return 0;
}

static int
EmptyQuit(uiContext d)
{
  int res;

  res = inEmptySelfInsert(d);
  d->status = QUIT_CALLBACK;
  if (d->cb->func[QUIT_CALLBACK] != NO_CALLBACK) {
    d->kanji_status_return->info &= ~KanjiThroughInfo; /* »Å»ö¤·¤¿ */
    popYomiMode(d);
  }
  return res;
}

static int
EmptyKakutei(uiContext d)
{
  int res;

  res = inEmptySelfInsert(d);
  d->status = EXIT_CALLBACK;
  if (d->cb->func[EXIT_CALLBACK] != NO_CALLBACK) {
    d->kanji_status_return->info &= ~KanjiThroughInfo; /* »Å»ö¤·¤¿ */
    popYomiMode(d);
  }
  return res;
}

static int
EmptyDeletePrevious(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_DELETE_DONT_QUIT) {
    /* Delete ¤Ç QUIT ¤·¤Ê¤¤¤Î¤Ç¤¢¤ì¤Ð¡¢selfInsert */
    return inEmptySelfInsert(d);
  }
  else {
    return EmptyQuit(d);
  }
}

extraFunc *
FindExtraFunc(int fnum)
{
  extern extraFunc *extrafuncp;
  extraFunc *extrafunc;

  for (extrafunc = extrafuncp; extrafunc; extrafunc = extrafunc->next) {
    if (extrafunc->fnum == fnum) {
      return extrafunc;
    }
  }
  return (extraFunc *)0;
}

static int
UserMode(uiContext d, extraFunc *estruct)
{
  newmode *nmode = estruct->u.modeptr;
  yomiContext yc = (yomiContext)d->modec;
  int modeid
    = estruct->fnum - CANNA_FN_MAX_FUNC + CANNA_MODE_MAX_IMAGINARY_MODE;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }

  yc->generalFlags &= ~CANNA_YOMI_ATTRFUNCS;
  yc->generalFlags |= nmode->flags;
  if (yc->generalFlags & CANNA_YOMI_END_IF_KAKUTEI) {
    /* ³ÎÄê¤Ç½ª¤ï¤ë¤è¤¦¤Ê¥â¡¼¥É¤À¤Ã¤¿¤é³ÎÄê¥â¡¼¥É¤Ë¤Ê¤é¤Ê¤¤ */
    yc->generalFlags &= ~CANNA_YOMI_KAKUTEI;
  }
  yc->romdic = nmode->romdic;
  d->current_mode = yc->myEmptyMode = nmode->emode;

  yc->majorMode = yc->minorMode = yc->myMinorMode = (BYTE)modeid;

  currentModeInfo(d);

  d->kanji_status_return->length = 0;
  return 0;
}

#ifndef NO_EXTEND_MENU /* continues to the bottom of this file */
static int
UserSelect(uiContext d, extraFunc *estruct)
{
  int curkigo = 0, *posp = (int *)0;
  kigoIchiran *kigop = (kigoIchiran *)0;
  selectinfo *selinfo = (selectinfo *)0, *info;
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    
  info = d->selinfo;
  while (info) {
    if (info->ichiran == estruct->u.kigoptr) {
      selinfo = info;
      break;
    }
    info = info->next;
  }

  if (!selinfo) {
    selinfo = (selectinfo *)malloc(sizeof(selectinfo));
    /* malloc ¤Ë¼ºÇÔ¤·¤¿¾ì¹ç¤Ï¡¢Á°²óÁªÂò¤·¤¿ÈÖ¹æ¤¬ÊÝÂ¸¤µ¤ì¤Ê¤¤ */
    if (selinfo) {
      selinfo->ichiran = estruct->u.kigoptr;
      selinfo->curnum = 0;
      selinfo->next = d->selinfo;
      d->selinfo = selinfo;
    }
  }

  if (selinfo) {
    curkigo = selinfo->curnum;
    posp = &selinfo->curnum;
  }

  kigop = estruct->u.kigoptr;
  if (!kigop) {
    return NothingChangedWithBeep(d);
  }
  return uuKigoMake(d, kigop->kigo_data, kigop->kigo_size, 
                    curkigo, kigop->kigo_mode, (int(*)(...))uuKigoGeneralExitCatch, posp);
}
  
static int
UserMenu(uiContext d, extraFunc *estruct)
{
  return showmenu(d, estruct->u.menuptr);
}
#endif /* NO_EXTEND_MENU */

/* ¥Ç¥Õ¥©¥ë¥È°Ê³°¤Î¥â¡¼¥É»ÈÍÑ»þ¤Ë¸Æ¤Ó½Ð¤¹´Ø¿ô¤òÀÚ¤êÊ¬¤±¤ë */

static int
ProcExtraFunc(uiContext d, int fnum)
{
  extraFunc *extrafunc;

  extrafunc = FindExtraFunc(fnum);
  if (extrafunc) {
    switch (extrafunc->keyword) {
      case EXTRA_FUNC_DEFMODE:
        return UserMode(d, extrafunc);
#ifndef NO_EXTEND_MENU
      case EXTRA_FUNC_DEFSELECTION:
        return UserSelect(d, extrafunc);
      case EXTRA_FUNC_DEFMENU:
        return UserMenu(d, extrafunc);
#endif
      default:
        break;
    }
  }
  return NothingChangedWithBeep(d);
}

int
getBaseMode(yomiContext yc)
{
  int res;
  long fl = yc->generalFlags;

  if (yc->myMinorMode) {
    return yc->myMinorMode;
  }
  else if (fl & CANNA_YOMI_ROMAJI) {
    res = CANNA_MODE_ZenAlphaHenkanMode;
  }
  else if (fl & CANNA_YOMI_KATAKANA) {
    res = CANNA_MODE_ZenKataHenkanMode;
  }
  else {
    res = CANNA_MODE_ZenHiraHenkanMode;
  }
  if (fl & CANNA_YOMI_BASE_HANKAKU) {
    res++;
  }
  if (fl & CANNA_YOMI_KAKUTEI) {
    res += CANNA_MODE_ZenHiraKakuteiMode - CANNA_MODE_ZenHiraHenkanMode;
  }
  if (res == CANNA_MODE_ZenHiraHenkanMode) {
    if (chikujip(yc)) {
      res = CANNA_MODE_ChikujiYomiMode;
    }
    else {
      res = CANNA_MODE_HenkanMode;
    }
  }
  return res;
}

/* ¥Ù¡¼¥¹Ê¸»ú¤ÎÀÚ¤êÂØ¤¨ */

void
EmptyBaseModeInfo(uiContext d, yomiContext yc)
{
  coreContext cc = (coreContext)d->modec;

  cc->minorMode = getBaseMode(yc);
  currentModeInfo(d);
}

int
EmptyBaseHira(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }
  yc->generalFlags &= ~(CANNA_YOMI_KATAKANA | CANNA_YOMI_HANKAKU |
			CANNA_YOMI_ROMAJI | CANNA_YOMI_ZENKAKU);
  EmptyBaseModeInfo(d, yc);
  return 0;
}

int
EmptyBaseKata(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if ((yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED)
      || (cannaconf.InhibitHankakuKana
	  && (yc->generalFlags & CANNA_YOMI_BASE_HANKAKU))) {
    return NothingChangedWithBeep(d);
  }
  yc->generalFlags &= ~(CANNA_YOMI_ROMAJI | CANNA_YOMI_ZENKAKU);
  yc->generalFlags |= CANNA_YOMI_KATAKANA |
    ((yc->generalFlags & CANNA_YOMI_BASE_HANKAKU) ? CANNA_YOMI_HANKAKU : 0);
  EmptyBaseModeInfo(d, yc);
  return 0;
}

int
EmptyBaseEisu(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }
/*  yc->generalFlags &= ~CANNA_YOMI_ATTRFUNCS; ¥¯¥ê¥¢¤¹¤ë¤Î¤ä¤á */
  yc->generalFlags |= CANNA_YOMI_ROMAJI |
    ((yc->generalFlags & CANNA_YOMI_BASE_HANKAKU) ? 0 : CANNA_YOMI_ZENKAKU);
  EmptyBaseModeInfo(d, yc);
  return 0;
}

int
EmptyBaseZen(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }
  yc->generalFlags &= ~CANNA_YOMI_BASE_HANKAKU;
  if (yc->generalFlags & CANNA_YOMI_ROMAJI) {
    yc->generalFlags |= CANNA_YOMI_ZENKAKU;
  }
  /* ¢¨Ãí ¥í¡¼¥Þ»ú¥â¡¼¥É¤Ç¤«¤Ä¥«¥¿¥«¥Ê¥â¡¼¥É¤Î»þ¤¬¤¢¤ë
          (¤½¤Î¾ì¹çÉ½ÌÌ¾å¤Ï¥í¡¼¥Þ»ú¥â¡¼¥É) */
  if (yc->generalFlags & CANNA_YOMI_KATAKANA) {
    yc->generalFlags &= ~CANNA_YOMI_HANKAKU;
  }
  EmptyBaseModeInfo(d, yc);
  return 0;
}

int
EmptyBaseHan(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if ((yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) ||
      /* ¥â¡¼¥ÉÊÑ¹¹¤¬¶Ø»ß¤µ¤ì¤Æ¤¤¤ë¤È¤« */
      (cannaconf.InhibitHankakuKana &&
       (yc->generalFlags & CANNA_YOMI_KATAKANA) &&
       !(yc->generalFlags & CANNA_YOMI_ROMAJI))) {
    /* È¾³Ñ¥«¥Ê¤¬¶Ø»ß¤µ¤ì¤Æ¤¤¤ë¤Î¤ËÈ¾³Ñ¥«¥Ê¤Ë¤¤¤­¤½¤¦¤Ê¤È¤­ */
    return NothingChangedWithBeep(d);
  }
  yc->generalFlags |= CANNA_YOMI_BASE_HANKAKU;
  if (yc->generalFlags & CANNA_YOMI_ROMAJI) {
    yc->generalFlags &= ~CANNA_YOMI_ZENKAKU;
  }
  /* ¢¨Ãí ¥í¡¼¥Þ»ú¥â¡¼¥É¤Ç¤«¤Ä¥«¥¿¥«¥Ê¥â¡¼¥É¤Î»þ¤¬¤¢¤ë
          (¤½¤Î¾ì¹çÉ½ÌÌ¾å¤Ï¥í¡¼¥Þ»ú¥â¡¼¥É) */
  if (yc->generalFlags & CANNA_YOMI_KATAKANA) {
    if (!cannaconf.InhibitHankakuKana) {
      yc->generalFlags |= CANNA_YOMI_HANKAKU;
    }
  }
  EmptyBaseModeInfo(d, yc);
  return 0;
}

int
EmptyBaseKana(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if ((yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) ||
      /* ¥â¡¼¥ÉÊÑ¹¹¤¬¶Ø»ß¤µ¤ì¤Æ¤¤¤¿¤ê */
      (!cannaconf.InhibitHankakuKana &&
       (yc->generalFlags & CANNA_YOMI_KATAKANA) &&
       (yc->generalFlags & CANNA_YOMI_BASE_HANKAKU))) {
    /* È¾³Ñ¥«¥Ê¤¬¶Ø»ß¤µ¤ì¤Æ¤¤¤ë¤Î¤ËÈ¾³Ñ¥«¥Ê¤Ë¤Ê¤Ã¤Æ¤·¤Þ¤¤¤½¤¦¤Ê¾ì¹ç */
    return NothingChangedWithBeep(d);
  }
  yc->generalFlags &= ~(CANNA_YOMI_ROMAJI | CANNA_YOMI_ZENKAKU);

  if ((yc->generalFlags & CANNA_YOMI_BASE_HANKAKU) &&
      (yc->generalFlags & CANNA_YOMI_KATAKANA)) {
    yc->generalFlags |= CANNA_YOMI_HANKAKU;
  }
  EmptyBaseModeInfo(d, yc);
  return 0;
}

int
EmptyBaseKakutei(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }
  yc->generalFlags |= CANNA_YOMI_KAKUTEI;

  EmptyBaseModeInfo(d, yc);
  return 0;
}

int
EmptyBaseHenkan(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }
  yc->generalFlags &= ~CANNA_YOMI_KAKUTEI;

  EmptyBaseModeInfo(d, yc);
  return 0;
}

#ifdef WIN

extern specialfunc (uiContext, int);

static HenkanRegion (uiContext);

static
HenkanRegion(uiContext d)
{
  return specialfunc(d, CANNA_FN_HenkanRegion);
}

static PhonoEdit (uiContext);

static
PhonoEdit(uiContext d)
{
  return specialfunc(d, CANNA_FN_PhonoEdit);
}

static DicEdit (uiContext);

static
DicEdit(uiContext d)
{
  return specialfunc(d, CANNA_FN_DicEdit);
}

static Configure (uiContext);

static
Configure(uiContext d)
{
  return specialfunc(d, CANNA_FN_Configure);
}

#endif

#ifndef NO_EXTEND_MENU
static int
renbunInit(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    
  d->status = 0;
  killmenu(d);
  if (ToggleChikuji(d, 0) == -1) {
    jrKanjiError = "\317\242\312\270\300\341\312\321\264\271\244\313\300\332"
	"\302\330\244\250\244\353\244\263\244\310\244\254\244\307\244\255"
	"\244\336\244\273\244\363";
                   /* Ï¢Ê¸ÀáÊÑ´¹¤ËÀÚÂØ¤¨¤ë¤³¤È¤¬¤Ç¤­¤Þ¤»¤ó */
    makeGLineMessageFromString(d, jrKanjiError);
    currentModeInfo(d);
    return(-1);
  }
  else {
    makeGLineMessageFromString(d, "\317\242\312\270\300\341\312\321\264\271"
	"\244\313\300\332\302\330\244\250\244\336\244\267\244\277");
                   /* Ï¢Ê¸ÀáÊÑ´¹¤ËÀÚÂØ¤¨¤Þ¤·¤¿ */
    currentModeInfo(d);
    return 0;
  }
}

static int
showVersion(uiContext d)
{
  int retval = 0;
  char s[512];
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    
  d->status = 0;
  killmenu(d);

  sprintf(s, "\306\374\313\334\270\354\306\376\316\317\245\267\245\271\245\306"
	"\245\340\241\330\244\253\244\363\244\312\241\331Version %d.%d",
	  cannaconf.CannaVersion / 1000, cannaconf.CannaVersion % 1000);
             /* ÆüËÜ¸ìÆþÎÏ¥·¥¹¥Æ¥à¡Ø¤«¤ó¤Ê¡Ù */
  strcat(s, CANNA_PATCH_LEVEL);
  makeGLineMessageFromString(d, s);
  currentModeInfo(d);

  return (retval);
}

static int
showServer(uiContext d)
{
#ifndef STANDALONE /* This is not used in Windows environment 1996.7.30 kon */
  int retval = 0;
  char s[512];
  extern defaultContext;
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    
  d->status = 0;
  killmenu(d);

  if(defaultContext == -1) {
    sprintf(s, "\244\253\244\312\264\301\273\372\312\321\264\271\245\265"
	"\241\274\245\320\244\310\244\316\300\334\302\263\244\254\300\332"
	"\244\354\244\306\244\244\244\336\244\271");
               /* ¤«¤Ê´Á»úÊÑ´¹¥µ¡¼¥Ð¤È¤ÎÀÜÂ³¤¬ÀÚ¤ì¤Æ¤¤¤Þ¤¹ */
  }
  else {
    sprintf(s, "%s \244\316\244\253\244\312\264\301\273\372\312\321\264\271"
	"\245\265\241\274\245\320\244\313\300\334\302\263\244\267\244\306"
	"\244\244\244\336\244\271", RkwGetServerName());
               /* ¤Î¤«¤Ê´Á»úÊÑ´¹¥µ¡¼¥Ð¤ËÀÜÂ³¤·¤Æ¤¤¤Þ¤¹ */
  }
  makeGLineMessageFromString(d, s);
  currentModeInfo(d);

  return (retval);
#else
  return (0);
#endif /* STANDALONE */
}

static int
showGakushu(uiContext d)
{
  int retval = 0;
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    
  d->status = 0;
  killmenu(d);
  
  if (cannaconf.Gakushu == 1) {
    makeGLineMessageFromString(d, "\263\330\275\254\244\254\243\317\243\316"
	"\244\316\276\365\302\326\244\307\244\271");
                                  /* ³Ø½¬¤¬£Ï£Î¤Î¾õÂÖ¤Ç¤¹ */
  }
  else {
    makeGLineMessageFromString(d, "\263\330\275\254\244\254\243\317\243\306"
	"\243\306\244\316\276\365\302\326\244\307\244\271");
                                  /* ³Ø½¬¤¬£Ï£Æ£Æ¤Î¾õÂÖ¤Ç¤¹ */
  }
    currentModeInfo(d);

  return (retval);
}

static int
showInitFile(uiContext d)
{
  int retval = 0;
  char s[512];
  extern char *CANNA_initfilename;
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    
  d->status = 0;
  killmenu(d);

  if (CANNA_initfilename && strlen(CANNA_initfilename)) {
    sprintf(s, "\245\253\245\271\245\277\245\336\245\244\245\272\245\325"
	"\245\241\245\244\245\353\244\317 %s \244\362\273\310\315\321\244\267"
	"\244\306\244\244\244\336\244\271", CANNA_initfilename);
               /* ¥«¥¹¥¿¥Þ¥¤¥º¥Õ¥¡¥¤¥ë¤Ï %s ¤ò»ÈÍÑ¤·¤Æ¤¤¤Þ¤¹ */
    makeGLineMessageFromString(d, s);
  }
  else {
    sprintf(s, "\245\253\245\271\245\277\245\336\245\244\245\272\245\325"
	"\245\241\245\244\245\353\244\317\300\337\304\352\244\265\244\354"
	"\244\306\244\244\244\336\244\273\244\363");
               /* ¥«¥¹¥¿¥Þ¥¤¥º¥Õ¥¡¥¤¥ë¤ÏÀßÄê¤µ¤ì¤Æ¤¤¤Þ¤»¤ó */
    makeGLineMessageFromString(d, s);
  }
    currentModeInfo(d);

  return (retval);
}

static int
showRomkanaFile(uiContext d)
{
  int retval = 0;
  char s[512];
  extern char *RomkanaTable;
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    
  d->status = 0;
  killmenu(d);
  
  if (RomkanaTable && romajidic) {
    sprintf(s, "\245\355\241\274\245\336\273\372\244\253\244\312\312\321"
	"\264\271\245\306\241\274\245\326\245\353\244\317 %s \244\362\273\310"
	"\315\321\244\267\244\306\244\244\244\336\244\271",
	    RomkanaTable);
               /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë¤Ï %s ¤ò»ÈÍÑ¤·¤Æ¤¤¤Þ¤¹ */
    makeGLineMessageFromString(d, s);
  }
  else {
    sprintf(s, "\245\355\241\274\245\336\273\372\244\253\244\312\312\321"
	"\264\271\245\306\241\274\245\326\245\353\244\317\273\310\315\321"
	"\244\265\244\354\244\306\244\244\244\336\244\273\244\363");
               /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë¤Ï»ÈÍÑ¤µ¤ì¤Æ¤¤¤Þ¤»¤ó */
    makeGLineMessageFromString(d, s);
  }
    currentModeInfo(d);

  return (retval);
}

static int
dicSync(uiContext d)
{
  int retval = 0;
  char s[512];
  extern int defaultContext;
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    
  d->status = 0;
  killmenu(d);

  retval = RkwSync(defaultContext, "");
  sprintf(s, "\274\255\275\361\244\316 Sync \275\350\315\375%s",
          retval < 0 ? "\244\313\274\272\307\324\244\267\244\336\244\267"
	"\244\277" : "\244\362\271\324\244\244\244\336\244\267\244\277");
          /* ¼­½ñ¤Î Sync ½èÍý%s",
                retval < 0 ? "¤Ë¼ºÇÔ¤·¤Þ¤·¤¿" : "¤ò¹Ô¤¤¤Þ¤·¤¿ */
  makeGLineMessageFromString(d, s);
  currentModeInfo(d);

  return 0;
}
#endif /* not NO_EXTEND_MENU */

#include "emptymap.h"
#include "alphamap.h"
