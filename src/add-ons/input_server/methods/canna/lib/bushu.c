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
static char rcs_id[] = "@(#) 102.1 $Id: bushu.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif /* lint */

#include	<errno.h>
#include "canna.h"
#include "RK.h"
#include "RKintern.h"

extern WCHAR_T *WString();



#define	BUSHU_SZ	150

static
char *bushu_schar[] = 
{ 
  /* "°ì", "Ð¨", "Ñá", "½½", "ÒÇ", "Åá", */
  "\260\354", "\320\250", "\321\341", "\275\275", "\322\307", "\305\341",
  
  /* "´¢¡Ê¤ê¤Ã¤È¤¦¡Ë", "ÎÏ", "ÒÌ", "Ò±", "ÑÄÒ¹Óø", "Ðµ", */
  "\264\242\241\312\244\352\244\303\244\310\244\246\241\313", "\316\317", "\322\314", "\322\261", "\321\304\322\271\323\370", "\320\265",
  
  /* "ÑÒ", "¿Í¡¿¿Î¡Ê¤Ë¤ó¤Ù¤ó¡Ë", "Ëô", "ÑÜ", "È¬", "Ñ¹", */
  "\321\322", "\277\315\241\277\277\316\241\312\244\313\244\363\244\331\244\363\241\313", "\313\364", "\321\334", "\310\254", "\321\271",
  
  /* "ÑÌ", "Õß", "×®", "°ê¡Ê¤ª¤ª¤¶¤È)", "¸Ê", "½÷", */
  "\321\314", "\325\337", "\327\256", "\260\352\241\312\244\252\244\252\244\266\244\310\51", "\270\312", "\275\367",
  
  /* "×Æ", "¸ý", "Áð¡Ê¤¯¤µ¤«¤ó¤à¤ê)", "ÆÈ¡Ê¤±¤â¤Î¤Ø¤ó¡Ë", */
  "\327\306", "\270\375", "\301\360\241\312\244\257\244\265\244\253\244\363\244\340\244\352\51", "\306\310\241\312\244\261\244\342\244\316\244\330\244\363\241\313",

  /* "»Ò", "ïú¡Ê¤³¤¶¤È¡Ë", "»Î", "¹¾¡Ê¤µ¤ó¤º¤¤¡Ë", "×µ", */
  "\273\322", "\357\372\241\312\244\263\244\266\244\310\241\313", "\273\316", "\271\276\241\312\244\265\244\363\244\272\244\244\241\313", "\327\265",
  
  /* "Õù", "¾®¡¿Ã±¡Ê¤Ä¡Ë", "íè¡Ê¤·¤ó¤Ë¤ç¤¦¡Ë", "À£", "Âç", */
  "\325\371", "\276\256\241\277\303\261\241\312\244\304\241\313", "\355\350\241\312\244\267\244\363\244\313\244\347\244\246\241\313", "\300\243", "\302\347",
  
  /* "ÅÚ", "¼ê¡Ê¤Æ¤Ø¤ó¡Ë", "¶Ò", "Öø", "»³", "Í¼", */
  "\305\332", "\274\352\241\312\244\306\244\330\244\363\241\313", "\266\322", "\326\370", "\273\263", "\315\274",
  
  /* "µÝ", "Ë»¡Ê¤ê¤Ã¤·¤ó¤Ù¤ó¡Ë", "·ç", "ÝÆ", "¸¤", */
  "\265\335", "\313\273\241\312\244\352\244\303\244\267\244\363\244\331\244\363\241\313", "\267\347", "\335\306", "\270\244",
  
  /* "µí¡¿²´¡Ê¤¦¤·¤Ø¤ó¡Ë", "ÊÒ", "ÌÚ", "Ýã", "ÌÓ", "¿´", */
  "\265\355\241\277\262\264\241\312\244\246\244\267\244\330\244\363\241\313", "\312\322", "\314\332", "\335\343", "\314\323", "\277\264",
  
  /* "¿å", "·î", "ÄÞ", "Æü", "Ú¾", "²Ð", */
  "\277\345", "\267\356", "\304\336", "\306\374", "\332\276", "\262\320",
  
  /* "Êý", "Øù", "ÅÀ¡Ê¤ì¤Ã¤«¡Ë", "ÝÕ", "·ê", "ÀÐ", */
  "\312\375", "\330\371", "\305\300\241\312\244\354\244\303\244\253\241\313", "\335\325", "\267\352", "\300\320",

  /* "¶Ì", "Èé", "´¤", "»®", "¼¨", "¿À¡Ê¤·¤á¤¹¤Ø¤ó¡Ë", "Çò", */
  "\266\314", "\310\351", "\264\244", "\273\256", "\274\250", "\277\300\241\312\244\267\244\341\244\271\244\330\244\363\241\313", "\307\362",
  
  /* "ÅÄ", "Î©", "²Ó", "ÌÜ", "â¢", "Ìð", */
  "\305\304", "\316\251", "\262\323", "\314\334", "\342\242", "\314\360",
  
  /* "áË¡Ê¤ä¤Þ¤¤¤À¤ì¡Ë", "»Í", "»å", "±±", "±»", "Ï·", */
  "\341\313\241\312\244\344\244\336\244\244\244\300\244\354\241\313", "\273\315", "\273\345", "\261\261", "\261\273", "\317\267",
  
  /* "´Ì", "°á", "½é¡Ê¤³¤í¤â¤Ø¤ó¡Ë", "ÊÆ", "Àå", "æÐ", */
  "\264\314", "\260\341", "\275\351\241\312\244\263\244\355\244\342\244\330\244\363\241\313", "\312\306", "\300\345", "\346\320",
  
  /* "ÃÝ¡Ê¤¿¤±¤«¤ó¤à¤ê¡Ë", "·ì", "¸×¡Ê¤È¤é¤«¤ó¤à¤ê¡Ë", "Æù", */
  "\303\335\241\312\244\277\244\261\244\253\244\363\244\340\244\352\241\313", "\267\354", "\270\327\241\312\244\310\244\351\244\253\244\363\244\340\244\352\241\313", "\306\371",
  
  /* "À¾", "±©", "ÍÓ", "ææ", "½®", "¼ª", */
  "\300\276", "\261\251", "\315\323", "\346\346", "\275\256", "\274\252",
  
  /* "Ãî", "ÀÖ", "Â­¡¿É¥", "ìµ", "¿Ã", */
  "\303\356", "\300\326", "\302\255\241\277\311\245", "\354\265", "\277\303",
  
  /* "³­", "¿É", "¼Ö", "¸«", "¸À", "ÆÓ", "Áö", "Ã«", */
  "\263\255", "\277\311", "\274\326", "\270\253", "\270\300", "\306\323", "\301\366", "\303\253",
  
  /* "³Ñ", "ÈÐ", "Çþ", "Æ¦", "¿È", "ì¸", "±«", "Èó", */
  "\263\321", "\310\320", "\307\376", "\306\246", "\277\310", "\354\270", "\261\253", "\310\363",
  
  /* "¶â", "Ìç", "ð²", "ÊÇ", "²»", "¹á", "³×", "É÷", */
  "\266\342", "\314\347", "\360\262", "\312\307", "\262\273", "\271\341", "\263\327", "\311\367",
  
  /* "¼ó", "¿©", "ðê", "ÌÌ", "ÇÏ", "µ´", "ñõ", "¹â", */
  "\274\363", "\277\251", "\360\352", "\314\314", "\307\317", "\265\264", "\361\365", "\271\342",
  
  /* "ò¨", "¹ü", "µû", "µµ", "Ä»", "¹õ", "¼¯", "É¡", */
  "\362\250", "\271\374", "\265\373", "\265\265", "\304\273", "\271\365", "\274\257", "\311\241",

  /* "óï", "µ­¹æ", "¤½¤ÎÂ¾" */
  "\363\357", "\265\255\271\346", "\244\275\244\316\302\276"
};

