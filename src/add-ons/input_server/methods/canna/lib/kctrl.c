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
static char rcs_id[] = "@(#) 102.1 $Id: kctrl.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif /* lint */

#include "canna.h"
#include "RK.h"
#include "RKintern.h"

#include <errno.h>
#include <sys/types.h>
#include <canna/mfdef.h>

#define DEFAULT_COLUMN_WIDTH	70


extern struct RkRxDic *romajidic, *englishdic;

extern char *CANNA_initfilename;
extern char saveapname[];

static int insertEmptySlots (uiContext);
static int callCallback (uiContext, int);
static void freeKeysup (void);
static void freeBuffer (void);
static void freeExtra (void);

static
int doInitializeFunctions(uiContext d)
{
  BYTE *p;
  int res = 0;
  wcKanjiStatus ks, *pks;
  extern BYTE *initfunc;
  WCHAR_T xxxx[10];

  d->ch = 0;
  d->buffer_return = xxxx;
  d->n_buffer = sizeof(xxxx) / sizeof(WCHAR_T);
  d->nbytes = 0;

  if (initfunc) {
    pks = d->kanji_status_return;
    d->kanji_status_return = &ks;
    for (p = initfunc ; *p ; p++) {
      res = _doFunc(d, *p);
    }
    res = _afterDoFunc(d, res);
    d->kanji_status_return = pks;
  }
  return res;
}

/* uiContext ¤Î½é´ü²½ */

int initRomeStruct(uiContext d, int flg)
{
  extern KanjiModeRec alpha_mode, empty_mode;
  extern KanjiModeRec kzhr_mode, kzkt_mode, kzal_mode;
  extern KanjiModeRec khkt_mode, khal_mode;
  yomiContext yc;
  extern int defaultContext, defaultBushuContext;

  bzero(d, sizeof(uiContextRec));

  if (insertEmptySlots(d) < 0) {
    return -1;
  }

  /* ¥³¥ó¥Æ¥¯¥¹¥È¤òºÇ½é¤Ë¥Ç¥å¥×¥ê¥±¡¼¥È¤·¤Æ¤¤¤¿²áµî¤¬¤¢¤Ã¤¿¤Ê¤¢ */
  d->contextCache = -1;

  /* ½é´ü¥â¡¼¥É(¥¢¥ë¥Õ¥¡¥Ù¥Ã¥È¥â¡¼¥É)¤òÆþ¤ì¤ë */
  d->majorMode = d->minorMode = CANNA_MODE_AlphaMode;
  yc = (yomiContext)d->modec;
  if (flg) {
    yc->minorMode = CANNA_MODE_ChikujiYomiMode;
    yc->generalFlags |= CANNA_YOMI_CHIKUJI_MODE;
  }
  alphaMode(d);

  /* ½é´ü func ¤ò¼Â¹Ô¤¹¤ë */

  (void)doInitializeFunctions(d);
  return 0;
}

static void
freeModec(mode_context modec)
{
  coreContext cc;
  union {
    coreContext c;
    yomiContext y;
    ichiranContext i;
    forichiranContext f;
    mountContext m;
    tourokuContext t;
  } gc;

  cc = (coreContext)modec;
  while (cc) {
    switch (cc->id) {
    case CORE_CONTEXT:
      gc.c = cc;
      cc = (coreContext)gc.c->next;
      freeCoreContext(gc.c);
      break;
    case YOMI_CONTEXT:
      gc.y = (yomiContext)cc;
      cc = (coreContext)gc.y->next;
      /* yc->context ¤Î close ¤Ï¤¤¤é¤Ê¤¤¤Î¤«¤Ê¤¢¡£1996.10.30 º£ */
      freeYomiContext(gc.y);
      break;
    case ICHIRAN_CONTEXT:
      gc.i = (ichiranContext)cc;
      cc = (coreContext)gc.i->next;
      freeIchiranContext(gc.i);
      break;
    case FORICHIRAN_CONTEXT:
      gc.f = (forichiranContext)cc;
      cc = (coreContext)gc.f->next;
      freeForIchiranContext(gc.f);
      break;
    case MOUNT_CONTEXT:
      gc.m = (mountContext)cc;
      cc = (coreContext)gc.m->next;
      freeIchiranContext(gc.i);
      break;
    case TOUROKU_CONTEXT:
      gc.t = (tourokuContext)cc;
      cc = (coreContext)gc.t->next;
      free(gc.t);
      break;
    default:
      break;
    }
  }
}

static void
freeCallbacks(struct callback *cb)
{
  struct callback *nextcb;

  for (; cb ; cb = nextcb) {
    nextcb = cb->next;
    free(cb);
  }
}

void
freeRomeStruct(uiContext d)
{
  freeModec(d->modec);
  if (d->cb) {
    freeCallbacks(d->cb);
  }
  if (d->contextCache >= 0) {
    if (RkwCloseContext(d->contextCache) < 0) {
      if (errno == EPIPE) {
	jrKanjiPipeError();
      }
    }
  }
#ifndef NO_EXTEND_MENU
  freeAllMenuInfo(d->minfo);
#endif
  if (d->selinfo) {
    selectinfo *p, *q;

    for (p = d->selinfo ; p ; p = q) {
      q = p->next;
      free(p);
    }
  }
  if (d->attr) {
    if (d->attr->u.attr) {
      free(d->attr->u.attr);
    }
    free(d->attr);
  }
  free(d);
}

static
int insertEmptySlots(uiContext d)
{
  extern KanjiModeRec	empty_mode;
  yomiContext		yc;

  if (pushCallback(d, (mode_context) NULL, NO_CALLBACK, NO_CALLBACK,
		   NO_CALLBACK, NO_CALLBACK) == (struct callback *)NULL)
    return NoMoreMemory();

  yc = newYomiContext((WCHAR_T *)NULL, 0, /* ·ë²Ì¤Ï³ÊÇ¼¤·¤Ê¤¤ */
		      CANNA_NOTHING_RESTRICTED,
		      (int)!CANNA_YOMI_CHGMODE_INHIBITTED,
		      (int)!CANNA_YOMI_END_IF_KAKUTEI,
		      CANNA_YOMI_INHIBIT_NONE);
  if (yc == (yomiContext)0) {
    popCallback(d);
    return NoMoreMemory();
  }
  yc->majorMode = yc->minorMode = CANNA_MODE_HenkanMode;
  d->majorMode = d->minorMode = CANNA_MODE_HenkanMode;
  d->modec = (mode_context)yc;

  d->current_mode = yc->curMode = yc->myEmptyMode = &empty_mode;
  yc->romdic = romajidic;
  d->ncolumns = DEFAULT_COLUMN_WIDTH;
  d->minfo = (menuinfo *)0;
  d->selinfo = (selectinfo *)0;
  d->prevMenu = (menustruct *)0;
  return 0;
}

/* 

  display ¤È window ¤ÎÁÈ¤ä ¥³¥ó¥Æ¥¯¥¹¥ÈID ¤ò¼ÂºÝ¤Î¥³¥ó¥Æ¥¯¥¹¥È¤ËÂÐ±þÉÕ
  ¤±¤ë¤¿¤á¤Î¥Ï¥Ã¥·¥å¥Æ¡¼¥Ö¥ë

  display ¤È window ¤«¤éºî¤é¤ì¤ë¥­¡¼¤Î¤È¤³¤í¤ò°ú¤¯¤È¡¢¤½¤³¤Ë¥³¥ó¥Æ¥¯¥¹¥È
  ¤¬Æþ¤Ã¤Æ¤¤¤ë³ÎÎ¨¤¬¹â¤¤¡£¤â¤·Æþ¤Ã¤Æ¤¤¤Ê¤¯¤È¤â¡¢¥Ý¥¤¥ó¥¿¥Á¥§¡¼¥ó¤ò¤¿¤É¤Ã¤Æ
  ¹Ô¤¯¤È¤¤¤Ä¤«¤Ï¥³¥ó¥Æ¥¯¥¹¥È¤¬ÆÀ¤é¤ì¤ë¤Ë°ã¤¤¤Ê¤¤¡£

 */

#define HASHTABLESIZE 96

static struct bukRec {
  unsigned int data1, data2;
  uiContext context;
  struct bukRec *next;
} *conHash[HASHTABLESIZE];

/* ¥Ï¥Ã¥·¥å¥Æ¡¼¥Ö¥ë¤òÄ´¤Ù¤Æ¥³¥ó¥Æ¥¯¥¹¥È¤¬¤¢¤ë¤«¤É¤¦¤«Ä´¤Ù¤ë´Ø¿ô */

static
int countContext(void)
{
  struct bukRec *hash;

  int i, c;
  for(i = 0, c = 0; i < HASHTABLESIZE; i++) {
    for(hash = conHash[i] ; hash && hash->context ;hash = hash->next){
      c++;
    }
  }
#if defined(DEBUG) && !defined(WIN)
  fprintf(stderr, "¹ç·×=%d\n", c);
#endif
  if(c) {
    return 0;
  }
  else {
    return 1;
  }
}

/* ¥Ï¥Ã¥·¥å¥­¡¼¤òºî¤ë´Ø¿ô(¤¤¤¤²Ã¸º) */

static unsigned int
makeKey(unsigned int data1, unsigned int data2)
{
  unsigned int key;

  key = data1 % HASHTABLESIZE;
  key += data2 % HASHTABLESIZE;
  key %= HASHTABLESIZE;
  return key;
}

/* 

  keyToContext -- Display ¤È Window ¤ÎÁÈ¤Ê¤É¤«¤é¥³¥ó¥Æ¥¯¥¹¥È¤ò³ä¤ê½Ð¤¹½èÍý

  display ¤È window ¤ÎÁÈ¤¬¥³¥ó¥Æ¥¯¥¹¥È¤ò»ý¤Ã¤Æ¤¤¤ì¤Ð¤½¤Î¥³¥ó¥Æ¥¯¥¹¥È
  ¤òÊÖ¤¹¡£

  »ý¤Ã¤Æ¤¤¤Ê¤¤¤Î¤Ç¤¢¤ì¤Ð¡¢NULL ¤òÊÖ¤¹¡£

  */

