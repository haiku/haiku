dnl Process this file with -*- autoconf -*- to produce a configure script.
AC_INIT(sed, 4.0.9, bonzini@gnu.org, sed)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR([sed/sed.c])
AM_CONFIG_HEADER(config.h:config_h.in)
AC_PREREQ(2.53)
AM_INIT_AUTOMAKE

SED_FEATURE_VERSION=4.0
AC_DEFINE_UNQUOTED(SED_FEATURE_VERSION, "$SED_FEATURE_VERSION",
  [Define to the version of GNU sed whose features are supported by this sed.])
AC_SUBST(SED_FEATURE_VERSION)

AC_PROG_CC
AC_PROG_RANLIB
AC_GNU_SOURCE
AC_AIX
AC_MINIX
AC_ISC_POSIX
AC_SYS_LARGEFILE

AC_MSG_CHECKING([whether -lcP is needed])
AC_TRY_RUN([
#include <stdio.h>
#include <errno.h>

int main()
{
  FILE *fp;
  int result;
  errno = 0;
  fp = fopen ("conftest.c", "r");
  if (!fp) return 0;       /* error, assume not needed */
  result = fflush (fp) == EOF && errno == 0;
  fclose (fp);
  return result;
}], [AC_MSG_RESULT(no)],
    [AC_MSG_RESULT(yes)
     LIBS="-lcP $LIBS"],
    [AC_MSG_RESULT([assuming no])
])

AC_HEADER_STDC
AC_CHECK_HEADERS(string.h strings.h stdlib.h unistd.h io.h \
		 limits.h locale.h stdarg.h alloca.h stddef.h errno.h \
		 wchar.h wctype.h sys/file.h)
AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_TYPE(ssize_t, int)

AC_FUNC_ALLOCA
AC_FUNC_VPRINTF
AM_FUNC_GETLINE
AC_FUNC_OBSTACK
gl_FUNC_STRVERSCMP
AC_FUNC_MBRTOWC
AC_TYPE_MBSTATE_T
AC_REPLACE_FUNCS(memchr memcmp memmove strerror mkstemp)
AC_CHECK_FUNCS(isatty bcopy bzero isascii memcpy memset strchr strtoul popen \
		isblank fchmod setlocale wcrtomb wcscoll)

AC_ARG_WITH(included-regex, 
[  --with-included-regex   use included regex matcher (default=yes)], ,
with_included_regex=yes)

AC_ARG_ENABLE(regex-tests,
[  --enable-regex-tests    enable regex matcher regression tests (default=no)], ,
enable_regex_tests=no)

if test "x$with_included_regex" = xno; then
  AC_CHECK_HEADERS(regex.h)
  AC_CHECK_LIB(regex, re_search)
  AC_CHECK_FUNC(re_search)
  if test $ac_cv_header_regex_h = no || test $ac_cv_func_re_search = no; then
    AC_MSG_WARN([GNU regex not found, falling back to the included version])
    with_included_regex=yes
  fi
fi

if test "x$with_included_regex" = xno; then
  enable_regex_tests=no
fi

AM_CONDITIONAL(TEST_REGEX, test "x$enable_regex_tests" != xno)
if test "x$with_included_regex" != xno; then
  AC_CONFIG_LINKS(lib/regex.h:lib/regex_.h)
  AC_LIBOBJ(regex)
fi

AC_ARG_ENABLE(html, 
[  --enable-html           build HTML manual (default=no)], ,
enable_html=no)

AM_CONDITIONAL(BUILD_HTML, test "x$enable_html" != xno)

: ${TEXI2HTML=texi2html -monolithic}
AC_SUBST(TEXI2HTML)

AC_MSG_CHECKING(how to build HTML documentation)
if eval $am_missing_run makeinfo --help 2>&1 | grep .-html > /dev/null; then
  AC_MSG_RESULT(with makeinfo)
  enable_html=makeinfo
else
  if $TEXI2HTML --help 2>&1 | grep monolithic > /dev/null; then
    AC_MSG_RESULT(with texi2html)
    enable_html=texi2html
  else
    AC_MSG_RESULT(not built)
    if test "x$enable_html" != xno; then
      AC_MSG_ERROR(cannot build HTML documentation, install makeinfo 4.0 or texi2html)
    fi
    enable_html=no
  fi
fi 

AM_CONDITIONAL(MAKEINFO_HTML, test "x$enable_html" = xmakeinfo)
AM_CONDITIONAL(TEXI2HTML_HTML, test "x$enable_html" = xtexi2html)


ALL_LINGUAS="ca cs da de el eo es et fi fr ga gl he hr hu id it ja ko nl pl pt_BR ro ru sk sl sr sv tr zh_CN"
AM_GNU_GETTEXT_VERSION(0.11)
AM_GNU_GETTEXT(, need-ngettext)

AC_CONFIG_FILES([bootstrap.sh], chmod +x bootstrap.sh)
AC_CONFIG_FILES([Makefile doc/Makefile \
lib/Makefile sed/Makefile testsuite/Makefile \
po/Makefile.in intl/Makefile])
AC_OUTPUT
