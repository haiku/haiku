# Local additions to Autoconf macros.
# Copyright (C) 1992, 1994 Free Software Foundation, Inc.
# Francois Pinard <pinard@iro.umontreal.ca>, 1992.

# NLS configuration macros added.
# Ulrich Drepper <drepper@gnu.ai.mit.edu>, August 1995.

# @defmac AC_PROG_CC_STDC
# @maindex PROG_CC_STDC
# @ovindex CC
# If the C compiler in not in ANSI C mode by default, try to add an option
# to output variable @code{CC} to make it so.  This macro tries various
# options that select ANSI C on some system or another.  It considers the
# compiler to be in ANSI C mode if it defines @code{__STDC__} to 1 and
# handles function prototypes correctly.
#
# If you use this macro, you should check after calling it whether the C
# compiler has been set to accept ANSI C; if not, the shell variable
# @code{ac_cv_prog_cc_stdc} is set to @samp{no}.  If you wrote your source
# code in ANSI C, you can make an un-ANSIfied copy of it by using the
# program @code{ansi2knr}, which comes with Ghostscript.
# @end defmac

define(fp_PROG_CC_STDC,
[AC_MSG_CHECKING(for ${CC-cc} option to accept ANSI C)
AC_CACHE_VAL(ac_cv_prog_cc_stdc,
[ac_cv_prog_cc_stdc=no
ac_save_CFLAGS="$CFLAGS"
# Don't try gcc -ansi; that turns off useful extensions and
# breaks some systems' header files.
# AIX			-qlanglvl=ansi
# Ultrix and OSF/1	-std1
# HP-UX			-Aa -D_HPUX_SOURCE
# SVR4			-Xc
for ac_arg in "" -qlanglvl=ansi -std1 "-Aa -D_HPUX_SOURCE" -Xc
do
  CFLAGS="$ac_save_CFLAGS $ac_arg"
  AC_TRY_COMPILE(
[#if !defined(__STDC__) || __STDC__ != 1
choke me
#endif
], [int test (int i, double x);
struct s1 {int (*f) (int a);};
struct s2 {int (*f) (double a);};],
[ac_cv_prog_cc_stdc="$ac_arg"; break])
done
CFLAGS="$ac_save_CFLAGS"
])
AC_MSG_RESULT($ac_cv_prog_cc_stdc)
case "x$ac_cv_prog_cc_stdc" in
  x|xno) ;;
  *) CC="$CC $ac_cv_prog_cc_stdc" ;;
esac
])

# Check for function prototypes.

AC_DEFUN(fp_C_PROTOTYPES,
[AC_REQUIRE([fp_PROG_CC_STDC])
AC_MSG_CHECKING([for function prototypes])
if test "$ac_cv_prog_cc_stdc" != no; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(PROTOTYPES)
  U= ANSI2KNR=
else
  AC_MSG_RESULT(no)
  U=_ ANSI2KNR=ansi2knr
fi
AC_SUBST(U)dnl
AC_SUBST(ANSI2KNR)dnl
])

# Check if --with-dmalloc was given.

AC_DEFUN(fp_WITH_DMALLOC,
[AC_MSG_CHECKING(if malloc debugging is wanted)
AC_ARG_WITH(dmalloc,
[  --with-dmalloc          use dmalloc, as in dmalloc.tar.gz from
                          @/ftp.antaire.com:antaire/src/dmalloc.],
[if test "$withval" = yes; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(WITH_DMALLOC)
  LIBS="$LIBS -ldmalloc"
  LDFLAGS="$LDFLAGS -g"
else
  AC_MSG_RESULT(no)
fi], [AC_MSG_RESULT(no)])])

dnl --------------------------------------------------------- ##
dnl Use AC_PROG_INSTALL, supplementing it with INSTALL_SCRIPT ##
dnl substitution.                                             ##
dnl --------------------------------------------------------- ##

AC_DEFUN(fp_PROG_INSTALL,
[AC_PROG_INSTALL
test -z "$INSTALL_SCRIPT" && INSTALL_SCRIPT='${INSTALL} -m 755'
AC_SUBST(INSTALL_SCRIPT)dnl
])

AC_DEFUN(md_PATH_PROG,
  [AC_PATH_PROG($1,$2,$3)dnl
   if echo $$1 | grep openwin > /dev/null; then
     echo "WARNING: Do not use OpenWin's $2.  (Better remove it.) >&AC_FD_MSG"
     ac_cv_path_$1=$2
     $1=$2
   fi
])

dnl Check NLS options

