/* Copyright 1994 NEC Corporation, Tokyo, Japan.
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
	static char rcsid[]="$Id: dd.c 10525 2004-12-23 21:23:50Z korli $";
#endif
/*LINTLIBRARY*/

#include	"RKintern.h"

#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define cx_gwt		cx_extdata.ptr
#define	STRCMP(d, s)	strcmp((char *)(d), (char *)(s))
	
#define RK_READABLE 'r'
#define RK_WRITABLE 'w'

static void _RkFreeDM(struct DM *dm);
static struct DM *_RkAllocDM(struct DF *df, unsigned char *dicname, unsigned char *nickname, int iclass);
static struct DF *_RkCreateDF(struct DD *dd, unsigned char *lnk, int type);
static void _RkFreeDF(struct DF *df);
static struct DF *_RkAllocDF(struct DD *dd, unsigned char *lnk, int type);
static int _RkParseDicsDir(char *line, char *lnk, char *member, char *nickname, int *dftype, int *dmclass, int *r_return, int *w_return);
static void _RkFreeDD(struct DD *dd);

/*
* DM
*/
inline
struct DM	*
_RkCreateDM(struct DF *df, unsigned char *dicname, unsigned char *nickname, int iclass)
{
	struct DM	*dm;
	
	dm = (struct DM *)calloc(1, sizeof(struct DM));
	if (dm) {
		dm->dm_next = dm->dm_prev = dm;
		dm->dm_file = df;
		dm->dm_dicname  = strdup((char *)dicname);
		if (dm->dm_dicname) {
			dm->dm_nickname = strdup((char *)nickname);
			if (dm->dm_nickname) {
				dm->dm_class = iclass;
				dm->dm_flags = dm->dm_packbyte = dm->dm_rcount = 0;
				dm->dm_gram = (struct RkGram *)0;
				dm->dm_extdata.ptr = (pointer)0;
				dm->dm_rut = (struct RUT *)0;
				dm->dm_nv = (struct NV *)0;
				return dm;
			}
			free(dm->dm_dicname);
		}
		free(dm);
	}
	return 0;
}

static
void
_RkFreeDM(struct DM *dm)
{
if (dm) {
	dm->dm_next->dm_prev = dm->dm_prev;
	dm->dm_prev->dm_next = dm->dm_next;
	if (dm->dm_dicname)
		free(dm->dm_dicname);
	if (dm->dm_nickname)
		free(dm->dm_nickname);
	free(dm);
};
}

inline
struct DM *
_RkAllocDM(struct DF *df, unsigned char *dicname, unsigned char *nickname, int iclass)
{
	struct DM	*m, *mh = &df->df_members;
	
	for (m = mh->dm_next; m != mh; m = m->dm_next) {
	if (!STRCMP(m->dm_dicname,  dicname)) {
		return m;
	}
	}
	m = _RkCreateDM(df, dicname, nickname, iclass);
	if (m) {
		m->dm_next = mh;
		m->dm_prev = mh->dm_prev;
		mh->dm_prev = m;
		m->dm_prev->dm_next = m;
	}
	return(m);
}

/*
* DF
*/
inline
struct DF *
_RkCreateDF(struct DD *dd, unsigned char *lnk, int type)
{
	struct DF	*df;
	
	df = (struct DF *)calloc(1, sizeof(struct DF));
	if (df) {
		struct DM	*dm = &df->df_members;
		
		df->df_next = df->df_prev = df;
		df->df_direct = dd;
		dm->dm_next = dm->dm_prev = dm;
		
		if (!(df->df_link = strdup((char *)lnk))) {
			free(df);
			return(0);
		};
		df->df_rcount = 0;
		df->df_type  = type;
		df->df_extdata.ptr = (pointer)0;
	};
	return(df);
}

static
void
_RkFreeDF(struct DF *df)
{
	struct DM	*m, *n;
	
	if (df) {
		struct DM	*mh = &df->df_members;
		
		/* remove all members in this file */
		for (m = mh->dm_next; m != mh; m = n) {
			n = m->dm_next;
			_RkFreeDM(m);
		};
		/* unlink from directory list */
		df->df_next->df_prev = df->df_prev;
		df->df_prev->df_next = df->df_next;
		if (df->df_link)
			free(df->df_link);
		free(df);
	};
}

static
struct DF	*
_RkAllocDF(struct DD *dd, unsigned char *lnk, int type)
{
	struct DF	*f;
	struct DF	*fh = &dd->dd_files;
	
	for (f = fh->df_next; f != fh; f = f->df_next) {
	if (!STRCMP(f->df_link,  lnk)) {
		return(f);
	}
	}
	f = _RkCreateDF(dd, lnk, type);
	if (f) {
		f->df_next = fh;
		f->df_prev = fh->df_prev;
		fh->df_prev = f;
		f->df_prev->df_next = f;
	};
	return(f);
}

int
_RkRealizeDF(struct DF *df)
{
	struct DD	*dd = df->df_direct;
	char		*pathname;
#ifdef WIN
	int oldmask;
#else
	unsigned long oldmask;
#endif
	int t;
	
	_RkRealizeDD(dd);
	/* create path filename */
	pathname = _RkCreatePath(df->df_direct, df->df_link);
	if (pathname) {
		oldmask = umask(2);
		/* create a file */
#ifdef WIN
{
	HANDLE fd;
	fd = CreateFile(pathname, GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fd == INVALID_HANDLE_VALUE) {
		t = -1;
		}else{
		CloseHandle(fd);
		t = 0;
	}
}
#else
		t = close(creat(pathname, CREAT_MODE));
#endif
		free(pathname);
		(void)umask(oldmask);
		if (t >= 0) {
			return 0;
		}
	}
	return -1;
}