uiContext 
keyToContext(unsigned int data1, unsigned int data2)
{
  unsigned int key;
  struct bukRec *p;

  key = makeKey(data1, data2);
  for (p = conHash[key] ; p ; p = p->next) {
    if (p->data1 == data1 && p->data2 == data2) {
      /* ¤³¤ê¤ã¤¢¥³¥ó¥Æ¥¯¥¹¥È¤¬¸«¤Ä¤«¤ê¤Þ¤·¤¿¤Ê */
      return p->context;
    }
  }
  return (uiContext)0; /* ¸«¤Ä¤«¤ê¤Þ¤»¤ó¤Ç¤·¤¿¡£ */
}


/* internContext -- ¥Ï¥Ã¥·¥å¥Æ¡¼¥Ö¥ë¤ËÅÐÏ¿¤¹¤ë 

  ¤³¤Î¤È¤­¡¢´û¤Ë¡¢display ¤È window ¤ÎÁÈ¤¬Â¸ºß¤¹¤ë¤Î¤Ç¤¢¤ì¤Ð¡¢
  ¤½¤ÎÀè¤Ë¤Ä¤Ê¤¬¤Ã¤Æ¤¤¤ë¥³¥ó¥Æ¥¯¥¹¥È¤ò¥Õ¥ê¡¼¤¹¤ë¤Î¤ÇÃí°Õ¡ª¡ª

*/

struct bukRec *
internContext(unsigned int data1, unsigned int data2, uiContext context)
{
  unsigned int key;
  struct bukRec *p, **pp;

  key = makeKey(data1, data2);
  for (pp = &conHash[key]; (p = *pp) != (struct bukRec *)0; pp = &(p->next)) {
    if (p->data1 == data1 && p->data2 == data2) {
      freeRomeStruct(p->context);
      p->context = context;
      return p;
    }
  }
  p = *pp = (struct bukRec *)malloc(sizeof(struct bukRec));
  if (p) {
    p->data1 = data1;
    p->data2 = data2;
    p->context = context;
    p->next = (struct bukRec *)0;
  }
  return p;
}


/* rmContext -- ¥Ï¥Ã¥·¥å¥Æ¡¼¥Ö¥ë¤«¤éºï½ü¤¹¤ë

*/

void
rmContext(unsigned int data1, unsigned int data2)
{
  unsigned int key;
  struct bukRec *p, *q, **pp;

  key = makeKey(data1, data2);
  pp = &conHash[key];
  for (p = *pp ; p ; p = q) {
    q = p->next;
    if (p->data1 == data1 && p->data2 == data2) {
      *pp = q;
      free(p);
    }
    else {
      pp = &(p->next);
    }
  }
}

/* cfuncdef

  freeBukRecs() -- ¥Ý¥¤¥ó¥È¤µ¤ì¤Æ¤¤¤ëÀè¤Î¥Ð¥±¥Ã¥È¤Î¥Õ¥ê¡¼

  ¥Ð¥±¥Ã¥È¤Ë¤è¤Ã¤Æ¥Ý¥¤¥ó¥È¤µ¤ì¤Æ¤¤¤ë¥Ç¡¼¥¿¤ò¤¹¤Ù¤Æ¥Õ¥ê¡¼¤¹¤ë¡£
  ¥Õ¥ê¡¼¤ÎÂÐ¾Ý¤Ë¤Ï uiContext ¤â´Þ¤Þ¤ì¤ë¡£

*/

static void
freeBukRecs(struct bukRec *p)
{
  struct bukRec *nextp;

  if (p) { /* reconfirm that p points some structure */
    freeRomeStruct(p->context);
    nextp = p->next;
    if (nextp) {
      freeBukRecs(nextp);
    }
    free(p);
  }
}

/* cfuncdef

  clearHashTable() -- ¥Ï¥Ã¥·¥å¥Æ¡¼¥Ö¥ë¤ÎÆâÍÆ¤ò¤¹¤Ù¤Æ¥Õ¥ê¡¼¤¹¤ë¡£

*/

static void
clearHashTable(void)
{
  int i;
  struct bukRec *p;

  for (i = 0 ; i < HASHTABLESIZE ; i++) {
    p = conHash[i];
    conHash[i] = 0;
    if (p) {
      freeBukRecs(p);
    }
  }
}

#define NWARNINGMESG 64
static char *WarningMesg[NWARNINGMESG + 1]; /* +1 ¤ÏºÇ¸å¤Î NULL ¥Ý¥¤¥ó¥¿¤ÎÊ¬ */
static int nWarningMesg = 0;

static void
initWarningMesg(void)
{
  int i;

  for (i = 0 ; i < nWarningMesg ; i++) {
    free(WarningMesg[i]);
    WarningMesg[i] = (char *)0;
  }
  nWarningMesg = 0;
}

void
addWarningMesg(char *s)
{
  int n;
  char *work;

  if (nWarningMesg < NWARNINGMESG) {
    n = strlen(s);
    work = (char *)malloc(n + 1);
    if (work) {
      strcpy(work, s);
      WarningMesg[nWarningMesg++] = work;
    }
  }
}

static int
KC_keyconvCallback(uiContext d, char *arg)
/* ARGSUSED */
{
extern void (*keyconvCallback)(...);

  if (arg) {
    keyconvCallback = (void (*)(...))arg;
  }
  else {
    keyconvCallback = (void (*)(...))NULL;
  }
  return 0;
}

extern void restoreBindings();

static
int KC_initialize(uiContext d, char *arg)
     /* ARGSUSED */
{
  extern int FirstTime;

  if (FirstTime) {
#ifdef ENGINE_SWITCH
    extern char *RkGetServerEngine (void);
    if (!RkGetServerEngine()) {
      RkSetServerName((char *)0);
    }
#endif

    InitCannaConfig(&cannaconf);

    if (WCinit() < 0) {
      /* locale ´Ä¶­¤¬ÉÔ½½Ê¬¤ÇÆüËÜ¸ìÆþÎÏ¤¬¤Ç¤­¤Ê¤¤ */
      jrKanjiError =
	"The locale database is insufficient for Japanese input system.";
      if (arg) *(char ***)arg = (char **)0;
      return -1;
    }

    debug_message("KC_INITIALIZE \244\362\313\334\305\366\244\313\244\271\244\353\244\276\n",0,0,0);
                                 /* ¤òËÜÅö¤Ë¤¹¤ë¤¾ */

#ifndef NO_EXTEND_MENU
    if (initExtMenu() < 0) {
      jrKanjiError = "Insufficient memory.";
      if (arg) *(char ***)arg = (char **)0;
      return -1;
    }
#endif

    /* ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿ÍÑ¥á¥â¥ê´ÉÍý¤Î½é´ü²½ */

    WStringOpen();
#ifndef NO_EXTEND_MENU
    if (initBushuTable() != NG) {
      if (initGyouTable() != NG) {
        if (initHinshiTable() != NG) {
          if (initUlKigoTable() != NG) {
            if (initUlKeisenTable() != NG) {
              if (initOnoffTable() != NG) {
                initKigoTable(); /* ²¿¤â¤·¤Æ¤¤¤Ê¤¤ */
                if (initHinshiMessage() != NG) {
#endif

                  /* ¥¦¥©¡¼¥Ë¥ó¥°¥á¥Ã¥»¡¼¥¸¤Î½é´ü²½ */
                  initWarningMesg();

                  /* ¥â¡¼¥ÉÌ¾¤Î½é´ü²½ */
                  initModeNames();

                  /* ¥­¡¼¥Æ¡¼¥Ö¥ë¤Î½é´ü²½ */
                  if (initKeyTables() != NG) {

                    /* ½é´ü²½¥Õ¥¡¥¤¥ë¤ÎÆÉ¤ß¹þ¤ß */
#ifdef BINARY_CUSTOM
                    binparse();
#else
                    parse();
#endif

                    /* keyconvCallback ¤Ï parse ¸å¤ÏÉÔÍ×¤Ê¤Î¤Ç¥¯¥ê¥¢¤¹¤ë */
                    KC_keyconvCallback(d, (char *)0);

                    /* °ìÍ÷´Ø·¸Ê¸»úÎó¤Î½é´ü²½ */
                    if (initIchiran() != NG) {

                      /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¤Î½é´ü²½ */
                      RomkanaInit();

                      /* ¥«¥Ê´Á»úÊÑ´¹¤Î½é´ü²½ */
                      KanjiInit();
                      /* ¤³¤³¤Ç¤â¥¨¥é¡¼¤ÏÌµ»ë¤·¤Þ¤¹¡£
                         ´Á»ú¤Ë¤Ê¤é¤Ê¤¯¤Æ¤â¤¤¤¤¤·¡£ */

		      {
			extern int standalone;
			standalone = RkwGetServerName() ? 0 : 1;
		      }

                      if (arg) {
                      *(char ***)arg = nWarningMesg ? WarningMesg : (char **)0;
                      }
                      FirstTime = 0;
                      return 0;
                    }
                    /* uiContext ¤Î¸¡º÷¤Î¤¿¤á¤Î¥Ï¥Ã¥·¥å¥Æ¡¼¥Ö¥ë¤ò¥¯¥ê¥¢
                       uiContext ¤â°ì½ï¤Ë¥Õ¥ê¡¼¤¹¤ë */
                    clearHashTable();

                    /* µ­¹æÄêµÁ¤ò¾Ã¤¹ */
                    freeKeysup();

                    /* ¿§¡¹¤È¥«¥¹¥¿¥Þ¥¤¥º¤µ¤ì¤Æ¤¤¤ë½ê¤ò¤â¤È¤ËÌá¤¹¡£ */
                    restoreBindings();

                    /* ³ÈÄ¥µ¡Ç½¤ÎinitfileÉ½¼¨ÍÑ¤Î¥Ð¥Ã¥Õ¥¡¤ò²òÊü¤¹¤ë */
                    freeBuffer();

#ifndef NO_EXTEND_MENU
                    /* ¥á¥Ë¥å¡¼´ØÏ¢¤Î¥á¥â¥ê¤Î³«Êü */
                    finExtMenu();
#endif

                    /* ¥Ç¥Õ¥©¥ë¥È°Ê³°¤Î¥â¡¼¥ÉÍÑ¥á¥â¥ê¤Î³«Êü */
                    freeExtra();

                    
                    /* ¥­¡¼¥Þ¥Ã¥×¥Æ¡¼¥Ö¥ë¤Î¥¯¥ê¥¢ */
                    restoreDefaultKeymaps();
                  }
                  /* ¥â¡¼¥ÉÊ¸»úÎó¤Î¥Õ¥ê¡¼ */
                  resetModeNames();
#ifndef NO_EXTEND_MENU
                }
              }
            }
          }
        }
      }
    }
#endif
    /* ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿ÍÑ¼«Æ°¥á¥â¥ê´ÉÍý¤Î½ªÎ»½èÍý */
    WStringClose();

    /* ¥µ¡¼¥ÐÌ¾ÊÝ»ýÍÑ¥á¥â¥ê¤Î³«Êü */
    /* RkSetServerName((char *)0); ¤·¤Æ¤Ï¤¤¤±¤Ê¤¤¤Î¤Ç¤Ï¡© */

    /* ¥¨¥ó¥¸¥ó¤Î¥¯¥í¡¼¥º */
    close_engine();

    return -1;
  }
  else {
    /* Á°¤ËInitialize¤ò¤·¤Æ¤¤¤ë¾ì¹ç¤Ë¤Ï¤â¤¦¥á¥Ã¥»¡¼¥¸¤ò¤À¤µ¤Ê¤¤¤³¤È¤Ë¤¹¤ë */
    if (arg) {
      *(char ***)arg = (char **)0;
    }
    return -1;
  }
}

