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
static char rcs_id[] = "@(#) 102.1 $Id: keydef.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif /* lint */

#include "canna.h"
#include <canna/mfdef.h>
#include <canna/keydef.h>

extern KanjiModeRec alpha_mode, empty_mode, yomi_mode;
extern KanjiModeRec jishu_mode, ce_mode, cy_mode, cb_mode;
extern KanjiModeRec tankouho_mode, ichiran_mode, onoff_mode;
extern KanjiModeRec khal_mode, khkt_mode, kzal_mode, kzhr_mode, kzkt_mode;
extern KanjiModeRec kigo_mode;
extern KanjiModeRec tourokureibun_mode;
extern KanjiModeRec bunsetsu_mode;
extern KanjiModeRec cy_mode, cb_mode;

//extern multiSequenceFunc
//  (struct _uiContext *, struct _kanjiMode *, int, int, int);

#define NONE 0
#define ACTHASHTABLESIZE 64
#define KEYHASHTABLESIZE 16

#define SINGLE 0
#define MULTI  1
#define OTHER  2

static unsigned char *duplicatekmap(unsigned char *kmap);
static int changeKeyOnSomeCondition(KanjiMode mode, int key, int fnum, unsigned char *actbuff, unsigned char *keybuff);
static void undefineKeyfunc(unsigned char *keytbl, unsigned fnum);
//static unsigned int createHashKey(unsigned char *data1, unsigned char data2, int which_seq);
static void regist_act_hash(unsigned char *tbl_ptr, unsigned char key, unsigned char *buff);
static void remove_hash(unsigned char *tbl_ptr, unsigned char key, int which_seq);
static void freeChain(struct seq_struct *p);
static void clearAllFuncSequence(void);
static void freeKeySeqMode(KanjiMode m);
static void freeMap(struct map *m);
static void clearAllKeySequence(void);
static int specialen(unsigned char *block);
static int to_write_act(int depth, int keysize, int actsize, unsigned singleAct);
static struct map *regist_map(KanjiMode tbl, unsigned char *keybuff, unsigned char *actbuff, int depth);
static int regist_key_hash(unsigned char *tbl_ptr, unsigned char *keybuff, unsigned char *actbuff);
static int copyMultiSequence(unsigned char key, KanjiMode old_tbl, KanjiMode new_tbl);
static void freeMultiSequence(unsigned char key, KanjiMode tbl);

struct  seq_struct{
  unsigned char    *to_tbl;
  unsigned char    as_key;
  unsigned char    *kinou_seq;
  struct  seq_struct *next;
};

static struct seq_struct *seq_hash[ACTHASHTABLESIZE];

struct map{
  KanjiMode tbl;
  unsigned char key;
  KanjiMode  mode;
  struct map *next;
};

static struct map *otherMap[KEYHASHTABLESIZE];

static KanjiMode ModeTbl[CANNA_MODE_MAX_REAL_MODE] = {
  &alpha_mode,        /* AlphaMode          ¥¢¥ë¥Õ¥¡¥Ù¥Ã¥È¥â¡¼¥É         */
  &empty_mode,	      /* EmptyMode          ÆÉ¤ßÆþÎÏ¤¬¤Ê¤¤¾õÂÖ           */
  &kigo_mode,	      /* KigoMode           ¸õÊä°ìÍ÷¤òÉ½¼¨¤·¤Æ¤¤¤ë¾õÂÖ   */
  &yomi_mode,         /* YomiMode           ÆÉ¤ßÆþÎÏ¤·¤Æ¤¤¤ë¾õÂÖ         */
  &jishu_mode,	      /* JishuMode          Ê¸»ú¼ïÊÑ´¹¤·¤Æ¤¤¤ë¾õÂÖ       */
  &tankouho_mode,     /* TankouhoMode       Ã±°ì¤Î¸õÊä¤òÉ½¼¨¤·¤Æ¤¤¤ë¾õÂÖ */
  &ichiran_mode,      /* IchiranMode        ¸õÊä°ìÍ÷¤òÉ½¼¨¤·¤Æ¤¤¤ë¾õÂÖ   */
  &tourokureibun_mode, /* TourokuReibunMode Ã±¸ìÅÐÏ¿¤ÎÎãÊ¸É½¼¨¾õÂÖ       */
  &onoff_mode,        /* OnOffMode          On/Off¤Î°ìÍ÷¤ÎÉ½¼¨¾õÂÖ       */
  &bunsetsu_mode,     /* AdjustBunsetsuMode Ê¸Àá¿­½Ì¥â¡¼¥É               */
  &cy_mode,	      /* ChikujiYomiMode    Ãà¼¡¤Î»þ¤ÎÆÉ¤ßÉôÊ¬           */
  &cb_mode,           /* ChikujiHenkanMode  Ãà¼¡¤Î»þ¤ÎÊÑ´¹¤ÎÉôÊ¬         */
};


