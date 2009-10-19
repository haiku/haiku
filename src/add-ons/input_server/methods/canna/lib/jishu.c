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
static char rcs_id[] = "@(#) 102.1 $Id: jishu.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif /* lint */

#include "canna.h"
#include <ctype.h>

extern struct RkRxDic *romajidic, *englishdic;

extern int WToupper (WCHAR_T);
static void setInhibitInformation (yomiContext);
static void jishuAdjustRome (uiContext), jishuAdjustRome (uiContext);
static int JishuZenkaku();
static int JishuHankaku();

/* yc->jishu_kc          ²¿¤ÎÊ¸»ú¼ï¤«
 * d->jishu_rEndp
 * d->jishu_kEndp
 * ¡ÚÎã¡Û 
 *               ¤¢¤¤¤·sh|
 * C-            ¥¢¥¤¥·sh|
 * C-            ¥¢¥¤¥·s|h
 * C-            ¥¢¥¤¥·|sh
 * C-            ¥¢¥¤|¤·sh
 *
 *               ¤¢¤¤¤·sh|
 * C-            aishish|
 * C-            £á£é£ó£è£é£ó£è|
 * C-            £á£é£ó£è£é£ó|h
 * C-            £á£é£ó£è£é|sh
 * C-            £á£é£ó£è|¤¤sh
 * C-            aish|¤¤sh
 * C-            ¤¢¤¤sh|¤¤sh
 * C-            ¥¢¥¤sh|sh
 * C-            ¥¢¥¤s|¤Òsh
 * C-            ¥¢¥¤|¤·sh
 * C-
 * 
 */

#define	INHIBIT_HANKATA	0x01
#define	INHIBIT_KANA	0x02
#define INHIBIT_ALPHA	0x04
#define INHIBIT_HIRA	0x08

static void setInhibitInformation(yomiContext yc);
static int inhibittedJishu(uiContext d);
static int nextJishu(uiContext d);
static int previousJishu(uiContext d);
static int JishuNextJishu(uiContext d);
static int JishuPreviousJishu(uiContext d);
static int JishuRotateWithInhibition(uiContext d, unsigned inhibit);
static int JishuKanaRotate(uiContext d);
static int JishuRomajiRotate(uiContext d);
static int JishuShrink(uiContext d);
static int JishuNop(uiContext d);
static int JishuExtend(uiContext d);
static void jishuAdjustRome(uiContext d);
static void myjishuAdjustRome(uiContext d);
static int JishuZenkaku(uiContext d);
static int JishuHankaku(uiContext d);
static int exitJishuAndDoSomething(uiContext d, int fnum);
static int JishuYomiInsert(uiContext d);
static int JishuQuit(uiContext d);
static int JishuToUpper(uiContext d);
static int JishuCapitalize(uiContext d);
static int JishuToLower(uiContext d);
static int JishuHiragana(uiContext d);
static int JishuKatakana(uiContext d);
static int JishuRomaji(uiContext d);
static void nextCase(yomiContext yc);
static int JishuCaseRotateForward(uiContext d);
static int JishuKanjiHenkan(uiContext d);
static int JishuKanjiHenkanOInsert(uiContext d);
static int JishuKanjiHenkanONothing(uiContext d);

void
enterJishuMode(uiContext d, yomiContext yc)
{
  extern KanjiModeRec jishu_mode;
  int pos;

  yc->jishu_kc = JISHU_HIRA;/* º£¤Ï¤Ò¤é¤¬¤Ê¥â¡¼¥É¤Ç¤¹ */
  yc->jishu_case = 0; /* Case ÊÑ´¹¤Ê¤·¤Î¥â¡¼¥É¤Ç¤¹ */
  setInhibitInformation(yc);
  if (yc->cmark < yc->cStartp) {
    yc->cmark = yc->cStartp;
  }
  if (yc->kCurs == yc->cmark) {
    yc->jishu_kEndp = yc->kEndp;
    yc->jishu_rEndp = yc->rEndp;
  }
  else if (yc->kCurs < yc->cmark) {
    int rpos;

    yc->jishu_kEndp = yc->cmark;
    yc->cmark = yc->kCurs;
    yc->kRStartp = yc->kCurs = yc->jishu_kEndp;
    kPos2rPos(yc, 0, yc->kCurs, (int *)0, &rpos);
    yc->jishu_rEndp = yc->rStartp = yc->rCurs = rpos;
  }
  else {
    yc->jishu_kEndp = yc->kCurs;
    yc->jishu_rEndp = yc->rCurs;
  }
/*  yc->majorMode = d->majorMode; */
  kPos2rPos(yc, 0, (int)yc->cmark, (int *)0, &pos);
  yc->rmark = (short)pos;
  d->current_mode = yc->curMode = &jishu_mode;
}

