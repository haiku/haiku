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

#if !defined(lint) && !defined(__CODECENTER__)
static char rcs_id[] = "@(#) 102.1 $Id: mode.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif /* lint */

#include "canna.h"
#include <canna/mfdef.h>

extern struct RkRxDic *romajidic, *englishdic;

extern int howToReturnModeInfo;
static WCHAR_T numMode[2];
static WCHAR_T *bad = (WCHAR_T *)0;
struct ModeNameRecs ModeNames[CANNA_MODE_MAX_IMAGINARY_MODE];

static
char *
_sModeNames[CANNA_MODE_MAX_IMAGINARY_MODE] = {
  "      ",		          /* AlphaMode */
  "[ \244\242 ]",	/* ¤¢ */  /* EmptyMode */ 
  "[\265\255\271\346]",	/* µ­¹æ *//* KigoMode */
  "[\244\350\244\337]",	/* ¤è¤ß *//* YomiMode (¥â¡¼¥ÉÊ¸»úÎóÉ½¼¨¤Ë¤Ï»È¤ï¤Ê¤¤) */
  "[\273\372\274\357]",	/* »ú¼ï *//* JishuMode */
  "[\264\301\273\372]",	/* ´Á»ú *//* TanKouhoMode */
  "[\260\354\315\367]",	/* °ìÍ÷ *//* IchiranMode */
  "[\274\301\314\344]",	/* ¼ÁÌä *//* YesNoMode */
  NULL,	                          /* OnOffMode */
  "[\312\270\300\341]", /* Ê¸Àá *//* AdjustBunsetsuMode */
  "[\303\340\274\241]",	/* Ãà¼¡ *//* ChikujiYomiMode    Ãà¼¡¤Î»þ¤ÎÆÉ¤ßÉôÊ¬ */
  "[\303\340\274\241]",	/* Ãà¼¡ *//* ChikujiHenkanMode  Ãà¼¡¤Î»þ¤ÎÊÑ´¹¤ÎÉôÊ¬ */

  /* Imaginary Mode */

  "[ \224\242 ]",	/* ¤¢ */  /* HenkanNyuryokuMode */
  "[\301\264\244\242]",	/* Á´¤¢ *//* ZenHiraHenkanMode */
  "[\310\276\244\242]",	/* È¾¤¢ *//* HanHiraHenkanMode */
  "[\301\264\245\242]",	/* Á´¥¢ *//* ZenKataHenkanMode */
  "[\310\276\245\242]",	/* È¾¥¢ *//* HanKataHenkanMode */
  "[\301\264\261\321]",	/* Á´±Ñ *//* ZenAlphaHenkanMode */
  "[\310\276\261\321]",	/* È¾±Ñ *//* HanAlphaHenkanMode */
  "<\301\264\244\242>",	/* Á´¤¢ *//* ZenHiraKakuteiMode */
  "<\310\276\244\242>",	/* È¾¤¢ *//* HanHiraKakuteiMode */
  "<\301\264\245\242>",	/* Á´¥¢ *//* ZenKataKakuteiMode */
  "<\310\276\245\242>",	/* È¾¥¢ *//* HanKataKakuteiMode */
  "<\301\264\261\321>",	/* Á´±Ñ *//* ZenAlphaKakuteiMode */
  "<\310\276\261\321>",	/* È¾±Ñ *//* HanAlphaKakuteiMode */
  "[16\277\312]",	/* 16¿Ê *//* HexMode */
  "[\311\364\274\363]",	/* Éô¼ó *//* BushuMode */
  "[\263\310\304\245]",	/* ³ÈÄ¥ *//* ExtendMode */
  "[ \245\355 ]",	/* ¥í */  /* RussianMode */
  "[ \245\256 ]",	/* ¥® */  /* GreekMode */
  "[\267\323\300\376]",	/* ·ÓÀþ *//* LineMode */
  "[\312\321\271\271]",	/* ÊÑ¹¹ *//* ChangingServerMode */
  "[\312\321\264\271]",	/* ÊÑ´¹ *//* HenkanMethodMode */
  "[\272\357\275\374]",	/* ºï½ü *//* DeleteDicMode */
  "[\305\320\317\277]",	/* ÅÐÏ¿ *//* TourokuMode */
  "[\311\312\273\354]",	/* ÉÊ»ì *//* TourokuHinshiMode */
  "[\274\255\275\361]",	/* ¼­½ñ *//* TourokuDicMode */
  "[ \243\361 ]",	/* £ñ */  /* QuotedInsertMode */
  "[\312\324\275\270]",	/* ÊÔ½¸ *//* BubunMuhenkanMode */
  "[\274\255\275\361]", /* ¼­½ñ *//* MountDicMode */
  };


static WCHAR_T * _ModeNames[CANNA_MODE_MAX_IMAGINARY_MODE];

extern extraFunc *FindExtraFunc();
#define findExtraMode(mnum) \
 FindExtraFunc((mnum) - CANNA_MODE_MAX_IMAGINARY_MODE + CANNA_FN_MAX_FUNC)

static WCHAR_T *modestr(int mid);
static void japaneseMode(uiContext d);