static void
freeKeysup(void)
{
  int i;
  extern keySupplement keysup[];
  extern int nkeysup;

  for (i = 0 ; i < nkeysup ; i++) {
    if (keysup[i].cand) {
      free(keysup[i].cand);
      keysup[i].cand = (WCHAR_T **)0;
    }
    if (keysup[i].fullword) {
      free(keysup[i].fullword);
      keysup[i].fullword = (WCHAR_T *)0;
    }
  }
  nkeysup = 0;
}

extern int nothermodes;

static void
freeBuffer(void)
{
  if(CANNA_initfilename) {
    free(CANNA_initfilename);
  }
  CANNA_initfilename = (char *)NULL;
}

static void
freeExtra(void)
{
  extern extraFunc *extrafuncp;
  extraFunc *p, *q;

  for (p = extrafuncp ; p ; p = q) {
    q = p->next;
    switch (p->keyword) {
      case EXTRA_FUNC_DEFMODE:
        if (p->u.modeptr->romdic_owner &&
	    p->u.modeptr->romdic != (struct RkRxDic *)NULL) {
	  RkwCloseRoma(p->u.modeptr->romdic);
	}
        free(p->u.modeptr->emode);
	if (p->u.modeptr->romaji_table) {
	  free(p->u.modeptr->romaji_table);
	}
        free(p->u.modeptr);
        break;
      case EXTRA_FUNC_DEFSELECTION:
        free(p->u.kigoptr->kigo_str);
        free(p->u.kigoptr->kigo_data);
        free(p->u.kigoptr);
        break;
#ifndef NO_EXTEND_MENU
      case EXTRA_FUNC_DEFMENU:
        freeMenu(p->u.menuptr);
        break;
#endif
    }
    free(p);
  }
  extrafuncp = (extraFunc *)0;
}

static
int KC_finalize(uiContext d, char *arg)
     /* ARGSUSED */
{
  extern int FirstTime;
  int res;
  
  /* ¥¦¥©¡¼¥Ë¥ó¥°¥á¥Ã¥»¡¼¥¸¤Î½é´ü²½ */
  initWarningMesg();
  if (arg) {
    *(char ***)arg = 0;
  }

  if (FirstTime) {
    jrKanjiError = "\275\351\264\374\262\275\244\342\244\265\244\354\244\306"
	"\244\244\244\312\244\244\244\316\244\313\241\330\275\252\244\357"
	"\244\354\241\331\244\310\270\300\244\357\244\354\244\336\244\267"
	"\244\277";
                   /* ½é´ü²½¤â¤µ¤ì¤Æ¤¤¤Ê¤¤¤Î¤Ë¡Ø½ª¤ï¤ì¡Ù¤È¸À¤ï¤ì¤Þ¤·¤¿ */
    return -1;
  }
  else {
    FirstTime = 1;

    /* ¥«¥Ê´Á»úÊÑ´¹¤Î½ªÎ» */
    res = KanjiFin();

    /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¤Î½ªÎ» */
    RomkanaFin();

    /* free all uiContexts and hash tables here */

    /* ¥­¡¼¥Þ¥Ã¥×¥Æ¡¼¥Ö¥ë¤Î¥¯¥ê¥¢ */
    restoreDefaultKeymaps();

    /* ¥â¡¼¥ÉÊ¸»úÎó¤Î¥Õ¥ê¡¼ */
    resetModeNames();

    /* uiContext ¤Î¸¡º÷¤Î¤¿¤á¤Î¥Ï¥Ã¥·¥å¥Æ¡¼¥Ö¥ë¤ò¥¯¥ê¥¢
       uiContext ¤â°ì½ï¤Ë¥Õ¥ê¡¼¤¹¤ë */
    clearHashTable();

    /* µ­¹æÄêµÁ¤ò¾Ã¤¹ */
    freeKeysup();

    /* ¿§¡¹¤È¥«¥¹¥¿¥Þ¥¤¥º¤µ¤ì¤Æ¤¤¤ë½ê¤ò¤â¤È¤ËÌá¤¹¡£ */
    restoreBindings();

    /* ³ÈÄ¥µ¡Ç½¤ÎinitfileÉ½¼¨ÍÑ¤Î¥Ð¥Ã¥Õ¥¡¤ò²òÊü¤¹¤ë */
    freeBuffer();

    /* ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿ÍÑ¼«Æ°¥á¥â¥ê´ÉÍý¤Î½ªÎ»½èÍý */
    WStringClose();

    /* ¥µ¡¼¥ÐÌ¾ÊÝ»ýÍÑ¥á¥â¥ê¤Î³«Êü */
    /* RkSetServerName((char *)0); ¤·¤Æ¤Ï¤¤¤±¤Ê¤¤¤Î¤Ç¤Ï¡© */

#ifndef NO_EXTEND_MENU
    /* ¥á¥Ë¥å¡¼´ØÏ¢¤Î¥á¥â¥ê¤Î³«Êü */
    finExtMenu();
#endif

    /* ¥Ç¥Õ¥©¥ë¥È°Ê³°¤Î¥â¡¼¥ÉÍÑ¥á¥â¥ê¤Î³«Êü */
    freeExtra();

    /* ¥¨¥ó¥¸¥ó¤Î¥¯¥í¡¼¥º */
    close_engine();

    if (arg) {
      *(char ***)arg = nWarningMesg ? WarningMesg : (char **)0;
    }
    return res;
  }
}

static
int KC_setWidth(uiContext d, caddr_t arg)
{
  d->ncolumns = (int)(POINTERINT)arg;
  return 0;
}

static
int KC_setBunsetsuKugiri(uiContext d, caddr_t arg)
     /* ARGSUSED */
{
  cannaconf.BunsetsuKugiri = (int)(POINTERINT)arg;
  return 0;
}

#define CHANGEBUFSIZE 1024

static long gflags[] = {
  0L,
  CANNA_YOMI_BASE_HANKAKU,
  CANNA_YOMI_KATAKANA,
  CANNA_YOMI_KATAKANA | CANNA_YOMI_HANKAKU | CANNA_YOMI_BASE_HANKAKU,
  CANNA_YOMI_ROMAJI | CANNA_YOMI_ZENKAKU,
  CANNA_YOMI_ROMAJI | CANNA_YOMI_BASE_HANKAKU,
  CANNA_YOMI_KAKUTEI,
  CANNA_YOMI_BASE_HANKAKU | CANNA_YOMI_KAKUTEI,
  CANNA_YOMI_KATAKANA | CANNA_YOMI_KAKUTEI,
  CANNA_YOMI_KATAKANA | CANNA_YOMI_HANKAKU | CANNA_YOMI_BASE_HANKAKU |
    CANNA_YOMI_KAKUTEI,
  CANNA_YOMI_ROMAJI | CANNA_YOMI_ZENKAKU | CANNA_YOMI_KAKUTEI,
  CANNA_YOMI_ROMAJI | CANNA_YOMI_BASE_HANKAKU | CANNA_YOMI_KAKUTEI,
};