static int
_RkParseDicsDir(char *line, char *lnk, char *member, char *nickname, int *dftype, int *dmclass, int *r_return, int *w_return) /* ¥¢¥¯¥»¥¹¸¢¤òÊÖ¤¹½ê */
{
	char	*s, *d, *t, par, ch;
	int	count;
	
	*dftype  = -1;
	*dmclass = -1;
	*r_return = *w_return = 0; /* ¸¢ÍøÌµ¤· */
	if (!isalpha(line[0]))
		return -1;
	
	/* parse line %s.s(%s.%s)   -%s--%c%c- */
	
	for (s = line; *s && isspace(*s);) {
		s++;
	}
	if (!*s)
		return -1;
	
	/* link name */
	for (d = lnk, count = 0; *s && *s != '('; count++) {
	if (count < RK_LINK_BMAX) {
		*d++ = *s++;
		} else {
		return -1;
	}
	}
	if (!count)
		return -1;
		if (count++ < RK_LINK_BMAX) {
			*d = 0;
		}
	if (!*s++)
		return -1;
	if (count > RK_LINK_BMAX)
		return -1;
	if (!(t = (char *)strrchr(lnk, '.')))
		return -1;
	if (!STRCMP(t, ".d") || !STRCMP(t, ".cbd"))
		*dftype = DF_PERMDIC;
	else if (!STRCMP(t, ".t") || !STRCMP(t, ".ctd"))
		*dftype = DF_TEMPDIC;
	else if (!STRCMP(t, ".fq") || !STRCMP(t, ".cld"))
		*dftype = DF_FREQDIC;
	else if (!STRCMP(t, ".ruc"))
		*dftype = DF_RUCDIC;
	else
		return -1;
	/* member name */
	for (d = member, count = 0; *s && *s != ')'; count++) {
	if (count < RK_MEMBER_BMAX) {
		*d++ = *s++;
		} else {
		return -1;
	}
	}
	if (!count)
		return -1;
	if (count++ < RK_MEMBER_BMAX)
		*d = 0;
	if (!*s++)
		return -1;
	if (count > RK_MEMBER_BMAX)
		return -1;
	if (!(t = (char *)strrchr(member, '.')))
		return -1;
	if (!STRCMP(t, ".mwd"))
		*dmclass = ND_MWD;
	else if (!STRCMP(t, ".swd"))
		*dmclass = ND_SWD;
	else if (!STRCMP(t, ".pre"))
		*dmclass = ND_PRE;
	else if (!STRCMP(t, ".suc"))
		*dmclass = ND_SUC;
	else
		return -1;
	/* nickname */
	for (; *s && isspace(*s);) {
		s++;
	}
	if (!(par = *s++))
		return -1;
	
	for (d = nickname, count = 0; *s && *s != par; count++) {
	if (count < RK_NICK_BMAX) {
		*d++ = *s++;
		} else {
		return -1;
	}
	}
	if (!count)
		return -1;
	if (count++ < RK_NICK_BMAX)
		*d = 0;
	if (!*s++)
		return -1;
	if (count > RK_NICK_BMAX)
		return -1;
	
	*w_return = 1; /* ²¼°Ì¸ß´¹¤Î¤¿¤á¤³¤³¤Þ¤Ç¤·¤«¤Ê¤¤¾ì¹ç¤Ï writable ¤Ë¤¹¤ë */
	
	while ((ch = *s) && ch != par) {
		s++;/* 1 ¥Õ¥£¡¼¥ë¥ÉÆÉ¤ßÈô¤Ð¤· */
	}
	
	if (*s++ /* == par ¤Ë¤Ê¤Ã¤Æ¤¤¤ë¤ï¤± */) {
		/* ¥¢¥¯¥»¥¹¸¢´ØÏ¢ */
		*w_return = 0;
	while ((ch = *s) && ch != par) {
	if (ch == RK_READABLE) {
		*r_return = 1;
	}
else if (ch == RK_WRITABLE) {
	*w_return = 1;
	} else {
	return -1;
}
		s++;
	}
	}
	
	return 0;
}

/*
* DD - dictonary directory record
*/
inline
struct DD	*
_RkCreateDD(unsigned char *path, unsigned char *name)
{
	struct DD	*dd;
	
	dd = (struct DD *)calloc(1, sizeof(struct DD));
	if (dd) {
		dd->dd_next = dd->dd_prev = dd;
		dd->dd_path = strdup((char *)path);
		if (dd->dd_path) {
			dd->dd_name = strdup((char *)name);
			if (dd->dd_name) {
				dd->dd_rcount = 0;
				dd->dd_files.df_next = dd->dd_files.df_prev = &dd->dd_files;
				dd->dd_flags = 0;
				dd->dd_text.ddt_next = dd->dd_text.ddt_prev = &dd->dd_text;
				return dd;
			}
			free(dd->dd_path);
		}
		free(dd);
	}
	return (struct DD *)0;
}

static
void
_RkFreeDD(struct DD *dd)
{
	struct DF	*f, *g;
	struct DF	*fh = &dd->dd_files;
	struct DDT	*ddLines;
	struct DDT	*p, *q;
	
	if (dd) {
	for (f = fh->df_next; f != fh; f = g) {
		g = f->df_next;
		_RkFreeDF(f);
	};
		dd->dd_next->dd_prev = dd->dd_prev;
		dd->dd_prev->dd_next = dd->dd_next;
		if (dd->dd_path)
			free(dd->dd_path);
		if (dd->dd_name)
			free(dd->dd_name);
		
		ddLines = &dd->dd_text;
		for (p = ddLines->ddt_next; p != ddLines; p = q) {
			q = p->ddt_next;
			p->ddt_next->ddt_prev = p->ddt_prev;
			p->ddt_prev->ddt_next = p->ddt_next;
			if (p->ddt_spec)
				free(p->ddt_spec);
			free(p);
		};
		free(dd);
	};
}

