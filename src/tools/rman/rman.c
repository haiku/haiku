static char cvsid[] = "$Header: /Users/phelps/cvs/prj/RosettaMan/rman.c,v 1.154 2003/07/26 19:00:48 phelps Exp $";

/*
   PolyglotMan by Thomas A. Phelps (phelps@ACM.org)

  accept man pages as formatted by (10)
     Hewlett-Packard HP-UX, AT&T System V, SunOS, Sun Solaris, OSF/1, 
     DEC Ultrix, SGI IRIX, Linux, FreeBSD, SCO

  output as (9)
     printable ASCII, section headers only, TkMan, [tn]roff, HTML,
     LaTeX, LaTeX2e, RTF, Perl pod, MIME, DocBook XML

  written March 24, 1993
	bs2tk generalized into RosettaMan November 4-5, 1993
	source interpretation added September 24, 1996
	renamed PolyglotMan due to lawsuit by Rosetta, Inc. August 8, 1997
*/

#define VOLLIST "1:2:3:4:5:6:7:8:9:o:l:n:p"
#define MANTITLEPRINTF "%s(%s) manual page"
#define MANREFPRINTF "%s.%s"
#define POLYGLOTMANVERSION "3.2"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/*** make #define's into consts? => can't because compilers not smart enough ***/
/* maximum number of tags per line */
#define MAXTAGS 50*100
#define MAXBUF 2*5000
#define MAXLINES 20000
#define MAXTOC 500
#define xputchar(c)		(fcharout? putchar(c): (c))
#define sputchar(c)		(fcharout? plain[sI++]=(char)c: (char)(c))
#define stagadd(tag)	tagadd(tag,sI,0)
enum { c_dagger='\xa7', c_bullet='\xb7', c_plusminus='\xb1' };


/*** tag management ***/

enum tagtype { NOTAG, TITLE, ITALICS, BOLD, SYMBOL, SMALLCAPS, BOLDITALICS, MONO, MANREF };	/* MANREF last */
struct { enum tagtype type; int first; int last; } tags[MAXTAGS], tagtmp;
int tagc=0;
struct { char *text; int type; int line; } toc[MAXTOC];
int tocc=0;


/* characters in this list automatically prefixed by a backslash (set in output format function */
char *escchars="";
char *vollist = VOLLIST;
const char *manvalid = "._-+:";	/* in addition to alphanumerics, valid characters to find in a man page name */
char *manrefname;
char *manrefsect;

enum command {

	/*BEGINCHARTAGS,*/
	CHARTAB='\t',
	CHARPERIOD='.', CHARLSQUOTE='`', CHARRSQUOTE='\'', CHARGT='>', CHARLT='<',
	CHARAMP='&', CHARBACKSLASH='\\', CHARDASH='-', CHARHAT='^', CHARVBAR='|',
 	CHARNBSP=0xa0,	CHARCENT=0xa2, CHARSECT=0xa7, CHARCOPYR=0xa9, CHARNOT=0xac,
 	CHARDAGGER=0xad, CHARREGTM=0xae, CHARDEG=0xb0, CHARPLUSMINUS=0xb1,
 	CHARACUTE=0xb4, CHARBULLET=0xb7, CHAR14=0xbc, CHAR12=0xbd, CHAR34=0xbe,
 	CHARMUL=0xd7, CHARDIV=0xf7,
	CHANGEBAR=0x100, CHARLQUOTE, CHARRQUOTE, 
	/*ENDCHARTAGS,*/

	/*BEGINFONTTAGS,*/
	BEGINBOLD, ENDBOLD, BEGINITALICS, ENDITALICS, BEGINBOLDITALICS, ENDBOLDITALICS,
	BEGINSC, ENDSC, BEGINY, ENDY, BEGINCODE, ENDCODE, BEGINMANREF, ENDMANREF,
	FONTSIZE,
	/*ENDFONTTAGS*/

	/*BEGINLAYOUTTAGS,*/
	ITAB, BEGINCENTER, ENDCENTER, HR,
	/*ENDLAYOUTTAGS,*/

	/*BEGINSTRUCTTAGS,*/
	BEGINDOC, ENDDOC, BEGINCOMMENT, ENDCOMMENT, COMMENTLINE, BEGINBODY, ENDBODY, 
	BEGINHEADER, ENDHEADER, BEGINFOOTER, ENDFOOTER, BEGINLINE, ENDLINE, SHORTLINE,
	BEGINSECTION, ENDSECTION, BEGINSUBSECTION, ENDSUBSECTION,
	BEGINSECTHEAD, ENDSECTHEAD, BEGINSUBSECTHEAD, ENDSUBSECTHEAD,
	BEGINBULPAIR, ENDBULPAIR, BEGINBULLET, ENDBULLET, BEGINBULTXT, ENDBULTXT,
	BEGINTABLE, ENDTABLE, BEGINTABLELINE, ENDTABLELINE, BEGINTABLEENTRY, ENDTABLEENTRY,
	BEGININDENT, ENDINDENT, BEGINCODEBLOCK, ENDCODEBLOCK,

	BEGINDIFFA, ENDDIFFA, BEGINDIFFD, ENDDIFFD
	/*,*//*ENDSTRUCTTAGS,*/
};

const char *tcltkOP[] = { "Command-Line Name", "Database Name", "Database Class" };


/* characters that need special handling in any output format, *more than just a backslash* */
/* characters in this list need a corresponding case statement in each output format */
/*char *trouble="\t.`'><&\\^|-\xa7\xb7\xb1";*/
const unsigned char trouble[]= { CHARTAB, CHARPERIOD, CHARLSQUOTE, CHARRSQUOTE,
	CHARGT, CHARLT, CHARAMP, CHARBACKSLASH, CHARDASH, CHARHAT, CHARVBAR, CHARCENT,
	CHARSECT, CHARCOPYR, CHARNOT, CHARDAGGER, CHARREGTM, CHARDEG, CHARPLUSMINUS,
	CHARACUTE, CHARBULLET, CHAR14, CHAR12, CHAR34, CHARMUL, CHARDIV,
	0 };


enum command tagbeginend[][2] = {	/* parallel to enum tagtype */
	{ -1,-1 },
	{ -1,-1 },
	{ BEGINITALICS, ENDITALICS },
	{ BEGINBOLD, ENDBOLD },
	{ BEGINY, ENDY },
	{ BEGINSC, ENDSC },
	{ BEGINBOLDITALICS, ENDBOLDITALICS },
	{ -1,-1 },
	{ BEGINMANREF, ENDMANREF }
};

void (*fn)(enum command) = NULL;
enum command prevcmd = BEGINDOC;


/*** globals ***/

int fSource=-1;	/* -1 => not determined yet */
int finlist=0;
int fDiff=0;
FILE *difffd;
char diffline[MAXBUF];
char diffline2[MAXBUF];
char *message = NULL;
int fontdelta=0;
int intArg;

int fPara=0;			/* line or paragraph groupings of text */
int fSubsections=0;	/* extract subsection titles too? */
int fChangeleft=0;	/* move change bars to left? (-1 => delete them) */
int fReflow=0;
int fURL=0;		/* scan for URLs too? */
/*int fMan=1;		/* invoke agressive man page filtering? */
int fQS=0;		/* squeeze out spaces (scnt and interword)? */
int fIQS=0;		/* squeeze out initial spaces (controlled separately from fQS) */
int fILQS=0;		/* squeeze out spaces for usual indent */
int fHeadfoot=0;	/* show canonical header and footer at bottom? */
int falluc=0;
int itabcnt=0;
int fQuiet=0;
int fTclTk=0;

/* patterns observed in section heads that don't conform to first-letter-uppercase-rest-lowercase pattern (stay all uc, or go all lc, or have subsequent uc) */
int lcexceptionslen = -1;	/* computed by system */
char *lcexceptions[] = {
/* new rule: double/all consonants == UC? */
	/* articles, verbs, conjunctions, prepositions, pronouns */
	"a", "an", "the", 
	"am", "are", "is", "were",
	"and", "or",
	"by", "for", "from", "in", "into", "it", "of", "on", "to", "with",
	"that", "this",

	/* terms */
	"API", "CD", "GUI", "UI", /*I/O=>I/O already*/ "ID", "IDs", "OO",
	"IOCTLs", "IPC", "RPC",

	/* system names */
	"AWK", "cvs", "rcs", "GL", "vi", "PGP", "QuickTime", "DDD", "XPG/3",
	"NFS", "NIS", "NIS+", "AFS", 
	"UNIX", "SysV", 
	"XFree86", "ICCCM",
	"MH", "MIME",
	"TeX", "LaTeX", "PicTeX",
	"PostScript", "EPS", "EPSF", "EPSI", 
	"HTML", "URL", "WWW",

	/* institution names */
	"ANSI", "CERN", "GNU", "ISO", "NCSA",

	/* Sun-specific */	
	"MT-Level", "SPARC",

	NULL
};


int TabStops=8;
int hanging=0;		/* location of hanging indent (if ==0, none) */
enum { NAME, SYNOPSIS, DESCRIPTION, SEEALSO, FILES, AUTHOR, RANDOM/*last!*/ };
char *sectheadname[] = {
  "NAME:NOMBRE", "SYNOPSIS", "DESCRIPTION:INTRODUCTION", "SEE ALSO:RELATED INFORMATION", "FILES", "AUTHOR:AUTHORS", "RANDOM"
};
int sectheadid = RANDOM;
int oldsectheadid = RANDOM;

int fCodeline=0;
int fNOHY=0;		/* re-linebreak so no words are hyphenated; not used by TkMan, but gotta keep for people converting formatted text */
int fNORM=0;		/* normalize?  initial space => tabs, no changebars, exactly one blank line between sections */
const char TABLEOFCONTENTS[] = "Table of Contents";
const char HEADERANDFOOTER[] = "Header and Footer";
char manName[80] = "man page";
char manSect[10] = "1";
const char PROVENANCE[] =
	"manual page source format generated by PolyglotMan v" POLYGLOTMANVERSION;
const char HOME[] = "available at http://polyglotman.sourceforge.net/";
const char horizontalrule[] = "------------------------------------------------------------";

const int LINEBREAK = 70;
int linelen = 0;		/* length of result in plain[] */
int spcsqz;		/* number of spaces squeezed out */
int ccnt = 0;	/* # of changebars */
int scnt, scnt2;	/* counts of initial spaces in line */
int s_sum, s_cnt;
int bs_sum, bs_cnt;
int ncnt=0, oncnt=0;	/* count of interline newlines */
int CurLine=1;
int AbsLine=1-1;	/* absolute line number */
int indent=0;		/* global indentation */
int lindent=0;		/* usual local indent */
int auxindent=0;	/* aux indent */
int I;			/* index into line/paragraph */
int fcharout=1;		/* show text or not */
char lookahead;
/*int tabgram[MAXBUF];*/	/* histogram of first character positions */
char buf[MAXBUF];
char plain[MAXBUF];		/* current text line with control characters stripped out */
char hitxt[MAXBUF];		/* highlighted text (available at time of BEGIN<highlight> signal */

char header[MAXBUF];	/* complete line */
char header2[MAXBUF];	/* SGIs have two lines of headers and footers */
char header3[MAXBUF];	/* GNU and some others have a third! */
char footer[MAXBUF];
char footer2[MAXBUF];
#define CRUFTS 5
char *cruft[CRUFTS] = { header, header2, header3, footer, footer2 };

char *File, *in;		/* File = pointer to full file contents, in = current file pointer */
char *argv0;
int finTable=0;
char tableSep='\0';	/*\t';*/
/*int fTable=0;
int fotable=0;*/
char *tblcellformat;
int tblcellspan;
/*int tblspanmax;*/
int listtype=-1;	/* current list type bogus to begin with */
enum listtypes { DL, OL, UL };

int fIP=0;



/*** utility functions ***/


/* case insensitive versions of strcmp and strncmp */

int
stricmp(const char *s1, const char *s2) {
	assert(s1!=NULL && s2!=NULL);
	/*strincmp(s1, s2, strlen(s1)+1);*/

	while (tolower(*s1)==tolower(*s2)) {
		if (*s1=='\0' /*&& *s2=='\0'*/) return 0;
		s1++; s2++;
	}

	if (tolower(*s1)<tolower(*s2)) return -1;
	else return 1;
}

int lcexceptionscmp(const char **a, const char **b) { return stricmp(*a, *b); }

int
strincmp(const char *s1, const char *s2, size_t n) {
	assert(s1!=NULL && s2!=NULL && n>0);

	while (n>0 && tolower(*s1)==tolower(*s2)) {
		n--; s1++; s2++;
	}
	if (n==0) return 0;
	else if (tolower(*s1)<tolower(*s2)) return -1;
	else return 1;
}

/* compare string and a colon-separated list of strings */
int
strcoloncmp2(char *candidate, int end, const char *list, int sen) {
	const char *l = list;
	char *c,c2;

	assert(candidate!=NULL && list!=NULL);
	assert(end>=-1 && end<=255);
	assert(sen==0 || sen==1);

	if (*l==':') l++;	/* tolerate a leading colon */

	/* invariant: c and v point to start of strings to compare */
	while (*l) {
		assert(l==list || l[-1]==':');
		for (c=candidate; *c && *l; c++,l++)
			if ((sen && *c!=*l) || (!sen && tolower(*c)!=tolower(*l)))
				break;

		/* if candidate matches a valid one as far as valid goes, it's a keeper */
		if ((*l=='\0' || *l==':') && (*c==end || end==-1)) {
		  if (*c=='\b') {
		    c2 = c[-1];
		    while (*c=='\b' && c[1]==c2) c+=2;
		  }
		  /* no volume qualifiers with digits */
		  if (!isdigit(*c)) return 1;
		}

		/* bump to start of next valid */
		while (*l && *l++!=':') /* nada */;
	}

	return 0;
}

int
strcoloncmp(char *candidate, int end, const char *list) {
	int sen=1;
	const char *l = list;

	assert(candidate!=NULL && list!=NULL);
	assert(end>=-1 && end<=255);

	if (*l=='=') l++; else end=-1;
	if (*l=='i') { sen=0; l++; }

	return strcoloncmp2(candidate, end, l, sen);
}

/* strdup not universally available */
char *
mystrdup(char *p) {
  char *q;

  if (p==NULL) return NULL;

  q = malloc(strlen(p)+1);	/* +1 gives space for \0 that is not reported by strlen */
  if (q!=NULL) strcpy(q,p);
  return q;
}


/* given line of text, return "casified" version in place:
   if word in exceptions list, return exception conversion
   else uc first letter, lc rest
*/
void casify(char *p) {
	char tmpch, *q, **exp;
	int fuc;

	for (fuc=1; *p; p++) {
		if (isspace(*p) || strchr("&/",*p)!=NULL) fuc=1;
		else if (fuc) {
			/* usually */
				if (p[1] && isupper(p[1]) /*&& p[2] && isupper(p[2])*/) fuc=0;
			/* check for exceptions */
			for (q=p; *q && !isspace(*q); q++) /*nada*/;
			tmpch = *q; *q='\0';
			exp = (char **)bsearch(&p, lcexceptions, lcexceptionslen, sizeof(char *), lcexceptionscmp);
			*q = tmpch;
			if (exp!=NULL) {
				for (q=*exp; *q; q++) *p++=*q;
				fuc = 1;
			}
		} else *p=tolower(*p);
	}
}


/* add an attribute tag to a range of characters */

void
tagadd(int /*enum tagtype--abused in source parsing*/ type, int first, int last) {
	assert(type!=NOTAG);

	if (tagc<MAXTAGS) {
		tags[tagc].type = type;
		tags[tagc].first = first;
		tags[tagc].last = last;
		tagc++;
	}
}


/*
   collect all saves to string table one one place, so that
   if decide to go with string table instead of multiple malloc, it's easy
   (probably few enough malloc's that more sophistication is unnecessary)
*/

void
tocadd(char *text, enum command type, int line) {
	char *r;

	assert(text!=NULL && strlen(text)>0);
	assert(type==BEGINSECTION || type==BEGINSUBSECTION);

	if (tocc<MAXTOC) {
		r = malloc(strlen(text)+1); if (r==NULL) return;
		strcpy(r,text);
		toc[tocc].text = r;
		toc[tocc].type = type;
		toc[tocc].line = line;
		tocc++;
	}
}



char *manTitle = MANTITLEPRINTF;
char *manRef = MANREFPRINTF;
char *href;
int fmanRef=1;	/* make 'em links or just show 'em? */

void
manrefextract(char *p) {
  char *p0;
  static char *nonhref = "\">'";

  while (*p==' ') p++;
  if (strincmp(p,"http",4)==0) {
	href="%s"; manrefname = p;
	p+=4;
	while (*p && !isspace(*p) && !strchr(nonhref,*p)) p++;
  } else {
	href = manRef;

	manrefname = p;
	while (*p && *p!=' ' && *p!='(') p++; *p++='\0';
	while (*p==' ' || *p=='(') p++; p0=p;
	while (*p && *p!=')') p++;
	manrefsect = p0;
  }
  *p='\0';
}




/*
 * OUTPUT FORMATS
 */

void
formattedonly(void) {
	fprintf(stderr, "The output formats for Tk and TkMan require nroff-formatted input\n");
	exit(1);
}


/*
 * DefaultFormat -- in weak OO inheritance, top of hierarchy for everybody
 */
void
DefaultFormat(enum command cmd) {
  int i;
  
  switch (cmd) {
  case ITAB:
    for (i=0; i<itabcnt; i++) putchar('\t');
    break;
  default:
    /* nada */
    break;
  }
}


/*
 * DefaultLine -- in weak OO inheritance, top of hierarchy for line-based formats
 * for output format to "inherit", have "default: DefaultLine(cmd)" and override case statement "methods"
 */

void
DefaultLine(enum command cmd) {
	switch (cmd) {
	default:
	  DefaultFormat(cmd);
	}
}


/*
 * DefaultPara -- top of hierarchy for output formats that are formatted by their viewers
 */

void
DefaultPara(enum command cmd) {
	switch (cmd) {
	default:
	  DefaultFormat(cmd);
	}
}



/*
 * Tk -- just emit list of text-tags pairs
 */

void
Tk(enum command cmd) {
	static int skip=0;	/* skip==1 when line has no text */
	int i;

	if (fSource) formattedonly();

	/* invariant: always ready to insert text */

	switch (cmd) {
	   case BEGINDOC:
		I=0; CurLine=1;
		escchars = "\"[]$";
		printf(/*$t insert end */ "\"");
		break;
	   case ENDDOC:
		if (fHeadfoot) {
/*	grr, should have +mark syntax for Tk text widget! -- maybe just just +sect#, +subsect#
		printf("\\n\\n\" {} \"%s\\n\" {+headfoot h2}\n", HEADERANDFOOTER);
*/
			printf("\\n\\n\" {} \"%s\\n\" h2\n",HEADERANDFOOTER);
			/*printf("$t mark set headfoot %d.0\n", CurLine);*/
			CurLine++;

			for (i=0; i<CRUFTS; i++) {
				if (*cruft[i]) {
					printf(/*$t insert end */"{%s} sc \\n\n", cruft[i]);
					CurLine++;
				}
			}
		} else printf("\"\n");
		break;

	   case COMMENTLINE: printf("# "); break;

	   case BEGINLINE:
		/*I=0; -- need to do this at end of line so set for filterline() */
		/* nothing to do at start of line except catch up on newlines */
		for (i=0; i<ncnt; i++) printf("\\n");
		CurLine+=ncnt;
		/*if (fSource) for (i=0; i<indent; i++) putchar('\t');*/
		break;
	   case ENDLINE:
		/*if (!fSource) {*/
		  if (!skip) /*if (ncnt)*/ printf("\\n"); /*else xputchar(' ');*/
		  skip=0;
		  CurLine++; I=0;
		/*
		} else {
		  putchar(' '); I++;
		}
		*/
		break;

	   case ENDSECTHEAD:
		printf("\\n\" h2 \"");
		tagc=0;
		skip=1;
		break;
	   case ENDSUBSECTHEAD:
		printf("\\n\" h3 \"");	/* add h3? */
		tagc=0;
		skip=1;
		break;
	   case HR:			/*printf("\\n%s\\n", horizontalrule); CurLine+=2; I=0;*/ break;
	   case BEGINTABLEENTRY:
		/*if (fSource) putchar('\t');*/
		break;
	   case BEGINTABLELINE:
	   case ENDTABLEENTRY:
		break;
	   case ENDTABLELINE:
		printf("\" tt \"");
		/*tagadd(MONO, 0, I);*/
		break;

	   case CHANGEBAR:		putchar('|'); I++; break;
	   case CHARLQUOTE:
	   case CHARRQUOTE:
		putchar('\\'); putchar('"'); I++;
		break;
	   case CHARLSQUOTE:
	   case CHARRSQUOTE:
	   case CHARPERIOD:
	   case CHARTAB:
	   case CHARDASH:
	   case CHARLT:
	   case CHARGT:
	   case CHARHAT:
	   case CHARVBAR:
	   case CHARAMP:
	   case CHARPLUSMINUS:
	   case CHARNBSP:
 	   case CHARCENT:
 	   case CHARSECT:
 	   case CHARCOPYR:
 	   case CHARNOT:
 	   case CHARREGTM:
 	   case CHARDEG:
 	   case CHARACUTE:
 	   case CHAR14:
 	   case CHAR12:
 	   case CHAR34:
 	   case CHARMUL:
 	   case CHARDIV:
  		putchar(cmd); I++; break;
 	   case CHARDAGGER:
 		putchar('+'); I++; break;
	   case CHARBACKSLASH:	printf("\\\\"); I++; break;
	   case CHARBULLET:		printf("\" {} %c symbol \"",c_bullet); I++; break;


	   case BEGINSECTHEAD:
	   case BEGINSUBSECTHEAD:
		/*if (fSource && sectheadid!=NAME) { printf("\\n\\n"); CurLine+=2; I=0; }*/
		tagc=0;	/* section and subsection formatting controlled descriptively */
		/* no break;*/

	   case BEGINBOLD:
	   case BEGINITALICS:
	   case BEGINBOLDITALICS:
	   case BEGINCODE:
	   case BEGINY:
	   case BEGINSC:
	   case BEGINMANREF:
		/* end text, begin attributed text */
		printf("\" {} \"");
		break;

	   /* rely on the fact that no more than one tag per range of text */
	   case ENDBOLD:		printf("\" b \""); break;
	   case ENDITALICS:		printf("\" i \""); break;
	   case ENDBOLDITALICS:	printf("\" bi \""); break;
	   case ENDCODE:		printf("\" tt \""); break;
	   case ENDY:			printf("\" symbol \""); break;
	   case ENDSC:			printf("\" sc \""); break;
	   case ENDMANREF:		printf("\" manref \""); break;
		/* presentation attributes dealt with at end of line */

	   case BEGINBODY:
		/*if (fSource) { printf("\\n\\n"); CurLine+=2; I=0; }*/
		break;
	   case SHORTLINE:
		/*if (fSource) { printf("\\n"); CurLine++; I=0; }*/
		break;
	   case ENDBODY:
	   case BEGINBULPAIR: case ENDBULPAIR:
		/*if (fSource) { printf("\\n"); CurLine++; I=0; }*/
		break;
	   case BEGINBULTXT:
		/*if (fSource) putchar('\t');*/
		break;
	   case BEGINBULLET: case ENDBULLET:
	   case ENDBULTXT:
	   case BEGINSECTION: case ENDSECTION:
	   case BEGINSUBSECTION: case ENDSUBSECTION:
	   case BEGINHEADER: case ENDHEADER:
	   case BEGINFOOTER: case ENDFOOTER:
	   case BEGINTABLE: case ENDTABLE:
	   case FONTSIZE:
	   case BEGININDENT: case ENDINDENT:
		/* no action */
		break;
	   default:
		DefaultLine(cmd);
	}
}




