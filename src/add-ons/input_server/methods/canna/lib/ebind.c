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
static char rcsid[] = "$Id: ebind.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif

#include "canna.h"
// There is Exported Symbols !!
extern int howToReturnModeInfo;

static WCHAR_T *inbuf = 0;
static int inbufsize = 0;


static int
StoreWCtoEUC(WCHAR_T *wbuf, int wbuflen, wcKanjiStatus *wks, char *ebuf, int maxebuf, jrKanjiStatus *ks, int ch, int nbytes)
{
  int ret, rest, totallen = 0, len;
  char *p;

  /* info */

  ks->info = wks->info;
    
  /* ·ë²Ì */

  if (ks->info & KanjiThroughInfo) {
    if (nbytes) {
      ebuf[0] = ch;
    }
    ret = nbytes;
  }
  else {
    ret = (wbuflen > 0) ? WCstombs(ebuf, wbuf, maxebuf) : 0;
    if (ks->info & KanjiYomiInfo) {
      WCHAR_T *ep;
      len = WCstombs(ebuf + ret + 1, wbuf + wbuflen + 1,
		     maxebuf - ret - 1);
      ep = wbuf + wbuflen + 1;
      while (*ep) ep++;
      WCstombs(ebuf + ret + 1 + len + 1, ep + 1,
	       maxebuf - ret - 1 - len - 1);
    }
  }

  if (wks->length > 0) {
    totallen = wks->length;
  }
  if (wks->info & KanjiModeInfo) {
    totallen += WStrlen(wks->mode);
  }
  if (wks->info & KanjiGLineInfo) {
    totallen += wks->gline.length;
  }

  if (inbufsize < totallen) {
    inbufsize = totallen; /* inbufsize will be greater than 0 */
    if (inbuf) free(inbuf);
    inbuf = (WCHAR_T *)malloc(inbufsize * sizeof(WCHAR_T));
    if (!inbuf) {
      inbufsize = 0;
      jrKanjiError = "\245\341\245\342\245\352\244\254\302\255\244\352\244\336\244\273\244\363";
                     /* ¥á¥â¥ê¤¬Â­¤ê¤Þ¤»¤ó */
      return -1;
    }
  }

  rest = inbufsize * sizeof(WCHAR_T);
  p = (char *)inbuf;

  if (wks->length < 0) {
    ks->length = -1;
  }
  else {
    /* ¥¨¥³¡¼Ê¸»ú */

    ks->length = ks->revLen = ks->revPos = 0;

    if (wks->length > 0) {
      ks->echoStr = (unsigned char *)p;
      if (wks->revPos > 0) {
	len = ks->revPos = CNvW2E(wks->echoStr, wks->revPos, p, rest);
	p += len;
	rest -= len;
      }
      if (wks->revLen > 0) {
	len = ks->revLen 
	  = CNvW2E(wks->echoStr + wks->revPos, wks->revLen, p, rest);
	p += len;
	rest -= len;
      }
      len = 0;
      if (wks->length - wks->revPos - wks->revLen > 0) {
	len = CNvW2E(wks->echoStr + wks->revPos + wks->revLen,
		     wks->length - wks->revPos - wks->revLen, p, rest);
	p += len;
	rest -= len;
      }
      ks->length = ks->revLen + ks->revPos + len;
      *p++ = '\0';
      rest--;
    }
  }

  /* ¥â¡¼¥ÉÉ½¼¨ */

  if (wks->info & KanjiModeInfo) {
    len = WCstombs(p, wks->mode, rest);
    ks->mode = (unsigned char *)p;
    p[len] = '\0';
    p += len + 1;
    rest -= len + 1;
  }

  /* °ìÍ÷¹ÔÉ½¼¨ */

  if (wks->info & KanjiGLineInfo) {
    ks->gline.length = ks->gline.revLen = ks->gline.revPos = 0;

    if (wks->gline.length > 0) {
      ks->gline.line = (unsigned char *)p;
      if (wks->gline.revPos > 0) {
	len = ks->gline.revPos 
	  = CNvW2E(wks->gline.line, wks->gline.revPos, p, rest);
	p += len;
	rest -= len;
      }
      if (wks->gline.revLen > 0) {
	len = ks->gline.revLen
	  = CNvW2E(wks->gline.line + wks->gline.revPos, wks->gline.revLen,
		   p, rest);
	p += len;
	rest -= len;
      }
      len = 0;
      if (wks->gline.length - wks->gline.revPos - wks->gline.revLen > 0) {
	len = CNvW2E(wks->gline.line + wks->gline.revPos +
		     wks->gline.revLen,
		     wks->gline.length -
		     wks->gline.revPos - wks->gline.revLen,
		     p, rest);
	p += len;
	rest -= len;
      }
      ks->gline.length = ks->gline.revLen + ks->gline.revPos + len;
      *p++ = '\0';
      rest--;
    }
  }
  return ret;
}

