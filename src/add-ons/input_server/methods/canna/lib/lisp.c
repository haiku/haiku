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
static char rcsid[] = "$Id: lisp.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif

/* 
** main program of lisp 
*/
#if (defined(_WINDOWS) || defined(WIN32)) && !defined(WIN)
#define WIN
#endif

#ifdef WIN
#define WIN_CANLISP
#endif

#include <InterfaceDefs.h>

#include "lisp.h"
#include "patchlevel.h"

#include <signal.h>

extern void (*keyconvCallback)(...);

//static int CANNA_mbstowcs(WCHAR_T *dest, char *src, int destlen);
static void fillMenuEntry(void);
static void intr(int sig);
static int initIS(void);
static void finIS(void);
static int identifySequence(unsigned c, int *val);
static int alloccell(void);
static int allocarea(void);
static void freearea(void);
static list getatmz(char *name);
static list mkatm(char *name);
static list getatm(char *name, int key);
static void error(char *msg, list v);
static void fatal(char *msg, list v);
static void argnerr(char *msg);
static void numerr(char *fn, list arg);
static void lisp_strerr(char *fn, list arg);
static list Lread(int n);
static list read1(void);
static int skipspaces(void);
static int zaplin(void);
static list newcons(void);
static list newsymbol(char *name);
static void print(list l);
static list ratom(void);
static list ratom2(int a);
static list rstring(void);
static list rcharacter(void);
static int isnum(char *name);
static void untyi(int c);
static int tyi(void);
static int tyipeek(void);
static void prins(char *s);
static int isterm(int c);
static void push(list value);
static void pop(int x);
static list pop1(void);
static void epush(list value);
static list epop(void);
static void patom(list atm);
static void gc(void);
static list allocstring(int n);
static list copystring(char *s, int n);
static list copycons(struct cell *l);
static void markcopycell(list *addr);
static list bindall(list var, list par, list a, list e);
static list Lquote(void);
static list Leval(int n);
static list assq(list e, list a);
static int evpsh(list args);
static list Lprogn(void);
static list Lcons(int n);
static list Lncons(int n);
static list Lxcons(int n);
static list Lprint(int n);
static list Lset(int n);
static list Lsetq(void);
static list Lequal(int n);
static int Strncmp(char *x, char *y, int len);
static char *Strncpy(char *x, char *y, int len);
static int equal(list x, list y);
static list Lgreaterp(int n);
static list Llessp(int n);
static list Leq(int n);
static list Lcond(void);
static list Lnull(int n);
static list Lor(void);
static list Land(void);
static list Lplus(int n);
static list Ltimes(int n);
static list Ldiff(int n);
static list Lquo(int n);
static list Lrem(int n);
static list Lgc(int n);
static list Lusedic(int n);
static list Llist(int n);
static list Lcopysym(int n);
static list Lload(int n);
static list Lmodestr(int n);
static int xfseq(char *fname, list l, unsigned char *arr, int arrsize);
static list Lsetkey(int n);
static list Lgsetkey(int n);
static list Lputd(int n);
static list Ldefun(void);
static list Ldefmacro(void);
static list Lcar(int n);
static list Lcdr(int n);
static list Latom(int n);
static list Llet(void);
static list Lif(void);
static list Lunbindkey(int n);
static list Lgunbindkey(int n);
static list Ldefmode(void);
static list Ldefsym(void);
static int getKutenCode(char *data, int *ku, int *ten);
static int howManyCharsAre(char *tdata, char *edata, int *tku, int *tten, int *codeset);
static char *pickupChars(int tku, int tten, int num, int kodata);
static void numtostr(unsigned long num, char *str);
static list Ldefselection(void);
static list Ldefmenu(void);
static list Lsetinifunc(int n);
static list Lboundp(int n);
static list Lfboundp(int n);
static list Lgetenv(int n);
static list LdefEscSeq(int n);
static list LdefXKeysym(int n);
static list Lconcat(int n);
static void ObtainVersion(void);
static list VTorNIL(BYTE *var, int setp, list arg);
static list StrAcc(char **var, int setp, list arg);
static list NumAcc(int *var, int setp, list arg);
static list Vnkouhobunsetsu(int setp, list arg);
static list VProtoVer(int setp, list arg);
static list VServVer(int setp, list arg);
static list VServName(int setp, list arg);
static list VCannaDir(int setp, list arg);
static list VCodeInput(int setp, list arg);
static void deflispfunc(void);
static void defcannavar(void);
static void defcannamode(void);
static void defcannafunc(void);
static void defatms(void);
static void restoreLocalVariables(void);

static FILE *outstream = (FILE *)0;

#ifdef WIN
extern int RkwGetProtocolVersion (int *, int *);
extern int RkwGetServerVersion (int *, int *);
#endif

static char *celltop, *cellbtm, *freecell;
static char *memtop;

static int ncells = CELLSIZE;


/* parameter stack */

static list	*stack, *sp;

/* environment stack	*/

static list	*estack, *esp;

/* oblist */

static list	*oblist;	/* oblist hashing array		*/

#define LISPERROR	-1

typedef struct {
  FILE *f;
  char *name;
  unsigned line;
} lispfile;

static lispfile *files;
static int  filep;

/* lisp read buffer & read pointer */

static char *readbuf;		/* read buffer	*/
static char *readptr;		/* read pointer	*/

/* error functions	*/

static void	argnerr(), numerr(), error();

/* multiple values */

#define MAXVALUES 16
static list *values;	/* multiple values here	*/
static int  valuec;	/* number of values here	*/

/* symbols */

static list QUOTE, T, _LAMBDA, _MACRO, COND, USER;
static list BUSHU, GRAMMAR, RENGO, KATAKANA, HIRAGANA, HYPHEN;

#include <setjmp.h>

static struct lispcenv {
  jmp_buf jmp_env;
  int     base_stack;
  int     base_estack;
} *env; /* environment for setjmp & longjmp	*/
static int  jmpenvp = MAX_DEPTH;

static jmp_buf fatal_env;

#ifdef WIN_CANLISP
#include "cannacnf.h"

struct winstruct {
  struct libconf *conf;
  struct libconfwrite *confwrite;
  struct RegInfo *rinfo;
  char *context;
} wins;
#endif

#ifdef WIN_CANLISP
char *RemoteGroup = (char *)NULL;
char *LocalGroup = (char *)NULL;
#endif

/* tyo -- output one character	*/

inline
void tyo(int c)
{
  if (outstream) {
    (void)putc(c, outstream);
  }
}

/* external functions

   ³°Éô´Ø¿ô¤Ï°Ê²¼¤Î£³¤Ä

  (1) clisp_init()  --  ¥«¥¹¥¿¥Þ¥¤¥º¥Õ¥¡¥¤¥ë¤òÆÉ¤à¤¿¤á¤Î½àÈ÷¤ò¤¹¤ë

    lisp ¤Î½é´ü²½¤ò¹Ô¤¤É¬Í×¤Ê¥á¥â¥ê¤ò allocate ¤¹¤ë¡£

  (2) clisp_fin()   --  ¥«¥¹¥¿¥Þ¥¤¥ºÆÉ¤ß¹þ¤ßÍÑ¤ÎÎÎ°è¤ò²òÊü¤¹¤ë¡£

    ¾åµ­¤Î½é´ü²½¤ÇÆÀ¤¿¥á¥â¥ê¤ò²òÊü¤¹¤ë¡£

  (3) YYparse_by_rcfilename((char *)s) -- ¥«¥¹¥¿¥Þ¥¤¥º¥Õ¥¡¥¤¥ë¤òÆÉ¤ß¹þ¤à¡£

    s ¤Ç»ØÄê¤µ¤ì¤¿¥Õ¥¡¥¤¥ëÌ¾¤Î¥«¥¹¥¿¥Þ¥¤¥º¥Õ¥¡¥¤¥ë¤òÆÉ¤ß¹þ¤ó¤Ç¥«¥¹¥¿
    ¥Þ¥¤¥º¤ÎÀßÄê¤ò¹Ô¤¦¡£¥Õ¥¡¥¤¥ë¤¬Â¸ºß¤¹¤ì¤Ð 1 ¤òÊÖ¤·¤½¤¦¤Ç¤Ê¤±¤ì¤Ð
    0 ¤òÊÖ¤¹¡£

 */

static list getatmz(char *);

#ifdef WIN_CANLISP
/*
 * ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿¥ª¥Ú¥ì¡¼¥·¥ç¥ó (from util.c)
 *
 */

static int wchar_type; /* ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿¤Î¥¿¥¤¥×(²¼¤ò¸«¤è) */

#define CANNA_WCTYPE_16 0  /* 16¥Ó¥Ã¥ÈÉ½¸½ */
#define CANNA_WCTYPE_32 1  /* 32¥Ó¥Ã¥ÈÉ½¸½ */
#define CANNA_WCTYPE_OT 99 /* ¤½¤ÎÂ¾¤ÎÉ½¸½ */

/*
 WCinit() -- ¥ï¥¤¥É¥­¥ã¥é¥¯¥¿¤È¤·¤Æ¤É¤ì¤¬»È¤ï¤ì¤Æ¤¤¤ë¤«¤ò³ÎÇ§¤¹¤ë

        ¤³¤Î´Ø¿ô¤¬¸Æ¤Ó½Ð¤µ¤ì¤ë¤Þ¤¨¤Ë setlocale ¤¬¤Ê¤µ¤ì¤Æ¤¤¤Ê¤±¤ì¤Ð¤Ê¤é¤Ê¤¤
 */

#define TYPE16A 0x0000a4a2
#define TYPE32A 0x30001222


int
WCinit(void)
{
#if defined(HAVE_WCHAR_OPERATION) && !defined(WIN)
  extern int locale_insufficient;
  WCHAR_T wc[24];
  char *a = "\244\242"; /* ¤¢ */ /* 0xa4a2 */

  locale_insufficient = 0;
  if (mbstowcs(wc, a, sizeof(wc) / sizeof(WCHAR_T)) != 1) {
    /* Â¿Ê¬ setlocale ¤¬¤Ê¤µ¤ì¤Æ¤¤¤Ê¤¤ */
    setlocale(LC_CTYPE, "");
    if (mbstowcs(wc, a, sizeof(wc) / sizeof(WCHAR_T)) != 1) {
      setlocale(LC_CTYPE, JAPANESE_LOCALE);
      if (mbstowcs(wc, a, sizeof(wc) / sizeof(WCHAR_T)) != 1) {
	locale_insufficient = 1;
	return -1;
      }
    }
  }
  switch (wc[0]) {
  case TYPE16A:
    wchar_type = CANNA_WCTYPE_16;
    break;
  case TYPE32A:
    wchar_type = CANNA_WCTYPE_32;
    break;
  default:
    wchar_type = CANNA_WCTYPE_OT;
    break;
  }
#else /* !HAVE_WCHAR_OPERATION || WIN */
# ifdef WCHAR16

  wchar_type = CANNA_WCTYPE_16;

# else /* !WCHAR16 */

  if (sizeof(WCHAR_T) == 2) {
    /* NOTREACHED */
    wchar_type = CANNA_WCTYPE_16;
  }
  else {
    /* NOTREACHED */
    wchar_type = CANNA_WCTYPE_32;
  }

# endif /* !WCHAR16 */
#endif /* !HAVE_WCHAR_OPERATION || WIN */

  return 0;
}

static int
CANNA_mbstowcs(WCHAR_T *dest, char *src, int destlen)
{
  register int i, j;
  register unsigned ec;

  if (wchar_type == CANNA_WCTYPE_16) {
    for (i = 0, j = 0 ;
	 (ec = (unsigned)(unsigned char)src[i]) != 0 && j < destlen ; i++) {
      if (ec & 0x80) {
	switch (ec) {
	case 0x8e: /* SS2 */
	  dest[j++] = (WCHAR_T)(0x80 | ((unsigned)src[++i] & 0x7f));
	  break;
	case 0x8f: /* SS3 */
	  dest[j++] = (WCHAR_T)(0x8000
				| (((unsigned)src[i + 1] & 0x7f) << 8)
				| ((unsigned)src[i + 2] & 0x7f));
	  i += 2;
	  break;
	default:
	  dest[j++] = (WCHAR_T)(0x8080 | (((unsigned)src[i] & 0x7f) << 8)
				| ((unsigned)src[i + 1] & 0x7f));
	  i++;
	  break;
	}
      }else{
	dest[j++] = (WCHAR_T)ec;
      }
    }
    if (j < destlen)
      dest[j] = (WCHAR_T)0;
    return j;
  }
  else if (wchar_type == CANNA_WCTYPE_32) {
    for (i = 0, j = 0 ;
	 (ec = (unsigned)(unsigned char)src[i]) != 0 && j < destlen ; i++) {
      if (ec & 0x80) {
	switch (ec) {
	case 0x8e: /* SS2 */
	  dest[j++] = (WCHAR_T)(0x10000000L | ((unsigned)src[++i] & 0x7f));
	  break;
	case 0x8f: /* SS3 */
	  dest[j++] = (WCHAR_T)(0x20000000L
				| (((unsigned)src[i + 1] & 0x7f) << 7)
				| ((unsigned)src[i + 2] & 0x7f));
	  i += 2;
	  break;
	default:
	  dest[j++] = (WCHAR_T)(0x30000000L | (((unsigned)src[i] & 0x7f) << 7)
				| ((unsigned)src[i + 1] & 0x7f));
	  i++;
	  break;
	}
      }else{
	dest[j++] = (WCHAR_T)ec;
      }
    }
    if (j < destlen)
      dest[j] = (WCHAR_T)0;
    return j;
  }
  else {
    return 0;
  }
}

#endif /* WIN */

int
clisp_init(void)
{
  int  i;

#ifdef WIN_CANLISP
  WCinit();
#endif

  if ( !allocarea() ) {
    return 0;
  }

  if ( !initIS() ) {
    freearea();
    return 0;
  }

  /* stack pointer initialization	*/
  sp = stack + STKSIZE;
  esp = estack + STKSIZE;
  epush(NIL);

  /* initialize read pointer	*/
  readptr = readbuf;
  *readptr = '\0';
  files[filep = 0].f = stdin;
  files[filep].name = (char *)0;
  files[filep].line = 0;

  /* oblist initialization	*/
  for (i = 0; i < BUFSIZE ; i++)
    oblist[i] = 0;

  /* symbol definitions */
  defatms();
  return 1;
}

#ifndef NO_EXTEND_MENU
static void
fillMenuEntry(void)
{
  extern extraFunc *extrafuncp;
  extraFunc *p, *fp;
  int i, n, fid;
  menuitem *mb;

  for (p = extrafuncp ; p ; p = p->next) {
    if (p->keyword == EXTRA_FUNC_DEFMENU) {
      n = p->u.menuptr->nentries;
      mb = p->u.menuptr->body;
      for (i = 0 ; i < n ; i++, mb++) {
	if (mb->flag == MENU_SUSPEND) {
	  list l = (list)mb->u.misc;
	  fid = symbolpointer(l)->fid;
	  if (fid < CANNA_FN_MAX_FUNC) {
	    goto just_a_func;
	  }
	  else {
	    fp = FindExtraFunc(fid);
	    if (fp && fp->keyword == EXTRA_FUNC_DEFMENU) {
	      mb->u.menu_next = fp->u.menuptr;
	      mb->flag = MENU_MENU;
	    }
	    else {
	    just_a_func:
	      mb->u.fnum = fid;
	      mb->flag = MENU_FUNC;
	    }
	  }
	}
      }
    }
  }
}
#endif /* NO_EXTEND_MENU */

#define UNTYIUNIT 32
static char *untyibuf = 0;
static int untyisize = 0, untyip = 0;

void
clisp_fin(void)
{
#ifndef NO_EXTEND_MENU
  /* ½ª¤ë¤ËÅö¤¿¤Ã¤Æ¡¢menu ´ØÏ¢¤Î¥Ç¡¼¥¿¤òËä¤á¤ë */
  fillMenuEntry();
#endif

  finIS();

  while (filep >= 0) {
    if (files[filep].f && files[filep].f != stdin) {
      fclose(files[filep].f);
    }
    if (files[filep].name) {
      free(files[filep].name);
    }
    filep--;
  }

  freearea();
  if (untyisize) {
    free(untyibuf);
    untyisize = 0;
    untyibuf = (char *)0;
  }
}

int
YYparse_by_rcfilename(char *s)
{
  extern int ckverbose;
  int retval = 0;
  FILE *f;
  FILE *saved_outstream;

  if (setjmp(fatal_env)) {
    retval = 0;
    goto quit_parse_rcfile;
  }

  if (jmpenvp <= 0) { /* ºÆµ¢¤¬¿¼¤¹¤®¤ë¾ì¹ç */
    return 0;
  }
  jmpenvp--;

  if (ckverbose >= CANNA_HALF_VERBOSE) {
    saved_outstream = outstream;
#ifndef WIN  /* what ? */
    outstream = stdout;
#endif
  }

  f = fopen(s, "r");
  if (f) {
    if (ckverbose == CANNA_FULL_VERBOSE) {
#ifndef WIN
      printf("¥«¥¹¥¿¥Þ¥¤¥º¥Õ¥¡¥¤¥ë¤È¤·¤Æ \"%s\" ¤òÍÑ¤¤¤Þ¤¹¡£\n", s);
#endif
    }
    files[++filep].f = f;
    files[filep].name = (char *)malloc(strlen(s) + 1);
    if (files[filep].name) {
      strcpy(files[filep].name, s);
    }
    else {
      filep--;
      fclose(f);
      goto quit_parse_rcfile;
    }
    files[filep].line = 0;

    setjmp(env[jmpenvp].jmp_env);
    env[jmpenvp].base_stack = sp - stack;
    env[jmpenvp].base_estack = esp - estack;

    for (;;) {
      push(Lread(0));
      if (valuec > 1 && null(values[1])) {
	break;
      }
      (void)Leval(1);
    }
    retval = 1;
  }

  if (ckverbose >= CANNA_HALF_VERBOSE) {
    outstream = saved_outstream;
  }

  jmpenvp++;
 quit_parse_rcfile:
  return retval;
}

#define WITH_MAIN
#ifdef WITH_MAIN

static void
intr(int sig)
/* ARGSUSED */
{
  error("Interrupt:",NON);
  /* NOTREACHED */
}