/*
 * TkMan -- Tk format wrapped with commands
 */

int linetabcnt[MAXLINES];	/* don't want to bother with realloc */
int clocnt=0, clo[MAXLINES];
int paracnt=0, para[MAXLINES];
int rebuscnt=0, rebus[MAXLINES];
int rebuspatcnt=0, rebuspatlen[25];
char *rebuspat[25];

void
TkMan(enum command cmd) {
	static int lastscnt=-1;
	static int lastlinelen=-1;
	static int lastsect=0;
	/*static int coalese=0;*/
	static int finflow=0;
	int i;
	char c,*p;

	/* invariant: always ready to insert text */

	switch (cmd) {
	   case BEGINDOC:
		printf("$t insert end ");	/* opening quote supplied in Tk() below */
		Tk(cmd);
		break;
	   case ENDDOC:
		Tk(ENDLINE);

		if (fHeadfoot) {
/*	grr, should have +mark syntax for Tk text widget!
		printf("\\n\\n\" {} \"%s\\n\" {+headfoot h2}\n", HEADERANDFOOTER);
*/
			printf("\\n\\n\" {} \"%s\\n\" h2\n", HEADERANDFOOTER);
/*			printf("$t mark set headfoot end-2l\n");*/
			CurLine++;

			for (i=0; i<CRUFTS; i++) {
				if (*cruft[i]) {
					printf("$t insert end {%s} sc \\n\n", cruft[i]);
					CurLine++;
				}
			}
		} else printf("\"\n");

/*
		printf("$t insert 1.0 {");
		for (i=0; i<MAXBUF; i++) if (tabgram[i]) printf("%d=%d, ", i, tabgram[i]);
		printf("\\n\\n}\n");
*/

		printf("set manx(tabcnts) {"); for (i=1; i<CurLine; i++) printf("%d ", linetabcnt[i]); printf("}\n");
		printf("set manx(clo) {"); for (i=0; i<clocnt; i++) printf("%d ", clo[i]); printf("}\n");
		printf("set manx(para) {"); for (i=0; i<paracnt; i++) printf("%d ", para[i]); printf("}\n");
		printf("set manx(reb) {"); for (i=0; i<rebuscnt; i++) printf("%d ", rebus[i]); printf("}\n");

		break;

	   case BEGINCOMMENT: fcharout=0; break;
	   case ENDCOMMENT: fcharout=1; break;
	   case COMMENTLINE: break;

	   case ENDSECTHEAD:
	   case ENDSUBSECTHEAD:
		lastsect=1;
		Tk(cmd);
		break;

	   case BEGINLINE:
	     Tk(cmd);
		linetabcnt[CurLine] = itabcnt;
		/* old pattern for command line options "^\\|*\[ \t\]+-\[^-\].*\[^ \t\]" */
		c = plain[0];
		if (linelen>=2 && ((c=='-' || c=='%' || c=='\\' || c=='$' /**/ /* not much talk of money in man pages so reasonable */) && (isalnum(plain[1]) /*<= plain[1]!='-'*//*no dash*/ || ncnt/*GNU long option*/) && plain[1]!=' ') ) clo[clocnt++] = CurLine;
		/*
		   would like to require second letter to be a capital letter to cut down on number of matches,
		   but command names usually start with lowercase letter
		   maybe use a uppercase requirement as secondary strategy, but probably not
		*/
		if ((ncnt || lastsect) && linelen>0 && scnt>0 && scnt<=7/*used to be <=5 until groff spontaneously started putting in 7*/) para[paracnt++] = CurLine;
		lastsect=0;


		/* rebus too, instead of search through whole Tk widget */
		if (rebuspatcnt && scnt>=5 /* not sect or subsect heads */) {
			for (p=plain; *p && *p!=' '; p++) /*empty*/;	/* never first word */
			while (*p) {
				for (i=0; i<rebuspatcnt; i++) {
					if (tolower(*p) == tolower(*rebuspat[i]) && strincmp(p, rebuspat[i], rebuspatlen[i])==0) {
						/* don't interfere with man page refs */
						for (; *p && !isspace(*p); p++) if (*p=='(') continue;
						rebus[rebuscnt++] = CurLine;
						p="";	/* break for outer */
						break;	/* just locating any line with any rebus, not exact positions */
					}
				}
				/* just check start of words, though doesn't have to be full word (if did, could use strlen rather than strnlen) */
				while (*p && *p!=' ') p++;
				while (*p && *p==' ') p++;
			}
		}


		if (fReflow && !ncnt && (finflow || lastlinelen>50) && (abs(scnt-lastscnt)<=1 || abs(scnt-hanging)<=1)) {
		  finflow=1;
		  putchar(' ');
		} else {
		  Tk(ENDLINE);
		  /*if ((CurLine&0x3f)==0x3f) printf("\"\nupdate idletasks\n$t insert end \""); blows up some Tk text buffer, apparently, on long lines*/
		  if ((CurLine&0x1f)==0x1f) printf("\"\nupdate idletasks\n$t insert end \"");
		  finflow=0;

		  /*if (fCodeline) printf("CODE");*/
		}
		lastlinelen=linelen; lastscnt=scnt;
		break;

	   case ENDLINE:
		/* don't call Tk(ENDLINE) */
		break;

	   default:	/* if not caught above, it's the same as Tk */
		Tk(cmd);
	}
}




/*
 * ASCII
 */

void
ASCII(enum command cmd) {
	int i;

	switch (cmd) {
	   case ENDDOC:
		if (fHeadfoot)	{
			printf("\n%s\n", HEADERANDFOOTER);
			for (i=0; i<CRUFTS; i++) if (*cruft[i]) printf("%s\n", cruft[i]);
		}
		break;
	   case CHARRQUOTE:
	   case CHARLQUOTE:
		putchar('"');
		break;
	   case CHARLSQUOTE:
		putchar('`');
		break;
	   case CHARRSQUOTE:
	   case CHARACUTE:
		putchar('\'');
		break;
	   case CHARPERIOD:
	   case CHARTAB:
	   case CHARDASH:
	   case CHARLT:
	   case CHARAMP:
	   case CHARBACKSLASH:
	   case CHARGT:
	   case CHARHAT:
	   case CHARVBAR:
	   case CHARNBSP:
		putchar(cmd); break;
	   case CHARDAGGER:	putchar('+'); break;
	   case CHARBULLET:	putchar('*'); break;
	   case CHARPLUSMINUS: printf("+-"); break;
	   case CHANGEBAR:	putchar('|'); break;
 	   case CHARCENT:	putchar('c'); break;
 	   case CHARSECT:	putchar('S'); break;
 	   case CHARCOPYR:	printf("(C)"); break;
 	   case CHARNOT:	putchar('~'); break;
 	   case CHARREGTM:	printf("(R)"); break;
 	   case CHARDEG:	putchar('o'); break;
 	   case CHAR14:		printf("1/4"); break;
 	   case CHAR12:		printf("1/2"); break;
 	   case CHAR34:		printf("3/4"); break;
 	   case CHARMUL:	putchar('X'); break;
 	   case CHARDIV:	putchar('/'); break;
	   case HR:		printf("\n%s\n", horizontalrule); break;

	   case BEGINLINE:
		for (i=0; i<ncnt; i++) putchar('\n');
		break;
	   case BEGINBODY:
	   case SHORTLINE:
		if (!fSource) break;
	   case ENDLINE:
		putchar('\n');
		CurLine++;
		break;

	   case BEGINDOC:
	   case ENDBODY:
	   case BEGINHEADER: case ENDHEADER:
	   case BEGINFOOTER: case ENDFOOTER:
	   case BEGINSECTION: case ENDSECTION:
	   case BEGINSECTHEAD: case ENDSECTHEAD:
	   case BEGINSUBSECTHEAD: case ENDSUBSECTHEAD:
	   case BEGINBULPAIR: case ENDBULPAIR:
	   case BEGINBULLET: case ENDBULLET:
	   case BEGINBULTXT: case ENDBULTXT:
	   case BEGINSUBSECTION: case ENDSUBSECTION:

	   case BEGINTABLE: case ENDTABLE:
	   case BEGINTABLELINE: case ENDTABLELINE: case BEGINTABLEENTRY: case ENDTABLEENTRY:
	   case BEGININDENT: case ENDINDENT:
	   case FONTSIZE:
	   case BEGINBOLD: case ENDBOLD:
	   case BEGINCODE: case ENDCODE:
	   case BEGINITALICS: case ENDITALICS:
	   case BEGINMANREF: case ENDMANREF:
	   case BEGINBOLDITALICS: case ENDBOLDITALICS:
	   case BEGINY: case ENDY:
	   case BEGINSC: case ENDSC:
		/* nothing */
		break;
	   default:
		DefaultLine(cmd);
	}
}



/*
 * Perl 5 pod ("plain old documentation")
 */

void
pod(enum command cmd) {
	static int curindent=0;
	int i;

	if (hanging==-1) {
		if (curindent) hanging=curindent; else hanging=5;
	}


	if (cmd==BEGINBULPAIR) {
		/* want to have multiply indented text */
		if (curindent && hanging!=curindent) printf("\n=back\n\n");
		if (hanging!=curindent) printf("\n=over %d\n\n",hanging);
		curindent=hanging;
	} else if (cmd==ENDBULPAIR) {
		/* nothing--wait until next command */
	} else if (cmd==BEGINLINE && !scnt) {
		if (curindent) printf("\n=back\n\n");
		curindent=0;
	} else if (cmd==BEGINBODY) {
		if (curindent) {
			printf("\n=back\n\n");
			curindent=0;
			auxindent=0;
		}
	}
/*
	   case BEGINBULPAIR:
		printf("=over %d\n\n", hanging);
		break;
	   case ENDBULPAIR:
		printf("\n=back\n\n");
		break;
*/
	switch (cmd) {
	   case BEGINDOC: I=0; break;

	   case BEGINCOMMENT: fcharout=0; break;
	   case ENDCOMMENT: fcharout=1; break;
	   case COMMENTLINE: break;

	   case CHARRQUOTE:
	   case CHARLQUOTE:
		putchar('"');
		break;
	   case CHARLSQUOTE:
		putchar('`');
		break;
	   case CHARRSQUOTE:
	   case CHARACUTE:
		putchar('\'');
		break;
	   case CHARPERIOD:
	   case CHARTAB:
	   case CHARDASH:
	   case CHARLT:
	   case CHARAMP:
	   case CHARBACKSLASH:
	   case CHARGT:
	   case CHARHAT:
	   case CHARVBAR:
	   case CHARNBSP:
		putchar(cmd); break;
	   case CHARDAGGER:	putchar('+'); break;
	   case CHARPLUSMINUS: printf("+-"); break;
	   case CHANGEBAR:	putchar('|'); break;
 	   case CHARCENT:	putchar('c'); break;
 	   case CHARSECT:	putchar('S'); break;
 	   case CHARCOPYR:	printf("(C)"); break;
 	   case CHARNOT:	putchar('~'); break;
	   case CHARREGTM:	printf("(R)"); break;
 	   case CHARDEG:	putchar('o'); break;
 	   case CHAR14:	printf("1/4"); break;
 	   case CHAR12:	printf("1/2"); break;
 	   case CHAR34:	printf("3/4"); break;
 	   case CHARMUL:	putchar('X'); break;
 	   case CHARDIV:	putchar('/'); break;
	   case HR:		printf("\n%s\n", horizontalrule); break;
	   case CHARBULLET:	putchar('*'); break;

	   case BEGINLINE:
		for (i=0; i<ncnt; i++) putchar('\n');
		CurLine+=ncnt;
		break;
	   case ENDLINE:
		putchar('\n');
		CurLine++;
		I=0;
		break;

	   case BEGINSECTHEAD:	printf("=head1 "); break;
	   case BEGINSUBSECTHEAD:	printf("=head2 "); break;

	   case ENDSECTHEAD:
	   case ENDSUBSECTHEAD:
		printf("\n");
		break;

	   case BEGINCODE:
	   case BEGINBOLD:		printf("B<"); break;
	   case BEGINITALICS:	printf("I<"); break;
	   case BEGINMANREF:	printf("L<"); break;

	   case ENDBOLD:
	   case ENDCODE:
	   case ENDITALICS:
	   case ENDMANREF:
		printf(">");
		break;

	   case BEGINBULLET:
		printf("\n=item ");
		break;
	   case ENDBULLET:
		printf("\n\n");
		fcharout=0;
		break;
	   case BEGINBULTXT:
		fcharout=1;
		auxindent=hanging;
		break;
	   case ENDBULTXT:
		auxindent=0;
		break;


	   case ENDDOC:
	   case BEGINBODY: case ENDBODY:
	   case BEGINHEADER: case ENDHEADER:
	   case BEGINFOOTER: case ENDFOOTER:
	   case BEGINSECTION: case ENDSECTION:
	   case BEGINSUBSECTION: case ENDSUBSECTION:
	   case BEGINBULPAIR: case ENDBULPAIR:

	   case SHORTLINE:
	   case BEGINTABLE: case ENDTABLE:
	   case BEGINTABLELINE: case ENDTABLELINE: case BEGINTABLEENTRY: case ENDTABLEENTRY:
	   case BEGININDENT: case ENDINDENT:
	   case FONTSIZE:
	   case BEGINBOLDITALICS: case ENDBOLDITALICS:
	   case BEGINY: case ENDY:
	   case BEGINSC: case ENDSC:
		/* nothing */
		break;
	   default:
		DefaultLine(cmd);
	}
}



void
Sections(enum command cmd) {

	switch (cmd) {
	   case ENDSECTHEAD:
	   case ENDSUBSECTHEAD:
		putchar('\n');
	   case BEGINDOC:
		fcharout=0;
		break;

	   case BEGINCOMMENT: fcharout=0; break;
	   case ENDCOMMENT: fcharout=1; break;
	   case COMMENTLINE: break;

	   case BEGINSUBSECTHEAD:
		printf("  ");
		/* no break */
	   case BEGINSECTHEAD:
		fcharout=1;
		break;
	   case CHARRQUOTE:
	   case CHARLQUOTE:
		xputchar('"');
		break;
	   case CHARLSQUOTE:
		xputchar('`');
		break;
	   case CHARRSQUOTE:
	   case CHARACUTE:
		xputchar('\'');
		break;
	   case BEGINTABLE: case ENDTABLE:
	   case BEGINTABLELINE: case ENDTABLELINE: case BEGINTABLEENTRY: case ENDTABLEENTRY:
	   case BEGININDENT: case ENDINDENT:
	   case FONTSIZE:
		break;
	   case CHARPERIOD:
	   case CHARTAB:
	   case CHARDASH:
	   case CHARBACKSLASH:
	   case CHARLT:
	   case CHARGT:
	   case CHARHAT:
	   case CHARVBAR:
	   case CHARAMP:
	   case CHARNBSP:
		xputchar(cmd); break;
	   case CHARDAGGER:	xputchar('+'); break;
	   case CHARBULLET:	xputchar('*'); break;
	   case CHARPLUSMINUS: xputchar('+'); xputchar('-'); break;
 	   case CHARCENT:	xputchar('c'); break;
 	   case CHARSECT:	xputchar('S'); break;
	   case CHARCOPYR:	xputchar('('); xputchar('C'); xputchar(')'); break;
 	   case CHARNOT:	xputchar('~'); break;
 	   case CHARREGTM:	xputchar('('); xputchar('R'); xputchar(')'); break;
 	   case CHARDEG:	xputchar('o'); break;
 	   case CHAR14:	xputchar('1'); xputchar('/'); xputchar('4'); break;
 	   case CHAR12:	xputchar('1'); xputchar('/'); xputchar('2'); break;
 	   case CHAR34:	xputchar('3'); xputchar('/'); xputchar('4'); break;
 	   case CHARMUL:	xputchar('X'); break;
 	   case CHARDIV:	xputchar('/'); break;
	   case ITAB:		DefaultLine(cmd); break;


	   default:
		/* nothing */
		break;
	}
}



void
Roff(enum command cmd) {
	switch (cmd) {
	   case BEGINDOC:
		I=1;
		printf(".TH %s %s \"generated by PolyglotMan\" UCB\n", manName, manSect);
		printf(".\\\"  %s,\n", PROVENANCE);
		printf(".\\\"  %s\n", HOME);
		CurLine=1;
		break;
	   case BEGINBODY:		printf(".LP\n"); break;

	   case BEGINCOMMENT: 
	   case ENDCOMMENT:
		break;
	   case COMMENTLINE: printf("'\\\" "); break;

	   case BEGINSECTHEAD:	printf(".SH "); break;
	   case BEGINSUBSECTHEAD:printf(".SS "); break;
	   case BEGINBULPAIR:	printf(".IP "); break;
	   case SHORTLINE:		printf("\n.br"); break;
	   case BEGINBOLD:		printf("\\fB"); break;	/* \n.B -- grr! */
	   case ENDCODE:
	   case ENDBOLD:		printf("\\fR"); break;	/* putchar('\n'); */
	   case BEGINITALICS:	printf("\\fI"); break;
	   case ENDITALICS:		printf("\\fR"); break;
	   case BEGINCODE:
	   case BEGINBOLDITALICS:printf("\\f4"); break;
	   case ENDBOLDITALICS:	printf("\\fR"); break;

	   case CHARLQUOTE:		printf("\\*(rq"); break;
	   case CHARRQUOTE:		printf("\\*(lq"); break;
	   case CHARNBSP:		printf("\\|"); break;
	   case CHARLSQUOTE:	putchar('`');	break;
	   case CHARRSQUOTE:	putchar('\'');	break;
	   case CHARPERIOD:		if (I==1) printf("\\&"); putchar('.'); I++; break;
	   case CHARDASH:		printf("\\-"); break;
	   case CHARTAB:
	   case CHARLT:
	   case CHARGT:
	   case CHARHAT:
	   case CHARVBAR:
	   case CHARAMP:
		putchar(cmd); break;
	   case CHARBULLET:		printf("\\(bu"); break;
	   case CHARDAGGER:		printf("\\(dg"); break;
	   case CHARPLUSMINUS:	printf("\\(+-"); break;
	   case CHANGEBAR:		putchar('|'); break;
 	   case CHARCENT:		printf("\\(ct"); break;
 	   case CHARSECT:		printf("\\(sc"); break;
 	   case CHARCOPYR:		printf("\\(co"); break;
 	   case CHARNOT:		printf("\\(no"); break;
 	   case CHARREGTM:		printf("\\(rg"); break;
 	   case CHARDEG:		printf("\\(de"); break;
 	   case CHARACUTE:		printf("\\(aa"); break;
 	   case CHAR14:		printf("\\(14"); break;
 	   case CHAR12:		printf("\\(12"); break;
 	   case CHAR34:		printf("\\(34"); break;
 	   case CHARMUL:		printf("\\(mu"); break;
 	   case CHARDIV:		printf("\\(di"); break;
	   case HR:			/*printf("\n%s\n", horizontalrule);*/ break;
	   case CHARBACKSLASH:	printf("\\\\"); break;  /* correct? */

	   case BEGINLINE:
		/*for (i=0; i<ncnt; i++) putchar('\n');*/
		break;

	   case BEGINBULLET:	putchar('"'); break;
	   case ENDBULLET:		printf("\"\n"); break;

	   case ENDLINE:
		CurLine++;
		I=1;
		/* no break */
	   case ENDSUBSECTHEAD:
	   case ENDSECTHEAD:
	   case ENDDOC:
		putchar('\n');
		break;

	   case BEGINCODEBLOCK:	printf(".nf\n");
	   case ENDCODEBLOCK:	printf(".fi\n");

	   case ENDBODY:
	   case ENDBULPAIR:
	   case BEGINBULTXT: case ENDBULTXT:
	   case BEGINSECTION: case ENDSECTION:
	   case BEGINSUBSECTION: case ENDSUBSECTION:
	   case BEGINY: case ENDY:
	   case BEGINSC: case ENDSC:
	   case BEGINTABLE: case ENDTABLE:
	   case BEGINTABLELINE: case ENDTABLELINE: case BEGINTABLEENTRY: case ENDTABLEENTRY:
	   case BEGININDENT: case ENDINDENT:
	   case FONTSIZE:
	   case BEGINHEADER: case ENDHEADER:
	   case BEGINFOOTER: case ENDFOOTER:
	   case BEGINMANREF: case ENDMANREF:
		/* nothing */
		break;
	   default:
		DefaultPara(cmd);
	}
}



/*
 * HTML
 */