inline int
XLookupKanji2(
	unsigned int	dpy, 
	unsigned int	win, 
	char			*buffer_return, 
	int				bytes_buffer, 
	int				nbytes, 
	int				functionalChar, 
	jrKanjiStatus	*kanji_status_return,
	int				key)
{
  int				ret;
  wcKanjiStatus		wks;

	  /* ÆâÉô¥Ð¥Ã¥Õ¥¡¤ò¥¢¥í¥±¡¼¥È¤¹¤ë */
	if (inbufsize < bytes_buffer) {
		inbufsize = bytes_buffer; /* inbufsize will be greater than 0 */
		if (inbuf) free(inbuf);
		inbuf = (WCHAR_T *)malloc(inbufsize * sizeof(WCHAR_T));
		if (!inbuf) {
			inbufsize = 0;
			jrKanjiError = "\245\341\245\342\245\352\244\254\302\255\244\352\244\336\244\273\244\363";
			/* ¥á¥â¥ê¤¬Â­¤ê¤Þ¤»¤ó */
			return -1;
		}
	}

	inbuf[0] = key;
	
	ret = XwcLookupKanji2(dpy, win, inbuf, inbufsize, 1/*nbytes*/, 1/*functionalChar*/,
	&wks);
	if (ret >= inbufsize)
		ret = inbufsize - 1;
	inbuf[ret] = 0;
	
	return StoreWCtoEUC(inbuf, ret, &wks,
			buffer_return, bytes_buffer, kanji_status_return,
			key, nbytes);
}
		      

