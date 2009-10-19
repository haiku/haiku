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
static char rcs_id[] = "@(#) 102.1 $Id: commondata.c 10525 2004-12-23 21:23:50Z korli $";
#endif /* lint */

#include "canna.h"
#include <canna/mfdef.h>
#include "patchlevel.h"
#include <Beep.h>

struct CannaConfig cannaconf;
     
/* ¥Ç¥Õ¥©¥ë¥È¤Î¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹ÍÑ¤Î¥Ð¥Ã¥Õ¥¡ */

int defaultContext = -1;
int defaultBushuContext = -1;

/* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë */
/*
 * ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë¤Ï£±¸Ä¤¢¤ì¤ÐÎÉ¤¤¤Ç¤·¤ç¤¦¡£Ê£¿ô¸ÄÉ¬Í×¤Ê¤Î¤Ç
 * ¤¢¤ì¤Ð RomeStruct ¤Î¥á¥ó¥Ð¤È¤·¤ÆÆþ¤ì¤Æ¤ª¤¯É¬Í×¤â¤¢¤ê¤Þ¤·¤ç¤¦¤¬...¤½
 * ¤Î»þ¤Ï¤½¤Î»þ¤Ç¹Í¤¨¤Þ¤·¤ç¤¦¡£
 */
     
struct RkRxDic *romajidic, *englishdic;

/* Ì¤ÄêµÁ¥­¡¼ÂÇ¸°»þ¤Î½èÍý¤Î¤·¤«¤¿ */

int howToBehaveInCaseOfUndefKey = kc_normal;

/*
 * ¼­½ñ¤ÎÌ¾Á°¤òÆþ¤ì¤Æ¤ª¤¯ÊÑ¿ô
 */

char saveapname[CANNA_MAXAPPNAME]; /* ¥µ¡¼¥Ð¤È¤ÎÀÜÂ³¤òÀÚ¤ë¤È¤­¤ÎAPÌ¾ */

/*
 * irohacheck ¥³¥Þ¥ó¥É¤Ë¤è¤Ã¤Æ»È¤ï¤ì¤Æ¤¤¤ë¤«¤È¤«¡¢
 * irohacheck ¤Ê¤¤¤Ç¤Î verbose ¤òÉ½¤¹ÃÍ¡£
 */

int ckverbose = 0;

/*
 * ¥¨¥é¡¼¤Î¥á¥Ã¥»¡¼¥¸¤òÆþ¤ì¤Æ¤ª¤¯ÊÑ¿ô
 */

char *jrKanjiError = "";

/*
 * ¥Ç¥Ð¥°¥á¥Ã¥»¡¼¥¸¤ò½Ð¤¹¤«¤É¤¦¤«¤Î¥Õ¥é¥°
 */

int iroha_debug = 0;

/*
 * »Ï¤á¤Æ¤Î»ÈÍÑ¤«¤É¤¦¤«¤ò¼¨¤¹¥Õ¥é¥°
 */

int FirstTime = 1;

/*
* dictonary base path
*/

char basepath[256];

/*
 * ¥Ó¡¼¥×²»¤òÌÄ¤é¤¹´Ø¿ô¤ò³ÊÇ¼¤¹¤ë¤È¤³¤í
 */

int (*jrBeepFunc)() = (int (*)())NULL;
//int (*jrBeepFunc)() = (int (*)())beep();

/*
 * KC_INITIALIZE Ä¾¸å¤Ë¼Â¹Ô¤¹¤ëµ¡Ç½¤ÎÎó
 */

BYTE *initfunc = (BYTE *)0;
int howToReturnModeInfo = ModeInfoStyleIsString;
char *RomkanaTable = (char *)NULL;
char *EnglishTable = (char *)NULL;
/* char *Dictionary = (char *)NULL; */
struct dicname *RengoGakushu = (struct dicname *)NULL;
struct dicname *KatakanaGakushu = (struct dicname *)NULL;
struct dicname *HiraganaGakushu = (struct dicname *)NULL;

int nKouhoBunsetsu = 16;

int KeepCursorPosition = 0;

int nothermodes = 0;

keySupplement keysup[MAX_KEY_SUP];
int nkeysup = 0;

/*
 * ½é´ü²½¤ÎºÝ»ÈÍÑ¤·¤¿½é´ü²½¥Õ¥¡¥¤¥ëÌ¾¤òÁ´¤Æ¤È¤Ã¤Æ¤ª¤¯¥Ð¥Ã¥Õ¥¡¡£
 * ¥Õ¥¡¥¤¥ëÌ¾¤Ï","¤Ç¶èÀÚ¤é¤ì¤ë¡£(³ÈÄ¥µ¡Ç½¤Ç»ÈÍÑ)
 */

char *CANNA_initfilename = (char *)NULL;