newmode *
findExtraKanjiMode(int mnum)
{
  extern extraFunc *extrafuncp;
  extraFunc *extrafunc;
  register int fnum =
    mnum - CANNA_MODE_MAX_IMAGINARY_MODE + CANNA_FN_MAX_FUNC;

  for (extrafunc = extrafuncp; extrafunc; extrafunc = extrafunc->next) {
    if (extrafunc->fnum == fnum) {
      switch (extrafunc->keyword) {
      case EXTRA_FUNC_DEFMODE:
	return extrafunc->u.modeptr;
      default:
	return (newmode *)0;
      }
    }
  }
  return (newmode *)0;
}

extern int nothermodes;

static WCHAR_T *
modestr(int mid)
{
  if (mid < CANNA_MODE_MAX_IMAGINARY_MODE) {
      return(ModeNames[mid].name);
  }
  else if (mid - CANNA_MODE_MAX_IMAGINARY_MODE < nothermodes) {
    extraFunc *ep = findExtraMode(mid);
    if (ep) {
      return ep->display_name;
    }
  }
  return (WCHAR_T *)0;
}

void    
currentModeInfo(uiContext d)
{
  coreContext cc = (coreContext)d->modec;

  if (d->current_mode->flags & CANNA_KANJIMODE_EMPTY_MODE) {
    d->kanji_status_return->info |= KanjiEmptyInfo;
  }

  if (howToReturnModeInfo == ModeInfoStyleIsString) {
    WCHAR_T *modename, *gmodename;
    if (cc->minorMode != d->minorMode) {
      modename = modestr(cc->minorMode);
      gmodename = modestr(d->minorMode);
      d->majorMode = cc->majorMode;
      d->minorMode = cc->minorMode;
      if (modename && (gmodename == (WCHAR_T *)NULL ||
		       WStrcmp(modename, gmodename))) {
	d->kanji_status_return->mode = modename;
	d->kanji_status_return->info |= KanjiModeInfo;
      }
    }
  }
  else {
    if (cc->majorMode != d->majorMode) {
      d->majorMode = cc->majorMode;
      d->minorMode = cc->minorMode;
      numMode[0] = (WCHAR_T)('@' + cc->majorMode);
      numMode[1] = (WCHAR_T) 0;
      d->kanji_status_return->info |= KanjiModeInfo;
      d->kanji_status_return->mode = numMode;
    }
  }
}
     
/* ¤³¤Î¥Õ¥¡¥¤¥ë¤Ë¤Ï¥â¡¼¥ÉÊÑ¹¹¤Ë´Ø¤¹¤ëÁàºî°ìÈÌ¤¬Æþ¤Ã¤Æ¤¤¤ë¡£¥â¡¼¥É¤ÎÊÑ
 * ¹¹¤È¤Ï¡¢¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¤ÎÉ½ÌÌ¤Ë¸½¤ì¤ë¥â¡¼¥É¤ÎÊÑ¹¹¤À¤±¤Ç¤Ï¤Ê¤¯¡¢Á´
 * ¤¯ÆÉ¤ß¤¬¤Ê¤¤¾õÂÖ¤«¤é¡¢ÆÉ¤ß¤¬¤¢¤ë¾õÂÖ¤Ë°Ü¤ë»þ¤Ë¤âÀ¸¤¸¤ë¤â¤Î¤ò»Ø¤¹¡£
 */

void
initModeNames(void)
{
  int i;
  
  for (i = 0 ; i < CANNA_MODE_MAX_IMAGINARY_MODE ; i++) {
    ModeNames[i].alloc = 0;
    ModeNames[i].name = _ModeNames[i] = 
      _sModeNames[i] ? WString(_sModeNames[i]) : 0;
  }
  if (!bad) {
    bad = WString("\245\341\245\342\245\352\244\254\302\255\244\352\244\336"
	"\244\273\244\363");
                  /* ¥á¥â¥ê¤¬Â­¤ê¤Þ¤»¤ó */
  }
}

void
resetModeNames(void)
{
  int i;

  for (i = 0 ; i < CANNA_MODE_MAX_IMAGINARY_MODE ; i++) {
    if (ModeNames[i].alloc && ModeNames[i].name) {
      ModeNames[i].alloc = 0;
      WSfree(ModeNames[i].name);
    }
    ModeNames[i].name = _ModeNames[i];
  }
}

static void
japaneseMode(uiContext d)
{
  coreContext cc = (coreContext)d->modec;

  d->current_mode = cc->prevMode;
  d->modec = cc->next;
  freeCoreContext(cc);
  d->status = EXIT_CALLBACK;
}

/* cfuncdef

  JapaneseMode(d) -- ¥â¡¼¥É¤òÆüËÜ¸ìÆþÎÏ¥â¡¼¥É¤ËÊÑ¤¨¤ë¡£

  ¢¨Ãí ¤³¤Î´Ø¿ô¤Ï¦Á¥â¡¼¥É¤Ç¤·¤«¸Æ¤ó¤Ç¤Ï¤¤¤±¤Ê¤¤¡£

 */