static
char *bushu_skey[] =  
{ 
/* "¤¤¤Á", "¤Î", "¤¦¤±¤Ð¤³", "¤¸¤å¤¦", "¤Õ¤·", "¤«¤¿¤Ê", */
"\244\244\244\301", "\244\316", "\244\246\244\261\244\320\244\263", "\244\270\244\345\244\246", "\244\325\244\267", "\244\253\244\277\244\312",

/* "¤ê¤Ã¤È¤¦", "¤«", "¤¬¤ó", "¤¯", "¤«¤Þ¤¨", "¤Ê¤Ù", "¤Ë", */
"\244\352\244\303\244\310\244\246", "\244\253", "\244\254\244\363", "\244\257", "\244\253\244\336\244\250", "\244\312\244\331", "\244\313",

/* "¤Ò¤È", "¤Ì", "¤Ä¤¯¤¨", "¤Ï¤Á", "¤ë", "¤ï", */
"\244\322\244\310", "\244\314", "\244\304\244\257\244\250", "\244\317\244\301", "\244\353", "\244\357",

/* "¤¦", "¤¨¤ó", "¤ª¤ª¤¶¤È", "¤ª¤Î¤ì", "¤ª¤ó¤Ê", "¤®¤ç¤¦", */
"\244\246", "\244\250\244\363", "\244\252\244\252\244\266\244\310", "\244\252\244\316\244\354", "\244\252\244\363\244\312", "\244\256\244\347\244\246",

/* "¤í", "¤¯¤µ", "¤±¤â¤Î", "¤³", "¤³¤¶¤È", "¤µ¤à¤é¤¤", */
"\244\355", "\244\257\244\265", "\244\261\244\342\244\316", "\244\263", "\244\263\244\266\244\310", "\244\265\244\340\244\351\244\244",

/* "¤·", "¤·¤­", "¤·¤ã¤¯", "¤Ä", "¤·¤ó", "¤¹¤ó", */
"\244\267", "\244\267\244\255", "\244\267\244\343\244\257", "\244\304", "\244\267\244\363", "\244\271\244\363",

/* "¤À¤¤", "¤É", "¤Æ", "¤Ï¤Ð", "¤Þ", "¤ä¤Þ", */
"\244\300\244\244", "\244\311", "\244\306", "\244\317\244\320", "\244\336", "\244\344\244\336",

/* "¤æ¤¦", "¤æ¤ß", "¤ê¤Ã¤·¤ó", "¤±¤Ä", "¤¤¤Á¤¿", "¤¤¤Ì", */
"\244\346\244\246", "\244\346\244\337", "\244\352\244\303\244\267\244\363", "\244\261\244\304", "\244\244\244\301\244\277", "\244\244\244\314",

/* "¤¦¤·", "¤«¤¿", "¤­", "¤­¤¬¤Þ¤¨", "¤±", "¤³¤³¤í", */
"\244\246\244\267", "\244\253\244\277", "\244\255", "\244\255\244\254\244\336\244\250", "\244\261", "\244\263\244\263\244\355",

/* "¤¹¤¤", "¤Ä¤­", "¤Ä¤á", "¤Ë¤Á", "¤Î¤Ö¤ó", "¤Ò", */
"\244\271\244\244", "\244\304\244\255", "\244\304\244\341", "\244\313\244\301", "\244\316\244\326\244\363", "\244\322",

/* "¤Û¤¦", "¤Û¤³", "¤è¤Ä¤Æ¤ó", "¤ë¤Þ¤¿", "¤¢¤Ê", "¤¤¤·", */
"\244\333\244\246", "\244\333\244\263", "\244\350\244\304\244\306\244\363", "\244\353\244\336\244\277", "\244\242\244\312", "\244\244\244\267",

/* "¤ª¤¦", "¤«¤ï", "¤«¤ï¤é", "¤µ¤é", "¤·¤á¤¹", "¤Í", */
"\244\252\244\246", "\244\253\244\357", "\244\253\244\357\244\351", "\244\265\244\351", "\244\267\244\341\244\271", "\244\315",

/* "¤·¤í", "¤¿", "¤¿¤Ä", "¤Î¤®", "¤á", "¤Ï¤Ä", "¤ä", */
"\244\267\244\355", "\244\277", "\244\277\244\304", "\244\316\244\256", "\244\341", "\244\317\244\304", "\244\344",

/* "¤ä¤Þ¤¤", "¤è¤ó", "¤¤¤È", "¤¦¤¹", "¤¦¤ê", "¤ª¤¤", */
"\244\344\244\336\244\244", "\244\350\244\363", "\244\244\244\310", "\244\246\244\271", "\244\246\244\352", "\244\252\244\244",

/* "¤«¤ó", "¤­¤Ì", "¤³¤í¤â", "¤³¤á", "¤·¤¿", "¤¹¤­", */
"\244\253\244\363", "\244\255\244\314", "\244\263\244\355\244\342", "\244\263\244\341", "\244\267\244\277", "\244\271\244\255",

/* "¤¿¤±", "¤Á", "¤È¤é", "¤Ë¤¯", "¤Ë¤·", "¤Ï¤Í", "¤Ò¤Ä¤¸", */
"\244\277\244\261", "\244\301", "\244\310\244\351", "\244\313\244\257", "\244\313\244\267", "\244\317\244\315", "\244\322\244\304\244\270",

/* "¤Õ¤Ç", "¤Õ¤Í", "¤ß¤ß", "¤à¤·", "¤¢¤«", "¤¢¤·", */
"\244\325\244\307", "\244\325\244\315", "\244\337\244\337", "\244\340\244\267", "\244\242\244\253", "\244\242\244\267",

/* "¤¤¤Î¤³", "¤ª¤ß", "¤«¤¤", "¤«¤é¤¤", "¤¯¤ë¤Þ", "¤±¤ó", */
"\244\244\244\316\244\263", "\244\252\244\337", "\244\253\244\244", "\244\253\244\351\244\244", "\244\257\244\353\244\336", "\244\261\244\363",

/* "¤´¤ó", "¤µ¤±", "¤½¤¦", "¤¿¤Ë", "¤Ä¤Î", "¤Î¤´¤á", */
"\244\264\244\363", "\244\265\244\261", "\244\275\244\246", "\244\277\244\313", "\244\304\244\316", "\244\316\244\264\244\341",

/* "¤Ð¤¯", "¤Þ¤á", "¤ß", "¤à¤¸¤Ê", "¤¢¤á", "¤¢¤é¤º", */
"\244\320\244\257", "\244\336\244\341", "\244\337", "\244\340\244\270\244\312", "\244\242\244\341", "\244\242\244\351\244\272",

/* "¤«¤Í", "¤â¤ó", "¤Õ¤ë¤È¤ê", "¤Ú¡¼¤¸", "¤ª¤È", "¤³¤¦", */
"\244\253\244\315", "\244\342\244\363", "\244\325\244\353\244\310\244\352", "\244\332\241\274\244\270", "\244\252\244\310", "\244\263\244\246",

/* "¤«¤¯", "¤«¤¼", "¤¯¤Ó", "¤·¤ç¤¯", "¤Ê¤á¤·", "¤á¤ó", */
"\244\253\244\257", "\244\253\244\274", "\244\257\244\323", "\244\267\244\347\244\257", "\244\312\244\341\244\267", "\244\341\244\363",

/* "¤¦¤Þ", "¤ª¤Ë", "¤«¤ß", "¤¿¤«¤¤", "¤È¤¦", "¤Û¤Í", */
"\244\246\244\336", "\244\252\244\313", "\244\253\244\337", "\244\277\244\253\244\244", "\244\310\244\246", "\244\333\244\315",

/* "¤¦¤ª", "¤«¤á", "¤È¤ê", "¤¯¤í", "¤·¤«", "¤Ï¤Ê", */
"\244\246\244\252", "\244\253\244\341", "\244\310\244\352", "\244\257\244\355", "\244\267\244\253", "\244\317\244\312",

/* "¤Ï", "¤­¤´¤¦", "¤½¤Î¤¿" */
"\244\317", "\244\255\244\264\244\246", "\244\275\244\316\244\277"
};