void
HTML(enum command cmd) {
	static int pre=0;
	int i;
	int lasttoc;

	/* always respond to these signals */
	switch (cmd) {
	   case CHARNBSP:	printf("&nbsp;"); I++; break;
	   case CHARTAB:	printf("<tt> </tt>&nbsp;<tt> </tt>&nbsp;");		break;
	   case CHARLQUOTE:	printf("&ldquo;"); break;
	   case CHARRQUOTE:	printf("&rdquo;"); break;
	   case CHARLSQUOTE:	printf("&lsquo;"); break;
	   case CHARRSQUOTE:	printf("&rsquo;"); break;
	   case CHARPERIOD:
	   case CHARDASH:
	   case CHARBACKSLASH:
	   case CHARVBAR:	/*printf("&brvbar;"); -- broken bar no good */
	   case CHARHAT:
		 putchar(cmd);
		 break;
	   case CHARDAGGER:	printf("&dagger;"); break;
	   case CHARBULLET:	if (I>0 || !finlist) printf("&#183;"/*"&middot;"*//*&sect;--middot hardly visible*/);
		break;
	   case CHARPLUSMINUS:	printf("&plusmn;"); break;
	   case CHARGT:		printf("&gt;"); break;
	   case CHARLT:		printf("&lt;"); break;
	   case CHARAMP:	printf("&amp;"); break;
	   case CHARCENT:	printf("&cent;"); break;
 	   case CHARSECT:	printf("&sect;"); break;
 	   case CHARCOPYR:	printf("&copy;"); break;
 	   case CHARNOT:	printf("&not;"); break;
 	   case CHARREGTM:	printf("&reg;"); break;
 	   case CHARDEG:	printf("&deg;"); break;
 	   case CHARACUTE:	printf("&acute;"); break;
 	   case CHAR14:		printf("&frac14;"); break;
 	   case CHAR12:		printf("&frac12;"); break;
 	   case CHAR34:		printf("&frac34;"); break;
 	   case CHARMUL:	printf("&#215;"); break;
 	   case CHARDIV:	printf("&#247;"); break;
	   default:
		break;
	}

	/* while in pre mode... */
	if (pre) {
		switch (cmd) {
		   case ENDLINE:	I=0; CurLine++; if (!fPara && scnt) printf("<br>"); printf("\n"); break;
		   case ENDTABLE:
			if (fSource) {
			  printf("</table>\n");
			} else {
			  printf("</pre><br>\n"); pre=0; fQS=fIQS=fPara=1;
			}
			break;
		   case ENDCODEBLOCK: printf("</pre>"); pre=0; break;
		   case SHORTLINE:
		   case ENDBODY:
			printf("\n");
			break;
		   default:
			/* nothing */
			break;
		}
		return;
	}

	/* usual operation */
	switch (cmd) {
	   case BEGINDOC:
		/* escchars = ...  => HTML doesn't backslash-quote metacharacters */
		printf("<!-- %s, -->\n", PROVENANCE);
		printf("<!-- %s -->\n\n", HOME);
		printf("<html>\n<head>\n");
/*		printf("<isindex>\n");*/
		/* better title possible? */
		printf("<title>"); printf(manTitle, manName, manSect); printf("</title>\n");
		printf("</head>\n<body bgcolor='white'>\n");
		printf("<a href='#toc'>%s</a><p>\n", TABLEOFCONTENTS);
		I=0;
		break;
	   case ENDDOC:
		/* header and footer wanted? */
		printf("<p>\n");
		if (fHeadfoot) {
			printf("<hr><h2>%s</h2>\n", HEADERANDFOOTER);
			for (i=0; i<CRUFTS; i++) if (*cruft[i]) printf("%s<br>\n", cruft[i]);
		}

		if (!tocc) {
			/*printf("\n<h1>ERROR: Empty man page</h1>\n");*/
		} else {
			printf("\n<hr><p>\n");
			printf("<a name='toc'><b>%s</b></a><p>\n", TABLEOFCONTENTS);
			printf("<ul>\n");
			for (i=0, lasttoc=BEGINSECTION; i<tocc; lasttoc=toc[i].type, i++) {
				if (lasttoc!=toc[i].type) {
					if (toc[i].type==BEGINSUBSECTION) printf("<ul>\n");
					else printf("</ul>\n");
				}
				printf("<li><a name='toc%d' href='#sect%d'>%s</a></li>\n", i, i, toc[i].text);
			}
			if (lasttoc==BEGINSUBSECTION) printf("</ul>");
			printf("</ul>\n");
		}
		printf("</body>\n</html>\n");
		break;
	   case BEGINBODY:
		printf("<p>\n");
		break;
	   case ENDBODY:		break;

	   case BEGINCOMMENT: printf("\n<!--\n"); break;
	   case ENDCOMMENT: printf("\n-->\n"); break;
	   case COMMENTLINE: break;

	   case BEGINSECTHEAD:
		printf("\n<h2><a name='sect%d' href='#toc%d'>", tocc, tocc);
		break;
	   case ENDSECTHEAD:
		printf("</a></h2>\n");
		/* useful extraction from FILES, ENVIRONMENT? */
		break;
	   case BEGINSUBSECTHEAD:
		printf("\n<h3><a name='sect%d' href='#toc%d'>", tocc, tocc);
		break;
	   case ENDSUBSECTHEAD:
		printf("</a></h3>\n");
		break;
	   case BEGINSECTION:	break;
	   case ENDSECTION:
		if (sectheadid==NAME && message!=NULL) printf(message);
		break;
	   case BEGINSUBSECTION:	break;
	   case ENDSUBSECTION:	break;

	   case BEGINBULPAIR:
		if (listtype==OL) printf("\n<ol>\n");
		else if (listtype==UL) printf("\n<ul>\n");
		else printf("\n<dl>\n");
		break;
	   case ENDBULPAIR:
		if (listtype==OL) printf("\n</ol>\n");
		else if (listtype==UL) printf("\n</ul>\n");
		else printf("</dl>\n");
		break;
	   case BEGINBULLET:
		if (listtype==OL || listtype==UL) fcharout=0;
		else printf("\n<dt>");
		break;
	   case ENDBULLET:
		if (listtype==OL || listtype==UL) fcharout=1;
		else printf("</dt>");
		break;
	   case BEGINBULTXT:
		if (listtype==OL || listtype==UL) printf("<li>");
		else printf("\n<dd>");
		break;
	   case ENDBULTXT:
		if (listtype==OL || listtype==UL) printf("</li>");
		else printf("</dd>\n");
		break;

	   case BEGINLINE:
		/*		if (ncnt) printf("<p>\n"); -- if haven't already generated structural tag */
		if (ncnt) printf("\n<p>");

		/* trailing spaces already trimmed off, so look for eol now */
		if (fCodeline) {
		  printf("<code>");
		  for (i=0; i<scnt-indent; i++) printf("&nbsp;"/*&#160;*/);		/* ? */
		  tagc=0;

		  /* already have .tag=BOLDITALICS, .first=0 */
		  /* would be more elegant, but can't print initial spaces before first tag
		  tags[0].last = linelen;
		  tagc=1;
		  fIQS=0;
		  */
		}

		break;

	   case ENDLINE:
		/*if (fCodeline) { fIQS=1; fCodeline=0; }*/
		if (fCodeline) { printf("</code><br>"); fCodeline=0; }
		I=0; CurLine++; if (!fPara && scnt) printf("<br>"); printf("\n");
		break;

	   case SHORTLINE:
		if (fCodeline) { printf("</code>"); fCodeline=0; }
		if (!fIP) printf("<br>\n");
		break;


	   case BEGINTABLE:
		if (fSource) {
		  /*printf("<center><table border>\n");*/
		  printf("<table border='0'>\n");
		} else {
		  printf("<br><pre>\n"); pre=1; fQS=fIQS=fPara=0;
		}
		break;
	   case ENDTABLE:
		if (fSource) {
		  printf("</table>\n");
		} else {
		  printf("</pre><br>\n"); pre=0; fQS=fIQS=fPara=1;
		}
		break;
	   case BEGINTABLELINE:		printf("<tr>"); break;
	   case ENDTABLELINE:		printf("</tr>\n"); break;
	   case BEGINTABLEENTRY:
		printf("<td align='");
		switch (tblcellformat[0]) {
		case 'c':	printf("center"); break;
		case 'n':	/*printf("decimal"); break;  -- fall through to right for now */
		case 'r':	printf("right"); break;
		case 'l':
		default:
		  printf("left");
		}
		if (tblcellspan>1) printf(" colspan=%d", tblcellspan);
		printf("'>");
		break;
	   case ENDTABLEENTRY:
		printf("</td>");
		break;

	   /* something better with CSS */
	   case BEGININDENT:		printf("<blockquote>"); break;
	   case ENDINDENT:			printf("</blockquote>\n"); break;

	   case FONTSIZE:
		/* HTML font step sizes are bigger than troff's */
		if ((fontdelta+=intArg)!=0) printf("<font size='%c1'>", (intArg>0)?'+':'-'); else printf("</font>\n");
		break;

	   case BEGINBOLD:		printf("<b>"); break;
	   case ENDBOLD:		printf("</b>"); break;
	   case BEGINITALICS:	printf("<i>"); break;
	   case ENDITALICS:		printf("</i>"); break;
	   case BEGINBOLDITALICS:
	   case BEGINCODE:		printf("<code>"); break;
	   case ENDBOLDITALICS:
	   case ENDCODE:		printf("</code>"); break;
	   case BEGINCODEBLOCK:	printf("<pre>"); pre=1; break;	/* wrong for two-column lists in kermit.1, pine.1, perl4.1 */
	   case ENDCODEBLOCK:	printf("</pre>"); pre=0; break;
	   case BEGINCENTER:		printf("<center>"); break;
	   case ENDCENTER:		printf("</center>"); break;
	   case BEGINMANREF:
		manrefextract(hitxt);
		if (fmanRef) { printf("<a href='"); printf(href, manrefname, manrefsect); printf("'>"); }
		else printf("<i>");
		break;
	   case ENDMANREF:
		if (fmanRef) printf("</a>\n"); else printf("</i>");
		break;
	   case HR:		printf("\n<hr>\n"); break;

		/* U (was B, I), strike -- all temporary until HTML 4.0's INS and DEL widespread */
	   case BEGINDIFFA: printf("<ins><u>"); break;
	   case ENDDIFFA: printf("</u></ins>"); break;
	   case BEGINDIFFD: printf("<del><strike>"); break;
	   case ENDDIFFD: printf("</strike></del>"); break;

	   case BEGINSC: case ENDSC:
	   case BEGINY: case ENDY:
	   case BEGINHEADER: case ENDHEADER:
	   case BEGINFOOTER: case ENDFOOTER:
	   case CHANGEBAR:
		/* nothing */
		break;
	   default:
		DefaultPara(cmd);
	}
}



/*
 * DocBook XML
 * improvements by Aaron Hawley applied 2003 June 5
 *
 * N.B. The framework for XML is in place but not done.  If you
 * are familiar with the DocBook DTD, however, it shouldn't be
 * too difficult to finish it.  If you do so, please send your
 * code to me so that I may share the wealth in the next release.
 */

const char *DOCBOOKPATH = "http://www.oasis-open.org/docbook/xml/4.2/docbookx.dtd";

void
XML(enum command cmd) {
	static int pre=0;
	int i;
	int lasttoc;
	char *p;
	static int fRefEntry=0;
	static int fRefPurpose=0;
	static int fSynopsisFirst=0;
	/*static char *bads => XML doesn't backslash-quote metacharacters */

/*
*/

	/* always respond to these signals */
	switch (cmd) {
	   case CHARLQUOTE: case CHARRQUOTE: printf("&quot;"); break;
	   case CHARBULLET: printf("&bull;"); break;
	   case CHARDAGGER: printf("&dagger;"); break;
	   case CHARPLUSMINUS: printf("&plusmn;"); break;
 	   case CHARCOPYR: printf("&copy;"); break;
 	   case CHARNOT: printf("&not;"); break;
 	   case CHARMUL: printf("&times;"); break;
 	   case CHARDIV: printf("&divide;"); break;
	   case CHARAMP: printf("&amp;"); break;
	   case CHARDASH:
		if (sectheadid==NAME && !fRefPurpose) {
			printf("</refname><refpurpose>");
			fRefPurpose=1;
		} else putchar('-');
		break;
	   case CHARBACKSLASH: putchar('\\'); break;
	   case CHARGT: printf("&gt;"); break;
	   case CHARLT: printf("&lt;"); break;
	   case CHARLSQUOTE:
	   case CHARRSQUOTE:
	   case CHARPERIOD:
	   case CHARTAB:
	   case CHARHAT:
	   case CHARVBAR:
	   case CHARNBSP:
 	   case CHARCENT:
 	   case CHARSECT:
 	   case CHARREGTM:
 	   case CHARDEG:
 	   case CHARACUTE:
 	   case CHAR14:
 	   case CHAR12:
 	   case CHAR34: 
		 putchar(cmd); 
		 break;
	   default:
		break;
	}

	/* while in pre mode... */
	if (pre) {
		switch (cmd) {
		   case ENDLINE:	I=0; CurLine++; if (!fPara && scnt) putchar(' '); break;
		   case ENDTABLE:
			if (fSource) printf("</table>\n");
			else { printf("</literallayout>\n"); pre=0; fQS=fIQS=fPara=1; }
			break;
		   default:
			/* nothing */
			break;
		}
		return;
	}

	/* usual operation */
	switch (cmd) {
	   case BEGINDOC:
		printf("\n<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.2//EN\"\n");
		printf("  \"%s\">\n", DOCBOOKPATH);

		printf("<!--\n\n\tI am looking for help to finish DocBook XML.\n\n-->\n");

		printf("<!-- %s\n", PROVENANCE);
		printf("     %s -->\n\n",HOME);
		/* better title possible? */
		for (p=manName; *p; p++) *p = tolower(*p);
		printf("<refentry id='%s.%s'>\n", manName, manSect);
		printf("<refmeta>\n<refentrytitle>%s</refentrytitle>\n", manName);
		printf("<manvolnum>%s</manvolnum>\n</refmeta>\n\n", manSect);

		I=0;
		break;

	   case ENDDOC:
		/* header and footer wanted? */
		if (fHeadfoot) {
			printf("\n<refsect1>\n<title>%s</title>\n", HEADERANDFOOTER);
			for (i=0; i<CRUFTS; i++) if (*cruft[i]) printf("<para>%s</para>\n", cruft[i]);
			printf("\n</refsect1>");
		}

		/* table of contents, such as found in HTML, can be generated automatically by XML software */

		printf("</refentry>\n");
		break;
	   case BEGINBODY:
		 if (fPara) printf("\n</para>");
		 printf("<para>"); fPara = 1;
		 break;
	   case ENDBODY:
		 if (fRefPurpose) { printf("</refpurpose>"); fRefPurpose=0; }
		 else { printf("\n</para>"); fPara=0; }
		 break;

	   case BEGINCOMMENT: printf("\n<!--\n"); break;
	   case ENDCOMMENT: printf("\n-->\n"); break;
	   case COMMENTLINE: break;

	   case BEGINSECTHEAD:
	   case BEGINSUBSECTHEAD:
		if (sectheadid != NAME && sectheadid != SYNOPSIS) printf("<title>");
		break;
	   case ENDSECTHEAD:
	   case ENDSUBSECTHEAD:
		if (sectheadid == NAME) printf("--><refname>");
		else if (sectheadid == SYNOPSIS) {}
		else { printf("</title>\n<para>"); fPara=1; }
		break;

	   case BEGINSECTION:
		if (sectheadid==NAME) printf("<refnamediv><!--\n");
			/*printf("<RefEntry>");  -- do lotsa parsing here for RefName, RefPurpose*/
		else if (sectheadid==SYNOPSIS) { fSynopsisFirst = 1; printf("<refsynopsisdiv>\n<cmdsynopsis><!--\n"); }
		else printf("\n<refsect1>\n");
		break;
	   case ENDSECTION:
		if (sectheadid==NAME) {
		  if (fRefPurpose) { printf("</refpurpose>"); fRefPurpose=0; }
		  printf("\n</refnamediv>\n\n");
		} else if (sectheadid==SYNOPSIS) printf("\n</cmdsynopsis>\n</refsynopsisdiv>\n");
		else {
		  if (fPara) { printf("\n</para>"); fPara=0; }
		  printf("\n</refsect1>\n");
		}
		break;

	   case BEGINSUBSECTION:	printf("\n<refsect2>"); break;
	   case ENDSUBSECTION:		if (fPara) { printf("\n</para>"); fPara=0; }
					printf("\n</refsect2>"); break;

	   /* need to update this for enumerated and plain lists */
	   case BEGINBULPAIR:	printf("<variablelist>\n"); break;
	   case ENDBULPAIR:		printf("</variablelist>\n"); break;
	   case BEGINBULLET:	printf("<varlistentry><term>"); break;
	   case ENDBULLET:		printf("</term>\n"); break;
	   case BEGINBULTXT:	printf("<listitem>\n<para>"); break;
	   case ENDBULTXT:		printf("\n</para></listitem></varlistentry>\n"); break;

	   case BEGINLINE:
		/* remember, get BEGINBODY call at start of paragraph */
		if (fRefEntry) {
			if (fRefPurpose) {
				for (p=plain; *p!='-'; p++) {
					/* nothing?! */
				}
			}
		}

		break;

	   case ENDLINE:
		/*if (fCodeline) { fIQS=1; fCodeline=0; }*/
		if (fCodeline) { printf("</code>"); fCodeline=0; } /*  */
		I=0; CurLine++; if (!fPara && scnt) printf("<sbr/>"); else putchar(' ');
		break;

	   case SHORTLINE:
		if (fCodeline) { printf("</code>"); fCodeline=0; }
		if (!fIP && !fPara) printf("<sbr/>\n");
		break;

	   case BEGINTABLE:
		if (fSource) printf("<table>\n");
		else { printf("<literallayout>\n"); pre=1; fQS=fIQS=fPara=0; }
		break;
	   case ENDTABLE:
		if (fSource) printf("</table>\n");
		else { printf("</literallayout>\n"); pre=0; fQS=fIQS=fPara=1; }
		break;
	   case BEGINTABLELINE:		printf("<row>"); break;
	   case ENDTABLELINE:		printf("</row>\n"); break;
	   case BEGINTABLEENTRY:		printf("<entry>"); break;
	   case ENDTABLEENTRY:		printf("</entry>"); break;

	   case BEGININDENT: case ENDINDENT: break;
	   case FONTSIZE:
		break;

	   /* have to make some guess about bold and italics */
	   case BEGINBOLD:		if (sectheadid==SYNOPSIS && fSynopsisFirst) { fSynopsisFirst = 0; printf("-->"); } 
					printf("<command>"); break;
	   case ENDBOLD:		printf("</command>"); break;
	   case BEGINITALICS:	printf("<emphasis>"); break;	/* could be literal or arg */
	   case ENDITALICS:		printf("</emphasis>"); break;
	   case BEGINBOLDITALICS: case BEGINCODE:	printf("<literal>"); break;
	   case ENDBOLDITALICS: case ENDCODE:	printf("</literal>"); break;
	   case BEGINMANREF:
		manrefextract(hitxt);
		if (fmanRef) { printf("<link linkend='"); printf(href, manrefname, manrefsect); printf("'>"); }
		break;
	   case ENDMANREF:
		if (fmanRef) printf("</link>");
		break;

	   case HR:
	   case BEGINSC: case ENDSC:
	   case BEGINY: case ENDY:
	   case BEGINHEADER: case ENDHEADER:
	   case BEGINFOOTER: case ENDFOOTER:
	   case CHANGEBAR:
		/* nothing */
		break;
	   default:
		DefaultPara(cmd);
	}
}



/* generates MIME compliant to RFC 1563 */

void
MIME(enum command cmd) {
	static int pre=0;
	int i;

	/* always respond to these signals */
	switch (cmd) {
	   case CHARDASH:
	   case CHARAMP:
	   case CHARPERIOD:
	   case CHARTAB:
		putchar(cmd); break;
	   case CHARLSQUOTE:	putchar('`'); break;
	   case CHARACUTE:
	   case CHARRSQUOTE:	putchar('\''); break;
	   case CHARBULLET:		putchar('*'); break;
	   case CHARDAGGER:		putchar('|'); break;
	   case CHARPLUSMINUS:	printf("+-"); break;
	   case CHARNBSP:		putchar(' '); break;
 	   case CHARCENT:	putchar('c'); break;
 	   case CHARSECT:	putchar('S'); break;
 	   case CHARCOPYR:	printf("(C)"); break;
 	   case CHARNOT:	putchar('~'); break;
 	   case CHARREGTM:	printf("(R)"); break;
 	   case CHARDEG:	putchar('o'); break;
 	   case CHAR14:		printf("1/4"); break;
 	   case CHAR12:		printf("1/2"); break;
 	   case CHAR34:		printf("3/4"); break;
 	   case CHARMUL:	putchar('X'); break;
 	   case CHARDIV:	putchar('/'); break;
	   case CHARLQUOTE:
	   case CHARRQUOTE:
		putchar('"');
		break;
	   case CHARBACKSLASH:	/* these should be caught as escaped chars */
	   case CHARGT:
	   case CHARLT:
		assert(1);
		break;
	   default:
		break;
	}

	/* while in pre mode... */
	if (pre) {
		switch (cmd) {
		   case ENDLINE:	I=0; CurLine++; if (!fPara && scnt) printf("\n\n"); break;
		   case ENDTABLE:	printf("</fixed>\n\n"); pre=0; fQS=fIQS=fPara=1; break;
		   default:
			/* nothing */
			break;
		}
		return;
	}

	/* usual operation */
	switch (cmd) {
	   case BEGINDOC:
		printf("Content-Type: text/enriched\n");
		printf("Text-Width: 60\n");
		escchars = "<>\\";

		I=0;
		break;
	   case ENDDOC:
		/* header and footer wanted? */
		printf("\n\n");
		if (fHeadfoot) {
			printf("\n");
			MIME(BEGINSECTHEAD); printf("%s",HEADERANDFOOTER); MIME(ENDSECTHEAD);
			for (i=0; i<CRUFTS; i++) if (*cruft[i]) printf("\n%s\n", cruft[i]);
		}

/*
		printf("\n<comment>\n");
		printf("%s\n%s\n", PROVENANCE, HOME);
		printf("</comment>\n\n");
*/

/*
		printf("\n<HR><P>\n");
		printf("<A NAME=\"toc\"><B>%s</B></A><P>\n", TABLEOFCONTENTS);
		printf("<UL>\n");
		for (i=0, lasttoc=BEGINSECTION; i<tocc; lasttoc=toc[i].type, i++) {
		  if (lasttoc!=toc[i].type) {
		    if (toc[i].type==BEGINSUBSECTION) printf("<UL>\n");
		    else printf("</UL>\n");
		  }
		  printf("<LI><A NAME=\"toc%d\" HREF=\"#sect%d\">%s</A></LI>\n", i, i, toc[i].text);
		}
		if (lasttoc==BEGINSUBSECTION) printf("</UL>");
		printf("</UL>\n");
		printf("</BODY></HTML>\n");
*/
		break;
	   case BEGINBODY:
		printf("\n\n");
		break;
	   case ENDBODY:		break;

	   case BEGINCOMMENT: fcharout=0; break;
	   case ENDCOMMENT: fcharout=1; break;
	   case COMMENTLINE: break;

	   case BEGINSECTHEAD:
		printf("\n<bigger><bigger><underline>");
		/*A NAME=\"sect%d\" HREF=\"#toc%d\"><H2>", tocc, tocc);*/
		break;
	   case ENDSECTHEAD:
		printf("</underline></bigger></bigger>\n\n<indent>");
		/* useful extraction from files, environment? */
		break;
	   case BEGINSUBSECTHEAD:
		printf("<bigger>");
		/*\n<A NAME=\"sect%d\" HREF=\"#toc%d\"><H3>", tocc, tocc);*/
		break;
	   case ENDSUBSECTHEAD:
		printf("</bigger>\n\n</indent>");
		break;
	   case BEGINSECTION:
	   case BEGINSUBSECTION:
		break;
	   case ENDSECTION:
	   case ENDSUBSECTION:
		printf("</indent>\n");
		break;

	   case BEGINBULPAIR:	break;
	   case ENDBULPAIR:		break;
	   case BEGINBULLET:	printf("<bold>"); break;
	   case ENDBULLET:		printf("</bold>\t"); break;
	   case BEGINBULTXT:
	   case BEGININDENT:
		printf("<indent>");
		break;
	   case ENDBULTXT:
	   case ENDINDENT:
		printf("</indent>\n");
		break;

	   case FONTSIZE:
		if ((fontdelta+=intArg)==0) {
		  if (intArg>0) printf("</smaller>"); else printf("</bigger>");
		} else {
		  if (intArg>0) printf("<bigger>"); else printf("<smaller>");
		}
		break;

	   case BEGINLINE:		/*if (ncnt) printf("\n\n");*/ break;
	   case ENDLINE:		I=0; CurLine++; printf("\n"); break;
	   case SHORTLINE:		if (!fIP) printf("\n\n"); break;
	   case BEGINTABLE:		printf("<nl><fixed>\n"); pre=1; fQS=fIQS=fPara=0; break;
	   case ENDTABLE:		printf("</fixed><nl>\n"); pre=0; fQS=fIQS=fPara=1; break;
	   case BEGINTABLELINE: case ENDTABLELINE: case BEGINTABLEENTRY: case ENDTABLEENTRY:
		break;
		/* could use a new list type */

	   case BEGINBOLD:		printf("<bold>"); break;
	   case ENDBOLD:		printf("</bold>"); break;
	   case BEGINITALICS:	printf("<italics>"); break;
	   case ENDITALICS:		printf("</italics>"); break;
	   case BEGINCODE:
	   case BEGINBOLDITALICS:printf("<bold><italics>"); break;
	   case ENDCODE:
	   case ENDBOLDITALICS:	printf("</bold></italics>"); break;
	   case BEGINMANREF:
		printf("<x-color><param>blue</param>");
/* how to make this hypertext?
   		manrefextract(hitxt);
		if (fmanRef) { printf("<A HREF=\""); printf(href, manrefname, manrefsect); printf("\">\n"); }
		else printf("<I>");
		break;
*/
		break;
	   case ENDMANREF:
		printf("</x-color>");
		break;

	   case HR:		printf("\n\n%s\n\n", horizontalrule); break;

	   case BEGINSC: case ENDSC:
	   case BEGINY: case ENDY:
	   case BEGINHEADER: case ENDHEADER:
	   case BEGINFOOTER: case ENDFOOTER:
	   case CHANGEBAR:
		/* nothing */
		break;
	   default:
		DefaultPara(cmd);
	}
}