AC_DEFUN(ud_LC_MESSAGES,
  [if test $ac_cv_header_locale_h = yes; then
    AC_MSG_CHECKING([for LC_MESSAGES])
    AC_CACHE_VAL(ud_cv_val_LC_MESSAGES,
      [AC_TRY_LINK([#include <locale.h>], [return LC_MESSAGES],
       ud_cv_val_LC_MESSAGES=yes, ud_cv_val_LC_MESSAGES=no)])
    AC_MSG_RESULT($ud_cv_val_LC_MESSAGES)
    if test $ud_cv_val_LC_MESSAGES = yes; then
      AC_DEFINE(HAVE_LC_MESSAGES)
    fi
  fi])

AC_DEFUN(ud_WITH_NLS,
  [AC_MSG_CHECKING([whether NLS is requested])
    dnl Default is enabled NLS
    AC_ARG_ENABLE(nls,
      [  --disable-nls           do not use Native Language Support],
      nls_cv_use_nls=$enableval, nls_cv_use_nls=yes)
    AC_MSG_RESULT($nls_cv_use_nls)

    dnl If we use NLS figure out what method
    if test "$nls_cv_use_nls" = "yes"; then
      AC_DEFINE(ENABLE_NLS)
      AC_MSG_CHECKING([for explicitly using GNU gettext])
      AC_ARG_WITH(gnu-gettext,
        [  --with-gnu-gettext      use the GNU gettext library],
        nls_cv_force_use_gnu_gettext=$withval,
        nls_cv_force_use_gnu_gettext=no)
      AC_MSG_RESULT($nls_cv_force_use_gnu_gettext)

      if test "$nls_cv_force_use_gnu_gettext" = "yes"; then
        nls_cv_use_gnu_gettext=yes
      else
        dnl User does not insist on using GNU NLS library.  Figure out what
        dnl to use.  If gettext or catgets are available (in this order) we
        dnl use this.  Else we have to fall back to GNU NLS library.
        AC_CHECK_LIB(intl, main)
        AC_CHECK_LIB(i, main)
        CATOBJEXT=NONE
        AC_CHECK_FUNC(gettext,
          [AC_DEFINE(HAVE_GETTEXT)
           AC_PATH_PROG(MSGFMT, msgfmt, no)dnl
	   if test "$MSGFMT" != "no"; then
	     AC_CHECK_FUNCS(dcgettext)
	     md_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)dnl
	     md_PATH_PROG(XGETTEXT, xgettext, xgettext)dnl
             CATOBJEXT=.mo
	     INSTOBJEXT=.mo
	     DATADIRNAME=lib
	   fi])

        if test "$CATOBJEXT" = "NONE"; then
          dnl No gettext in C library.  Try catgets next.
          AC_CHECK_FUNC(catgets,
            [AC_DEFINE(HAVE_CATGETS)
             INTLOBJS="\$(CATOBJS)"
             AC_PATH_PROG(GENCAT, gencat, no)dnl
	     if test "$GENCAT" != "no"; then
	       AC_PATH_PROGS(GMSGFMT, [gmsgfmt msgfmt], msgfmt)dnl
	       md_PATH_PROG(XGETTEXT, xgettext, xgettext)dnl
               CATOBJEXT=.cat
	       INSTOBJEXT=.cat
	       DATADIRNAME=lib
	       INTLDEPS="../intl/libintl.a"
	       INTLLIBS=$INTLDEPS
	       LIBS=`echo $LIBS | sed -e 's/-lintl//'`
	       nls_cv_header_intl=intl/libintl.h
	       nls_cv_header_libgt=intl/libgettext.h

	       # We need to process the intl/ and po/ directory.
	       INTLSUB=intl
	     fi])
        fi

        if test "$CATOBJEXT" = "NONE"; then
	  dnl Neither gettext nor catgets in included in the C library.
	  dnl Fall back on GNU gettext library.
	  nls_cv_use_gnu_gettext=yes
        fi
      fi

      if test "$nls_cv_use_gnu_gettext" = "yes"; then
        dnl Mark actions used to generate GNU NLS library.
        INTLOBJS="\$(GETTOBJS)"
        md_PATH_PROG(MSGFMT, msgfmt, msgfmt)dnl
        md_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)dnl
        md_PATH_PROG(XGETTEXT, xgettext, xgettext)dnl
        AC_SUBST(MSGFMT)
        CATOBJEXT=.gmo
        INSTOBJEXT=.mo
        DATADIRNAME=share
        INTLDEPS="../intl/libintl.a"
        INTLLIBS=$INTLDEPS
	LIBS=`echo $LIBS | sed -e 's/-lintl//'`
        nls_cv_header_intl=intl/libintl.h
        nls_cv_header_libgt=intl/libgettext.h

	# We need to process the intl/ directory.
	INTLSUB=intl
      fi

      # We need to process the po/ directory.
      POSUB=po
    else
      DATADIRNAME=share
      nls_cv_header_intl=intl/libintl.h
      nls_cv_header_libgt=intl/libgettext.h
    fi

    dnl These rules are solely for the distribution goal.  While doing this
    dnl we only have to keep exactly one list of the available catalogs
    dnl in configure.in.
    for lang in $ALL_LINGUAS; do
      GMOFILES="$GMOFILES $lang.gmo"
      POFILES="$POFILES $lang.po"
    done

    dnl Make all variables we use known to autoconf.
    AC_SUBST(CATALOGS)
    AC_SUBST(CATOBJEXT)
    AC_SUBST(DATADIRNAME)
    AC_SUBST(GMOFILES)
    AC_SUBST(INSTOBJEXT)
    AC_SUBST(INTLDEPS)
    AC_SUBST(INTLLIBS)
    AC_SUBST(INTLOBJS)
    AC_SUBST(INTLSUB)
    AC_SUBST(POFILES)
    AC_SUBST(POSUB)
  ])

