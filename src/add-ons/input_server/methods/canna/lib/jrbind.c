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
static char rcs_id[] = "@(#) 102.1 $Id: jrbind.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif /* lint */

#include "canna.h"
#include <canna/mfdef.h>
#include <sys/types.h>

#define ACTHASHTABLESIZE 64
#define KEYHASHTABLESIZE 16

typedef int (*keycallback)(unsigned, unsigned char *, int,
			   unsigned char *, int, char *);

static uiContext newUiContext(unsigned int dpy, unsigned int win);
static void GetCannaKeyOnAMode(unsigned modeid, unsigned char *mode, keycallback keyfn, char *con);
static void GetCannaKeyfunc(keycallback keyfn, char *con);
   
/*

  jrKanjiString ¤Ï TTY ¤Î¥­¡¼ÆþÎÏ¤ò¼õ¤±¼è¤ê¡¢¤½¤Î¥­¡¼¤Ë¤·¤¿¤¬¤Ã¤ÆÉ¬Í×
  ¤Ê¤é¥«¥Ê´Á»úÊÑ´¹¤ò¹Ô¤¤¡¢¤½¤Î¥­¡¼ÆþÎÏ¤Î·ë²Ì¤È¤·¤ÆÆÀ¤é¤ì¤ëÊ¸»úÎó¤ò 
  buffer_return ¤ÇÊÖ¤¹¡£buffer_return ¤Ï¥¢¥×¥ê¥±¡¼¥·¥ç¥óÂ¦¤ËÍÑ°Õ¤¹¤ë¥Ð¥Ã
  ¥Õ¥¡¤Ç¤¢¤ê¡¢¥¢¥×¥ê¥±¡¼¥·¥ç¥ó¤Ï¤½¤Î¥Ð¥Ã¥Õ¥¡¤ÎÄ¹¤µ¤ò bytes_buffer ¤ÇÅÏ
  ¤¹¡£

  kanji_status_return ¤Ï³ÎÄê¤·¤Æ¤¤¤Ê¤¤ÆþÎÏÊ¸»úÎó¤òÉ½¼¨¤¹¤ë¤¿¤á¤Î¥Ç¡¼¥¿
  ¤Ç¤¢¤ê¡¢Ì¤³ÎÄê¤ÎÆÉ¤ß¤ä¸õÊä´Á»ú¤Ê¤É¤¬ÊÖ¤µ¤ì¤ë¡£kanji_status_return¤Î
  ¥á¥ó¥Ð¤Ë¤Ï¡¢ echoStr, length, revPos, revLen ¤¬¤¢¤ê¤½¤ì¤¾¤ì¡¢Ì¤³ÎÄê
  Ê¸»úÎó¤Ø¤Î¥Ý¥¤¥ó¥¿¡¢¤½¤ÎÄ¹¤µ¡¢Ì¤³ÎÄêÊ¸»úÎó¤Î¤¦¤Á¡¢¶¯Ä´¤¹¤ëÉôÊ¬¤Ø¤Î¥ª
  ¥Õ¥»¥Ã¥È¡¢¶¯Ä´¤¹¤ëÉôÊ¬¤ÎÄ¹¤µ¤òÊÖ¤¹¡£Ì¤³ÎÄêÊ¸»úÎó¤ò³ÊÇ¼¤¹¤ëÎÎ°è¤Ï 
  jrKanjiString ¤Ç¼«Æ°Åª¤ËÍÑ°Õ¤µ¤ì¤ë¡£

 */

extern int FirstTime;

extern BYTE *actFromHash();

int
wcKanjiString (
	int context_id,
	int ch,
	WCHAR_T *buffer_return,
	const int nbuffer,
	wcKanjiStatus *kanji_status_return)
{
  int res;

  *buffer_return = (WCHAR_T)ch;

  res = XwcLookupKanji2((unsigned int)0, (unsigned int)context_id,
			buffer_return, nbuffer,
			1/* byte */, 1/* functional char*/,
			kanji_status_return);
  return res;
}

/* jrKanjiControl -- ¥«¥Ê´Á»úÊÑ´¹¤ÎÀ©¸æ¤ò¹Ô¤¦ */

int
wcKanjiControl (
const int context,
const int request,
char *arg)
{
  return XwcKanjiControl2((unsigned int)0, (unsigned int)context,
			  (unsigned int)request, (BYTE *)arg);
}

inline
uiContext
newUiContext(unsigned int dpy, unsigned int win)
{
  extern struct CannaConfig cannaconf;
  uiContext d;

  if ((d = (uiContext)malloc(sizeof(uiContextRec))) != (uiContext)0) {
    if (initRomeStruct(d, cannaconf.chikuji) == 0) {
      if (internContext(dpy, win, d)) {
	return d;
      }
      freeRomeStruct(d);
    }
    free(d);
  }
  return (uiContext)0;
}

extern int kanjiControl (int, uiContext, caddr_t);