void
leaveJishuMode(uiContext d, yomiContext yc)
{
  extern KanjiModeRec yomi_mode, cy_mode;

  yc->jishu_kEndp = 0;
  if (yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) {
    d->current_mode = yc->curMode = &cy_mode;
  }
  else {
    d->current_mode = yc->curMode = &yomi_mode;
  }
  yc->minorMode = getBaseMode(yc);
  currentModeInfo(d);
}

static void
setInhibitInformation(yomiContext yc)
{
  int i;

  yc->inhibition = cannaconf.InhibitHankakuKana ? INHIBIT_HANKATA : 0;
  for (i = 0 ; i < yc->kEndp ; i++) {
    if ( !(yc->kAttr[i] & HENKANSUMI) && WIsG0(yc->kana_buffer[i]) ) {
      yc->inhibition |= INHIBIT_KANA;
      break;
    }
  }
  for (i = 0 ; i < yc->rEndp ; i++) {
    if (!WIsG0(yc->romaji_buffer[i])) {
      yc->inhibition |= INHIBIT_ALPHA;
    }
  }
}

int extractJishuString(yomiContext yc, WCHAR_T *s, WCHAR_T *e, WCHAR_T **sr, WCHAR_T **er)
{
  WCHAR_T *ss = s;
  int jishulen, len, revlen;
#ifndef WIN
  WCHAR_T xxxx[1024], yyyy[1024];
#else
  WCHAR_T *xxxx, *yyyy;
  xxxx = (WCHAR_T *)malloc(sizeof(WCHAR_T) * 1024);
  yyyy = (WCHAR_T *)malloc(sizeof(WCHAR_T) * 1024);
  if (!xxxx || !yyyy) {
    if (xxxx) {
      free(xxxx);
    }
    if (yyyy) {
      free(yyyy);
    }
    return 0;
  }
#endif

  if (s + yc->cmark - yc->cStartp < e) {
    WStrncpy(s, yc->kana_buffer + yc->cStartp, yc->cmark - yc->cStartp);
    s += yc->cmark - yc->cStartp;
  }
  else {
    WStrncpy(s, yc->kana_buffer + yc->cStartp, (int)(e - s));
    s = e;
  }

  if ((yc->jishu_kc == JISHU_ZEN_KATA ||
       yc->jishu_kc == JISHU_HAN_KATA ||
       yc->jishu_kc == JISHU_HIRA)) {
    int i, j, m, n, t, r;
    WCHAR_T *p = yyyy;
    for (i = yc->cmark ; i < yc->jishu_kEndp ;) {
      if (yc->kAttr[i] & STAYROMAJI) {
	j = i++;
	while (i < yc->jishu_kEndp && (yc->kAttr[i] & STAYROMAJI)) {
	  i++;
	}
	t = r = 0;
	while (j < i) {
	  int st = t;
	  WStrncpy(xxxx + t, yc->kana_buffer + j, i - j);
	  RkwMapPhonogram(yc->romdic, p, 1024 - (int)(p - yyyy), 
			  xxxx, st + i - j, xxxx[0],
			  RK_FLUSH | RK_SOKON, &n, &m, &t, &r);
	  /* RK_SOKON ¤òÉÕ¤±¤ë¤Î¤Ïµì¼­½ñÍÑ */
	  p += m;
	  j += n - st;
	  WStrncpy(xxxx, p, t);
	}
      }else{
	*p++ = yc->kana_buffer[i++];
      }
    }
    jishulen = p - yyyy;
  }

  switch (yc->jishu_kc)
    {
    case JISHU_ZEN_KATA: /* Á´³Ñ¥«¥¿¥«¥Ê¤ËÊÑ´¹¤¹¤ë */
      len = RkwCvtZen(xxxx, 1024, yyyy, jishulen);
      revlen = RkwCvtKana(s, (int)(e - s), xxxx, len);
      break;

    case JISHU_HAN_KATA: /* È¾³Ñ¥«¥¿¥«¥Ê¤ËÊÑ´¹¤¹¤ë */
      len = RkwCvtKana(xxxx, 1024, yyyy, jishulen);
      revlen = RkwCvtHan(s, (int)(e - s), xxxx, len);
      break;

    case JISHU_HIRA: /* ¤Ò¤é¤¬¤Ê¤ËÊÑ´¹¤¹¤ë */
      len = RkwCvtZen(xxxx, 1024, yyyy, jishulen);
      revlen = RkwCvtHira(s, (int)(e - s), xxxx, len);
      break;

    case JISHU_ZEN_ALPHA: /* Á´³Ñ±Ñ¿ô¤ËÊÑ´¹¤¹¤ë */
      if (yc->jishu_case == CANNA_JISHU_UPPER ||
	  yc->jishu_case == CANNA_JISHU_LOWER ||
	  yc->jishu_case == CANNA_JISHU_CAPITALIZE) {
	int i, head = 1;
	WCHAR_T *p = yc->romaji_buffer;

	for (i = yc->rmark ; i < yc->jishu_rEndp ; i++) {
	  xxxx[i - yc->rmark] =
	    (yc->jishu_case == CANNA_JISHU_UPPER) ? WToupper(p[i]) :
	    (yc->jishu_case == CANNA_JISHU_LOWER) ? WTolower(p[i]) : p[i];
	  if (yc->jishu_case == CANNA_JISHU_CAPITALIZE) {
	    if (p[i] <= ' ') {
	      head = 1;
	    }
	    else if (head) {
	      head = 0;
	      xxxx[i - yc->rmark] = WToupper(p[i]);
	    }
	  }
	}
	xxxx[yc->jishu_rEndp - yc->rmark] = 0;
	revlen = RkwCvtZen(s, (int)(e - s), xxxx, 
			   yc->jishu_rEndp - yc->rmark);
#if 0
      } else if (yc->jishu_case == CANNA_JISHU_CAPITALIZE) {
	WStrncpy(xxxx, yc->romaji_buffer + yc->rmark,
		 yc->jishu_rEndp - yc->rmark);
	*xxxx = WToupper(*xxxx);
	xxxx[yc->jishu_rEndp - yc->rmark] = 0;
	revlen = RkwCvtZen(s, (int)(e - s), xxxx, 
			   yc->jishu_rEndp - yc->rmark);
#endif
      } else {
	revlen = RkwCvtZen(s, (int)(e - s), yc->romaji_buffer + yc->rmark,
			   yc->jishu_rEndp - yc->rmark);
      }
      break;

    case JISHU_HAN_ALPHA: /* È¾³Ñ±Ñ¿ô¤ËÊÑ´¹¤¹¤ë */
      revlen = yc->jishu_rEndp - yc->rmark;
      if (yc->jishu_case == CANNA_JISHU_UPPER ||
	  yc->jishu_case == CANNA_JISHU_LOWER ||
	  yc->jishu_case == CANNA_JISHU_CAPITALIZE) {
	int i, head = 1;
	WCHAR_T *p = yc->romaji_buffer + yc->rmark;

	for (i = 0 ; i < revlen && s < e ; i++) {
	  *s++ =
	    (yc->jishu_case == CANNA_JISHU_UPPER) ? WToupper(p[i]) :
	    (yc->jishu_case == CANNA_JISHU_LOWER) ? WTolower(p[i]) : p[i];
	  if (yc->jishu_case == CANNA_JISHU_CAPITALIZE) {
	    if (p[i] <= ' ') {
	      head = 1;
	    }
	    else if (head) {
	      head = 0;
	      s[-1] = WToupper(p[i]);
	    }
	  }
	}
	s -= revlen;
#if 0
      } else if (yc->jishu_case == CANNA_JISHU_CAPITALIZE) {
	if (s + revlen < e) {
	  WStrncpy(s, yc->romaji_buffer + yc->rmark, revlen);
	}
	else {
	  WStrncpy(s, yc->romaji_buffer + yc->rmark, (int)(e - s));
	  revlen = (int)(e - s);
	}
	*s = WToupper(yc->romaji_buffer[yc->rmark]);
#endif
      }
      else if (s + revlen < e) {
	WStrncpy(s, yc->romaji_buffer + yc->rmark, revlen);
      }else{
	WStrncpy(s, yc->romaji_buffer + yc->rmark, (int)(e - s));
	revlen = (int)(e - s);
      }
      break;

    default:/* ¤É¤ì¤Ç¤â¤Ê¤«¤Ã¤¿¤éÊÑ´¹½ÐÍè¤Ê¤¤¤Î¤Ç²¿¤â¤·¤Ê¤¤ */
      break;
    }

  *sr = s;
  s += revlen;
  *er = s;

/* Ê¸»ú¼ïÊÑ´¹¤·¤Ê¤¤ÉôÊ¬¤òÉÕ¤±²Ã¤¨¤ë */
  switch (yc->jishu_kc)
    {
    case JISHU_HIRA: /* ¤Ò¤é¤¬¤Ê¤Ê¤é */
    case JISHU_ZEN_KATA: /* Á´³Ñ¥«¥¿¥«¥Ê¤Ê¤é */
    case JISHU_HAN_KATA: /* È¾³Ñ¥«¥¿¥«¥Ê¤Ê¤é */
      /* ¤«¤Ê¥Ð¥Ã¥Õ¥¡¤«¤éÊ¸»úÎó¤ò¼è¤ê½Ð¤¹ */
      if (s + yc->kEndp - yc->jishu_kEndp < e) {
	WStrncpy(s, yc->kana_buffer + yc->jishu_kEndp, 
		 yc->kEndp - yc->jishu_kEndp);
	s += yc->kEndp - yc->jishu_kEndp;
      }else{
	WStrncpy(s, yc->kana_buffer + yc->jishu_kEndp, (int)(e - s));
	s = e;
      }
      break;

    case JISHU_ZEN_ALPHA: /* Á´³Ñ±Ñ¿ô¤Ê¤é */
    case JISHU_HAN_ALPHA: /* È¾³Ñ±Ñ¿ô¤Ê¤é */
      len = RkwCvtRoma(romajidic, s, (int)(e - s),
		       yc->romaji_buffer + yc->jishu_rEndp,
		       yc->rEndp - yc->jishu_rEndp,
		       RK_FLUSH | RK_SOKON | RK_XFER);
      s += len;
      break;
    default:/* ¤É¤ì¤Ç¤â¤Ê¤«¤Ã¤¿¤é²¿¤â¤·¤Ê¤¤ */
      break;
    }

  if (s < e) {
    *s = (WCHAR_T)0;
  }
#ifdef WIN
  free(xxxx);
  free(yyyy);
#endif
  return (int)(s - ss);
}