/*
 * LaTeX
 */

void
LaTeX(enum command cmd) {

	switch (cmd) {
	   case BEGINDOC:
		escchars = "$&%#_{}"; /* and more to come? */
		printf("%% %s,\n", PROVENANCE);
		printf("%% %s\n\n", HOME);
		/* definitions */
		printf(
		  "\\documentstyle{article}\n"
		  "\\def\\thefootnote{\\fnsymbol{footnote}}\n"
		  "\\setlength{\\parindent}{0pt}\n"
		  "\\setlength{\\parskip}{0.5\\baselineskip plus 2pt minus 1pt}\n"
		  "\\begin{document}\n"
			  );
		I=0;
		break;
	   case ENDDOC:
		/* header and footer wanted? */
		printf("\n\\end{document}\n");

		break;
	   case BEGINBODY:
		printf("\n\n");
		break;
	   case ENDBODY:		break;

	   case BEGINCOMMENT:
	   case ENDCOMMENT:
		break;
	   case COMMENTLINE: printf("%% "); break;


	   case BEGINSECTION:	break;
	   case ENDSECTION:		break;
	   case BEGINSECTHEAD:	printf("\n\\section{"); tagc=0; break;
	   case ENDSECTHEAD:
		printf("}");
/*
		if (CurLine==1) printf("\\footnote{"
		  "\\it conversion to \\LaTeX\ format by PolyglotMan "
		  "available via anonymous ftp from {\\tt ftp.berkeley.edu:/ucb/people/phelps/tcltk}}"
			  );
*/
		/* useful extraction from files, environment? */
		printf("\n");
		break;
	   case BEGINSUBSECTHEAD:printf("\n\\subsection{"); break;
	   case ENDSUBSECTHEAD:
		printf("}");
		break;
	   case BEGINSUBSECTION:	break;
	   case ENDSUBSECTION:	break;
	   case BEGINBULPAIR:	printf("\\begin{itemize}\n"); break;
	   case ENDBULPAIR:		printf("\\end{itemize}\n"); break;
	   case BEGINBULLET:	printf("\\item ["); break;
	   case ENDBULLET:		printf("] "); break;
	   case BEGINLINE:		/*if (ncnt) printf("\n\n");*/ break;
	   case ENDLINE:		I=0; putchar('\n'); CurLine++; break;
	   case BEGINTABLE:		printf("\\begin{verbatim}\n"); break;
	   case ENDTABLE:		printf("\\end{verbatim}\n"); break;
	   case BEGINTABLELINE: case ENDTABLELINE: case BEGINTABLEENTRY: case ENDTABLEENTRY:
		break;
	   case BEGININDENT: case ENDINDENT:
	   case FONTSIZE:
		break;
	   case SHORTLINE:		if (!fIP) printf("\n\n"); break;
	   case BEGINBULTXT:	break;
	   case ENDBULTXT:		putchar('\n'); break;

	   case CHARLQUOTE:		printf("``"); break;
	   case CHARRQUOTE:		printf("''"); break;
	   case CHARLSQUOTE:
	   case CHARRSQUOTE:
	   case CHARPERIOD:
	   case CHARTAB:
	   case CHARDASH:
	   case CHARNBSP:
		putchar(cmd); break;
	   case CHARBACKSLASH:	printf("$\\backslash$"); break;
	   case CHARGT:		printf("$>$"); break;
	   case CHARLT:		printf("$<$"); break;
	   case CHARHAT:		printf("$\\char94{}$"); break;
	   case CHARVBAR:		printf("$|$"); break;
	   case CHARAMP:		printf("\\&"); break;
	   case CHARBULLET:		printf("$\\bullet$ "); break;
	   case CHARDAGGER:		printf("\\dag "); break;
	   case CHARPLUSMINUS:	printf("\\pm "); break;
 	   case CHARCENT:		printf("\\hbox{\\rm\\rlap/c}"); break;
 	   case CHARSECT:		printf("\\S "); break;
 	   case CHARCOPYR:		printf("\\copyright "); break;
 	   case CHARNOT:		printf("$\\neg$"); break;
 	   case CHARREGTM:		printf("(R)"); break;
 	   case CHARDEG:		printf("$^\\circ$"); break;
 	   case CHARACUTE:		putchar('\''); break;
 	   case CHAR14:		printf("$\\frac{1}{4}$"); break;
 	   case CHAR12:		printf("$\\frac{1}{2}$"); break;
 	   case CHAR34:		printf("$\\frac{3}{4}$"); break;
 	   case CHARMUL:		printf("\\times "); break;
 	   case CHARDIV:		printf("\\div "); break;

	   case BEGINCODE:
	   case BEGINBOLD:		printf("{\\bf "); break;
	   case BEGINSC:		printf("{\\sc "); break;
	   case BEGINITALICS:	printf("{\\it "); break;
	   case BEGINBOLDITALICS:printf("{\\bf\\it "); break;
	   case BEGINMANREF:	printf("{\\sf "); break;
	   case ENDCODE:
	   case ENDBOLD:
	   case ENDSC:
	   case ENDITALICS:
	   case ENDBOLDITALICS:
	   case ENDMANREF:
		putchar('}');
		break;
	   case HR:		/*printf("\n%s\n", horizontalrule);*/ break;

	   case BEGINY: case ENDY:
	   case BEGINHEADER: case ENDHEADER:
	   case BEGINFOOTER: case ENDFOOTER:
	   case CHANGEBAR:
		/* nothing */
		break;
	   default:
		DefaultPara(cmd);
	}
}


void
LaTeX2e(enum command cmd) {

	switch (cmd) {
		/* replace selected commands ... */
	   case BEGINDOC:
		escchars = "$&%#_{}";
		printf("%% %s,\n", PROVENANCE);
		printf("%% %s\n\n", HOME);
		/* definitions */
		printf(
		  "\\documentclass{article}\n"
		  "\\def\\thefootnote{\\fnsymbol{footnote}}\n"
		  "\\setlength{\\parindent}{0pt}\n"
		  "\\setlength{\\parskip}{0.5\\baselineskip plus 2pt minus 1pt}\n"
		  "\\begin{document}\n"
			  );
		I=0;
		break;
	   case BEGINCODE:
	   case BEGINBOLD:		printf("\\textbf{"); break;
	   case BEGINSC:		printf("\\textsc{"); break;
	   case BEGINITALICS:	printf("\\textit{"); break;
	   case BEGINBOLDITALICS:printf("\\textbf{\\textit{"); break;
	   case BEGINMANREF:	printf("\\textsf{"); break;
	   case ENDBOLDITALICS:	printf("}}"); break;

		/* ... rest same as old LaTeX */
	   default:
		LaTeX(cmd);
	}
}



/*
 * Rich Text Format (RTF)
 */

/* RTF could use more work */

void
RTF(enum command cmd) {

	switch (cmd) {
	   case BEGINDOC:
		escchars = "{}";
		/* definitions */
		printf(
		  /* fonts */
		  "{\\rtf1\\deff2 {\\fonttbl"
		  "{\\f20\\froman Times;}{\\f150\\fnil I Times Italic;}"
		  "{\\f151\\fnil B Times Bold;}{\\f152\\fnil BI Times BoldItalic;}"
		  "{\\f22\\fmodern Courier;}{\\f23\\ftech Symbol;}"
		  "{\\f135\\fnil I Courier Oblique;}{\\f136\\fnil B Courier Bold;}{\\f137\\fnil BI Courier BoldOblique;}"
		  "{\\f138\\fnil I Helvetica Oblique;}{\\f139\\fnil B Helvetica Bold;}}"
		  "\n"

		  /* style sheets */
		  "{\\stylesheet{\\li720\\sa120 \\f20 \\sbasedon222\\snext0 Normal;}"
		  "{\\s2\\sb200\\sa120 \\b\\f3\\fs20 \\sbasedon0\\snext2 section head;}"
		  "{\\s3\\li180\\sa120 \\b\\f20 \\sbasedon0\\snext3 subsection head;}"
		  "{\\s4\\fi-1440\\li2160\\sa240\\tx2160 \\f20 \\sbasedon0\\snext4 detailed list;}}"
		  "\n"

/* more header to come--do undefined values default to nice values? */
			   );
		I=0;
		break;
	   case ENDDOC:
		/* header and footer wanted? */
		printf("\\par{\\f150 %s,\n%s}", PROVENANCE, HOME);
		printf("}\n");
		break;
	   case BEGINBODY:
		printf("\n\n");
		break;
	   case ENDBODY:
		CurLine++;
		printf("\\par\n");
		tagc=0;
		break;

	   case BEGINCOMMENT: fcharout=0; break;
	   case ENDCOMMENT: fcharout=1; break;
	   case COMMENTLINE: break;

	   case BEGINSECTION:	break;
	   case ENDSECTION:		printf("\n\\par\n"); break;
	   case BEGINSECTHEAD:	printf("{\\s2 "); tagc=0; break;
	   case ENDSECTHEAD:
		printf("}\\par");
		/* useful extraction from files, environment? */
		printf("\n");
		break;
	   case BEGINSUBSECTHEAD:printf("{\\s3 "); break;
	   case ENDSUBSECTHEAD:
		printf("}\\par\n");
		break;
	   case BEGINSUBSECTION:	break;
	   case ENDSUBSECTION:	break;
	   case BEGINLINE:		/*if (ncnt) printf("\n\n");*/ break;
	   case ENDLINE:		I=0; putchar(' '); /*putchar('\n'); CurLine++;*/ break;
	   case SHORTLINE:		if (!fIP) printf("\\line\n"); break;
	   case BEGINBULPAIR:	printf("{\\s4 "); break;
	   case ENDBULPAIR:		printf("}\\par\n"); break;
	   case BEGINBULLET:	break;
	   case ENDBULLET:		printf("\\tab "); fcharout=0; break;
	   case BEGINBULTXT:	fcharout=1; break;
	   case ENDBULTXT:		break;

	   case CHARLQUOTE:		printf("``"); break;
	   case CHARRQUOTE:		printf("''"); break;
	   case CHARLSQUOTE:
	   case CHARRSQUOTE:
	   case CHARPERIOD:
	   case CHARTAB:
	   case CHARDASH:
	   case CHARBACKSLASH:
	   case CHARGT:
	   case CHARLT:
	   case CHARHAT:
	   case CHARVBAR:
	   case CHARAMP:
	   case CHARNBSP:
 	   case CHARCENT:
 	   case CHARSECT:
 	   case CHARCOPYR:
 	   case CHARNOT:
 	   case CHARREGTM:
 	   case CHARDEG:
 	   case CHARACUTE:
 	   case CHAR14:
 	   case CHAR12:
 	   case CHAR34:
 	   case CHARMUL:
 	   case CHARDIV:
		putchar(cmd); break;
	   case CHARBULLET:		printf("\\bullet "); break;
	   case CHARDAGGER:		printf("\\dag "); break;
	   case CHARPLUSMINUS:	printf("\\pm "); break;

	   case BEGINCODE:
	   case BEGINBOLD:		printf("{\\b "); break;
	   case BEGINSC:		printf("{\\fs20 "); break;
	   case BEGINITALICS:	printf("{\\i "); break;
	   case BEGINBOLDITALICS:printf("{\\b \\i "); break;
	   case BEGINMANREF:	printf("{\\f22 "); break;
	   case ENDBOLD:
	   case ENDCODE:
	   case ENDSC:
	   case ENDITALICS:
	   case ENDBOLDITALICS:
	   case ENDMANREF:
		putchar('}');
		break;
	   case HR:		printf("\n%s\n", horizontalrule); break;

	   case BEGINY: case ENDY:
	   case BEGINHEADER: case ENDHEADER:
	   case BEGINFOOTER: case ENDFOOTER:
	   case BEGINTABLE: case ENDTABLE:
	   case BEGINTABLELINE: case ENDTABLELINE: case BEGINTABLEENTRY: case ENDTABLEENTRY:
	   case BEGININDENT: case ENDINDENT:
	   case FONTSIZE:
	   case CHANGEBAR:
		/* nothing */
		break;
	   default:
		DefaultPara(cmd);
	}
}



/*
 * pointers to existing tools
 */

void
PostScript(enum command cmd) {
	fprintf(stderr, "Use groff or psroff to generate PostScript.\n");
	exit(1);
}


void
FrameMaker(enum command cmd) {
	fprintf(stderr, "FrameMaker comes with filters that convert from roff to MIF.\n");
	exit(1);
}




/*
 * Utilities common to both parses
 */


/*
 level 0: DOC - need match
 level 1: SECTION - need match
 level 2: SUBSECTION | BODY | BULLETPAIR
 level 3: BODY (within SUB) | BULLETPAIR (within SUB) | BULTXT (within BULLETPAIR)
 level 4: BULTXT (within BULLETPAIR within SUBSECTION)

 never see: SECTHEAD, SUBSECTHEAD, BULLET
*/

int Psect=0, Psub=0, Pbp=0, Pbt=0, Pb=0, Pbul=0;

void
pop(enum command cmd) {
	assert(cmd==ENDINDENT || cmd==BEGINBULLET || cmd==BEGINBULTXT || cmd==BEGINBULPAIR || cmd==BEGINBODY || cmd==BEGINSECTION || cmd==BEGINSUBSECTION || cmd==ENDDOC);
/*
	int i;
	int p;
	int match;

	p=cmdp-1;
	for (i=cmdp-1;i>=0; i--)
		if (cmd==cmdstack[i]) { match=i; break; }
*/

	/* if match, pop off all up to and including match */
	/* otherwise, pop off one level*/

	if (Pbul) {
	  (*fn)(ENDBULLET); Pbul=0;
	  if (cmd==BEGINBULLET) return;
	} /* else close off ENDBULTXT */

	if (Pbt) { (*fn)(ENDBULTXT); Pbt=0; }
	if (cmd==BEGINBULTXT || cmd==BEGINBULLET) return;

	if (Pb && cmd==BEGINBULPAIR) { (*fn)(ENDBODY); Pb=0; }	/* special */
	if (Pbp) { (*fn)(ENDBULPAIR); Pbp=0; }
	if (cmd==BEGINBULPAIR || cmd==ENDINDENT) return;

	if (Pb) { (*fn)(ENDBODY); Pb=0; }
	if (cmd==BEGINBODY) return;

	if (Psub) { (*fn)(ENDSUBSECTION); Psub=0; }
	if (cmd==BEGINSUBSECTION) return;

	if (Psect) { (*fn)(ENDSECTION); Psect=0; }
	if (cmd==BEGINSECTION) return;
}


void
poppush(enum command cmd) {
	assert(cmd==ENDINDENT || cmd==BEGINBULLET || cmd==BEGINBULTXT || cmd==BEGINBULPAIR || cmd==BEGINBODY || cmd==BEGINSECTION || cmd==BEGINSUBSECTION);

	pop(cmd);

	switch (cmd) {
	   case BEGINBULLET: Pbul=1; break;
	   case BEGINBULTXT: Pbt=1; break;
	   case BEGINBULPAIR: Pbp=1; break;
	   case BEGINBODY: Pb=1; break;
	   case BEGINSECTION: Psect=1; break;
	   case BEGINSUBSECTION: Psub=1; break;
	   default:
		if (!fQuiet) fprintf(stderr, "poppush: unrecognized code %d\n", cmd);
	}

	(*fn)(cmd);
	prevcmd = cmd;
}



/*
 * PREFORMATTED PAGES PARSING
 */

/* wrapper for getchar() that expands tabs, and sends maximum of n=40 consecutive spaces */

int
getchartab(void) {
	static int tabexp = 0;
	static int charinline = 0;
	static int cspccnt = 0;
	char c;

	c = lookahead;
	if (tabexp) tabexp--;
	else if (c=='\n') {
		charinline=0;
		cspccnt=0;
	} else if (c=='\t') {
		tabexp = TabStops-(charinline%TabStops); if (tabexp==TabStops) tabexp=0;
		lookahead = c = ' ';
	} else if (cspccnt>=40) {
		if (*in==' ') {
			while (*in==' '||*in=='\t') in++;
			in--;
		}
		cspccnt=0;
	}

	if (!tabexp && lookahead) lookahead = *in++;
	if (c=='\b') charinline--; else charinline++;
	if (c==' ') cspccnt++;
	return c;
}


/* replace gets.  handles hyphenation too */
char *
la_gets(char *buf) {
	static char la_buf[MAXBUF];	/* can lookahead a full line, but nobody does now */
	static int fla=0, hy=0;
	char *ret,*p;
	int c,i;

	assert(buf!=NULL);

	if (fla) {
		/* could avoid copying if callers used return value */
		strcpy(buf,la_buf); fla=0;
		ret=buf;	/* correct? */
	} else {
		/*ret=gets(buf); -- gets is deprecated (since it can read too much?) */
		/* could do this...
		ret=fgets(buf, MAXBUF, stdin);
		buf[strlen(buf)-1]='\0';
		... but don't want to have to rescan line with strlen, so... */

		i=0; p=buf;

		/* recover spaces if re-linebreaking */
		for (; hy; hy--) { *p++=' '; i++; }

		while (lookahead && (c=getchartab())!='\n' && i<MAXBUF) { *p++=c; i++; }
		assert(i<MAXBUF);

		/*lookahead=ungetc(getchar(), stdin);	/* only looking ahead one character for now */

		/* very special case: if in SEE ALSO section, re-linebreak so references aren't linebroken
		   (also do this if fNOHY flag is set) -- doesn't affect lookahead */
		/* 0xad is an en dash on Linux? */
		if ((fPara || sectheadid==SEEALSO || fNOHY) && p>buf && p[-1]=='-' && isspace(lookahead)) {
			p--;	/* zap hyphen */
			/* zap boldfaced hyphens, gr! */
			while (p[-1]=='\b' && p[-2]=='-') p-=2;

			/* start getting next line, spaces first ... */
			while (lookahead && isspace(lookahead) && lookahead!='\n') { getchartab(); hy++; }

			/* ... append next nonspace string to previous ... */
			while (lookahead && !isspace(lookahead) && i++<MAXBUF) *p++=getchartab();

			/* gobble following spaces (until, perhaps including, end of line) */
			while (lookahead && isspace(lookahead) && lookahead!='\n') getchartab();
			if (lookahead=='\n') { getchartab(); hy=0; }
		}

		*p='\0';
		ret=(lookahead)?buf:NULL;
	}

	AbsLine++;
	return ret;	/* change this to line length? (-1==old NULL) */
}


/*** Kong ***/

char phrase[MAXBUF];	/* first "phrase" (space of >=3 spaces) */
int phraselen;