int JapaneseMode(uiContext d)
{
  coreContext cc = (coreContext)d->modec;
  yomiContext yc = (yomiContext)cc->next;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }

  japaneseMode(d);
  d->kanji_status_return->length = 0;
  return 0;
}

int AlphaMode(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }
  else {
    alphaMode(d);
    currentModeInfo(d);
    d->kanji_status_return->length = 0;
    return 0;
  }
}

int HenkanNyuryokuMode(uiContext d)
{
  extern KanjiModeRec empty_mode;
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }

  yc->generalFlags &= (~CANNA_YOMI_ATTRFUNCS & ~CANNA_YOMI_IGNORE_USERSYMBOLS &
		       ~CANNA_YOMI_BASE_HANKAKU);
  d->current_mode = yc->myEmptyMode = &empty_mode;
  yc->majorMode = yc->minorMode = CANNA_MODE_EmptyMode;
  yc->myMinorMode = 0; /* 0 ¤Ï AlphaMode ¤È½Å¤Ê¤ë¤¬ÌäÂê¤Ï¤Ê¤¤ */
  yc->romdic = romajidic;
  EmptyBaseModeInfo(d, yc);

  if(yc->rCurs)
    return RomajiFlushYomi(d, (WCHAR_T *)0, 0); /* ¤³¤ì¤Ï»ÃÄêÅª */

  d->kanji_status_return->length = 0;
  return 0;
}

int queryMode(uiContext d, WCHAR_T *arg)
{
  coreContext cc = (coreContext)d->modec;
  WCHAR_T *mode_str = (WCHAR_T *)0;
  extraFunc *ep;

  switch (howToReturnModeInfo) {
  case ModeInfoStyleIsString:
    if (d->minorMode < (BYTE)CANNA_MODE_MAX_IMAGINARY_MODE) {
      mode_str = ModeNames[d->minorMode].name;
    }
    else if (d->minorMode <
	     (BYTE)(CANNA_MODE_MAX_IMAGINARY_MODE + nothermodes)) {
      ep = findExtraMode(d->minorMode);
      if (ep) {
	mode_str = ep->display_name;
      }
    }
    if (!mode_str) {
      int ii;
      for (ii = 0; ii < 4; ii++, arg++) {
	*arg = (WCHAR_T)'\0';
      }
    }
    else {
      WStrcpy(arg, mode_str);
    }
    break;
  case ModeInfoStyleIsBaseNumeric:
    {
      coreContext ccc;
      yomiContext yc;
      long fl;
      int res;

      arg[3] = 0;

      for (ccc = cc ; ccc && ccc->id != YOMI_CONTEXT ; ccc = ccc->next);
      yc = (yomiContext)ccc; /* This must not be NULL */

      if (yc->id == YOMI_CONTEXT) {
        fl = yc->generalFlags;
       
        if (fl & CANNA_YOMI_ROMAJI) {
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
           res += (CANNA_MODE_ZenHiraKakuteiMode
                     - CANNA_MODE_ZenHiraHenkanMode);
        }
        if (fl & (CANNA_YOMI_CHIKUJI_MODE | CANNA_YOMI_BASE_CHIKUJI))
          arg[3] = CANNA_MODE_ChikujiYomiMode;
      }else{
        res = CANNA_MODE_HanAlphaHenkanMode;
      }
      arg[2] = res;
    }
  case ModeInfoStyleIsExtendedNumeric:
    arg[1] = (WCHAR_T)('@' + (int)cc->minorMode);
  case ModeInfoStyleIsNumeric:
    arg[0] = (WCHAR_T)('@' + (int)cc->majorMode);
    break;
  default:
    return(-1);
    /* NOTREACHED */
    break;
  }
  return 0;
}

/* 
 *   ¤¢¤ë¥â¡¼¥É¤ËÂÐ¤·¤Æ¥â¡¼¥ÉÉ½¼¨Ê¸»úÎó¤ò·èÄê¤¹¤ë¡£
 *
 */

int changeModeName(int modeid, char *str)
{
  extraFunc *ep;

  if (modeid == CANNA_MODE_HenkanNyuryokuMode)
    modeid = CANNA_MODE_EmptyMode;

  if (modeid >= 0) {
    if (modeid < CANNA_MODE_MAX_IMAGINARY_MODE) {
      if (ModeNames[modeid].alloc && ModeNames[modeid].name) {
	WSfree(ModeNames[modeid].name);
      }
      if (str) {
	ModeNames[modeid].alloc = 1;
	ModeNames[modeid].name = WString(str);
      }else{
	ModeNames[modeid].alloc = 0;
	ModeNames[modeid].name = (WCHAR_T *)0;
      }
    }
    else if (modeid < (CANNA_MODE_MAX_IMAGINARY_MODE + nothermodes)) {
      ep = findExtraMode(modeid);
      if (ep) {
	if (ep->display_name) {
	  WSfree(ep->display_name);
	}
	if (str) {
	  ep->display_name = WString(str);
	}
	else {
	  ep->display_name = (WCHAR_T *)0;
	}
      }else{
	return -1;
      }
    }
    return 0;
  }
  return -1;
}