static
int inhibittedJishu(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  return (((yc->inhibition & INHIBIT_KANA) &&
	   (yc->jishu_kc == JISHU_ZEN_KATA ||
	    yc->jishu_kc == JISHU_HAN_KATA)) ||
	  ((yc->inhibition & INHIBIT_ALPHA) &&
	   (yc->jishu_kc == JISHU_ZEN_ALPHA ||
	    yc->jishu_kc == JISHU_HAN_ALPHA)) ||
	  ((yc->inhibition & INHIBIT_HANKATA) &&
	   (yc->jishu_kc == JISHU_HAN_KATA))
	  );
}

static
int nextJishu(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  BYTE startkc = yc->jishu_kc;

  do {
    yc->jishu_kc = (BYTE)(((int)yc->jishu_kc + 1) % MAX_JISHU);
  } while (inhibittedJishu(d) && yc->jishu_kc != startkc);
  return yc->jishu_kc != startkc;
}

static
int previousJishu(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  BYTE startkc = yc->jishu_kc;

  do {
    yc->jishu_kc = (unsigned char)
      (((int)yc->jishu_kc + MAX_JISHU - 1) % MAX_JISHU);
  } while (inhibittedJishu(d) && yc->jishu_kc != startkc);
  return yc->jishu_kc != startkc;
}