#define	BUSHU_CNT	(sizeof(bushu_schar)/sizeof(char *))


static forichiranContext newForIchiranContext(void);
static int vBushuMode(uiContext d, int major_mode);
static int vBushuIchiranQuitCatch(uiContext d, int retval, mode_context env);
static int vBushuExitCatch(uiContext d, int retval, mode_context env);
static int bushuEveryTimeCatch(uiContext d, int retval, mode_context env);
static int bushuExitCatch(uiContext d, int retval, mode_context env);
static int bushuQuitCatch(uiContext d, int retval, mode_context env);
static int convBushuQuitCatch(uiContext d, int retval, mode_context env);
static int bushuHenkan(uiContext d, int flag, int ext, int cur, int (*quitfunc )(uiContext, int, mode_context ));
static int makeBushuIchiranQuit(uiContext d, int flag);

static WCHAR_T *bushu_char[BUSHU_CNT];
static WCHAR_T *bushu_key[BUSHU_CNT];

int
initBushuTable(void)
{
  int retval = 0;

  retval = setWStrings(bushu_char, bushu_schar, BUSHU_CNT);
  if (retval != NG) {
    retval = setWStrings(bushu_key, bushu_skey, BUSHU_CNT);
  }
  return retval;
}


/*
 * Éô¼ó¸õÊä¤Î¥¨¥³¡¼ÍÑ¤ÎÊ¸»úÎó¤òºî¤ë
 *
 * °ú¤­¿ô	RomeStruct
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0
 */
