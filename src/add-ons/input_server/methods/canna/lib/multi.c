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
static char m_s_map_id[] = "@(#) 102.1 $Id: multi.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif /* lint */

#include "canna.h"
#include <canna/mfdef.h>

#define NONE CANNA_FN_Undefined

static unsigned char *showChar(int c);
static int _DoFuncSequence(uiContext d, BYTE *keytbl, BYTE key);

extern int askQuitKey();
extern int checkGLineLen();
extern int NothingChangedWithBeep();

static unsigned char *keyHistory;

struct map {
  KanjiMode tbl;
  unsigned char key;
  KanjiMode mode;
  struct map *next;
};

extern struct map *mapFromHash();

static unsigned char *
showChar(int c)
{
  static unsigned char Gkey[9];
  static char *keyCharMap[] = {               
    "space",    "DEL",      "Nfer",     "Xfer",     "Up",
    "Left",     "Right",    "Down",     "Insert",   "Rollup",
    "Rolldown", "Home",     "HELP",     "KeyPad",   "S-nfer",
    "S-xfer",   "S-up",     "S-left",   "S-right",  "S-down",
    "C-nfer",   "C-xfer",   "C-up",     "C-left",   "C-right",
    "C-down",   "F1",       "F2",       "F3",       "F4",
    "F5",       "F6",       "F7",       "F8",       "F9",
    "F10",      "PF1",      "PF2",      "PF3",      "PF4",
    "PF5",      "PF6",      "PF7",      "PF8",      "PF9",
    "PF10",
  };

  if (c < 0x20) {
    strcpy((char *)Gkey, "C-");
    if (c == 0x00 || (c > 0x1a && c < 0x20 ))
      Gkey[2] = c + 0x40;
    else
      Gkey[2] = c + 0x60;
    Gkey[3] = '\0';
  }
  else if (c > ' ' && c <= '~' ) {
    Gkey[0] = c;
    Gkey[1] = '\0';
  }
  else if (c > 0xa0 && c < 0xdf) {
    Gkey[0] = 0x8e;
    Gkey[1] = c;
    Gkey[2] = '\0';
  }
  else if (c == 0x20)
    strcpy((char *)Gkey, keyCharMap[0]);
  else if (c > 0x7e && c < 0x8c)
    strcpy((char *)Gkey, keyCharMap[c -0x7f +1]);
  else if (c > 0x8f && c < 0x9c)
    strcpy((char *)Gkey, keyCharMap[c -0x90 +14]);
  else if (c > 0xdf && c < 0xea)
    strcpy((char *)Gkey, keyCharMap[c -0xe0 +26]);
  else if (c > 0xef && c < 0xfa)
    strcpy((char *)Gkey, keyCharMap[c -0xf0 +36]);
  else
    return 0;
  return Gkey;
}

int UseOtherKeymap(uiContext d)
{
  struct map *p;
  unsigned char showKey[10];
  
  strcpy((char *)showKey, (char *)showChar(d->ch));
  p = mapFromHash((KanjiMode)d->current_mode->keytbl,
		  d->ch, (struct map ***)0);
  if (p == (struct map *)NULL) 
    return NothingChangedWithBeep(d);
  p->mode->ftbl = (struct funccfunc *)d->current_mode;
  keyHistory = (unsigned char *)malloc(strlen((char *)showKey) + 1);
  if (keyHistory) {
    strcpy((char *)keyHistory,(char *)showKey);
    makeGLineMessageFromString(d, (char *)keyHistory);
    if (p->mode->keytbl == (BYTE *)NULL) {
      free(keyHistory);
      return NothingChangedWithBeep(d);
    }
    d->current_mode = p->mode;
  }
  return NothingForGLine(d);
}