static int JishuNextJishu (uiContext);	    

static
int JishuNextJishu(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

/* ¼è¤ê½Ð¤·¤¿Ê¸»úÎó¤òÊÑ´¹¤¹¤ë */
  if (!nextJishu(d)) {
    return NothingChangedWithBeep(d);
  }
  if (yc->jishu_kc == JISHU_HIRA) {
    if (yc->jishu_kEndp == yc->kEndp && yc->jishu_rEndp == yc->rEndp) {
      leaveJishuMode(d, yc);
    }
  }
  makeKanjiStatusReturn(d, yc);
  return 0;
}

static int JishuPreviousJishu (uiContext);

static
int JishuPreviousJishu(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

/* ¼è¤ê½Ð¤·¤¿Ê¸»úÎó¤òÊÑ´¹¤¹¤ë */
  if (!previousJishu(d)) {
    return NothingChangedWithBeep(d);
  }
  if (yc->jishu_kc == JISHU_HIRA) {
    if (yc->jishu_kEndp == yc->kEndp && yc->jishu_rEndp == yc->rEndp) {
      leaveJishuMode(d, yc);
    }
  }
  makeKanjiStatusReturn(d, yc);
  return 0;
}

static int JishuRotateWithInhibition (uiContext, unsigned);

static
int JishuRotateWithInhibition(uiContext d, unsigned inhibit)
{
  yomiContext yc = (yomiContext)d->modec;
  BYTE savedInhibition = yc->inhibition;
  int res;

  yc->inhibition |= inhibit;
  
  res = JishuNextJishu(d);
  yc->inhibition = savedInhibition;
  return res;
}

static int JishuKanaRotate (uiContext);

static
int JishuKanaRotate(uiContext d)
{
  return JishuRotateWithInhibition(d, INHIBIT_ALPHA);
}

static int JishuRomajiRotate (uiContext);

static
int JishuRomajiRotate(uiContext d)
{
  return JishuRotateWithInhibition(d, INHIBIT_KANA | INHIBIT_HIRA);
}

static void myjishuAdjustRome (uiContext);
static int JishuShrink (uiContext);