inline int
makeBushuEchoStr(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;

  d->kanji_status_return->echoStr = ic->allkouho[*(ic->curIkouho)];
  d->kanji_status_return->length = 1;
  d->kanji_status_return->revPos = 0;
  d->kanji_status_return->revLen = 1;

  return(0);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * forichiranContextÍÑ´Ø¿ô                                                   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * forichiranContext ¤Î½é´ü²½
 */
inline int
clearForIchiranContext(forichiranContext p)
{
  p->id = FORICHIRAN_CONTEXT;
  p->curIkouho = 0;
  p->allkouho = 0;

  return(0);
}
  
static forichiranContext
newForIchiranContext(void)
{
  forichiranContext fcxt;

  if ((fcxt = (forichiranContext)malloc(sizeof(forichiranContextRec)))
                                             == (forichiranContext)NULL) {
#ifndef WIN
    jrKanjiError = "malloc (newForIchiranContext) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "malloc (newForIchiranContext) \244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277";  /* ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
#endif
    return (forichiranContext)NULL;
  }
  clearForIchiranContext(fcxt);

  return fcxt;
}

int
getForIchiranContext(uiContext d)
{
  forichiranContext fc;
  int retval = 0;

  if (pushCallback(d, d->modec, NO_CALLBACK, NO_CALLBACK,
                                  NO_CALLBACK, NO_CALLBACK) == 0) {
#ifndef WIN
    jrKanjiError = "malloc (pushCallback) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "malloc (pushCallback) \244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277"; /* ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
#endif
    return(NG);
  }
  
  if((fc = newForIchiranContext()) == NULL) {
    popCallback(d);
    return(NG);
  }
  fc->next = d->modec;
  d->modec = (mode_context)fc;

  fc->prevMode = d->current_mode;
  fc->majorMode = d->majorMode;

  return(retval);
}

void
popForIchiranMode(uiContext d)
{
  forichiranContext fc = (forichiranContext)d->modec;

  d->modec = fc->next;
  d->current_mode = fc->prevMode;
  freeForIchiranContext(fc);
}

#ifndef NO_EXTEND_MENU
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Éô¼ó¥â¡¼¥ÉÆþÎÏ                                                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static int
vBushuMode(uiContext d, int major_mode)
{
  forichiranContext fc;
  ichiranContext ic;
  unsigned inhibit = 0;
  int retval = 0;

  d->status = 0;

  if((retval = getForIchiranContext(d)) == NG) {
    killmenu(d);
    return(GLineNGReturn(d));
  }

  fc = (forichiranContext)d->modec;

  /* selectOne ¤ò¸Æ¤Ö¤¿¤á¤Î½àÈ÷ */
  fc->allkouho = bushu_char;
  fc->curIkouho = 0;
  if (!cannaconf.HexkeySelect)
    inhibit |= ((unsigned char)NUMBERING | (unsigned char)CHARINSERT);
  else
    inhibit |= (unsigned char)CHARINSERT;

  if((retval = selectOne(d, fc->allkouho, &fc->curIkouho, BUSHU_SZ,
		 BANGOMAX, inhibit, 0, WITH_LIST_CALLBACK,
		 NO_CALLBACK, vBushuExitCatch,
		 bushuQuitCatch, uiUtilIchiranTooSmall)) == NG) {
    killmenu(d);
    return(GLineNGReturnFI(d));
  }

  ic = (ichiranContext)d->modec;
  ic->majorMode = major_mode;
  ic->minorMode = CANNA_MODE_BushuMode;
  currentModeInfo(d);

  *(ic->curIkouho) = d->curbushu;

  /* ¸õÊä°ìÍ÷¹Ô¤¬¶¹¤¯¤Æ¸õÊä°ìÍ÷¤¬½Ð¤»¤Ê¤¤ */
  if(ic->tooSmall) {
    d->status = AUX_CALLBACK;
    killmenu(d);
    return(retval);
  }

  if ( !(ic->flags & ICHIRAN_ALLOW_CALLBACK) ) {
    makeGlineStatus(d);
  }
  /* d->status = ICHIRAN_EVERYTIME; */

  return(retval);
}

static int
vBushuIchiranQuitCatch(uiContext d, int retval, mode_context env)
     /* ARGSUSED */
{
  popCallback(d); /* °ìÍ÷¤ò¥Ý¥Ã¥× */

  if (((forichiranContext)env)->allkouho != (WCHAR_T **)bushu_char) {
    /* bushu_char ¤Ï static ¤ÎÇÛÎó¤À¤«¤é free ¤·¤Æ¤Ï¤¤¤±¤Ê¤¤¡£
       ¤³¤¦¸À¤¦¤Î¤Ã¤Æ¤Ê¤ó¤«±ø¤¤¤Ê¤¢ */
    freeGetIchiranList(((forichiranContext)env)->allkouho);
  }
  popForIchiranMode(d);
  popCallback(d);

  return(vBushuMode(d, CANNA_MODE_BushuMode));
}

static int
vBushuExitCatch(uiContext d, int retval, mode_context env)
     /* ARGSUSED */
{
  forichiranContext fc;
  int cur, res;

  popCallback(d); /* °ìÍ÷¤ò¥Ý¥Ã¥× */

  fc = (forichiranContext)d->modec;
  cur = fc->curIkouho;

  popForIchiranMode(d);
  popCallback(d);

  res = bushuHenkan(d, 1, 1, cur, vBushuIchiranQuitCatch);
  if (res < 0) {
    makeYomiReturnStruct(d);
    return 0;
  }
  return res;
}

int
BushuMode(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    killmenu(d);
    return NothingChangedWithBeep(d);
  }    

  return(vBushuMode(d, CANNA_MODE_BushuMode));
}
#endif /* not NO_EXTEND_MENU */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Éô¼ó¥â¡¼¥ÉÆþÎÏ¤Î°ìÍ÷É½¼¨                                                  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static int bushuEveryTimeCatch (uiContext, int, mode_context);

