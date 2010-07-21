/* Copyright 1993 NEC Corporation, Tokyo, Japan.
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

/* $Id: RK.h 10527 2004-12-23 22:08:39Z korli $ */

/************************************************************************/
/* THIS SOURCE CODE IS MODIFIED FOR TKO BY T.MURAI 1997 */
/************************************************************************/

#ifndef		_RK_h
#define		_RK_h

typedef	struct {
   int		ylen;		/* yomigana no nagasa (in byte) */ 
   int		klen;		/* kanji no nagasa (in byte) */
   int		rownum;		/* row number */
   int		colnum;		/* column number */
   int		dicnum;		/* dic number */
}RkLex;

typedef	struct {
   int		bunnum;		/* bunsetsu bangou */
   int		candnum;	/* kouho bangou */
   int		maxcand;  	/* sou kouho suu */
   int		diccand;	/* jisho ni aru kouho suu */
   int		ylen;		/* yomigana no nagasa (in byte) */ 
   int		klen;		/* kanji no nagasa (in byte) */
   int		tlen;		/* tango no kosuu */
}		RkStat;

struct DicInfo {
    unsigned char	*di_dic;
    unsigned char	*di_file;
    int			di_kind;
    int			di_form;
    unsigned		di_count;
    int			di_mode;
    long		di_time;
};

/* romaji/kanakanji henkan code */
const unsigned long	RK_XFERBITS = 4;	/* bit-field width */
#define	RK_XFERMASK	((1<<RK_XFERBITS)-1)
#define	RK_NFER		0	/* muhenkan */
#define	RK_XFER		1	/* hiragana henkan */
#define	RK_HFER		2	/* hankaku henkan */
#define	RK_KFER		3	/* katakana henkan */
#define	RK_ZFER		4	/* zenkaku  henkan */

#define	RK_CTRLHENKAN		0xf
#define	RK_HENKANMODE(flags)	(((flags)<<RK_XFERBITS)|RK_CTRLHENKAN)

#define RK_TANBUN		0x01
#define RK_MAKE_WORD		0x02
#define RK_MAKE_EISUUJI		0x04
#define RK_MAKE_KANSUUJI	0x08

/* RkRxDic
 *	romaji/kana henkan jisho 
 */
struct RkRxDic	{
    int                 dic;		/* dictionary version: see below */
    unsigned char	*nr_string;	/* romaji/kana taiou hyou */
    int			nr_strsz;	/* taiou hyou no size */
    unsigned char	**nr_keyaddr;	/* romaji key no kaishi iti */
    int			nr_nkey;	/* romaji/kana taiou suu */
    unsigned char       *nr_bchars;     /* backtrack no trigger moji */
    unsigned char       *nr_brules;     /* backtrack no kanouseino aru rule */
};

#define RX_KPDIC 0 /* new format dictionary */
#define RX_RXDIC 1 /* old format dictionary */

/* kanakanji henkan */

/* romaji hennkan code */
#define	RK_FLUSH	0x8000	/* flush */
#define	RK_SOKON	0x4000	/* sokuon shori */
#define RK_IGNORECASE	0x2000  /* ignore case */

#define	RK_BIN		0
#define	RK_TXT		0x01

#define	RK_MWD	    0
#define	RK_SWD		1
#define	RK_PRE		2
#define	RK_SUC		3

#define KYOUSEI		0x01		/* jisho_overwrite_mode */

#define	Rk_MWD		0x80		/* jiritsugo_jisho */
#define	Rk_SWD		0x40		/* fuzokugo_jisho */
#define	Rk_PRE		0x20		/* settougo_jisho */
#define	Rk_SUC		0x10		/* setsubigo_jisho */

/* permission for RkwChmod() */
#define RK_ENABLE_READ   0x01
#define RK_DISABLE_READ  0x02
#define RK_ENABLE_WRITE  0x04
#define RK_DISABLE_WRITE 0x08
/* chmod for directories */
#define RK_USR_DIR       0x3000
#define RK_GRP_DIR       0x1000
#define RK_SYS_DIR       0x2000
#define RK_DIRECTORY     (RK_USR_DIR | RK_GRP_DIR | RK_SYS_DIR)
/* chmod for dictionaries */
#define RK_USR_DIC       0	/* specify user dic */
#define RK_GRP_DIC       0x4000	/* specify group dic */
#define RK_SYS_DIC       0x8000	/* specify system dic */

#define PL_DIC		 0x0100
#define PL_ALLOW	 0x0200
#define PL_INHIBIT	 0x0400
#define PL_FORCE	 0x0800

#define	NOENT	-2	/* No such file or directory		*/
#define	IO	-5	/* I/O error				*/
#define	NOTALC	-6	/* Cann't alloc. 			*/
#define	BADF	-9	/* irregal argument			*/
#define	BADDR	-10	/* irregal dics.dir	 		*/
#define	ACCES	-13	/* Permission denied 			*/
#define	NOMOUNT	-15	/* cannot mount				*/
#define	MOUNT	-16	/* file already mounted			*/
#define	EXIST	-17	/* file already exits			*/
#define	INVAL	-22	/* irregal argument			*/
#define	TXTBSY	-26	/* text file busy			*/
#define BADARG	-99	/* Bad Argment				*/
#define BADCONT -100	/* Bad Context				*/
#define OLDSRV    -110
#define NOTUXSRV  -111
#define NOTOWNSRV -112

/* kanakanji henkan */