/* cfuncdef

   parse_string -- Ê¸»úÎó¤ò¥Ñ¡¼¥¹¤¹¤ë¡£

*/

int parse_string(char *str)
{
  char *readbufbk;

  if (clisp_init() == 0) {
    return -1;
  }

  /* read buffer ¤È¤·¤ÆÍ¿¤¨¤é¤ì¤¿Ê¸»ú¤ò»È¤¦ */
  readbufbk = readbuf;
  readptr = readbuf = str;

  if (setjmp(fatal_env)) {
    goto quit_parse_string;
  }

  if (jmpenvp <= 0) { /* ºÆµ¢¤¬¿¼¤¹¤®¤ë¾ì¹ç */
    return -1;
  }

  jmpenvp--;
  files[++filep].f = (FILE *)0;
  files[filep].name = (char *)0;
  files[filep].line = 0;

  setjmp(env[jmpenvp].jmp_env);
  env[jmpenvp].base_stack = sp - stack;
  env[jmpenvp].base_estack = esp - estack;

  for (;;) {
    list t;

    t = Lread(0);
    if (valuec > 1 && null(values[1])) {
      break;
    }
    else {
      push(t);
      Leval(1);
    }
  }
  jmpenvp++;
 quit_parse_string:
  readbuf = readbufbk;
  clisp_fin();
  return 0;
}

static void intr();

void
clisp_main(void)
{
  if (clisp_init() == 0) {	/* initialize data area	& etc..	*/
    fprintf(stderr, "CannaLisp: initialization failed.\n");
#ifndef WIN
    exit(1);
#endif
  }

  if (setjmp(fatal_env)) {
    goto quit_clisp_main;
  }

  if (jmpenvp <= 0) { /* ºÆµ¢¤¬¿¼¤¹¤®¤ë¾ì¹ç */
    return;
  }
  jmpenvp--;

  fprintf(stderr,"CannaLisp listener %d.%d%s\n",
	  CANNA_MAJOR_MINOR / 1000, CANNA_MAJOR_MINOR % 1000,
	  CANNA_PATCH_LEVEL);

  outstream = stdout;

  setjmp(env[jmpenvp].jmp_env);
  env[jmpenvp].base_stack = sp - stack;
  env[jmpenvp].base_estack = esp - estack;

#ifndef WIN
  signal(SIGINT, intr);
#endif
  for (;;) {
    prins("-> ");		/* prompt	*/
    push(Lread(0));
    if (valuec > 1 && null(values[1])) {
      break;
    }
    push(Leval(1));
    if (sp[0] == LISPERROR) {
      (void)pop1();
    }
    else {
      (void)Lprint(1);
      prins("\n");
    }
  }
  jmpenvp++;
 quit_clisp_main:
  prins("\nGoodbye.\n");
  clisp_fin();
}

#endif /* WITH_MAIN */

static int longestkeywordlen;

typedef struct {
  char *seq;
  int id;
} SeqToID;

/* #include <InterfaceDefs.h> */
static SeqToID keywordtable[] = {
  {"Space"      ,' '},
  {"Escape"     ,'\033'},
  {"Tab"        ,B_TAB},
  {"Nfer"       ,CANNA_KEY_Nfer},
  {"Xfer"       ,CANNA_KEY_Xfer},
  {"Backspace"  ,B_BACKSPACE},
  {"Delete"     ,'\177'},
  {"Insert"     ,CANNA_KEY_Insert},
  {"Rollup"     ,CANNA_KEY_Rollup},
  {"Rolldown"   ,CANNA_KEY_Rolldown},
  {"Up"         ,CANNA_KEY_Up},
  {"Left"       ,CANNA_KEY_Left},
  {"Right"      ,CANNA_KEY_Right},
  {"Down"       ,CANNA_KEY_Down},
  {"Home"       ,CANNA_KEY_Home},
  {"Clear"      ,'\013'},
  {"Help"       ,CANNA_KEY_Help},
  {"Enter"      ,B_RETURN},
  {"Return"     ,B_RETURN},
/* "F1" is processed by program */
  {"F2"         ,CANNA_KEY_F2},
  {"F3"         ,CANNA_KEY_F3},
  {"F4"         ,CANNA_KEY_F4},
  {"F5"         ,CANNA_KEY_F5},
  {"F6"         ,CANNA_KEY_F6},
  {"F7"         ,CANNA_KEY_F7},
  {"F8"         ,CANNA_KEY_F8},
  {"F9"         ,CANNA_KEY_F9},
  {"F10"        ,CANNA_KEY_F10},
/* "Pf1" is processed by program */
  {"Pf2"        ,CANNA_KEY_PF2},
  {"Pf3"        ,CANNA_KEY_PF3},
  {"Pf4"        ,CANNA_KEY_PF4},
  {"Pf5"        ,CANNA_KEY_PF5},
  {"Pf6"        ,CANNA_KEY_PF6},
  {"Pf7"        ,CANNA_KEY_PF7},
  {"Pf8"        ,CANNA_KEY_PF8},
  {"Pf9"        ,CANNA_KEY_PF9},
  {"Pf10"       ,CANNA_KEY_PF10},
  {"S-Nfer"     ,CANNA_KEY_Shift_Nfer},
  {"S-Xfer"     ,CANNA_KEY_Shift_Xfer},
  {"S-Up"       ,CANNA_KEY_Shift_Up},
  {"S-Down"     ,CANNA_KEY_Shift_Down},
  {"S-Left"     ,CANNA_KEY_Shift_Left},
  {"S-Right"    ,CANNA_KEY_Shift_Right},
  {"C-Nfer"     ,CANNA_KEY_Cntrl_Nfer},
  {"C-Xfer"     ,CANNA_KEY_Cntrl_Xfer},
  {"C-Up"       ,CANNA_KEY_Cntrl_Up},
  {"C-Down"     ,CANNA_KEY_Cntrl_Down},
  {"C-Left"     ,CANNA_KEY_Cntrl_Left},
  {"C-Right"    ,CANNA_KEY_Cntrl_Right},
  {0            ,0},
};

#define charToNum(c) charToNumTbl[(c) - ' ']

static int *charToNumTbl;

typedef struct {
  int id;
  int *tbl;
} seqlines;

static seqlines *seqTbl;	/* ÆâÉô¤ÎÉ½(¼ÂºÝ¤Ë¤ÏÉ½¤ÎÉ½) */
static int nseqtbl;		/* ¾õÂÖ¤Î¿ô¡£¾õÂÖ¤Î¿ô¤À¤±É½¤¬¤¢¤ë */
static int nseq;
static int seqline;

static
int initIS(void)
{
  SeqToID *p;
  char *s;
  int i;
  seqlines seqTbls[1024];

  seqTbl = (seqlines *)0;
  seqline = 0;
  nseqtbl = 0;
  nseq = 0;
  longestkeywordlen = 0;
  for (i = 0 ; i < 1024 ; i++) {
    seqTbls[i].tbl = (int *)0;
    seqTbls[i].id = 0;
  }
  charToNumTbl = (int *)calloc('~' - ' ' + 1, sizeof(int));
  if ( !charToNumTbl ) {
    return 0;
  }

  /* ¤Þ¤º²¿Ê¸»ú»È¤ï¤ì¤Æ¤¤¤ë¤«¤òÄ´¤Ù¤ë¡£
     nseq ¤Ï»È¤ï¤ì¤Æ¤¤¤ëÊ¸»ú¿ô¤è¤ê£±Âç¤­¤¤ÃÍ¤Ç¤¢¤ë */
  for (p = keywordtable ; p->id ; p++) {
    int len = 0;
    for (s = p->seq ; *s ; s++) {
      if ( !charToNumTbl[*s - ' '] ) {
	charToNumTbl[*s - ' '] = nseq; /* ³ÆÊ¸»ú¤Ë¥·¥ê¥¢¥ëÈÖ¹æ¤ò¿¶¤ë */
	nseq++;
      }
      len ++;
    }
    if (len > longestkeywordlen) {
      longestkeywordlen = len;
    }
  }
  /* Ê¸»ú¿ôÊ¬¤Î¥Æ¡¼¥Ö¥ë */
  seqTbls[nseqtbl].tbl = (int *)calloc(nseq, sizeof(int));
  if ( !seqTbls[nseqtbl].tbl ) {
    goto initISerr;
  }
  nseqtbl++;
  for (p = keywordtable ; p->id ; p++) {
    int line, nextline;
    
    line = 0;
    for (s = p->seq ; *s ; s++) {
      if (seqTbls[line].tbl == 0) { /* ¥Æ¡¼¥Ö¥ë¤¬¤Ê¤¤ */
	seqTbls[line].tbl = (int *)calloc(nseq, sizeof(int));
	if ( !seqTbls[line].tbl ) {
	  goto initISerr;
	}
      }
      nextline = seqTbls[line].tbl[charToNum(*s)];
      /* ¤Á¤Ê¤ß¤Ë¡¢charToNum(*s) ¤ÏÀäÂÐ¤Ë£°¤Ë¤Ê¤é¤Ê¤¤ */
      if ( nextline ) {
	line = nextline;
      }else{ /* ºÇ½é¤Ë¥¢¥¯¥»¥¹¤·¤¿ */
	line = seqTbls[line].tbl[charToNum(*s)] = nseqtbl++;
      }
    }
    seqTbls[line].id = p->id;
  }
  seqTbl = (seqlines *)calloc(nseqtbl, sizeof(seqlines));
  if ( !seqTbl ) {
    goto initISerr;
  }
  for (i = 0 ; i < nseqtbl ; i++) {
    seqTbl[i].id  = seqTbls[i].id;
    seqTbl[i].tbl = seqTbls[i].tbl;
  }
  return 1;

 initISerr:
  free(charToNumTbl);
  charToNumTbl = (int *)0;
  if (seqTbl) {
    free(seqTbl);
    seqTbl = (seqlines *)0;
  }
  for (i = 0 ; i < nseqtbl ; i++) {
    if (seqTbls[i].tbl) {
      free(seqTbls[i].tbl);
      seqTbls[i].tbl = (int *)0;
    }
  }
  return 0;
}

static void
finIS(void) /* identifySequence ¤ËÍÑ¤¤¤¿¥á¥â¥ê»ñ¸»¤ò³«Êü¤¹¤ë */
{
  int i;

  if (seqTbl) {
    for (i = 0 ; i < nseqtbl ; i++) {
      if (seqTbl[i].tbl) free(seqTbl[i].tbl);
      seqTbl[i].tbl = (int *)0;
    }
    free(seqTbl);
    seqTbl = (seqlines *)0;
  }
  if (charToNumTbl) {
    free(charToNumTbl);
    charToNumTbl = (int *)0;
  }
}

/* cvariable

  seqline: identifySequence ¤Ç¤Î¾õÂÖ¤òÊÝ»ý¤¹¤ëÊÑ¿ô

 */

#define CONTINUE 1
#define END	 0

static
int identifySequence(unsigned c, int *val)
{
  int nextline;

  if (' ' <= c && c <= '~' && charToNum(c) &&
      (nextline = seqTbl[seqline].tbl[charToNum(c)]) ) {
    seqline = nextline;
    *val = seqTbl[seqline].id;
    if (*val) {
      seqline = 0;
      return END;
    }
    else {
      return CONTINUE; /* continue */
    }
  }
  else {
    *val = -1;
    seqline = 0;
    return END;
  }
}


static int
alloccell(void)
{
  int  cellsize, odd;
  char *p;

  cellsize = ncells * sizeof(list);
  p = (char *)malloc(cellsize);
  if (p == (char *)0) {
    return 0;
  }
  memtop = p;
  odd = (int)((pointerint)memtop % sizeof(list));
  freecell = celltop = memtop + (odd ? (sizeof(list)) - odd : 0);
  cellbtm = memtop + cellsize - odd;
  return 1;
}

/* ¤¦¤Þ¤¯¹Ô¤«¤Ê¤«¤Ã¤¿¤é£°¤òÊÖ¤¹ */

static
int allocarea(void)
{
  /* ¤Þ¤º¤Ï¥»¥ëÎÎ°è */
  if (alloccell()) {
    /* ¥¹¥¿¥Ã¥¯ÎÎ°è */
    stack = (list *)calloc(STKSIZE, sizeof(list));
    if (stack) {
      estack = (list *)calloc(STKSIZE, sizeof(list));
      if (estack) {
	/* oblist */
	oblist = (list *)calloc(BUFSIZE, sizeof(list));
	if (oblist) {
	  /* I/O */
	  filep = 0;
	  files = (lispfile *)calloc(MAX_DEPTH, sizeof(lispfile));
	  if (files) {
	    readbuf = (char *)malloc(BUFSIZE);
	    if (readbuf) {
	      /* jump env */
	      jmpenvp = MAX_DEPTH;
	      env = (struct lispcenv *)
		calloc(MAX_DEPTH, sizeof(struct lispcenv));
	      if (env) {
		/* multiple values returning buffer */
		valuec = 1;
		values = (list *)calloc(MAXVALUES, sizeof(list));
		if (values) {
		  return 1;
		}
		free(env);
	      }
	      free(readbuf);
	    }
	    free(files);
	  }
	  free(oblist);
	}
	free(estack);
      }
      free(stack);
    }
    free(memtop);
  }
  return 0;
}

static void
freearea(void)
{
  free(memtop);
  free(stack);
  free(estack);
  free(oblist);
  free(files);
  free(env);
  free(readbuf);
  if (values) {
    free(values);
    values = 0;
  }
}

static list
getatmz(char *name)
{
  int  key;
  char *p;

  for (p = name, key = 0 ; *p ; p++)
    key += *p;
  return getatm(name,key);
}

/* mkatm -
	making symbol function	*/

static list 
mkatm(char *name)
{
  list temp;
  struct atomcell *newatom;

  temp = newsymbol(name);
  newatom = symbolpointer(temp);
  newatom->value = (*name == ':') ? (list)temp : (list)UNBOUND;
  newatom->plist = NIL;			/* set null plist	*/
  newatom->ftype = UNDEF;		/* set undef func-type	*/
  newatom->func  = (list (*)(...))0;	/* Don't kill this line	*/
  newatom->valfunc  = (list (*)(...))0;	/* Don't kill this line	*/
  newatom->hlink = NIL;		/* no hash linking	*/
  newatom->mid = -1;
  newatom->fid = -1;

  return temp;
}

/* getatm -- get atom from the oblist if possible	*/

static list 
getatm(char *name, int key)
{
  list p;
  struct atomcell *atomp;

  key &= 0x00ff;
  for (p = oblist[key] ; p ;) {
    atomp = symbolpointer(p);
    if (!strcmp(atomp->pname, name)) {
      return p;
    }
    p = atomp->hlink;
  }
  p = mkatm(name);
  atomp = symbolpointer(p);
  atomp->hlink = oblist[key];
  oblist[key] = p;
  return p;
}

#define MESSAGE_MAX 256

static void
error(char *msg, list v)
/* ARGSUSED */
{
  char buf[MESSAGE_MAX];

  prins(msg);
  if (v != (list)NON)
    print(v);
  if (files[filep].f == stdin) {
    prins("\n");
  }
  else {
    if (files[filep].name) {
      sprintf(buf, " (%s near line %d)\n",
	      files[filep].name, files[filep].line);
    }
    else {
      sprintf(buf, " (near line %d)\n", files[filep].line);
    }
    prins(buf);
  }
  sp = &stack[env[jmpenvp].base_stack];
  esp = &estack[env[jmpenvp].base_estack];
/*  epush(NIL); */
  longjmp(env[jmpenvp].jmp_env,YES);
}

static void
fatal(char *msg, list v)
/* ARGSUSED */
{
  char buf[MESSAGE_MAX];

  prins(msg);
  if (v != (list)NON)
    print(v);
  if (files[filep].f == stdin) {
    prins("\n");
  }
  else {
    if (files[filep].name) {
      sprintf(buf, " (%s near line %d)\n",
	      files[filep].name, files[filep].line);
    }
    else {
      sprintf(buf, " (near line %d)\n", files[filep].line);
    }
    prins(buf);
  }
  longjmp(fatal_env, 1);
}

static void
argnerr(char *msg)
{
  prins("incorrect number of args to ");
  error(msg, NON);
  /* NOTREACHED */
}

static void
numerr(char *fn, list arg)
{
  prins("Non-number ");
  if (fn) {
    prins("to ");
    prins(fn);
  }
  error(": ",arg);
  /* NOTREACHED */
}

static void
lisp_strerr(char *fn, list arg)
{
  prins("Non-string ");
  if (fn) {
    prins("to ");
    prins(fn);
  }
  error(": ",arg);
  /* NOTREACHED */
}

static list
Lread(int n)
{
  list t;

  argnchk("read",0);
  valuec = 1;
  if ((t = read1()) == (list)LISPERROR) {
    readptr = readbuf;
    *readptr = '\0';
    if (files[filep].f != stdin) {
      fclose(files[filep].f);
      if (files[filep].name) {
	free(files[filep].name);
      }
      filep--;
    }
    values[0] = NIL;
    values[1] = NIL;
    valuec = 2;
    return(NIL);
  }
  else {
    values[0] = t;
    values[1] = T;
    valuec = 2;
    return(t);
  }
  /* NOTREACHED */
}

static void untyi (int);
static list rcharacter (void);