static int
bushuEveryTimeCatch(uiContext d, int retval, mode_context env)
     /* ARGSUSED */
{
  makeBushuEchoStr(d);

  return(retval);
}

static int bushuExitCatch (uiContext, int, mode_context);

static int
bushuExitCatch(uiContext d, int retval, mode_context env)
{
  yomiContext yc;

  popCallback(d); /* °ìÍ÷¤ò¥Ý¥Ã¥× */

  if (((forichiranContext)env)->allkouho != bushu_char) {
    /* bushu_char ¤Ï static ¤ÎÇÛÎó¤À¤«¤é free ¤·¤Æ¤Ï¤¤¤±¤Ê¤¤¡£
       ¤³¤¦¸À¤¦¤Î¤Ã¤Æ¤Ê¤ó¤«±ø¤¤¤Ê¤¢ */
    freeGetIchiranList(((forichiranContext)env)->allkouho);
  }
  popForIchiranMode(d);
  popCallback(d);
  yc = (yomiContext)d->modec;
  if (yc->savedFlags & CANNA_YOMI_MODE_SAVED) {
    restoreFlags(yc);
  }
  retval = YomiExit(d, retval);
  killmenu(d);
  currentModeInfo(d);

  return retval;
}

#ifndef NO_EXTEND_MENU
static int
bushuQuitCatch(uiContext d, int retval, mode_context env)
     /* ARGSUSED */
{
  popCallback(d); /* °ìÍ÷¤ò¥Ý¥Ã¥× */

  if (((forichiranContext)env)->allkouho != (WCHAR_T **)bushu_char) {
    /* bushu_char ¤Ï static ¤ÎÇÛÎó¤À¤«¤é free ¤·¤Æ¤Ï¤¤¤±¤Ê¤¤¡£
       ¤³¤¦¸À¤¦¤Î¤Ã¤Æ¤Ê¤ó¤«±ø¤¤¤Ê¤¢ */
    freeGetIchiranList(((forichiranContext)env)->allkouho);
  }
  popForIchiranMode(d);
  popCallback(d);
  currentModeInfo(d);
  GlineClear(d);

  return prevMenuIfExist(d);
}
#endif /* not NO_EXTEND_MENU */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Éô¼ó¤È¤·¤Æ¤ÎÊÑ´¹¤Î°ìÍ÷É½¼¨                                                *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static int
convBushuQuitCatch(uiContext d, int retval, mode_context env)
{
  popCallback(d); /* °ìÍ÷¤ò¥Ý¥Ã¥× */

  if (((forichiranContext)env)->allkouho != (WCHAR_T **)bushu_char) {
    /* bushu_char ¤Ï static ¤ÎÇÛÎó¤À¤«¤é free ¤·¤Æ¤Ï¤¤¤±¤Ê¤¤¡£
       ¤³¤¦¸À¤¦¤Î¤Ã¤Æ¤Ê¤ó¤«±ø¤¤¤Ê¤¢ */
    freeGetIchiranList(((forichiranContext)env)->allkouho);
  }
  popForIchiranMode(d);
  popCallback(d);

  makeYomiReturnStruct(d);
  currentModeInfo(d);

  return(retval);
}

