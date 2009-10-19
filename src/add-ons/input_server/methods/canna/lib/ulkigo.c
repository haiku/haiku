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
static char rcs_id[] = "@(#) 102.1 $Id: ulkigo.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif

#include	<errno.h>
#include "canna.h"

#ifndef NO_EXTEND_MENU
extern int uiUtilIchiranTooSmall();

static
char *srussia_data[] = 
{
  /*"§¡", "§¢", "§£", "§¤", "§¥", "§¦", "§§", "§¨",*/
  "\247\241", "\247\242", "\247\243", "\247\244", "\247\245", "\247\246", "\247\247", "\247\250", 

  /*"§©", "§ª", "§«", "§¬", "§­", "§®", "§¯", "§°", */
  "\247\251", "\247\252", "\247\253", "\247\254", "\247\255", "\247\256", "\247\257", "\247\260",

  /*"§±", "§²", "§³", "§´", "§µ", "§¶", "§·", "§¸",*/
  "\247\261", "\247\262", "\247\263", "\247\264", "\247\265", "\247\266", "\247\267", "\247\270", 

  /*"§¹", "§º", "§»", "§¼", "§½", "§¾", "§¿", "§À", */
  "\247\271", "\247\272", "\247\273", "\247\274", "\247\275", "\247\276", "\247\277", "\247\300", 

  /*"§Á", "§Ñ", "§Ò", "§Ó", "§Ô", "§Õ", "§Ö", "§×",*/
  "\247\301", "\247\321", "\247\322", "\247\323", "\247\324", "\247\325", "\247\326", "\247\327", 

  /*"§Ø", "§Ù", "§Ú", "§Û", "§Ü", "§Ý", "§Þ", "§ß",*/
  "\247\330", "\247\331", "\247\332", "\247\333", "\247\334", "\247\335", "\247\336", "\247\337",

  /*"§à", "§á", "§â", "§ã", "§ä", "§å", "§æ", "§ç",*/
  "\247\340", "\247\341", "\247\342", "\247\343", "\247\344", "\247\345", "\247\346", "\247\347", 

  /*"§è", "§é", "§ê", "§ë", "§ì", "§í", "§î", "§ï",*/
  "\247\350", "\247\351", "\247\352", "\247\353", "\247\354", "\247\355", "\247\356", "\247\357", 

  /*"§ð", "§ñ", */
  "\247\360", "\247\361",
};

#define	UURD_SZ	(sizeof(srussia_data) / sizeof(char *))

static
char *sgreek_data[] =  
{
  /* "¦¡", "¦¢", "¦£", "¦¤", "¦¥", "¦¦", "¦§", "¦¨", */
  "\246\241", "\246\242", "\246\243", "\246\244", "\246\245", "\246\246", "\246\247", "\246\250",

  /* "¦©", "¦ª", "¦«", "¦¬", "¦­", "¦®", "¦¯", "¦°", */
  "\246\251", "\246\252", "\246\253", "\246\254", "\246\255", "\246\256", "\246\257", "\246\260",

  /* "¦±", "¦²", "¦³", "¦´", "¦µ", "¦¶", "¦·", "¦¸", */
  "\246\261", "\246\262", "\246\263", "\246\264", "\246\265", "\246\266", "\246\267", "\246\270",

  /* "¦Á", "¦Â", "¦Ã", "¦Ä", "¦Å", "¦Æ", "¦Ç", "¦È", */
  "\246\301", "\246\302", "\246\303", "\246\304", "\246\305", "\246\306", "\246\307", "\246\310",

  /* "¦É", "¦Ê", "¦Ë", "¦Ì", "¦Í", "¦Î", "¦Ï", "¦Ð", */
  "\246\311", "\246\312", "\246\313", "\246\314", "\246\315", "\246\316", "\246\317", "\246\320",

  /* "¦Ñ", "¦Ò", "¦Ó", "¦Ô", "¦Õ", "¦Ö", "¦×", "¦Ø", */
  "\246\321", "\246\322", "\246\323", "\246\324", "\246\325", "\246\326", "\246\327", "\246\330",
};

#define	UUGD_SZ	(sizeof(sgreek_data) / sizeof(char *))

static int uuKigoExitDo(uiContext d, int retval);
static int uuKigoRExitCatch(uiContext d, int retval, mode_context env);
static int uuKigoGExitCatch(uiContext d, int retval, mode_context env);
static int uuKigoKExitCatch(uiContext d, int retval, mode_context env);
static int uuKigoQuitCatch(uiContext d, int retval, mode_context env);
static int kigoZenpan(uiContext d);