inline struct DD	*
_RkLookupDD(struct DD *dd, unsigned char *name)
{
	struct DD		*d;
	
	for (d = dd->dd_next; d != dd; d = d->dd_next)
		if (!STRCMP(d->dd_name,  name))
			return d;
	return (struct DD *)0;
}

/* _RkReadDD
*	read a DD directory using dics.dir file.
*/
inline struct DD	*
_RkReadDD(char *name)
{
	char		*dics_dir = "/dics.dir";
	struct DD *dd = (struct DD *)0;
	struct DF		*df;
	struct DM		*dm;
	struct DDT		*ddLines;
	struct DDT		*ddt;
	struct RkParam	*sx = RkGetSystem();
	int r, w;
	int fdes;
	FILE		*fp;
	
#ifndef USE_MALLOC_FOR_BIG_ARRAY
	char		direct[RK_PATH_BMAX];
	char		file[RK_PATH_BMAX];
	char		path[RK_PATH_BMAX];
	unsigned char	line[RK_LINE_BMAX];
	unsigned char	lnk[RK_LINK_BMAX+1];
	unsigned char	member[RK_MEMBER_BMAX+1];
	unsigned char	nickname[RK_NICK_BMAX+1];
#else
	char *direct, *file, *path;
	unsigned char *line, *lnk, *member, *nickname;
	direct = (char *)malloc(RK_PATH_BMAX);
	file = (char *)malloc(RK_PATH_BMAX);
	path = (char *)malloc(RK_PATH_BMAX);
	line = (unsigned char *)malloc(RK_LINE_BMAX);
	lnk = (unsigned char *)malloc(RK_LINK_BMAX + 1);
	member = (unsigned char *)malloc(RK_MEMBER_BMAX + 1);
	nickname = (unsigned char *)malloc(RK_NICK_BMAX + 1);
	if (!direct || !file || !path || !line || !lnk || !member || !nickname) {
		if (direct) free(direct);
		if (file) free(file);
		if (path) free(path);
		if (line) free(line);
		if (lnk) free(lnk);
		if (member) free(member);
		if (nickname) free(nickname);
		return dd;
	}
#endif
	
	/* create dd even if there is no directory or dics.dir file */
	sprintf(path, "%s/%s", sx->ddhome, name);
	dd = _RkCreateDD((unsigned char *)path, (unsigned char *)name);
	if (!dd) {
		goto return_dd;
	}
	/* jisho table ga aruka ? */
	if (strlen(path) + strlen(dics_dir) + 1 >= RK_PATH_BMAX) {
		dd = (struct DD *)0;
		goto return_dd;
	}
	sprintf(direct, "%s%s", path, dics_dir);
	
	/* check for accessing right */
	if ((fdes = open(direct, 0)) < 0) { /* no file? */
		dd->dd_flags |= DD_WRITEOK;
	}
else {
	struct stat buf;
	
	if (fstat(fdes, &buf) == 0) {
	if (buf.st_mode & 004) { /* if readable for others */
		dd->dd_flags |= DD_READOK;
	}
	}
	close(fdes);
	if (!close(open(direct, 2))) {
		dd->dd_flags |= DD_WRITEOK;
	}
}
	
	fp = fopen(direct, "r");
if (!fp) {
	goto return_dd;
}
	ddLines = &dd->dd_text;
	/* read dics_dir lines */
	while (fgets((char *)line, RK_LINE_BMAX, fp))
{
	int		dftype, dmclass;
	
	ddt = (struct DDT *)malloc(sizeof(struct DDT));
	if (!ddt)
		continue;
	
	ddt->ddt_spec = (char *)malloc(strlen((char *)line) + 2); /* 2 for \n\0 */
	if (!ddt->ddt_spec)
	{
		free(ddt);
		continue;
	};
{
	int len = strlen((char *)line);
	if (line[len - 1] == '\n') {
		line[len - 1] = '\0';
	}
}
	strcpy(ddt->ddt_spec, (char *)line);	
	ddt->ddt_next = ddLines;
	ddt->ddt_prev = ddLines->ddt_prev;
	ddt->ddt_status = 1;
	ddLines->ddt_prev = ddt;
	ddt->ddt_prev->ddt_next = ddt;
	
	if (_RkParseDicsDir(ddt->ddt_spec, (char *)lnk, (char *)member,
	(char *)nickname, &dftype, &dmclass, &r, &w) < 0) {
		continue;
	}
	
	if (strlen((char *)path) + strlen((char *)lnk) + 1 >= RK_PATH_BMAX)
		continue;
	sprintf(file, "%s/%s", path, (char *)lnk);
	if (close(open(file, 0)) < 0)
		continue;
	df = _RkAllocDF(dd, lnk, dftype);
	if (df) {
		dm = _RkAllocDM(df, member, nickname, dmclass);
		if (dm) {
			dm->dm_line = ddt;
			if (r) {
				dm->dm_flags |= DM_READOK;
			}
			if (w) {
				dm->dm_flags |= DM_WRITEOK;
			}
			ddt->ddt_status = 0;
		};
	};
};
	(void)fclose(fp);
	
return_dd:
#ifdef USE_MALLOC_FOR_BIG_ARRAY
	free(direct);
	free(file);
	free(path);
	free(line);
	free(lnk);
	free(member);
	free(nickname);
#endif
	return dd;
}

inline
struct DD	*
_RkOpenDD(char *name)
{
	struct RkParam	*sx = RkGetSystem();
	struct DD		*dd;
	struct DD		*knownDD = &sx->dd;
	
	dd = _RkLookupDD(knownDD, (unsigned char *)name);
	if (dd)
		return dd;
	dd = _RkReadDD(name);
	/* link to the known list */
	if (dd) {
		dd->dd_next = knownDD;
		dd->dd_prev = knownDD->dd_prev;
		knownDD->dd_prev = dd;
		dd->dd_prev->dd_next = dd;
	};
	return dd;
}