static
int KC_changeMode(uiContext d, wcKanjiStatusWithValue *arg)
{
  coreContext cc;
  yomiContext yc;

  d->buffer_return = arg->buffer;
  d->n_buffer = arg->n_buffer;
  d->kanji_status_return = arg->ks;

  bzero(d->kanji_status_return, sizeof(wcKanjiStatus));

  d->nbytes = escapeToBasicStat(d, CANNA_FN_Quit);
  cc = (coreContext)d->modec;
  d->kanji_status_return->info &= ~(KanjiThroughInfo | KanjiEmptyInfo);

  if (cc->majorMode == CANNA_MODE_AlphaMode) {
    /* ¦Á¥â¡¼¥É¤À¤Ã¤¿¤éÈ´¤±¤ë¡£
       ¥Ù¡¼¥·¥Ã¥¯¥â¡¼¥É¤Ï¦Á¥â¡¼¥É¤«ÊÑ´¹¥â¡¼¥É¤°¤é¤¤¤·¤«¤Ê¤¤¤È»×¤¦¡£ */
    if (arg->val == CANNA_MODE_AlphaMode) {
      return 0;
    }
    else {
      cc = (coreContext)cc->next; /* ¼¡¤Î¥³¥ó¥Æ¥¯¥¹¥È */
      yc = (yomiContext)cc;

      if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
	CannaBeep();
	arg->val = 0;
	return 0;
      }

      doFunc(d, CANNA_FN_JapaneseMode);
    }
  }
  else {
    yc = (yomiContext)cc;

    if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
      CannaBeep();
      arg->val = 0;
      return 0;
    }
  }

  switch (arg->val) {
  case CANNA_MODE_AlphaMode:
    arg->val = doFunc(d, CANNA_FN_AlphaMode);
    return 0;

  case CANNA_MODE_HenkanMode:
    arg->val = doFunc(d, CANNA_FN_HenkanNyuryokuMode);
    return 0;

#ifndef NO_EXTEND_MENU
  case CANNA_MODE_HexMode:
    arg->val = doFunc(d, CANNA_FN_HexMode);
    return 0;
#endif /* not NO_EXTEND_MENU */

  case CANNA_MODE_BushuMode:
    arg->val = doFunc(d, CANNA_FN_BushuMode);
    return 0;

  case CANNA_MODE_KigoMode:
    arg->val = doFunc(d, CANNA_FN_KigouMode);
    return 0;

  case CANNA_MODE_TourokuMode:
    arg->val = doFunc(d, CANNA_FN_ExtendMode);
    return 0;

  case CANNA_MODE_HanKataHenkanMode:
  case CANNA_MODE_HanKataKakuteiMode:
    if (cannaconf.InhibitHankakuKana) {
      CannaBeep();
      arg->val = 0;
      return 0;
    }
  case CANNA_MODE_ZenHiraHenkanMode:
  case CANNA_MODE_HanHiraHenkanMode:
  case CANNA_MODE_ZenKataHenkanMode:
  case CANNA_MODE_ZenAlphaHenkanMode:
  case CANNA_MODE_HanAlphaHenkanMode:
  case CANNA_MODE_ZenHiraKakuteiMode:
  case CANNA_MODE_HanHiraKakuteiMode:
  case CANNA_MODE_ZenKataKakuteiMode:
  case CANNA_MODE_ZenAlphaKakuteiMode:
  case CANNA_MODE_HanAlphaKakuteiMode:
    yc->generalFlags &= ~(CANNA_YOMI_ATTRFUNCS | CANNA_YOMI_BASE_HANKAKU);
    yc->generalFlags |= gflags[arg->val - CANNA_MODE_ZenHiraHenkanMode];
    EmptyBaseModeInfo(d, yc);
    arg->val = 0;
    return 0;
  default:
    return(-1);
  }
  /* NOTREACHED */
}

static
int baseModeP(uiContext d)
{
  extern KanjiModeRec alpha_mode, empty_mode;

  return (d->current_mode == &alpha_mode) ||
    (d->current_mode == &empty_mode
     && ((yomiContext)(d->modec))->next == (mode_context)0);
}

/*

  ´ðËÜÅª¤Ê¾õÂÖ¤Ë¤â¤É¤ë¡£¤¹¤Ê¤ï¤Á¡¢ÆÉ¤ß¤¬Æþ¤Ã¤Æ¤¤¤¿¤êÊÑ´¹Ãæ¤Î¾õÂÖ¤«¤éÈ´
  ¤±¤ë¡£¤¤¤«¤ËÈ´¤±¤ë¤«¤ÏÂè£²°ú¿ô¤Ç»ØÄê¤¹¤ë¡£È´¤±Êý¤È¤·¤Æ¤Ï

  ¡¦QUIT (C-g) ¤ÇÈ´¤±¤ë
  ¡¦³ÎÄê (Return) ¤ÇÈ´¤±¤ë

  ¤¬¤¢¤ë¡£

*/

int escapeToBasicStat(uiContext d, int how)
{
  int len = 0, totallen = 0;
  WCHAR_T *p = d->buffer_return;
  int totalinfo = 0;
  int maxcount = 32;

  do {
    if(d->kanji_status_return) {
      d->kanji_status_return->length = 0;
      totalinfo |= (d->kanji_status_return->info & KanjiModeInfo);
    }
    else {
      return -1;
    }
    d->kanji_status_return->info = 0;
    d->nbytes = 0; /* ¤³¤ÎÃÍ¤òÆþÎÏÊ¸»úÄ¹¤È¤·¤Æ»È¤¦¾ì¹ç¤¬¤¢¤ë¤Î¤Ç¥¯¥ê¥¢¤¹¤ë */
    len = doFunc(d, how);
    d->buffer_return += len;
    d->n_buffer -= len;
    totallen += len;
    maxcount--;
  } while (maxcount > 0 && !baseModeP(d));
  d->kanji_status_return->info |= KanjiGLineInfo | totalinfo;
  d->kanji_status_return->gline.length = 0;
  d->kanji_status_return->gline.revPos = 0;
  d->kanji_status_return->gline.revLen = 0;
  d->buffer_return = p;
  return totallen;
}

static
int KC_setUFunc(uiContext d, caddr_t arg)
     /* ARGSUSED */
{
  extern int howToBehaveInCaseOfUndefKey;

  howToBehaveInCaseOfUndefKey = (int)(POINTERINT)arg;
  return 0;
}

static
int KC_setModeInfoStyle(uiContext d, caddr_t arg)
     /* ARGSUSED */
{
  int	tmpval;
  extern int howToReturnModeInfo;

  if ((tmpval = (int)(POINTERINT)arg) < 0 || tmpval > MaxModeInfoStyle)
    return(-1);
  howToReturnModeInfo = (int)(POINTERINT)arg;
  return 0;
}

static
int KC_setHexInputStyle(uiContext d, caddr_t arg)
     /* ARGSUSED */
{
  cannaconf.hexCharacterDefiningStyle = (int)(POINTERINT)arg;
  return 0;
}

static
int KC_inhibitHankakuKana(uiContext d, caddr_t arg)
     /* ARGSUSED */
{
  cannaconf.InhibitHankakuKana = (int)(POINTERINT)arg;
  return 0;
}

#ifndef NO_EXTEND_MENU
extern void popTourokuMode (uiContext);

static
int popTourokuWithGLineClear(uiContext d, int retval, mode_context env)
     /* ARGSUSED */
{
  tourokuContext tc;

  popCallback(d); /* ÆÉ¤ß¤ò pop */

  tc = (tourokuContext)d->modec;
  if (tc->udic) {
    free(tc->udic);
  }
  popTourokuMode(d);
  popCallback(d);
  GlineClear(d);
  currentModeInfo(d);
  return 0;
}
#endif

static
int KC_defineKanji(uiContext d, wcKanjiStatusWithValue *arg)
{
#ifdef NO_EXTEND_MENU
  return 0;
#else
  /* I will leave the arrays on stack because the code below will not
     be used in case WIN is defined. 1996.6.5 kon */
  d->buffer_return = arg->buffer;
  d->n_buffer = arg->n_buffer;
  d->kanji_status_return = arg->ks;

  if(arg->ks->length > 0 && arg->ks->echoStr && arg->ks->echoStr[0]) {
    WCHAR_T xxxx[ROMEBUFSIZE];

    WStrncpy(xxxx, arg->ks->echoStr, arg->ks->length);
    xxxx[arg->ks->length] = (WCHAR_T)0;
    
    bzero(d->kanji_status_return, sizeof(wcKanjiStatus));

    d->nbytes = escapeToBasicStat(d, CANNA_FN_Quit);
    d->kanji_status_return->info &= ~(KanjiThroughInfo | KanjiEmptyInfo);
    dicTourokuControl(d, xxxx, popTourokuWithGLineClear);
    arg->val = d->nbytes;
  } else {
    d->nbytes = escapeToBasicStat(d, CANNA_FN_Quit);
    d->kanji_status_return->info &= ~(KanjiThroughInfo | KanjiEmptyInfo);
    arg->val = dicTourokuControl(d, 0, popTourokuWithGLineClear);
  }
  arg->val = callCallback(d, arg->val);

  return 0;
#endif /* NO_EXTEND_MENU */
}


/* cfuncdef

  RK ¥³¥ó¥Æ¥¯¥¹¥È¤òÌµ¸ú¤Ë¤¹¤ë¡£
  flag ¤¬£°°Ê³°¤Ê¤é RkwClose() ¤â¹Ô¤¦¡£

 */

static void
closeRK(int *cxp, int flag)
{
  if (flag && *cxp >= 0) {
    RkwCloseContext(*cxp);
  }
  *cxp = -1;
}

/* cfuncdef

   closeRKContextInUIContext -- uiContext Ãæ¤Î RK ¥³¥ó¥Æ¥¯¥¹¥È¤ò close ¤¹¤ë¡£

 */

static void closeRKContextInUIContext (uiContext, int);

static void
closeRKContextInUIContext(uiContext d, int flag) /* £°°Ê³°¤Ê¤é¥¯¥í¡¼¥º¤â¤¹¤ë¡£ */
{
  coreContext cc;

  closeRK(&(d->contextCache), flag);
  for (cc = (coreContext)d->modec ; cc ; cc = (coreContext)cc->next) {
    if (cc->id == YOMI_CONTEXT) {
      closeRK(&(((yomiContext)cc)->context), flag);
    }
  }
}