static list
read1(void)
{
  int  c;
  list p, *pp;
  list t;
  char *eofmsg = "EOF hit in reading a list : ";

 lab:
  if ( !skipspaces() ) {
    return((list)LISPERROR);
  }
  switch (c = tyi()) {
  case '(':
    push(NIL);
    p = Lncons(1);	/* get a new cell	*/
    car(p) = p;
    push(p);
    pp = sp;
    
    for (;;) {
    lab2:
      if ( !skipspaces() ) {
	error(eofmsg,cdr(*pp));
	/* NOTREACHED */
      }
      switch (c = tyi()) {
      case ';':
	zaplin();
	goto lab2;
      case ')':
	return(cdr(pop1()));
      case '.':
	if ( !(c = tyipeek()) ) {
	  error(eofmsg,cdr(*pp));
	  /* NOTREACHED */
	}
	else if ( !isterm(c) ) {
	  push(ratom2('.'));
	  push(NIL);
	  car(*pp) = cdar(*pp) = Lcons(2);
	  break;
	}
	else {
	  cdar(*pp) = read1();
	  if (cdar(*pp) == (list)LISPERROR) {
	    error(eofmsg,cdr(*pp));
	    /* NOTREACHED */
	  }
	  while (')' != (c = tyi()))
	    if ( !c ) {
	      error(eofmsg,cdr(*pp));
	      /* NOTREACHED */
	    }
	  return(cdr(pop1()));
	}
      default:
	untyi(c);
	if ((t = read1()) == (list)LISPERROR) {
	  error(eofmsg,cdr(*pp));
	  /* NOTREACHED */
	}
	push(t);
	push(NIL);
	car(*pp) = cdar(*pp) = Lcons(2);
      }
    }
  case '\'':
    push(QUOTE);
    if ((t = read1()) == (list)LISPERROR) {
      error(eofmsg,NIL);
      /* NOTREACHED */
    }
    push(t);
    push(NIL);
    push(Lcons(2));
    return Lcons(2);
  case '"':
    return rstring();
  case '?':
    return rcharacter();
  case ';':
    zaplin();
    goto lab;
  default:
    untyi(c);
    return ratom();
  }
}

/* skipping spaces function -
	if eof read then return NO	*/

static
int skipspaces(void)
{
  int c;

  while ((c = tyi()) <= ' ') {
    if ( !c ) {
      return(NO);
    }
#ifdef QUIT_IF_BINARY_CANNARC
/* ¼Â¤Ï fatal() ¤Ë¤·¤Æ¤·¤Þ¤¦¤È read ¤Ç¤­¤Ê¤«¤Ã¤¿¤È»×¤¤¡¢¼¡¤Î¥Õ¥¡¥¤¥ë¤ò
   Ãµ¤·¤Ë¹Ô¤¯¤Î¤Ç¤¢¤Þ¤êÎÉ¤¯¤Ê¤¤¡£return ¤ò¼õ¤±¤¿¤È¤³¤í¤âÊÑ¤¨¤Ê¤±¤ì¤Ð¤Ê
   ¤é¤Ê¤¤¡£ÌÌÅÝ¤Ê¤Î¤Ç¡¢¤È¤ê¤¢¤¨¤º³°¤¹ */
    if (c != '\033' && c != '\n' && c != '\r' && c!= '\t' && c < ' ') {
      fatal("read: Binary data read.", NON);
    }
#endif
  }
  untyi(c);
  return(YES);
}

/* skip reading until '\n' -
	if eof read then return NO	*/

static
int zaplin(void)
{
	int c;

	while ((c = tyi()) != '\n')
		if ( !c )
			return(NO);
	return(YES);
}

static void gc();

static list
newcons(void)
{
  list retval;

  if (freecell + sizeof(struct cell) >= cellbtm) {
    gc();
  }
  retval = CONS_TAG | (freecell - celltop);
  freecell += sizeof(struct cell);
  return retval;
}

static list
newsymbol(char *name)
{
  list retval;
  struct atomcell *temp;
  int namesize;

  namesize = strlen(name);
  namesize = ((namesize / sizeof(list)) + 1) * sizeof(list); /* +1¤Ï'\0'¤ÎÊ¬ */
  if (freecell + (sizeof(struct atomcell)) + namesize >= cellbtm) {
    gc();
  }
  temp = (struct atomcell *)freecell;
  retval = SYMBOL_TAG | (freecell - celltop);
  freecell += sizeof(struct atomcell);
  (void)strcpy(freecell, name);
  temp->pname = freecell;
  freecell += namesize;
  
  return retval;
}

static void patom();

static void
print(list l)
{
	if ( !l )	/* case NIL	*/
		prins("nil");
	else if (atom(l))
		patom(l);
	else {
		tyo('(');
		print(car(l));
		for (l = cdr(l) ; l ; l = cdr(l)) {
			tyo(' ');
			if (atom(l)) {
				tyo('.');
				tyo(' ');
				patom(l);
				break;
			}
			else 
				print(car(l));
		}
		tyo(')');
	}
}



/*
** read atom
*/


static list 
ratom(void)
{
	return(ratom2(tyi()));
}

/* read atom with the first one character -
	check if the token is numeric or pure symbol & return proper value */

static int isnum();

static list 
ratom2(int a)
{
  int  i, c, flag;
  char atmbuf[BUFSIZE];

  flag = NO;
  if (a == '\\') {
    flag = YES;
    a = tyi();
  }
  atmbuf[0] = a;
  for (i = 1, c = tyi(); !isterm(c) ; i++, c = tyi()) {
    if ( !c ) {
      error("Eof hit in reading symbol.", NON);
      /* NOTREACHED */
    }
    if (c == '\\') {
      flag = YES;
    }
    if (i < BUFSIZE) {
      atmbuf[i] = c;
    }
    else {
      error("Too long symbol name read", NON);
      /* NOTREACHED */
    }
  }
  untyi(c);
  if (i < BUFSIZE) {
    atmbuf[i] = '\0';
  }
  else {
    error("Too long symbol name read", NON);
    /* NOTREACHED */
  }
  if ( !flag && isnum(atmbuf)) {
    return(mknum(atoi(atmbuf)));
  }
  else if ( !flag && !strcmp("nil",atmbuf) ) {
    return(NIL);
  }
  else {
    return (getatmz(atmbuf));
  }
}

static list
rstring(void)
{
  char strb[BUFSIZE];
  int c;
  int strp = 0;

  while ((c = tyi()) != '"') {
    if ( !c ) {
      error("Eof hit in reading a string.", NON);
      /* NOTREACHED */
    }
    if (strp < BUFSIZE) {
      if (c == '\\') {
	untyi(c);
	c = (char)(((unsigned POINTERINT)rcharacter()) & 0xff);
      }
      strb[strp++] = (char)c;
    }
    else {
      error("Too long string read.", NON);
      /* NOTREACHED */
    }
  }
  if (strp < BUFSIZE) {
    strb[strp] = '\0';
  }
  else {
    error("Too long string read.", NON);
    /* NOTREACHED */
  }

  return copystring(strb, strp);
}

/* rcharacter -- °ìÊ¸»úÆÉ¤ó¤ÇÍè¤ë¡£ */

static list
rcharacter(void)
{
  char *tempbuf;
  unsigned ch;
  list retval;
  int bufp;

  tempbuf = (char *)malloc(longestkeywordlen + 1);
  if ( !tempbuf ) {
    fatal("read: (char *)malloc failed in reading character.", NON);
    /* NOTREACHED */
  }
  bufp = 0;

  ch = tyi();
  if (ch == '\\') {
    int code, res;

    do { /* ¥­¡¼¥ï¡¼¥É¤È¾È¹ç¤¹¤ë */
      tempbuf[bufp++] = ch = tyi();
      res = identifySequence(ch, &code);
    } while (res == CONTINUE);
    if (code != -1) { /* ¥­¡¼¥ï¡¼¥É¤È°ìÃ×¤·¤¿¡£ */
      retval = mknum(code);
    }
    else if (bufp > 2 && tempbuf[0] == 'C' && tempbuf[1] == '-') {
      while (bufp > 3) {
	untyi(tempbuf[--bufp]);
      }
      retval = mknum(tempbuf[2] & (' ' - 1));
    }
    else if (bufp == 3 && tempbuf[0] == 'F' && tempbuf[1] == '1') {
      untyi(tempbuf[2]);
      retval = mknum(CANNA_KEY_F1);
    }
    else if (bufp == 4 && tempbuf[0] == 'P' && tempbuf[1] == 'f' &&
	     tempbuf[2] == '1') {
      untyi(tempbuf[3]);
      retval = mknum(CANNA_KEY_PF1);
    }
    else { /* Á´Á³ÂÌÌÜ */
      while (bufp > 1) {
	untyi(tempbuf[--bufp]);
      }
      ch = (unsigned)(unsigned char)tempbuf[0];
      goto return_char;
    }
  }
  else {
  return_char:
    if (ch == 0x8f) { /* SS3 */
      ch <<= 8;
      ch += tyi();
      goto shift_more;
    }
    else if (ch & 0x80) { /* ¤¦¡Á¤ó¡¢ÆüËÜ¸ì¤Ë°ÍÂ¸¤·¤Æ¤¤¤ë */
    shift_more:
      ch <<= 8;
      ch += tyi();
    }
    retval = mknum(ch);
  }

  free(tempbuf);
  return retval;
}

static int
isnum(char *name)
{
	if (*name == '-') {
		name++;
		if ( !*name )
			return(NO);
	}
	for(; *name ; name++) {
		if (*name < '0' || '9' < *name) {
			if (*name != '.' || *(name + 1)) {
				return(NO);
			}
		}
	}
	return(YES);
}

/* tyi -- input one character from buffered stream	*/

static void
untyi(int c)
{
  if (readbuf < readptr) {
    *--readptr = c;
  }
  else {
    if (untyip >= untyisize) {
      if (untyisize == 0) {
	untyibuf = (char *)malloc(UNTYIUNIT);
	if (untyibuf) {
	  untyisize = UNTYIUNIT;
	}
      }else{
	untyibuf = (char *)realloc(untyibuf, UNTYIUNIT + untyisize);
	if (untyibuf) {
	  untyisize += UNTYIUNIT;
	}
      }
    }
    if (untyip < untyisize) { /* ¤½¤ì¤Ç¤â¥Á¥§¥Ã¥¯¤¹¤ë */
      untyibuf[untyip++] = c;
    }
  }
}

static int
tyi(void)
{
  if (untyibuf) {
    int ret = untyibuf[--untyip];
    if (untyip == 0) {
      free(untyibuf);
      untyibuf = (char *)0;
      untyisize = 0;
    }
    return ret;
  }

  if (readptr && *readptr) {
    return ((int)(unsigned char)*readptr++);
  }
  else if (!files[filep].f) {
    return NO;
  }
  else if (files[filep].f == stdin) {
    readptr = fgets(readbuf, BUFSIZE, stdin);
    files[filep].line++;
    if ( !readptr ) {
      return NO;
    }
    else {
      return tyi();
    }
  }
  else {
    readptr = fgets(readbuf,BUFSIZE,files[filep].f);
    files[filep].line++;
    if (readptr) {
      return(tyi());
    }
    else {
      return(NO);
    }
  }
  /* NOTREACHED */
}

/* tyipeek -- input one character without advance the read pointer	*/

static int
tyipeek(void)
{
  int c = tyi();
  untyi(c);
  return c;
}

	

/* prins -
	print string	*/

static void prins(char *s)
{
	while (*s) {
		tyo(*s++);
	}
}


/* isterm -
	check if the character is terminating the lisp expression	*/

static int isterm(int c)
{
	if (c <= ' ')
		return(YES);
	else {
		switch (c)
		{
		case '(':
		case ')':
		case ';':
			return(YES);
		default:
			return(NO);
		}
	}
}

/* push down an S-expression to parameter stack	*/

static void
push(list value)
{
  if (sp <= stack) {
    error("Stack over flow",NON);
    /* NOTREACHED */
  }
  else
    *--sp = value;
}

/* pop up n S-expressions from parameter stack	*/

static void 
pop(int x)
{
  if (0 < x && sp >= &stack[STKSIZE]) {
    error("Stack under flow",NON);
    /* NOTREACHED */
  }
  sp += x;
}

/* pop up an S-expression from parameter stack	*/

static list 
pop1(void)
{
  if (sp >= &stack[STKSIZE]) {
    error("Stack under flow",NON);
    /* NOTREACHED */
  }
  return(*sp++);
}

static void
epush(list value)
{
  if (esp <= estack) {
    error("Estack over flow",NON);
    /* NOTREACHED */
  }
  else
    *--esp = value;
}

static list 
epop(void)
{
  if (esp >= &estack[STKSIZE]) {
    error("Lstack under flow",NON);
    /* NOTREACHED */
  }
  return(*esp++);
}


/*
** output function for lisp S-Expression
*/


/*
**  print atom function
**  please make sure it is an atom (not list)
**  no check is done here.
*/

static void
patom(list atm)
{
  char namebuf[BUFSIZE];

  if (constp(atm)) {
    if (numberp(atm)) {
      (void)sprintf(namebuf,"%d",xnum(atm));
      prins(namebuf);
    }
    else {		/* this is a string */
      int i, len = xstrlen(atm);
      char *s = xstring(atm);

      tyo('"');
      for (i = 0 ; i < len ; i++) {
	tyo(s[i]);
      }
      tyo('"');
    }
  }
  else {
    prins(symbolpointer(atm)->pname);
  }
}

static char *oldcelltop;
static char *oldcellp;

#define oldpointer(x) (oldcelltop + celloffset(x))

static void
gc(void) /* ¥³¥Ô¡¼Êý¼°¤Î¥¬¡¼¥Ù¥¸¥³¥ì¥¯¥·¥ç¥ó¤Ç¤¢¤ë */
{
  int i;
  list *p;
  static int under_gc = 0;

  if (under_gc) {
    fatal("GC: memory exhausted.", NON);
  }
  else {
    under_gc = 1;
  }

  oldcellp = memtop; oldcelltop = celltop;

  if ( !alloccell() ) {
    fatal("GC: failed in allocating new cell area.", NON);
    /* NOTREACHED */
  }

  for (i = 0 ; i < BUFSIZE ; i++) {
    markcopycell(oblist + i);
  }
  for (p = sp ; p < &stack[STKSIZE] ; p++) {
    markcopycell(p);
  }
  for (p = esp ; p < &estack[STKSIZE] ; p++) {
    markcopycell(p);
  }
  for (i = 0 ; i < valuec ; i++) {
    markcopycell(values + i);
  }
  markcopycell(&T);
  markcopycell(&QUOTE);
  markcopycell(&_LAMBDA);
  markcopycell(&_MACRO);
  markcopycell(&COND);
  markcopycell(&USER);
  markcopycell(&BUSHU);
  markcopycell(&GRAMMAR);
  markcopycell(&RENGO);
  markcopycell(&KATAKANA);
  markcopycell(&HIRAGANA);
  markcopycell(&HYPHEN);
  free(oldcellp);
  if ((freecell - celltop) * 2 > cellbtm -celltop) {
    ncells = (freecell - celltop) * 2 / sizeof(list);
  }
  under_gc = 0;
}

static char *Strncpy();

static list
allocstring(int n)
{
  int namesize;
  list retval;

  namesize = ((n + (sizeof(pointerint)) + 1 + 3)/ sizeof(list)) * sizeof(list);
  if (freecell + namesize >= cellbtm) { /* gc Ãæ¤Ïµ¯¤³¤êÆÀ¤Ê¤¤¤Ï¤º */
    gc();
  }
  ((struct stringcell *)freecell)->length = n;
  retval = STRING_TAG | (freecell - celltop);
  freecell += namesize;
  return retval;
}

static list
copystring(char *s, int n)
{
  list retval;

  retval = allocstring(n);
  (void)Strncpy(xstring(retval), s, n);
  xstring(retval)[n] = '\0';
  return retval;
}

static list
copycons(struct cell *l)
{
  list newcell;

  newcell = newcons();
  car(newcell) = l->head;
  cdr(newcell) = l->tail;
  return newcell;
}

static void
markcopycell(list *addr)
{
  list temp;
 redo:
  if (null(*addr) || numberp(*addr)) {
    return;
  }
  else if (alreadycopied(oldpointer(*addr))) {
    *addr = newaddr(gcfield(oldpointer(*addr)));
    return;
  }
  else if (stringp(*addr)) {
    temp = copystring(((struct stringcell *)oldpointer(*addr))->str,
		      ((struct stringcell *)oldpointer(*addr))->length);
    gcfield(oldpointer(*addr)) = mkcopied(temp);
    *addr = temp;
    return;
  }
  else if (consp(*addr)) {
    temp = copycons((struct cell *)(oldpointer(*addr)));
    gcfield(oldpointer(*addr)) = mkcopied(temp);
    *addr = temp;
    markcopycell(&car(temp));
    addr = &cdr(temp);
    goto redo;
  }
  else { /* symbol */
    struct atomcell *newatom, *oldatom;

    oldatom = (struct atomcell *)(oldpointer(*addr));
    temp = newsymbol(oldatom->pname);
    newatom = symbolpointer(temp);
    newatom->value = oldatom->value;
    newatom->plist = oldatom->plist;
    newatom->ftype = oldatom->ftype;
    newatom->func  = oldatom->func;
    newatom->fid   = oldatom->fid;
    newatom->mid   = oldatom->mid;
    newatom->valfunc  = oldatom->valfunc;
    newatom->hlink = oldatom->hlink;

    gcfield(oldpointer(*addr)) = mkcopied(temp);
    *addr = temp;

    if (newatom->value != (list)UNBOUND) {
      markcopycell(&newatom->value);
    }
    markcopycell(&newatom->plist);
    if (newatom->ftype == EXPR || newatom->ftype == MACRO) {
      markcopycell((list *)&newatom->func);
    }
    addr = &newatom->hlink;
    goto redo;
  }
}

static list
bindall(list var, list par, list a, list e)
{
  list *pa, *pe, retval;

  push(a); pa = sp;
  push(e); pe = sp;
 retry:
  if (constp(var)) {
    pop(2);
    return(*pa);
  }
  else if (atom(var)) {
    push(var);
    push(par);
    push(Lcons(2));
    push(*pa);
    retval = Lcons(2);
    pop(2);
    return retval;
  }
  else if (atom(par)) {
    error("Bad macro form ",e);
    /* NOTREACHED */
  }
  push(par);
  push(var);
  *pa = bindall(car(var),car(par),*pa,*pe);
  var = cdr(pop1());
  par = cdr(pop1());
  goto retry;
  /* NOTREACHED */
}

static list
Lquote(void)
{
	list p;

	p = pop1();
	if (atom(p))
		return(NIL);
	else
		return(car(p));
}