static
int JishuShrink(uiContext d)
{  
  yomiContext yc = (yomiContext)d->modec;

  /* ¼ï¡¹¤Î¥Ý¥¤¥ó¥¿¤òÌá¤¹ */
  switch (yc->jishu_kc)
    {
    case JISHU_ZEN_ALPHA:
    case JISHU_HAN_ALPHA: /* Á´³Ñ±Ñ¿ô»ú¤«È¾³Ñ±Ñ¿ô»ú¤Ê¤é */
      myjishuAdjustRome(d);
      yc->jishu_rEndp--; /* »ú¼ï¥í¡¼¥Þ»ú¥Ð¥Ã¥Õ¥¡¥¤¥ó¥Ç¥Ã¥¯¥¹¤ò£±Ìá¤¹ */
      if (yc->rAttr[yc->jishu_rEndp] & SENTOU) {
	                       /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹ÀèÆ¬¥Õ¥é¥°¥Ð¥Ã¥Õ¥¡¤¬
				* Î©¤Ã¤Æ¤¤¤¿¤é
			        */
	for (--yc->jishu_kEndp ; 
	     yc->jishu_kEndp > 0 && !(yc->kAttr[yc->jishu_kEndp] & SENTOU) ;) {
	  --yc->jishu_kEndp;
	}
	                       /* ¤«¤ÊÊÑ´¹¤·¤¿¥Õ¥é¥°¥Ð¥Ã¥Õ¥¡¤ÎÀèÆ¬¤¬
				* Î©¤Ã¤Æ¤¤¤ë½ê¤Þ¤Ç
				* »ú¼ï¤«¤Ê¥Ð¥Ã¥Õ¥¡¥¤¥ó¥Ç¥Ã¥¯¥¹¤ò
				* Ìá¤¹
			        */
      }
      break;
    case JISHU_HIRA:
    case JISHU_ZEN_KATA:
    case JISHU_HAN_KATA: /* ¤Ò¤é¤¬¤Ê¤«Á´³Ñ¥«¥¿¥«¥Ê¤«È¾³Ñ¥«¥¿¥«¥Ê¤Ê¤é */
      jishuAdjustRome(d);
      yc->jishu_kEndp--; /* »ú¼ï¤«¤Ê¥Ð¥Ã¥Õ¥¡¥¤¥ó¥Ç¥Ã¥¯¥¹¤ò£±Ê¸»úÊ¬Ìá¤¹ */
      if (yc->kAttr[yc->jishu_kEndp] & SENTOU) {
                               /* ¤«¤ÊÊÑ´¹¤·¤¿¥Õ¥é¥°¥Ð¥Ã¥Õ¥¡¤¬
				* Î©¤Ã¤Æ¤¤¤¿¤é
			        */
	for (--yc->jishu_rEndp ; 
	     yc->jishu_rEndp > 0 && !(yc->rAttr[yc->jishu_rEndp] & SENTOU) ;) {
	  --yc->jishu_rEndp;
	}
	                       /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹ÀèÆ¬¥Õ¥é¥°¥Ð¥Ã¥Õ¥¡¤¬
				* Î©¤Ã¤Æ¤¤¤ë½ê¤Þ¤Ç
				* »ú¼ï¥í¡¼¥Þ»ú¥Ð¥Ã¥Õ¥¡¥¤¥ó¥Ç¥Ã¥¯¥¹¤ò
				* Ìá¤¹
			        */
      }
      break;
    }

  if(yc->jishu_rEndp <= yc->rmark) {/* £±¼þ¤·¤¿¤é»ú¼ï¥Ð¥Ã¥Õ¥¡¥¤¥ó¥Ç¥Ã¥¯¥¹¤ò
				     * ¸µ¤ÎÄ¹¤µ¤ËÌá¤¹
				     */
    yc->jishu_kEndp = yc->kEndp;
    yc->jishu_rEndp = yc->rEndp;
  }
  makeKanjiStatusReturn(d, yc);
  return 0;
}

static int JishuNop (uiContext);

static
int JishuNop(uiContext d)
{
  /* currentModeInfo ¤Ç¥â¡¼¥É¾ðÊó¤¬É¬¤ºÊÖ¤ë¤è¤¦¤Ë¥À¥ß¡¼¤Î¥â¡¼¥É¤òÆþ¤ì¤Æ¤ª¤¯ */
  d->majorMode = d->minorMode = CANNA_MODE_AlphaMode;
  currentModeInfo(d);

  makeKanjiStatusReturn(d, (yomiContext)d->modec);
  return 0;
}

static int JishuExtend (uiContext);