/* cfuncdef

  closeRKContextInMemory() -- ¤¹¤Ù¤Æ¤Î RK ¥³¥ó¥Æ¥¯¥¹¥È¤Î¥¯¥í¡¼¥º

  ¥Ð¥±¥Ã¥È¤Ë¤è¤Ã¤Æ¥Ý¥¤¥ó¥È¤µ¤ì¤Æ¤¤¤ë¥Ç¡¼¥¿Æâ¤ÎÁ´¤Æ¤Î RK ¥³¥ó¥Æ¥¯¥¹¥È¤ò
  ¥¯¥í¡¼¥º¤¹¤ë¡£

*/

static void
closeRKContextInMemory(struct bukRec *p, int flag)
{

  while (p) { /* reconfirm that p points some structure */
    closeRKContextInUIContext(p->context, flag);
    p = p->next;
  }
}

/* cfuncdef

  makeContextToBeClosed() -- ¥Ï¥Ã¥·¥å¥Æ¡¼¥Ö¥ëÆâ¤Î¥³¥ó¥Æ¥¯¥¹¥È¤òÌµ¸ú¤Ë¤¹¤ë

*/

void
makeAllContextToBeClosed(int flag)
{
  int i;
  struct bukRec *p;

  for (i = 0 ; i < HASHTABLESIZE ; i++) {
    p = conHash[i];
    if (p) {
      closeRKContextInMemory(p, flag);
    }
  }
}

static
int KC_kakutei(uiContext d, wcKanjiStatusWithValue *arg)
{
  d->buffer_return = arg->buffer;
  d->n_buffer = arg->n_buffer;
  d->kanji_status_return = arg->ks;

  bzero(d->kanji_status_return, sizeof(wcKanjiStatus));

  d->nbytes = escapeToBasicStat(d, CANNA_FN_Kakutei);
  if ( !baseModeP(d) ) {
    d->nbytes = escapeToBasicStat(d, CANNA_FN_Quit);
  }
  d->kanji_status_return->info &= ~KanjiThroughInfo;
  arg->val = d->nbytes;
  return d->nbytes;
}

static
int KC_kill(uiContext d, wcKanjiStatusWithValue *arg)
{
  d->buffer_return = arg->buffer;
  d->n_buffer = arg->n_buffer;
  d->kanji_status_return = arg->ks;

  bzero(d->kanji_status_return, sizeof(wcKanjiStatus));

  d->nbytes = escapeToBasicStat(d, CANNA_FN_Quit);
  d->kanji_status_return->info &= ~KanjiThroughInfo;
  arg->val = d->nbytes;
  return d->nbytes;
}

static
int KC_modekeys(uiContext d, unsigned char *arg)
{
  int n = 0;
  int i;
  extern KanjiModeRec alpha_mode;
  int func;

  for (i = 0 ; i < 256 ; i++) {
    func = alpha_mode.keytbl[i];
    if (func != CANNA_FN_SelfInsert &&
	func != CANNA_FN_FunctionalInsert &&
	func != CANNA_FN_Undefined &&
	func != CANNA_FN_FuncSequence &&
	func != CANNA_FN_UseOtherKeymap	&&
	alpha_mode.func(d, &alpha_mode, KEY_CHECK, 0/*dummy*/, func)) {
      arg[n++] = i;
    }
  }
  return n;
}

static
int KC_queryMode(uiContext d, WCHAR_T *arg)
{
  return queryMode(d, arg);
}

static
int KC_queryConnection(uiContext d, unsigned char *arg)
     /* ARGSUSED */
{
  extern int defaultContext;

  if (defaultContext != -1) {
    return 1;
  }
  else {
    return 0;
  }
}

static
int KC_setServerName(uiContext d, unsigned char *arg)
     /* ARGSUSED */
{
  return RkSetServerName((char *)arg);
}

static
int KC_parse(uiContext d, char **arg)
     /* ARGSUSED */
{
  initWarningMesg();

  parse_string(*arg);

  *(char ***)arg = nWarningMesg ? WarningMesg : (char **)0;

  return nWarningMesg;
}

int yomiInfoLevel = 0;

static
int KC_yomiInfo(uiContext d, int arg)
     /* ARGSUSED */
{
  yomiInfoLevel = arg;
  return 0;
}

static
int KC_storeYomi(uiContext d, wcKanjiStatusWithValue *arg)
{
  extern KanjiModeRec yomi_mode, cy_mode;
  coreContext cc;
  WCHAR_T *p, *q;
  int len = 0;
#ifndef WIN
  WCHAR_T buf[2048];
#else
  WCHAR_T *buf = (WCHAR_T *)malloc(sizeof(WCHAR_T) * 2048);
  if (!buf) {
    /* This should the 'no more memory' message on the guide line... */
    arg->val = 0;
    arg->ks->length = 0;
    arg->ks->info = 0;
    return len;
  }
#endif

  p = arg->ks->echoStr;
  q = arg->ks->mode;
  if (p) {
    WStrcpy(buf, p);
    p = buf;
    len = WStrlen(buf);
  }
  if (q) {
    WStrcpy(buf + len + 1, q);
    q = buf + len + 1;
  }
  KC_kill(d, arg);
  cc = (coreContext)d->modec;
  if (cc->majorMode == CANNA_MODE_AlphaMode) {
    doFunc(d, CANNA_FN_JapaneseMode);
  }
  d->kanji_status_return = arg->ks;
  d->kanji_status_return->info &= ~(KanjiThroughInfo | KanjiEmptyInfo);
  RomajiStoreYomi(d, p, q);
  if (p && *p) {
    d->current_mode =
      (((yomiContext)d->modec)->generalFlags & CANNA_YOMI_CHIKUJI_MODE)
	? &cy_mode : &yomi_mode;
  }
  makeYomiReturnStruct(d);
  arg->val = 0;
#ifdef WIN
  free(buf);
#endif
  return 0;
}

char *initFileSpecified = (char *)NULL;

static
int KC_setInitFileName(uiContext d, char *arg)
     /* ARGSUSED */
{
  int len;

  if (initFileSpecified) { /* °ÊÁ°¤Î¤â¤Î¤ò¥Õ¥ê¡¼¤¹¤ë */
    free(initFileSpecified);
  }

  if ( arg && *arg ) {
    len = strlen(arg);
    initFileSpecified = (char *)malloc(len + 1);
    if (initFileSpecified) {
      strcpy(initFileSpecified, arg);
    }
    else {
      return -1;
    }
  }
  else {
    initFileSpecified = (char *)NULL;
  }
  return 0;
}

static
int KC_do(uiContext d, wcKanjiStatusWithValue *arg)
{
  d->buffer_return = arg->buffer;
  d->n_buffer = arg->n_buffer;
  d->kanji_status_return = arg->ks;
  d->ch = (unsigned)*(d->buffer_return);
  d->nbytes = 1;

  bzero(d->kanji_status_return, sizeof(wcKanjiStatus));

  arg->val = doFunc(d, arg->val);
  return 0;
}

#ifndef NO_EXTEND_MENU
/*

  ¥È¥Ã¥×¥ì¥Ù¥ë¤Ë¤Ï¤Ê¤¤¥â¡¼¥É¤ËÂÐ¤·¤Æ²¿¤é¤«¤Îºî¶È¤ò
  ¤µ¤»¤¿¤¤¤È¤­¤Ë¸Æ¤Ó½Ð¤¹´Ø¿ô¡£fnum == 0 ¤Ç d->ch ¤ò¸«¤ë¡£

  '91.12.28 ¸½ºß¤Ç uldefine.c ¤«¤é¤·¤«¸Æ¤Ð¤ì¤Æ¤ª¤é¤º modec ¤ÎÃÍ¤Ï
  yomi_mode ¤·¤«Æþ¤Ã¤Æ¤¤¤Ê¤¤¡£

  ¤³¤Î´Ø¿ô¤Ï¤½¤ÎÀè¤Ç¸Æ¤Ö´Ø¿ô¤Ë¤è¤Ã¤Æ¥â¡¼¥É¤¬¥×¥Ã¥·¥å¤µ¤ì¤¿»þ¤Ê¤ÉÉüµ¢¤µ
  ¤»¤ë¤³¤È¤¬½ÐÍè¤Ê¤¯¤Ê¤Ã¤Æ¤·¤Þ¤¦¤Î¤Ç¥â¡¼¥ÉÊÑ¹¹¤òÈ¼¤¦µ¡Ç½¤Î¸Æ½Ð¤·¤ò°ì»þ
  Åª¤Ë¶Ø»ß¤¹¤ë¤³¤È¤È¤¹¤ë¡£

 */

int _do_func_slightly(uiContext d, int fnum, mode_context mode_c, KanjiMode c_mode)
{
  wcKanjiStatus ks;
  long gfback;
  BYTE inhback;
  int retval;
  yomiContext yc = (yomiContext)0;
#ifndef WIN
  uiContextRec f, *e = &f;
#else
  uiContext e = (uiContext)malloc(sizeof(uiContextRec));

  if (e) {
#endif

  bzero(e, sizeof(uiContextRec));
  e->buffer_return = e->genbuf;
  e->n_buffer = ROMEBUFSIZE;
  e->kanji_status_return = &ks;

  e->nbytes = d->nbytes;
  e->ch     = d->ch;

  e->status = 0; /* ¥â¡¼¥É¤Ë¤Ä¤¤¤Æ"½èÍýÃæ"¤Î¥¹¥Æ¡¼¥¿¥¹¤ò´ûÄêÃÍ¤È¤¹¤ë */
  e->more.todo = 0;
  e->modec = mode_c;
  e->current_mode = c_mode;
  e->cb = (struct callback *)0;

  if (((coreContext)mode_c)->id == YOMI_CONTEXT) {
    yc = (yomiContext)mode_c;
    gfback = yc->generalFlags;
    inhback = yc->henkanInhibition;
    yc->generalFlags |= CANNA_YOMI_CHGMODE_INHIBITTED;
    yc->henkanInhibition |= CANNA_YOMI_INHIBIT_ALL;
  }

  retval = (*c_mode->func)(e, c_mode, KEY_CALL, e->ch, fnum);

  if (yc) {
    yc->generalFlags = gfback;
    yc->henkanInhibition = inhback;
  }
#ifdef WIN
    free(e);
  }
#endif
  return retval;
}

