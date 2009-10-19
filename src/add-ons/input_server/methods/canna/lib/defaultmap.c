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
static char rcs_id[] = "@(#) 102.1 $Id: defaultmap.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif	/* lint */

#include "canna.h"
#include <canna/mfdef.h>

extern int howToBehaveInCaseOfUndefKey;

#define DEFAULTBEHAVIOR 0

static int
(*getfunc(
struct funccfunc *tbl,
unsigned char f))(...)
{
  struct funccfunc *p;

  for (p = tbl ; p->funcid || p->cfunc ; p++) {
    if (p->funcid == (unsigned char)f) {
      return (int (*)(...))p->cfunc;
    }
  }
  return (int (*)(...))0;
}

static int
simpleUndefBehavior(uiContext d)
{
  switch (howToBehaveInCaseOfUndefKey)
    {
    case kc_through:
      d->kanji_status_return->length = -1;
      return d->nbytes;
    case kc_kakutei:
      d->nbytes = escapeToBasicStat(d, CANNA_FN_Kakutei);
      if (d->n_buffer > d->nbytes) {
	int check;
	d->buffer_return[d->nbytes] =
	  key2wchar(d->ch, &check);
	if (check) {
	  d->nbytes++;
	}
      }
      return d->nbytes;
    case kc_kill:
      d->nbytes = escapeToBasicStat(d, CANNA_FN_Quit);
      if (d->n_buffer > d->nbytes) {
	int check;
	d->buffer_return[d->nbytes] =
	  key2wchar(d->ch, &check);
	if (check) {
	  d->nbytes++;
	}
      }
      return d->nbytes;
    case kc_normal:
    default:
      return NothingChangedWithBeep(d);
    }
}

int
searchfunc(uiContext d, KanjiMode mode, int whattodo, int key, int fnum)
{
  int (*func)(...);

  if (fnum == 0) {
    fnum = mode->keytbl[key];
  }
  switch (whattodo) {
  case KEY_CALL:
    /* ¥¢¥ë¥Õ¥¡¥Ù¥Ã¥È¥â¡¼¥É¤¬ strokelimit ¥¹¥È¥í¡¼¥¯°Ê¾åÂ³¤¤¤¿¤é
       ¥µ¡¼¥Ð¤È¤ÎÀÜÂ³¤òÀÚ¤ë */
    if (cannaconf.strokelimit > 0) {
      extern KanjiModeRec alpha_mode;
      if (mode == &alpha_mode) {
	d->strokecounter++;
#ifdef DEBUG
	if (iroha_debug) {
	  fprintf(stderr, "d->strokecounter=%d\n", d->strokecounter);
	}
#endif
	if (d->strokecounter == cannaconf.strokelimit + 1) {
	  jrKanjiPipeError();
	}	
      }else{
	d->strokecounter = 0;
#ifdef DEBUG
	if (iroha_debug) {
	  fprintf(stderr, "d->strokecounter=%d\n", d->strokecounter);
	}
#endif
      }
    }
    /* ¤¤¤è¤¤¤èËÜ³ÊÅª¤Ê½èÍý(¤³¤³¤Þ¤Ç¤ÏÁ°½èÍý) */
    if (fnum < CANNA_FN_MAX_FUNC) {
      func = getfunc(mode->ftbl, fnum);
      if (func) {
	return (*func)(d);
      }
    }
    else {
      func = getfunc(mode->ftbl, CANNA_FN_UserMode);
      if (func) {
	/* func ¤Î¥¿¥¤¥×¤¬¾å¤È°ã¤Ã¤Æ¤Æ±ø¤¤¤Ê¤¢... */
	return (*func)(d, fnum);
      }
    }
    /* ¤½¤Î¥â¡¼¥É¤Ç fnum ¤ËÂÐ±þ¤¹¤ëµ¡Ç½¤¬¤Ê¤¤¡£¤·¤«¤¿¤¬¤Ê¤¤¤Î¤Ç¡¢
       ¥Ç¥Õ¥©¥ë¥Èµ¡Ç½¤òÃµ¤¹ */
    func = getfunc(mode->ftbl, DEFAULTBEHAVIOR);
    if (func) {
      return (*func)(d);
    }
    else {
      return simpleUndefBehavior(d);
    }
    /* NOTREACHED */
    break;
  case KEY_CHECK:
    if (fnum >= CANNA_FN_MAX_FUNC) {
      fnum = CANNA_FN_UserMode;
    }
    return getfunc(mode->ftbl, fnum) ? 1 : 0;
    /* NOTREACHED */
    break;
  case KEY_SET: /* is not supported yet */
    return 0;
    /* NOTREACHED */
    break;
  }
  /* NOTREACHED */
}

