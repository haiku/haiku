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
static char rcs_id[] = "@(#) 102.1 $Id: hex.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif /* lint */

#ifndef NO_EXTEND_MENU
#include "canna.h"

#define HEXPROMPT "\245\263\241\274\245\311: "
#define HEXPROMPTLEN  5 /* "¥³¡¼¥É: " ¤ÎÄ¹¤µ¤Ï5¥Ð¥¤¥È */

static int hexEveryTimeCatch(uiContext d, int retval, mode_context env);
static int exitHex(uiContext d, int retval, mode_context env);
static int quitHex(uiContext d, int retval, mode_context env);
static int hexMode(uiContext d, int major_mode);

static int quitHex();

/* cfuncdef

  hexEveryTimeCatch -- ÆÉ¤ß¤ò£±£¶¿ÊÆþÎÏ¥â¡¼¥É¤ÇÉ½¼¨¤¹¤ë´Ø¿ô

 */

static int
hexEveryTimeCatch(uiContext d, int retval, mode_context env)
     /* ARGSUSED */
{
  yomiContext yc = (yomiContext)d->modec;
  static WCHAR_T buf[256];
  /* ??? ¤³¤Î¤è¤¦¤Ê¥Ð¥Ã¥Õ¥¡¤ò¤¤¤í¤¤¤í¤ÊÉôÊ¬¤Ç»ý¤Ä¤Î¤Ï¹¥¤Þ¤·¤¯¤Ê¤¤¤Î¤Ç¡¢
     uiContext ¤Ë¤Þ¤È¤á¤Æ»ý¤Ã¤Æ¶¦Í­¤·¤Æ»È¤Ã¤¿Êý¤¬ÎÉ¤¤ */
  int codelen = d->kanji_status_return->length;

  d->kanji_status_return->info &= ~(KanjiThroughInfo | KanjiEmptyInfo);

  if (codelen >= 0) {
    MBstowcs(buf, HEXPROMPT, 256);
    WStrncpy(buf + HEXPROMPTLEN, d->kanji_status_return->echoStr, codelen);
    d->kanji_status_return->gline.line = buf;
    d->kanji_status_return->gline.length = codelen + HEXPROMPTLEN;
    d->kanji_status_return->gline.revPos =
      d->kanji_status_return->revPos + HEXPROMPTLEN;
    d->kanji_status_return->gline.revLen = d->kanji_status_return->revLen;
    d->kanji_status_return->info |= KanjiGLineInfo;
    echostrClear(d);
    if (codelen == 4) { /* £´Ê¸»ú¤Ë¤Ê¤Ã¤¿¤È¤­¤Ë¤Ï.... */
      if (convertAsHex(d)) {
	yc->allowedChars = CANNA_NOTHING_ALLOWED;
	*(d->kanji_status_return->echoStr = yc->kana_buffer + yc->kEndp + 1)
	  = *(d->buffer_return);
	d->kanji_status_return->revPos = d->kanji_status_return->revLen = 0;
	d->kanji_status_return->length = 1;
	retval = 0;
	if (cannaconf.hexCharacterDefiningStyle != HEX_USUAL) {
	  d->more.todo = 1;
	  d->more.ch = d->ch;
	  d->more.fnum = CANNA_FN_Kakutei;
	}
      }else{
	CannaBeep();
	d->more.todo = 1;
	d->more.ch = d->ch;
	d->more.fnum = CANNA_FN_DeletePrevious;
      }
    }
    else {
      yc->allowedChars = CANNA_ONLY_HEX;
    }
  }
  checkGLineLen(d);
  return retval;
}

static int
exitHex(uiContext d, int retval, mode_context env)
{
  killmenu(d);
  if (cvtAsHex(d, d->buffer_return, d->buffer_return, d->nbytes)) {
    GlineClear(d);
    popCallback(d);
    retval = YomiExit(d, 1);
    currentModeInfo(d);
    return retval;
  }
  else {
    return quitHex(d, 0, env);
  }
}

static int
quitHex(uiContext d, int retval, mode_context env)
     /* ARGSUSED */
{
  GlineClear(d);
  popCallback(d);
  currentModeInfo(d);
  return prevMenuIfExist(d);
}

yomiContext GetKanjiString();

static int
hexMode(uiContext d, int major_mode)
{
  yomiContext yc;

  yc = GetKanjiString(d, (WCHAR_T *)NULL, 0,
		      CANNA_ONLY_HEX,
		      (int)CANNA_YOMI_CHGMODE_INHIBITTED,
		      (int)CANNA_YOMI_END_IF_KAKUTEI,
		      CANNA_YOMI_INHIBIT_ALL,
		      hexEveryTimeCatch, exitHex, quitHex);
  if (yc == (yomiContext)0) {
    return NoMoreMemory();
  }
  yc->majorMode = major_mode;
  yc->minorMode = CANNA_MODE_HexMode;
  currentModeInfo(d);
  return 0;
}

/* cfuncdef

  HexMode -- £±£¶¿ÊÆþÎÏ¥â¡¼¥É¤Ë¤Ê¤ë¤È¤­¤Ë¸Æ¤Ð¤ì¤ë¡£

 */

int HexMode(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    

  return hexMode(d, CANNA_MODE_HexMode);
}

#endif /* NO_EXTEND_MENU */
