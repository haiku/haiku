dnl Wget-specific Autoconf macros.
dnl Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
dnl 2004, 2005, 2006, 2007, 2008 Free Software Foundation, Inc.

dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.

dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

dnl Additional permission under GNU GPL version 3 section 7

dnl If you modify this program, or any covered work, by linking or
dnl combining it with the OpenSSL project's OpenSSL library (or a
dnl modified version of that library), containing parts covered by the
dnl terms of the OpenSSL or SSLeay licenses, the Free Software Foundation
dnl grants you additional permission to convey the resulting work.
dnl Corresponding Source for a non-source form of such a combination
dnl shall include the source code for the parts of OpenSSL used as well
dnl as that of the covered work.

dnl
dnl Check for `struct utimbuf'.
dnl

AC_DEFUN([WGET_STRUCT_UTIMBUF], [
  AC_CHECK_TYPES([struct utimbuf], [], [], [
#include <stdio.h>
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_UTIME_H
# include <utime.h>
#endif
  ])
])


dnl Check for socklen_t.  The third argument of accept, getsockname,
dnl etc. is int * on some systems, but size_t * on others.  POSIX
dnl finally standardized on socklen_t, but older systems don't have
dnl it.  If socklen_t exists, we use it, else if accept() accepts
dnl size_t *, we use that, else we use int.

AC_DEFUN([WGET_SOCKLEN_T], [
  AC_MSG_CHECKING(for socklen_t)
  AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <sys/socket.h>
socklen_t x;
  ], [AC_MSG_RESULT(socklen_t)], [
    AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <sys/socket.h>
int accept (int, struct sockaddr *, size_t *);
    ], [
      AC_MSG_RESULT(size_t)
      AC_DEFINE([socklen_t], [size_t],
                [Define to int or size_t on systems without socklen_t.])
    ], [
      AC_MSG_RESULT(int)
      AC_DEFINE([socklen_t], [int],
                [Define to int or size_t on systems without socklen_t.])
    ])
  ])
])

dnl Check whether fnmatch.h can be included.  This doesn't use
dnl AC_FUNC_FNMATCH because Wget is already careful to only use
dnl fnmatch on certain OS'es.  However, fnmatch.h is sometimes broken
dnl even on those because Apache installs its own fnmatch.h to
dnl /usr/local/include (!), which GCC uses before /usr/include.

AC_DEFUN([WGET_FNMATCH], [
  AC_MSG_CHECKING([for working fnmatch.h])
  AC_COMPILE_IFELSE([#include <fnmatch.h>
                    ], [
    AC_MSG_RESULT(yes)
    AC_DEFINE([HAVE_WORKING_FNMATCH_H], 1,
              [Define if fnmatch.h can be included.])
  ], [
    AC_MSG_RESULT(no)
  ])
])

dnl Check for nanosleep.  For nanosleep to work on Solaris, we must
dnl link with -lrt (recently) or with -lposix4 (older releases).

AC_DEFUN([WGET_NANOSLEEP], [
  AC_CHECK_FUNCS(nanosleep, [], [
    AC_CHECK_LIB(rt, nanosleep, [
      AC_DEFINE([HAVE_NANOSLEEP], 1,
                [Define if you have the nanosleep function.])
      LIBS="-lrt $LIBS"
    ], [
      AC_CHECK_LIB(posix4, nanosleep, [
	AC_DEFINE([HAVE_NANOSLEEP], 1,
		  [Define if you have the nanosleep function.])
	LIBS="-lposix4 $LIBS"
      ])
    ])
  ])
])

AC_DEFUN([WGET_POSIX_CLOCK], [
  AC_CHECK_FUNCS(clock_gettime, [], [
    AC_CHECK_LIB(rt, clock_gettime)
  ])
])

dnl Check whether we need to link with -lnsl and -lsocket, as is the
dnl case on e.g. Solaris.

AC_DEFUN([WGET_NSL_SOCKET], [
  dnl On Solaris, -lnsl is needed to use gethostbyname.  But checking
  dnl for gethostbyname is not enough because on "NCR MP-RAS 3.0"
  dnl gethostbyname is in libc, but -lnsl is still needed to use
  dnl -lsocket, as well as for functions such as inet_ntoa.  We look
  dnl for such known offenders and if one of them is not found, we
  dnl check if -lnsl is needed.
  wget_check_in_nsl=NONE
  AC_CHECK_FUNCS(gethostbyname, [], [
    wget_check_in_nsl=gethostbyname
  ])
  AC_CHECK_FUNCS(inet_ntoa, [], [
    wget_check_in_nsl=inet_ntoa
  ])
  if test $wget_check_in_nsl != NONE; then
    AC_CHECK_LIB(nsl, $wget_check_in_nsl)
  fi
  AC_CHECK_LIB(socket, socket)
])


dnl ************************************************************
dnl START OF IPv6 AUTOCONFIGURATION SUPPORT MACROS
dnl ************************************************************

AC_DEFUN([TYPE_STRUCT_SOCKADDR_IN6],[
  wget_have_sockaddr_in6=
  AC_CHECK_TYPES([struct sockaddr_in6],[
    wget_have_sockaddr_in6=yes
  ],[
    wget_have_sockaddr_in6=no
  ],[
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
  ])

  if test "X$wget_have_sockaddr_in6" = "Xyes"; then :
    $1
  else :
    $2
  fi
])


AC_DEFUN([MEMBER_SIN6_SCOPE_ID],[
  AC_REQUIRE([TYPE_STRUCT_SOCKADDR_IN6])
  
  wget_member_sin6_scope_id=
  if test "X$wget_have_sockaddr_in6" = "Xyes"; then
    AC_CHECK_MEMBER([struct sockaddr_in6.sin6_scope_id],[
      wget_member_sin6_scope_id=yes
    ],[
      wget_member_sin6_scope_id=no
    ],[
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
    ])
  fi

  if test "X$wget_member_sin6_scope_id" = "Xyes"; then
    AC_DEFINE([HAVE_SOCKADDR_IN6_SCOPE_ID], 1,
      [Define if struct sockaddr_in6 has the sin6_scope_id member])
    $1
  else :
    $2
  fi
])


AC_DEFUN([PROTO_INET6],[
  AC_CACHE_CHECK([for INET6 protocol support], [wget_cv_proto_inet6],[
    AC_TRY_CPP([
#include <sys/types.h>
#include <sys/socket.h>

#ifndef PF_INET6
#error Missing PF_INET6
#endif
#ifndef AF_INET6
#error Mlssing AF_INET6
#endif
    ],[
      wget_cv_proto_inet6=yes
    ],[
      wget_cv_proto_inet6=no
    ])
  ])

  if test "X$wget_cv_proto_inet6" = "Xyes"; then :
    $1
  else :
    $2
  fi
])


AC_DEFUN([WGET_STRUCT_SOCKADDR_STORAGE],[
  AC_CHECK_TYPES([struct sockaddr_storage],[], [], [
#include <sys/types.h>
#include <sys/socket.h>
  ])
])

dnl ************************************************************
dnl END OF IPv6 AUTOCONFIGURATION SUPPORT MACROS
dnl ************************************************************

# This code originates from Ulrich Drepper's AM_WITH_NLS.

AC_DEFUN([WGET_WITH_NLS],
  [AC_MSG_CHECKING([whether NLS is requested])
    dnl Default is enabled NLS
    AC_ARG_ENABLE(nls,
      [  --disable-nls           do not use Native Language Support],
      HAVE_NLS=$enableval, HAVE_NLS=yes)
    AC_MSG_RESULT($HAVE_NLS)

    dnl If something goes wrong, we may still decide not to use NLS.
    dnl For this reason, defer AC_SUBST'ing HAVE_NLS until the very
    dnl last moment.

    if test x"$HAVE_NLS" = xyes; then
      dnl If LINGUAS is specified, use only those languages.  In fact,
      dnl compute an intersection of languages in LINGUAS and
      dnl ALL_LINGUAS, and use that.
      if test x"$LINGUAS" != x; then
        new_linguas=
        for lang1 in $ALL_LINGUAS; do
          for lang2 in $LINGUAS; do
            if test "$lang1" = "$lang2"; then
              new_linguas="$new_linguas $lang1"
            fi
          done
        done
        ALL_LINGUAS=$new_linguas
      fi
      AC_MSG_NOTICE([language catalogs: $ALL_LINGUAS])
      AM_PATH_PROG_WITH_TEST(MSGFMT, msgfmt,
	[test -z "`$ac_dir/$ac_word -h 2>&1 | grep 'dv '`"], msgfmt)
      AM_PATH_PROG_WITH_TEST(XGETTEXT, xgettext,
	  [test -z "`$ac_dir/$ac_word -h 2>&1 | grep '(HELP)'`"], :)
      AC_SUBST(MSGFMT)
      AC_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)
      CATOBJEXT=.gmo
      INSTOBJEXT=.mo
      DATADIRNAME=share

      dnl Test whether we really found GNU xgettext.
      if test "$XGETTEXT" != ":"; then
	dnl If it is no GNU xgettext we define it as : so that the
	dnl Makefiles still can work.
	if $XGETTEXT --omit-header /dev/null 2> /dev/null; then
	  : ;
	else
	  AC_MSG_RESULT(
	    [found xgettext programs is not GNU xgettext; ignore it])
	  XGETTEXT=":"
	fi
      fi

      AC_CHECK_HEADERS(libintl.h)

      dnl Prefer gettext found in -lintl to the one in libc.
      dnl Otherwise it can happen that we include libintl.h from
      dnl /usr/local/lib, but fail to specify -lintl, which results in
      dnl link or run-time failures.  (Symptom: libintl_bindtextdomain
      dnl not found at link-time.)

      AC_CHECK_LIB(intl, gettext, [
        dnl gettext is in libintl; announce the fact manually.
        LIBS="-lintl $LIBS"
	AC_DEFINE([HAVE_GETTEXT], 1,
                  [Define if you have the gettext function.])
      ], [
        AC_CHECK_FUNCS(gettext, [], [
          AC_MSG_RESULT([gettext not found; disabling NLS])
          HAVE_NLS=no
        ])
      ])

      for lang in $ALL_LINGUAS; do
	GMOFILES="$GMOFILES $lang.gmo"
	POFILES="$POFILES $lang.po"
      done
      dnl Construct list of names of catalog files to be constructed.
      for lang in $ALL_LINGUAS; do
        CATALOGS="$CATALOGS ${lang}${CATOBJEXT}"
      done

      dnl Make all variables we use known to autoconf.
      AC_SUBST(CATALOGS)
      AC_SUBST(CATOBJEXT)
      AC_SUBST(DATADIRNAME)
      AC_SUBST(GMOFILES)
      AC_SUBST(INSTOBJEXT)
      AC_SUBST(INTLLIBS)
      AC_SUBST(POFILES)
    fi
    AC_SUBST(HAVE_NLS)
    dnl Some independently maintained files, such as po/Makefile.in,
    dnl use `USE_NLS', so support it.
    USE_NLS=$HAVE_NLS
    AC_SUBST(USE_NLS)
    if test "x$HAVE_NLS" = xyes; then
      AC_DEFINE([HAVE_NLS], 1, [Define this if you want the NLS support.])
    fi
  ])