#endif

static
int callCallback(uiContext d, int res)
{
  struct callback *cbp;

  for (cbp = d->cb; cbp ;) {
    int index;
    int (*callbackfunc) (uiContext, int, mode_context);

    index = d->status;
    d->status = 0; /* Callback ¤¬¤Ê¤¯¤Æ¤â EXIT¡¢QUIT¡¢AUX ¤Ï¥¯¥ê¥¢¤¹¤ë */
    callbackfunc = cbp->func[index];
    if (callbackfunc) {
      d->kanji_status_return->info &= ~KanjiEmptyInfo;
      if (index) { /* everytime °Ê³° */
	res = (*callbackfunc)(d, res, cbp->env);
	cbp = d->cb; /* ¥³¡¼¥ë¥Ð¥Ã¥¯´Ø¿ô¤Ë¤è¤ê¥³¡¼¥ë¥Ð¥Ã¥¯¤¬
			¸Æ¤Ó½Ð¤µ¤ì¤ë¤Î¤ò»Ù±ç¤¹¤ë¤¿¤áÆþ¤ìÄ¾¤¹ */
	/* ¤³¤³¤Ç¥³¡¼¥ë¥Ð¥Ã¥¯´Ø¿ô¤ò¥Ý¥Ã¥×¥¢¥Ã¥×¤·¤è¤¦¤«¤É¤¦¤«¹Í¤¨¤É¤³¤í */
	continue;
      }else{
	res = (*callbackfunc)(d, res, cbp->env);
      }
    }
    cbp = cbp->next;
  }
  return res;
}

int _doFunc(uiContext d, int fnum)
{
  int res = 0, tmpres, ginfo = 0;
  int reallyThrough = 1;
  WCHAR_T *prevEcho, *prevGEcho;
  int prevEchoLen = -1, prevRevPos, prevRevLen;
  int prevGEchoLen, prevGRevPos, prevGRevLen;

  d->status = 0; /* ¥â¡¼¥É¤Ë¤Ä¤¤¤Æ"½èÍýÃæ"¤Î¥¹¥Æ¡¼¥¿¥¹¤ò´ûÄêÃÍ¤È¤¹¤ë */
  d->more.todo = 0;
  tmpres = d->current_mode->func(d, d->current_mode, KEY_CALL, d->ch, fnum);

  if (d->flags & MULTI_SEQUENCE_EXECUTED) {
    d->flags &= ~MULTI_SEQUENCE_EXECUTED;
    return tmpres;
  }

  /* ¥³¡¼¥ë¥Ð¥Ã¥¯¤ò¼Â¹Ô¤¹¤ë */
  res = tmpres = callCallback(d, tmpres);

  if (d->kanji_status_return->length >= 0) {
    prevEcho    = d->kanji_status_return->echoStr;
    prevEchoLen = d->kanji_status_return->length;
    prevRevPos  = d->kanji_status_return->revPos;
    prevRevLen  = d->kanji_status_return->revLen;
  }
  if (d->kanji_status_return->info & KanjiGLineInfo) {
    ginfo = 1;
    prevGEcho    = d->kanji_status_return->gline.line;
    prevGEchoLen = d->kanji_status_return->gline.length;
    prevGRevPos  = d->kanji_status_return->gline.revPos;
    prevGRevLen  = d->kanji_status_return->gline.revLen;
  }

  /* moreToDo ¤â¼Â¹Ô¤·¤Ê¤¯¤Æ¤Ï */
  while (d->more.todo) {
    if (!(d->kanji_status_return->info & KanjiThroughInfo)) {
      reallyThrough = 0;
    }
    d->kanji_status_return->info &= ~(KanjiThroughInfo | KanjiEmptyInfo);
    d->more.todo = 0;
    d->ch = d->more.ch;	/* moreTodo ¤Ë more.ch ¤Ï¤¤¤é¤Ê¤¤¤Î¤Ç¤Ï¡© */
    d->nbytes = 1;
    d->buffer_return += tmpres;
    d->n_buffer -= tmpres;

    {
      int check;
      /* £²²óÌÜ°Ê¹ß¤Ë°Ê²¼¤Î¥Ç¡¼¥¿¤¬¼º¤ï¤ì¤Æ¤¤¤ë¾ì¹ç¤¬¤¢¤ë¤Î¤ÇÆþ¤ìÄ¾¤¹¡£ */
      d->buffer_return[0] = key2wchar(d->ch, &check);
      if (!check) {
	d->nbytes = 0;
      }
    }

    tmpres = _doFunc(d, d->more.fnum);

    if (tmpres >= 0) {
      res += tmpres;
      if (d->kanji_status_return->length >= 0) {
	prevEcho    = d->kanji_status_return->echoStr;
	prevEchoLen = d->kanji_status_return->length;
	prevRevPos  = d->kanji_status_return->revPos;
	prevRevLen  = d->kanji_status_return->revLen;
      }
      if (d->kanji_status_return->info & KanjiGLineInfo) {
	ginfo = 1;
	prevGEcho    = d->kanji_status_return->gline.line;
	prevGEchoLen = d->kanji_status_return->gline.length;
	prevGRevPos  = d->kanji_status_return->gline.revPos;
	prevGRevLen  = d->kanji_status_return->gline.revLen;
      }
    }
  }
  if (!reallyThrough) {
    d->kanji_status_return->info &= ~KanjiThroughInfo;
  }

  d->kanji_status_return->length  = prevEchoLen;
  if (prevEchoLen >= 0) {
    d->kanji_status_return->echoStr = prevEcho;
    d->kanji_status_return->revPos  = prevRevPos;
    d->kanji_status_return->revLen  = prevRevLen;
  }
  if (ginfo) {
    d->kanji_status_return->gline.line    = prevGEcho;
    d->kanji_status_return->gline.length  = prevGEchoLen;
    d->kanji_status_return->gline.revPos  = prevGRevPos;
    d->kanji_status_return->gline.revLen  = prevGRevLen;
    d->kanji_status_return->info |= KanjiGLineInfo;
  }

  return res;
}

int _afterDoFunc(uiContext d, int retval)
{
  int res = retval;
  wcKanjiStatus   *kanji_status_return = d->kanji_status_return;

  /* GLine ¤ò¾Ã¤»¤È¸À¤¦¤Î¤Ê¤é¾Ã¤·¤Þ¤·¤ç¤¦ */
  if (d->flags & PLEASE_CLEAR_GLINE) {
    if (d->flags & PCG_RECOGNIZED) { /* Á°¤ÎÁ°°ÊÁ°¤Ê¤é */
      if (res >= 0 &&	kanji_status_return->length >= 0) {
	d->flags &= ~(PLEASE_CLEAR_GLINE | PCG_RECOGNIZED);
	   /* ¤³¤ì¤ÇÌòÌÜ¤ò²Ì¤¿¤·¤Þ¤·¤¿ */
	if (!(kanji_status_return->info & KanjiGLineInfo)) {
	  GlineClear(d);
	}
      }
    }
    else {
      d->flags |= PCG_RECOGNIZED;
    }
  }
  return res;
}

/* cfuncdef

  doFunc -- _doFunc ¤òÆÉ¤ó¤Ç¡¢¤µ¤é¤Ë ClearGLine ½èÍý¤ä¡¢¥³¡¼¥ë¥Ð¥Ã¥¯¤Î
            ½èÍý¤ò¤¹¤ë¡£

 */

int doFunc(uiContext d, int fnum)
{
  return _afterDoFunc(d, _doFunc(d, fnum));
}

static
int KC_getContext(uiContext d, int arg)
     /* ARGSUSED */
{
  extern int defaultContext, defaultBushuContext;

  switch (arg)
    {
    case 0:
      return RkwDuplicateContext(defaultContext);
    case 1:
      return RkwDuplicateContext(defaultBushuContext);
    case 2:
      return defaultContext;
    default:
      return(-1);
    }
  /* NOTREACHED */
}

static
int KC_closeUIContext(uiContext d, wcKanjiStatusWithValue *arg)
{
  extern struct ModeNameRecs ModeNames[];
  int ret;

  d->buffer_return = arg->buffer;
  d->n_buffer = arg->n_buffer;
  d->kanji_status_return = arg->ks;

  bzero(d->kanji_status_return, sizeof(wcKanjiStatus));

  if ((d->nbytes = escapeToBasicStat(d, CANNA_FN_Quit)) < 0)
    return -1;
  d->kanji_status_return->info &= ~KanjiThroughInfo;
  arg->val = d->nbytes;
  freeRomeStruct(d);

  ret = countContext();
#if defined(DEBUG) && !defined(WIN)
  fprintf(stderr, "ret=%d\n", ret);
#endif
  return ret;
}

static yomiContext
getYomiContext(uiContext d)
{
  coreContext cc = (coreContext)d->modec;
  yomiContext yc;

  switch (cc->id) {
  case YOMI_CONTEXT:
    yc = (yomiContext)cc;
    break;
  default:
    if (cc->minorMode == CANNA_MODE_AlphaMode) {
      yc = (yomiContext)(cc->next);
    }
    else {
      yc = (yomiContext)0;
    }
    break;
  }
  return yc;
}

static
int KC_inhibitChangeMode(uiContext d, int arg)
{
  yomiContext yc;

  yc = getYomiContext(d);
  if (yc) {
    if (arg) {
      yc->generalFlags |= CANNA_YOMI_CHGMODE_INHIBITTED;
    }
    else {
      yc->generalFlags &= ~CANNA_YOMI_CHGMODE_INHIBITTED;
    }
    return 0;
  }
  else {
    return -1;
  }  
}

static
int KC_letterRestriction(uiContext d, int arg)
{
  yomiContext yc;

  yc = getYomiContext(d);
  if (yc) {
    yc->allowedChars = arg;
    return 0;
  }
  else {
    return -1;
  }
}