static
int JishuExtend(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  /* ¼ï¡¹¤Î¥Ý¥¤¥ó¥¿¤òÁý¤ä¤¹ */
  switch (yc->jishu_kc) {
    case JISHU_ZEN_ALPHA:
    case JISHU_HAN_ALPHA: /* Á´³Ñ±Ñ¿ô»ú¤«È¾³Ñ±Ñ¿ô»ú¤Ê¤é */
      myjishuAdjustRome(d);

      if(yc->jishu_rEndp >= yc->rEndp && yc->jishu_kEndp >= yc->kEndp ) {
                                    /* £±¼þ¤·¤¿¤é»ú¼ï¥Ð¥Ã¥Õ¥¡¥¤¥ó¥Ç¥Ã¥¯¥¹¤ò
				     * °ìÈÖÁ°¤ËÌá¤¹
				     */
	yc->jishu_rEndp = yc->rmark;
	yc->jishu_kEndp = yc->cmark;
      }

      if (yc->rAttr[yc->jishu_rEndp] & SENTOU) {
	                       /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹ÀèÆ¬¥Õ¥é¥°¥Ð¥Ã¥Õ¥¡¤¬
				* Î©¤Ã¤Æ¤¤¤¿¤é
			        */

	for (yc->jishu_kEndp++ ; 
	     yc->jishu_kEndp > 0 && !(yc->kAttr[yc->jishu_kEndp] & SENTOU) ;) {
	  yc->jishu_kEndp++;
	}
	                       /* ¤«¤ÊÊÑ´¹¤·¤¿¥Õ¥é¥°¥Ð¥Ã¥Õ¥¡¤ÎÀèÆ¬¤¬
				* Î©¤Ã¤Æ¤¤¤ë½ê¤Þ¤Ç
				* »ú¼ï¤«¤Ê¥Ð¥Ã¥Õ¥¡¥¤¥ó¥Ç¥Ã¥¯¥¹¤òÁý¤ä¤¹
			        */
      }
      yc->jishu_rEndp++; /* »ú¼ï¥í¡¼¥Þ»ú¥Ð¥Ã¥Õ¥¡¥¤¥ó¥Ç¥Ã¥¯¥¹¤ò£±Áý¤ä¤¹ */
      break;
    case JISHU_HIRA:
    case JISHU_ZEN_KATA:
    case JISHU_HAN_KATA: /* ¤Ò¤é¤¬¤Ê¤«Á´³Ñ¥«¥¿¥«¥Ê¤«È¾³Ñ¥«¥¿¥«¥Ê¤Ê¤é */
      jishuAdjustRome(d);

      if(yc->jishu_rEndp >= yc->rEndp && yc->jishu_kEndp >= yc->kEndp ) {
                                    /* £±¼þ¤·¤¿¤é»ú¼ï¥Ð¥Ã¥Õ¥¡¥¤¥ó¥Ç¥Ã¥¯¥¹¤ò
				     * °ìÈÖÁ°¤ËÌá¤¹
				     */
	yc->jishu_rEndp = yc->rmark;
	yc->jishu_kEndp = yc->cmark;
      }

      if (yc->kAttr[yc->jishu_kEndp] & SENTOU) {
                               /* ¤«¤ÊÊÑ´¹¤·¤¿¥Õ¥é¥°¥Ð¥Ã¥Õ¥¡¤¬
				* Î©¤Ã¤Æ¤¤¤¿¤é
			        */
	for (yc->jishu_rEndp++ ; 
	     yc->jishu_rEndp > 0 && !(yc->rAttr[yc->jishu_rEndp] & SENTOU) ;) {
	  yc->jishu_rEndp++;
	}
	                       /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹ÀèÆ¬¥Õ¥é¥°¥Ð¥Ã¥Õ¥¡¤¬
				* Î©¤Ã¤Æ¤¤¤ë½ê¤Þ¤Ç
				* »ú¼ï¥í¡¼¥Þ»ú¥Ð¥Ã¥Õ¥¡¥¤¥ó¥Ç¥Ã¥¯¥¹¤òÁý¤ä¤¹
			        */
      }
      yc->jishu_kEndp++; /* »ú¼ï¤«¤Ê¥Ð¥Ã¥Õ¥¡¥¤¥ó¥Ç¥Ã¥¯¥¹¤ò£±Ê¸»úÊ¬Áý¤ä¤¹ */
      break;
    }
  makeKanjiStatusReturn(d, yc);
  return 0;
}

static void
jishuAdjustRome(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  while (!(yc->rAttr[yc->jishu_rEndp] & SENTOU)) {
    ++yc->jishu_rEndp;
  }
}

static void
myjishuAdjustRome(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  while (!(yc->kAttr[yc->jishu_kEndp] & SENTOU)
	 && !(yc->jishu_kEndp == yc->kEndp)) {
    ++yc->jishu_kEndp;
  }
}

static int
JishuZenkaku(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

/* ¼è¤ê½Ð¤·¤¿Ê¸»úÎó¤òÊÑ´¹¤¹¤ë */
  switch(yc->jishu_kc)
    {
    case JISHU_HIRA: /* ¤Ò¤é¤¬¤Ê¤Ê¤é²¿¤â¤·¤Ê¤¤ */
      break;
      
    case JISHU_HAN_ALPHA: /* È¾³Ñ±Ñ¿ô¤Ê¤éÁ´³Ñ±Ñ¿ô¤ËÊÑ´¹¤¹¤ë */
      yc->jishu_kc = JISHU_ZEN_ALPHA;
      break;
      
    case JISHU_ZEN_ALPHA: /* Á´³Ñ±Ñ¿ô¤Ê¤é²¿¤â¤·¤Ê¤¤ */
      break;
      
    case JISHU_HAN_KATA: /* È¾³Ñ¥«¥¿¥«¥Ê¤Ê¤éÁ´³Ñ¥«¥¿¥«¥Ê¤ËÊÑ´¹¤¹¤ë */
      yc->jishu_kc = JISHU_ZEN_KATA;
      break;
      
    case JISHU_ZEN_KATA: /* Á´³Ñ¥«¥¿¥«¥Ê¤Ê¤é²¿¤â¤·¤Ê¤¤ */
      break;
      
    default: /* ¤É¤ì¤Ç¤â¤Ê¤«¤Ã¤¿¤éÊÑ´¹½ÐÍè¤Ê¤¤¤Î¤Ç²¿¤â¤·¤Ê¤¤ */
      break;
    }

  makeKanjiStatusReturn(d, yc);
  return 0;
}