static unsigned char *
duplicatekmap(unsigned char *kmap)
{
  unsigned char *res;
  int i;

  res = (unsigned char *)calloc(256, sizeof(unsigned char));
  if (res) {
    for (i = 0 ; i < 256 ; i++) {
      res[i] = kmap[i];
    }
  }
  return res;
}

static unsigned char *defaultkeytables[CANNA_MODE_MAX_REAL_MODE];
static unsigned char defaultsharing[CANNA_MODE_MAX_REAL_MODE];
static unsigned char *defaultmap;
unsigned char *alphamap, *emptymap;

/* cfuncdef

  initKeyTables() -- ¥­¡¼¥Æ¡¼¥Ö¥ë¤ò½é´ü²½¤¹¤ë´Ø¿ô¡£

  ¥Ç¥Õ¥©¥ë¥È¤Î¥­¡¼¥Æ¡¼¥Ö¥ë¤òµ­Ï¿¤·¤Æ¤ª¤­¡¢¼Â»ÈÍÑ¤Î¥Æ¡¼¥Ö¥ë¤ò ¥Ç¥Õ¥©¥ë
  ¥È¥Æ¡¼¥Ö¥ë¤«¤é¥³¥Ô¡¼¤¹¤ë½èÍý¤ò¹Ô¤¦¡£

*/

int initKeyTables(void)
{
  int i;
  unsigned char *tbl;
  extern unsigned char default_kmap[], alpha_kmap[], empty_kmap[];

  defaultmap = duplicatekmap(default_kmap);
  if (defaultmap) {
    alphamap   = duplicatekmap(alpha_kmap);
    if (alphamap) {
      emptymap   = duplicatekmap(empty_kmap);
      if (emptymap) {
        for (i = 0 ; i < CANNA_MODE_MAX_REAL_MODE ; i++) {
          if (ModeTbl[i]) {
            defaultsharing[i] = ModeTbl[i]->flags;
            tbl = defaultkeytables[i] = ModeTbl[i]->keytbl;
            if (tbl == default_kmap) {
              ModeTbl[i]->keytbl = defaultmap;
            }
            else if (tbl == alpha_kmap) {
              ModeTbl[i]->keytbl = alphamap;
            }
            else if (tbl == empty_kmap) {
              ModeTbl[i]->keytbl = emptymap;
            }
          }
        }
        return 0;
      }
      free(alphamap);
    }
    free(defaultmap);
  }
  return NG;
}

void
restoreDefaultKeymaps(void)
{
  int i;

  for (i = 0 ; i < CANNA_MODE_MAX_REAL_MODE ; i++) {
    if (ModeTbl[i]) {
      if ( !(ModeTbl[i]->flags & CANNA_KANJIMODE_TABLE_SHARED) ) {
	free(ModeTbl[i]->keytbl);
      }
      ModeTbl[i]->keytbl = defaultkeytables[i];
      ModeTbl[i]->flags = defaultsharing[i];
    }
  }
  free(defaultmap);
  free(alphamap);
  free(emptymap);
  clearAllFuncSequence();
  clearAllKeySequence();
}


/* 
 * ¤¢¤ë¥â¡¼¥É¤Î¥­¡¼¤ËÂÐ¤·¤Æ´Ø¿ô¤ò³ä¤êÅö¤Æ¤ë½èÍý
 *
 */

/* 

  £±£¶¿Ê¤Î»þ¤Ï£´Ê¸»úÌÜ¤òÆþ¤ì¤¿»þ¤Î¥â¡¼¥É¤Ë¤âÀßÄê¤¹¤ë¡£

 */

extern int nothermodes;