char *
_RkCreatePath(struct DD *dd, char *name)
{
	unsigned 	sz;
	char        *ddname;
	
	if (!dd || !dd->dd_path || !name)
		return (char *)0;
	sz = strlen(dd->dd_path) + strlen(name) + 2;
	ddname = (char *)malloc(sz);
	if (ddname)  {
		sprintf(ddname, "%s/%s", dd->dd_path, name);
	};
	return ddname;
}

char *
_RkCreateUniquePath(struct DD *dd, char *proto)
{
	static char	newLinkName[RK_LINK_BMAX];
	unsigned 	i;
	
	/* now checking ... */
	if (!dd || !dd->dd_path || !proto)
		return (char *)0;
	/* create directory */
	if (_RkRealizeDD(dd) < 0)
		return (char *)0;
	/* try at most 100 times */
	for (i = 1; i < 100; i++) {
		int		count;
		struct DF		*f;
		struct DF		*fh = &dd->dd_files;
		unsigned long       oldmask;
		char		*filename;
		
		count = 0;
		sprintf(newLinkName, proto, i);
		for (f = fh->df_next; f != fh; f = f->df_next)
			if (!STRCMP(f->df_link,  newLinkName))
				count++;
		if (count)
			continue;
		filename = _RkCreatePath(dd, newLinkName);
		if (filename) {
			oldmask = umask(2);
			
			if (close(creat(filename, CREAT_MODE)) < 0)
				count++;
			free(filename);
			(void)umask(oldmask);
			if (!count)
				return newLinkName;
		};
	};
	return (char *)0;
}

char	*
_RkMakePath(struct DF *df)
{
	if (df)
		return _RkCreatePath(df->df_direct, df->df_link);
	else
		return (char *)0;
}

int
_RkRealizeDD(struct DD	*dd)
{
	struct DDT		*ddLines;
	struct DDT		*ddt;
	int			n;
	int ret = -1;
	int tmpres;
	int			fdes;
	long		tloc;
	
	char *whattime, *header, *dicsdir, *backup;
	whattime = (char *)malloc(RK_LINE_BMAX);
	header = (char *)malloc(RK_LINE_BMAX);
	dicsdir = (char *)malloc(RK_PATH_BMAX);
	backup = (char *)malloc(RK_PATH_BMAX);
	if (!whattime || !header || !dicsdir || !backup) {
		if (whattime) free(whattime);
		if (header) free(header);
		if (dicsdir) free(dicsdir);
		if (backup) free(backup);
		return ret;
	}
	/* create directory if needed */
	if (close(open(dd->dd_path, 0, 0664)) < 0) {
	if (mkdir(dd->dd_path, MKDIR_MODE) < 0) {
		goto return_ret;
	}
		/* change owner
		if (pw)
			chown(dd->dd_path, getuid(), pw->pw_gid);
		*/
	}
	
	/* dics.dir */
	sprintf(dicsdir, "%s/dics.dir", dd->dd_path);
	backup[0] = 0;
	tmpres = close(open(dicsdir, 0));
	if (tmpres >= 0) {
		sprintf(backup, "%s/#dics.dir", dd->dd_path);
		if (rename(dicsdir, backup)) {
			goto return_ret;
		}
	};
	/* create dics.dir */
	
	if ((fdes = creat(dicsdir, CREAT_MODE)) < 0)
	{
	if (backup[0]) {
		rename(backup, dicsdir);
	}
		goto return_ret;
	};
	/* header */
	tloc = time(0);
	strcpy(whattime, ctime(&tloc));
	whattime[strlen(whattime)-1] = 0;
	n = sprintf(header, "#CANNA dics.dir [%s] %s\n", whattime, dd->dd_name);
	//  n = strlen(header);
	tmpres = write(fdes, header, n);
	if (tmpres != n) {
	if (backup[0]) {
		rename(backup, dicsdir);
	}
		else
			unlink(dicsdir);
		close(fdes);
		goto return_ret;
	};
	/* fill up bodies */
	ddLines = &dd->dd_text;
	for (ddt = ddLines->ddt_next; ddt != ddLines; ddt = ddt->ddt_next)
		if (strncmp(ddt->ddt_spec, "#CANNA ",  7)) {
			n = strlen(ddt->ddt_spec);
			ddt->ddt_spec[n] = '\n';
			tmpres = write(fdes, ddt->ddt_spec, n + 1);
			if (tmpres > 0) {
				tmpres--; /* for \n */
			}
			
			
			ddt->ddt_spec[n] = '\0';
			
			if (tmpres != n) {
			if (backup[0]) {
				rename(backup, dicsdir);
			}
				else
					unlink(dicsdir);
				close(fdes);
				goto return_ret;
			};
		};
	close(fdes);
	/* change owner
	if (pw)
		chown(dicsdir, getuid(), pw->pw_gid);
	*/
	ret = 0;
	
return_ret:
	free(whattime);
	free(header);
	free(dicsdir);
	free(backup);
	return ret;
}

/*
* DDP
*/
int
_RkIsInDDP(struct DD	**ddp, struct DD	*dd)
{
	while (*ddp)
		if (*ddp++ == dd)
			return 1;
	return 0;
}

static
int
_RkCountDDP(
	struct DD	**ddp)
{
	int	count = 0;
	
	if (ddp)
		while (ddp[count])  count++;
	return count;
}

struct DD	**
_RkCopyDDP(
	struct DD	**ddp)
{
	struct DD	**newc = (struct DD **)0;
	int		i;
	struct DD	*dd;
	