static int
JishuHankaku(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  
  /* ¼è¤ê½Ð¤·¤¿Ê¸»úÎó¤òÊÑ´¹¤¹¤ë */
  switch(yc->jishu_kc)
    {
    case JISHU_HIRA: /* ¤Ò¤é¤¬¤Ê¤Ê¤éÈ¾³Ñ¥«¥¿¥«¥Ê¤ËÊÑ´¹¤¹¤ë */
      if (cannaconf.InhibitHankakuKana) {
	return NothingChangedWithBeep(d);
      }
      yc->jishu_kc = JISHU_HAN_KATA;
      break;
      
    case JISHU_ZEN_KATA: /* Á´³Ñ¥«¥¿¥«¥Ê¤Ê¤éÈ¾³Ñ¥«¥¿¥«¥Ê¤ËÊÑ´¹¤¹¤ë */
      if (cannaconf.InhibitHankakuKana) {
	return NothingChangedWithBeep(d);
      }
      yc->jishu_kc = JISHU_HAN_KATA;
      break;
      
    case JISHU_HAN_KATA: /* È¾³Ñ¥«¥¿¥«¥Ê¤Ê¤é²¿¤â¤·¤Ê¤¤ */
      break;
      
    case JISHU_ZEN_ALPHA: /* Á´³Ñ±Ñ¿ô¤Ê¤éÈ¾³Ñ±Ñ¿ô¤ËÊÑ´¹¤¹¤ë */
      yc->jishu_kc = JISHU_HAN_ALPHA;
      break;
      
    case JISHU_HAN_ALPHA: /* È¾³Ñ±Ñ¿ô¤Ê¤é²¿¤â¤·¤Ê¤¤ */
      break;
      
    default: /* ¤É¤ì¤Ç¤â¤Ê¤«¤Ã¤¿¤éÊÑ´¹½ÐÍè¤Ê¤¤¤Î¤Ç²¿¤â¤·¤Ê¤¤ */
      break;
    }

  makeKanjiStatusReturn(d, yc);
  return 0;
}

static
int exitJishuAndDoSomething(uiContext d, int fnum)
{
  exitJishu(d);
  d->more.todo = 1;
  d->more.ch = d->ch;
  d->more.fnum = fnum;
  makeYomiReturnStruct(d);
  currentModeInfo(d);
  return d->nbytes = 0;
}

static int JishuYomiInsert (uiContext);

static
int JishuYomiInsert(uiContext d)
{
  if (cannaconf.MojishuContinue) {
    return exitJishuAndDoSomething(d, 0);
  }
  else {
    int res;

    res = YomiKakutei(d);
    /* ¿·¤·¤¯¥Õ¥é¥°¤òÀß¤±¤Æ¡¢¤½¤ì¤Ë¤è¤Ã¤Æ¡¢
       YomiKakutei(); more.todo = self-insert ¤âÁªÂò¤Ç¤­¤ë¤è¤¦¤Ë¤·¤è¤¦ */
    d->more.todo = 1;
    d->more.ch = d->ch;
    d->more.fnum = CANNA_FN_FunctionalInsert;
    makeYomiReturnStruct(d);
    currentModeInfo(d);
    return res;
  }
}

static int JishuQuit (uiContext);

static
int JishuQuit(uiContext d)
{
  leaveJishuMode(d, (yomiContext)d->modec);
  makeKanjiStatusReturn(d, (yomiContext)d->modec);
  return 0;
}

/* ÂçÊ¸»ú¤Ë¤¹¤ë´Ø¿ô */

static int JishuToUpper (uiContext);

static
int JishuToUpper(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (!(yc->inhibition & INHIBIT_ALPHA)) { /* ÌµÍýÌðÍýÂçÊ¸»ú¤ËÊÑ´¹¤¹¤ë */
    if (yc->jishu_kc == JISHU_HIRA || yc->jishu_kc == JISHU_ZEN_KATA) {
      yc->jishu_kc = JISHU_ZEN_ALPHA;
    }
    else if (yc->jishu_kc == JISHU_HAN_KATA) {
      yc->jishu_kc = JISHU_HAN_ALPHA;
    }
  }

  if (yc->jishu_kc == JISHU_ZEN_ALPHA || yc->jishu_kc == JISHU_HAN_ALPHA) {
    yc->jishu_case = CANNA_JISHU_UPPER;
    makeKanjiStatusReturn(d, yc);
    return 0;
  }
  else {
    /* Á°¤È²¿¤âÊÑ¤ï¤ê¤Þ¤»¤ó */
    d->kanji_status_return->length = -1;
    return 0;
  }
}