#ifdef __cplusplus
extern "C" {
#endif
canna_export void RkwFinalize(void);
canna_export int RkwInitialize(char *);
canna_export int RkwCreateContext(void);
canna_export int RkwCloseContext(int);
canna_export int RkwDuplicateContext(int);
canna_export int RkwSetDicPath(int, char *);
canna_export int RkwGetDirList(int, char *,int);
canna_export int RkwGetDicList(int, char *,int);
canna_export int RkwMountDic(int, char *, int);
canna_export int RkwUnmountDic(int, char *);
canna_export int RkwRemountDic(int, char *, int);
canna_export int RkwSync(int, char *);
canna_export int RkwGetMountList(int, char *, int);
canna_export int RkwDefineDic(int, char *, WCHAR_T *);
canna_export int RkwDeleteDic(int, char *, WCHAR_T *);
canna_export int RkwBgnBun(int, WCHAR_T *, int, int);
canna_export int RkwEndBun(int, int);
canna_export int RkwGoTo(int, int);
canna_export int RkwLeft(int);
canna_export int RkwRight(int);
canna_export int RkwXfer(int, int);
canna_export int RkwNfer(int);
canna_export int RkwNext(int);
canna_export int RkwPrev(int);
canna_export int RkwResize(int, int);
canna_export int RkwEnlarge(int);
canna_export int RkwShorten(int);
canna_export int RkwSubstYomi(int, int, int, WCHAR_T *, int);
canna_export int RkwStoreYomi(int, WCHAR_T *, int);
canna_export int RkwGetLastYomi(int, WCHAR_T *, int);
canna_export int RkwFlushYomi(int);
canna_export int RkwRemoveBun(int, int);
canna_export int RkwGetStat(int, RkStat *);
canna_export int RkwGetYomi(int, WCHAR_T *, int);
canna_export int RkwGetHinshi(int, WCHAR_T *, int);
canna_export int RkwGetKanji(int, WCHAR_T *, int);
canna_export int RkwGetKanjiList(int, WCHAR_T *, int);
canna_export int RkwGetLex(int, RkLex *, int);
canna_export int RkwCvtHira(WCHAR_T *, int, WCHAR_T *, int);
canna_export int RkwCvtKana(WCHAR_T *, int, WCHAR_T *, int);
canna_export int RkwCvtHan(WCHAR_T *, int, WCHAR_T *, int);
canna_export int RkwCvtZen(WCHAR_T *, int, WCHAR_T *, int);
canna_export int RkwCvtEuc(WCHAR_T *, int, WCHAR_T *, int);
canna_export int RkwCvtSuuji(WCHAR_T *, int , WCHAR_T *, int , int );
canna_export int RkwCvtNone(WCHAR_T *, int , WCHAR_T *, int );
canna_export int RkwCreateDic(int, char *, int);
canna_export int RkwQueryDic(int, char *, char *, struct DicInfo *);
canna_export void RkwCloseRoma(struct RkRxDic *);
canna_export struct RkRxDic * RkwOpenRoma(char *);
canna_export int RkwSetUserInfo(char *, char *, char *);
canna_export char * RkwGetServerName(void);
canna_export int RkwGetServerVersion(int *, int *);
canna_export int RkwListDic(int, char *, char *, int);
canna_export int RkwCopyDic(int, char *, char *, char *, int);
canna_export int RkwRemoveDic(int, char *, int);
canna_export int RkwRenameDic(int, char *, char *, int);
canna_export int RkwChmodDic(int, char *, int);
canna_export int RkwGetWordTextDic(int, unsigned char *, unsigned char *, WCHAR_T *, int);
canna_export int RkwGetSimpleKanji(int, char *, WCHAR_T *, int, WCHAR_T *, int, char *, int);

void RkFinalize(void);
int RkInitialize(char *);
int RkCreateContext(void);
int RkCloseContext(int);
int	RkDuplicateContext(int);
int	RkSetDicPath(int, char *);
int	RkGetDirList(int, char *,int);
int	RkGetDicList(int, char *,int);
int	RkMountDic(int, char *, int);
int	RkUnmountDic(int, char *);
int	RkRemountDic(int, char *, int);
int	RkSync(int, char *);
int	RkGetMountList(int, char *, int);
int	RkDefineDic(int, char *, char *);
int	RkDeleteDic(int, char *, char *);
int	RkBgnBun(int, char *, int, int);
int	RkEndBun(int, int);
int	RkGoTo(int, int);
int	RkLeft(int);
int	RkRight(int);
int	RkXfer(int, int);
int	RkNfer(int);
int	RkNext(int);
int	RkPrev(int);
int	RkResize(int, int);
int	RkEnlarge(int);
int	RkShorten(int);
int	RkSubstYomi(int, int, int, char *, int);
int	RkStoreYomi(int, char *, int);
int	RkGetLastYomi(int, char *, int);
int	RkFlushYomi(int);
int	RkRemoveBun(int, int);
int	RkGetStat(int, RkStat *);
int	RkGetYomi(int, unsigned char *, int);
int	RkGetHinshi(int, unsigned char *, int);
int	RkGetKanji(int, unsigned char *, int);
int	RkGetKanjiList(int, unsigned char *, int);
int	RkGetLex(int, RkLex *, int);
int	RkCvtHira(unsigned char *, int, unsigned char *, int);
int	RkCvtKana(unsigned char *, int, unsigned char *, int);
int	RkCvtHan(unsigned char *, int, unsigned char *, int);
int	RkCvtZen(unsigned char *, int, unsigned char *, int);
int	RkCvtNone(unsigned char *, int, unsigned char *, int);
int	RkCvtEuc(unsigned char *, int, unsigned char *, int);
int RkCvtWide(WCHAR_T *, int , char *, int );
int RkCvtNarrow(char *, int , WCHAR_T *, int );
int	RkQueryDic(int, char *, char *, struct DicInfo *);

#ifdef __cplusplus
}
#endif

#endif	/* _RK_h */
/* don't add stuff after this line */