int changeKeyfunc(int modenum, int key, int fnum, unsigned char *actbuff, unsigned char *keybuff)
{
  int i, retval = 0;
  unsigned char *p, *q;
  KanjiMode mode;
  newmode *nmode;

  /* ¤Á¤ç¤Ã¤È¾®ºÙ¹© */
  if (modenum == CANNA_MODE_HenkanNyuryokuMode) {
    retval = changeKeyfunc(CANNA_MODE_EmptyMode, key, fnum, actbuff, keybuff);
    if (retval < 0) {
      return retval;
    }
    modenum = CANNA_MODE_YomiMode;
  }

  if (modenum < 0) {
    return 0;
  }
  else if (modenum < CANNA_MODE_MAX_REAL_MODE) {
    mode = ModeTbl[modenum];
  }
  else if (modenum < CANNA_MODE_MAX_IMAGINARY_MODE) {
    return 0;
  }
  else if (modenum < CANNA_MODE_MAX_IMAGINARY_MODE + nothermodes) {
    nmode = findExtraKanjiMode(modenum);
    if (!nmode) {
      return 0;
    }
    else {
      mode = nmode->emode;
    }
  }
  else {
    return 0;
  }

  if (mode && mode->func((uiContext)0/*dummy*/, mode,
                           KEY_CHECK, 0/*dummy*/, fnum)) { 
    /* ¤½¤Îµ¡Ç½¤¬¤½¤Î¥â¡¼¥É¤Ë¤ª¤¤¤ÆÍ­¸ú¤Êµ¡Ç½¤Ç¤¢¤ì¤Ð */
    if (mode->keytbl) { /* ¥­¡¼¥Æ¡¼¥Ö¥ë¤¬Â¸ºß¤¹¤ì¤Ð */
      /* ¤³¤ì¤ÏÀäÂÐ¤ËÂ¸ºß¤¹¤ë¤Î¤Ç¤Ï¡© */
      if (mode->flags & CANNA_KANJIMODE_TABLE_SHARED) {
	/* ¥­¡¼¥Þ¥Ã¥×¤¬Â¾¤Î¥â¡¼¥É¤È¶¦Í­¤µ¤ì¤Æ¤¤¤ë¤Ê¤é */
	p = (unsigned char *)calloc(256, sizeof(unsigned char));
        if (!p) {
          return -1;
        }
        bcopy(mode->keytbl, p, 256 * sizeof(unsigned char));
        for (i = 0; i < 256; i++) {
          if (mode->keytbl[i] == CANNA_FN_FuncSequence) {
            q = actFromHash(mode->keytbl,i);
            if (q) { /* ³ºÅö¤¹¤ë¥­¡¼¥·¡¼¥±¥ó¥¹¤¬¤¢¤Ã¤¿¤é */
              regist_act_hash(p, i, q);
            }
          }
          if (mode->keytbl[i] == CANNA_FN_UseOtherKeymap) {
	    debug_message("changeKeyfunc:\245\306\241\274\245\326\245\353"
		"\260\334\306\260\72\244\263\244\316\244\310\244\255\244\316"
		"\245\255\241\274\244\317%d\n",i,0,0);
                          /* ¥Æ¡¼¥Ö¥ë°ÜÆ°:¤³¤Î¤È¤­¤Î¥­¡¼¤Ï */
            (void)copyMultiSequence(i, (KanjiMode)mode->keytbl,
                                       (KanjiMode)p);
          }
        }
        mode->keytbl = p;
        mode->flags &= ~CANNA_KANJIMODE_TABLE_SHARED;
        if (modenum == CANNA_MODE_YomiMode &&
             (ModeTbl[CANNA_MODE_ChikujiYomiMode]->flags
              & CANNA_KANJIMODE_TABLE_SHARED)) {
          ModeTbl[CANNA_MODE_ChikujiYomiMode]->keytbl = p;
        }
        else if (modenum == CANNA_MODE_TankouhoMode &&
	          (ModeTbl[CANNA_MODE_ChikujiTanMode]->flags
                   & CANNA_KANJIMODE_TABLE_SHARED)) {
	  ModeTbl[CANNA_MODE_ChikujiTanMode]->keytbl = p;
        }
      }
      if (key >= 0 && key < 255) {
        if (mode->keytbl[key] == CANNA_FN_UseOtherKeymap &&
             fnum != CANNA_FN_UseOtherKeymap)
          freeMultiSequence(key,(KanjiMode)mode->keytbl);
        mode->keytbl[key] = fnum;
        if (fnum == CANNA_FN_FuncSequence) {
          regist_act_hash(mode->keytbl,key,actbuff);
        }
        if (fnum == CANNA_FN_UseOtherKeymap) {
          retval = regist_key_hash(mode->keytbl,keybuff,actbuff);
          if (retval) {
            return retval;
          }
        }
      }
      else if (key == CANNA_KEY_Undefine) {
        undefineKeyfunc(mode->keytbl, fnum);
      }
    }
  }
  return 0;
}