int XwcLookupKanji2(unsigned int dpy, unsigned int win, WCHAR_T *buffer_return, int nbuffer, int nbytes, int functionalChar, wcKanjiStatus *kanji_status_return)
{
  uiContext d;
  int retval;
  extern int locale_insufficient;

  /* locale ¥Ç¡¼¥¿¥Ù¡¼¥¹¤¬ÉÔ½½Ê¬¤Ç WCHAR_T ¤È¤ÎÊÑ´¹½èÍý¤¬¤Ç¤­¤Ê¤¤¾ì¹ç¤Ï
     ¡Ø¤«¤ó¤Ê¡Ù¤Ë¤È¤Ã¤ÆÂçÂÇ·â¡£¤â¤¦ÊÑ´¹¤Ï¤Ç¤­¤Ê¤¤¡ª */
  if (locale_insufficient) {
    kanji_status_return->info = KanjiEmptyInfo | KanjiThroughInfo;
    if (nbytes) { /* ¥­¥ã¥é¥¯¥¿¥³¡¼¥É¤¬¤È¤ì¤¿¾ì¹ç */
      kanji_status_return->length =
	kanji_status_return->revPos =
	  kanji_status_return->revLen = 0;
      return nbytes;
    }
    else { /* ¥­¥ã¥é¥¯¥¿¥³¡¼¥É¤¬¤È¤ì¤Ê¤«¤Ã¤¿¾ì¹ç¡Ê¥·¥Õ¥È¥­¡¼¤Ê¤É¡Ë... */
      kanji_status_return->length = -1;
      return 0;
    }
  }

  /* ½é¤á¤Æ XLookupKanjiString ¤¬¸Æ¤Ð¤ì¤¿»þ¤Ï¼­½ñ¤Î½é´ü²½¤Ê¤É¤Î½èÍý¤¬
     ¹Ô¤ï¤ì¤ë¡£ */

  if (FirstTime) {
    if (kanjiControl(KC_INITIALIZE, (uiContext)NULL, (char *)NULL) == -1) {
      return -1;
    }
    FirstTime = 0;
  }

  d = keyToContext(dpy, win);

  if (d == (uiContext)NULL) {
    /* ¤³¤Î¥¦¥£¥ó¥É¥¦¤«¤é¥¤¥Ù¥ó¥È¤¬Íè¤¿¤Î¤¬»Ï¤á¤Æ¤À¤Ã¤¿¤ê¤¹¤ë¤ï¤±¤è */
    d = newUiContext(dpy, win);
    if (d == (uiContext)NULL) {
      return NoMoreMemory();
    }
  }


  bzero(kanji_status_return, sizeof(wcKanjiStatus));
  
  d->ch = (unsigned)*buffer_return;
  d->buffer_return = buffer_return;
  d->n_buffer = nbuffer;
  d->kanji_status_return = kanji_status_return;

  debug_message("current_mode(0x%x)\n", d->current_mode,0,0);

  if ( nbytes || functionalChar ) { /* ¥­¥ã¥é¥¯¥¿¥³¡¼¥É¤¬¤È¤ì¤¿¾ì¹ç */
    int check;

    *buffer_return = key2wchar(d->ch, &check);
    if (!check) {
      return NothingChangedWithBeep(d);
    }

    d->nbytes = nbytes;

    retval = doFunc(d, 0);
#ifdef DEBUG
    checkModec(d);
#endif /* DEBUG */
    return(retval);
  }
  else { /* ¥­¥ã¥é¥¯¥¿¥³¡¼¥É¤¬¤È¤ì¤Ê¤«¤Ã¤¿¾ì¹ç¡Ê¥·¥Õ¥È¥­¡¼¤Ê¤É¡Ë... */
    d->kanji_status_return->length = -1;
    return 0;
  }
}

int
XwcKanjiControl2(unsigned int display, unsigned int window, unsigned int request, BYTE *arg)
{
  if (request == KC_INITIALIZE || request == KC_FINALIZE ||
      request == KC_SETSERVERNAME || request == KC_SETINITFILENAME ||
      request == KC_SETVERBOSE || request == KC_KEYCONVCALLBACK ||
      request == KC_QUERYCONNECTION || request == KC_SETUSERINFO ||
      request == KC_QUERYCUSTOM) {
    return kanjiControl(request, (uiContext)NULL, (char *)arg);
  }
  else if (/* 0 <= request && (É¬¤º¿¿) */ request < MAX_KC_REQUEST) {
    uiContext d;

    /* ½é¤á¤Æ wcKanjiString ¤¬¸Æ¤Ð¤ì¤¿»þ¤Ï¼­½ñ¤Î½é´ü²½¤Ê¤É¤Î½èÍý¤¬
       ¹Ô¤ï¤ì¤ë¡£ */

    if (FirstTime) {
      if (kanjiControl(KC_INITIALIZE, (uiContext)NULL, (char *)NULL) == -1) {
	return -1;
      }
      FirstTime = 0;
    }

    d = keyToContext((unsigned int)display, (unsigned int)window);

    if (d == (uiContext)NULL) {
      d = newUiContext(display, window);
      if (d == (uiContext)NULL) {
	return NoMoreMemory();
      }
    }

    if (request == KC_CLOSEUICONTEXT) {
      rmContext(display, window);
    }
    return kanjiControl(request, d, (char *)arg);
  }
  else {
    return -1;
  }
}