void
filterline(char *buf, char *plain) {
	char *p,*q,*r;
	char *ph;
	int iq;
	int i,j;
	int hl=-1, hl2=-1;
	int iscnt=0;	/* interword space count */
	int tagci;
	int I0;
	int etype;
	int efirst;
	enum tagtype tag = NOTAG;
	int esccode;

	assert(buf!=NULL && plain!=NULL);

	etype=NOTAG;
	efirst=-1;
	tagci=tagc;
	ph=phrase; phraselen=0;
	scnt=scnt2=0;
	s_sum=s_cnt=0;
	bs_sum=bs_cnt=0;
	ccnt=0;
	spcsqz=0;

	/* strip only certain \x1b's and only at very beginning of line */
	for (p=buf; *p=='\x1b' && (p[1]=='8'||p[1]=='9'); p+=2)
		/* nop */;

	strcpy(plain,p);
	q=&plain[strlen(p)];

	/*** spaces and change bars ***/
	for (scnt=0,p=plain; *p==' '; p++) scnt++;	/* initial space count */
	if (scnt>200) scnt=130-(q-p);

	assert(*q=='\0');
	q--;
	if (fChangeleft)
		for (; q-40>plain && *q=='|'; q--)	{	/* change bars */
			if (fChangeleft!=-1) ccnt++;
			while (q-2>=plain && q[-1]=='\b' && q[-2]=='|') q-=2;	/* boldface changebars! */
		}

	/*if (q!=&plain[scnt-1])*/			/* zap trailing spaces */
		for (; *q==' ' && q>plain; q--) /* nop */;

	/* second changebar way out east! HACK HACK HACK */
	if (q-plain>100 && *q=='|') {
	  while (*q=='|' && q>plain) { q--; if (fChangeleft!=-1) ccnt++; }
	  while ((*q==' ' || *q=='_' || *q=='-') && q>plain) q--;
	}

	for (r=q; (*r&0xff)==CHARDAGGER; r--) *r='-';	/* convert daggers at end of line to hyphens */

	if (q-plain < scnt) scnt = q-plain+1;
	q[1]='\0';

	/* set I for tags below */
	if (indent>=0 && scnt>=indent) scnt-=indent;
	if (!fPara && !fIQS) {
		if (fChangeleft) I+=(scnt>ccnt)?scnt:ccnt;
		else I+=scnt;
	}
	I0=I;

	/*** tags and filler spaces ***/

	iq=0; falluc=1;
	for (q=plain; *p; p++) {

		iscnt=0;
		if (*p==' ') {
			for (r=p; *r==' '; r++) { iscnt++; spcsqz++; }
			s_sum+=iscnt; s_cnt++;
			if (iscnt>1 && !scnt2 && *p==' ') scnt2=iscnt;
			if (iscnt>2) { bs_cnt++; bs_sum+=iscnt; }	/* keep track of large gaps */
			iscnt--;		/* leave last space for tail portion of loop */

			/* write out spaces */
			if (fQS && iscnt<3) { p=r-1;	iscnt=0; } /* reduce strings of <3 spaces to 1 */
			/* else if (fQS && iscnt>=3) { replace with tab? } */
			else {
				for (i=0; i<iscnt; i++) { p++; *q++=' '; }
			}
		} /* need to go through if chain for closing off annotations */

		/** backspace-related filtering **/

		/* else */ if (*p=='\b' && p[1]=='_' && q>plain && q[-1]=='+') {
			/* bold plus/minus(!) */
			q[-1]=c_plusminus;
			while (*p=='\b' && p[1]=='_') p+=2;
			continue;
		} else if ((*p=='_' && p[1]=='\b' && p[2]!='_' && p[3]!='\b')
			|| (*p=='\b' && p[1]=='_')) {
			/* italics */
			if (tag!=ITALICS && hl>=0) { tagadd(tag, hl, I+iq); hl=-1; }
			if (hl==-1) hl=I+iq;
			tag=ITALICS;
			p+=2;
		} else if (*p=='_' && p[2]==p[4] && p[1]=='\b' && p[3]=='\b' && p[2]!='_') {
			/* bold italics (for Solaris) */
			for (p+=2; *p==p[2] && p[1]=='\b';) p+=2;
			if (tag!=BOLDITALICS && hl>=0) { tagadd(tag, hl, I+iq); hl=-1; }
			if (hl==-1) hl=I+iq;
			tag=BOLDITALICS;
		} else if (*p==p[2] && p[1]=='\b') {
			/* boldface */
			while (*p==p[2] && p[1]=='\b') p+=2;
			if (tag!=BOLD && hl>=0) { tagadd(tag, hl, I+iq); hl=-1; }
			if (hl==-1) hl=I+iq;
			tag=BOLD;
		} else if (p[1]=='\b' &&
				 ((*p=='o' && p[2]=='+') ||
				  (*p=='+' && p[2]=='o')) ) {
			/* bullets */
			p+=2;
			while (p[1]=='\b' && (*p=='o' || p[2]=='+') ) p+=2;		/* bold bullets(!) */
			*q++=c_bullet; iq++;
			continue;
		} else if (*p=='\b' && p>plain && p[-1]=='o' && p[1]=='+') {
			/* OSF bullets */
			while (*p=='\b' && p[1]=='+') p+=2;	/* bold bullets(!) */
			q[-1]=c_bullet; p--;
			continue;
		} else if (p[1]=='\b' && *p=='+' && p[2]=='_') {
			/* plus/minus */
			p+=2;
			*q++=c_plusminus; iq++;
			continue;
		} else if (p[1]=='\b' && *p=='|' && p[2]=='-') {
			/* dagger */
			*q++=c_dagger; iq++;
			p+=2; continue;
		} else if (*p=='\b') {
			/* supress unattended backspaces */
			continue;
		} else if (*p=='\x1b') {
			p++;
			if (*p=='[' && isdigit(p[1])) {	/* 0/1/22/24/.../8/9/... */
				esccode=0; for (p++; isdigit(*p); p++) esccode = esccode * 10 + *p - '0';

				if (efirst>=0 /*&& (esccode==0 || esccode==1 || esccode==4 || esccode==22 || esccode==24) /*&& hl>=0 && hl2==-1 && tags[MAXTAGS].first<I+iq*/) {
					/* doesn't catch tag if spans line -- just make tag and hl static? */
					/*tagadd(tags[MAXTAGS].type, tags[MAXTAGS].first, I+iq);*/
					if (hl==-1 && hl2==-1 && efirst!=-1/*<I+iq*/)
						tagadd(etype, efirst, I+iq);
					efirst=-1;
				}

				if (esccode==1 /*&& hl==-1*/) {
					/* stash attributes in "invalid" array element */
					efirst=I+iq; etype=BOLD;
					/*hl=I+iq; tag=BOLD; -- faces immediate end of range */
				} else if (esccode==4 /*&& hl==-1*/) {
					efirst=I+iq; etype=ITALICS;

				} /* else skip unrecognized escape codes like 8/9 */
			}

			/*assert(*p=='m'); OR if (*p == 'm') ? */
			/*p++;	/* ending 'm' -- inc done in overarching for */
			continue;

		} else if ((isupper(*p) /*|| *p=='_' || *p=='&'*/) &&
				 (hl>=0 || isupper(p[1]) || (p[1]=='_' && p[2]!='\b') || p[1]=='&')) {
			if (hl==-1 && efirst==-1) { hl=I+iq; tag=SMALLCAPS; }
		} else {
			/* end of tag, one way or another */
			/* collect tags in this pass, interspersed later if need be */
			/* can't handle overlapping tags */
			if (hl>=0) {
				if (hl2==-1) tagadd(tag, hl, I+iq);
				hl=-1;
			}
		}

		/** non-backspace related filtering **/
		/* case statement here in place of if chain? */
/* Tk 3.x's text widget tabs too crazy
		if (*p==' ' && strncmp("     ",p,5)==0) {
			xputchar('\t'); i+=5-1; ci++; continue;
		} else
*/
/* copyright symbol: too much work for so little
		if (p[i]=='o' && (strncmp("opyright (C) 19",&p[i],15)==0
				    || strncmp("opyright (c) 19",&p[i],15)==0)) {
			printf("opyright \xd3 19");
			tagadd(SYMBOL, ci+9, ci+10);
			i+=15-1; ci+=13; continue;
		} else
*/
		if (*p=='(' && q>plain && (isalnum(q[-1])||strchr(manvalid/*"._-+"*/,q[-1])!=NULL)
		    && strcoloncmp(&p[1],')',vollist)
		    /* && p[1]!='s' && p[-1]!='`' && p[-1]!='\'' && p[-1]!='"'*/ ) {
			hl2=I+iq;
			for (r=q-1; r>=plain && (isalnum(*r)||strchr(manvalid/*"._-+:"*/,*r)!=NULL); r--)
				hl2--;
			/* else ref to a function? */
			/* maybe save position of opening paren so don't highlight it later */
		} else if (*p==')' && hl2!=-1) {
			/* don't overlap tags on man page references */
			while (tagc>0 && tags[tagc-1].last>hl2) tagc--;
			tagadd(MANREF, hl2, I+iq+1);
			hl2=hl=-1;
		} else if (hl2!=-1) {
			/* section names are alphanumic or '+' for C++ */
			if (!isalnum(*p) && *p!='+') hl2=-1;
		}


		/*assert(*p!='\0');*/
		if (!*p) break;	/* not just safety check -- check out sgmls.1 */

		*q++=*p;
/*		falluc = falluc && (isupper(*p) || isspace(*p) || isdigit(*p) || strchr("-+&_'/()?!.,;",*p)!=NULL);*/
		falluc = falluc && !islower(*p);
		if (!scnt2) { *ph++=*p; phraselen++; }
		iq+=iscnt+1;
	}
	if (hl>=0) tagadd(tag, hl, I+iq);
	else if (efirst>=0) tagadd(etype, efirst, I+iq);
	*q=*ph='\0';
	linelen=iq+ccnt;


	/* special case for Solaris:
	   if line has ONLY <CODE> tags AND they SPAN line, convert to one tag */
	fCodeline=0;
	if (tagc && tags[0].first==0 && tags[tagc-1].last==linelen) {
		fCodeline=1;
		j=0;
		/* invariant: at start of a tag */
		for (i=0; fCodeline && i<tagc; i++) {
			if (tags[i].type!=BOLDITALICS /*&& tags[i].type!=BOLD*/) fCodeline=0;
			else if ((j=tags[i].last)<linelen) {
			  for (; j < tags[i+1].first ; j++)
			  	if (!isspace(plain[j])) { fCodeline=0; break; }
			}
		}
	}


	/* verify tag lists -- in production, compiler should kill with dead code elimination */
	for (i=tagci; i<tagc; i++) {
		/* verify valid ranges */
		assert(tags[i].type>NOTAG && tags[i].type<=MANREF);
		assert(tags[i].first>=I0 && tags[i].last<=linelen+I0);
		assert(tags[i].first<=tags[i].last);

		/* verify for no overlap with other tags */
		for (j=i+1; j<tagc; j++) {
			assert(tags[i].last<=tags[j].first /*|| tags[i].first>=tags[j].last*/);
		}
	}
}


/*
  buf[] == input text (read only)
  plain[] == output (initial, trailing spaces stripped; tabs=>spaces;
     underlines, overstrikes => tag array; spaces squeezed, if requested)
  ccnt = count of changebars
  scnt = count of initial spaces
  linelen = length result in plain[]
*/

int fHead=0;
int fFoot=0;

void
preformatted_filter(void) {
	const int MINRM=50;		/* minimum column for right margin */
	const int MINMID=20;
	const int HEADFOOTSKIP=20;
	const int HEADFOOTMAX=25;
	int curtag;
	char *p,*r;
	char head[MAXBUF]="";		/* first "word" */
	char foot[MAXBUF]="";
	int header_m=0, footer_m=0;
	int headlen=0, footlen=0;
/*	int line=1-1; */
	int i,j,k,l,off;
	int sect=0,subsect=0,bulpair=0,osubsect=0;
	int title=1;
	int oscnt=-1;
	int empty=0,oempty;
	int fcont=0;
	int Pnew=0,I0;
	float s_avg=0.0;
	int spaceout;
	int skiplines=0;
	int c;

	/* try to keep tabeginend[][] in parallel with enum tagtype */
	assert(tagbeginend[ITALICS][0]==BEGINITALICS);
	assert(tagbeginend[MANREF][1]==ENDMANREF);
	in++;	/* lookahead = current character, in points to following */

	/*	for (i=0; i<MAXBUF; i++) tabgram[i]=0;*/

	/*if (fMan) */indent=-1;
	I=1;
	CurLine=1;
	(*fn)(BEGINDOC); I0=I;

	/* run through each line */
	while (la_gets(buf)!=NULL) {
		if (title) I=I0;
		/* strip out Ousterhout box: it's confusing the section line counts in TkMan outlining */
		if (fNORM && *buf=='_'
		    && strncmp(buf,"_________________________________________________________________",65)==0) {
			fTclTk = 1;
			if (fChangeleft==0) fChangeleft=1;
			skiplines = 2;
		}
		if (skiplines) { skiplines--; AbsLine++; continue; }
		filterline(buf,plain);	/* ALL LINES ARE FILTERED */

#if 0
		/* dealing with tables in formatted pages is hopeless */
		finTable = fTable &&
			((!ncnt && fotable) ||
			(ncnt && bs_cnt>=2 && bs_cnt<=5 && ((float) bs_sum / (float) bs_cnt)>3.0));
		if (finTable) {
			if (!fotable) (*fn)(BEGINTABLE);
		} else if (fotable) {
			(*fn)(ENDTABLE);
			I=I0; tagc=0; filterline(buf,plain);	/* rescan first line out of table */
		}
#endif

		s_avg=(float) s_sum;
		if (s_cnt>=2) {
			/* don't count large second space gap */
			if (scnt2) s_avg= (float) (s_sum - scnt2) / (float) (s_cnt-1);
			else s_avg= (float) (s_sum) / (float) (s_cnt);
		}

		p=plain;	/* points to current character in plain */

		/*** determine header and global indentation ***/
		if (/*fMan && (*/!fHead || indent==-1/*)*/) {
			if (!linelen) continue;
			if (!*header) {
				/* check for missing first header--but this doesn't catch subsequent pages */
				if (stricmp(p,"NAME")==0 || stricmp(p,"NOMBRE")==0) {	/* works because line already filtered */
					indent=scnt; /*filterline(buf,plain);*/ scnt=0; I=I0; fHead=1;
				} else {
					fHead=1;
					(*fn)(BEGINHEADER);
					/* grab header and its first word */
					strcpy(header,p);
					if ((header_m=HEADFOOTSKIP)>linelen) header_m=0;
					strcpy(head,phrase); headlen=phraselen;
					la_gets(buf); filterline(buf,plain);
					if (linelen) {
						strcpy(header2,plain);
						if (strincmp(plain,"Digital",7)==0 || strincmp(plain,"OSF",3)==0) {
							fFoot=1;
							fSubsections=0;
						}
					}
					(*fn)(ENDHEADER); tagc=0;
					continue;
				}
			} else {
				/* some idiot pages have a *third* header line, possibly after a null line */
				if (*header && scnt>MINMID) { strcpy(header3,p); ncnt=0; continue; }
				/* indent of first line ("NAME") after header sets global indent */
				/* check '<' for Plan 9(?) */
				if (*p!='<') {
					indent=scnt; I=I0; scnt=0;
				} else continue;
			}
/*			if (indent==-1) continue;*/
		}
		if (!lindent && scnt) lindent=scnt;
/*printf("lindent = %d, scnt=%d\n", lindent,scnt);*/


		/**** for each ordinary line... *****/

		/*** skip over global indentation */
		oempty=empty; empty=(linelen==0);
		if (empty) {ncnt++; continue;}

		/*** strip out per-page titles ***/

		if (/*fMan && (*/scnt==0 || scnt>MINMID/*)*/) {
/*printf("***ncnt = %d, fFoot = %d, line = %d***", ncnt,fFoot,AbsLine);*/
			if (!fFoot && !isspace(*p) && (scnt>5 || (*p!='-' && *p!='_')) &&
			    /* don't add ncnt -- AbsLine gets absolute line number */
			    (((ncnt>=2 && AbsLine/*+ncnt*/>=61/*was 58*/ && AbsLine/*+ncnt*/<70)
				 || (ncnt>=4 && AbsLine/*+ncnt*/>=59 && AbsLine/*+ncnt*/<74)
				 || (ncnt && AbsLine/*+ncnt*/>=61 && AbsLine/*+ncnt*/<=66))
				 && (/*lookahead!=' ' ||*/ (s_cnt>=1 && s_avg>1.1) || !falluc) )
			    ) {
				(*fn)(BEGINFOOTER);
				/* grab footer and its first word */
				strcpy(footer,p);
/*				if ((footer_m=linelen-HEADFOOTSKIP)<0) footer_m=0;*/
				if ((footer_m=HEADFOOTSKIP)>linelen) footer_m=0;
				/*grabphrase(p);*/ strcpy(foot,phrase); footlen=phraselen;
				/* permit variations at end, as for SGI "Page N", but keep minimum length */
				if (footlen>3) footlen--;
				la_gets(buf); filterline(buf,plain); if (linelen) strcpy(footer2,plain);
				title=1;
				(*fn)(ENDFOOTER); tagc=0;

				/* if no header on first page, try again after first footer */
				if (!fFoot && *header=='\0') fHead=0;	/* this is dangerous */
				fFoot=1;
				continue;
			} else
				/* a lot of work, but only for a few lines (about 4%) */
				if (fFoot && (scnt==0 || scnt+indent>MINMID) &&
					 (   (headlen && strncmp(head,p,headlen)==0)
					  || strcmp(header2,p)==0 || strcmp(header3,p)==0
					  || (footlen && strncmp(foot,p,footlen)==0)
					  || strcmp(footer2,p)==0
					  /* try to recognize lines with dates and page numbers */
					  /* skip into line */
					  || (header_m && header_m<linelen &&
						 strncmp(&header[header_m],&p[header_m],HEADFOOTMAX)==0)
					  || (footer_m && footer_m<linelen &&
						 strncmp(&footer[footer_m],&p[footer_m],HEADFOOTMAX)==0)
					  /* skip into line allowing for off-by-one */
					  || (header_m && header_m<linelen &&
						 strncmp(&header[header_m],&p[header_m+1],HEADFOOTMAX)==0)
					  || (footer_m && footer_m<linelen &&
						 strncmp(&footer[footer_m],&p[footer_m+1],HEADFOOTMAX)==0)
					  /* or two */
					  || (header_m && header_m<linelen &&
						 strncmp(&header[header_m],&p[header_m+2],HEADFOOTMAX)==0)
					  || (footer_m && footer_m<linelen &&
						 strncmp(&footer[footer_m],&p[footer_m+2],HEADFOOTMAX)==0)
					  /* or with reflected odd and even pages */
					  || (headlen && headlen<linelen &&
						 strncmp(head,&p[linelen-headlen],headlen)==0)
					  || (footlen && footlen<linelen &&
						 strncmp(foot,&p[linelen-footlen],footlen)==0)
					  )) {
				tagc=0; title=1; continue;
			}

			/* page numbers at end of line */
			for(i=0; p[i] && isdigit(p[i]); i++)
				/* empty */;
			if (&p[i]!=plain && !p[i]) {title=1; fFoot=1; continue;}
		}

		/*** interline spacing ***/
		/* multiple \n: paragraph mode=>new paragraph, line mode=>blank lines */
		/* need to chop up lines for Roff */

		/*tabgram[scnt]++;*/
		if (title) ncnt=(scnt!=oscnt || (/*scnt<4 &&*/ isupper(*p)));
		itabcnt = scnt/5;
		if (CurLine==1) {ncnt=0; tagc=0;} /* gobble all newlines before first text line */
		sect = (scnt==0 && isupper(*p));
		subsect = (fSubsections && (scnt==2||scnt==3));
		if ((sect || subsect) && ncnt>1) ncnt=1; /* single blank line between sections */
		(*fn)(BEGINLINE);
		if (/*fPara &&*/ ncnt) Pnew=1;
		title=0; /*ncnt=0;--moved down*/
		/*if (finTable) (*fn)(BEGINTABLELINE);*/
		oscnt=scnt; /*fotable=finTable;*/

/* let output modules decide what to do at the start of a paragraph
		if (fPara && !Pnew && (prevcmd==BEGINBODY || prevcmd==BEGINBULTXT)) {
			putchar(' '); I++;
		}
*/

		/*** identify structural sections and notify fn */

		/*if (fMan) {*/
/*			bulpair = (scnt<7 && (*p==c_bullet || *p=='-'));*/
			/* decode the below */
			bulpair = ((!auxindent || scnt!=lindent+auxindent) /*!bulpair*/
					 && ((scnt>=2 && scnt2>5) || scnt>=5 || (tagc>0 && tags[0].first==scnt) ) /* scnt>=2?? */
					 && (((*p==c_bullet || strchr("-+.",*p)!=NULL || falluc) && (ncnt || scnt2>4)) || 
					  (scnt2-s_avg>=2 && phrase[phraselen-1]!='.') ||
					  (scnt2>3 && s_cnt==1)
					  ));
			if (bulpair) {
				if (tagc>0 && tags[0].first==scnt) {
					k=tags[0].last;
					for (l=1; l<tagc; l++) {
						if (tags[l].first - k <=3)
							k=tags[l].last;
						else break;
					}
					phraselen=k-scnt;
					for (k=phraselen; plain[k]==' ' && k<linelen; k++) /* nothing */;
					if (k>=5 && k<linelen) hanging=k; else hanging=-1;
				} else if (scnt2) hanging=phraselen+scnt2;
				else hanging=5;
			} else hanging=0;

/*			hanging = bulpair? phraselen+scnt2 : 0;*/
/*if (bulpair) printf("hanging = %d\n", hanging);*/
			/* maybe, bulpair=0 would be best */
			/*end fMan}*/

		/* certain sections (subsections too?) special, like SEE ALSO */
		/* to make canonical name as plain, all lowercase */
		if (sect /*||subsect -- no widespread subsection names*/) {
		  for (j=0; (sectheadid=j)<RANDOM; j++) if (strcoloncmp2(plain,'\0',sectheadname[j],0)) break;
		}

		/* normalized section headers are put into mixed case */
		if (/*fNORM &&*/falluc && (sect || subsect)) casify(plain);

		if (sect) {
			poppush(BEGINSECTION); (*fn)(BEGINSECTHEAD);
			tocadd(plain, BEGINSECTION, CurLine);
		} else if (subsect && !osubsect) {
			poppush(BEGINSUBSECTION); (*fn)(BEGINSUBSECTHEAD);
			tocadd(plain, BEGINSUBSECTION, CurLine);
		} else if (bulpair) {
			/* used to be just poppush(BEGINBULPAIR); */
			if (!Pbp) poppush(BEGINBULPAIR);
			(poppush)(BEGINBULLET);
			fIP=1; /*grabphrase(plain);*/
		} else if (Pnew) {
			poppush(BEGINBODY);
		}
		Pnew=0;
		oldsectheadid = sectheadid;


		/* move change bars to left */
		if (fChangeleft && !fNORM) {
			if (fPara) (*fn)(CHANGEBAR);
			/* replace initial spaces with changebars */
			else for (i=0; i<ccnt; i++) { /*xputchar('|'); */ (*fn)(CHANGEBAR); }
		}

		/* show initial spaces */
		if (!fIQS && fcharout) {
			spaceout = (scnt>ccnt)?(scnt-ccnt):0;
			if (fILQS) { if (spaceout>=lindent) spaceout-=lindent; else spaceout=0; }
			if (auxindent) { if (spaceout>=auxindent) spaceout-=auxindent; else spaceout=0; }
			if (fNORM) {
			  	if (itabcnt>0) (*fn)(ITAB);
				for (i=0; i<(scnt%5); i++) putchar(' ');
			} else printf("%*s",spaceout,"");
		}


		/*** iterate over each character in line, ***/
		/*** handling underlining, tabbing, copyrights ***/

		off=(!fIQS&&!fPara)?scnt:0;
		for (i=0, p=plain, curtag=0, fcont=0; *p; p++,i++,fcont=0) {
			/* interspersed presentation signals */
			/* start tags in reverse order of addition (so structural first) */
			if (curtag<tagc && i+I0+off==tags[curtag].first) {
				for (r=hitxt, j=tags[curtag].last-tags[curtag].first, hitxt[j]='\0'; j; j--)
					hitxt[j-1]=p[j-1];
				(*fn)(tagbeginend[tags[curtag].type][0]);
			}

			/* special characters */
			switch(*p) {
			   case '"':
				if (p==plain || isspace(p[-1])) { (*fn)(CHARLQUOTE); fcont=1; }
				else if (isspace(p[1])) { (*fn)(CHARRQUOTE); fcont=1; }
				break;
			   case '\'':
				if (p==plain || isspace(p[-1])) { (*fn)(CHARLSQUOTE); fcont=1; }
				else if (isspace(p[1])) { (*fn)(CHARRSQUOTE); fcont=1; }
				break;
			   case '-':
				/* check for -opt => \-opt */
				if (p==plain || (isspace(p[-1]) && !isspace(p[1]))) {
				  (*fn)(CHARDASH); fcont=1;
				}
				break;
			}

			/* troublemaker characters */
			c = (*p)&0xff;
			if (!fcont && fcharout) {
				if (strchr(escchars,c)!=NULL) {
					putchar('\\'); putchar(c); I++;
				} else if (strchr(trouble,c)!=NULL) {
					(*fn)(c); fcont=1;
				} else {
					putchar(c); I++;
				}
			}

/*default:*/
			if (curtag<tagc && i+I0+off+1==tags[curtag].last) {
				(*fn)(tagbeginend[tags[curtag].type][1]);
				curtag++;
			}

			if (fIP && ((*p==' ' && i==phraselen) || *p=='\0')) {
				p++;  /* needed but why? */
				(*fn)(ENDBULLET); fIP=0;
				if (*p!='\0') {
					/*oscnt+=phraselen;*/
					oscnt+=i;
					for (r=p; *r==' '; r++) {
						oscnt++;
/*
						i++;
						if (fQS || !fcharout) p++;
*/
					}
				}
				p--;	/* to counteract increment in loop */

				poppush(BEGINBULTXT);
			}
		}


		/*** end of line in buf[] ***/
		/*** deal with section titles, hyperlinks ***/

		if (sect) { (*fn)(ENDSECTHEAD); Pnew=1; }
		else if (subsect) { (*fn)(ENDSUBSECTHEAD); Pnew=1; }
		else if (fIP) { (*fn)(ENDBULLET); fIP=0; poppush(BEGINBULTXT); }
/* oscnt not right here */
		else if (scnt+linelen+spcsqz<MINRM /*&& ncnt*/ && lookahead!='\n'
			    && prevcmd!=BEGINBULTXT && prevcmd!=ENDSUBSECTHEAD && prevcmd!=ENDSUBSECTHEAD)
			(*fn)(SHORTLINE);
		osubsect=subsect;

		/*if (finTable) (*fn)(ENDTABLELINE);*/
		/*if (!fPara)*/ (*fn)(ENDLINE); tagc=0;
		ncnt=0;
		I0=I;	/* save I here in case skipping lines screws it up */
	}

	/* wrap up at end */
	pop(ENDDOC);	/* clear up all tags on stack */
	(*fn)(ENDDOC);
}