static int
changeKeyOnSomeCondition(KanjiMode mode, int key, int fnum, unsigned char *actbuff, unsigned char *keybuff)
{
  int retval = 0;

  if (mode && /* ¤½¤Î¥â¡¼¥É¤¬Â¸ºß¤¹¤ë¤Ê¤é */
      mode->func((uiContext)0/*dummy*/, mode,
                   KEY_CHECK, 0/*dummy*/, fnum)) {
    /* ´Ø¿ô¤¬¤½¤Î¥â¡¼¥É¤ÇÍ­¸ú¤Ê¤é */
    if ( !(mode->flags & CANNA_KANJIMODE_TABLE_SHARED) ) {
      /* ¥Æ¡¼¥Ö¥ë¤¬¶¦Í­¤µ¤ì¤Æ¤¤¤Ê¤¤¤Ê¤é */
      if (mode->keytbl) { /* ¥­¡¼¥Æ¡¼¥Ö¥ë¤¬Â¸ºß¤¹¤ì¤Ð */
	if (mode->keytbl[key] == CANNA_FN_UseOtherKeymap &&
	    fnum != CANNA_FN_UseOtherKeymap)
	  freeMultiSequence(key,(KanjiMode)mode->keytbl);
	mode->keytbl[key] = fnum;
	if (fnum == CANNA_FN_FuncSequence) {
	  regist_act_hash(mode->keytbl,key,actbuff);
	}
	if (fnum == CANNA_FN_UseOtherKeymap) {
	  retval = regist_key_hash(mode->keytbl,keybuff,actbuff);
	}
      }
    }
  }
  return retval;
}

/*
 * Á´¤Æ¤Î¥â¡¼¥É¤Î¡¢¤¢¤ë¥­¡¼¤ËÂÐ¤·¤Æ´Ø¿ô¤ò³ä¤êÅö¤Æ¤ë½èÍý
 *
 */

int changeKeyfuncOfAll(int key, int fnum, unsigned char *actbuff, unsigned char *keybuff)
{
  extern extraFunc *extrafuncp;
  extraFunc *ep;
  KanjiMode mode;
  int i, retval = 0;

  if (key >= 0 && key < 255) {
    if (defaultmap[key] == CANNA_FN_UseOtherKeymap &&
	      fnum != CANNA_FN_UseOtherKeymap)
      freeMultiSequence(key,(KanjiMode)defaultmap);
    if (alphamap[key] == CANNA_FN_UseOtherKeymap &&
	      fnum != CANNA_FN_UseOtherKeymap)
      freeMultiSequence(key,(KanjiMode)alphamap);
    if (emptymap[key] == CANNA_FN_UseOtherKeymap &&
	      fnum != CANNA_FN_UseOtherKeymap)
      freeMultiSequence(key,(KanjiMode)emptymap);
    defaultmap[key] = fnum;
    alphamap[key]   = fnum;
    emptymap[key]   = fnum;
    if (fnum == CANNA_FN_FuncSequence) {
      regist_act_hash(defaultmap,key,actbuff);
      regist_act_hash(alphamap,key,actbuff);
      regist_act_hash(emptymap,key,actbuff);
    }
    if (fnum == CANNA_FN_UseOtherKeymap) {
      if (regist_key_hash(defaultmap,keybuff,actbuff) == NG ||
            regist_key_hash(alphamap,keybuff,actbuff) == NG ||
            regist_key_hash(emptymap,keybuff,actbuff) == NG) {
        return -1;
      }
    }
    for (i = 0 ; i < CANNA_MODE_MAX_REAL_MODE ; i++) {
      mode = ModeTbl[i];
      retval = changeKeyOnSomeCondition(mode, key, fnum, actbuff, keybuff);
      if (retval < 0) {
        return retval;
      }
    }
    for (ep = extrafuncp ; ep ; ep = ep->next) {
      /* defmode ¤Ç¤ÎÁ´¤Æ¤Î¥â¡¼¥É¤ËÂÐ¤·¤Æ¤ä¤ë */
      if (ep->keyword == EXTRA_FUNC_DEFMODE) {
	retval = changeKeyOnSomeCondition(ep->u.modeptr->emode, key, fnum,
                                            actbuff, keybuff);
        if (retval < 0) {
          return retval;
        }
      }
    }
  }
  else if (key == CANNA_KEY_Undefine) {
    undefineKeyfunc(defaultmap, (unsigned)fnum);
    undefineKeyfunc(alphamap, (unsigned)fnum);
    undefineKeyfunc(emptymap, (unsigned)fnum);
    for (i = 0 ; i < CANNA_MODE_MAX_REAL_MODE ; i++) {
      mode = ModeTbl[i];
      if (mode && /* ¤½¤Î¥â¡¼¥É¤¬Â¸ºß¤¹¤ë¤Ê¤é */
	  mode->func((uiContext)0/*dummy*/, mode,
                       KEY_CHECK, 0/*dummy*/, fnum)) {
	/* ´Ø¿ô¤¬¤½¤Î¥â¡¼¥É¤ÇÍ­¸ú¤Ê¤é */
	if ( !(mode->flags & CANNA_KANJIMODE_TABLE_SHARED) ) {
	  /* ¥Æ¡¼¥Ö¥ë¤¬¶¦Í­¤µ¤ì¤Æ¤¤¤Ê¤¤¤Ê¤é */
	  if (mode->keytbl) { /* ¥­¡¼¥Æ¡¼¥Ö¥ë¤¬Â¸ºß¤¹¤ì¤Ð */
	    undefineKeyfunc(mode->keytbl, (unsigned)fnum);
	  }
	}
      }
    }
  }
  return retval;
}