static
int _DoFuncSequence(uiContext d, BYTE *keytbl, BYTE key)
{
  int res, total_res, ginfo = 0;
  int prevEchoLen = -1, prevRevPos, prevRevLen;
  int prevGEchoLen, prevGRevPos, prevGRevLen;
  WCHAR_T *prevEcho, *prevGEcho;
  BYTE *p;
  WCHAR_T *malloc_echo = (WCHAR_T *)0, *malloc_gline = (WCHAR_T *)0;

  if (key == 0) {
    key = (BYTE)d->ch;
  }
  if (keytbl == (BYTE *)NULL)
    keytbl = d->current_mode->keytbl;

  p = actFromHash(keytbl, key);

  if (p == (BYTE *)NULL) {
    return 0;
  }
    
  total_res = 0;
  for(; *p ; p++) {
    /* £²²óÌÜ°Ê¹ß¤Ë°Ê²¼¤Î¥Ç¡¼¥¿¤¬¼º¤ï¤ì¤Æ¤¤¤ë¾ì¹ç¤¬¤¢¤ë¤Î¤ÇÆþ¤ìÄ¾¤¹¡£ */
    d->ch = (unsigned)(*(d->buffer_return) = (WCHAR_T)key);
    d->nbytes = 1;
    res = _doFunc(d, (int)*p); /* À¸¤Î doFunc ¤ò¸Æ¤Ö¡£ */

    if (d->kanji_status_return->length >= 0) {
      prevEcho    = d->kanji_status_return->echoStr;
      prevEchoLen = d->kanji_status_return->length;
      prevRevPos  = d->kanji_status_return->revPos;
      prevRevLen  = d->kanji_status_return->revLen;
      if (d->genbuf <= prevEcho && prevEcho < d->genbuf + ROMEBUFSIZE) {
	/* ¥Ç¡¼¥¿¤Ï d->genbuf ¤Ë¤¢¤ë¤Í */
	if (!malloc_echo &&
	    !(malloc_echo =
	      (WCHAR_T *)malloc(ROMEBUFSIZE * sizeof(WCHAR_T)))) {
	  res = -1; /* ¥¨¥é¡¼¤¬¤â¤È¤â¤ÈÊÖ¤Ã¤ÆÍè¤¿¤È¤¤¤¦¤³¤È¤Ë¤¹¤ë */
	}
	else {
	  prevEcho = malloc_echo;
	  WStrncpy(prevEcho, d->kanji_status_return->echoStr, prevEchoLen);
	  prevEcho[prevEchoLen] = (WCHAR_T)0;
	  d->kanji_status_return->echoStr = prevEcho;
	}
      }
    }
    if (d->kanji_status_return->info & KanjiGLineInfo) {
      ginfo = 1;
      prevGEcho    = d->kanji_status_return->gline.line;
      prevGEchoLen = d->kanji_status_return->gline.length;
      prevGRevPos  = d->kanji_status_return->gline.revPos;
      prevGRevLen  = d->kanji_status_return->gline.revLen;
      if (d->genbuf <= prevGEcho && prevGEcho < d->genbuf + ROMEBUFSIZE) {
	/* ¥Ç¡¼¥¿¤Ï d->genbuf ¤Ë¤¢¤ë¤Í */
	if (!malloc_gline &&
	    !(malloc_gline =
	      (WCHAR_T *)malloc(ROMEBUFSIZE * sizeof(WCHAR_T)))) {
	  res = -1; /* ¥¨¥é¡¼¤¬¤â¤È¤â¤ÈÊÖ¤Ã¤ÆÍè¤¿¤È¤¤¤¦¤³¤È¤Ë¤¹¤ë */
	}
	else {
	  prevGEcho = malloc_gline;
	  WStrncpy(prevGEcho, d->kanji_status_return->gline.line,
		   prevGEchoLen);
	  prevGEcho[prevGEchoLen] = (WCHAR_T)0;
	  d->kanji_status_return->gline.line = prevGEcho;
	  d->kanji_status_return->info &= ~KanjiGLineInfo;
	}
      }
    }
    if (res < 0) {
      break;
    }
    if (res > 0) {
      total_res += res;
      d->buffer_return += res;
      d->n_buffer -= res;
    }
  }
  total_res = _afterDoFunc(d, total_res);
  d->flags |= MULTI_SEQUENCE_EXECUTED;
  if (malloc_echo) {
    WStrncpy(d->genbuf, prevEcho, prevEchoLen);
    d->genbuf[prevEchoLen] = (WCHAR_T)0;
    free(malloc_echo); /* Â¿Ê¬ malloc_echo ¤¬ prevEcho ¤«¤â */
    prevEcho = d->genbuf;
  }
  d->kanji_status_return->echoStr = prevEcho;
  d->kanji_status_return->length  = prevEchoLen;
  d->kanji_status_return->revPos  = prevRevPos;
  d->kanji_status_return->revLen  = prevRevLen;
  if (ginfo) {
    if (malloc_gline) {
      WStrncpy(d->genbuf, prevGEcho, prevGEchoLen);
      d->genbuf[prevGEchoLen] = (WCHAR_T)0;
      free(malloc_gline); /* Â¿Ê¬ malloc_gline ¤¬ prevGEcho ¤«¤â */
      prevGEcho = d->genbuf;
    }
    d->kanji_status_return->gline.line    = prevGEcho;
    d->kanji_status_return->gline.length  = prevGEchoLen;
    d->kanji_status_return->gline.revPos  = prevGRevPos;
    d->kanji_status_return->gline.revLen  = prevGRevLen;
    d->kanji_status_return->info |= KanjiGLineInfo;
  }
  return total_res;
}

