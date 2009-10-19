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
static	char	rcs_id[] = "@(#) 102.1 $Id: yesno.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif  /* lint */

#include	<errno.h>
#include	"canna.h"

#ifdef luna88k
extern int errno;
#endif

static coreContext newYesNoContext(void);
static void freeYesNoContext(coreContext qc);
static void popYesNoMode(uiContext d);
static int YesNoNop(uiContext d);
static int YesNo(uiContext d);
static int YesNoQuit(uiContext d);

/* cfunc yesNoContext
 *
 * yesNoContext
 *
 */
static coreContext
newYesNoContext(void)
{
  coreContext ccxt;

  if ((ccxt = (coreContext)malloc(sizeof(coreContextRec)))
                                       == (coreContext)NULL) {
#ifndef WIN
    jrKanjiError = "malloc (newcoreContext) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "malloc (newcoreContext) \244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277";
#endif
    return (coreContext)NULL;
  }
  ccxt->id = CORE_CONTEXT;

  return ccxt;
}

static void
freeYesNoContext(coreContext qc)
{
  free(qc);
}

/*
 * ¸õÊä°ìÍ÷¹Ô¤òºî¤ë
 */
int getYesNoContext(uiContext d, canna_callback_t everyTimeCallback, canna_callback_t exitCallback, canna_callback_t quitCallback, canna_callback_t auxCallback)
{
  extern KanjiModeRec tourokureibun_mode;
  coreContext qc;
  int retval = 0;

  if(pushCallback(d, d->modec,
	everyTimeCallback, exitCallback, quitCallback, auxCallback) == 0) {
    jrKanjiError = "malloc (pushCallback) \244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277";
                  /* ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
    return(NG);
  }
  
  if((qc = newYesNoContext()) == (coreContext)NULL) {
    popCallback(d);
    return(NG);
  }
  qc->majorMode = d->majorMode;
  qc->minorMode = CANNA_MODE_HenkanMode;
  qc->next = d->modec;
  d->modec = (mode_context)qc;

  qc->prevMode = d->current_mode;
  d->current_mode = &tourokureibun_mode;

  return(retval);
}

static void
popYesNoMode(uiContext d)
{
  coreContext qc = (coreContext)d->modec;

  d->modec = qc->next;
  d->current_mode = qc->prevMode;
  freeYesNoContext(qc);
}

#if DOYESNONOP
/*
  Nop ¤òºî¤í¤¦¤È¤·¤¿¤¬¡¢ getYesNoContext ¤ò¸Æ¤Ó½Ð¤·¤Æ¤¤¤ë¤È¤³¤í¤Ç¡¢
  everyTimeCallback ¤òÀßÄê¤·¤Æ¤¤¤Ê¤¤¤Î¤Ç¡¢²¼¤Î½èÍý¤¬¤¦¤Þ¤¯Æ°¤«¤Ê¤¤
 */

static
YesNoNop(uiContext d)
{
  /* currentModeInfo ¤Ç¥â¡¼¥É¾ðÊó¤¬É¬¤ºÊÖ¤ë¤è¤¦¤Ë¥À¥ß¡¼¤Î¥â¡¼¥É¤òÆþ¤ì¤Æ¤ª¤¯ */
  d->majorMode = d->minorMode = CANNA_MODE_AlphaMode;
  currentModeInfo(d);
  return 0;
}
#endif /* DOYESNONOP */

/*
 * EveryTimeCallback ... y/n °Ê³°¤ÎÊ¸»ú¤¬ÆþÎÏ¤µ¤ì¤¿
 * ExitCallback ...      y ¤¬ÆþÎÏ¤µ¤ì¤¿
 * quitCallback ...      quit ¤¬ÆþÎÏ¤µ¤ì¤¿
 * auxCallback ...       n ¤¬ÆþÎÏ¤µ¤ì¤¿
 */

static int YesNo (uiContext);

static
int YesNo(uiContext d)
{
  if((d->ch == 'y') || (d->ch == 'Y')) {
    popYesNoMode(d);
    d->status = EXIT_CALLBACK;
  } else if((d->ch == 'n') || (d->ch == 'N')) {
    popYesNoMode(d);
    d->status = AUX_CALLBACK;
  } else {
    /* d->status = EVERYTIME_CALLBACK; */
    return(NothingChangedWithBeep(d));
  }

  return(0);
}

static int YesNoQuit (uiContext);

static
int YesNoQuit(uiContext d)
{
  int retval = 0;

  popYesNoMode(d);
  d->status = QUIT_CALLBACK;
  
  return(retval);
}

#include	"t_reimap.h"