static void
undefineKeyfunc(unsigned char *keytbl, unsigned fnum)
{
  int i;

  for (i = 0 ; i < ' ' ; i++) {
    if (keytbl[i] == fnum) {
      keytbl[i] = CANNA_FN_Undefined;
    }
  }
  for (i = ' ' ; i < 0x7f ; i++) {
    if (keytbl[i] == fnum) {
      keytbl[i] = CANNA_FN_FunctionalInsert;
    }
  }
  for (i = 0x7f ; i < 0xa0 ; i++) {
    if (keytbl[i] == fnum) {
      keytbl[i] = CANNA_FN_Undefined;
    }
  }
  for (i = 0xa0 ; i < 0xe0 ; i++) {
    if (keytbl[i] == fnum) {
      keytbl[i] = CANNA_FN_FunctionalInsert;
    }
  }
  for (i = 0xe0 ; i < 0x100 ; i++) {
    if (keytbl[i] == fnum) {
      keytbl[i] = CANNA_FN_Undefined;
    }
  }
}

inline
unsigned int
createHashKey(unsigned char *data1, unsigned char data2, unsigned long which_seq)
{
  unsigned long hashKey;

  hashKey = (*data1 + data2) % which_seq;
  return hashKey;
}

/* µ¡Ç½¥·¡¼¥±¥ó¥¹¤ò³ä¤ê½Ð¤¹ */
unsigned char *
actFromHash(unsigned char *tbl_ptr, unsigned char key)
{
  unsigned int hashKey;
  struct seq_struct *p;

  hashKey = createHashKey(tbl_ptr, key, ACTHASHTABLESIZE);
  for (p = seq_hash[hashKey] ; p ; p = p->next) {
    if (p->to_tbl == tbl_ptr && p->as_key == key) {
      return p->kinou_seq;
    }
  }
#ifndef WIN
  debug_message("actFromHash:¥­¡¼¥·¥±¥ó¥¹¤ò¤ß¤Ä¤±¤é¤ì¤Þ¤»¤ó¤Ç¤·¤¿¡£\n",0,0,0);
#else
  debug_message("actFromHash:\245\255\241\274\245\267\245\261\245\363\245\271"
	"\244\362\244\337\244\304\244\261\244\351\244\354\244\336\244\273"
	"\244\363\244\307\244\267\244\277\241\243\n",0,0,0);
#endif
  return (unsigned char *)NULL; /* ³ºÅö¤¹¤ë¥­¡¼¥·¡¼¥±¥ó¥¹¤ÏÂ¸ºß¤·¤Ê¤¤ */
}

/* ¥Ï¥Ã¥·¥å¥Æ¡¼¥Ö¥ë¤ËÅÐÏ¿ */
static void
regist_act_hash(unsigned char *tbl_ptr, unsigned char key, unsigned char *buff)
{
  unsigned int hashKey;
  struct seq_struct *p, **pp;

  hashKey = createHashKey(tbl_ptr, key, ACTHASHTABLESIZE);
  for (pp = &seq_hash[hashKey] ; (p = *pp) != (struct seq_struct *)0 ;
       pp = &(p->next)) {
    if (p->to_tbl == tbl_ptr && p->as_key == key) {
      if (p->kinou_seq)
	free(p->kinou_seq);
      p->kinou_seq = (unsigned char *)malloc(strlen((char *)buff)+1);
      if (p->kinou_seq)
	strcpy((char *)p->kinou_seq,(char *)buff);
      return;
    }
  }
  p = *pp = (struct seq_struct *)malloc(sizeof(struct seq_struct));
  if(p) {
    p->to_tbl = tbl_ptr;
    p->as_key = key;
    p->kinou_seq = (unsigned char *)malloc(strlen((char *)buff)+1);
    if(p->kinou_seq)
      strcpy((char *)p->kinou_seq,(char *)buff);
    p->next = (struct seq_struct *)NULL;
  }
}  

/* ¥Ï¥Ã¥·¥å¥Æ¡¼¥Ö¥ë¤«¤éºï½ü */
static void
remove_hash(unsigned char *tbl_ptr, unsigned char key, int which_seq)
{
  unsigned int hashKey;
  struct seq_struct *p, **pp;

  hashKey = createHashKey(tbl_ptr, key, which_seq);
  for (pp = &seq_hash[hashKey] ; (p = *pp) != (struct seq_struct *)0 ;
       pp = &(p->next)) {
    if (p->to_tbl == tbl_ptr && p->as_key == key) {
      *pp = p->next;
      free(p);
    }
  }
}