	if (ddp) {
		int	count = _RkCountDDP(ddp);
		
		newc = (struct DD **)calloc(count + 1, (unsigned)sizeof(struct DD *));
		if (newc)
			for (i = 0; (dd = newc[i] = ddp[i]) != (struct DD *)0 ; i++)
				dd->dd_rcount++;
	};
	return newc;
}

inline
struct DD	**
_RkAppendDDP(
	struct DD	**ddp,
	struct DD	*dd)
{
	struct DD	**newc;
	int		i;
	int		count = _RkCountDDP(ddp);
	
	newc = (struct DD **)calloc(count + 2, (unsigned)sizeof(struct DD *));
	if (newc) {
	if (ddp) {
		for (i = 0; i < count; i++) newc[i] = ddp[i];
		free(ddp);
	};
		newc[count++] = dd;
		newc[count] = (struct DD *)0;
		dd->dd_rcount++;
	} else
	newc = ddp;
	return newc;
}

struct DD	**
_RkCreateDDP(
	char		*ddpath)
{
	char		*d, *s;
	struct DD 	*dd;
	struct DD	**ddp  = (struct DD **)0;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
	char dir[RK_PATH_BMAX + 1];
#else
	char *dir = (char *)malloc(RK_PATH_BMAX + 1);
	if (!dir) {
		return ddp;
	}
#endif
	
	for (s = ddpath; *s; ) {
		int		count;
		
		for (;*s && isspace(*s);) {
			s++;
		}
		if (!*s)
			break;
		for (d = dir, count = 0; *s; count++)
			if (*s == ':') {
				s++;
				break;
				} else {
				if (count < RK_PATH_BMAX)
					*d++ = *s;
				s++;
			};
		*d = 0;
		dd = _RkOpenDD(dir);
		if (dd)
			ddp = _RkAppendDDP(ddp, dd);
	};
#ifdef USE_MALLOC_FOR_BIG_ARRAY
	free(dir);
#endif
	return ddp;
}

void
_RkFreeDDP(
	struct DD	**ddp)
{
	struct DD	*dd;
	int		i;
	
	if (ddp) {
		for (i = 0; (dd = ddp[i]) != (struct DD *)0 ; i++)
			if (--dd->dd_rcount == 0) {
				_RkFreeDD(dd);
			};
		free(ddp);
	};
}

/* _RkSearchDDP/Q
*	search dictionary file by nickname
*/
struct DM	*
_RkSearchDDP(
	struct DD	**ddp,
	char	*name)
{
	struct DD	*dd;
	struct DF	*f, *fh;
	struct DM	*m, *mh;
	int		i;
	
	if (ddp) {
	for (i = 0; (dd = ddp[i]) != (struct DD *)0 ; i++) {
		fh = &dd->dd_files;
		for (f = fh->df_next; f && (f != fh); f = f->df_next) {
		if (f->df_type == DF_FREQDIC) {
			/* Æ±¤¸¥Ç¥£¥ì¥¯¥È¥ê³¬ÁØ¤Ç¤Ï .fq ¤«¤éÃµ¤¹ */
			mh = &f->df_members;
			for (m = mh->dm_next; m != mh; m = m->dm_next) {
			if (!STRCMP(m->dm_nickname, name)) {
				return m;
			}
			}
		}
		}
		fh = &dd->dd_files;
		for (f = fh->df_next; f && (f != fh); f = f->df_next) {
		if (f->df_type != DF_RUCDIC && f->df_type != DF_FREQDIC) {
			mh = &f->df_members;
			for (m = mh->dm_next ; m != mh; m = m->dm_next) {
			if (!STRCMP(m->dm_nickname, name)) {
				return m;
			}
			}
		}
		}
	}
	}
	return (struct DM *)0;
}

/* _RkSearchDDQ
¤¢¤ë¥¿¥¤¥×¤Î¼­½ñ¤À¤±Ãµ¤·¤ÆÊÖ¤¹
*/

struct DM	*
_RkSearchDDQ(
	struct DD	**ddp,
	char	*name,
	int	type)
{
	struct DD	*dd;
	struct DF	*f, *fh;
	struct DM	*m, *mh;
	int		i;
	
	if (ddp) {
	for (i = 0; (dd = ddp[i]) != (struct DD *)0 ; i++) {
		fh = &dd->dd_files;
		for (f = fh->df_next; f && (f != fh); f = f->df_next)
			if (f->df_type == (unsigned)type) {
				mh = &f->df_members;
				for (m = mh->dm_next; m != mh; m = m->dm_next) {
					if (!STRCMP(m->dm_nickname, name))
						return(m);
				};
			};
	};
	};
	return((struct DM *)0);
}

/*
_RkSearchUDDP()
	ºÇ½é¤Ë¸«ÉÕ¤«¤ë¤Î¤¬¥·¥¹¥Æ¥à¼­½ñ¤Ë¤¢¤ë¤ä¤Ä¤«¤É¤¦¤«¤òÈ½ÃÇ¤·¤Ê¤¬¤éÊÖ¤¹
*/

struct DM	*
_RkSearchUDDP(
	struct DD		**ddp,
	unsigned char	*name)
{
	struct DM	*dm = _RkSearchDDP(ddp, (char *)name);
	
	if (dm && STRCMP(dm->dm_file->df_direct->dd_name, SYSTEM_DDHOME_NAME)) {
		return dm;
	}
	return((struct DM *)0);
}

/* ¼­½ñ¥á¥ó¥ÐÌ¾¤Ç¼­½ñ¤òÃµ¤¹

³Ø½¬¥Õ¥¡¥¤¥ë¤Ï½ü³°¤·¤ÆÃµ¤¹
*/