static int JishuCapitalize (uiContext);

static
int JishuCapitalize(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (!(yc->inhibition & INHIBIT_ALPHA)) { /* ÌµÍýÌðÍýÂçÊ¸»ú¤ËÊÑ´¹¤¹¤ë */
    if (yc->jishu_kc == JISHU_HIRA || yc->jishu_kc == JISHU_ZEN_KATA) {
      yc->jishu_kc = JISHU_ZEN_ALPHA;
    }
    else if (yc->jishu_kc == JISHU_HAN_KATA) {
      yc->jishu_kc = JISHU_HAN_ALPHA;
    }
  }

  if (yc->jishu_kc == JISHU_ZEN_ALPHA || yc->jishu_kc == JISHU_HAN_ALPHA) {
    yc->jishu_case = CANNA_JISHU_CAPITALIZE;
    makeKanjiStatusReturn(d, yc);
    return 0;
  }
  else {
    /* Á°¤È²¿¤âÊÑ¤ï¤ê¤Þ¤»¤ó */
    d->kanji_status_return->length = -1;
    return 0;
  }
}

static int JishuToLower (uiContext);

static
int JishuToLower(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (!(yc->inhibition & INHIBIT_ALPHA)) { /* ÌµÍýÌðÍýÂçÊ¸»ú¤ËÊÑ´¹¤¹¤ë */
    if (yc->jishu_kc == JISHU_HIRA || yc->jishu_kc == JISHU_ZEN_KATA) {
      yc->jishu_kc = JISHU_ZEN_ALPHA;
    }
    else if (yc->jishu_kc == JISHU_HAN_KATA) {
      yc->jishu_kc = JISHU_HAN_ALPHA;
    }
  }

  if (yc->jishu_kc == JISHU_ZEN_ALPHA || yc->jishu_kc == JISHU_HAN_ALPHA) {
    yc->jishu_case = CANNA_JISHU_LOWER;
    makeKanjiStatusReturn(d, yc);
    return 0;
  }
  else {
    /* Á°¤È²¿¤âÊÑ¤ï¤ê¤Þ¤»¤ó */
    d->kanji_status_return->length = -1;
    return 0;
  }
}

static int JishuHiragana (uiContext);

static
int JishuHiragana(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  yc->jishu_kc = JISHU_HIRA;
  makeKanjiStatusReturn(d, yc);
  return 0;
}

static int JishuKatakana (uiContext);

static
int JishuKatakana(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  yc->jishu_kc = JISHU_ZEN_KATA;
  makeKanjiStatusReturn(d, yc);
  return 0;
}

static int JishuRomaji (uiContext);

static
int JishuRomaji(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->inhibition == INHIBIT_ALPHA) {
    return NothingChangedWithBeep(d);
  }
  yc->jishu_kc = JISHU_ZEN_ALPHA;
  makeKanjiStatusReturn(d, yc);
  return 0;
}

static void
nextCase(yomiContext yc)
{
  yc->jishu_case = (BYTE)(((int)yc->jishu_case + 1) % CANNA_JISHU_MAX_CASE);
}

static int JishuCaseRotateForward (uiContext);

static
int JishuCaseRotateForward(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->inhibition == INHIBIT_ALPHA) {
    return NothingChangedWithBeep(d);
  }
  if (yc->jishu_kc == JISHU_ZEN_ALPHA ||
      yc->jishu_kc == JISHU_HAN_ALPHA) {
    nextCase(yc);
  }
  else if (yc->jishu_kc == JISHU_HIRA || yc->jishu_kc == JISHU_ZEN_KATA) {
    yc->jishu_kc = JISHU_ZEN_ALPHA;
  }
  else if (yc->jishu_kc == JISHU_HAN_KATA) {
    yc->jishu_kc = JISHU_HAN_ALPHA;
  }
  makeKanjiStatusReturn(d, yc);
  return 0;
}

/*
 * ¤«¤Ê´Á»úÊÑ´¹¤ò¹Ô¤¤(ÊÑ´¹¥­¡¼¤¬½é¤á¤Æ²¡¤µ¤ì¤¿)¡¢TanKouhoMode¤Ë°Ü¹Ô¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */

static int JishuKanjiHenkan (uiContext);

static
int JishuKanjiHenkan(uiContext d)
{
  return exitJishuAndDoSomething(d, CANNA_FN_Henkan);
}

static int JishuKanjiHenkanOInsert (uiContext);

static
int JishuKanjiHenkanOInsert(uiContext d)
{
  return exitJishuAndDoSomething(d, CANNA_FN_HenkanOrInsert);
}

static int JishuKanjiHenkanONothing (uiContext);

static
int JishuKanjiHenkanONothing(uiContext d)
{
  return exitJishuAndDoSomething(d, CANNA_FN_HenkanOrNothing);
}

#include "jishumap.h"