static void
freeChain(struct seq_struct *p)
{
  struct seq_struct *nextp;

  while (p) {
    free(p->kinou_seq);
    nextp = p->next;
    free(p);
    p = nextp;
  }
}

static void
clearAllFuncSequence(void)
{
  int i;

  for (i = 0 ; i < ACTHASHTABLESIZE ; i++) {
    freeChain(seq_hash[i]);
    seq_hash[i] = 0;
  }
}

static void
freeKeySeqMode(KanjiMode m)
{
  if (m) {
    if (m->keytbl) {
      free(m->keytbl);
    }
    free(m);
  }
}

static void
freeMap(struct map *m)
{
  struct map *n;

  while (m) {
    freeKeySeqMode(m->mode);
    n = m->next;
    free(m);
    m = n;
  }
}

static void
clearAllKeySequence(void)
{
  int i;

  for (i = 0 ; i < KEYHASHTABLESIZE ; i++) {
    freeMap(otherMap[i]);
    otherMap[i] = 0;
  }
}

static
int specialen(unsigned char *block)
{
  int i;
  for (i = 0 ; block[i] != 255 ;) {
    i++;
  }
  debug_message("specialen:\304\271\244\265\244\317%d\244\311\244\271\241\243\n",i,0,0);
                /* specialen:Ä¹¤µ¤Ï%d¤É¤¹¡£ */
  return i;
}

static
int to_write_act(int depth, int keysize, int actsize, unsigned singleAct)
{
  if (depth == (keysize -2)) {
    if (actsize > 1){
      debug_message("to_write_act:CANNA_FN_FuncSequence\244\307\244\271\241\243\n",0,0,0);
                                                     /* ¤Ç¤¹¡£ */
      return CANNA_FN_FuncSequence;
    }
    if (actsize == 1) {
      debug_message("to_write_act:singleAct%d\244\307\244\271\241\243\n",singleAct,0,0);
                                              /* ¤Ç¤¹¡£ */
      return (int)singleAct;
    }
    else { /* Í­¤êÆÀ¤Ê¤¤¡© */
      return 0;
    }
  } else if (depth < (keysize -2)){
    debug_message("to_write_act:CANNA_FN_UseOtherKeymap\244\307\244\271\241\243\n",0,0,0);
                                              /* ¤Ç¤¹¡£ */
    return CANNA_FN_UseOtherKeymap;
  }
  else { /* Í­¤êÆÀ¤Ê¤¤¡© */
    return 0;
  }
}