static list
Leval(int n)
{
  list e, t, s, tmp, aa, *pe, *pt, *ps, *paa;
  list fn, (*cfn)(...), *pfn;
  int i, j;
  argnchk("eval",1);
  e = sp[0];
  pe = sp;
  if (atom(e)) {
    if (constp(e)) {
      pop1();
      return(e);
    }
    else {
      struct atomcell *sym;

      t = assq(e, *esp);
      if (t) {
	(void)pop1();
	return(cdr(t));
      }
      else if ((sym = symbolpointer(e))->valfunc) {
	(void)pop1();
	return (sym->valfunc)(VALGET, 0);
      }else{
	if ((t = (sym->value)) != (list)UNBOUND) {
	  pop1();
	  return(t);
	}
	else {
	  error("Unbound variable: ",*pe);
	  /* NOTREACHED */
	}
      }
    }
  }
  else if (constp((fn = car(e)))) {	/* not atom	*/
    error("eval: undefined function ", fn);
    /* NOTREACHED */
  }
  else if (atom(fn)) {
    switch (symbolpointer(fn)->ftype) {
    case UNDEF:
      error("eval: undefined function ", fn);
      /* NOTREACHED */
      break;
    case SUBR:
      cfn = symbolpointer(fn)->func;
      i = evpsh(cdr(e));
      epush(NIL);
      t = (*cfn)(i);
      epop();
      pop1();
      return (t);
    case SPECIAL:
      push(cdr(e));
      t = (*(symbolpointer(fn)->func))();
      pop1();
      return (t);
    case EXPR:
      fn = (list)(symbolpointer(fn)->func);
      aa = NIL; /* previous env won't be used */
    expr:
      if (atom(fn) || car(fn) != _LAMBDA || atom(cdr(fn))) {
	error("eval: bad lambda form ", fn);
	/* NOTREACHED */
      }
/* Lambda binding begins here ...					*/
      s = cdr(e);		/* actual parameter	*/
      t = cadr(fn);		/* lambda list		*/
      push(s); ps = sp;
      push(t); pt = sp;
      push(fn); pfn = sp;
      push(aa); paa = sp;
      i = 0;			/* count of variables	*/
      for (; consp(*ps) && consp(*pt) ; *ps = cdr(*ps), *pt = cdr(*pt)) {
	if (consp(car(*pt))) {
	  tmp = cdar(*pt);	/* push the cdr of element */
	  if (!(atom(tmp) || null(cdr(tmp)))) {
	    push(cdr(tmp));
	    push(T);
	    push(Lcons(2));
	    i++;
	  }
	  push(caar(*pt));
	}
	else {
	  push(car(*pt));
	}
	push(car(*ps));
	push(Leval(1));
	push(Lcons(2));
	i++;
      }
      for (; consp(*pt) ; *pt = cdr(*pt)) {
	if (atom(car(*pt))) {
	  error("Too few actual parameters ",*pe);
	  /* NOTREACHED */
	}
	else {
	  tmp = cdar(*pt);
	  if (!(atom(tmp) || null(cdr(tmp)))) {
	    push(cdr(tmp));
	    push(NIL);
	    push(Lcons(2));
	    i++;
	  }
	  push(caar(*pt));
	  tmp = cdar(*pt); /* restore for GC */
	  if (atom(tmp))
	    push(NIL);
	  else {
	    push(car(tmp));
	    push(Leval(1));
	  }
	  push(Lcons(2));
	  i++;
	}
      }
      if (null(*pt) && consp(*ps)) {
	error("Too many actual arguments ",*pe);
	/* NOTREACHED */
      }
      else if (*pt) {
	push(*pt);
	for (j = 1 ; consp(*ps) ; j++) {
	  push(car(*ps));
	  push(Leval(1));
	  *ps = cdr(*ps);
	}
	push(NIL);
	for (; j ; j--) {
	  push(Lcons(2));
	}
	i++;
      }
      push(*paa);
      for (; i ; i--) {
	push(Lcons(2));
      }
/* Lambda binding finished, and a new environment is established.	*/
      epush(pop1());	/* set the new environment	*/
      push(cddr(*pfn));
      t = Lprogn();
      epop();
      pop(5);
      return (t);
    case MACRO:
      fn = (list)(symbolpointer(fn)->func);
      if (atom(fn) || car(fn) != _MACRO || atom(cdr(fn))) {
	error("eval: bad macro form ",fn);
	/* NOTREACHED */
      }
      s = cdr(e);	/* actual parameter	*/
      t = cadr(fn);	/* lambda list	*/
      push(fn);
      epush(bindall(t,s,NIL,e));
      push(cddr(pop1()));
      t = Lprogn();
      epop();
      push(t);
      push(t);
      s = Leval(1);
      t = pop1();
      if (!atom(t)) {
	car(*pe) = car(t);
	cdr(*pe) = cdr(t);
      }
      pop1();
      return (s);
    case CMACRO:
      push(e);
      push(t = (*(symbolpointer(fn)->func))());
      push(t);
      s = Leval(1);
      t = pop1();
      if (!atom(t)) {
	car(e) = car(t);
	cdr(e) = cdr(t);
      }
      pop1();
      return (s);
    default:
      error("eval: unrecognized ftype used in ", fn);
      /* NOTREACHED */
      break;
    }
    /* NOTREACHED */
  }
  else {	/* fn is list (lambda expression)	*/
    aa = *esp; /* previous environment is also used */
    goto expr;
  }
  /* maybe NOTREACHED */
  return NIL;
}

static list
assq(list e, list a)
{
  list i;

  for (i = a ; i ; i = cdr(i)) {
    if (consp(car(i)) && e == caar(i)) {
      return(car(i));
    }
  }
  return((list)NIL);
}

/* eval each argument and push down each value to parameter stack	*/

static int
evpsh(list args)
{
  int  counter;
  list temp;

  counter = 0;
  while (consp(args)) {
    push(args);
    push(car(args));
    temp = Leval(1);
    args = cdr(pop1());
    counter++;
    push(temp);
  }
  return (counter);
}

/*
static int
psh(args)
list args;
{
  int  counter;

  counter = 0;
  while (consp(args)) {
    push(car(args));
    counter++;
    args = cdr(args);
  }
  return (counter);
}
*/

static list
Lprogn(void)
{
  list val, *pf;

  val = NIL;
  pf = sp;
  for (; consp(*pf) ; *pf = cdr(*pf)) {
    symbolpointer(T)->value = T;
    push(car(*pf));
    val = Leval(1);
  }
  pop1();
  return (val);
}

static list
Lcons(int n)
{
	list temp;

	argnchk("cons",2);
	temp = newcons();
	cdr(temp) = pop1();
	car(temp) = pop1();
	return(temp);
}

static list 
Lncons(int n)
{
	list temp;

	argnchk("ncons",1);
	temp = newcons();
	car(temp) = pop1();
	cdr(temp) = NIL;
	return(temp);
}

static list
Lxcons(int n)
{
	list temp;

	argnchk("cons",2);
	temp = newcons();
	car(temp) = pop1();
	cdr(temp) = pop1();
	return(temp);
}

static list 
Lprint(int n)
{
	print(sp[0]);
	pop(n);
	return (T);
}

static list
Lset(int n)
{
  list val, t;
  list var;
  struct atomcell *sym;

  argnchk("set",2);
  val = pop1();
  var = pop1();
  if (!symbolp(var)) {
    error("set/setq: bad variable type  ",var);
    /* NOTREACHED */
  }
  sym = symbolpointer(var);
  t = assq(var,*esp);
  if (t) {
    return cdr(t) = val;
  }
  else if (sym->valfunc) {
    return (*(sym->valfunc))(VALSET, val);
  }
  else {
    return sym->value = val;	/* global set	*/
  }
}

static list
Lsetq(void)
{
  list a, *pp;

  a = NIL;
  for (pp = sp; consp(*pp) ; *pp = cdr(*pp)) {
    push(car(*pp));
    *pp = cdr(*pp);
    if ( atom(*pp) ) {
      error("Odd number of args to setq",NON);
      /* NOTREACHED */
    }
    push(car(*pp));
    push(Leval(1));
    a = Lset(2);
  }
  pop1();
  return(a);
}

static int equal();

static list 
Lequal(int n)
{
  argnchk("equal (=)",2);
  if (equal(pop1(),pop1()))
    return(T);
  else
    return(NIL);
}

/* null Ê¸»ú¤Ç½ª¤ï¤é¤Ê¤¤ strncmp */

static int
Strncmp(char *x, char *y, int len)
{
  int i;

  for (i = 0 ; i < len ; i++) {
    if (x[i] != y[i]) {
      return (x[i] - y[i]);
    }
  }
  return 0;
}

/* null Ê¸»ú¤Ç½ª¤ï¤é¤Ê¤¤ strncpy */

static char *
Strncpy(char *x, char *y, int len)
{
  int i;

  for (i = 0 ; i < len ; i++) {
    x[i] = y[i];
  }
  return x;
}

static int
equal(list x, list y)
{
 equaltop:
  if (x == y)
    return(YES);
  else if (null(x) || null(y))
    return(NO);
  else if (numberp(x) || numberp(y)) {
    return NO;
  }
  else if (stringp(x)) {
    if (stringp(y)) {
      return ((xstrlen(x) == xstrlen(y)) ?
	      (!Strncmp(xstring(x), xstring(y), xstrlen(x))) : 0);
    }
    else {
      return NO;
    }
  }
  else if (symbolp(x) || symbolp(y)) {
    return(NO);
  }
  else {
    if (equal(car(x), car(y))) {
      x = cdr(x);
      y = cdr(y);
      goto equaltop;
    }
    else 
      return(NO);
  }
}

static list 
Lgreaterp(int n)
{
  list p;
  pointerint x, y;

  if ( !n )
    return(T);
  else {
    p = pop1();
    if (!numberp(p)) {
      numerr("greaterp",p);
      /* NOTREACHED */
    }
    x = xnum(p);
    for (n-- ; n ; n--) {
      p = pop1();
      if (!numberp(p)) {
	numerr("greaterp",p);
	/* NOTREACHED */
      }
      y = xnum(p);
      if (y <= x)		/* !(y > x)	*/
	return(NIL);
      x = y;
    }
    return(T);
  }
}

static list 
Llessp(int n)
{
  list p;
  pointerint x, y;

  if ( !n )
    return(T);
  else {
    p = pop1();
    if (!numberp(p)) {
      numerr("lessp",p);
      /* NOTREACHED */
    }
    x = xnum(p);
    for (n-- ; n ; n--) {
      p = pop1();
      if (!numberp(p)) {
	numerr("lessp",p);
	/* NOTREACHED */
      }
      y = xnum(p);
      if (y >= x)		/* !(y < x)	*/
	return(NIL);
      x = y;
    }
    return(T);
  }
}

static list
Leq(int n)
{
  list f;

  argnchk("eq",2);
  f = pop1();
  if (f == pop1())
    return(T);
  else
    return(NIL);
}

static list
Lcond(void)
{
  list *pp, t, a, c;

  pp = sp;
  for (; consp(*pp) ; *pp = cdr(*pp)) {
    t = car(*pp);
    if (atom(t)) {
      pop1();
      return (NIL);
    }
    else {
      push(cdr(t));
      if ((c = car(t)) == T || (push(c), (a = Leval(1)))) {
	/* if non NIL */
	t = pop1();
	if (null(t)) {	/* if cdr is NIL */
	  (void)pop1();
	  return (a);
	}
	else {
	  (void)pop1();
	  push(t);
	  return(Lprogn());
	}
      }else{
	(void)pop1();
      }
    }
  }
  pop1();
  return (NIL);
}

static list
Lnull(int n)
{
  argnchk("null",1);
  if (pop1())
    return NIL;
  else
    return T;
}

static list 
Lor(void)
{
  list *pp, t;

  for (pp = sp; consp(*pp) ; *pp = cdr(*pp)) {
    push(car(*pp));
    t = Leval(1);
    if (t) {
      pop1();
      return(t);
    }
  }
  pop1();
  return(NIL);
}

static list 
Land(void)
{
  list *pp, t;

  t = T;
  for (pp = sp; consp(*pp) ; *pp = cdr(*pp)) {
    push(car(*pp));
    if ( !(t = Leval(1)) ) {
      pop1();
      return(NIL);
    }
  }
  pop1();
  return(t);
}

static list 
Lplus(int n)
{
  list t;
  int  i;
  pointerint sum;

  i = n;
  sum = 0;
  while (i--) {
    t = sp[i];
    if ( !numberp(t) ) {
      numerr("+",t);
      /* NOTREACHED */
    }
    else {
      sum += xnum(t);
    }
  }
  pop(n);
  return(mknum(sum));
}

static list
Ltimes(int n)
{
  list t;
  int  i;
  pointerint sum;

  i = n;
  sum = 1;
  while (i--) {
    t = sp[i];
    if ( !numberp(t) ) {
      numerr("*",t);
      /* NOTREACHED */
    }
    else
      sum *= xnum(t);
  }
  pop(n);
  return(mknum(sum));
}

static list
Ldiff(int n)
{
  list t;
  int  i;
  pointerint sum;

  if ( !n )
    return(mknum(0));
  t = sp[n - 1];
  if ( !numberp(t) ) {
    numerr("-",t);
    /* NOTREACHED */
  }
  sum = xnum(t);
  if (n == 1) {
    pop1();
    return(mknum(-sum));
  }
  else {
    i = n - 1;
    while (i--) {
      t = sp[i];
      if ( !numberp(t) ) {
	numerr("-",t);
	/* NOTREACHED */
      }
      else
	sum -= xnum(t);
    }
    pop(n);
    return(mknum(sum));
  }
}

static list 
Lquo(int n)
{
  list t;
  int  i;
  pointerint sum;

  if ( !n )
    return(mknum(1));
  t = sp[n - 1];
  if ( !numberp(t) ) {
    numerr("/",t);
    /* NOTREACHED */
  }
  sum = xnum(t);
  i = n - 1;
  while (i--) {
    t = sp[i];
    if ( !numberp(t) ) {
      numerr("/",t);
      /* NOTREACHED */
    }
    else if (xnum(t) != 0) {
      sum = sum / (pointerint)xnum(t); /* CP/M68K is bad...	*/
    }
    else { /* division by zero */
      error("Division by zero",NON);
    }
  }
  pop(n);
  return(mknum(sum));
}

static list 
Lrem(int n)
{
  list t;
  int  i;
  pointerint sum;

  if ( !n )
    return(mknum(0));
  t = sp[n - 1];
  if ( !numberp(t) ) {
    numerr("%",t);
    /* NOTREACHED */
  }
  sum = xnum(t);
  i = n - 1;
  while (i--) {
    t = sp[i];
    if ( !numberp(t) ) {
      numerr("%",t);
      /* NOTREACHED */
    }
    else if (xnum(t) != 0) {
      sum = sum % (pointerint)xnum(t); /* CP/M68K is bad ..	*/
    }
    else { /* division by zero */
      error("Division by zero",NON);
    }
  }
  pop(n);
  return(mknum(sum));
}

/*
 *	Garbage Collection
 */

static list 
Lgc(int n)
{
  argnchk("gc",0);
  gc();
  return(NIL);
}

static list
Lusedic(int n)
{
  int i;
  list retval = NIL, temp;
  int dictype;
#ifndef WIN_CANLISP
  extern struct dicname *kanjidicnames;
  struct dicname *kanjidicname;
  extern int auto_define;
#endif

  for (i = n ; i ; i--) {
    temp = sp[i - 1];
    dictype = DIC_PLAIN;
    if (symbolp(temp) && i - 1 > 0) {
      if (temp == USER) {
	dictype = DIC_USER;
      }
      else if (temp == BUSHU) {
	dictype = DIC_BUSHU;
      }
      else if (temp == GRAMMAR) {
	dictype = DIC_GRAMMAR;
      }
      else if (temp == RENGO) {
	dictype = DIC_RENGO;
      }
      else if (temp == KATAKANA) {
	dictype = DIC_KATAKANA;
#ifndef WIN_CANLISP
        auto_define = 1;
#endif
      }
      else if (temp == HIRAGANA) {
	dictype = DIC_HIRAGANA;
#if defined(HIRAGANAAUTO) && defined(WIN_CANLISP)
        auto_define = 1;
#endif
      }
      i--; temp = sp[i - 1];
    }
    if (stringp(temp)) {
#ifndef WIN_CANLISP
      kanjidicname  = (struct dicname *)malloc(sizeof(struct dicname));
      if (kanjidicname) {
	kanjidicname->name = (char *)malloc(strlen(xstring(temp)) + 1);
	if (kanjidicname->name) {
	  strcpy(kanjidicname->name , xstring(temp));
	  kanjidicname->dictype = dictype;
	  kanjidicname->dicflag = DIC_NOT_MOUNTED;
	  kanjidicname->next = kanjidicnames;
	  kanjidicnames = kanjidicname;
	  retval = T;
	  continue;
	}
	free(kanjidicname);
      }
#else /* if WIN_CANLISP */
      if (wins.conf && wins.conf->dicfn) {
	(*wins.conf->dicfn)(xstring(temp), dictype, wins.context);
      }
#endif /* WIN_CANLISP */
    }
  }
  pop(n);
  return retval;
}

static list
Llist(int n)
{
	push(NIL);
	for (; n ; n--) {
		push(Lcons(2));
	}
	return (pop1());
}

static list
Lcopysym(int n)
{
  list src, dst;
  struct atomcell *dsta, *srca;

  argnchk("copy-symbol",2);
  src = pop1();
  dst = pop1();
  if (!symbolp(dst)) {
    error("copy-symbol: bad arg  ", dst);
    /* NOTREACHED */
  }
  if (!symbolp(src)) {
    error("copy-symbol: bad arg  ", src);
    /* NOTREACHED */
  }
  dsta = symbolpointer(dst);
  srca = symbolpointer(src);
  dsta->plist   = srca->plist;
  dsta->value   = srca->value;
  dsta->ftype   = srca->ftype;
  dsta->func    = srca->func;
  dsta->valfunc = srca->valfunc;
  dsta->mid     = srca->mid;
  dsta->fid     = srca->fid;
  return src;
}

static list
Lload(int n)
{
  list p, t;
  FILE *instream;

  argnchk("load",1);
  p = pop1();
  if ( !stringp(p) ) {
    error("load: illegal file name  ",p);
    /* NOTREACHED */
  }
  if ((instream = fopen(xstring(p), "r")) == (FILE *)NULL) {
    error("load: file not found  ",p);
    /* NOTREACHED */
  }
  prins("[load ");
  print(p);
  prins("]\n");

  if (jmpenvp <= 0) { /* ºÆµ¢¤¬¿¼¤¹¤®¤ë¾ì¹ç */
    return NIL;
  }
  jmpenvp--;
  files[++filep].f = instream;
  files[filep].name = (char *)malloc(xstrlen(p) + 1);
  if (files[filep].name) {
    strcpy(files[filep].name, xstring(p));
  }
  files[filep].line = 0;

  setjmp(env[jmpenvp].jmp_env);
  env[jmpenvp].base_stack = sp - stack;
  env[jmpenvp].base_estack = esp - estack;

  for (;;) {
    t = Lread(0);
    if (valuec > 1 && null(values[1])) {
      break;
    }
    else {
      push(t);
      Leval(1);
    }
  }
  jmpenvp++;
  return(T);
}