/*
 * SOURCE CODE PARSING
 *    for better transcription short of full nroff interpreter
 *
 *    Macros derived empirically, except for weird register ones that were looked up in groff
 *
 * buffer usage
 *    buf = incoming text from man page file
 *    plain = "second pass" buffer used to identify man page references
 *
 * test pages
 *    Solaris: fdisk.1m, fcntl.2, curs_getwch.3x, locale.5 (numbered lists),
 *		getservbyname.3n (font size changes)
 */

/* macros */
/*
/* put as much in here, as opposed to in code, as possible.
   less expensive and here they can be overridden by other macros */
/*const int macromax=100; -- dumb compiler */
#define MACROMAX 1000
struct { char *key; char *subst; } macro[MACROMAX] = {
  /* Solaris */
  {"NA", ".SH NAME"},
  {"SB", "\\s-2\\fB\\$1\\fR\\s0"},
  /* HP-UX */
/*
  {"SM", "\\s-2\\$1\\s0"},
  {"C", "\\f3\\$1\\fR"},
  {"CR", "\\f3\\$1\\fR\\$2"},
  {"CI", "\\f3\\$1\\fI\\$2\\fR"},
  {"RC", "\\fR\\$1\\f3\\$2\\fR"},
*/
  /* SGI -- doesn't ship man page source */

  /* 4.4BSD  -  http://intergate.sonyinteractive.com/cgi-bin/manlink/7/mdoc */
  /* scads more, but for them definition in -mandoc is sufficient */
  /*
  {"Dt", ".TH \\$1 \\$2 \\$3"},
  {"Sh", ".SH \\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9"},
  {"Ss", ".SS \\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9"},
  {"Pp", ".P"},
  {"Nm", ".BR \\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9"},	/* name * /
  {"Ar", ".IR \\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9"},
  */
  { NULL, NULL }
};
/* what all can be represented as a macro? */
int macrocnt=-1;	/* length of table counted at start */

char *macnotfound[MACROMAX];
int macnotcnt=0;

#define SUBSTMAX 1000
/* "*" or "*(" prefixed */
struct { char *key; char *subst; } subst[SUBSTMAX] = {
  {"rq", "'"}, {"lq", "`"}, {"L\"", "``"}, {"R\"", "''"},
  {"L", "\\f3"}, {"E", "\\f2"}, {"V", "\\f4"}, {"O", "\\f1"}
};
int substcnt=8;

#define REGISTERMAX 1000
struct { char *name; char *value; } reg[REGISTERMAX];
int regcnt=0;

/* special characters */
const struct { char key[4]; unsigned char subst[4]; } spec[] = {
  { "**", "*" },
  { "+-", { CHARPLUSMINUS, '\0' }},
  { "12", { CHAR12, '\0' }},
  { "14", { CHAR14, '\0' }},
  { "34", { CHAR34, '\0' }},
  { "aa", { CHARACUTE, '\0' }},
  { "ap", "~" },
  { "br", "|" },
  { "bu", { CHARBULLET, '\0' }},
  { "bv", "|" },
  { "ci", "O" },
  { "co", { CHARCOPYR, '\0' }},
  { "ct", { CHARCENT, '\0' }},
  { "de", { CHARDEG, '\0' }},
  { "dg", { CHARDAGGER, '\0' }},
  { "di", { CHARDIV, '\0' }},
  { "em", "--" },
  { "eq", "=" },
  { "hy", "-" },
  { "mi", "-" },
  { "mu", { CHARMUL, '\0' }},
  { "no", { CHARNOT, '\0' }},
  { "or", "|" },
  { "pl", "+" },
  { "rg", { CHARREGTM, '\0' }},
  { "ru", "_" },
  { "sc", { CHARSECT, '\0' }},
  { "sl", "/" },
  { "ua", "^" },
  { "ul", "_" }
};
#define speccnt (sizeof spec / sizeof spec[0])
 
/* tbl line descriptions */
char *tbl[20][20];	/* space enough for twenty description lines, twenty parts each */
int tblc=0;
int tbli;

int fsourceTab = 0, fosourceTab=0;
int supresseol=0;
int finitDoc=0;
int sublevel=0;

char *
source_gets(void) {
  char *p,*q;
  char *ret = (*in)?buf:NULL;
  int i;
  char tmpbuf[MAXBUF];
  char name[3];

  if (!sublevel) AbsLine++;

  p = tmpbuf;
  falluc = 1;
  while (1) {
    /* collect characters in line */
    while (*in && *in!='\n') {
	 if (p[0]=='\\' && p[1]=='\n') p+=2;	/* \<newline> */
	 falluc = falluc && !islower(*in);
	 *p++ = *in++;
    }
    if (*in) in++;
    *p='\0';

    /* normalize commands */
    p=tmpbuf; q=buf;	/* copy from tmpbuf to buf */
    /* no spaces between command-initiating period and command letters */
    if (*p=='\'') { *p='.'; }	/* what's the difference? */
    if (*p=='.') { *q++ = *p++; while (isspace(*p)) p++; }


    /* convert lines with tabs to tables? */
    fsourceTab=0;

    /* if comment at start of line, OK */
    /* dynamically determine iff Tcl/Tk page by scanning comments */
    if (*p=='\\' && *(p+1)=='"') {
	    if (!fTclTk && strstr(p+1,"supplemental macros used in Tcl/Tk")!=NULL) fTclTk=1;
	    p+=2;
    }

    while (*p) {
	 if (*p=='\t') fsourceTab++;
	 if (*p=='\\') {
	   p++;
	   if (*p=='n') {
		p++;
		if (*p=='(') {
		  p++; name[0]=*p++; name[1]=*p++; name[2]='\0';
		} else {
		  name[0]=*p++; name[1]='\0';
		}
		*q='0'; *(q+1)='\0';	/* defaults to 0, in case doesn't exist */
		for (i=0; i<regcnt; i++) {
		  if (strcmp(reg[i].name,name)==0) {
		    strcpy(q, reg[i].value);
		    break;
		  }
		}
		q+=strlen(q);
	   } else if (*p=='"') {	/* comment in Digital UNIX, OK elsewhere? */
		*p='\0';
		q--; while (q>buf && isspace(*q)) q--;	/* trim tailing whitespace */
		q++; *q='\0';
	   } else {
		/* verbatim character (often a backslash) */
		*q++ = '\\';	/* postpone interpretation (not the right thing but...) */
		*q++ = *p++;
	   }
	 } else *q++ = *p++;
    }

    /* dumb Digital--later */
    /*if (q-3>plain && q[-1]=='{' && q[-2]=='\\' && q[-3]==' ') q[-3]='\n';*/

    /* close off buf */
    *q='\0';

    /*if (q>buf && q[-1]=='\\' && *in=='.') { /* append next line * /} else break;*/
    break;
  }

  /*printf("*ret = |%s|\n", ret!=NULL?ret : "NULL");*/
  return ret;
}


/* dump characters from buffer, signalling right tags along the way */
/* all this work to introduce an internal second pass to recognize man page references */
/* now for HTTP references too */

int sI=0;
/* use int linelen from up top */
int fFlush=1;

void
source_flush(void) {
  int i,j;
  char *p,*q,*r;
  int c;
  int manoff,posn;

  if (!sI) return;
  plain[sI] = '\0';

  /* flush called often enough that all man page references are at end of text to be flushed */
  /* find man page ref */
  if (sI>=4/*+1*/ && (plain[sI-(manoff=1)-1]==')' || plain[sI-(manoff=0)-1]==')')) {
    for (q=&plain[sI-manoff-1-1]; q>plain && isalnum(*q) && *q!='('; q--) /* nada */;
    if (*q=='(' && strcoloncmp(&q[1],')',vollist)) {
	 r=q-1;
	 if (*r==' ' && (sectheadid==SEEALSO || /*single letter volume */ *(q+2)==')' || *(q+3)==')')) r--;	/* permitted single intervening space */
	 for ( ; r>=plain && (isalnum(*r) || strchr(manvalid,*r)!=NULL); r--) /* nada */;
	 r++;
	 if (isalpha(*r) && r<q) {
	   /* got one: clear out tags and spaces to make normalized form */
	   posn = r-plain;
	   /*while (tagc && tags[tagc-1].first >= posn) tagc--;*/

	   /* add MANREF tags */
	   strcpy(hitxt,r);
	   tagadd(BEGINMANREF, posn, 0);
	   /* already generated other start tags, so move BEGINMANREF to start in order to be well nested (ugh) */
	   tagtmp = tags[tagc-1]; for (j=tagc-1; j>0; j--) tags[j]=tags[j-1]; tags[0]=tagtmp;
	   tagadd(ENDMANREF, sI-manoff-1+1, 0);
	 }
    }

  /* HTML hyperlinks */
  } else if (fURL && sI>=4 && (p=strstr(plain,"http"))!=NULL) {
    i = p-plain;
    tagadd(BEGINMANREF, i, 0); tagtmp = tags[tagc-1]; for (j=tagc-1; j>0; j--) tags[j]=tags[j-1]; tags[0]=tagtmp;
    for (j=0; i<sI && !isspace(*p) && *p!='"' && *p!='>'; i++,j++) hitxt[j] = *p++;
    hitxt[j]='\0';
    tagadd(ENDMANREF, i, 0);
  }

  if (!fFlush) return;

  /* output text */
  for (i=0,j=0,p=plain; i<sI && *p; i++,p++) {
    if (!linelen) (*fn)(BEGINLINE);	/* issue BEGINLINE when know will be chars on line */

    /* dump tags */
    /*for ( ; j<tagc && tags[j].first == i; j++) (*fn)(tags[j].type);*/
    for (j=0; j<tagc; j++) if (tags[j].first == i) (*fn)((enum command)tags[j].type);

    /* dump text */
    c = (*p)&0xff;	/* just make c unsigned? */
    if (strchr(escchars,c)!=NULL) {
	 xputchar('\\'); xputchar(c);
	 if (fcharout) linelen++;
    } else if (strchr(trouble,c)!=NULL) {
	 (*fn)(c);
    } else if (linelen>=LINEBREAK && c==' ') { (*fn)(ENDLINE); linelen=0;
    } else {		/* normal character */
	 xputchar(c);
	 if (fcharout) linelen++;
    }

    /*if (linelen>=LINEBREAK && c==' ') { (*fn)(ENDLINE); linelen=0; } -- leaves space at end of line*/
  }
  /* dump tags at end */
  /*for ( ; j<tagc && tags[j].first == sI; j++) (*fn)(tags[j].type);*/
  for (j=0; j<tagc; j++) if (tags[j].first==sI) (*fn)((enum command)tags[j].type);

  sI=0; tagc=0;
}


/* source_out stuffs characters in a buffer */
char *
source_out0(char *p, char end) {
  /* stack of character formattings */
  static enum tagtype styles[20];
  static int style=-1;
  int funwind=0;
  int i, j;
  int len;
  int sign;

  while (*p && *p!=end) {
    if (*p=='\\') {	/* escape character */
	 switch (*++p) {
 
 	 case '&':	/* no space.  used as a no-op sometimes */
 	 case '^':	/* 1/12 em space */
 	 case '|':      /* 1/6 em space */
 	 case '%':      /* hyphenation indicator */
 	   /* just ignore it */
 	   p++;
 	   break;
 	 case '0':	/* digit width space */
 	   p++;
 	   sputchar(' ');
 	   break;
 	 case ' ':	/* unpaddable space */
 	   stagadd(CHARNBSP);	/* nonbreaking space */
 	   /*sputchar(' ');*/
 	   p++;
 	   break;
 	 case 's':	/* font size change */
	   p++;
	   sign=1;
	   if (*p=='-' || *p=='+') if (*p++=='-') sign=-1;
	   intArg = sign * ((*p++)-'0');
	   if (intArg==0) intArg = -fontdelta;	/* s0 returns to normal height */
	   if (fontdelta || intArg) { source_flush(); (*fn)(FONTSIZE); }
	   break;
 	 case 'v':	/* vertical motion */
 	 case 'h':	/* horizontal motion */
 	 case 'L':	/* vertical line */
 	 case 'l':	/* horizontal line */
	   /* ignore */
	   p++;
	   if (*p=='\'') { p++; while (*p++!='\''); }
	   break;
	 case '"':	/* comment */
	   *p='\0';	/* rest of line is comment */
	   break;
	 case 'f':
	   p++;
	   switch (*p++) {
	   case '3': case 'B':		/* boldface */
		styles[++style] = BOLD;
		stagadd(BEGINBOLD);
		break;
	   case '2': case 'I':		/* italics */
		styles[++style] = ITALICS;
		stagadd(BEGINITALICS);
		break;
	   case '4':	 /* bolditalics mode => program code */
		styles[++style] = BOLDITALICS;
		stagadd(BEGINBOLDITALICS);
		break;
	   case '1': case '0': case 'R': case 'P':	/* back to Roman */
		/*sputchar(' '); -- taken out; not needed, I hope */
		funwind=1;
		break;
	   case '-':
		p++;
		break;
	   }
	   break;
	 case '(':	/* multicharacter macros */
	   p++;
	   for (i=0; i<speccnt; i++) {
		if (p[0]==spec[i].key[0] && p[1]==spec[i].key[1]) {
		  p+=2;
		  for (j=0; spec[i].subst[j]; j++) sputchar(spec[i].subst[j]);
		  break;
		}
	   }
	   break;
	 case '*':	/* strings */
	   p++;
 	   if (*p!='(') {	/* single character */
 	     for (i=0; i<substcnt; i++) {
		  if (*p==subst[i].key[0] && subst[i].key[1]=='\0') {
		    source_out0(subst[i].subst,'\0');
		    break;
		  }
 	     }
 	     p++;
	   } else {	/* multicharacter macros */
		p++;
		for (i=0; i<substcnt; i++) {
		  len = strlen(subst[i].key);
		  if (strncmp(p,subst[i].key,len)==0) {
		    source_out0(subst[i].subst,'\0');
		    p+=len;
		    break;
		  }
		}
	   }
	   break;
	   /*------------------
	 } else if (*p=='|') {
	   stagadd(CHARNBSP);	/* nonbreaking space * /
	   /*sputchar(' ');* /
	   p++;
	   -------------------*/
	 case 'e':		/* escape */
	   sputchar('\\');
	   p++;
	   break;
	 case 'c':
		/* supress following carriage return-induced space */
	   /* handled in source_gets(); ignore within line => can't because next line might start with a command */
	   supresseol=1;
	   p++;
	   break;
	 case '-':		/* minus sign */
	   sputchar(CHARDASH);
	   p++;
	   break;
	   /*-----------------------
	 } else if (*p=='^') {
	   /* end stylings? (found in Solaris) * /
	   p++;
	   -------------------*/
	 default:		/* unknown escaped character */
	   sputchar(*p++);
	 }

    } else {		/* normal character */
	 if (*p) sputchar(*p++);
    }


    /* unwind character formatting stack */
    if (funwind) {
	 for ( ; style>=0; style--) {
	   if (styles[style]==BOLD) stagadd(ENDBOLD);
	   else if (styles[style]==ITALICS) stagadd(ENDITALICS);
	   else stagadd(ENDBOLDITALICS);
	 } /* else error */
	 assert(style==-1);

	 funwind=0;
    }

    /* check for man page reference and flush buffer if safe */
    /* postpone check until after following character so catch closing tags */
    if ((sI>=4+1 && plain[sI-1-1]==')') ||
	   /*  (plain[sI-1]==' ' && (q=strchr(plain,' '))!=NULL && q<&plain[sI-1])) {*/
	 (plain[sI-1]==' ' && !isalnum(plain[sI-1-1]))) {
	 /* regardless, flush buffer */
	 source_flush();
    }
  }

  if (*p && *p!=' ') p++;		/* skip over end character */
  return p;
}

/* oh, for function overloading.  inlined by compiler, probably */
char *source_out(char *p) {
  return source_out0(p,'\0');
}


char *
source_out_word(char *p) {
  char end = ' ';

  while (*p && isspace(*p)) p++;
  if (*p=='"' /* || *p=='`' ? */) {
    end = *p;
    p++;
  }
  p = source_out0(p,end);
  /*while (*p && isspace(*p)) p++;*/
  return p;
}


void
source_struct(enum command cmd) {
  source_out("\\fR\\s0");	/* don't let run-on stylings run past structural units */
  source_flush();
  if (cmd==SHORTLINE) linelen=0;
  (*fn)(cmd);
}

#define checkcmd(str)	strcmp(cmd,str)==0

int finnf=0;

void source_line(char *p);
void
source_subfile(char *newin) {
  char *p;
  char *oldin = in;

  sublevel++;

  in = newin;
  while ((p=source_gets())!=NULL) {
    source_line(p);
  }
  in = oldin;

  sublevel--;
}

/* have to delay acquisition of list tag */
void
source_list(void) {
  static int oldlisttype;	/* OK to have just one because nested lists done with RS/RE */
  char *q;
  int i;

  /* guard against empty bullet */
  for (i=0, q=plain; i<sI && isspace(*q); q++,i++) /* empty */;
  if (i==sI) return;

  assert(finlist);

  fFlush=1;

  /* try to determine type of list: DL, OL, UL */
  q=plain; plain[sI]='\0';
  if (/*c==CHARBULLET || q=='-' -- command line opts! ||*/ *q=='.' || *q&0x80) {
    listtype=UL;
    q++;
  } else {
    if (strchr("[(",*q)) q++;
    while (isdigit(*q)) { listtype=OL; q++; }	/* I hope this gives the right number */
    if (*q=='.') q++;
    if (strchr(")]",*q)) q++;
    if (*q=='.') q++;
    while (isspace(*q)) q++;
    if (*q) listtype=DL;
  }
  oldlisttype = listtype;

  /* interpretation left to output formats based on listtype (HTML: DL, OL, UL) */
  i = sI; sI=0;
  if (!Pbp || listtype!=oldlisttype) poppush(BEGINBULPAIR);
  poppush(BEGINBULLET);
  /*if (tphp) source_line(p); else source_out_word(p);*/
  /*if (listtype!=OL && listtype!=UL)*/ sI=i;
  source_struct(ENDBULLET); Pbul=0; /* handled immediately below */
  poppush(BEGINBULTXT);

  finlist=0;
}

static int inComment=0;
static int isComment=0;