static struct map *
regist_map(KanjiMode tbl, unsigned char *keybuff, unsigned char *actbuff, int depth)
{
  unsigned int hashKey;
  int sequencelen, keybuffsize, actbuffsize, offs;
  struct map *p,**pp;
  unsigned char *q, prevfunc;

  actbuffsize = strlen((char *)actbuff);
  keybuffsize = specialen(keybuff);
  hashKey = 
    createHashKey((unsigned char *)tbl, keybuff[depth], KEYHASHTABLESIZE);
  debug_message("regist_map:hashKey = %d \244\307\244\271\241\243\n",hashKey,0,0);
                                         /* ¤Ç¤¹¡£ */
  for (pp = &otherMap[hashKey]; (p = *pp) != (struct map *)0 ;
       pp = &(p->next)) {
    if (p->key == keybuff[depth] && p->tbl == tbl) { 
      for (q = p->mode->keytbl; *q != 255; q += 2) {
	if (*q == keybuff[depth+1]) {  /* ´û¤ËÆ±¤¸¥­¡¼¤¬Â¸ºß¤·¤¿¡£ */
	  ++q;
	  prevfunc = *q; /* ¤½¤Î¥­¡¼¤Îº£¤Þ¤Ç¤Îµ¡Ç½¤ò¼è¤Ã¤Æ¤ª¤¯ */
	  *q = to_write_act(depth,keybuffsize,actbuffsize,actbuff[0]);
	  if(prevfunc == CANNA_FN_UseOtherKeymap &&
	     *q != CANNA_FN_UseOtherKeymap) {
            freeMultiSequence(keybuff[depth + 1], p->mode);
          }
	  if (*q == CANNA_FN_FuncSequence) {
	    regist_act_hash((unsigned char *)p->mode, keybuff[depth+1],
			    actbuff);
	  }
	  debug_message("regist_map:\264\373\244\313\306\261\244\270\245\255\241\274\244\254\302\270\272\337:q=%d\n",*q,0,0);
                        /* ´û¤ËÆ±¤¸¥­¡¼¤¬Â¸ºß */
	  return p;
	}
      }
      /* ¤½¤³¤Þ¤Ç¤Î¡¢¥­¡¼¤ÎÍúÎò¤Ï¤¢¤Ã¤¿¤¬¤³¤Î¥­¡¼:keybuff[depth +1]¤Ï½é¤á¤Æ */
      sequencelen = specialen(p->mode->keytbl);
      offs = q - p->mode->keytbl;
      if (p->mode->keytbl) {
	p->mode->keytbl =
	  (unsigned char *)realloc(p->mode->keytbl,sequencelen +3);
        if (!p->mode->keytbl) {
          return (struct map *)0;
        }
        p->mode->keytbl[sequencelen] = keybuff[depth +1];
        p->mode->keytbl[++sequencelen] =
        to_write_act(depth,keybuffsize,actbuffsize,actbuff[0]);
        p->mode->keytbl[++sequencelen] = (BYTE)-1;
      }
      if (p->mode->keytbl[offs] == CANNA_FN_FuncSequence) {
	regist_act_hash((unsigned char *)p->mode, keybuff[depth+1], actbuff);
      }
      debug_message("regist_map:\244\275\244\263\244\336\244\307\244\316"
	"\241\242\245\255\241\274\244\316\315\372\316\362\244\317\244\242"
	"\244\303\244\277\244\254\244\263\244\316\245\255\241\274%u\244\317"
	"\275\351\244\341\244\306\n",
		    p->mode->keytbl[sequencelen-3],0,0);
                /* ¤½¤³¤Þ¤Ç¤Î¡¢¥­¡¼¤ÎÍúÎò¤Ï¤¢¤Ã¤¿¤¬¤³¤Î¥­¡¼%u¤Ï½é¤á¤Æ */
      debug_message("regist_map:sequencelen¤Ï%d¤Ç¤¹¡£\n",sequencelen,0,0);
      return p;
    }
  }
  /* ²áµî¤ÎÍúÎò¤ÏÁ´¤Æ¤Ê¤·¤Î¤Ï¤º¡¢¿·µ¬¤ËºîÀ® */
  p = *pp = (struct map *)malloc(sizeof(struct map));
  if (p) {
    p->tbl = tbl;
    p->key = keybuff[depth];
    p->mode = (KanjiMode)malloc(sizeof(KanjiModeRec));
    if (p->mode) {
      p->mode->func = multiSequenceFunc;
      p->mode->flags = 0;
      p->mode->keytbl = (unsigned char *)malloc(3);
      if (p->mode->keytbl) {
	p->mode->keytbl[0] = keybuff[depth +1];
	p->mode->keytbl[1] = to_write_act(depth,keybuffsize,actbuffsize,actbuff[0]);
	debug_message("regist_map:p->mode->keytbl[1]\244\317%d\244\307\244\271\241\243\n",p->mode->keytbl[1],0,0);
                                                  /* ¤Ï%d¤Ç¤¹¡£ */
	p->mode->keytbl[2] = (BYTE)-1;

        p->next = (struct map *)NULL;
        if (p->mode->keytbl[1] == CANNA_FN_FuncSequence) {
          regist_act_hash((unsigned char *)p->mode, keybuff[depth+1], actbuff);
        }
        return p;
      }
      free(p->mode);
    }
    free(p);
  }
  return (struct map *)0;
}

struct map *
mapFromHash(KanjiMode tbl, unsigned char key, struct map ***ppp)
{
  unsigned int hashKey;
  struct map *p, **pp;

  hashKey = createHashKey((unsigned char *)tbl, key, KEYHASHTABLESIZE);
  debug_message("mapFromHash:hashKey¤Ï%d\n",hashKey,0,0);
  for(pp = otherMap + hashKey ; (p = *pp) != (struct map *)0 ;
      pp = &(p->next)) {
    if (p->tbl == tbl && p->key == key) {
      debug_message("mapFromHash:map\244\254\244\337\244\304\244\253\244\352"
	"\244\336\244\267\244\277\241\243\n",0,0,0);
                            /* ¤¬¤ß¤Ä¤«¤ê¤Þ¤·¤¿¡£ */
      if (ppp) {
	*ppp = pp;
      }
      return p;
    }
  }
#ifndef WIN
  debug_message("mapFromHash:map¤¬¤ß¤Ä¤«¤ê¤Þ¤»¤ó¡£\n",0,0,0);
#else
  debug_message("mapFromHash:map\244\254\244\337\244\304\244\253\244\352"
	"\244\336\244\273\244\363\241\243\n",0,0,0);
#endif
  return (struct map *)NULL;
}