AC_DEFUN(ud_GNU_GETTEXT,
  [AC_REQUIRE([AC_PROG_MAKE_SET])dnl
   AC_REQUIRE([AC_PROG_CC])dnl
   AC_REQUIRE([AC_PROG_RANLIB])dnl
   AC_REQUIRE([AC_HEADER_STDC])dnl
   AC_REQUIRE([AC_C_CONST])dnl
   AC_REQUIRE([AC_C_INLINE])dnl
   AC_REQUIRE([AC_TYPE_OFF_T])dnl
   AC_REQUIRE([AC_TYPE_SIZE_T])dnl
   AC_REQUIRE([AC_FUNC_ALLOCA])dnl
   AC_REQUIRE([AC_FUNC_MMAP])dnl

   AC_CHECK_HEADERS([limits.h locale.h nl_types.h malloc.h string.h unistd.h values.h])
   AC_CHECK_FUNCS([getcwd munmap putenv setenv setlocale strchr strcasecmp])

   if test "${ac_cv_func_stpcpy+set}" != "set"; then
     AC_CHECK_FUNCS(stpcpy)
   fi
   if test "${ac_cv_func_stpcpy}" = "yes"; then
     AC_DEFINE(HAVE_STPCPY)
   fi

   ud_LC_MESSAGES
   ud_WITH_NLS

   if test "x$CATOBJEXT" != "x"; then
     if test "x$ALL_LINGUAS" = "x"; then
       LINGUAS=
     else
       AC_MSG_CHECKING(for catalogs to be installed)
       NEW_LINGUAS=
       for lang in ${LINGUAS=$ALL_LINGUAS}; do
         case "$ALL_LINGUAS" in
          *$lang*) NEW_LINGUAS="$NEW_LINGUAS $lang" ;;
         esac
       done
       LINGUAS=$NEW_LINGUAS
       AC_MSG_RESULT($LINGUAS)
     fi

     dnl Construct list of names of catalog files to be constructed.
     if test -n "$LINGUAS"; then
       for lang in $LINGUAS; do CATALOGS="$CATALOGS $lang$CATOBJEXT"; done
     fi
   fi

   dnl Determine which catalog format we have (if any is needed)
   dnl For now we know about two different formats:
   dnl   Linux and the normal X/Open format
   test -d intl || mkdir intl
   if test "$CATOBJEXT" = ".cat"; then
     AC_CHECK_HEADER(linux/version.h, msgformat=linux, msgformat=xopen)

     dnl Transform the SED scripts while copying because some dumb SEDs
     dnl cannot handle comments.
     sed -e '/^#/d' $srcdir/intl/$msgformat-msg.sed > intl/po2msg.sed
   fi
   dnl po2tbl.sed is always needed.
   sed -e '/^#.*[^\\]$/d' -e '/^#$/d' \
     $srcdir/intl/po2tbl.sed.in > intl/po2tbl.sed

   dnl Generate list of files to be processed by xgettext which will
   dnl be included in po/Makefile.
   test -d po || mkdir po
   if test "x$srcdir" != "x."; then
     if test "x`echo $srcdir | sed 's@/.*@@'`" = "x"; then
       posrcprefix="$srcdir/"
     else
       posrcprefix="../$srcdir/"
     fi
   else
     posrcprefix="../"
   fi
   sed -e "/^#/d" -e "/^\$/d" -e "s,.*,	$posrcprefix& \\\\," -e "\$s/\(.*\) \\\\/\1/" \
	< $srcdir/po/POTFILES.in > po/POTFILES
  ])