static
int countColumns(WCHAR_T *str)
{
  int len = 0;
  WCHAR_T *p;

  if (str) {
    for (p = str ; *p ; p++) {
      switch (WWhatGPlain(*p)) {
      case 0:
      case 2:
	len += 1;
	break;
      case 1:
      case 3:
	len += 2;
	break;
      }
    }
  }
  return len;
}

static
int KC_queryMaxModeStr(uiContext d, int arg)
     /* ARGSUSED */
{
  int i, maxcolumns = 0, ncols;
  extern struct ModeNameRecs ModeNames[];
  extern extraFunc *extrafuncp;
  extraFunc *ep;

  for (i = 0 ; i < CANNA_MODE_MAX_IMAGINARY_MODE ; i++) {
    ncols = countColumns(ModeNames[i].name);
    if (ncols > maxcolumns) {
      maxcolumns = ncols;
    }
  }
  for (ep = extrafuncp ; ep ; ep = ep->next) {
    ncols = countColumns(ep->display_name);
    if (ncols > maxcolumns) {
      maxcolumns = ncols;
    }
  }
  return maxcolumns;
}

static int
KC_setListCallback(uiContext d, jrListCallbackStruct *arg)
{
  if (cannaconf.iListCB) {
    d->client_data = (char *)0;
    d->list_func = (int (*) (char *, int, WCHAR_T **, int, int *))0;
    return -1;
  }
  if (arg->callback_func) {
    d->client_data = arg->client_data;
    d->list_func = arg->callback_func;
  }
  else {
    d->client_data = (char *)0;
    d->list_func = (int (*) (char *, int, WCHAR_T **, int, int *))0;
  }
  return 0;
}

static int
KC_setVerbose(uiContext d, int arg)
     /* ARGSUSED */
{
  extern int ckverbose;

  ckverbose = arg;
  return 0;
}

/* kanjiInitialize ¤«¤Ê´Á»úÊÑ´¹¤Î½é´ü²½ KC_INITIALIZE¤ÈÅù²Á¤Ç¤¢¤ë¡£ */

int kanjiInitialize (char ***mes)
{
  return KC_initialize((uiContext)0, (char *)mes);
}

/* kanjiFinalize KC_FINALIZE¤ÈÅù²Á¤Ç¤¢¤ë¡£ */

int kanjiFinalize (char ***mes)
{
  return KC_finalize((uiContext)0, (char *)mes);
}

/* createKanjiContext ¥³¥ó¥Æ¥¯¥¹¥È¤òºîÀ®¤¹¤ë¤â¤Î¤Ç¤¢¤ë¡£ */

static unsigned char context_table[100] = "";

int createKanjiContext()
{
  int i;

  for (i = 0; i < 100; i++) {
    if (!context_table[i]) {
      context_table[i] = 1;
      return i;
    }
  }
  return -1; /* ¥³¥ó¥Æ¥¯¥¹¥È¤ò¼èÆÀ¤Ç¤­¤Ê¤«¤Ã¤¿¡£ */
}

/* wcCloseKanjiContext ¥³¥ó¥Æ¥¯¥¹¥È¤ò¥¯¥í¡¼¥º¤¹¤ë¤â¤Î¡£ */
int
wcCloseKanjiContext (
const int context,
wcKanjiStatusWithValue *ksva)
{
  context_table[context] = 0;
  return  XwcKanjiControl2(0, context, KC_CLOSEUICONTEXT, (BYTE *)ksva);
}

/* jrCloseKanjiContext ¥³¥ó¥Æ¥¯¥¹¥È¤ò¥¯¥í¡¼¥º¤¹¤ë¤â¤Î¡£ */

int
jrCloseKanjiContext (
	const int context,
	jrKanjiStatusWithValue *ksva
)
{
  context_table[context] = 0;
  return  XKanjiControl2(0, context, KC_CLOSEUICONTEXT, (BYTE *)ksva);
}

int
ToggleChikuji(uiContext d, int flg)
{
  yomiContext	yc = (yomiContext)d->modec;
  extern KanjiModeRec empty_mode;
  extern struct CannaConfig cannaconf;

  if ((yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) &&
      yc->context != -1) {
    RkwEndBun(yc->context, 0);
    abandonContext(d, yc);
  }
  (void)escapeToBasicStat(d, CANNA_FN_Kakutei);
  d->kanji_status_return->info &= ~KanjiThroughInfo;

  if (flg) {
    /* Ãà¼¡¼«Æ°¤ËÊÑ¤¨¤ë */
    yc->generalFlags |= CANNA_YOMI_CHIKUJI_MODE;
    yc->majorMode = CANNA_MODE_HenkanMode;
    cannaconf.chikuji = 1;
  }
  else {
    /* Ï¢Ê¸Àá¤ËÊÑ¤¨¤ë */
    yc->generalFlags &= ~CANNA_YOMI_CHIKUJI_MODE;
    yc->majorMode = CANNA_MODE_HenkanMode;
    cannaconf.chikuji = 0;
  }
  yc->minorMode = getBaseMode(yc);
  d->majorMode = d->minorMode = CANNA_MODE_AlphaMode; /* ¥À¥ß¡¼ */
  currentModeInfo(d);
  return 0;
}

static int
KC_lispInteraction(uiContext d, int arg)
/* ARGSUSED */
{
  clisp_main();
  return 0;
}

/*
 * ¥µ¡¼¥Ð¤È¤ÎÀÜÂ³¤òÀÚ¤ë
 */
static int
KC_disconnectServer(uiContext d, int arg)
/* ARGSUSED */
{

#if defined(DEBUG) && !defined(WIN)
  fprintf(stderr,"¥µ¡¼¥Ð¤È¤ÎÀÜÂ³¤òÀÚ¤ë\n");
#endif
  jrKanjiPipeError();
  return 0;
}

static int
KC_setAppName(uiContext d, unsigned char *arg)
/* ARGSUSED */
{
  extern int defaultContext;

  if (strlen((char *)arg) > CANNA_MAXAPPNAME) {
    strncpy(saveapname, (char *)arg, CANNA_MAXAPPNAME);
    saveapname[CANNA_MAXAPPNAME - 1] = '\0';
  }
  else {
    strcpy(saveapname, (char *)arg);
  }
  RkwSetAppName(defaultContext, saveapname);

  return(0);
}

static int
KC_debugmode(uiContext d, int arg)
/* ARGSUSED */
{
  extern int iroha_debug;

  iroha_debug = arg;
  return 0;
}

static void
debug_yomibuf(yomiContext yc)
/* ARGSUSED */
{
#if defined(DEBUG) && !defined(WIN)
  char kana[1024], roma[1024], ka[1024], ya[1024], *kanap, *romap, *kap, *yap;
  int len, i, j, k, maxcol, columns, tmp;
  WCHAR_T xxx[1024];

#define MANYSPACES "                                                  "

  kanap = kana; romap = roma; kap = ka; yap = ya;

  for (i = 0, j = 0 ; i < yc->kEndp || j < yc->rEndp ;) {
    maxcol = 0;

    if (i < yc->kEndp) {
      k = i + 1;
      columns = 0;
      tmp = 
	(WIsG0(yc->kana_buffer[i]) || WIsG2(yc->kana_buffer[i])) ? 1 : 2;
      if (i == yc->kRStartp && i != yc->kCurs) {
	*kanap++ = '\'';
	*kap++ = '\'';
	columns++;
      }
      if (yc->kAttr[i] & HENKANSUMI) {
	*kap++ = ' ';
      }else{
	*kap++ = 'x';
      }
      if (tmp > 1) {
	*kap++ = ' ';
      }
      columns += tmp;
      while (!(yc->kAttr[k] & SENTOU)) {
	tmp = (WIsG0(yc->kana_buffer[k]) || WIsG2(yc->kana_buffer[k])) ? 1 : 2;
	columns += tmp;
	if (yc->kAttr[k] & HENKANSUMI) {
	  *kap++ = ' ';
	}
	else {
	  *kap++ = 'x';
	}
	if (tmp > 1) {
	  *kap++ = ' ';
	}
	k++;
      }
      WStrncpy(xxx, yc->kana_buffer + i, k - i);
      xxx[k - i] = (WCHAR_T)0;
      sprintf(kanap, "%ws ", xxx);
      *kap++ = ' ';
      len = strlen(kanap);
      if (columns > maxcol) {
	maxcol = columns;
      }else{
	strncpy(kanap + len, MANYSPACES, maxcol - columns);
	strncpy(kap, MANYSPACES, maxcol - columns);
	kap += maxcol - columns;
	len += maxcol - columns;
      }
      kanap += len;
      i = k;
    }

    if (j < yc->rEndp) {
      k = j + 1;
      columns = 
	(WIsG0(yc->romaji_buffer[j]) || WIsG2(yc->romaji_buffer[j])) ? 1 : 2;
      if (j == yc->rStartp && j != yc->rCurs) {
	*romap++ = '\'';
	columns++;
      }
      while (!(yc->rAttr[k] & SENTOU)) {
	columns += 
	  (WIsG0(yc->romaji_buffer[k]) || WIsG2(yc->romaji_buffer[k])) ? 1 : 2;
	k++;
      }
      WStrncpy(xxx, yc->romaji_buffer + j, k - j);
      xxx[k - j] = (WCHAR_T)0;
      sprintf(romap, "%ws ", xxx);
      len = strlen(romap);
      if (columns > maxcol) {
	strncpy(kanap, MANYSPACES, columns - maxcol);
	kanap += columns - maxcol;
	strncpy(kap, MANYSPACES, columns - maxcol);
	kap += columns - maxcol;
	maxcol = columns;
      }else{
	strncpy(romap + len, MANYSPACES, maxcol - columns);
	len += maxcol - columns;
      }
      romap += len;
      j = k;
    }
  }
  *kap = *kanap = *romap = '\0';
  printf("%s\n", roma);
  printf("%s\n", kana);
  printf("%s\n", ka);
#endif /* DEBUG */
}