/*
 * ÆÉ¤ß¤òÉô¼ó¤È¤·¤ÆÊÑ´¹¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
int ConvertAsBushu (uiContext);

int
ConvertAsBushu(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int res;

  d->status = 0; /* clear status */
  
  if (yc->henkanInhibition & CANNA_YOMI_INHIBIT_ASBUSHU ||
      yc->right || yc->left) {
    return NothingChangedWithBeep(d);
  }

  if (yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) {
    if (!(yc->status & CHIKUJI_OVERWRAP) && yc->nbunsetsu) {
      moveToChikujiTanMode(d);
      return TanKouhoIchiran(d);
    }
    else if (yc->nbunsetsu) {
      return NothingChanged(d);
    }
  }

  d->nbytes = yc->kEndp;
  WStrncpy(d->buffer_return, yc->kana_buffer, d->nbytes);

  /* 0 ¤Ï¡¢ConvertAsBushu ¤«¤é¸Æ¤Ð¤ì¤¿¤³¤È¤ò¼¨¤¹ */
  res = bushuHenkan(d, 0, 1, 0, convBushuQuitCatch);
  if (res < 0) {
    makeYomiReturnStruct(d);
    return 0;
  }
  return res;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * ¶¦ÄÌÉô                                                                    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * ÆÉ¤ß¤òÉô¼ó¼­½ñ¤«¤éÉô¼óÊÑ´¹¤¹¤ë
 */