/* Ãà¼¡ÆÉ¤ß¥â¡¼¥ÉÍÑ */

int
CYsearchfunc(uiContext d, KanjiMode mode, int whattodo, int key, int fnum)
{
  int (*func)(...);
  extern KanjiModeRec yomi_mode;

  if (fnum == 0) {
    fnum = mode->keytbl[key];
  }
  if (Yomisearchfunc(d, mode, KEY_CHECK, key, fnum)) {
    return Yomisearchfunc(d, mode, whattodo, key, fnum);
  }
  else {
    func = getfunc(yomi_mode.ftbl, fnum);
    switch (whattodo) {
    case KEY_CALL:
      if (func) {
	return (*func)(d);
      }else{
	return Yomisearchfunc(d, mode, whattodo, key, fnum);
      }
      /* NOTREACHED */
      break;
    case KEY_CHECK:
      return func ? 1 : 0;
      /* NOTREACHED */
      break;
    case KEY_SET:
      return 0;
      /* NOTREACHED */
      break;
    }
  }
  /* may be NOTREACHED */
  return 0;
}

#define NONE CANNA_FN_Undefined

BYTE default_kmap[256] =
{               
/* C-@ */       CANNA_FN_Mark,
/* C-a */       CANNA_FN_BeginningOfLine,
/* C-b */       CANNA_FN_Backward,
/* C-c */       CANNA_FN_BubunMuhenkan,
/* C-d */       CANNA_FN_DeleteNext,
/* C-e */       CANNA_FN_EndOfLine,
/* C-f */       CANNA_FN_Forward,
/* C-g */       CANNA_FN_Quit,
/* C-h */       CANNA_FN_DeletePrevious,
/* C-i */       CANNA_FN_Shrink,
/* C-j */       CANNA_FN_BubunKakutei,
/* C-k */       CANNA_FN_KillToEndOfLine,
/* C-l */       CANNA_FN_ToLower,
/* C-m */       CANNA_FN_Kakutei,
/* C-n */       CANNA_FN_Next,
/* C-o */       CANNA_FN_Extend,
/* C-p */       CANNA_FN_Prev,
/* C-q */       CANNA_FN_QuotedInsert,
/* C-r */       NONE,
/* C-s */       NONE,
/* C-t */       NONE,
/* C-u */       CANNA_FN_ToUpper,
/* C-v */       NONE,
/* C-w */       CANNA_FN_KouhoIchiran,
/* C-x */       NONE,
/* C-y */       CANNA_FN_ConvertAsHex,
/* C-z */       NONE,
#ifdef WIN
/* C-[ */       CANNA_FN_Quit,
#else
/* C-[ */       NONE,
#endif
/* C-\ */       NONE,
/* C-] */       NONE,
/* C-^ */       NONE,
/* C-_ */       NONE,
/* space */     CANNA_FN_Henkan,
/* ! */         CANNA_FN_FunctionalInsert,
/* " */         CANNA_FN_FunctionalInsert,
/* # */         CANNA_FN_FunctionalInsert,
/* $ */         CANNA_FN_FunctionalInsert,
/* % */         CANNA_FN_FunctionalInsert,
/* & */         CANNA_FN_FunctionalInsert,
/* ' */         CANNA_FN_FunctionalInsert,
/* ( */         CANNA_FN_FunctionalInsert,
/* ) */         CANNA_FN_FunctionalInsert,
/* * */         CANNA_FN_FunctionalInsert,
/* + */         CANNA_FN_FunctionalInsert,
/* , */         CANNA_FN_FunctionalInsert,
/* - */         CANNA_FN_FunctionalInsert,
/* . */         CANNA_FN_FunctionalInsert,
/* / */         CANNA_FN_FunctionalInsert,
/* 0 */         CANNA_FN_FunctionalInsert,
/* 1 */         CANNA_FN_FunctionalInsert,
/* 2 */         CANNA_FN_FunctionalInsert,
/* 3 */         CANNA_FN_FunctionalInsert,
/* 4 */         CANNA_FN_FunctionalInsert,
/* 5 */         CANNA_FN_FunctionalInsert,
/* 6 */         CANNA_FN_FunctionalInsert,
/* 7 */         CANNA_FN_FunctionalInsert,
/* 8 */         CANNA_FN_FunctionalInsert,
/* 9 */         CANNA_FN_FunctionalInsert,
/* : */         CANNA_FN_FunctionalInsert,
/* ; */         CANNA_FN_FunctionalInsert,
/* < */         CANNA_FN_FunctionalInsert,
/* = */         CANNA_FN_FunctionalInsert,
/* > */         CANNA_FN_FunctionalInsert,
/* ? */         CANNA_FN_FunctionalInsert,
/* @ */         CANNA_FN_FunctionalInsert,
/* A */         CANNA_FN_FunctionalInsert,
/* B */         CANNA_FN_FunctionalInsert,
/* C */         CANNA_FN_FunctionalInsert,
/* D */         CANNA_FN_FunctionalInsert,
/* E */         CANNA_FN_FunctionalInsert,
/* F */         CANNA_FN_FunctionalInsert,
/* G */         CANNA_FN_FunctionalInsert,
/* H */         CANNA_FN_FunctionalInsert,
/* I */         CANNA_FN_FunctionalInsert,
/* J */         CANNA_FN_FunctionalInsert,
/* K */         CANNA_FN_FunctionalInsert,
/* L */         CANNA_FN_FunctionalInsert,
/* M */         CANNA_FN_FunctionalInsert,
/* N */         CANNA_FN_FunctionalInsert,
/* O */         CANNA_FN_FunctionalInsert,
/* P */         CANNA_FN_FunctionalInsert,
/* Q */         CANNA_FN_FunctionalInsert,
/* R */         CANNA_FN_FunctionalInsert,
/* S */         CANNA_FN_FunctionalInsert,
/* T */         CANNA_FN_FunctionalInsert,
/* U */         CANNA_FN_FunctionalInsert,
/* V */         CANNA_FN_FunctionalInsert,
/* W */         CANNA_FN_FunctionalInsert,
/* X */         CANNA_FN_FunctionalInsert,
/* Y */         CANNA_FN_FunctionalInsert,
/* Z */         CANNA_FN_FunctionalInsert,
/* [ */         CANNA_FN_FunctionalInsert,
/* \ */         CANNA_FN_FunctionalInsert,
/* ] */         CANNA_FN_FunctionalInsert,
/* ^ */         CANNA_FN_FunctionalInsert,
/* _ */         CANNA_FN_FunctionalInsert,
/* ` */         CANNA_FN_FunctionalInsert,
/* a */         CANNA_FN_FunctionalInsert,
/* b */         CANNA_FN_FunctionalInsert,
/* c */         CANNA_FN_FunctionalInsert,
/* d */         CANNA_FN_FunctionalInsert,
/* e */         CANNA_FN_FunctionalInsert,
/* f */         CANNA_FN_FunctionalInsert,
/* g */         CANNA_FN_FunctionalInsert,
/* h */         CANNA_FN_FunctionalInsert,
/* i */         CANNA_FN_FunctionalInsert,
/* j */         CANNA_FN_FunctionalInsert,
/* k */         CANNA_FN_FunctionalInsert,
/* l */         CANNA_FN_FunctionalInsert,
/* m */         CANNA_FN_FunctionalInsert,
/* n */         CANNA_FN_FunctionalInsert,
/* o */         CANNA_FN_FunctionalInsert,
/* p */         CANNA_FN_FunctionalInsert,
/* q */         CANNA_FN_FunctionalInsert,
/* r */         CANNA_FN_FunctionalInsert,
/* s */         CANNA_FN_FunctionalInsert,
/* t */         CANNA_FN_FunctionalInsert,
/* u */         CANNA_FN_FunctionalInsert,
/* v */         CANNA_FN_FunctionalInsert,
/* w */         CANNA_FN_FunctionalInsert,
/* x */         CANNA_FN_FunctionalInsert,
/* y */         CANNA_FN_FunctionalInsert,
/* z */         CANNA_FN_FunctionalInsert,
/* { */         CANNA_FN_FunctionalInsert,
/* | */         CANNA_FN_FunctionalInsert,
/* } */         CANNA_FN_FunctionalInsert,
/* ~ */         CANNA_FN_FunctionalInsert,
/* DEL */       NONE,
#ifdef WIN
/* Nfer */      CANNA_FN_KanaRotate,
#else
/* Nfer */      CANNA_FN_Kakutei,
#endif
/* Xfer */      CANNA_FN_Henkan,
/* Up */        CANNA_FN_Prev,
/* Left */      CANNA_FN_Backward,
/* Right */     CANNA_FN_Forward,
/* Down */      CANNA_FN_Next,
#ifdef WIN
/* Insert */    NONE,
#else
/* Insert */    CANNA_FN_KigouMode,
#endif
/* Rollup */    CANNA_FN_PageDown,
/* Rolldown */  CANNA_FN_PageUp,
/* Home */      CANNA_FN_BeginningOfLine,
/* Help */      CANNA_FN_EndOfLine,
/* KeyPad */	NONE,
/* End */       CANNA_FN_EndOfLine,
/* 8d */        NONE,
/* 8e */        NONE,
/* 8f */        NONE,
#ifdef WIN
/* S-nfer */    CANNA_FN_RomajiRotate,
#else
/* S-nfer */    NONE,
#endif
/* S-xfer */    CANNA_FN_Prev,
/* S-up */      NONE,
/* S-left */    CANNA_FN_Shrink,
/* S-right */   CANNA_FN_Extend,
/* S-down */    NONE,
/* C-nfer */    NONE,
/* C-xfer */    NONE,
/* C-up */      NONE,
/* C-left */    CANNA_FN_Shrink,
/* C-right */   CANNA_FN_Extend,
/* C-down */    NONE,
/* KP-, */      NONE,
/* KP-. */      NONE,
/* KP-/ */      NONE,
/* KP-- */      NONE,
/* S-space */   NONE,
/* ¡£ */        CANNA_FN_FunctionalInsert,
/* ¡Ö */        CANNA_FN_FunctionalInsert,
/* ¡× */        CANNA_FN_FunctionalInsert,
/* ¡¢ */        CANNA_FN_FunctionalInsert,
/* ¡¦ */        CANNA_FN_FunctionalInsert,
/* ¥ò */        CANNA_FN_FunctionalInsert,
/* ¥¡ */        CANNA_FN_FunctionalInsert,
/* ¥£ */        CANNA_FN_FunctionalInsert,
/* ¥¥ */        CANNA_FN_FunctionalInsert,
/* ¥§ */        CANNA_FN_FunctionalInsert,
/* ¥© */        CANNA_FN_FunctionalInsert,
/* ¥ã */        CANNA_FN_FunctionalInsert,
/* ¥å */        CANNA_FN_FunctionalInsert,
/* ¥ç */        CANNA_FN_FunctionalInsert,
/* ¥Ã */        CANNA_FN_FunctionalInsert,
/* ¡¼ */        CANNA_FN_FunctionalInsert,
/* ¥¢ */        CANNA_FN_FunctionalInsert,
/* ¥¤ */        CANNA_FN_FunctionalInsert,
/* ¥¦ */        CANNA_FN_FunctionalInsert,
/* ¥¨ */        CANNA_FN_FunctionalInsert,
/* ¥ª */        CANNA_FN_FunctionalInsert,
/* ¥« */        CANNA_FN_FunctionalInsert,
/* ¥­ */        CANNA_FN_FunctionalInsert,
/* ¥¯ */        CANNA_FN_FunctionalInsert,
/* ¥± */        CANNA_FN_FunctionalInsert,
/* ¥³ */        CANNA_FN_FunctionalInsert,
/* ¥µ */        CANNA_FN_FunctionalInsert,
/* ¥· */        CANNA_FN_FunctionalInsert,
/* ¥¹ */        CANNA_FN_FunctionalInsert,
/* ¥» */        CANNA_FN_FunctionalInsert,
/* ¥½ */        CANNA_FN_FunctionalInsert,
/* ¥¿ */        CANNA_FN_FunctionalInsert,
/* ¥Á */        CANNA_FN_FunctionalInsert,
/* ¥Ä */        CANNA_FN_FunctionalInsert,
/* ¥Æ */        CANNA_FN_FunctionalInsert,
/* ¥È */        CANNA_FN_FunctionalInsert,
/* ¥Ê */        CANNA_FN_FunctionalInsert,
/* ¥Ë */        CANNA_FN_FunctionalInsert,
/* ¥Ì */        CANNA_FN_FunctionalInsert,
/* ¥Í */        CANNA_FN_FunctionalInsert,
/* ¥Î */        CANNA_FN_FunctionalInsert,
/* ¥Ï */        CANNA_FN_FunctionalInsert,
/* ¥Ò */        CANNA_FN_FunctionalInsert,
/* ¥Õ */        CANNA_FN_FunctionalInsert,
/* ¥Ø */        CANNA_FN_FunctionalInsert,
/* ¥Û */        CANNA_FN_FunctionalInsert,
/* ¥Þ */        CANNA_FN_FunctionalInsert,
/* ¥ß */        CANNA_FN_FunctionalInsert,
/* ¥à */        CANNA_FN_FunctionalInsert,
/* ¥á */        CANNA_FN_FunctionalInsert,
/* ¥â */        CANNA_FN_FunctionalInsert,
/* ¥ä */        CANNA_FN_FunctionalInsert,
/* ¥æ */        CANNA_FN_FunctionalInsert,
/* ¥è */        CANNA_FN_FunctionalInsert,
/* ¥é */        CANNA_FN_FunctionalInsert,
/* ¥ê */        CANNA_FN_FunctionalInsert,
/* ¥ë */        CANNA_FN_FunctionalInsert,
/* ¥ì */        CANNA_FN_FunctionalInsert,
/* ¥í */        CANNA_FN_FunctionalInsert,
/* ¥ï */        CANNA_FN_FunctionalInsert,
/* ¥ó */        CANNA_FN_FunctionalInsert,
/* ¡« */        CANNA_FN_FunctionalInsert,
/* ¡¬ */        CANNA_FN_FunctionalInsert,
/* F1 */        NONE,
/* F2 */        NONE,
/* F3 */        NONE,
/* F4 */        NONE,
/* F5 */        NONE,
/* F6 */        NONE,
/* F7 */        NONE,
/* F8 */        NONE,
/* F9 */        NONE,
/* F10 */       NONE,
/* ea */        NONE,
/* eb */        NONE,
/* ec */        NONE,
/* ed */        NONE,
/* ee */        NONE,
/* ef */        NONE,
/* PF1 */       NONE,
/* PF2 */       NONE,
/* PF3 */       NONE,
/* PF4 */       NONE,
/* PF5 */       NONE,
/* PF6 */       NONE,
/* PF7 */       NONE,
/* PF8 */       NONE,
/* PF9 */       NONE,
/* PF10 */      NONE,
/* HIRAGANA */  CANNA_FN_BaseHiragana,
/* KATAKANA */  CANNA_FN_BaseKatakana,
/* HAN-ZEN */   CANNA_FN_BaseZenHanToggle,
/* EISU */      CANNA_FN_BaseEisu,
/* fe */        NONE,
/* ff */        NONE,
};