int
XKanjiControl2(unsigned int display, unsigned int window, unsigned int request, BYTE *arg)
{
  int ret = -1, len1, len2;
  wcKanjiStatusWithValue wksv;
  wcKanjiStatus wks;
  int ch;
  WCHAR_T arg2[256];
  WCHAR_T wbuf[320], wbuf1[320], wbuf2[320];

  wksv.buffer = wbuf;
  wksv.n_buffer = 320;
  wksv.ks = &wks;

  switch (request) {
  case KC_DO: /* val ¤È buffer_return ¤ËÆþ¤ì¤ë¥¿¥¤¥× */
    wbuf[0] = ((jrKanjiStatusWithValue *)arg)->buffer[0];
    /* ²¼¤ØÂ³¤¯ */
  case KC_CHANGEMODE: /* val ¤òÍ¿¤¨¤ë¥¿¥¤¥× */
    wksv.val = ((jrKanjiStatusWithValue *)arg)->val;
    goto withksv;
  case KC_STOREYOMI: /* echoStr ¤È length ¤È mode ¤òÍ¿¤¨¤ë¥¿¥¤¥× */
    /* ¤Þ¤º mode ¤ò¥ï¥¤¥É¤Ë¤·¤Æ¤ß¤è¤¦ */
    if (((jrKanjiStatusWithValue *)arg)->ks->mode) {
      len2 = MBstowcs(wbuf2, (char *)((jrKanjiStatusWithValue *)arg)->ks->mode,
		      320);
      wbuf2[len2] = (WCHAR_T)0;
      wks.mode = wbuf2;
    }
    else {
      wks.mode = (WCHAR_T *)0;
    }
    /* ²¼¤ØÂ³¤¯ */
  case KC_DEFINEKANJI: /* echoStr ¤È length ¤òÍ¿¤¨¤ë¥¿¥¤¥× */
    /* echoStr ¤ò¥ï¥¤¥É¤Ë¤·¤ÆÍ¿¤¨¤Æ¤ß¤è¤¦ */
    len1 = MBstowcs(wbuf1,
		    (char *)((jrKanjiStatusWithValue *)arg)->ks->echoStr, 320);
    wbuf1[len1] = (WCHAR_T)0;
    wks.echoStr = wbuf1;
    wks.length = len1;
    /* ²¼¤ØÂ³¤¯ */
  case KC_KAKUTEI: /* ¤¿¤ÀÃ±¤ËÍ¿¤¨¤ÆÊÖ¤Ã¤ÆÍè¤ë¥¿¥¤¥× */
  case KC_KILL:
    goto withksv;
  case KC_CLOSEUICONTEXT:
    goto closecont;
  case KC_QUERYMODE: /* querymode */
    ret = XwcKanjiControl2(display, window, request, (BYTE *)arg2);
    if (!ret) {
      switch (howToReturnModeInfo) {
      case ModeInfoStyleIsString:
	WCstombs((char *)arg, arg2, 256);
	break;
      case ModeInfoStyleIsBaseNumeric:
        arg[2] = (unsigned char)arg2[2];
      case ModeInfoStyleIsExtendedNumeric:
	arg[1] = (unsigned char)arg2[1];
      case ModeInfoStyleIsNumeric:
	arg[0] = (unsigned char)arg2[0];
	break;
      }
    }
    goto return_ret;
  case KC_SETLISTCALLBACK: /* ¤É¤¦¤·¤¿¤éÎÉ¤¤¤«¤ï¤«¤é¤Ê¤¤¤â¤Î */
    ret = -1;
    goto return_ret;
  default: /* ¥ï¥¤¥É¤Ç¤âEUC¤Ç¤âÊÑ¤ï¤é¤Ê¤¤¤â¤Î */
    ret = XwcKanjiControl2(display, window, request, arg);
    goto return_ret;
  }
 withksv:
  ch = ((jrKanjiStatusWithValue *)arg)->buffer[0];
  ret = XwcKanjiControl2(display, window, request, (BYTE *)&wksv);
  if (ret < 0) {
    goto return_ret;
  }
  else {
    wksv.buffer[ret] = (WCHAR_T)0;
    ((jrKanjiStatusWithValue *)arg)->val =
      StoreWCtoEUC(wksv.buffer, wksv.val, wksv.ks,
		   (char *)((jrKanjiStatusWithValue *)arg)->buffer,
		   ((jrKanjiStatusWithValue *)arg)->bytes_buffer,
		   ((jrKanjiStatusWithValue *)arg)->ks,
		   ch, ((jrKanjiStatusWithValue *)arg)->val);
    ret = ((jrKanjiStatusWithValue *)arg)->val;
    goto return_ret;
  }
 closecont:
  ch = ((jrKanjiStatusWithValue *)arg)->buffer[0];
  ret = XwcKanjiControl2(display, window, request, (BYTE *)&wksv);
  if (ret < 0) {
    goto return_ret;
  }
  else {
    wksv.val = 0;
    ((jrKanjiStatusWithValue *)arg)->val =
      StoreWCtoEUC(wksv.buffer, wksv.val, wksv.ks,
		   (char *)((jrKanjiStatusWithValue *)arg)->buffer,
		   ((jrKanjiStatusWithValue *)arg)->bytes_buffer,
		   ((jrKanjiStatusWithValue *)arg)->ks,
		   ch, ((jrKanjiStatusWithValue *)arg)->val);
    goto return_ret;
  }
 return_ret:
  return ret;
}

int
jrKanjiString (
	int context_id,
	int ch,
	char *buffer_return,
	int nbuffer,
	jrKanjiStatus *kanji_status_return)
{
  *buffer_return = ch;

  return XLookupKanji2(0, context_id,
		       buffer_return, nbuffer,
		       1/* byte */, 1/* functional char*/,
		       kanji_status_return, ch);
}

/* jrKanjiControl -- ¥«¥Ê´Á»úÊÑ´¹¤ÎÀ©¸æ¤ò¹Ô¤¦ */
int
jrKanjiControl (
int context,
int request,
char *arg)
{
  return XKanjiControl2((unsigned int)0, (unsigned int)context,
			request, (BYTE *)arg);
}

void
setBasePath(const char *path)
{
extern char basepath[];
	strcpy(basepath, path);
}