struct DM	*
_RkSearchDDMEM(
	struct DD	**ddp,
	char	*name)
{
	struct DD	*dd;
	struct DF	*f, *fh;
	struct DM	*m, *mh;
	int		i;
	
	if (ddp) {
	for (i = 0; (dd = ddp[i]) != (struct DD *)0 ; i++) {
		fh = &dd->dd_files;
		for (f = fh->df_next; f && (f != fh); f = f->df_next) {
		if (f->df_type != DF_FREQDIC && f->df_type != DF_RUCDIC) {
			mh = &f->df_members;
			for (m = mh->dm_next; m != mh; m = m->dm_next) {
			if (!STRCMP(m->dm_dicname, name)) {
				return m;
			}
			}
		}
		}
	}
	}
	return (struct DM *)0;
}

/*
_RkSearchDicWithFreq -- ¼­½ñ(³Ø½¬¼­½ñ¤ò´Þ¤à)¤òÃµ¤·ÊÖ¤¹¡£

ddpath ¤ÎÀèÆ¬¤«¤é½ç¤Ë³Ø½¬¥Õ¥¡¥¤¥ë¤¢¤ë¤¤¤Ï¼­½ñ¤ò¤µ¤¬¤·¡¢¤ß¤Ä¤«¤Ã¤¿¤Î
¤òÊÖ¤¹¡£³Ø½¬¥Õ¥¡¥¤¥ë¤¬¤ß¤Ä¤«¤Ã¤¿¾ì¹ç¤Ë¤Ï³Ø½¬¥Õ¥¡¥¤¥ë¤Î¸µ¼­½ñ¤òÃµ¤·¤Æ
¤½¤ì¤ò dm ¤ËÊÖ¤·¡¢³Ø½¬¥Õ¥¡¥¤¥ë¼«¿È¤Ï qmp ¤ÎÀè¤Ë³ÊÇ¼¤·¤ÆÊÖ¤¹¡£

*/

struct DM *
_RkSearchDicWithFreq(
	struct DD **ddpath,
	char *name,
	struct DM **qmp)
{
	struct DD *udd[2];
	struct DM *dm, *qm;
	
	udd[1] = (struct DD *)0;
	
	*qmp = (struct DM *)0;
	while (*ddpath) {
		udd[0] = *ddpath;
		qm = _RkSearchDDQ(udd, name, DF_FREQDIC);
		if (qm) {
			*qmp = qm;
			return _RkSearchDDMEM(ddpath, qm->dm_dicname);
		}
		dm = _RkSearchDDP(udd, name);
		if (dm) {
			return dm;
		}
		ddpath++;
	}
	return (struct DM *)0;
}

/* DMcheck

DMcreate ¤Ç name ¤òÂè£³°ú¿ô¤ËÉÕ¤±¤Æ¤¤¤¿»þ¤Î¥Á¥§¥Ã¥¯¤òÆÈÎ©¤µ¤»¤¿¡£

return value;
1: OK
0: bad
*/

int
DMcheck(char *spec, char *name)
{
	int dftype, dmclass;
	int r, w, ret;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
	char lnk[RK_LINK_BMAX+1];
	char member[RK_MEMBER_BMAX+1];
	char nickname[RK_NICK_BMAX+1];
#else
	char *lnk, *member, *nickname;
	lnk = (char *)malloc(RK_LINK_BMAX + 1);
	member = (char *)malloc(RK_MEMBER_BMAX + 1);
	nickname = (char *)malloc(RK_NICK_BMAX + 1);
	if (!lnk || !member || !nickname) {
		if (lnk) free(lnk);
		if (member) free(member);
		if (nickname) free(nickname);
		return 0;
	}
#endif
	
	if (_RkParseDicsDir(spec, lnk, member, nickname, &dftype, &dmclass,
		&r, &w) < 0 ||
	STRCMP(nickname, name)) {
		ret = 0;
	}
else {
	ret = 1;
}
#ifdef USE_MALLOC_FOR_BIG_ARRAY
	free(lnk);
	free(member);
	free(nickname);
#endif
	return ret;
}

/* DMcreate
*	create a newc member under dd
*	DMcreate does not create an actual dictionary file.
*/
struct DM	*
DMcreate(
	struct DD		*dd,
	char		*spec)
{
	int		dftype, dmclass;
	struct DF	*df;
	struct DM	*dm = (struct DM *)0;
	struct DDT	*ddt;
	struct DDT	*ddLines = &dd->dd_text;
	int r, w;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
	char		lnk[RK_LINK_BMAX+1];
	char		member[RK_MEMBER_BMAX+1];
	char		nickname[RK_NICK_BMAX+1];
#else
	char *lnk, *member, *nickname;
	lnk = (char *)malloc(RK_LINK_BMAX + 1);
	member = (char *)malloc(RK_MEMBER_BMAX + 1);
	nickname = (char *)malloc(RK_NICK_BMAX + 1);
	if (!lnk || !member || !nickname) {
		if (lnk) free(lnk);
		if (member) free(member);
		if (nickname) free(nickname);
		return dm;
	}
#endif
	
	if (_RkParseDicsDir(spec, lnk, member, nickname, &dftype, &dmclass,
	&r, &w) >= 0) {
		int len = strlen(spec);
		if (spec[len - 1] == '\n') {
			spec[len - 1] = '\0';
		}
		
		ddt = (struct DDT *)malloc(sizeof(struct DDT));
		if (ddt) {
			ddt->ddt_spec = (char *)malloc(strlen(spec) + 3); /* 3 for \r\n\0 */
			if (ddt->ddt_spec) {
				strcpy(ddt->ddt_spec, spec);
				df = _RkAllocDF(dd, (unsigned char *)lnk, dftype);
				if (df) {
					dm = _RkAllocDM(df, (unsigned char *)member,
						(unsigned char *)nickname, dmclass);
					if (dm) {
						ddt->ddt_next = ddLines;
						ddt->ddt_prev = ddLines->ddt_prev;
						ddLines->ddt_prev = ddt;
						ddt->ddt_prev->ddt_next = ddt;
						ddt->ddt_status = 0;
						dm->dm_line = ddt;
						dm->dm_flags |= DM_WRITEOK; /* default access right */
						goto return_dm;
					}
					_RkFreeDF(df);
				}
				free(ddt->ddt_spec);
			}
			free(ddt);
		}
	}
return_dm:
#ifdef USE_MALLOC_FOR_BIG_ARRAY
	free(lnk);
	free(member);
	free(nickname);
#endif
	return(dm);
}