static WCHAR_T *russia_data[UURD_SZ];
static WCHAR_T *greek_data[UUGD_SZ];

int
initUlKigoTable(void)
{
  int retval;

  retval = setWStrings(russia_data, srussia_data, UURD_SZ);
  if (retval != NG) {
    retval = setWStrings(greek_data, sgreek_data, UUGD_SZ);
  }
  return retval;
}

#ifdef pcux_r32 
static
char *skeisen_data[] =  
{ 
  /* "¬¤", "¬¥", "¬¦", "¬§", "¬¨", "¬©", "¬ª", "¬«", */
  "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244",

  /* "¬¬", "¬­", "¬®", "¬¯", "¬°", "¬±", "¬²", "¬³", */
  "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244",
  
  /* "¬´", "¬µ", "¬¶", "¬·", "¬¸", "¬¹", "¬º", "¬»", */
  "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244",
  
  /* "¬¼", "¬½", "¬¾", "¬¿", "¬À", "¬Á", "¬Â", "¬Ã", */
  "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244",
  
  /* "¬Ä", "¬Å", "¬Æ", "¬Ç", "¬È", "¬É", "¬Ê", "¬Ë", */
  "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244",
  
  /* "¬Ì", "¬Í", "¬Î", "¬Ï", "¬Ð", "¬Ñ", "¬Ò", "¬Ó", */
  "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244",
  
  /* "¬Ô", "¬Õ", "¬Ö", "¬×", "¬Ø", "¬Ù", "¬Ú", "¬Û", */
  "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244",
  
  /* "¬Ü", "¬Ý", "¬Þ", "¬ß", "¬à", "¬á", "¬â", "¬ã", */
  "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244",
  
  /* "¬ä", "¬å", "¬æ", "¬ç", "¬è", "¬é", "¬ê", "¬ë", */
  "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244", "\254\244",
  
  /*  "¬ì", "¬í", "¬î", "¬ï", */
  "\254\244", "\254\244", "\254\244", "\254\244",
};
#else /* EWS-UX/V */
static
char *skeisen_data[] =  
{ 
  /* "¨¡", "¨¢", "¨£", "¨¤", "¨¥", "¨¦", "¨§", "¨¨", */
  "\250\241", "\250\242", "\250\243", "\250\244", "\250\245", "\250\246", "\250\247", "\250\250",
  
  /* "¨©", "¨ª", "¨«", "¨¬", "¨­", "¨®", "¨¯", "¨°", */
  "\250\251", "\250\252", "\250\253", "\250\254", "\250\255", "\250\256", "\250\257", "\250\260",
  
  /* "¨±", "¨²", "¨³", "¨´", "¨µ", "¨¶", "¨·", "¨¸", */
  "\250\261", "\250\262", "\250\263", "\250\264", "\250\265", "\250\266", "\250\267", "\250\270",
  
  /* "¨¹", "¨º", "¨»", "¨¼", "¨½", "¨¾", "¨¿", "¨À", */
  "\250\271", "\250\272", "\250\273", "\250\274", "\250\275", "\250\276", "\250\277", "\250\300",
};
#endif


#define UUKD_SZ (sizeof(skeisen_data) / sizeof(char *))

static WCHAR_T *keisen_data[UUKD_SZ];

int
initUlKeisenTable(void)
{
  return setWStrings(keisen_data, skeisen_data, UUKD_SZ);
}

static
int uuKigoExitDo(uiContext d, int retval)
{
  popForIchiranMode(d);
  popCallback(d);
  retval = YomiExit(d, retval);
  currentModeInfo(d);

  killmenu(d);

  return(retval);
}

static
int uuKigoRExitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  forichiranContext fc;

  popCallback(d); /* °ìÍ÷¤ò pop */

  fc = (forichiranContext)d->modec;
  d->currussia = fc->curIkouho;

  return(uuKigoExitDo(d, retval));
}

static
int uuKigoGExitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  forichiranContext fc;

  popCallback(d); /* °ìÍ÷¤ò pop */

  fc = (forichiranContext)d->modec;
  d->curgreek = fc->curIkouho;

  return(uuKigoExitDo(d, retval));
}

static
int uuKigoKExitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  forichiranContext fc;

  popCallback(d); /* °ìÍ÷¤ò pop */

  fc = (forichiranContext)d->modec;
  d->curkeisen = fc->curIkouho;

  return(uuKigoExitDo(d, retval));
}

int uuKigoGeneralExitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  forichiranContext fc;

  popCallback(d); /* °ìÍ÷¤ò pop */

  fc = (forichiranContext)d->modec;
  if (fc->prevcurp) {
    *(fc->prevcurp) = fc->curIkouho;
  }

  return(uuKigoExitDo(d, retval));
}

static
int uuKigoQuitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d); /* °ìÍ÷¤ò pop */

  popForIchiranMode(d);
  popCallback(d);
  currentModeInfo(d);

  return prevMenuIfExist(d);
}

int uuKigoMake(uiContext d, WCHAR_T **allkouho, int size, char cur, char mode, int (*exitfunc )(...), int *posp)
{
  forichiranContext fc;
  ichiranContext ic;
  unsigned inhibit = 0;
  int retval = 0;

  d->status = 0;

  if((retval = getForIchiranContext(d)) == NG) {
    return(GLineNGReturn(d));
  }
  fc = (forichiranContext)d->modec;

  /* selectOne ¤ò¸Æ¤Ö¤¿¤á¤Î½àÈ÷ */
  fc->allkouho = allkouho;
  fc->curIkouho = 0;
  fc->prevcurp = posp;
  inhibit |= (unsigned char)NUMBERING;

  if((retval = selectOne(d, fc->allkouho, &fc->curIkouho, size,
		 KIGOBANGOMAX, inhibit, 0, WITH_LIST_CALLBACK,
		 NO_CALLBACK, (int (*)(_uiContext*, int, _coreContextRec*))exitfunc,
		 uuKigoQuitCatch, uiUtilIchiranTooSmall)) == NG) {
    return(GLineNGReturnFI(d));
  }

  ic = (ichiranContext)d->modec;
  ic->minorMode = mode;
  ic->flags |= cannaconf.quickly_escape ? 0 : ICHIRAN_STAY_LONG;
  currentModeInfo(d);

  *(ic->curIkouho) = (int)cur;

  /* ¸õÊä°ìÍ÷¹Ô¤¬¶¹¤¯¤Æ¸õÊä°ìÍ÷¤¬½Ð¤»¤Ê¤¤ */
  if(ic->tooSmall) {
    d->status = AUX_CALLBACK;
    return(retval);
  }

  if ( !(ic->flags & ICHIRAN_ALLOW_CALLBACK) ) {
    makeGlineStatus(d);
  }

  /* d->status = ICHIRAN_EVERYTIME; */

  return(retval);
}

#if 0
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * µ­¹æ°ìÍ÷                                                                  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
kigoZenpan(uiContext d)
{
  if(makeKigoIchiran(d, CANNA_MODE_ExtendMode) == NG) /* 0 ¤Ï³ÈÄ¥¤Îµ­¹æ°ìÍ÷ */
    return(GLineNGReturn(d));
  else
    return(0);
}
#endif

#endif /* NO_EXTEND_MENU */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * ¥í¥·¥¢Ê¸»ú¤ÎÆþÎÏ                                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int kigoRussia(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    
#ifdef NO_EXTEND_MENU
  d->kanji_status_return->info |= KanjiRussianInfo;
  return 0;
#else
  return(uuKigoMake(d, (WCHAR_T **)russia_data, UURD_SZ,
           d->currussia,
		   CANNA_MODE_RussianMode,
		   (int(*)(...))uuKigoRExitCatch, (int *)0));
#endif
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * ¥®¥ê¥·¥ãÊ¸»ú¤ÎÆþÎÏ                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int kigoGreek(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    
#ifdef NO_EXTEND_MENU
  d->kanji_status_return->info |= KanjiGreekInfo;
  return 0;
#else
  return(uuKigoMake(d, (WCHAR_T **)greek_data, UUGD_SZ,
           d->curgreek, CANNA_MODE_GreekMode, (int(*)(...))uuKigoGExitCatch, (int *)0));
#endif
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * ·ÓÀþ¤ÎÆþÎÏ                                                                *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int kigoKeisen(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }
#ifdef NO_EXTEND_MENU
  d->kanji_status_return->info |= KanjiLineInfo;
  return 0;
#else
  return(uuKigoMake(d, (WCHAR_T **)keisen_data, UUKD_SZ,
           d->curkeisen, CANNA_MODE_LineMode, (int(*)(...))uuKigoKExitCatch, (int *)0));
#endif
}