static int
KC_debugyomi(uiContext d, int arg)
/* ARGSUSED */
{
  if (((coreContext)(d)->modec)->id == YOMI_CONTEXT) {
    debug_yomibuf((yomiContext)d->modec);
  }
  return 0;
}

static int
KC_queryPhono(uiContext d, char *arg)
/* ARGSUSED */
{
  extern struct RkRxDic *romajidic;
  struct RkRxDic **foo = (struct RkRxDic **)arg;

  *foo = romajidic;
  return 0;
}

static int
KC_changeServer(uiContext d, char *arg)
/* ARGSUSED */
{
  extern int defaultContext;
  char *p;

  if (!arg) {
    RkSetServerName((char *)0);
    return 0;
  }

  jrKanjiPipeError();
  if (RkSetServerName((char *)arg) && (p = index((char *)arg, '@'))) {
#ifndef WIN
    char xxxx[512];
#else
    char *xxxx = malloc(512);
    if (!xxxx) {
      return 0;
    }
#endif

    *p = '\0';
#ifndef WIN
    sprintf(xxxx, "¤«¤Ê´Á»úÊÑ´¹¥¨¥ó¥¸¥ó %s ¤ÏÍøÍÑ¤Ç¤­¤Þ¤»¤ó", (char *)arg);
#else
    sprintf(xxxx, "\244\253\244\312\264\301\273\372\312\321\264\271\245\250\245\363\245\270\245\363 %s \244\317\315\370\315\321\244\307\244\255\244\336\244\273\244\363\n",
	    (char *)arg);
#endif
    makeGLineMessageFromString(d, xxxx);

    RkSetServerName((char *)0);
#ifdef WIN
    free(xxxx);
#endif
    return 0;
  }

  if (defaultContext == -1) {
    if ((KanjiInit() != 0) || (defaultContext == -1)) {
#ifndef WIN
      jrKanjiError = "¤«¤Ê´Á»úÊÑ´¹¥µ¡¼¥Ð¤ÈÄÌ¿®¤Ç¤­¤Þ¤»¤ó";
#else
      jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271"
                     "\245\265\241\274\245\320\244\310\304\314\277\256"
                     "\244\307\244\255\244\336\244\273\244\363";
#endif
      return 0;
    }
  }
  return (int)RkwGetServerName();
}

static int
KC_setUserInfo(uiContext d, jrUserInfoStruct *arg)
/* ARGSUSED */
{
  extern jrUserInfoStruct *uinfo;
  int ret = -1;
  char *uname, *gname, *srvname, *topdir, *cannafile, *romkanatable;
#ifndef WIN
  char buf[256];
#else
  char *buf = malloc(256);
  if (!buf) {
    return -1;
  }
#endif

  if (arg) {
    uname = arg->uname ? strdup(arg->uname) : (char *)0;
    if (uname || !arg->uname) {
      gname = arg->gname ? strdup(arg->gname) : (char *)0;
      if (gname || !arg->gname) {
        srvname = arg->srvname ? strdup(arg->srvname) : (char *)0;
        if (srvname || !arg->srvname) {
	  topdir = arg->topdir ? strdup(arg->topdir) : (char *)0;
          if (topdir || !arg->topdir) {
	    cannafile = arg->cannafile ? strdup(arg->cannafile) : (char *)0;
            if (cannafile || !arg->cannafile) {
              romkanatable =
		arg->romkanatable ? strdup(arg->romkanatable) : (char *)0;
              if (romkanatable || !arg->romkanatable) {
                uinfo = (jrUserInfoStruct *)malloc(sizeof(jrUserInfoStruct));
                if (uinfo) {
                  uinfo->uname = uname;
                  uinfo->gname = gname;
                  uinfo->srvname = srvname;
                  uinfo->topdir = topdir;
                  uinfo->cannafile = cannafile;
                  uinfo->romkanatable = romkanatable;

                  if (uinfo->srvname) {
		    KC_setServerName(d, (unsigned char *)uinfo->srvname);
		  }
                  if (uinfo->cannafile) {
                    char *p = uinfo->cannafile;

                    if (strlen(p) >= 3 && (p[0] == '\\' || p[0] == '/' ||
                        p[1] == ':' && p[2] == '\\' ||
                        p[1] == ':' && p[2] == '/'))
                      strcpy(buf, p);
                    else if (uinfo->uname)
                      sprintf(buf, "%s/%s/%s/%s/%s",
                              uinfo->topdir ? uinfo->topdir : "",
			      "dic", "user",
                              uinfo->uname, uinfo->cannafile);
                    else
                      buf[0] = '\0';
                  }
                  else {
                    sprintf(buf, "%s/%s",
			    uinfo->topdir ? uinfo->topdir : "", "default.can");
                  }
                  wcKanjiControl((int)d, KC_SETINITFILENAME, buf);
		  RkwSetUserInfo(uinfo->uname, uinfo->gname, uinfo->topdir);
                  ret = 1;
		  goto return_ret;
                }
		if (romkanatable) free(romkanatable);
              }
              if (cannafile) free(cannafile);
            }
            if (topdir) free(topdir);
          }
          if (srvname) free(srvname);
        }
        if (gname) free(gname);
      }
      if (uname) free(uname);
    }
#ifndef WIN
    jrKanjiError = "malloc (SetUserinfo) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "malloc (SetUserinfo) \244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277";
#endif
  }
  ret = -1;

 return_ret:
#ifdef WIN
  free(buf);
#endif
  return ret;
}

static int
KC_queryCustom(uiContext d, jrCInfoStruct *arg)
/* ARGSUSED */
{
  extern struct CannaConfig cannaconf;
  static char *input_code[CANNA_MAX_CODE] = {"jis", "sjis", "kuten"};

  if (/* 0 <= cannaconf.code_input && // unsigned ¤Ê¤Î¤ÇÉ¬¤º¿¿ */
      cannaconf.code_input <= CANNA_MAX_CODE) {
    strcpy(arg->codeinput, input_code[cannaconf.code_input]);
  }
  arg->quicklyescape = cannaconf.quickly_escape;
  arg->indexhankaku = cannaconf.indexHankaku;
  arg->indexseparator = cannaconf.indexSeparator;
  arg->selectdirect = cannaconf.SelectDirect;
  arg->numericalkeysel = cannaconf.HexkeySelect;
  arg->kouhocount = cannaconf.kCount;

  return 0;
}

static int
KC_closeAllContext(uiContext d, char *arg)
/* ARGSUSED */
{
  makeAllContextToBeClosed(1);
  return 0;
}

static int
KC_attributeInfo(uiContext d, char *arg)
{
  wcKanjiAttributeInternal **p = (wcKanjiAttributeInternal **)arg;

  if (p) {
    if (!d->attr) {
      d->attr = (wcKanjiAttributeInternal *)
	malloc(sizeof(wcKanjiAttributeInternal));
      if (d->attr) {
	d->attr->u.attr = (char *)malloc(ROMEBUFSIZE);
	if (d->attr->u.attr) {
	  d->attr->len = ROMEBUFSIZE;
	  *p = d->attr;
	  return 0;
	}
	free(d->attr);
	d->attr = (wcKanjiAttributeInternal *)0;
      }
    }
    else { /* called twice */
      *p = d->attr;
      return 0;
    }
  }
  else if (d->attr) { /* && !p */
    free(d->attr->u.attr);
    free(d->attr);
    d->attr = (wcKanjiAttributeInternal *)0;
    return 0;
  }
  return -1;
}

/* KanjiControl¤Î¸Ä¡¹¤ÎÀ©¸æ´Ø¿ô¤Ø¤Î¥Ý¥¤¥ó¥¿ */

static int (*kctlfunc[MAX_KC_REQUEST])(...) = {
  (int (*)(...))KC_initialize,
  (int (*)(...))KC_finalize,
  (int (*)(...))KC_changeMode,
  (int (*)(...))KC_setWidth,
  (int (*)(...))KC_setUFunc,
  (int (*)(...))KC_setBunsetsuKugiri,
  (int (*)(...))KC_setModeInfoStyle,
  (int (*)(...))KC_setHexInputStyle,
  (int (*)(...))KC_inhibitHankakuKana,
  (int (*)(...))KC_defineKanji,
  (int (*)(...))KC_kakutei,
  (int (*)(...))KC_kill,
  (int (*)(...))KC_modekeys,
  (int (*)(...))KC_queryMode,
  (int (*)(...))KC_queryConnection,
  (int (*)(...))KC_setServerName,
  (int (*)(...))KC_parse,
  (int (*)(...))KC_yomiInfo,
  (int (*)(...))KC_storeYomi,
  (int (*)(...))KC_setInitFileName,
  (int (*)(...))KC_do,
  (int (*)(...))KC_getContext,
  (int (*)(...))KC_closeUIContext,
  (int (*)(...))KC_inhibitChangeMode,
  (int (*)(...))KC_letterRestriction,
  (int (*)(...))KC_queryMaxModeStr,
  (int (*)(...))KC_setListCallback,
  (int (*)(...))KC_setVerbose,
  (int (*)(...))KC_lispInteraction,
  (int (*)(...))KC_disconnectServer,
  (int (*)(...))KC_setAppName,
  (int (*)(...))KC_debugmode,
  (int (*)(...))KC_debugyomi,
  (int (*)(...))KC_keyconvCallback,
  (int (*)(...))KC_queryPhono,
  (int (*)(...))KC_changeServer,
  (int (*)(...))KC_setUserInfo,
  (int (*)(...))KC_queryCustom,
  (int (*)(...))KC_closeAllContext,
  (int (*)(...))KC_attributeInfo,
};

int kanjiControl(int request, uiContext d, caddr_t arg)
{
  return kctlfunc[request](d, arg);
}