struct map {
  KanjiMode tbl;
  BYTE key;
  KanjiMode mode;
  struct map *next;
};
 
struct map *mapFromHash();

/* cfuncdef

  pushCallback -- ¥³¡¼¥ë¥Ð¥Ã¥¯¤Î½¸¹ç¤ò¥×¥Ã¥·¥å¤¹¤ë¡£

  ¥³¡¼¥ë¥Ð¥Ã¥¯¤Î½¸¹ç¤ò³ÊÇ¼¤¹¤ëÇÛÎó¤¬ malloc ¤µ¤ì¤Æ¡¢¤½¤ì¤¬ uiContext ¤Ë
  ¥×¥Ã¥·¥å¤µ¤ì¤ë¡£

  malloc ¤µ¤ì¤¿ÇÛÎó¤¬Ìá¤êÃÍ¤È¤·¤ÆÊÖ¤ë¡£

 */

struct callback *
pushCallback(uiContext d, mode_context env, canna_callback_t ev, canna_callback_t ex, canna_callback_t qu, canna_callback_t au)
{
  struct callback *newCB;

  newCB = (struct callback *)malloc(sizeof(struct callback));
  if (newCB) {
    newCB->func[0] = ev;
    newCB->func[1] = ex;
    newCB->func[2] = qu;
    newCB->func[3] = au;
    newCB->env = env;
    newCB->next = d->cb;
    d->cb = newCB;
  }
  return newCB;
}

void
popCallback(uiContext d)
{
  struct callback *oldCB;

  oldCB = d->cb;
  d->cb = oldCB->next;
  free(oldCB);
}

#if defined(WIN) && defined(_RK_h)

extern RkwGetProtocolVersion (int *, int *);
extern char *RkwGetServerName (void);

struct cannafn{
  {
    RkwGetProtocolVersion,
    RkwGetServerName,
    RkwGetServerVersion,
    RkwInitialize,
    RkwFinalize,
    RkwCreateContext,
    RkwDuplicateContext,
    RkwCloseContext,
    RkwSetDicPath,
    RkwCreateDic,
    RkwSync,
    RkwGetDicList,
    RkwGetMountList,
    RkwMountDic,
    RkwRemountDic,
    RkwUnmountDic,
    RkwDefineDic,
    RkwDeleteDic,
    RkwGetHinshi,
    RkwGetKanji,
    RkwGetYomi,
    RkwGetLex,
    RkwGetStat,
    RkwGetKanjiList,
    RkwFlushYomi,
    RkwGetLastYomi,
    RkwRemoveBun,
    RkwSubstYomi,
    RkwBgnBun,
    RkwEndBun,
    RkwGoTo,
    RkwLeft,
    RkwRight,
    RkwNext,
    RkwPrev,
    RkwNfer,
    RkwXfer,
    RkwResize,
    RkwEnlarge,
    RkwShorten,
    RkwStoreYomi,
    RkwSetAppName,
    RkwSetUserInfo,
    RkwQueryDic,
    RkwCopyDic,
    RkwListDic,
    RkwRemoveDic,
    RkwRenameDic,
    RkwChmodDic,
    RkwGetWordTextDic,
    RkwGetSimpleKanji,
  },
  wcKanjiControl,
  wcKanjiString,
};
#endif

#ifdef WIN
#include "cannacnf.h"

/* Interfaces for CannaGetConfigure/CannaSetConfigure */

#define CANNA_MODE_AllModes 255
#define MAX_KEYS_IN_A_MODE 256


inline void
GetCannaKeyOnAMode(unsigned modeid, unsigned char *mode, 
		   keycallback keyfn, char *con)
{
  unsigned char key;
  int i;

  for (i = 0 ; i < MAX_KEYS_IN_A_MODE ; i++) {
    if (mode[i] != CANNA_FN_Undefined) { /* is this required? */
      key = i;
      (*keyfn)(modeid, &key, 1, mode + i, 1, con);
    }
  }
}

inline void
GetCannaKeyfunc(keycallback keyfn, char *con)
{
  extern unsigned char default_kmap[], alpha_kmap[], empty_kmap[];

  GetCannaKeyOnAMode(CANNA_MODE_AllModes, default_kmap, keyfn, con);
  GetCannaKeyOnAMode(CANNA_MODE_AlphaMode, alpha_kmap, keyfn, con);
  GetCannaKeyOnAMode(CANNA_MODE_EmptyMode, empty_kmap, keyfn, con);
}

#endif