inline int
bushuBgnBun(RkStat *st, WCHAR_T *yomi, int length)
{
  int nbunsetsu;
  extern int defaultBushuContext;

  /* Ï¢Ê¸ÀáÊÑ´¹¤ò³«»Ï¤¹¤ë *//* ¼­½ñ¤Ë¤¢¤ë¸õÊä¤Î¤ß¼è¤ê½Ð¤¹ */
  if ((defaultBushuContext == -1)) {
    if (KanjiInit() == -1 || defaultBushuContext == -1) {
      jrKanjiError = KanjiInitError();
      return(NG);
    }
  }

  nbunsetsu = RkwBgnBun(defaultBushuContext, yomi, length, RK_CTRLHENKAN);
  if(nbunsetsu == -1) {
    if(errno == EPIPE)
      jrKanjiPipeError();
    jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\313\274\272\307\324\244\267\244\336\244\267\244\277"; 
	    /* ¤«¤Ê´Á»úÊÑ´¹¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    return(NG);
  }
  
  if(RkwGetStat(defaultBushuContext, st) == -1) {
    if(errno == EPIPE)
      jrKanjiPipeError();
    jrKanjiError = "\245\271\245\306\245\244\245\277\245\271\244\362\274\350\244\352\275\320\244\273\244\336\244\273\244\363\244\307\244\267\244\277";
                  /* ¥¹¥Æ¥¤¥¿¥¹¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿ */
    return(NG);
  }

  return(nbunsetsu);
}

/*
 * ÆÉ¤ß¤ËÈ¾ÂùÅÀ¤òÉÕ²Ã¤·¤Æ¸õÊä°ìÍ÷¹Ô¤òÉ½¼¨¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 *		flag	ConvertAsBushu¤«¤é¸Æ¤Ð¤ì¤¿ 0
 *			BushuYomiHenkan¤«¤é¸Æ¤Ð¤ì¤¿ 1
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 *
 *
 * ¤³¤³¤ËÍè¤ë»þ¤Ï¤Þ¤À getForIchiranContext ¤¬¸Æ¤Ð¤ì¤Æ¤¤¤Ê¤¤¤â¤Î¤È¤¹¤ë
 */