static list
Lmodestr(int n)
{
  list p;
  int mode;

  argnchk(S_SetModeDisp, 2);
  if ( !null(p = sp[0]) && !stringp(p) ) {
    lisp_strerr(S_SetModeDisp, p);
    /* NOTREACHED */
  }
  if (!symbolp(sp[1]) || (mode = symbolpointer(sp[1])->mid) == -1) {
    error("Illegal mode ", sp[1]);
    /* NOTREACHED */
  }
#ifndef WIN_CANLISP
  changeModeName(mode, null(p) ? 0 : xstring(p));
#endif
  pop(2);
  return p;
}

/* µ¡Ç½¥·¡¼¥±¥ó¥¹¤Î¼è¤ê½Ð¤· */

static int
xfseq(char *fname, list l, unsigned char *arr, int arrsize)
{
  int i;

  if (atom(l)) {
    if (symbolp(l) &&
	(arr[0] = (unsigned char)(symbolpointer(l)->fid)) != 255) {
      arr[1] = 0;
    }
    else {
      prins(fname);
      error(": illegal function ", l);
      /* NOTREACHED */
    }
    return 1;
  }
  else {
    for (i = 0 ; i < arrsize - 1 && consp(l) ; i++, l = cdr(l)) {
      list temp = car(l);

      if (!symbolp(temp) ||
	  (arr[i] = (unsigned char)(symbolpointer(temp)->fid)) == 255) {
	prins(fname);
	error(": illegal function ", temp);
	/* NOTREACHED */
      }
    }
    arr[i] = 0;
    return i;
  }
}

static list
Lsetkey(int n)
{
  list p;
  int mode, slen;
  unsigned char fseq[256];
  unsigned char keyseq[256];
#ifndef WIN_CANLISP
  int retval;
#endif

  argnchk(S_SetKey, 3);
  if ( !stringp(p = sp[1]) ) {
    lisp_strerr(S_SetKey, p);
    /* NOTREACHED */
  }
  if (!symbolp(sp[2]) || (mode = symbolpointer(sp[2])->mid) < 0 ||
      (CANNA_MODE_MAX_REAL_MODE <= mode &&
       mode < CANNA_MODE_MAX_IMAGINARY_MODE &&
       mode != CANNA_MODE_HenkanNyuryokuMode)) {
    error("Illegal mode for set-key ", sp[2]);
    /* NOTREACHED */
  }
  if (xfseq(S_SetKey, sp[0], fseq, 256)) {
    slen = xstrlen(p);
    Strncpy((char *)keyseq, xstring(p), slen);
    keyseq[slen] = 255;
#ifndef WIN_CANLISP
    retval = changeKeyfunc(mode, (unsigned)keyseq[0],
		           slen > 1 ? CANNA_FN_UseOtherKeymap :
                           (fseq[1] != 0 ? CANNA_FN_FuncSequence : fseq[0]),
                           fseq, keyseq);
    if (retval == NG) {
      error("Insufficient memory.", NON);
      /* NOTREACHED */
    }
#else
    if (wins.conf && wins.conf->keyfn) {
      (*wins.conf->keyfn)(mode, keyseq, slen, fseq, strlen(fseq),
			  wins.context);
    }
#endif
  }
  pop(3);
  return p;
}

static list
Lgsetkey(int n)
{
  list p;
  int slen;
  unsigned char fseq[256];
  unsigned char keyseq[256];
#ifndef WIN_CANLISP
  int retval;
#endif

  argnchk(S_GSetKey, 2);
  if ( !stringp(p = sp[1]) ) {
    lisp_strerr(S_GSetKey, p);
    /* NOTREACHED */
  }
  if (xfseq(S_GSetKey, sp[0], fseq, 256)) {
    slen = xstrlen(p);
    Strncpy((char *)keyseq, xstring(p), slen);
    keyseq[slen] = 255;
#ifndef WIN_CANLISP
    retval = changeKeyfuncOfAll((unsigned)keyseq[0],
               slen > 1 ? CANNA_FN_UseOtherKeymap :
               (fseq[1] != 0 ? CANNA_FN_FuncSequence : fseq[0]),
               fseq, keyseq);
    if (retval == NG) {
      error("Insufficient memory.", NON);
      /* NOTREACHED */
    }
#else /* if WIN_CANLISP */
    if (wins.conf && wins.conf->keyfn) {
      (*wins.conf->keyfn)(255, keyseq, slen, fseq, strlen(fseq), wins.context);
    }
#endif /* WIN_CANLISP */
    pop(2);
    return p;
  }
  else {
    pop(2);
    return NIL;
  }
}

static list
Lputd(int n)
{
  list body, a;
  list sym;
  struct atomcell *symp;

  argnchk("putd",2);
  a = body = pop1();
  sym = pop1();
  symp = symbolpointer(sym);
  if (constp(sym) || consp(sym)) {
    error("putd: function name must be a symbol : ",sym);
    /* NOTREACHED */
  }
  if (null(body)) {
    symp->ftype = UNDEF;
    symp->func = (list (*)(...))UNDEF;
  }
  else if (consp(body)) {
    if (car(body) == _MACRO) {
      symp->ftype = MACRO;
      symp->func = (list (*)(...))body;
    }
    else {
      symp->ftype = EXPR;
      symp->func = (list (*)(...))body;
    }
  }
  return(a);
}

static list
Ldefun(void)
{
  list form, res;

  form = sp[0];
  if (atom(form)) {
    error("defun: illegal form ",form);
    /* NOTREACHED */
  }
  push(car(form));
  push(_LAMBDA);
  push(cdr(form));
  push(Lcons(2));
  Lputd(2);
  res = car(pop1());
  return (res);
}

static list
Ldefmacro(void)
{
  list form, res;

  form = sp[0];
  if (atom(form)) {
    error("defmacro: illegal form ",form);
    /* NOTREACHED */
  }
  push(res = car(form));
  push(_MACRO);
  push(cdr(form));
  push(Lcons(2));
  Lputd(2);
  pop1();
  return (res);
}

static list
Lcar(int n)
{
  list f;

  argnchk("car",1);
  f = pop1();
  if (!f)
    return(NIL);
  else if (atom(f)) {
    error("Bad arg to car ",f);
    /* NOTREACHED */
  }
  return(car(f));
}

static list
Lcdr(int n)
{
  list f;

  argnchk("cdr",1);
  f = pop1();
  if (!f)
    return(NIL);
  else if (atom(f)) {
    error("Bad arg to cdr ",f);
    /* NOTREACHED */
  }
  return(cdr(f));
}

static list
Latom(int n)
{
  list f;

  argnchk("atom",1);
  f = pop1();
  if (atom(f))
    return(T);
  else
    return(NIL);
}

static list
Llet(void)
{
  list lambda, args, p, *pp, *pq, *pl, *px;

  px = sp;
  *px = cdr(*px);
  if (atom(*px)) {
    (void)pop1();
    return(NIL);
  }
  else {
    push(NIL);
    args = Lncons(1);
    push(args); pq = sp;
    push(NIL);
    lambda = p = Lncons(1);
    push(lambda);

    push(p); pp = sp;
    push(*pq); pq = sp;
    push(NIL); pl = sp;
    for (*pl = car(*px) ; consp(*pl) ; *pl = cdr(*pl)) {
      if (atom(car(*pl))) {
	push(car(*pl));
	*pp = cdr(*pp) = Lncons(1);
	push(NIL);
	*pq = cdr(*pq) = Lncons(1);
      }
      else if (atom(cdar(*pl))) {
	push(caar(*pl));
	*pp = cdr(*pp) = Lncons(1);
	push(NIL);
	*pq = cdr(*pq) = Lncons(1);
      }else{
	push(caar(*pl));
	*pp = cdr(*pp) = Lncons(1);
	push(cadr(car(*pl)));
	*pq = cdr(*pq) = Lncons(1);
      }
    }
    pop(3);
    sp[0] = cdr(sp[0]);
    sp[1] = cdr(sp[1]);
    push(cdr(*px));
    push(Lcons(2));
    push(_LAMBDA);
    push(Lxcons(2));
    p = Lxcons(2);
    (void)pop1();
    return(p);
  }
}

/* (if con tr . falist) -> (cond (con tr) (t . falist))*/

static list
Lif(void)
{
  list x, *px, retval;

  x = cdr(sp[0]);
  if (atom(x) || atom(cdr(x))) {
    (void)pop1();
    return NIL;
  }
  else {
    push(x); px = sp;

    push(COND);

    push(car(x));
    push(cadr(x));
    push(Llist(2));

    push(T);
    push(cddr(*px));
    push(Lcons(2));

    retval = Llist(3);
    pop(2);
    return retval;
  }
}

static list
Lunbindkey(int n)
{
  unsigned char fseq[2];
  static unsigned char keyseq[2] = {(unsigned char)CANNA_KEY_Undefine,
				      (unsigned char)255};
  int mode;
  list retval;

  argnchk(S_UnbindKey, 2);
  if (!symbolp(sp[1]) || (mode = symbolpointer(sp[1])->mid) == -1) {
    error("Illegal mode ", sp[1]);
    /* NOTREACHED */
  }
  if (xfseq(S_UnbindKey, sp[0], fseq, 2)) {
#ifndef WIN_CANLISP
    int ret;
    ret = changeKeyfunc(mode, CANNA_KEY_Undefine,
                        fseq[1] != 0 ? CANNA_FN_FuncSequence : fseq[0],
                        fseq, keyseq);
    if (ret == NG) {
      error("Insufficient memory.", NON);
      /* NOTREACHED */
    }
#else /* if WIN_CANLISP */
    if (wins.conf && wins.conf->keyfn) {
      (*wins.conf->keyfn)(mode, keyseq, 1, fseq, 1, wins.context);
    }
#endif /* WIN_CANLISP */
    retval = T;
  }
  else {
    retval = NIL;
  }
  pop(2);
  return retval;
}

static list
Lgunbindkey(int n)
{
  unsigned char fseq[2];
  static unsigned char keyseq[2] = {(unsigned char)CANNA_KEY_Undefine,
				      (unsigned char)255};
  list retval;

  argnchk(S_GUnbindKey, 1);
  if (xfseq(S_GUnbindKey, sp[0], fseq, 2)) {
#ifndef WIN_CANLISP
    int ret;
    ret = changeKeyfuncOfAll(CANNA_KEY_Undefine,
		       fseq[1] != 0 ? CANNA_FN_FuncSequence : fseq[0],
		       fseq, keyseq);
    if (ret == NG) {
      error("Insufficient memory.", NON);
      /* NOTREACHED */
    }
#else /* if WIN_CANLISP */
    if (wins.conf && wins.conf->keyfn) {
      (*wins.conf->keyfn)(255, keyseq, 1, fseq, 1, wins.context);
    }
#endif /* WIN_CANLISP */
    retval = T;
  }
  else {
    retval = NIL;
  }
  (void)pop1();
  return retval;
}

#define DEFMODE_MEMORY      0
#define DEFMODE_NOTSTRING   1
#define DEFMODE_ILLFUNCTION 2

static list
Ldefmode(void)
{
  list form, *sym, e, *p, fn, rd, md, us;
  extern extraFunc *extrafuncp;
  extern int nothermodes;
  extraFunc *extrafunc = (extraFunc *)0;
  int i, j;
#ifndef WIN_CANLISP
  int ecode;
  list l, edata;
#endif

  form = pop1();
  if (atom(form)) {
    error("Bad form ", form);
    /* NOTREACHED */
  }
  push(car(form));
  sym = sp;
  if (!symbolp(*sym)) {
    error("Symbol data expected ", *sym);
    /* NOTREACHED */
  }

  /* °ú¿ô¤ò¥×¥Ã¥·¥å¤¹¤ë */
  for (i = 0, e = cdr(form) ; i < 4 ; i++, e = cdr(e)) {
    if (atom(e)) {
      for (j = i ; j < 4 ; j++) {
	push(NIL);
      }
      break;
    }
    push(car(e));
  }
  if (consp(e)) {
    error("Bad form ", form);
    /* NOTREACHED */
  }

  /* É¾²Á¤¹¤ë */
  for (i = 0, p = sym - 1 ; i < 4 ; i++, p--) {
    push(*p);
    push(Leval(1));
  }
  us = pop1();
  fn = pop1();
  rd = pop1();
  md = pop1();
  pop(4);

#ifndef WIN_CANLISP
  ecode = DEFMODE_MEMORY;
  extrafunc = (extraFunc *)malloc(sizeof(extraFunc));
  if (extrafunc) {
    /* ¥·¥ó¥Ü¥ë¤Î´Ø¿ôÃÍ¤È¤·¤Æ¤ÎÄêµÁ */
    symbolpointer(*sym)->mid = CANNA_MODE_MAX_IMAGINARY_MODE + nothermodes;
    symbolpointer(*sym)->fid =
      extrafunc->fnum = CANNA_FN_MAX_FUNC + nothermodes;

    /* ¥Ç¥Õ¥©¥ë¥È¤ÎÀßÄê */
    extrafunc->display_name = (WCHAR_T *)NULL;
    extrafunc->u.modeptr = (newmode *)malloc(sizeof(newmode));
    if (extrafunc->u.modeptr) {
      KanjiMode kanjimode;

      extrafunc->u.modeptr->romaji_table = (char *)0;
      extrafunc->u.modeptr->romdic = (struct RkRxDic *)0;
      extrafunc->u.modeptr->romdic_owner = 0;
      extrafunc->u.modeptr->flags = CANNA_YOMI_IGNORE_USERSYMBOLS;
      extrafunc->u.modeptr->emode = (KanjiMode)0;

      /* ¥â¡¼¥É¹½Â¤ÂÎ¤ÎºîÀ® */
      kanjimode = (KanjiMode)malloc(sizeof(KanjiModeRec));
      if (kanjimode) {
	extern KanjiModeRec empty_mode;
	extern BYTE *emptymap;

	kanjimode->func = searchfunc;
	kanjimode->keytbl = emptymap;
	kanjimode->flags = 
	  CANNA_KANJIMODE_TABLE_SHARED | CANNA_KANJIMODE_EMPTY_MODE;
	kanjimode->ftbl = empty_mode.ftbl;
	extrafunc->u.modeptr->emode = kanjimode;

	/* ¥â¡¼¥ÉÉ½¼¨Ê¸»úÎó */
	ecode = DEFMODE_NOTSTRING;
	edata = md;
	if (stringp(md) || null(md)) {
	  if (stringp(md)) {
	    extrafunc->display_name = WString(xstring(md));
	  }
	  ecode = DEFMODE_MEMORY;
	  if (null(md) || extrafunc->display_name) {
	    /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë */
	    ecode = DEFMODE_NOTSTRING;
	    edata = rd;
	    if (stringp(rd) || null(rd)) {
	      char *newstr;
	      long f = extrafunc->u.modeptr->flags;

	      if (stringp(rd)) {
		newstr = (char *)malloc(strlen(xstring(rd)) + 1);
	      }
	      ecode = DEFMODE_MEMORY;
	      if (null(rd) || newstr) {
		if (!null(rd)) {
		  strcpy(newstr, xstring(rd));
		  extrafunc->u.modeptr->romaji_table = newstr;
		}
		/* ¼Â¹Ôµ¡Ç½ */
		for (e = fn ; consp(e) ; e = cdr(e)) {
		  l = car(e);
		  if (symbolp(l) && symbolpointer(l)->fid) {
		    switch (symbolpointer(l)->fid) {
		    case CANNA_FN_Kakutei:
		      f |= CANNA_YOMI_KAKUTEI;
		      break;
		    case CANNA_FN_Henkan:
		      f |= CANNA_YOMI_HENKAN;
		      break;
		    case CANNA_FN_Zenkaku:
		      f |= CANNA_YOMI_ZENKAKU;
		      break;
		    case CANNA_FN_Hankaku:
		      f |= CANNA_YOMI_HANKAKU;
		      break;
		    case CANNA_FN_Hiragana:
		      f |= CANNA_YOMI_HIRAGANA;
		      break;
		    case CANNA_FN_Katakana:
		      f |= CANNA_YOMI_KATAKANA;
		      break;
		    case CANNA_FN_Romaji:
		      f |= CANNA_YOMI_ROMAJI;
		      break;
		      /* °Ê²¼¤Ï¤½¤Î¤¦¤Á¤ä¤í¤¦ */
		    case CANNA_FN_ToUpper:
		      break;
		    case CANNA_FN_Capitalize:
		      break;
		    case CANNA_FN_ToLower:
		      break;
		    default:
		      goto defmode_not_function;
		    }
		  }
		  else {
		    goto defmode_not_function;
		  }
		}
		extrafunc->u.modeptr->flags = f;

		/* ¥æ¡¼¥¶¥·¥ó¥Ü¥ë¤Î»ÈÍÑ¤ÎÍ­Ìµ */
		if (us) {
		  extrafunc->u.modeptr->flags &=
		    ~CANNA_YOMI_IGNORE_USERSYMBOLS;
		}
 
		extrafunc->keyword = EXTRA_FUNC_DEFMODE;
		extrafunc->next = extrafuncp;
		extrafuncp = extrafunc;
		nothermodes++;
		return pop1();

	      defmode_not_function:
		ecode = DEFMODE_ILLFUNCTION;
		edata = l;
		if (!null(rd)) {
		  free(newstr);
		}
	      }
	    }
	    if (extrafunc->display_name) {
	      WSfree(extrafunc->display_name);
	    }
	  }
	}
	free(kanjimode);
      }
      free(extrafunc->u.modeptr);
    }
    free(extrafunc);
  }
  switch (ecode) {
  case DEFMODE_MEMORY:
    error("Insufficient memory", NON);
  case DEFMODE_NOTSTRING:
    error("String data expected ", edata);
  case DEFMODE_ILLFUNCTION:
    error("defmode: illegal subfunction ", edata);
  }
  /* NOTREACHED */
#else /* if WIN_CANLISP */
  /* ¥·¥ó¥Ü¥ë¤Î´Ø¿ôÃÍ¤È¤·¤Æ¤ÎÄêµÁ */
  symbolpointer(*sym)->mid = CANNA_MODE_MAX_IMAGINARY_MODE + nothermodes;
  symbolpointer(*sym)->fid = CANNA_FN_MAX_FUNC + nothermodes;
  nothermodes++;
  return pop1();
#endif /* WIN_CANLISP */
}