void
source_command(char *p) {
  static int lastif=1;
  int mylastif;
  char *cmd=p;
  char *q;
  int i,j,endch;
  int fid;
  struct stat fileinfo;
  char *sobuf;
  char *macroArg[9];
  char endig[10];
  int err=0;
  char ch;
  int tphp=0;
  int ie=0;
  int cond,invcond=0;
  char delim,op;
  char if0[80], if1[80];
  float nif0, nif1;
  int insertat;
  char macrobuf[MAXBUF];		/* local so can have nested macros */
  static char ft='\0';
  static int fTableCenter=0;

  /* should centralize command matching (binary search?), pointer bumping here
     if for no other reason than to catch conflicts -- and allow overrides? */
  /* parse out command */
  while (*p && !isspace(*p)) p++;
  if (*p) { *p='\0'; p++; }
  /* should set up argv, argc for command arguments--it's regular enough that everyone doesn't have to do it itself */

  if (isComment) {
    /* maybe have BEGINCOMMENT, ENDCOMMENT, COMMENTLINE */
    supresseol=0;
    if (!inComment) { source_flush(); source_struct(BEGINCOMMENT); inComment=1; }
    source_struct(COMMENTLINE);
    printf("%s\n", p);	/* could contain --> or other comment closer, but unlikely */

  /* structural commands */
  } else if (checkcmd("TH")) {
    /* sample: .TH CC 1 "Dec 1990" */
    /* overrides command line -- should fix this */
    if (!finitDoc) {
	 while (isspace(*p)) p++;
	 if (*p) {
	   /* name */
	   q=strchr(p, ' '); if (q!=NULL) *q++='\0';
	   strcpy(manName, p);
	   /* number */
	   p = q;
	   if (p!=NULL) {
		 while (isspace(*p)) p++;
		 while (*p == '\"') p++;
		 if (*p) { q=strchr(p,' '); if (*(q-1) == '\"') *(q-1) = '\0';
			if (q!=NULL) *q++='\0';  }
	   }
	   strcpy(manSect, p!=NULL? p: "?");
	 }
	 sI=0;
	 finitDoc=1;
	 (*fn)(BEGINDOC);
	 /* emit information in .TH line? */
    } /* else complain about multiple definitions? */

  } else if (checkcmd("SH") || checkcmd("Sh")) {	/* section title */
    while (indent) { source_command("RE"); }
    source_flush();

    pop(BEGINSECTION);	/* before reset sectheadid */

    if (*p) {
	 if (*p=='"') { p++; q=p; while (*q && *q!='"') q++; *q='\0'; }
	 finnf=0;
	 for (j=0; (sectheadid=j)<RANDOM; j++) if (strcoloncmp2(p,'\0',sectheadname[j],0)) break;
	 if (!finitDoc) {
	   /* handle missing .TH */
	   /* if secthead!=NAME -- insist on this?
	   fprintf(stderr, "Bogus man page: no .TH or \".SH NAME\" lines\n");
	   exit(1);
	   */
	   (*fn)(BEGINDOC);
	   finitDoc=1;
	 }
	 poppush(BEGINSECTION); source_struct(BEGINSECTHEAD);
	 fFlush=0;
	 if (falluc) casify(p);
	 source_out(p);		/* people forget the quotes around multiple words */
	 while (isspace(plain[--sI])) /*nada*/;
	 plain[++sI]='\0'; tocadd(plain, BEGINSECTION, CurLine);	/* flushed with source_struct above */
	 fFlush=1;
	 source_struct(ENDSECTHEAD);
    }
  } else if (checkcmd("SS")) {	/* subsection title */
    while (indent) { source_command("RE"); }
    source_flush();

    if (*p) {
	 if (*p=='"') { p++; q=p; while (*q && *q!='"') q++; *q='\0'; }
	 finnf=0;
	 source_flush();
	 poppush(BEGINSUBSECTION); source_struct(BEGINSUBSECTHEAD);
	 fFlush=0;

	 if (falluc) casify(p);
	 source_out(p);		/* people forget the quotes around multiple words */
	 while (isspace(plain[--sI])) /*nada*/;
	 plain[++sI]='\0'; tocadd(plain, BEGINSUBSECTION, CurLine);
	 fFlush=1;
	 source_struct(ENDSUBSECTHEAD);
    }

  } else if (checkcmd("P") || checkcmd("PP") || checkcmd("LP")) {	 /* new paragraph */
    source_flush();
    poppush(BEGINBODY);

  } else if ((tphp=checkcmd("TP")) || (tphp=checkcmd("HP")) || checkcmd("IP") || checkcmd("LI")) {
    /* TP, HP: indented paragraph, tag on next line (DL list) */
    /* IP, LI: tag as argument */
    source_flush();
    fFlush=0;
    finlist=1;
    if (!tphp) { source_out_word(p); source_list(); }
    /* lists terminated only at start of non-lists */
  } else if (checkcmd("RS")) {	/* set indent */
    source_struct(BEGININDENT);
    indent++;
  } else if (checkcmd("RE")) {
    if (indent) indent--;
    pop(ENDINDENT);
    source_struct(ENDINDENT);
/*
  } else if (checkcmd("Xr")) {
    /* 4.4BSD man ref * /
    supresseol=0;
    p=source_out_word(p);
    source_out("(");
    p=source_out_word(p);
    source_out(")");
*/

  } else if (checkcmd("nf")) {
    source_struct(SHORTLINE); 
    finnf=1;
    source_struct(BEGINCODEBLOCK);
  } else if (checkcmd("fi")) {
    source_struct(ENDCODEBLOCK);
    finnf=0;
  } else if (checkcmd("br")) {
    source_struct(SHORTLINE);
  } else if (checkcmd("sp") || checkcmd("SP")) {	/* blank lines */
    /*if (!finTable) {*/
	 if (finnf) source_struct(SHORTLINE); else source_struct(BEGINBODY);
	 /*}*/
  } else if (checkcmd("ta")) {	/* set tab stop(s?) */
    /* argument is tab stop -- handle these as tables => leave to output format */
    /* HTML handles tables but not tabs, Tk's text tabs but not tables */
    /* does cause a linebreak */
    stagadd(BEGINBODY);
  } else if (checkcmd("ce")) {
	  /* get line count, recursively filter for that many lines */
	  if (sscanf(p, "%d", &i)) {
		  source_struct(BEGINCENTER);
		  for (; i>0 && (p=source_gets())!=NULL; i--) source_line(p);
		  source_struct(ENDCENTER);
	  }

  /* limited selection of control structures */
  } else if (checkcmd("if") || (checkcmd("ie"))) {	/* if <test> cmd, if <test> command and else on next line */
    supresseol=1;
    ie = checkcmd("ie");
    mylastif=lastif;

    if (*p=='!') { invcond=1; p++; }

    if (*p=='n') { cond=1; p++; }		/* masquerading as nroff the right thing to do? */
    else if (*p=='t') { cond=0; p++; }
    else if (*p=='(' || *p=='-' || *p=='+' || isdigit(*p)) {
	 if (*p=='(') p++;
	 nif0=atof(p);
	 if (*p=='-' || *p=='+') p++; while (isdigit(*p)) p++;
	 op = *p++;	/* operator: =, >, < */
	 if (op==' ') {
	   cond = (nif0!=0);
	 } else {
	   nif1=atoi(p);
	   while (isdigit(*p)) p++;
	   if (*p==')') p++;
	   if (op=='=') cond = (nif0==nif1);
	   else if (op=='<') cond = (nif0<nif1);
	   else /* op=='>' -- ignore >=, <= */ cond = (nif0>nif1);
	 }
    } else if (!isalpha(*p)) {	/* usually quote, ^G in Digital UNIX */
	 /* gobble up comparators between delimiters */
	 delim = *p++;
	 q = if0; while (*p!=delim) { *q++=*p++; } *q='\0'; p++;
	 q = if1; while (*p!=delim) { *q++=*p++; } *q='\0'; p++;
	 cond = (strcmp(if0,if1)==0);
    } else cond=0;	/* a guess, seems to be right bettern than half the time */
    if (invcond) cond=1-cond;
    while (isspace(*p)) p++;

    lastif = cond;
    if (strncmp(p,"\\{",2)==0) {	/* rather than handle groups here, have turn on/off output flag? */
	    p+=2; while (isspace(*p)) p++;
	    while (strncmp(p,".\\}",3)!=0 || strncmp(p,"\\}",2)!=0 /*Solaris*/) {
		    if (cond) source_line(p);
		    if ((p=source_gets())==NULL) break;
	    }
    } else if (cond) source_line(p);

    if (ie) source_line(source_gets());		/* do else part with prevailing lastif */

    lastif=mylastif;

  } else if (checkcmd("el")) {
    mylastif=lastif;

    /* should centralize gobbling of groups */
    cond = lastif = !lastif;
    if (strncmp(p,"\\{",2)==0) {
	 p+=2; while (isspace(*p)) p++;
	 while (strncmp(p,".\\}",3)!=0 || strncmp(p,"\\}",2)!=0 /*Solaris*/) {
	   if (cond) source_line(p);
	   if ((p=source_gets())==NULL) break;
	 }
    } else if (cond) source_line(p);

    lastif=mylastif;

  } else if (checkcmd("ig")) {	/* "ignore group" */
    strcpy(endig,".."); if (*p) { endig[0]='.'; strcpy(&endig[1],p); }
    while ((p=source_gets())!=NULL) {
	 if (strcmp(p,endig)==0) break;
	 if (!lastif) source_line(p);		/* usually ignore line, except in one weird case */
    }


  /* macros and substitutions */
  } else if (checkcmd("de")) {
    /* grab key */
    q=p; while (*q && !isspace(*q)) q++; *q='\0';

    /* if already have a macro of that name, override it */
    /* could use a good dictionary class */
    for (insertat=0; insertat<macrocnt; insertat++) {
	 if (strcmp(p,macro[insertat].key)==0) break;
    }
    if (insertat==macrocnt) macrocnt++;

    /* should replace one with same name, if one exists */
    macro[insertat].key = mystrdup(p);

    /* build up macro in plain[] ... */
    /* everything until ".." line part of macro */
    q=plain; i=0;
    while ((p=source_gets())!=NULL) {
	 if (strcmp(p,"..")==0) break;
	 while (*p) {	/* append string, interpreting quotes along the way--just double backslash to single now */
	   if (*p=='\\' && p[1]=='\\') p++;
	   *q++=*p++;
	 }
	 *q++='\n';
    }
    *q='\0';

    /* ... then copy once have whole thing */
    macro[insertat].subst = mystrdup(plain);
    /*fprintf(stderr, "defining macro %s as %s\n", macro[insertat].key, macro[insertat].subst);*/
    sI=0;

  } else if (checkcmd("rm")) {	/* remove macro definition, can have multiple arguments */
    for (i=0; i<macrocnt; i++) {	/* moot as new definitions replace old when conflicts */
	 if (strcmp(p,macro[i].key)) {
	   macro[i] = macro[--macrocnt];
	   break;
	 }
    }

  } else if (checkcmd("ds")) {	/* text substitutions (like macros) */
    /* /usr/sww/man/man1/CC.1 a good test of this */
    q = strchr(p,' ');
    if (q!=NULL) {
	 *q='\0'; while (*++q) /*nada*/;
	 if (*q=='"') q++;
	 if (substcnt<SUBSTMAX) {
	   subst[substcnt].key = mystrdup(p); subst[substcnt].subst = mystrdup(q); substcnt++;
	 }
	 /*fprintf(stderr, "defining substitution: name=%s, body=%s\n", p, q);*/
    }

  } else if (checkcmd("so")) {
    /* assuming in .../man like nroff, source in file and execute it as nested file, */
    /* so nested .so's OK */

	err = 1;	/* assume error unless successful */
	if (fTclTk) {
		err=0;
	} else if (stat(p, &fileinfo)==0) {
		sobuf = malloc(fileinfo.st_size + 1);
		if (sobuf!=NULL) {
			/* suck in entire file */
			fid = open(p, O_RDONLY);
			if (fid!=-1) {
				if (read(fid, sobuf, fileinfo.st_size) == fileinfo.st_size) {
				  sobuf[fileinfo.st_size]='\0';
				  /* dumb Digital puts \\} closers on same line */
				  for (q=sobuf; (q=strstr(q," \\}"))!=NULL; q+=3) *q='\n';
				  source_subfile(sobuf);
				  err = 0;
				}
				close(fid);
			}
			free(sobuf);
		}
	}

	if (err) {
		fprintf(stderr, "%s: couldn't read in referenced file %s.\n", argv0, p);
		if (strchr(p,'/')==NULL) {
		  fprintf(stderr, "\tTry cd'ing into same directory of man page first.\n");
		} else if (strchr(p,'/')==strrchr(p,'.')) {
		  fprintf(stderr, "\tTry cd'ing into parent directory of man page first.\n");
		} else {
		  fprintf(stderr, "\tTry cd'ing into ancestor directory that makes relative path valid first.\n");
		}
		exit(1);
	}


  /* character formatting */
  /* reencode m/any as macro definitions? would like to but the below don't have "words" */
  } else if (checkcmd("ft")) {	/* change font, next char is R,I,B.  P=previous not supported */
    if (ft=='B') stagadd(ENDBOLD); else if (ft=='I') stagadd(ENDITALICS);
    ft = *p++;
    if (ft=='B') stagadd(BEGINBOLD); else if (ft=='I') stagadd(BEGINITALICS);
  } else if (checkcmd("B")) {
    supresseol=0;
    stagadd(BEGINBOLD); p = source_out_word(p); source_out(p); stagadd(ENDBOLD);
  } else if (checkcmd("I")) {
    supresseol=0;
    stagadd(BEGINITALICS); p = source_out_word(p); stagadd(ENDITALICS);
    source_out(p);
  } else if (checkcmd("BI")) {
    supresseol=0;
    while (*p) {
	 stagadd(BEGINBOLD); p = source_out_word(p); stagadd(ENDBOLD);
	 if (*p) { stagadd(BEGINITALICS); p = source_out_word(p); stagadd(ENDITALICS); }
    }
  } else if (checkcmd("IB")) {
    supresseol=0;
    while (*p) {
	 stagadd(BEGINITALICS); p = source_out_word(p); stagadd(ENDITALICS);
	 if (*p) { stagadd(BEGINBOLD); p = source_out_word(p); stagadd(ENDBOLD); }
    }
  } else if (checkcmd("RB")) {
    supresseol=0;
    while (*p) {
	 p = source_out_word(p);
	 if (*p) { stagadd(BEGINBOLD); p = source_out_word(p); stagadd(ENDBOLD); }
    }
  } else if (checkcmd("BR")) {
    supresseol=0;
    while (*p) {
	 stagadd(BEGINBOLD); p = source_out_word(p); stagadd(ENDBOLD);
	 p = source_out_word(p);
    }
  } else if (checkcmd("IR")) {
    supresseol=0;
    while (*p) {
	 stagadd(BEGINITALICS); p=source_out_word(p); stagadd(ENDITALICS);
	 p=source_out_word(p);
    }
  } else if (checkcmd("RI")) {
    supresseol=0;
    while (*p) {
	 p=source_out_word(p);
	 stagadd(BEGINITALICS); p=source_out_word(p); stagadd(ENDITALICS);
    }


  /* HP-UX */
  } else if (checkcmd("SM")) {
    supresseol=0; source_out("\\s-1"); while (*p) p=source_out(p); source_out("\\s0");
  } else if (checkcmd("C")) {
    supresseol=0;
    stagadd(BEGINCODE); while (*p) p=source_out_word(p); stagadd(ENDCODE);
  } else if (checkcmd("CR")) {
    supresseol=0;
    while (*p) {
	 stagadd(BEGINCODE); p=source_out_word(p); stagadd(ENDCODE);
	 if (*p) p=source_out_word(p);
    }
  } else if (checkcmd("RC")) {
    supresseol=0;
    while (*p) {
	 p=source_out_word(p);
	 if (*p) { stagadd(BEGINCODE); p=source_out_word(p); stagadd(ENDCODE); }
    }
  } else if (checkcmd("CI")) {
    supresseol=0;
    while (*p) {
	 stagadd(BEGINCODE); p=source_out_word(p); stagadd(ENDCODE);
	 if (*p) { stagadd(BEGINITALICS); p=source_out_word(p); stagadd(ENDITALICS); }
    }


  /* tables */
  } else if (checkcmd("TS")) {
    tblc=0; /*tblspanmax=0;*/ tableSep='\0';		/* need to reset each time because tabbed lines (.ta) made into tables too */
    while ((p = source_gets())!=NULL) {
	 if ((q=strstr(p,"tab"))!=NULL) {	/* "tab(" or "tab (".  table entry separator */
	   p=(q+3); while (isspace(*p)) p++;
	   p++;	/* jump over '(' */
	   tableSep=*p;
	   continue;
	 }
	 if (strincmp(p,"center",strlen("center"))==0) {	/* center entire table; should look for "left" and "right", probably */
	   fTableCenter=1; source_struct(BEGINCENTER);
	   p+=strlen("center"); while (isspace(*p)) p++;
	   continue;
	 }
	 if (p[strlen(p)-1]==';') { tblc=0; continue; }	/* HP has a prequel terminated by ';' */

	 for (i=0; *p; i++,p=q) {
	   if (*p=='.') break;
	   if (*p=='f') p+=2;	/* DEC sets font here */
	   q=p+1;
	   if (strchr("lrcn",*q)==NULL) {	/* dumb DEC doesn't put spaces between them */
		while (*q && *q!='.' && !isspace(*q)) q++;
	   }
	   ch=*q; *q='\0';
	   tbl[tblc][i] = mystrdup(p);
	   tbl[tblc][i+1] = "";	/* mark end */
	   *q=ch;
	   while (*q && isspace(*q)) q++;
	 }
	 /*if (i>tblspanmax) tblspanmax=i;*/
	 tbl[tblc++][i]="";		/* mark end */
	 if (*p=='.') break;
    }
    tbli=0;
    source_struct(BEGINTABLE);

    while ((p=source_gets())!=NULL) {
	 if (strncmp(p,".TE",3)==0) break;
	 if (*p=='.') { source_line(p); continue; }

	 /* count number of entries on line.  if >1, can use to set tableSep */
	 insertat=0; for (j=0; *tbl[tbli][j]; j++) if (*tbl[tbli][j]!='s') insertat++;
	 if (!tableSep && insertat>1) if (fsourceTab) tableSep='\t'; else tableSep='@';
	 source_struct(BEGINTABLELINE);
	 if (strcmp(p,"_")==0 || /* double line */ strcmp(p,"=")==0) {
	   source_out(" ");
	   /*stagadd(HR);*/	/* empty row -- need ROWSPAN for HTML */
	   continue;
	 }

	 for (i=0; *tbl[tbli][i] && *p; i++) {
	   tblcellspan=1;
	   tblcellformat = tbl[tbli][i];
	   if (*tblcellformat=='^') {	/* vertical span => blank entry */
		tblcellformat="l";
	   } else if (*tblcellformat=='|') {
		/* stagadd(VBAR); */
		continue;
	   } else if (strchr("lrcn", *tblcellformat)==NULL) {
		tblcellformat="l";
		/*continue;*/
	   }

	   while (strncmp(tbl[tbli][i+1],"s",1)==0) { tblcellspan++; i++; }

	   source_struct(BEGINTABLEENTRY);
	   if (toupper(tblcellformat[1])=='B') stagadd(BEGINBOLD);
	   else if (toupper(tblcellformat[1])=='I') stagadd(BEGINITALICS);
	   /* not supporting DEC's w(<num><unit>) */

	   if (strcmp(p,"T{")==0) {	/* DEC, HP */
		while (strncmp(p=source_gets(),"T}",2)!=0) source_line(p);
		p+=2; if (*p) p++;
	   } else {
		p = source_out0(p, tableSep);
	   }
	   if (toupper(tblcellformat[1])=='B') stagadd(ENDBOLD);
	   else if (toupper(tblcellformat[1])=='I') stagadd(ENDITALICS);
	   source_struct(ENDTABLEENTRY);
	 }
	 if (tbli+1<tblc) tbli++;
	 source_struct(ENDTABLELINE);
    }
    source_struct(ENDTABLE);
    if (fTableCenter) { source_struct(ENDCENTER); fTableCenter=0; }


  } else if (checkcmd("nr")) {
    q=p; while (*q && !isspace(*q)) q++; *q='\0'; q++;

    for (insertat=0; insertat<regcnt; insertat++) {
	 if (strcmp(reg[insertat].name,p)==0) break;
    }
    if (insertat==regcnt) { regcnt++; reg[insertat].name = mystrdup(p); } /* else use same name */
    p=q;
    if (*q=='+' || *q=='-') q++;	/* accept signed, floating point numbers */
    if (*q=='.') q++;
    if (!*q || isspace(*q)) { *q++='0'; *q++='\0'; }
    while (isdigit(*q)) { q++; } *q='\0'; /* ignore units */
    reg[insertat].value = mystrdup(p);

  } else if (checkcmd("EQ")) {	/* eqn not supported */
    source_out("\\s-1\\fBeqn not supported\\fR\\s0");
    while ((p=source_gets())!=NULL) {
	 if (strncmp(p,".EN",3)==0) break;
    }



  /* Tcl/Tk macros */
  } else if (fTclTk && (checkcmd("VS") || checkcmd("VE"))) {
	  /* nothing for sidebars */
  } else if (fTclTk && checkcmd("OP")) {
    source_struct(BEGINBODY);
    for (i=0; i<3; i++) {
	 if (fcharout) { source_out(tcltkOP[i]); source_out(": "); }
	 stagadd(BEGINBOLD); p=source_out_word(p); stagadd(ENDBOLD); 
	 source_struct(SHORTLINE);
    }
    source_struct(BEGINBODY);

  } else if (fTclTk && checkcmd("BS")) {	/* box */
    /*source_struct(HR); -- ugh, no Ouster box */
  } else if (fTclTk && checkcmd("BE")) {
    /*source_struct(HR);*/

  } else if (fTclTk && (checkcmd("CS")||checkcmd("DS"))) {	/* code excerpt */
    /* respect line ends, set in teletype */
    /* source_struct(SHORTLINE); -- done as part of CS's ENDLINE */
    finnf=1;
    source_struct(SHORTLINE);
    if (checkcmd("DS")) source_line(".P");
    stagadd(BEGINCODE);
  } else if (fTclTk && (checkcmd("CE")||checkcmd("DE"))) {
    stagadd(ENDCODE);
    finnf=0;

  } else if (fTclTk && checkcmd("SO")) {
    source_struct(BEGINSECTION);
    source_struct(BEGINSECTHEAD); source_out("STANDARD OPTIONS"); source_struct(ENDSECTHEAD);
    tblc=1; tbl[0][0]=tbl[0][1]=tbl[0][2]="l"; tbl[0][3]="";
    source_struct(BEGINTABLE);
    while (1) {
	 p = source_gets();
	 if ((strncmp(p,".SE",3))==0) break;
	 tblcellformat = "l";
	 source_struct(BEGINTABLELINE);
	 if (*p=='.') {
	   source_command(p);
	 } else {
	   while (*p) {
		source_struct(BEGINTABLEENTRY);
		p = source_out0(p, '\t');
		source_struct(ENDTABLEENTRY);
	   }
	 }
	 source_struct(ENDTABLELINE);
    }
    source_struct(ENDTABLE);
    source_struct(ENDSECTION);

  } else if (fTclTk && checkcmd("AP")) {	/* arguments */
    source_struct(BEGINBODY);
    p = source_out_word(p); source_out("   ");
    stagadd(BEGINITALICS); p = source_out_word(p); stagadd(ENDITALICS); source_out("\t");
    source_out("("); p = source_out_word(p); source_out(")");
    source_struct(SHORTLINE);
    source_out("\t");
  } else if (fTclTk && checkcmd("AS")) {	/* arguments */

/* let these be defined as macros.  if they're not, they're just caught as unrecognized macros
  } else if (checkcmd("ll") || checkcmd("IX") ||
		   checkcmd("nh")||checkcmd("hy")||checkcmd("hc")||checkcmd("hw")	/* hyphenation * /
	) {	/* unsupported macros -- usually roff specific * /

    fprintf(stderr, "macro \"%s\" not supported -- ignoring\n", cmd);
*/

  } else {		/* could be a macro definition */
    supresseol=0;

    for (i=0; i<macrocnt; i++) {
	 if (macro[i].key == NULL) continue;	/* !!! how does this happen? */
	 if (checkcmd(macro[i].key)) {

	   /* it is, collect arguments */
	   for (j=0; j<9; j++) macroArg[j]="";
	   for (j=0; p!=NULL && *p && j<9; j++, p=q) {
		endch=' '; if (*p=='"') { endch='"'; p++; }
		q = strchr(p,endch);
		if (q!=NULL) {
		  *q++='\0';
		  if (*q && endch!=' ') q++;
		}
		macroArg[j] = p;
	   }

	   /* instantiate that text, substituting \\[1-9]'s */
	   p = macro[i].subst;
	   q = macrobuf;	/* allocated on stack */
	   while (*p) {
		if (*p=='\\') {
		  p++;
		  if (*p=='t') {
		    *q++ = '\t';
		    p++;
		  } else if (*p=='$' && isdigit(p[1])) {
		    j = p[1]-'0'-1;	/* convert to ASCII and align with macroArg array */
		    p+=2;
		    /* *q++='"'; -- no */
		    strcpy(q, macroArg[j]); q += strlen(q);
		    /* *q++='"'; -- no */

		  } else {
		    *q++ = '\\';
		  }
		} else {
		  *q++ = *p++;
		}
	   }
	   *q='\0';

	   /* execute that text */
	   /*fprintf(stderr, "for macro %s, substituted text is \n%s\n", macro[i].key, macrobuf);*/
	   source_subfile(macrobuf);

	   break;
	 }
    }

    /* macro not found */
    if (i==macrocnt) {
	   /* report missing macros only once */
	   for (j=0; j<macnotcnt; j++) if (strcmp(macnotfound[j],cmd)==0) break;
	   if (j==macnotcnt) {
		  if (!fQuiet) fprintf(stderr, "macro \"%s\" not recognized -- ignoring\n", cmd);
		  q = malloc(strlen(cmd)+1); strcpy(q,cmd);
		  macnotfound[macnotcnt++] = q;
	   }
    }
  } /* else command is unrecognized -- ignore it: we're not a complete [tn]roff implementation */

  /* popular but meaningless commands: .ne (need <n> lines--on infinite scroll */
}