static int
regist_key_hash(unsigned char *tbl_ptr, unsigned char *keybuff, unsigned char *actbuff)
{
  struct map *map_ptr;
  int keybuffsize, i;

  keybuffsize = specialen(keybuff);
  map_ptr = regist_map((KanjiMode)tbl_ptr, keybuff, actbuff, 0);  
  if (!map_ptr) {
    return NG;
  }
  for (i = 1; i <= (keybuffsize -2); i++) {
    map_ptr = regist_map(map_ptr->mode, keybuff, actbuff, i);
    if (!map_ptr) {
      return NG;
    }
  }
  debug_message("regist_key_hash:keybuffsize\244\317%d¡¡actbuffsize"
	"\244\317¤Ï%d¡¡i\244\317%d\244\307\244\271\241\243\n",
		keybuffsize,strlen(actbuff),i);
                                     /* ¤Ï */ /* ¤Ï */ /* ¤Ï */ /* ¤Ç¤¹¡£ */
  return 0;
}

static
int
copyMultiSequence(unsigned char key, KanjiMode old_tbl, KanjiMode new_tbl)
{
  unsigned char hashKey;
  unsigned char *old_sequence, *new_sequence;
  int i, sequencelen;
  struct map *p, **pp;
  struct map *old_map;

  old_map = mapFromHash(old_tbl, key, (struct map ***)0);
  old_sequence = old_map->mode->keytbl;
  sequencelen = specialen(old_sequence);
  hashKey = createHashKey((unsigned char *)new_tbl, key, KEYHASHTABLESIZE);
  for (pp = &otherMap[hashKey]; (p = *pp) != (struct map *)0 ;
       pp = &(p->next)) {
    if (p->key == key && p->tbl == new_tbl) { 
      return 0;
    }
  }
  p = *pp = (struct map *)malloc(sizeof(struct map));
  if (p) {
    p->tbl = new_tbl;
    p->key = key;
    p->mode = (KanjiMode)malloc(sizeof(KanjiModeRec));
    if (p->mode) {
      p->mode->func = multiSequenceFunc;
      p->mode->flags = 0;
      p->next = (struct map *)NULL;
      p->mode->keytbl = (unsigned char *)malloc(sequencelen+1);
      if (p->mode->keytbl) {
	for (i = 0, new_sequence = p->mode->keytbl; i <= sequencelen; i++) {
	  new_sequence[i] = old_sequence[i];
	  if (i%2 == 1) {
	    if (old_sequence[i] == CANNA_FN_UseOtherKeymap) {
	      if (copyMultiSequence(old_sequence[i-1],
				    old_map->mode, p->mode) < 0) {
		free(p->mode->keytbl);
		free(p->mode);
		free(p);
		*pp = (struct map *)0;
		return(-1);
	      }		
	    } else if (old_sequence[i] == CANNA_FN_FuncSequence)
	      regist_act_hash((unsigned char *)p->mode, old_sequence[i-1],
			      actFromHash((unsigned char *)old_map->mode,
					  old_sequence[i-1]));
	  }
	}
	return 0;
      } else {
	free(p->mode);
	free(p);
	*pp = (struct map *)0;
	return(-1);
      }
    } else {
      free(p);
      *pp = (struct map *)0;
      return(-1);
    }
  } else
    return(-1);
}

static void
freeMultiSequence(unsigned char key, KanjiMode tbl)
{
  unsigned char *sequence;
  int i, sequencelen;
  struct map *map, **ptr;

  map = mapFromHash(tbl, key, &ptr);
  if (!map)
    return;
  *ptr = map->next;
  sequence = map->mode->keytbl;
  sequencelen = specialen(sequence);

  for (i = 0; i <= sequencelen; i++) {
    if (i%2 == 1) {
      if (sequence[i] == CANNA_FN_UseOtherKeymap)
	freeMultiSequence(sequence[i-1], map->mode);
      if (sequence[i] == CANNA_FN_FuncSequence)
	remove_hash((unsigned char *)map->mode, sequence[i-1],
		    ACTHASHTABLESIZE);
    }
  }
  debug_message("\241\374\153\145\171\133\45\144\135\244\316\155\141\160\260"
	"\312\262\274\244\362\245\325\245\352\241\274\244\267\244\306\244\244"
	"\244\353\244\276\n",key,0,0);
    /* ¡ükey[%d]¤Îmap°Ê²¼¤ò¥Õ¥ê¡¼¤·¤Æ¤¤¤ë¤¾ */
  if (map->mode && sequence)
    free(sequence);
  if (map->mode)
    free(map->mode);
  free(map);
}

int askQuitKey(unsigned key)
{
  if (defaultmap[key] == CANNA_FN_Quit) {
    return 1; /* ¼õ¤±¼è¤Ã¤¿key¤Ïquit¤À¤Ã¤¿¡£ */
  }
  return 0; /* ¼õ¤±¼è¤Ã¤¿key¤Ïquit¤Ç¤Ê¤«¤Ã¤¿¡£ */
}