static list
Ldefsym(void)
{
  list form, res, e;
  int i, ncand, group;
  WCHAR_T cand[1024], *p, *mcand, **acand, key, xkey;
  int mcandsize;
  extern int nkeysup;
  extern keySupplement keysup[];

  form = sp[0];
  if (atom(form)) {
    error("Illegal form ",form);
    /* NOTREACHED */
  }
  /* ¤Þ¤º¿ô¤ò¤«¤¾¤¨¤ë */
  for (ncand = 0 ; consp(form) ; ) {
    e = car(form);
    if (!numberp(e)) {
      error("Key data expected ", e);
      /* NOTREACHED */
    }
    if (null(cdr(form))) {
      error("Illegal form ",sp[0]);
      /* NOTREACHED */
    }
    if (numberp(car(cdr(form)))) {
      form = cdr(form);
    }
    for (i = 0, form = cdr(form) ; consp(form) ; i++, form = cdr(form)) {
      e = car(form);
      if (!stringp(e)) {
	break;
      }
    }
    if (ncand == 0) {
      ncand = i;
    }
    else if (ncand != i) {
      error("Inconsist number for each key definition ", sp[0]);
      /* NOTREACHED */
    }
  }

  group = nkeysup;

  for (form = sp[0] ; consp(form) ;) {
    if (nkeysup >= MAX_KEY_SUP) {
      error("Too many symbol definitions", sp[0]);
      /* NOTREACHED */
    }
    /* The following lines are for xkey translation rule */
    key = (WCHAR_T)xnum(car(form));
    if (numberp(car(cdr(form)))) {
      xkey = (WCHAR_T)xnum(car(cdr(form)));
      form = cdr(form);
    }
    else {
      xkey = key;
    }
    p = cand;
    for (form = cdr(form) ; consp(form) ; form = cdr(form)) {
      int len;

      e = car(form);
      if (!stringp(e)) {
	break;
      }
      len = MBstowcs(p, xstring(e), 1024 - (p - cand));
      p += len;
      *p++ = (WCHAR_T)0;
    }
    *p++ = (WCHAR_T)0;
    mcandsize = p - cand;
    mcand = (WCHAR_T *)malloc(mcandsize * sizeof(WCHAR_T));
    if (mcand == 0) {
      error("Insufficient memory", NON);
      /* NOTREACHED */
    }
    acand = (WCHAR_T **)calloc(ncand + 1, sizeof(WCHAR_T *));
    if (acand == 0) {
      free(mcand);
      error("Insufficient memory", NON);
      /* NOTREACHED */
    }

    for (i = 0 ; i < p - cand ; i++) {
      mcand[i] = cand[i];
    }
    for (i = 0, p = mcand ; i < ncand ; i++) {
      acand[i] = p;
      while (*p++)
	/* EMPTY */
	;
    }
    acand[i] = 0;
    /* ¼ÂºÝ¤Ë³ÊÇ¼¤¹¤ë */
    keysup[nkeysup].key = key;
    keysup[nkeysup].xkey = xkey;
    keysup[nkeysup].groupid = group;
    keysup[nkeysup].ncand = ncand;
    keysup[nkeysup].cand = acand;
    keysup[nkeysup].fullword = mcand;
#ifdef WIN_CANLISP
    keysup[nkeysup].fullwordsize = mcandsize - 1; /* exclude the extra EOS */
#endif
    nkeysup++;
  }
#ifdef WIN_CANLISP
  if (wins.conf && wins.conf->symfn) {
    unsigned char *keys, *xkeys;
    WCHAR_T *words;
    int ngroups = nkeysup - group, fullwordlen, i;

    for (fullwordlen = 0, i = group ; i < nkeysup ; i++) {
      fullwordlen += keysup[i].fullwordsize;
    }

    keys = (char *)malloc(ngroups + 1);
    if (keys) {
      xkeys = (char *)malloc(ngroups + 1);
      if (xkeys) {
	words = (WCHAR_T *)malloc(fullwordlen * sizeof(WCHAR_T));
	if (words) {
	  unsigned char *pk = keys, *px = xkeys;
	  WCHAR_T *pw = words, *ps;
	  int j, len;

	  for (i = group ; i < nkeysup ; i++) {
	    *pk++ = (unsigned char)keysup[i].key;
	    *px++ = (unsigned char)keysup[i].xkey;
	    len = keysup[i].fullwordsize;
	    ps = keysup[i].fullword;
	    for (j = 0 ; j < len ; j++) {
	      *pw++ = *ps++;
	    }
	  }
	  *pk = (unsigned char)0;
	  *px = (unsigned char)0;

	  (*wins.conf->symfn)(keysup[group].ncand, nkeysup - group,
			      pw - words, keys, xkeys, words, wins.context);

	  free(words);
	}
	free(xkeys);
      }
      free(keys);
    }
  }
#endif
  res = car(pop1());
  return (res);
}

#ifndef NO_EXTEND_MENU

/* 
   defselection ¤Ç°ìÍ÷¤ÎÊ¸»ú¤ò¼è¤ê½Ð¤¹¤¿¤á¤ËÉ¬Í×¤Ê¤Î¤Ç¡¢°Ê²¼¤òÄêµÁ¤¹¤ë
 */

#define SS2	((char)0x8e)
#define SS3	((char)0x8f)

#define G0	0
#define G1	1
#define G2	2
#define G3	3

static int cswidth[4] = {1, 2, 2, 3};


/*
   getKutenCode -- Ê¸»ú¤Î¶èÅÀ¥³¡¼¥É¤ò¼è¤ê½Ð¤¹
 */

static int
getKutenCode(char *data, int *ku, int *ten)
{
  int codeset;

  *ku = (data[0] & 0x7f) - 0x20;
  *ten = (data[1] & 0x7f) - 0x20;
  if (*data == SS2) {
    codeset = G2;
    *ku = 0;
  }
  else if (*data == SS3) {
    codeset = G3;
    *ku = *ten;
    *ten = (data[2] & 0x7f) - 0x20;
  }
  else if (*data & 0x80) {
    codeset = G1;
  }
  else {
    codeset = G0;
    *ten = *ku;
    *ku = 0;
  }
  return codeset;
}
    
/* 
   howManuCharsAre -- defselection ¤ÇÈÏ°Ï»ØÄê¤·¤¿¾ì¹ç¤Ë
                      ¤½¤ÎÈÏ°ÏÆâ¤Î¿Þ·ÁÊ¸»ú¤Î¸Ä¿ô¤òÊÖ¤¹
 */

static int
howManyCharsAre(char *tdata, char *edata, int *tku, int *tten, int *codeset)
{
  int eku, eten, kosdata, koedata;

  kosdata = getKutenCode(tdata, tku, tten);
  koedata = getKutenCode(edata, &eku, &eten);
  if (kosdata != koedata) {
    return 0;
  }
  else {
    *codeset = kosdata;
    return ((eku - *tku) * 94 + eten - *tten + 1);
  }
}


/*
   pickupChars -- ÈÏ°ÏÆâ¤Î¿Þ·ÁÊ¸»ú¤ò¼è¤ê½Ð¤¹
 */

static char *
pickupChars(int tku, int tten, int num, int kodata)
{
  char *dptr, *tdptr, *edptr;

  dptr = (char *)malloc(num * cswidth[kodata] + 1);
  if (dptr) {
    tdptr = dptr;
    edptr = dptr + num * cswidth[kodata];
    for (; dptr < edptr ; tten++) {
      if (tten > 94) {
        tku++;
        tten = 1;
      }
      switch(kodata) {
        case G0:
          *dptr++ = (tten + 0x20);
          break;
        case G1:
          *dptr++ = (tku + 0x20) | 0x80;
          *dptr++ = (tten + 0x20) | 0x80;
          break;
        case G2:
          *dptr++ = SS2;
          *dptr++ = (tten + 0x20) | 0x80;
          break;
        case G3:
          *dptr++ = SS3;
          *dptr++ = (tku + 0x20) | 0x80;
          *dptr++ = (tten + 0x20) | 0x80;
          break;
        default:
          break;
      }
    }
    *dptr++ = '\0';
    return tdptr;
  }
  else {
    error("Insufficient memory", NON);
    /* NOTREACHED */
  }
}

/* 
   numtostr -- Key data ¤«¤éÊ¸»ú¤ò¼è¤ê½Ð¤¹
 */

static void
numtostr(unsigned long num, char *str)
{
  if (num & 0xff0000) {
    *str++ = (char)((num >> 16) & 0xff);
  }
  if (num & 0xff00) {
    *str++ = (char)((num >> 8) & 0xff);
  }
  *str++ = (char)(num & 0xff);
  *str = '\0';
}

/*
  defselection -- Ê¸»ú°ìÍ÷¤ÎÄêµÁ

  ¡Ô·Á¼°¡Õ
  (defselection function-symbol "¥â¡¼¥ÉÉ½¼¨" '(character-list))
 */

static list
Ldefselection(void)
{
  list form, sym, e, e2, md, kigo_list, buf;
  extern extraFunc *extrafuncp;
  extern int nothermodes;
  int i, len, cs, nkigo_data = 0, kigolen = 0;
  WCHAR_T *p, *kigo_str, **akigo_data;
  extraFunc *extrafunc = (extraFunc *)0;

  form = sp[0];

  if (atom(form) || atom(cdr(form)) || atom(cdr(cdr(form)))) {
    error("Illegal form ",form);
    /* NOTREACHED */
  }

  sym = car(form);
  if (!symbolp(sym)) {
    error("Symbol data expected ", sym);
    /* NOTREACHED */
  }

  md = car(cdr(form));
  if (!stringp(md) && !null(md)) {
    error("String data expected ", md);
    /* NOTREACHED */
  }
    
  push(car(cdr(cdr(form))));
  push(Leval(1));

  kigo_list = sp[0];
  if (atom(kigo_list)) {
    error("Illegal form ", kigo_list);
    /* NOTREACHED */
  }

  /* ¤Þ¤ºÎÎ°è¤ò³ÎÊÝ¤¹¤ë */
  buf = kigo_list;
  while (!atom(buf)) {
    if (!atom(cdr(buf)) && (car(cdr(buf)) == HYPHEN)) {
      /* ÈÏ°Ï»ØÄê¤Î¤È¤­ */
      if (atom(cdr(cdr(buf)))) {
        error("Illegal form ", buf);
        /* NOTREACHED */
      }else{
        int sku, sten, num;
        char ss[4], ee[4];

        e = car(buf);
        if (!numberp(e)) {
          error("Key data expected ", e);
          /* NOTREACHED */
        }
        e2 = car(cdr(cdr(buf)));
        if (!numberp(e2)) {
          error("Key data expected ", e2);
          /* NOTREACHED */
        }

        numtostr(xnum(e), ss);
        numtostr(xnum(e2), ee);
        num = howManyCharsAre(ss, ee, &sku, &sten, &cs);
        if (num <= 0) {
          error("Inconsistent range of charcter code ", buf);
          /* NOTREACHED */
        }
        kigolen = kigolen + (cswidth[cs] + 1) * num;
        nkigo_data += num;
      }
      buf = cdr(cdr(cdr(buf)));
    }
    else {
     /* Í×ÁÇ»ØÄê¤Î¤È¤­ */
      char xx[4], *xxp;

      e = car(buf);
      if (!numberp(e) && !stringp(e)) {
        error("Key or string data expected ", e);
        /* NOTREACHED */
      }
      else if (numberp(e)) {
        numtostr(xnum(e), xx);
        xxp = xx;
      }else{
        xxp = xstring(e);
      }

      for ( ; *xxp ; xxp += cswidth[cs] ) {
        if (*xxp == SS2) {
          cs = G2;
        }
        else if (*xxp == SS3) {
          cs = G3;
        }
        else if (*xxp & 0x80) {
          cs = G1;
        }
        else {
          cs = G0;
        }
        kigolen = kigolen + cswidth[cs];
      }
      kigolen += 1;  /* ³ÆÍ×ÁÇ¤ÎºÇ¸å¤Ë \0 ¤òÆþ¤ì¤ë */
      nkigo_data++;
      buf = cdr(buf);
    }
  }

  kigo_str = (WCHAR_T *)malloc(kigolen * sizeof(WCHAR_T));
  if (!kigo_str) {
    error("Insufficient memory ", NON);
    /* NOTREACHED */
  }
  p = kigo_str;

  /* °ìÍ÷¤ò¼è¤ê½Ð¤¹ */
  while (!atom(kigo_list)) {
    if (!atom(cdr(kigo_list)) && (car(cdr(kigo_list)) == HYPHEN)) {
    /* ÈÏ°Ï»ØÄê¤Î¤È¤­ */
      int sku, sten, codeset, num;
      char *ww, *sww, *eww, ss[4], ee[4], bak;

      e = car(kigo_list);
      e2 = car(cdr(cdr(kigo_list)));
      numtostr(xnum(e), ss);
      numtostr(xnum(e2), ee);
      num = howManyCharsAre(ss, ee, &sku, &sten, &codeset);
      sww = ww = pickupChars(sku, sten, num, codeset);
      cs = cswidth[codeset];
      eww = ww + num * cs;
      while (ww < eww) {
        bak = ww[cs];
        ww[cs] = '\0';
        len = MBstowcs(p, ww, kigolen - (p - kigo_str));
        p += len;
        *p++ = (WCHAR_T)0;
        ww += cs;
        ww[0] = bak;
      }
      free(sww);
      kigo_list = cdr(cdr(cdr(kigo_list)));
    }
    else {
      /* Í×ÁÇ»ØÄê¤Î¤È¤­ */
      char xx[4], *xxp;

      e = car(kigo_list);
      if (numberp(e)) {
        numtostr(xnum(e), xx);
        xxp = xx;
      }else{
        xxp = xstring(e);
      }
      len = MBstowcs(p, xxp, kigolen - (p - kigo_str));
      p += len;
      *p++ = (WCHAR_T)0;
      kigo_list = cdr(kigo_list);
    }
  }

  akigo_data = (WCHAR_T **)calloc(nkigo_data + 1, sizeof(WCHAR_T *));
  if (akigo_data == 0) {
    free(kigo_str);
    error("Insufficient memory", NON);
    /* NOTREACHED */
  }

  for (i = 0, p = kigo_str ; i < nkigo_data ; i++) {
    akigo_data[i] = p;
    while (*p++)
      /* EMPTY */
      ;
  }

  /* ÎÎ°è¤ò³ÎÊÝ¤¹¤ë */
  extrafunc = (extraFunc *)malloc(sizeof(extraFunc));
  if (!extrafunc) {
    free(kigo_str);
    free(akigo_data);
    error("Insufficient memory", NON);
    /* NOTREACHED */
  }
  extrafunc->u.kigoptr = (kigoIchiran *)malloc(sizeof(kigoIchiran));
  if (!extrafunc->u.kigoptr) {
    free(kigo_str);
    free(akigo_data);
    free(extrafunc);
    error("Insufficient memory", NON);
    /* NOTREACHED */
  }

  /* ¥·¥ó¥Ü¥ë¤Î´Ø¿ôÃÍ¤È¤·¤Æ¤ÎÄêµÁ */
  symbolpointer(sym)->mid = extrafunc->u.kigoptr->kigo_mode
                          = CANNA_MODE_MAX_IMAGINARY_MODE + nothermodes;
  symbolpointer(sym)->fid = extrafunc->fnum
                          = CANNA_FN_MAX_FUNC + nothermodes;

  /* ¼ÂºÝ¤Ë³ÊÇ¼¤¹¤ë */
  extrafunc->u.kigoptr->kigo_data = akigo_data;
  extrafunc->u.kigoptr->kigo_str = kigo_str;
  extrafunc->u.kigoptr->kigo_size = nkigo_data;
  if (stringp(md)) {
    extrafunc->display_name = WString(xstring(md));
  }
  else {
    extrafunc->display_name = (WCHAR_T *)0;
  }

  extrafunc->keyword = EXTRA_FUNC_DEFSELECTION;
  extrafunc->next = extrafuncp;
  extrafuncp = extrafunc;
  pop(2);
  nothermodes++;
  return sym;
}

/*
  defmenu -- ¥á¥Ë¥å¡¼¤ÎÄêµÁ

  ¡Ô·Á¼°¡Õ
  (defmenu first-menu
    ("ÅÐÏ¿" touroku)
    ("¥µ¡¼¥ÐÁàºî" server))
 */