/*
 * ¥Ð¡¼¥¸¥ç¥ó
 */

int protocol_version = -1;
int server_version = -1;
char *server_name = (char *)NULL;

int chikuji_debug = 0;
int auto_define = 0;

int locale_insufficient = 0;

void (*keyconvCallback)(...) = (void (*)(...))0;

extraFunc *extrafuncp = (extraFunc *)NULL;
struct dicname *kanjidicnames; /* .canna ¤Ç»ØÄê¤·¤Æ¤¤¤ë¼­½ñ¥ê¥¹¥È */
char *kataautodic = (char *)NULL; /* ¥«¥¿¥«¥Ê¸ì¼«Æ°ÅÐÏ¿ÍÑ¼­½ñ */
#ifdef HIRAGANAAUTO
char *hiraautodic = (char *)NULL; /* ¤Ò¤é¤¬¤Ê¸ì¼«Æ°ÅÐÏ¿ÍÑ¼­½ñ */
#endif

static void freeUInfo(void);

/* ¥æ¡¼¥¶¾ðÊó */
jrUserInfoStruct *uinfo = (jrUserInfoStruct *)NULL;

/* ¥¹¥¿¥ó¥É¥¢¥í¥ó¤«¤É¤¦¤«¤Î¥Õ¥é¥° */
int standalone = 0;

void
InitCannaConfig(struct CannaConfig *cf)
{
  bzero(cf, sizeof(struct CannaConfig));
  cf->CannaVersion = CANNA_MAJOR_MINOR;
  cf->kouho_threshold = 2;
  cf->strokelimit = STROKE_LIMIT;
  cf->CursorWrap = 1;
  cf->SelectDirect = 1;
  cf->HexkeySelect = 1;
  cf->ChBasedMove = 1;
  cf->Gakushu = 1;
  cf->grammaticalQuestion = 1;
  cf->stayAfterValidate = 1;
  cf->kCount = 1;
  cf->hexCharacterDefiningStyle = HEX_USUAL;
  cf->ChikujiContinue = 1;
  cf->MojishuContinue = 1;
  cf->kojin = 1;
  cf->indexSeparator = DEFAULTINDEXSEPARATOR;
  cf->allowNextInput = 1;
  cf->chikujiRealBackspace = 1;
  cf->BackspaceBehavesAsQuit = 1;
  cf->doKatakanaGakushu = 1;
  cf->doHiraganaGakushu = 1;
  cf->auto_sync = 1;
}

static void freeUInfo (void);

static void
freeUInfo(void)
{
  if (uinfo) {
    if (uinfo->uname)
      free(uinfo->uname);
    if (uinfo->gname)
      free(uinfo->gname);
    if (uinfo->srvname)
      free(uinfo->srvname);
    if (uinfo->topdir)
      free(uinfo->topdir);
    if (uinfo->cannafile)
      free(uinfo->cannafile);
    if (uinfo->romkanatable)
      free(uinfo->romkanatable);
    free(uinfo);
    uinfo = (jrUserInfoStruct *)NULL;
  }
}

/*
  ¥Ç¥Õ¥¡¡¼¥ë¥ÈÃÍ¤Ë¤â¤É¤¹¡£
*/
void
restoreBindings(void)
{
  InitCannaConfig(&cannaconf);

  if (initfunc) free(initfunc);
  initfunc = (BYTE *)NULL;

  if (server_name) free(server_name);
  server_name = (char *)NULL;

  if (RomkanaTable) {
    free(RomkanaTable);
    RomkanaTable = (char *)NULL;
  }
  if (EnglishTable) {
    free(EnglishTable);
    EnglishTable = (char *)NULL;
  }
  romajidic = (struct RkRxDic *)NULL;
  englishdic = (struct RkRxDic *)NULL;
  RengoGakushu = (struct dicname *)NULL;
  KatakanaGakushu = (struct dicname *)NULL;
  HiraganaGakushu = (struct dicname *)NULL;
  howToBehaveInCaseOfUndefKey = kc_normal;
/*  kanjidicname[nkanjidics = 0] = (char *)NULL; Âå¤ï¤ê¤Î¤³¤È¤ò¤·¤Ê¤±¤ì¤Ð */
  kanjidicnames = (struct dicname *)NULL;
  kataautodic = (char *)NULL;
#ifdef HIRAGANAAUTO
  hiraautodic = (char *)NULL;
#endif
  auto_define = 0;
  saveapname[0] = '\0';
  KeepCursorPosition = 0;

  nothermodes = 0;
  protocol_version = server_version = -1;
  nKouhoBunsetsu = 16;
  nkeysup = 0;
  chikuji_debug = 0;
  keyconvCallback = (void (*)(...))0;
  locale_insufficient = 0;
  freeUInfo();
  standalone = 0;
}