int
DMremove(
	struct DM	*dm)
{
	struct DF	*df = dm->dm_file;
	struct DDT	*ddt = dm->dm_line;
	
	
	/* free up dirs.dic line */
	if (ddt) {
		ddt->ddt_next->ddt_prev = ddt->ddt_prev;
		ddt->ddt_prev->ddt_next = ddt->ddt_next;
		if (ddt->ddt_spec)
			free(ddt->ddt_spec);
		free(ddt);
	};
	/* free dm itself */
	_RkFreeDM(dm);
	if (df) {
		struct DM	*mh = &df->df_members;
		
		if (mh == mh->dm_next)
			_RkFreeDF(df);
	};
	return 0;
}

int
DMrename(
	struct DM		*dm,
	unsigned char	*nickname)
{
	struct DF	*df = dm->dm_file;
	struct DDT	*ddt = dm->dm_line;
	char		*new_spec;
	char		*new_nick;
	char		member[5];
	char *dicname = (char *)0;
	int ret = -1;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
	char		spec[RK_LINE_BMAX];
#else
	char *spec = (char *)malloc(RK_LINE_BMAX);
	if (!spec) {
		return ret;
	}
#endif
	
	if (!df || !df->df_link || !dm->dm_file || !ddt || !ddt->ddt_spec)
		goto return_ret;
		if (df->df_type == DF_FREQDIC) {
			dicname = dm->dm_dicname;
		}
		switch (dm->dm_class) {
		default:
		case ND_MWD:
			(void)strcpy(member, ".mwd");
			break;
		case ND_SWD:
			(void)strcpy(member, ".swd");
			break;
		case ND_PRE:
			(void)strcpy(member, ".pre");
			break;
		case ND_SUC:
			(void)strcpy(member, ".suc");
			break;
		};
	(void)sprintf(spec, "%s(%s) -%s--%s%s-", df->df_link,
		dicname ? dicname : member, nickname,
		(dm->dm_flags & DM_READOK) ? "r" : "",
		(dm->dm_flags & DM_WRITEOK) ? "w" : "");
	new_spec = (char *)malloc(strlen(spec) + 3); /* 3 for \r\n\0 */
	if (!new_spec) {
		goto return_ret;
	}
	strcpy(new_spec, spec);
	if (!(new_nick = strdup((char *)nickname))) {
		free(new_spec);
		goto return_ret;
	};
	free(ddt->ddt_spec);
	ddt->ddt_spec = new_spec;
	free(dm->dm_nickname);
	dm->dm_nickname = new_nick;
	ret = 0;
	
return_ret:
#ifdef USE_MALLOC_FOR_BIG_ARRAY
	free(spec);
#endif
	return ret;
}

int
DMchmod(struct DM *dm, int mode)
{
	struct DF	*df = dm->dm_file;
	struct DDT	*ddt = dm->dm_line;
	char		*new_spec;
	unsigned newflags = dm->dm_flags;
	int ret = -1;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
	char		spec[RK_LINE_BMAX];
#else
	char *spec = (char *)malloc(RK_LINE_BMAX);
	if (!spec) {
		return ret;
	}
#endif
	
	if (!df || !df->df_link || !ddt || !ddt->ddt_spec)
		goto return_ret;
	
	/* READ ¥Ñ¡¼¥ß¥Ã¥·¥ç¥ó¤ÎÁàºî */
	if ((mode & (RK_ENABLE_READ | RK_DISABLE_READ)) == RK_ENABLE_READ) {
		newflags |= DM_READOK;
	}
else if ((mode & (RK_ENABLE_READ | RK_DISABLE_READ)) == RK_DISABLE_READ) {
	newflags &= ~DM_READOK;
}
	
	/* WRITE ¥Ñ¡¼¥ß¥Ã¥·¥ç¥ó¤ÎÁàºî */
	if ((mode & (RK_ENABLE_WRITE | RK_DISABLE_WRITE)) == RK_ENABLE_WRITE) {
		newflags |= DM_WRITEOK;
	}
else if ((mode & (RK_ENABLE_WRITE | RK_DISABLE_WRITE)) == RK_DISABLE_WRITE) {
	newflags &= ~DM_WRITEOK;
}
	
	if (newflags != dm->dm_flags) {
	if (df->df_direct->dd_flags & DD_WRITEOK) {
		dm->dm_flags = newflags;
		(void)sprintf(spec, "%s(%s) -%s--%s%s-",
			df->df_link, dm->dm_dicname, dm->dm_nickname,
			(dm->dm_flags & DM_READOK) ? "r" : "",
			(dm->dm_flags & DM_WRITEOK) ? "w" : "");
		new_spec = (char *)malloc(strlen(spec) + 3); /* 3 for \r\n\0 */
		if (!new_spec) {
			ret = NOTALC;
			goto return_ret;
		}
		strcpy(new_spec, spec);
		free(ddt->ddt_spec);
		ddt->ddt_spec = new_spec;
	}
	}
	ret = (((dm->dm_flags & DM_WRITEOK) ? RK_ENABLE_WRITE : RK_DISABLE_WRITE) |
		((dm->dm_flags & DM_READOK) ? RK_ENABLE_READ : RK_DISABLE_READ));
	
return_ret:
#ifdef USE_MALLOC_FOR_BIG_ARRAY
	free(spec);
#endif
	return ret;
}