static list
Ldefmenu(void)
{
  list form, sym, e;
  extern extraFunc *extrafuncp;
  extern int nothermodes;
  extraFunc *extrafunc = (extraFunc *)0;
  int i, n, clen, len;
  WCHAR_T foo[512];
  menustruct *men;
  menuitem *menubody;
  WCHAR_T *wp, **wpp;

  form = sp[0];
  if (atom(form) || atom(cdr(form))) {
    error("Bad form ", form);
    /* NOTREACHED */
  }
  sym = car(form);
  if (!symbolp(sym)) {
    error("Symbol data expected ", sym);
    /* NOTREACHED */
  }

  /* °ú¿ô¤ò¿ô¤¨¤ë¡£¤Ä¤¤¤Ç¤ËÉ½¼¨Ê¸»úÎó¤ÎÊ¸»ú¿ô¤ò¿ô¤¨¤ë */
  for (n = 0, clen = 0, e = cdr(form) ; !atom(e) ; n++, e = cdr(e)) {
    list l = car(e), d, fn;
    if (atom(l) || atom(cdr(l))) {
      error("Bad form ", form);
    }
    d = car(l);
    fn = car(cdr(l));
    if (!stringp(d) || !symbolp(fn)) {
      error("Bad form ", form);
    }
    len = MBstowcs(foo, xstring(d), 512);
    if (len >= 0) {
      clen += len + 1;
    }
  }

  extrafunc = (extraFunc *)malloc(sizeof(extraFunc));
  if (extrafunc) {
    men = allocMenu(n, clen);
    if (men) {
      menubody = men->body;
      /* ¥¿¥¤¥È¥ëÊ¸»ú¤ò¥Ç¡¼¥¿¥Ð¥Ã¥Õ¥¡¤Ë¥³¥Ô¡¼ */
      for (i = 0, wp = men->titledata, wpp = men->titles, e = cdr(form) ;
	   i < n ; i++, e = cdr(e)) {
	len = MBstowcs(wp, xstring(car(car(e))), 512);
	*wpp++ = wp;
	wp += len + 1;

	menubody[i].flag = MENU_SUSPEND;
	menubody[i].u.misc = (char *)car(cdr(car(e)));
      }
      men->nentries = n;

      /* ¥·¥ó¥Ü¥ë¤Î´Ø¿ôÃÍ¤È¤·¤Æ¤ÎÄêµÁ */
      symbolpointer(sym)->mid =
	men->modeid = CANNA_MODE_MAX_IMAGINARY_MODE + nothermodes;
      symbolpointer(sym)->fid =
	extrafunc->fnum = CANNA_FN_MAX_FUNC + nothermodes;
      extrafunc->keyword = EXTRA_FUNC_DEFMENU;
      extrafunc->display_name = (WCHAR_T *)0;
      extrafunc->u.menuptr = men;

      extrafunc->next = extrafuncp;
      extrafuncp = extrafunc;
      nothermodes++;
      (void)pop1();
      return sym;
    }
    free(extrafunc);
  }
  error("Insufficient memory", NON);
  /* NOTREACHED */
}
#endif /* NO_EXTEND_MENU */

static list
Lsetinifunc(int n)
{
  unsigned char fseq[256];
  int i, len;
  list ret = NIL;
  extern BYTE *initfunc;

  argnchk(S_SetInitFunc, 1);

  len = xfseq(S_SetInitFunc, sp[0], fseq, 256);

  if (len > 0) {
    if (initfunc) free(initfunc);
    initfunc = (BYTE *)malloc(len + 1);
    if (!initfunc) {
      error("Insufficient memory", NON);
      /* NOTREACHED */
    }
    for (i = 0 ; i < len ; i++) {
      initfunc[i] = fseq[i];
    }
    initfunc[i] = 0;
    ret = T;
  }
  (void)pop1();
  return ret;
}

static list
Lboundp(int n)
{
  list e;
  struct atomcell *sym;

  argnchk("boundp",1);
  e = pop1();

  if (!atom(e)) {
    error("boundp: bad arg ", e);
    /* NOTREACHED */
  }
  else if (constp(e)) {
    error("boundp: bad arg ", e);
    /* NOTREACHED */
  }

  if (assq(e, *esp)) {
    return T;
  }
  else if ((sym = symbolpointer(e))->valfunc) {
    return T;
  }
  else {
    if (sym->value != (list)UNBOUND) {
      return T;
    }
    else {
      return NIL;
    }
  }
}

static list
Lfboundp(int n)
{
  list e;

  argnchk("fboundp",1);
  e = pop1();

  if (!atom(e)) {
    error("fboundp: bad arg ", e);
    /* NOTREACHED */
  }
  else if (constp(e)) {
    error("fboundp: bad arg ", e);
    /* NOTREACHED */
  }
  if (symbolpointer(e)->ftype == UNDEF) {
    return NIL;
  }
  else {
    return T;
  }
}

static list
Lgetenv(int n)
{
  list e;
  char strbuf[256], *ret;
  list retval;

  argnchk("getenv",1);
  e = sp[0];

  if (!stringp(e)) {
    error("getenv: bad arg ", e);
    /* NOTREACHED */
  }

  strncpy(strbuf, xstring(e), xstrlen(e));
  strbuf[xstrlen(e)] = '\0';
  ret = getenv(strbuf);
  if (ret) {
    retval = copystring(ret, strlen(ret));
  }
  else {
    retval = NIL;
  }
  (void)pop1();
  return retval;
}

static list
LdefEscSeq(int n)
{

  argnchk("define-esc-sequence",3);

  if (!stringp(sp[2])) {
    error("define-esc-sequence: bad arg ", sp[2]);
    /* NOTREACHED */
  }
  if (!stringp(sp[1])) {
    error("define-esc-sequence: bad arg ", sp[1]);
    /* NOTREACHED */
  }
  if (!numberp(sp[0])) {
    error("define-esc-sequence: bad arg ", sp[0]);
    /* NOTREACHED */
  }
  if (keyconvCallback) {
    (*keyconvCallback)(CANNA_CTERMINAL, 
		       xstring(sp[2]), xstring(sp[1]), xnum(sp[0]));
  }
  pop(3);
  return NIL;
}

static list
LdefXKeysym(int n)
{

  argnchk("define-x-keysym",2);

  if (!stringp(sp[1])) {
    error("define-esc-sequence: bad arg ", sp[1]);
    /* NOTREACHED */
  }
  if (!numberp(sp[0])) {
    error("define-esc-sequence: bad arg ", sp[0]);
    /* NOTREACHED */
  }
  if (keyconvCallback) {
    (*keyconvCallback)(CANNA_XTERMINAL, 
		       xstring(sp[2]), xstring(sp[1]), xnum(sp[0]));
  }
  pop(2);
  return NIL;
}

static list
Lconcat(int n)
{
  list t, res;
  int  i, len;
  char *p;

  /* ¤Þ¤ºÄ¹¤µ¤ò¿ô¤¨¤ë¡£ */
  for (len= 0, i = n ; i-- ;) {
    t = sp[i];
    if (!stringp(t)) {
      lisp_strerr("concat", t);
      /* NOTREACHED */
    }
    len += xstrlen(t);
  }
  res = allocstring(len);
  for (p = xstring(res), i = n ; i-- ;) {
    t = sp[i];
    len = xstrlen(t);
    Strncpy(p, xstring(t), len);
    p += len;
  }
  *p = '\0';
  pop(n);
  return res;
}

/* lispfuncend */

extern char *RkGetServerHost();


/* ÊÑ¿ô¥¢¥¯¥»¥¹¤Î¤¿¤á¤Î´Ø¿ô */

static list
VTorNIL(BYTE *var, int setp, list arg)
{
  if (setp == VALSET) {
    *var = (arg == NIL) ? 0 : 1;
    return arg;
  }
  else { /* get */
    return *var ? T : NIL;
  }
}

static list
StrAcc(char **var, int setp, list arg)
{
  if (setp == VALSET) {
    if (null(arg) || stringp(arg)) {
      if (*var) {
	free(*var);
      }
      if (stringp(arg)) {
	*var = (char *)malloc(strlen(xstring(arg)) + 1);
	if (*var) {
	  strcpy(*var, xstring(arg));
	  return arg;
	}
	else {
	  error("Insufficient memory.", NON);
	  /* NOTREACHED */
	}
      }else{
	*var = (char *)0;
	return NIL;
      }
    }
    else {
      lisp_strerr((char *)0, arg);
      /* NOTREACHED */
    }
  }
  /* else { .. */
  if (*var) {
    return copystring(*var, strlen(*var));
  }
  else {
    return NIL;
  }
  /* end else .. } */
}

static list
NumAcc(int *var, int setp, list arg)
{
  if (setp == VALSET) {
    if (numberp(arg)) {
      *var = (int)xnum(arg);
      return arg;
    }
    else {
      numerr((char *)0, arg);
      /* NOTREACHED */
    }
  }
  return (list)mknum(*var);
}

#ifdef WIN_CANLISP
static struct RegInfo reginfo;
#endif

/* ¤³¤³¤«¤é²¼¤¬¥«¥¹¥¿¥Þ¥¤¥º¤ÎÄÉ²ÃÅù¤ÇÎÉ¤¯¤¤¤¸¤ëÉôÊ¬ */

/* ¼ÂºÝ¤Î¥¢¥¯¥»¥¹´Ø¿ô */

#define DEFVAR(fn, acc, ty, var) \
static list fn(int setp, list arg) { \
  extern ty var; return acc(&var, setp, arg); }

#define DEFVAREX(fn, acc, var) \
static list fn(int setp, list arg) { \
  extern struct CannaConfig cannaconf; return acc(&var, setp, arg); }

static list Vnkouhobunsetsu(int setp, list arg)
{
  extern int nKouhoBunsetsu;

  arg = NumAcc(&nKouhoBunsetsu, setp, arg);
#ifdef RESTRICT_NKOUHOBUNSETSU
  if (nKouhoBunsetsu < 3 || nKouhoBunsetsu > 60)
    nKouhoBunsetsu = 16;
#else
  if (nKouhoBunsetsu < 0) {
    nKouhoBunsetsu = 0;
  }
#endif
  return arg;
}

static list VProtoVer(int setp, list arg)
{
}

static list VServVer(int setp, list arg)
{
}

static list VServName(int setp, list arg)
{
}

static list
VCannaDir(int setp, list arg)
{
extern char	basepath[];

  char *canna_dir = basepath;

  if (setp == VALGET) {
    return StrAcc(&canna_dir, setp, arg);
  }
  else {
    return NIL;
  }
}

static list VCodeInput(int setp, list arg)
{
  extern struct CannaConfig cannaconf;
  static char *input_code[CANNA_MAX_CODE] = {"jis", "sjis", "kuten"};

  if (setp == VALSET) {
    if (null(arg) || stringp(arg)) {
      if (stringp(arg)) {
	int i;
	char *s = xstring(arg);

	for (i = 0 ; i < CANNA_MAX_CODE ; i++) {
	  if (!strcmp(s, input_code[i])) {
	    cannaconf.code_input = i;
	    break;
	  }
	}
	if (i < CANNA_MAX_CODE) {
	  return arg;
	}
	else {
	  return NIL;
	}
      }else{
	cannaconf.code_input = 0; /* use default */
	return copystring(input_code[0], strlen(input_code[0]));
      }
    }
    else {
      lisp_strerr((char *)0, arg);
      /* NOTREACHED */
    }
  }
  /* else { .. */
  if (/* 0 <= cannaconf.code_input && /* unsigned ¤Ë¤·¤¿¤Î¤Ç¾éÄ¹¤Ë¤Ê¤Ã¤¿ */
      cannaconf.code_input <= CANNA_CODE_KUTEN) {
    return copystring(input_code[cannaconf.code_input],
		      strlen(input_code[cannaconf.code_input]));
  }
  else {
    return NIL;
  }
  /* end else .. } */
}


DEFVAR(Vromkana         ,StrAcc  ,char * ,RomkanaTable)
DEFVAR(Venglish         ,StrAcc  ,char * ,EnglishTable)

DEFVAREX(Vnhenkan       ,NumAcc          ,cannaconf.kouho_threshold)
DEFVAREX(Vndisconnect   ,NumAcc          ,cannaconf.strokelimit)
DEFVAREX(VCannaVersion  ,NumAcc          ,cannaconf.CannaVersion)
DEFVAREX(VIndexSeparator,NumAcc          ,cannaconf.indexSeparator)

DEFVAREX(Vgakushu       ,VTorNIL         ,cannaconf.Gakushu)
DEFVAREX(Vcursorw       ,VTorNIL         ,cannaconf.CursorWrap)
DEFVAREX(Vselectd       ,VTorNIL         ,cannaconf.SelectDirect)
DEFVAREX(Vnumeric       ,VTorNIL         ,cannaconf.HexkeySelect)
DEFVAREX(Vbunsets       ,VTorNIL         ,cannaconf.BunsetsuKugiri)
DEFVAREX(Vcharact       ,VTorNIL         ,cannaconf.ChBasedMove)
DEFVAREX(Vreverse       ,VTorNIL         ,cannaconf.ReverseWidely)
DEFVAREX(VreverseWord   ,VTorNIL         ,cannaconf.ReverseWord)
DEFVAREX(Vquitich       ,VTorNIL         ,cannaconf.QuitIchiranIfEnd)
DEFVAREX(Vkakutei       ,VTorNIL         ,cannaconf.kakuteiIfEndOfBunsetsu)
DEFVAREX(Vstayaft       ,VTorNIL         ,cannaconf.stayAfterValidate)
DEFVAREX(Vbreakin       ,VTorNIL         ,cannaconf.BreakIntoRoman)
DEFVAREX(Vgrammati      ,VTorNIL         ,cannaconf.grammaticalQuestion)
DEFVAREX(Vforceka       ,VTorNIL         ,cannaconf.forceKana)
DEFVAREX(Vkouhoco       ,VTorNIL         ,cannaconf.kCount)
DEFVAREX(Vauto          ,VTorNIL         ,cannaconf.chikuji)
DEFVAREX(VlearnNumTy    ,VTorNIL         ,cannaconf.LearnNumericalType)
DEFVAREX(VBSasQuit      ,VTorNIL         ,cannaconf.BackspaceBehavesAsQuit)
DEFVAREX(Vinhibi        ,VTorNIL         ,cannaconf.iListCB)
DEFVAREX(Vkeepcupos     ,VTorNIL         ,cannaconf.keepCursorPosition)
DEFVAREX(VAbandon       ,VTorNIL         ,cannaconf.abandonIllegalPhono)
DEFVAREX(VHexStyle      ,VTorNIL         ,cannaconf.hexCharacterDefiningStyle)
DEFVAREX(VKojin         ,VTorNIL         ,cannaconf.kojin)
DEFVAREX(VIndexHankaku  ,VTorNIL         ,cannaconf.indexHankaku)
DEFVAREX(VAllowNext     ,VTorNIL         ,cannaconf.allowNextInput)
DEFVAREX(VkanaGaku      ,VTorNIL         ,cannaconf.doKatakanaGakushu)
DEFVAREX(VhiraGaku      ,VTorNIL         ,cannaconf.doHiraganaGakushu)
DEFVAREX(VChikujiContinue ,VTorNIL       ,cannaconf.ChikujiContinue)
DEFVAREX(VRenbunContinue  ,VTorNIL       ,cannaconf.RenbunContinue)
DEFVAREX(VMojishuContinue ,VTorNIL       ,cannaconf.MojishuContinue)
DEFVAREX(VcRealBS       ,VTorNIL         ,cannaconf.chikujiRealBackspace)
DEFVAREX(VIgnoreCase    ,VTorNIL         ,cannaconf.ignore_case)
DEFVAREX(VRomajiYuusen  ,VTorNIL         ,cannaconf.romaji_yuusen)
DEFVAREX(VAutoSync      ,VTorNIL         ,cannaconf.auto_sync)
DEFVAREX(VQuicklyEscape ,VTorNIL         ,cannaconf.quickly_escape)
DEFVAREX(VInhibitHankana,VTorNIL         ,cannaconf.InhibitHankakuKana)
#ifdef WIN_CANLISP
DEFVAR(VremoteGroup	,StrAcc  ,char * ,RemoteGroup)
DEFVAR(VlocalGroup	,StrAcc  ,char * ,LocalGroup)

DEFVAREX(VcandInitWidth   ,NumAcc  ,reginfo.cand_init_width)
DEFVAREX(VcandInitHeight  ,NumAcc  ,reginfo.cand_init_height)
DEFVAREX(VcandMaxWidth    ,NumAcc  ,reginfo.cand_max_width)
DEFVAREX(VcandMaxHeight   ,NumAcc  ,reginfo.cand_max_height)
DEFVAREX(VstatusSize      ,NumAcc  ,reginfo.status_size)
#endif

#ifdef DEFINE_SOMETHING
DEFVAR(Vchikuji_debug, VTorNIL, int, chikuji_debug)
#endif

/* Lisp ¤Î´Ø¿ô¤È C ¤Î´Ø¿ô¤ÎÂÐ±þÉ½ */

static struct atomdefs initatom[] = {
  {"quote"		,SPECIAL,(list(*)(...))Lquote		},
  {"setq"		,SPECIAL,(list(*)(...))Lsetq		},
  {"set"		,SUBR	,(list(*)(...))Lset		},
  {"equal"		,SUBR	,(list(*)(...))Lequal		},
  {"="			,SUBR	,(list(*)(...))Lequal		},
  {">"			,SUBR	,(list(*)(...))Lgreaterp	},
  {"<"			,SUBR	,(list(*)(...))Llessp		},
  {"progn"		,SPECIAL,(list(*)(...))Lprogn		},
  {"eq"			,SUBR	,(list(*)(...))Leq   		},
  {"cond"		,SPECIAL,(list(*)(...))Lcond		},
  {"null"		,SUBR	,(list(*)(...))Lnull		},
  {"not"		,SUBR	,(list(*)(...))Lnull		},
  {"and"		,SPECIAL,(list(*)(...))Land		},
  {"or"			,SPECIAL,(list(*)(...))Lor		},
  {"+"			,SUBR	,(list(*)(...))Lplus		},
  {"-"			,SUBR	,(list(*)(...))Ldiff		},
  {"*"			,SUBR	,(list(*)(...))Ltimes		},
  {"/"			,SUBR	,(list(*)(...))Lquo		},
  {"%"			,SUBR	,(list(*)(...))Lrem		},
  {"gc"			,SUBR	,(list(*)(...))Lgc		},
  {"load"		,SUBR	,(list(*)(...))Lload		},
  {"list"		,SUBR	,(list(*)(...))Llist		},
  {"sequence"		,SUBR	,(list(*)(...))Llist		},
  {"defun"		,SPECIAL,(list(*)(...))Ldefun		},
  {"defmacro"		,SPECIAL,(list(*)(...))Ldefmacro	},
  {"cons"		,SUBR	,(list(*)(...))Lcons		},
  {"car"		,SUBR	,(list(*)(...))Lcar		},
  {"cdr"		,SUBR	,(list(*)(...))Lcdr		},
  {"atom"		,SUBR	,(list(*)(...))Latom		},
  {"let"		,CMACRO	,(list(*)(...))Llet		},
  {"if"			,CMACRO	,(list(*)(...))Lif		},
  {"boundp"		,SUBR	,(list(*)(...))Lboundp	},
  {"fboundp"		,SUBR	,(list(*)(...))Lfboundp	},
  {"getenv"		,SUBR	,(list(*)(...))Lgetenv	},
  {"copy-symbol"	,SUBR	,(list(*)(...))Lcopysym	},
  {"concat"		,SUBR	,(list(*)(...))Lconcat	},
  {S_FN_UseDictionary	,SUBR	,(list(*)(...))Lusedic	},
  {S_SetModeDisp	,SUBR	,(list(*)(...))Lmodestr	},
  {S_SetKey		,SUBR	,(list(*)(...))Lsetkey	},
  {S_GSetKey		,SUBR	,(list(*)(...))Lgsetkey	},
  {S_UnbindKey		,SUBR	,(list(*)(...))Lunbindkey	},
  {S_GUnbindKey		,SUBR	,(list(*)(...))Lgunbindkey	},
  {S_DefMode		,SPECIAL,(list(*)(...))Ldefmode	},
  {S_DefSymbol		,SPECIAL,(list(*)(...))Ldefsym	},
#ifndef NO_EXTEND_MENU
  {S_DefSelection	,SPECIAL,(list(*)(...))Ldefselection	},
  {S_DefMenu		,SPECIAL,(list(*)(...))Ldefmenu	},
#endif
  {S_SetInitFunc	,SUBR	,(list(*)(...))Lsetinifunc	},
  {S_defEscSequence	,SUBR	,(list(*)(...))LdefEscSeq	},
  {S_defXKeysym		,SUBR	,(list(*)(...))LdefXKeysym	},
  {0			,UNDEF	,0		}, /* DUMMY */
};