dnl Generate list of files to be processed by xgettext which will
dnl be included in po/Makefile.
dnl
dnl This is not strictly an Autoconf macro, because it is run from
dnl within `config.status' rather than from within configure.  This
dnl is why special rules must be applied for it.
AC_DEFUN([WGET_PROCESS_PO],
  [
   dnl I wonder what the following several lines do...
   if test "x$srcdir" != "x."; then
     if test "x`echo $srcdir | sed 's@/.*@@'`" = "x"; then
       posrcprefix="$srcdir/"
     else
       posrcprefix="../$srcdir/"
     fi
   else
     posrcprefix="../"
   fi
   rm -f po/POTFILES
   dnl Use `echo' rather than AC_MSG_RESULT, because this is run from
   dnl `config.status'.
   echo "generating po/POTFILES from $srcdir/po/POTFILES.in"
   sed -e "/^#/d" -e "/^\$/d" -e "s,.*,	$posrcprefix& \\\\,"  \
       -e "\$s/\(.*\) \\\\/\1/" \
        < $srcdir/po/POTFILES.in > po/POTFILES
   echo "creating po/Makefile"
   sed -e "/POTFILES =/r po/POTFILES" po/Makefile.in > po/Makefile
  ])

# Search path for a program which passes the given test.
# Ulrich Drepper <drepper@cygnus.com>, 1996.
#
# This file may be copied and used freely without restrictions.  It
# can be used in projects which are not available under the GNU Public
# License but which still want to provide support for the GNU gettext
# functionality.  Please note that the actual code is *not* freely
# available.

dnl AM_PATH_PROG_WITH_TEST(VARIABLE, PROG-TO-CHECK-FOR,
dnl   TEST-PERFORMED-ON-FOUND_PROGRAM [, VALUE-IF-NOT-FOUND [, PATH]])
AC_DEFUN([AM_PATH_PROG_WITH_TEST],
[# Extract the first word of "$2", so it can be a program name with args.
set dummy $2; ac_word=[$]2
AC_MSG_CHECKING([for $ac_word])
AC_CACHE_VAL(ac_cv_path_$1,
[case "[$]$1" in
  /*)
  ac_cv_path_$1="[$]$1" # Let the user override the test with a path.
  ;;
  *)
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:"
  for ac_dir in ifelse([$5], , $PATH, [$5]); do
    test -z "$ac_dir" && ac_dir=.
    if test -f $ac_dir/$ac_word; then
      if [$3]; then
	ac_cv_path_$1="$ac_dir/$ac_word"
	break
      fi
    fi
  done
  IFS="$ac_save_ifs"
dnl If no 4th arg is given, leave the cache variable unset,
dnl so AC_PATH_PROGS will keep looking.
ifelse([$4], , , [  test -z "[$]ac_cv_path_$1" && ac_cv_path_$1="$4"
])dnl
  ;;
esac])dnl
$1="$ac_cv_path_$1"
if test -n "[$]$1"; then
  AC_MSG_RESULT([$]$1)
else
  AC_MSG_RESULT(no)
fi
AC_SUBST($1)dnl
])