static int
bushuHenkan(uiContext d, int flag, int ext, int cur, int (*quitfunc )(uiContext, int, mode_context ))
{
  forichiranContext fc;
  ichiranContext ic;
  unsigned inhibit = 0;
  WCHAR_T *yomi, **allBushuCands;
  RkStat	st;
  int nelem, currentkouho, nbunsetsu, length, retval = 0;
  extern int defaultBushuContext;
  

  if(flag) {
    yomi = (WCHAR_T *)bushu_key[cur];
    length = WStrlen(yomi);
    d->curbushu = (short)cur;
  } else {
    d->nbytes = RomajiFlushYomi(d, d->buffer_return, d->n_buffer);
    yomi = d->buffer_return;
    length = d->nbytes;
  }

  if((nbunsetsu = bushuBgnBun(&st, yomi, length)) == NG) {
    killmenu(d);
    (void)GLineNGReturn(d);
    return -1;
  }

  if((nbunsetsu != 1) || (st.klen > 1) || (st.maxcand == 0)) {
    /* Éô¼ó¤È¤·¤Æ¤Î¸õÊä¤¬¤Ê¤¤ */

    d->kanji_status_return->length = -1;

    makeBushuIchiranQuit(d, flag);
    currentModeInfo(d);

    killmenu(d);
    if(flag) {
      makeGLineMessageFromString(d, "\244\263\244\316\311\364\274\363\244\316\270\365\312\344\244\317\244\242\244\352\244\336\244\273\244\363");
                                  /* ¤³¤ÎÉô¼ó¤Î¸õÊä¤Ï¤¢¤ê¤Þ¤»¤ó */
    } else {
      return(NothingChangedWithBeep(d));
    }
    return(0);
  }

  /* ¸õÊä°ìÍ÷¹Ô¤òÉ½¼¨¤¹¤ë */
  /* 0 ¤Ï¡¢¥«¥ì¥ó¥È¸õÊä + 0 ¤ò¥«¥ì¥ó¥È¸õÊä¤Ë¤¹¤ë¤³¤È¤ò¼¨¤¹ */

  if((allBushuCands
      = getIchiranList(defaultBushuContext, &nelem, &currentkouho)) == 0) {
    killmenu(d);
    (void)GLineNGReturn(d);
    return -1;
  }

  /* Éô¼óÊÑ´¹¤Ï³Ø½¬¤·¤Ê¤¤¡£ */
  if(RkwEndBun(defaultBushuContext, 0) == -1) { /* 0:³Ø½¬¤·¤Ê¤¤ */
    if(errno == EPIPE)
      jrKanjiPipeError();
    jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\316\275\252\316\273\244\313\274\272\307\324\244\267\244\336\244\267\244\277";
                   /* ¤«¤Ê´Á»úÊÑ´¹¤Î½ªÎ»¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    freeGetIchiranList(allBushuCands);
    killmenu(d);
    (void)GLineNGReturn(d);
    return -1;
  }

  if(getForIchiranContext(d) == NG) {
    freeGetIchiranList(allBushuCands);
    killmenu(d);
    (void)GLineNGReturn(d);
    return -1;
  }

  fc = (forichiranContext)d->modec;
  fc->allkouho = allBushuCands;

  if (!cannaconf.HexkeySelect)
    inhibit |= (unsigned char)NUMBERING;
  fc->curIkouho = currentkouho;	/* ¸½ºß¤Î¥«¥ì¥ó¥È¸õÊäÈÖ¹æ¤òÊÝÂ¸¤¹¤ë */
  currentkouho = 0;	/* ¥«¥ì¥ó¥È¸õÊä¤«¤é²¿ÈÖÌÜ¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë¤« */

  if((retval = selectOne(d, fc->allkouho, &fc->curIkouho, nelem, BANGOMAX,
			 inhibit, currentkouho, WITH_LIST_CALLBACK,
			 bushuEveryTimeCatch, bushuExitCatch,
			 quitfunc, uiUtilIchiranTooSmall)) == NG) {
    freeGetIchiranList(allBushuCands);
    killmenu(d);
    (void)GLineNGReturnFI(d);
    return -1;
  }

  ic = (ichiranContext)d->modec;

  if(!flag) { /* convertAsBushu */
    ic->majorMode = ic->minorMode = CANNA_MODE_BushuMode;
  } else {
    if(ext) {
      ic->majorMode = ic->minorMode = CANNA_MODE_BushuMode;
    } else {
      ic->majorMode = CANNA_MODE_ExtendMode;
      ic->minorMode = CANNA_MODE_BushuMode;
    }
  }
  currentModeInfo(d);

  /* ¸õÊä°ìÍ÷¹Ô¤¬¶¹¤¯¤Æ¸õÊä°ìÍ÷¤¬½Ð¤»¤Ê¤¤ */
  if(ic->tooSmall) {
    d->status = AUX_CALLBACK;
    killmenu(d);
    return(retval);
  }

  if ( !(ic->flags & ICHIRAN_ALLOW_CALLBACK) ) {
    makeGlineStatus(d);
  }
  /* d->status = EVERYTIME_CALLBACK; */

  return(retval);
}

/*
 * ¸õÊä¹Ô¤ò¾Ãµî¤·¡¢Éô¼ó¥â¡¼¥É¤«¤éÈ´¤±¡¢ÆÉ¤ß¤¬¤Ê¤¤¥â¡¼¥É¤Ë°Ü¹Ô¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 *		flag	ConvertAsBushu¤«¤é¸Æ¤Ð¤ì¤¿ 0
 *			BushuYomiHenkan¤«¤é¸Æ¤Ð¤ì¤¿ 1
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
static int
makeBushuIchiranQuit(uiContext d, int flag)
{
  extern int defaultBushuContext;

  /* Éô¼óÊÑ´¹¤Ï³Ø½¬¤·¤Ê¤¤¡£ */
  if(RkwEndBun(defaultBushuContext, 0) == -1) { /* 0:³Ø½¬¤·¤Ê¤¤ */
    if(errno == EPIPE)
      jrKanjiPipeError();
    jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\316\275\252\316\273\244\313\274\272\307\324\244\267\244\336\244\267\244\277";
                   /* ¤«¤Ê´Á»úÊÑ´¹¤Î½ªÎ»¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    return(NG);
  }

  if(flag) {
    /* kanji_status_return ¤ò¥¯¥ê¥¢¤¹¤ë */
    d->kanji_status_return->length  = 0;
    d->kanji_status_return->revLen  = 0;
    
/*
    d->status = QUIT_CALLBACK;
*/
  } else {
    makeYomiReturnStruct(d);
  }
  GlineClear(d);
  
  return(0);
}


