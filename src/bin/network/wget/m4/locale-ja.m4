# locale-ja.m4 serial 7
dnl Copyright (C) 2003, 2005-2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Bruno Haible.

dnl Determine the name of a japanese locale with EUC-JP encoding.
AC_DEFUN([gt_LOCALE_JA],
[
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_REQUIRE([AM_LANGINFO_CODESET])
  AC_CACHE_CHECK([for a traditional japanese locale], [gt_cv_locale_ja], [
    AC_LANG_CONFTEST([AC_LANG_SOURCE([
changequote(,)dnl
#include <locale.h>
#include <time.h>
#if HAVE_LANGINFO_CODESET
# include <langinfo.h>
#endif
#include <stdlib.h>
#include <string.h>
struct tm t;
char buf[16];
int main ()
{
  const char *p;
  /* Check whether the given locale name is recognized by the system.  */
  if (setlocale (LC_ALL, "") == NULL) return 1;
  /* Check whether nl_langinfo(CODESET) is nonempty and not "ASCII" or "646".
     On MacOS X 10.3.5 (Darwin 7.5) in the fr_FR locale, nl_langinfo(CODESET)
     is empty, and the behaviour of Tcl 8.4 in this locale is not useful.
     On OpenBSD 4.0, when an unsupported locale is specified, setlocale()
     succeeds but then nl_langinfo(CODESET) is "646". In this situation,
     some unit tests fail.  */
#if HAVE_LANGINFO_CODESET
  {
    const char *cs = nl_langinfo (CODESET);
    if (cs[0] == '\0' || strcmp (cs, "ASCII") == 0 || strcmp (cs, "646") == 0)
      return 1;
  }
#endif
#ifdef __CYGWIN__
  /* On Cygwin, avoid locale names without encoding suffix, because the
     locale_charset() function relies on the encoding suffix.  Note that
     LC_ALL is set on the command line.  */
  if (strchr (getenv ("LC_ALL"), '.') == NULL) return 1;
#endif
  /* Check whether MB_CUR_MAX is > 1.  This excludes the dysfunctional locales
     on Cygwin 1.5.x.  */
  if (MB_CUR_MAX == 1)
    return 1;
  /* Check whether in a month name, no byte in the range 0x80..0x9F occurs.
     This excludes the UTF-8 encoding.  */
  t.tm_year = 1975 - 1900; t.tm_mon = 2 - 1; t.tm_mday = 4;
  if (strftime (buf, sizeof (buf), "%B", &t) < 2) return 1;
  for (p = buf; *p != '\0'; p++)
    if ((unsigned char) *p >= 0x80 && (unsigned char) *p < 0xa0)
      return 1;
  return 0;
}
changequote([,])dnl
      ])])
    if AC_TRY_EVAL([ac_link]) && test -s conftest$ac_exeext; then
      # Setting LC_ALL is not enough. Need to set LC_TIME to empty, because
      # otherwise on MacOS X 10.3.5 the LC_TIME=C from the beginning of the
      # configure script would override the LC_ALL setting. Likewise for
      # LC_CTYPE, which is also set at the beginning of the configure script.
      # Test for the AIX locale name.
      if (LC_ALL=ja_JP LC_TIME= LC_CTYPE= ./conftest; exit) 2>/dev/null; then
        gt_cv_locale_ja=ja_JP
      else
        # Test for the locale name with explicit encoding suffix.
        if (LC_ALL=ja_JP.EUC-JP LC_TIME= LC_CTYPE= ./conftest; exit) 2>/dev/null; then
          gt_cv_locale_ja=ja_JP.EUC-JP
        else
          # Test for the HP-UX, OSF/1, NetBSD locale name.
          if (LC_ALL=ja_JP.eucJP LC_TIME= LC_CTYPE= ./conftest; exit) 2>/dev/null; then
            gt_cv_locale_ja=ja_JP.eucJP
          else
            # Test for the IRIX, FreeBSD locale name.
            if (LC_ALL=ja_JP.EUC LC_TIME= LC_CTYPE= ./conftest; exit) 2>/dev/null; then
              gt_cv_locale_ja=ja_JP.EUC
            else
              # Test for the Solaris 7 locale name.
              if (LC_ALL=ja LC_TIME= LC_CTYPE= ./conftest; exit) 2>/dev/null; then
                gt_cv_locale_ja=ja
              else
                # Special test for NetBSD 1.6.
                if test -f /usr/share/locale/ja_JP.eucJP/LC_CTYPE; then
                  gt_cv_locale_ja=ja_JP.eucJP
                else
                  # None found.
                  gt_cv_locale_ja=none
                fi
              fi
            fi
          fi
        fi
      fi
    fi
    rm -fr conftest*
  ])
  LOCALE_JA=$gt_cv_locale_ja
  AC_SUBST([LOCALE_JA])
])