int DoFuncSequence(uiContext d)
{
  return _DoFuncSequence(d, (BYTE *)NULL, (BYTE)NULL);
}

int
multiSequenceFunc(uiContext d, KanjiMode mode, int whattodo, int key, int fnum)
{
  int i;
  unsigned char *p;
  struct map *m;

  if (whattodo != KEY_CALL) 
    return 0;

  if (fnum == CANNA_FN_Kakutei || fnum == CANNA_FN_Quit || askQuitKey(key)) {
    /* Kakutei ¤Ï KC_KAKUTEI ¤Ø¤ÎÂÐ±þ */
    free(keyHistory);
    GlineClear(d);
    d->current_mode = (KanjiMode)(mode->ftbl);
    if (d->current_mode->flags & CANNA_KANJIMODE_EMPTY_MODE) {
      d->kanji_status_return->info |= KanjiEmptyInfo;
    }
    /* Nop ¤ò¹Ô¤¦ */
    (void)doFunc(d, CANNA_FN_Nop);
    d->flags |= MULTI_SEQUENCE_EXECUTED;
    return 0;
  }
  for (i= 0, p = mode->keytbl; *p != 255; p += 2,i+=2) {
    debug_message("multiSequenceFunc:\263\254\301\330[%d]\n",i,0,0);
                                   /* ³¬ÁØ */
    if (*p == key) { /* ¤³¤Î¥­¡¼¤ÏÅÐÏ¿¤µ¤ì¤Æ¤¤¤¿¡£ */
      keyHistory =
	(unsigned char *)realloc(keyHistory, strlen((char *)keyHistory) + strlen((char *)showChar(key)) +2);
      if (keyHistory) {
	strcat((char *)keyHistory," ");
	strcat((char *)keyHistory,(char *)showChar(key));

        makeGLineMessageFromString(d, (char *)keyHistory);
        if (*++p == CANNA_FN_UseOtherKeymap) { /* ¤Þ¤À¥­¡¼¥·¥±¥ó¥¹¤ÎÂ³¤­¤¬Â¸ºß */
          m = mapFromHash(mode, key, (struct map ***)0);
          m->mode->ftbl = mode->ftbl;
          d->current_mode = m->mode;
          return NothingForGLine(d);
        }
        free(keyHistory);
      }
      GlineClear(d);
      d->current_mode = (KanjiMode)(mode->ftbl); /* µ¡Ç½¤ò¼Â¹Ô */
      if (*p == CANNA_FN_FuncSequence) {
	return _DoFuncSequence(d, (unsigned char *)mode, key);
      }
      return (*d->current_mode->func)(d, d->current_mode, KEY_CALL, 0, *p);
    }
  }
  return NothingForGLineWithBeep(d);  /* ÅÐÏ¿¤·¤Æ¤¤¤Ê¤¤¥­¡¼¤ò²¡¤·¤¿ */
}