int
DDchmod(struct DD *dd, int mode)
{
	char *dicsdir;
	unsigned newflags = dd->dd_flags;
	
	/* READ ¥Ñ¡¼¥ß¥Ã¥·¥ç¥ó¤ÎÁàºî */
	if ((mode & (RK_ENABLE_READ | RK_DISABLE_READ)) == RK_ENABLE_READ) {
		newflags |= DD_READOK;
	}
else if ((mode & (RK_ENABLE_READ | RK_DISABLE_READ)) == RK_DISABLE_READ) {
	newflags &= ~DD_READOK;
}
	
	/* WRITE ¥Ñ¡¼¥ß¥Ã¥·¥ç¥ó¤ÎÁàºî */
	if ((mode & (RK_ENABLE_WRITE | RK_DISABLE_WRITE)) == RK_ENABLE_WRITE) {
		newflags |= DD_WRITEOK;
	}
else if ((mode & (RK_ENABLE_WRITE | RK_DISABLE_WRITE)) == RK_DISABLE_WRITE) {
	newflags &= ~DD_WRITEOK;
}
	
	if (newflags != dd->dd_flags) {
		dicsdir = (char *)malloc(strlen(dd->dd_path + strlen("/dics.dir") + 1));
		if (dicsdir) {
			int filemode;
			
			sprintf(dicsdir, "%s/dics.dir", dd->dd_path);
			
			filemode = ((newflags & DD_WRITEOK) ? 0640 : 0440)
				| ((newflags & DD_READOK) ? 04 : 0);
			
			if (chmod(dicsdir, filemode) == 0) {
				dd->dd_flags = newflags;
			}
			free(dicsdir);
		}
	}
	newflags = dd->dd_flags;
	return (((newflags & DD_WRITEOK) ? RK_ENABLE_WRITE : RK_DISABLE_WRITE) |
		((newflags & DD_READOK) ? RK_ENABLE_READ : RK_DISABLE_READ));
}

int
_RkMountMD(
	struct RkContext	*cx,
	struct DM		*dm,
	struct DM		*qm,
	int		mode,
	int		firsttime)
{
	struct MD	*md, *head;
	struct DF	*df;
	struct DD	*dd;
	char		*file;
	int		status;
	
	if (!dm || !(md = (struct MD *)calloc(1, sizeof(struct MD))))
		return -1;
	/* increment the reference counter */
	if (dm->dm_rcount == 0) {
		df = dm->dm_file;
		dd = df->df_direct;
		if (!(file = _RkCreatePath(dd, df->df_link))) {
			free(md);
			return -1;
		};
		status = DST_OPEN(dm, file,  DM_WRITABLE, cx->gram->gramdic);
		free(file);
		if (status) {
			free(md);
			return -1;
		};
	};
	if (qm && qm->dm_rcount == 0) {
		df = qm->dm_file;
		dd = df->df_direct;
		if (!(file = _RkCreatePath(dd, df->df_link))) {
			free(md);
			return -1;
		};
		status = FQopen(dm, qm, file, DM_WRITABLE);
		free(file);
		if (status) {
			free(md);
			return -1;
		};
	};
	/* use the dic as the default grammatical dic if it contains */
	if (firsttime && DM2TYPE(dm) == DF_PERMDIC && dm->dm_gram) {
		cx->gram = dm->dm_gram;
		cx->gram->refcount++;
	}
	/* increment the reference counter */
	dm->dm_rcount++;
	if (qm) {
		qm->dm_rcount++;
		if (!cx->nv && qm->dm_nv)
			cx->nv = qm->dm_nv;
	}
	/* put it at the end of the mount list */
	head = cx->md[dm->dm_class];
	md->md_next = head;
	md->md_prev = head->md_prev;
	head->md_prev = md;
	md->md_prev->md_next = md;
	md->md_dic = dm;
	md->md_freq = qm;
	md->md_flags = mode&MD_WRITE;
	/* wait for the translation to finish */
	if (IS_XFERCTX(cx))
		md->md_flags |= MD_MPEND;
	return 0;
}

void
_RkUmountMD(
	struct RkContext	*cx,
	struct MD		*md)
{
	struct DM	*dm = md->md_dic;
	struct DM	*qm = md->md_freq;
	struct DF	*df;
	struct DD	*dd;
	char		*file;
	
	cx->dmprev = (struct DM *)0;
	cx->qmprev = (struct DM *)0;
	if (IS_XFERCTX(cx))
		md->md_flags |= MD_UPEND;
	else {
		md->md_prev->md_next = md->md_next;
		md->md_next->md_prev = md->md_prev;
		free(md);
		if (qm) {
			df = qm->dm_file;
			dd = df->df_direct;
			if (cx->nv == qm->dm_nv)
				cx->nv = (struct NV *)0;
				if (qm->dm_rcount > 0 && --qm->dm_rcount == 0) {
					file = _RkCreatePath(dd, df->df_link);
					if (file) {
						FQclose(cx, dm, qm, file);
						if (!df->df_rcount) {
							if (dd->dd_rcount > 0 && --dd->dd_rcount == 0)
								_RkFreeDD(dd);
						};
						free(file);
					};
				};
		};
		if (dm->dm_rcount > 0 && --dm->dm_rcount == 0) {
			df = dm->dm_file;
			dd = df->df_direct;
			file  = _RkCreatePath(dd, df->df_link);
			if (file) {
				(void)DST_CLOSE(dm, file, cx->gram->gramdic);
				if (df->df_rcount == 0) {
					if (dd->dd_rcount > 0 && --dd->dd_rcount == 0)
						_RkFreeDD(dd);
				};
				free(file);
			};
		};
	};
}
