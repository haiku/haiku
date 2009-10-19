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
static char rcs_id[] = "@(#) 102.1 $Id: parse.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif /* lint */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "canna.h"

extern char *CANNA_initfilename;

#define BUF_LEN 1024

static char CANNA_rcfilename[BUF_LEN] = "";


/* cfuncdef

   YYparse -- ¥«¥¹¥¿¥Þ¥¤¥º¥Õ¥¡¥¤¥ë¤òÆÉ¤à¡£

   ¥Õ¥¡¥¤¥ë¥Ç¥£¥¹¥¯¥ê¥×¥¿¤Ç»ØÄê¤µ¤ì¤¿¥Õ¥¡¥¤¥ë¤òÆÉ¤ß¹þ¤à¡£

*/

extern int ckverbose;

extern int YYparse_by_rcfilename();

/* cfuncdef

  parse -- .canna ¥Õ¥¡¥¤¥ë¤òÃµ¤·¤Æ¤­¤ÆÆÉ¤ß¹þ¤à¡£

  parse ¤Ï¥«¥¹¥¿¥Þ¥¤¥º¥Õ¥¡¥¤¥ë¤òÃµ¤·¡¢¤½¤Î¥Õ¥¡¥¤¥ë¤ò¥ª¡¼¥×¥ó¤·¥Ñ¡¼¥¹¤¹
  ¤ë¡£

  ¥Ñ¡¼¥¹Ãæ¤Î¥Õ¥¡¥¤¥ë¤ÎÌ¾Á°¤ò CANNA_rcfilename ¤ËÆþ¤ì¤Æ¤ª¤¯¡£

  */

#define NAMEBUFSIZE 1024
#define RCFILENAME  ".canna"
#define FILEENVNAME "CANNAFILE"
#define HOSTENVNAME "CANNAHOST"

#define OBSOLETE_RCFILENAME  ".iroha"
#define OBSOLETE_FILEENVNAME "IROHAFILE"
#define OBSOLETE_HOSTENVNAME "IROHAHOST"

static int DISPLAY_to_hostname(char *name, char *buf, int bufsize);

static
int make_initfilename(void)
{
  if(!CANNA_initfilename) {
    CANNA_initfilename = (char *)malloc(1024);
    if (!CANNA_initfilename) {
      return -1;
    }
    strcpy(CANNA_initfilename, CANNA_rcfilename);
  }
  else {
    strcat(CANNA_initfilename, ",");
    strcat(CANNA_initfilename, CANNA_rcfilename);
  }
  return 0;
}

static void
fit_initfilename(void)
{
  char *tmpstr;

  if (CANNA_initfilename) {
    tmpstr = (char *)malloc(strlen(CANNA_initfilename) + 1);
    if (!tmpstr) return;
    strcpy(tmpstr, CANNA_initfilename);
    free(CANNA_initfilename);
    CANNA_initfilename = tmpstr;
  }
}

  extern char *initFileSpecified;
void
parse(void)
{
  if (clisp_init() == 0) {
    addWarningMesg("Can't alocate memory\n");
    goto quitparse;
  }

  if (initFileSpecified) {
    strcpy(CANNA_rcfilename, initFileSpecified);
    if (YYparse_by_rcfilename(CANNA_rcfilename)) {
      make_initfilename();
      goto quitparse;
    }else {
      goto error;
    }
  }else{
	extern char basepath[];
    sprintf(CANNA_rcfilename, "%s%s%s",basepath, "default/", RCFILENAME);
    if (YYparse_by_rcfilename(CANNA_rcfilename)) {
      make_initfilename();
      goto quitparse;
    }
  }

error:
	{
	    char buf[256];
	    sprintf(buf, "Can't open customize file : %s", CANNA_rcfilename);
	    addWarningMesg(buf);
    }

quitparse:
  /* CANNA_initfilename ¤ò¥¸¥ã¥¹¥È¥µ¥¤¥º¤Ë´¢¤ê¹þ¤à */
  fit_initfilename();
  clisp_fin();
}

#if 0
static
DISPLAY_to_hostname(char *name, char *buf, int bufsize)
{
  if (name[0] == ':' || !strncmp(name, "unix", 4)) {
 //   gethostname(buf, bufsize);
  }
  else {
    int i, len = strlen(name);
    for (i = 0 ; i < len && i < bufsize ; i++) {
      if (name[i] == ':') {
	break;
      }else{
	buf[i] = name[i];
      }
    }
    if (i < bufsize) {
      buf[i] = '\0';
    }
  }
}
#endif