static void
deflispfunc(void)
{
  struct atomdefs *p;

  for (p = initatom ; p->symname ; p++) {
    struct atomcell *atomp;
    list temp;

    temp = getatmz(p->symname);
    atomp = symbolpointer(temp);
    atomp->ftype = p->symtype;
    if (atomp->ftype != UNDEF) {
      atomp->func = p->symfunc;
    }
  }
}


/* ÊÑ¿ôÉ½ */

static struct cannavardefs cannavars[] = {
  {S_VA_RomkanaTable		,(list(*)(...))Vromkana},
  {S_VA_EnglishTable		,(list(*)(...))Venglish},
  {S_VA_CursorWrap		,(list(*)(...))Vcursorw},
  {S_VA_SelectDirect		,(list(*)(...))Vselectd},
  {S_VA_NumericalKeySelect	,(list(*)(...))Vnumeric},
  {S_VA_BunsetsuKugiri		,(list(*)(...))Vbunsets},
  {S_VA_CharacterBasedMove	,(list(*)(...))Vcharact},
  {S_VA_ReverseWidely		,(list(*)(...))Vreverse},
  {S_VA_ReverseWord		,(list(*)(...))VreverseWord},
  {S_VA_Gakushu			,(list(*)(...))Vgakushu},
  {S_VA_QuitIfEOIchiran		,(list(*)(...))Vquitich},
  {S_VA_KakuteiIfEOBunsetsu	,(list(*)(...))Vkakutei},
  {S_VA_StayAfterValidate	,(list(*)(...))Vstayaft},
  {S_VA_BreakIntoRoman		,(list(*)(...))Vbreakin},
  {S_VA_NHenkanForIchiran	,(list(*)(...))Vnhenkan},
  {S_VA_GrammaticalQuestion	,(list(*)(...))Vgrammati},
  {"gramatical-question"	,(list(*)(...))Vgrammati}, /* °ÊÁ°¤Î¥¹¥Ú¥ë¥ß¥¹¤ÎµßºÑ */
  {S_VA_ForceKana		,(list(*)(...))Vforceka},
  {S_VA_KouhoCount		,(list(*)(...))Vkouhoco},
  {S_VA_Auto			,(list(*)(...))Vauto},
  {S_VA_LearnNumericalType	,(list(*)(...))VlearnNumTy},
  {S_VA_BackspaceBehavesAsQuit	,(list(*)(...))VBSasQuit},
  {S_VA_InhibitListCallback	,(list(*)(...))Vinhibi},
  {S_VA_nKouhoBunsetsu		,(list(*)(...))Vnkouhobunsetsu},
  {S_VA_keepCursorPosition	,(list(*)(...))Vkeepcupos},
  {S_VA_CannaVersion		,(list(*)(...))VCannaVersion},
  {S_VA_Abandon			,(list(*)(...))VAbandon},
  {S_VA_HexDirect		,(list(*)(...))VHexStyle},
  {S_VA_ProtocolVersion		,(list(*)(...))VProtoVer},
  {S_VA_ServerVersion		,(list(*)(...))VServVer},
  {S_VA_ServerName		,(list(*)(...))VServName},
  {S_VA_CannaDir		,(list(*)(...))VCannaDir},
  {S_VA_Kojin			,(list(*)(...))VKojin},
  {S_VA_IndexHankaku	       	,(list(*)(...))VIndexHankaku},
  {S_VA_IndexSeparator	       	,(list(*)(...))VIndexSeparator},
  {S_VA_AllowNextInput		,(list(*)(...))VAllowNext},
  {S_VA_doKatakanaGakushu	,(list(*)(...))VkanaGaku},
  {S_VA_doHiraganaGakushu	,(list(*)(...))VhiraGaku},
#ifdef	DEFINE_SOMETHING
  {S_VA_chikuji_debug		,(list(*)(...))Vchikuji_debug},
#endif	/* DEFINE_SOMETHING */
  {S_VA_ChikujiContinue		,(list(*)(...))VChikujiContinue},
  {S_VA_RenbunContinue		,(list(*)(...))VRenbunContinue},
  {S_VA_MojishuContinue		,(list(*)(...))VMojishuContinue},
  {S_VA_ChikujiRealBackspace	,(list(*)(...))VcRealBS},
  {S_VA_nDisconnectServer	,(list(*)(...))Vndisconnect},
  {S_VA_ignoreCase		,(list(*)(...))VIgnoreCase},
  {S_VA_RomajiYuusen		,(list(*)(...))VRomajiYuusen},
  {S_VA_AutoSync		,(list(*)(...))VAutoSync},
  {S_VA_QuicklyEscape		,(list(*)(...))VQuicklyEscape},
  {S_VA_InhibitHanKana		,(list(*)(...))VInhibitHankana},
  {S_VA_CodeInput		,(list(*)(...))VCodeInput},
#ifdef WIN_CANLISP
  {"remote-group"		,(list(*)(...))VremoteGroup},
  {"local-group"		,(list(*)(...))VlocalGroup},
  {"candlist-initial-width"     ,(list(*)(...))VcandInitWidth},
  {"candlist-initial-height"    ,(list(*)(...))VcandInitHeight},
  {"candlist-max-width"         ,(list(*)(...))VcandMaxWidth},
  {"candlist-max-height"        ,(list(*)(...))VcandMaxHeight},
  {"toolbar-icon-size"          ,(list(*)(...))VstatusSize},
#endif
  {0				,0},
};

static void
defcannavar(void)
{
  struct cannavardefs *p;

  for (p = cannavars ; p->varname ; p++) {
    symbolpointer(getatmz(p->varname))->valfunc = p->varfunc;
  }
}



/* ¥â¡¼¥ÉÉ½ */

static struct cannamodedefs cannamodes[] = {
  {S_AlphaMode			,CANNA_MODE_AlphaMode},
  {S_YomiganaiMode		,CANNA_MODE_EmptyMode},
  {S_YomiMode			,CANNA_MODE_YomiMode},
  {S_MojishuMode		,CANNA_MODE_JishuMode},
  {S_TankouhoMode		,CANNA_MODE_TankouhoMode},
  {S_IchiranMode		,CANNA_MODE_IchiranMode},
  {S_KigouMode			,CANNA_MODE_KigoMode},
  {S_YesNoMode			,CANNA_MODE_YesNoMode},
  {S_OnOffMode			,CANNA_MODE_OnOffMode},
  {S_ShinshukuMode		,CANNA_MODE_AdjustBunsetsuMode},

  {S_AutoYomiMode		,CANNA_MODE_ChikujiYomiMode},
  {S_AutoBunsetsuMode		,CANNA_MODE_ChikujiTanMode},

  {S_HenkanNyuuryokuMode	,CANNA_MODE_HenkanNyuryokuMode},
  {S_HexMode			,CANNA_MODE_HexMode},
  {S_BushuMode			,CANNA_MODE_BushuMode},
  {S_ExtendMode			,CANNA_MODE_ExtendMode},
  {S_RussianMode		,CANNA_MODE_RussianMode},
  {S_GreekMode			,CANNA_MODE_GreekMode},
  {S_LineMode			,CANNA_MODE_LineMode},
  {S_ChangingServerMode		,CANNA_MODE_ChangingServerMode},
  {S_HenkanMethodMode		,CANNA_MODE_HenkanMethodMode},
  {S_DeleteDicMode		,CANNA_MODE_DeleteDicMode},
  {S_TourokuMode		,CANNA_MODE_TourokuMode},
  {S_TourokuHinshiMode		,CANNA_MODE_TourokuHinshiMode},
  {S_TourokuDicMode		,CANNA_MODE_TourokuDicMode},
  {S_QuotedInsertMode		,CANNA_MODE_QuotedInsertMode},
  {S_BubunMuhenkanMode		,CANNA_MODE_BubunMuhenkanMode},
  {S_MountDicMode		,CANNA_MODE_MountDicMode},
  {S_ZenHiraHenkanMode		,CANNA_MODE_ZenHiraHenkanMode},
  {S_HanHiraHenkanMode		,CANNA_MODE_HanHiraHenkanMode},
  {S_ZenKataHenkanMode		,CANNA_MODE_ZenKataHenkanMode},
  {S_HanKataHenkanMode		,CANNA_MODE_HanKataHenkanMode},
  {S_ZenAlphaHenkanMode		,CANNA_MODE_ZenAlphaHenkanMode},
  {S_HanAlphaHenkanMode		,CANNA_MODE_HanAlphaHenkanMode},
  {S_ZenHiraKakuteiMode		,CANNA_MODE_ZenHiraKakuteiMode},
  {S_HanHiraKakuteiMode		,CANNA_MODE_HanHiraKakuteiMode},
  {S_ZenKataKakuteiMode		,CANNA_MODE_ZenKataKakuteiMode},
  {S_HanKataKakuteiMode		,CANNA_MODE_HanKataKakuteiMode},
  {S_ZenAlphaKakuteiMode	,CANNA_MODE_ZenAlphaKakuteiMode},
  {S_HanAlphaKakuteiMode	,CANNA_MODE_HanAlphaKakuteiMode},
  {0				,0},
};

static void
defcannamode(void)
{
  struct cannamodedefs *p;

  for (p = cannamodes ; p->mdname ; p++) {
    symbolpointer(getatmz(p->mdname))->mid = p->mdid;
  }
}



/* µ¡Ç½É½ */

static struct cannafndefs cannafns[] = {
  {S_FN_Undefined		,CANNA_FN_Undefined},
  {S_FN_SelfInsert		,CANNA_FN_FunctionalInsert},
  {S_FN_QuotedInsert		,CANNA_FN_QuotedInsert},
  {S_FN_JapaneseMode		,CANNA_FN_JapaneseMode},
  {S_AlphaMode			,CANNA_FN_AlphaMode},
  {S_HenkanNyuuryokuMode	,CANNA_FN_HenkanNyuryokuMode},
  {S_HexMode			,CANNA_FN_HexMode},
  {S_BushuMode			,CANNA_FN_BushuMode},
  {S_KigouMode			,CANNA_FN_KigouMode},
  {S_FN_Forward			,CANNA_FN_Forward},
  {S_FN_Backward		,CANNA_FN_Backward},
  {S_FN_Next			,CANNA_FN_Next},
  {S_FN_Prev			,CANNA_FN_Prev},
  {S_FN_BeginningOfLine		,CANNA_FN_BeginningOfLine},
  {S_FN_EndOfLine		,CANNA_FN_EndOfLine},
  {S_FN_DeleteNext		,CANNA_FN_DeleteNext},
  {S_FN_DeletePrevious		,CANNA_FN_DeletePrevious},
  {S_FN_KillToEndOfLine		,CANNA_FN_KillToEndOfLine},
  {S_FN_Henkan			,CANNA_FN_Henkan},
  {S_FN_HenkanNaive		,CANNA_FN_HenkanOrInsert}, /* for compati */
  {S_FN_HenkanOrSelfInsert	,CANNA_FN_HenkanOrInsert},
  {S_FN_HenkanOrDoNothing	,CANNA_FN_HenkanOrNothing},
  {S_FN_Kakutei			,CANNA_FN_Kakutei},
  {S_FN_Extend			,CANNA_FN_Extend},
  {S_FN_Shrink			,CANNA_FN_Shrink},
  {S_ShinshukuMode		,CANNA_FN_AdjustBunsetsu},
  {S_FN_Quit			,CANNA_FN_Quit},
  {S_ExtendMode			,CANNA_FN_ExtendMode},
  {S_FN_Touroku			,CANNA_FN_Touroku},
  {S_FN_ConvertAsHex		,CANNA_FN_ConvertAsHex},
  {S_FN_ConvertAsBushu		,CANNA_FN_ConvertAsBushu},
  {S_FN_KouhoIchiran		,CANNA_FN_KouhoIchiran},
  {S_FN_BubunMuhenkan		,CANNA_FN_BubunMuhenkan},
  {S_FN_Zenkaku			,CANNA_FN_Zenkaku},
  {S_FN_Hankaku			,CANNA_FN_Hankaku},
  {S_FN_ToUpper			,CANNA_FN_ToUpper},
  {S_FN_Capitalize		,CANNA_FN_Capitalize},
  {S_FN_ToLower			,CANNA_FN_ToLower},
  {S_FN_Hiragana		,CANNA_FN_Hiragana},
  {S_FN_Katakana		,CANNA_FN_Katakana},
  {S_FN_Romaji			,CANNA_FN_Romaji},
  {S_FN_KanaRotate		,CANNA_FN_KanaRotate},
  {S_FN_RomajiRotate		,CANNA_FN_RomajiRotate},
  {S_FN_CaseRotate		,CANNA_FN_CaseRotate},
  {S_FN_BaseHiragana		,CANNA_FN_BaseHiragana},
  {S_FN_BaseKatakana		,CANNA_FN_BaseKatakana},
  {S_FN_BaseKana		,CANNA_FN_BaseKana},	
  {S_FN_BaseEisu		,CANNA_FN_BaseEisu},	
  {S_FN_BaseZenkaku		,CANNA_FN_BaseZenkaku},
  {S_FN_BaseHankaku		,CANNA_FN_BaseHankaku},
  {S_FN_BaseKakutei		,CANNA_FN_BaseKakutei},
  {S_FN_BaseHenkan		,CANNA_FN_BaseHenkan},
  {S_FN_BaseHiraKataToggle	,CANNA_FN_BaseHiraKataToggle},
  {S_FN_BaseZenHanToggle	,CANNA_FN_BaseZenHanToggle},
  {S_FN_BaseKanaEisuToggle	,CANNA_FN_BaseKanaEisuToggle},
  {S_FN_BaseKakuteiHenkanToggle	,CANNA_FN_BaseKakuteiHenkanToggle},
  {S_FN_BaseRotateForward	,CANNA_FN_BaseRotateForward},
  {S_FN_BaseRotateBackward	,CANNA_FN_BaseRotateBackward},
  {S_FN_Mark			,CANNA_FN_Mark},
  {S_FN_Temporary		,CANNA_FN_TemporalMode},
  {S_FN_SyncDic			,CANNA_FN_SyncDic},
  {S_RussianMode		,CANNA_FN_RussianMode},
  {S_GreekMode			,CANNA_FN_GreekMode},
  {S_LineMode			,CANNA_FN_LineMode},
  {S_FN_DefineDicMode		,CANNA_FN_DefineDicMode},
  {S_FN_DeleteDicMode		,CANNA_FN_DeleteDicMode},
  {S_FN_DicMountMode		,CANNA_FN_DicMountMode},
  {S_FN_EnterChikujiMode	,CANNA_FN_EnterChikujiMode},
  {S_FN_EnterRenbunMode		,CANNA_FN_EnterRenbunMode},
  {S_FN_DisconnectServer	,CANNA_FN_DisconnectServer},
  {S_FN_ChangeServerMode	,CANNA_FN_ChangeServerMode},
  {S_FN_ShowServer		,CANNA_FN_ShowServer},
  {S_FN_ShowGakushu		,CANNA_FN_ShowGakushu},
  {S_FN_ShowVersion		,CANNA_FN_ShowVersion},
  {S_FN_ShowPhonogramFile	,CANNA_FN_ShowPhonogramFile},
  {S_FN_ShowCannaFile		,CANNA_FN_ShowCannaFile},
  {S_FN_PageUp			,CANNA_FN_PageUp},
  {S_FN_PageDown		,CANNA_FN_PageDown},
  {S_FN_Edit			,CANNA_FN_Edit},
  {S_FN_BubunKakutei		,CANNA_FN_BubunKakutei},
  {S_FN_HenkanRegion		,CANNA_FN_HenkanRegion},
  {S_FN_PhonoEdit		,CANNA_FN_PhonoEdit},
  {S_FN_DicEdit			,CANNA_FN_DicEdit},
  {S_FN_Configure		,CANNA_FN_Configure},
  {S_FN_KanaRotate		,CANNA_FN_KanaRotate},
  {S_FN_RomajiRotate		,CANNA_FN_RomajiRotate},
  {S_FN_CaseRotate		,CANNA_FN_CaseRotate},
  {0				,0},
};

static void
defcannafunc(void)
{
  struct cannafndefs *p;

  for (p = cannafns ; p->fnname ; p++) {
    symbolpointer(getatmz(p->fnname))->fid = p->fnid;
  }
}


static void
defatms(void)
{
  deflispfunc();
  defcannavar();
  defcannamode();
  defcannafunc();
  QUOTE		= getatmz("quote");
  T		= getatmz("t");
  _LAMBDA	= getatmz("lambda");
  _MACRO	= getatmz("macro");
  COND		= getatmz("cond");
  USER		= getatmz(":user");
  BUSHU		= getatmz(":bushu");
  RENGO		= getatmz(":rengo");
  KATAKANA	= getatmz(":katakana");
  HIRAGANA	= getatmz(":hiragana");
  GRAMMAR       = getatmz(":grammar");
  HYPHEN	= getatmz("-");
  symbolpointer(T)->value = T;
}