void
source_line(char *p) {
  /*stagadd(BEGINLINE);*/
  char *cmd=p;
  if (p==NULL) return;	/* bug somewhere else, but where? */

  isComment = (/*checkcmd("") ||*/ checkcmd("\\\"") || /*DEC triple dot*/checkcmd(".."));
  if (inComment && !isComment) { source_struct(ENDCOMMENT); inComment=0; }	/* special case to handle transition */

#if 0
  if (*p!='.' && *p!='\'' && !finlist) {
    if (fsourceTab && !fosourceTab) {
	 tblc=1; tbli=0; tableSep='\t';
	 tbl[0][0]=tbl[0][1]=tbl[0][2]=tbl[0][3]=tbl[0][4]=tbl[0][5]=tbl[0][6]=tbl[0][7]=tbl[0][8]="l";
	 source_struct(BEGINTABLE); finTable=1;
    } else if (!fsourceTab && fosourceTab) {
	 source_struct(ENDTABLE); finTable=0;
    }
    fosourceTab=fsourceTab;
  }
#endif

  if (*p=='.' /*|| *p=='\''  -- normalized */) {	/* command == starts with "." */
    p++;
    supresseol=1;
    source_command(p);

  } else if (!*p) {		/* blank line */
    /*source_command("P");*/
    ncnt=1; source_struct(BEGINLINE); ncnt=0;	/* empty line => paragraph break */

#if 0
  } else if (fsourceTab && !finlist /* && pmode */) {	/* can't handle tabs, so try tables */
    source_struct(BEGINTABLE);
    tblcellformat = "l";
    do {
	 source_struct(BEGINTABLELINE);
	 while (*p) {
	   source_struct(BEGINTABLEENTRY);
	   p = source_out0(p, '\t');
	   source_struct(ENDTABLEENTRY);
	 }
	 source_struct(ENDTABLELINE);
    } while ((p=source_gets())!=NULL && fsourceTab);
    source_struct(ENDTABLE);
    source_line(p);
#endif

  } else {			/* otherwise normal text */
    source_out(p);
    if (finnf || isspace(*cmd)) source_struct(SHORTLINE);
  }

  if (!supresseol && !finnf) { source_out(" "); if (finlist) source_list(); }
  supresseol=0;
  /*stagadd(ENDLINE);*/
}


void
source_filter(void) {
  char *p = in, *q;
  char *oldv,*newv,*shiftp,*shiftq,*endq;
  int lenp,lenq;
  int i,on1,on2,nn1,nn2,first;
  int insertcnt=0, deletecnt=0, insertcnt0;
  int nextDiffLine=-1;
  char diffcmd, tmpc, tmpendq;

  AbsLine=0;

  /* just count length of macro table! */
  for (i=0; macro[i].key!=NULL; i++) /*empty*/;
  macrocnt = i;

  /* dumb Digital puts \\} closers on same line */
  for (p=in; (p=strstr(p," \\}"))!=NULL; p+=3) *p='\n';

  sI=0;
  /* (*fn)(BEGINDOC); -- done at .TH or first .SH */


  /* was: source_subfile(in); */
  while (fDiff && fgets(diffline, MAXBUF, difffd)!=NULL) {
	  /* requirements: no context lines, no errors in files, ...
		 change-command: 8a12,15 or 5,7c8,10 or 5,7d3
		 < from-file-line
		 < from-file-line...
		 --
		 > to-file-line
		 > to-file-line...
	  */
	  for (q=diffline; ; q++) { diffcmd=*q; if (diffcmd=='a'||diffcmd=='c'||diffcmd=='d') break; }
	  if (sscanf(diffline, "%d,%d", &on1,&on2)==1) on2=on1-1+(diffcmd=='d'||diffcmd=='c');
	  if (sscanf(++q, "%d,%d", &nn1,&nn2)==1) nn2=nn1-1+(diffcmd=='a'||diffcmd=='c');

	  deletecnt = on2-on1+1;
	  insertcnt = nn2-nn1+1;

	  nextDiffLine = nn1;
	  /*assert(nextDiffLine>=AbsLine); -- can happen if inside a macro? */
	  if (nextDiffLine<AbsLine) continue;

	  while (AbsLine<nextDiffLine && (p=source_gets())!=NULL) {
		  source_line(p);
	  }

	  insertcnt0=insertcnt+1;	/* eat duplicate insert lines and '---' too */
	  diffline2[0] = '\0';
	  while (insertcnt && deletecnt) {
		  if (ungetc(fgetc(difffd),difffd)=='<') { fgetc(difffd); fgetc(difffd); }	/* skip '<' */		  
		  /* fill buffer with old line -- but replace if command */
		  /* stay away from commands -- too careful if .B <word> */
		  do {
			  p = oldv = fgets(diffline, MAXBUF, difffd);
			  p[strlen(p)-1]='\0';	/* fgets's \n ending => \0 */
			  deletecnt--;
		  } while (deletecnt && *p=='.');	/* throw out commands in old version */

		  q = newv = source_gets();
		  insertcnt--;
		  while (insertcnt && *q=='.') {
			  source_line(q);
			  insertcnt--;
		  }

		  if (*p=='.' || *q=='.') break;


		  /* make larger chunk for better diff -- but still keep away from commands */
		  lenp=strlen(p); lenq=strlen(q);
		  while (deletecnt && MAXBUF-lenq>80*2) {
			  fgetc(difffd); fgetc(difffd);	/* skip '<' */
			  if (ungetc(fgetc(difffd),difffd)=='.') break;
			  p=&diffline[lenp]; *p++=' '; lenp++;
			  fgets(p, MAXBUF-lenp, difffd); p[strlen(p)-1]='\0'; lenp+=strlen(p);
			  deletecnt--;
		  }

		  while (insertcnt && *in!='.' && MAXBUF-lenq>80*2) {
			  if (newv!=diffline2) { strcpy(diffline2,q); newv=diffline2; }
			  q=source_gets(); diffline2[lenq]=' '; lenq++;
			  strcpy(&diffline2[lenq],q); lenq+=strlen(q);
			  insertcnt--;
		  }

		  /* common endings */
		  p = &p[strlen(oldv)]; q=&q[strlen(newv)];
		  while (p>oldv && q>newv && p[-1]==q[-1]) { p--; q--; }
		  if ((p>oldv && p[-1]=='\\') || (q>newv && q[-1]=='\\'))
			  while (*p && *q && !isspace(*p)) { p++; q++; }	/* steer clear of escapes */
		  tmpendq=*q; *p=*q='\0'; endq=q;

		  p=oldv; q=newv;
		  while (*p && *q) {
			  /* common starts */
			  newv=q; while (*p && *q && *p==*q) { p++; q++; }
			  if (q>newv) {
				  tmpc=*q; *q='\0'; source_line(newv); *q=tmpc;
			  }

			  /* too hard to read */
			  /* difference: try to find hunk of p in remainder of q */
			  if (strlen(p)<15 || (shiftp=strchr(&p[15],' ') /*|| shiftp-p>30*/)==NULL) break;
			  shiftp++; /* include the space */
			  tmpc=*shiftp; *shiftp='\0'; shiftq=strstr(q,p); *shiftp=tmpc;	/* includes space */
			  if (shiftq!=NULL) {
			  	/* call that part of q inserted */
				tmpc=*shiftq; *shiftq='\0';
				stagadd(BEGINDIFFA); source_line(q); stagadd(ENDDIFFA); source_line(" ");
				*shiftq=tmpc; q=shiftq;
			  } else {
				/* call that part of p deleted */
				shiftp--;	*shiftp='\0';	/* squash the trailing space */
				stagadd(BEGINDIFFD); source_line(p); stagadd(ENDDIFFD); source_line(" ");
				p=shiftp+1;
			  }
/*#endif*/
		  }

		  if (*p) { stagadd(BEGINDIFFD); source_line(p); stagadd(ENDDIFFD); }
		  if (*q) { stagadd(BEGINDIFFA); source_line(q); stagadd(ENDDIFFA); }
		  if (tmpendq!='\0') { *endq=tmpendq; source_line(endq); }
		  source_line(" ");
	  }

	  /* even if diffcmd=='c', could still have remaining old version lines */
	  first=1;
	  while (deletecnt--) {
		  fgets(diffline, MAXBUF, difffd);
		  if (diffline[2]!='.') {
			  if (first) { stagadd(BEGINDIFFD); first=0; }
			  source_line(&diffline[2]);	/* don't do commands; skip initial '<' */
		  }
	  }
	  if (!first) { stagadd(ENDDIFFD); source_line(" "); }

	  /* skip over duplicated from old */
	  if (diffcmd=='c') while (insertcnt0--) fgets(diffline, MAXBUF, difffd);

	  /* even if diffcmd=='c', could still have remaining new version lines */
	  first=1;
	  nextDiffLine = AbsLine + insertcnt;
	  while (insertcnt--) fgets(diffline, MAXBUF, difffd);	/* eat duplicate text of above */
	  while (/*insertcnt--*/AbsLine<nextDiffLine && (p=source_gets())!=NULL) {
		  if (first && *p!='.') { stagadd(BEGINDIFFA); first=0; }
		  source_line(p);
	  }
	  if (!first) { stagadd(ENDDIFFA); source_line(" "); }
  }
  /* finish up remainder (identical to both) */
  while ((p=source_gets())!=NULL) {
	  source_line(p);
  }

  source_flush();
  pop(ENDDOC); (*fn)(ENDDOC);
}



/*
 * STARTUP
 */

int
setFilterDefaults(char *outputformat) {
	static struct {
		void (*fn)(enum command);
		int fPara; int fQS; int fIQS; int fNOHY; int fChangeleft; int fURL; char *names;
		/* (could set parameters at BEGINDOC call...) */
	} format[] = {
		/*{default	0par 0qs	0iqs	0hy	0cl	0url	"name"},*/
		{ ASCII,		0,	0,	0,	0,	0,	0,	"ASCII" },
		{ TkMan,		0,	0,	0,	0,	0,	0,	"TkMan" },
		{ Tk,		0,	0,	0,	0,	0,	0,	"Tk:Tcl" },
		{ Sections,	0,	0,	0,	0,	0,	0,	"sections" },
		{ Roff,		0,	1,	1,	1,	-1,	0,	"roff:troff:nroff" },
		{ HTML,		1,	1,	1,	1,	1,	1,	"HTML:WWW:htm" },
		{ XML,		1,	1,	1,	1,	1,	1,	"XML:docbook:DocBook" },
		{ MIME,		1,	1,	1,	1,	1,	0,	"MIME:Emacs:enriched" },
		{ LaTeX,		1,	1,	1,	1,	1,	0,	"LaTeX:LaTeX209:209:TeX" },
		{ LaTeX2e,	1,	1,	1,	1,	1,	0,	"LaTeX2e:2e" },
		{ RTF,		1,	1,	1,	1,	1,	0,	"RTF" },
		{ pod,		0,	0,	1,	0,	-1,	0,	"pod:Perl" },

		{ PostScript,	0,	0,	0,	0,	0,	0,	"PostScript:ps" },
		{ FrameMaker,	0,	0,	0,	0,	0,	0,	"FrameMaker:Frame:Maker:MIF" },

		{ NULL,		0,	0,	0,	0,	0,	0,	NULL }
	};

	int i, found=0;
	for (i=0; format[i].fn!=NULL; i++) {
		if (strcoloncmp2(outputformat,'\0',format[i].names,0)) {
			fn = format[i].fn;
			fPara = format[i].fPara;
			fQS = format[i].fQS;
			fIQS = format[i].fIQS;
			fNOHY = format[i].fNOHY;
			fChangeleft = format[i].fChangeleft;
			fURL = format[i].fURL;
			found=1;
			break;
		}
	}

	return (found==0);
}


/* read in whole file.  caller responsible for freeing memory */
char *
filesuck(FILE *in) {
  const int inc=1024*100;	/* what's 100K these days? */
  int len=0,cnt;
  char *file = malloc(1);  /*NULL -- relloc on NULL not reducing to malloc on some machines? */

  do {
    file = realloc(file, len+inc+1);	/* what's the ANSI way to find the file size? */
    cnt = fread(&file[len], 1, inc, in);
    len+=cnt;
  } while (cnt==inc);
  file[len]='\0';

  return file;
}

int
main(int argc, char *argv[]) {
	int c;
	int i,j;
	char *p,*oldp;
	int fname=0;
	const int helpbreak=75;
	const int helpispace=4;
	int helplen=0;
	int desclen;
	char **argvch;	/* remapped argv */
	char *argvbuf;
	char *processing = "stdin";
	extern char *optarg;
	extern int optind;
/*	FILE *macros; -- interpret -man macros? */

	char strgetopt[80];
	/* options with an arg must have a '<' in the description */
	static struct { char letter; int arg; char *longnames; char *desc; } option[] = {
		{ 'f', 1, "filter", " <ASCII|roff|TkMan|Tk|Sections|HTML|XML|MIME|LaTeX|LaTeX2e|RTF|pod>" },
		{ 'S', 0, "source", "(ource of man page passed in)" },	/* autodetected */
		{ 'F', 0, "formatted:format", "(ormatted man page passed in)" },	/* autodetected */

		{ 'r', 1, "reference:manref:ref", " <man reference printf string>" },
		{ 'l', 1, "title", " <title printf string>" },
		{ 'V', 1, "volumes:vol", "(olume) <colon-separated list>" },
		{ 'U', 0, "url:urls", "(RLs as hyperlinks)" },

		/* following options apply to formatted pages only */
		{ 'b', 0, "subsections:sub", " (show subsections)" },
		{ 'k', 0, "keep:head:foot:header:footer", "(eep head/foot)" },
		{ 'n', 1, "name", "(ame of man page) <string>" },
		{ 's', 1, "section:sect", "(ection) <string>" },
		{ 'p', 0, "paragraph:para", "(aragraph mode toggle)" },
		{ 't', 1, "tabstop:tabstops", "(abstops spacing) <number>" },
		{ 'N', 0, "normalize:normal", "(ormalize spacing, changebars)" },
		{ 'y', 0, "zap:nohyphens", " (zap hyphens toggle)" },
		{ 'K', 0, "nobreak", " (declare that page has no breaks)" },	/* autodetected */
		{ 'd', 1, "diff", "(iff) <file> (diff of old page source to incorporate)" },
		{ 'M', 1, "message", "(essage) <text> (included verbatim at end of Name section)" },
		/*{ 'l', 0, "number lines", "... can number lines in a pipe" */
		/*{ 'T', 0, "tables", "(able agressive parsing ON)" },*/
/*		{ 'c', 0, "changeleft:changebar", "(hangebarstoleft toggle)" }, -- default is perfect */
		/*{ 'R', 0, "reflow", "(eflow text lines)" },*/
		{ 'R', 1, "rebus", "(ebus words for TkMan)" },
		{ 'C', 0, "TclTk", " (enable Tcl/Tk formatting)" },	/* autodetected */

		/*{ 'D', 0, "debug", "(ebugging mode)" }, -- dump unrecognized macros, e.g.*/
		{ 'o', 0, "noop", " (no op)" },
		{ 'O', 0, "noop", " <arg> (no op with arg)" },
		{ 'q', 0, "quiet", "(uiet--don't report warnings)" },
		{ 'h', 0, "help", "(elp)" },
		/*{ '?', 0, "help", " (help)" },	-- getopt returns '?' as error flag */
		{ 'v', 0, "version", "(ersion)" },
		{ '\0', 0, "", NULL }
	};

	/* calculate strgetopt from options list */
	for (i=0,p=strgetopt; option[i].letter!='\0'; i++) {
		*p++ = option[i].letter;
		/* check for duplicate option letters */
		assert(strchr(strgetopt,option[i].letter)==&p[-1]);
		if (option[i].arg) *p++=':';
	}
	*p='\0';

	/* spot check construction of strgetopt */
	assert(p<strgetopt+80);
	assert(strlen(strgetopt)>10);
	assert(strchr(strgetopt,'f')!=NULL);
	assert(strchr(strgetopt,'v')!=NULL);
	assert(strchr(strgetopt,':')!=NULL);

	/* count, sort exception strings */
	for (lcexceptionslen=0; (p=lcexceptions[lcexceptionslen])!=NULL; lcexceptionslen++) /*empty*/;
	qsort(lcexceptions, lcexceptionslen, sizeof(char*), lcexceptionscmp);

	/* map long option names to single letters for switching */
	/* (GNU probably has a reusable function to do this...) */
	/* deep six getopt in favor of integrated long names + letters? */
	argvch = malloc(argc * sizeof(char*));
	p = argvbuf = malloc(argc*3 * sizeof(char));	/* either -<char>'\0' or no space used */
	for (i=0; i<argc; i++) argvch[i]=argv[i];	/* need argvch[0] for getopt? */
	argv0 = mystrdup(argv[0]);
	for (i=1; i<argc; i++) {
		if (argv[i][0]=='-' && argv[i][1]=='-') {
			if (argv[i][2]=='\0') break;	/* end of options */
			for (j=0; option[j].letter!='\0'; j++) {
				if (strcoloncmp2(&argv[i][2],'\0',option[j].longnames,0)) {
					argvch[i] = p;
					*p++ = '-'; *p++ = option[j].letter; *p++ = '\0';
					if (option[j].arg) i++;	/* skip arguments of options */
					break;
				}
			}
			if (option[j].letter=='\0') fprintf(stderr, "%s: unknown option %s\n", argv[0], argv[i]);
		}
	}



	/* pass through options to set defaults for chosen format */
	setFilterDefaults("ASCII");		/* default to ASCII (used by TkMan's Glimpse indexing */

	/* initialize header/footer buffers (save room in binary) */
	for (i=0; i<CRUFTS; i++) { *cruft[i] = '\0'; }	/* automatically done, guaranteed? */
	/*for (i=0; i<MAXLINES; i++) { linetabcnt[i] = 0; } */

	while ((c=getopt(argc,argvch,strgetopt))!=-1) {

		switch (c) {
		   case 'k': fHeadfoot=1; break;
		   case 'b': fSubsections=1; break;
/*		   case 'c': fChangeleft=1; break; -- obsolete */
			/* case 'R': fReflow=1; break;*/
		   case 'n': strcpy(manName,optarg); fname=1; break;	/* name & section for when using stdin */
		   case 's': strcpy(manSect,optarg); break;
		   /*case 'D': docbookpath = optarg; break;*/
		   case 'V': vollist = optarg; break;
		   case 'l': manTitle = optarg; break;
		   case 'r': manRef = optarg;
			if (strlen(manRef)==0 || strcmp(manRef,"-")==0 || strcmp(manRef,"off")==0) fmanRef=0;
			break;
		   case 't': TabStops=atoi(optarg); break;
		   /*case 'T': fTable=1; break;	-- if preformatted doesn't work, if source automatic */
		   case 'p': fPara=!fPara; break;
		   case 'K': fFoot=1; break;
		   case 'y': fNOHY=1; break;
		   case 'N': fNORM=1; break;

		   case 'f': /* set format */
			if (setFilterDefaults(optarg)) {
				fprintf(stderr, "%s: unknown format: %s\n", argv0, optarg);
				exit(1);
			}
			break;
		   case 'F': fSource=0; break;
		   case 'S': fSource=1; break;

		   case 'd':
			difffd = fopen(optarg, "r");
			if (difffd==NULL) { fprintf(stderr, "%s: can't open %s\n", argv0, optarg); exit(1); }
/* read in a line at a time
			diff = filesuck(fd);
			fclose(fd);
*/
			fDiff=1;
			break;

		   case 'M': message = optarg; break;

		   case 'C': fTclTk=1; break;
		   case 'R':
			p = malloc(strlen(optarg)+1);
			strcpy(p, optarg);	/* string may not be in writable address space */
			oldp = "";
			for (; *p; oldp=p, p++) {
				if (*oldp=='\0') rebuspat[rebuspatcnt++] = p;
				if (*p=='|') *p='\0';
			}
			for (i=0; i<rebuspatcnt; i++) rebuspatlen[i] = strlen(rebuspat[i]);	/* for strnlen() */
			break;

		   case 'q': fQuiet=1; break;
		   case 'o': /*no op*/ break;
		   case 'O': /* no op with arg */ break;
		   case 'h':
			printf("rman"); helplen=strlen("rman");

			/* linebreak options */
			assert(helplen>0);
			for (i=0; option[i].letter!='\0'; i++) {
			  desclen = strlen(option[i].desc);
			  if (helplen+desclen+5 > helpbreak) { printf("\n%*s",helpispace,""); helplen=helpispace; }
			  printf(" [-%c%s]", option[i].letter, option[i].desc);
			  helplen += desclen+5;
			}
			if (helplen>helpispace) printf("\n");
			printf("%*s [<filename>]\n",helpispace,"");
			exit(0);

		   case 'v': /*case '?':*/
			printf("PolyglotMan v" POLYGLOTMANVERSION " of $Date: 2003/07/26 19:00:48 $\n");
			exit(0);

		   default:
			fprintf(stderr, "%s: unidentified option -%c (-h for help)\n",argvch[0],c);
			exit(2);
		}
	}



	/* read from given file name(s) */
	if (optind<argc) {
		processing = argvch[optind];

		if (!fname) {	/* if no name given, create from file name */
			/* take name from tail of path */
			if ((p=strrchr(argvch[optind],'/'))!=NULL) p++; else p=argvch[optind];
			strcpy(manName,p);

			/* search backward from end for final dot. split there */
			if ((p=strrchr(manName,'.'))!=NULL) {
				strcpy(manSect,p+1);
				*p='\0';
			}
		}

		strcpy(plain,argvch[optind]);

		if (freopen(argvch[optind], "r", stdin)==NULL) {
			fprintf(stderr, "%s: can't open %s\n", argvch[0], argvch[optind]);
			exit(1);
		}
	}

	/* need to read macros, ok if fail; from /usr/lib/tmac/an => needs to be set in Makefile, maybe a searchpath */
	/*
	if ((macros=fopen("/usr/lib/tmac/an", "r"))!=NULL) {
		in = File = filesuck(macros);
		lookahead = File[0];
		source_filter();
		free(File);
	}
	*/

	/* suck in whole file and just operate on pointers */
	in = File = filesuck(stdin);


	/* minimal check for roff source: first character dot command or apostrophe comment */
	/* MUST initialize lookahead here, BEFORE first call to la_gets */
	if (fSource==-1) {
		lookahead = File[0];
		fSource = (lookahead=='.' || lookahead=='\'' || /*dumb HP*/lookahead=='/'
		    /* HP needs this too but causes problems || isalpha(lookahead)--use --source flag*/);
	}

	if (fDiff && (!fSource || fn!=HTML)) {
		fprintf(stderr, "diff incorporation supported for man page source, generating HTML\n");
		exit(1);
	}

	if (fSource) source_filter(); else preformatted_filter();
	if (fDiff) fclose(difffd);
	/*free(File);	-- let system clean up, perhaps more efficiently */

	return 0;
}
