dnl
dnl configure.ac --- autoconf input file for gawk
dnl
dnl Copyright (C) 1995-2003 the Free Software Foundation, Inc.
dnl
dnl This file is part of GAWK, the GNU implementation of the
dnl AWK Programming Language.
dnl
dnl GAWK is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl GAWK is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
dnl

dnl Process this file with autoconf to produce a configure script.

dnl *** IMPORTANT ***: Fix version in both AC_INIT and AM_INIT_AUTOMAKE
AC_INIT([GNU Awk], 3.1.3, bug-gawk@gnu.org, gawk)

# This is a hack. Different versions of install on different systems
# are just too different. Chuck it and use install-sh.
#
# If the user supplies $INSTALL, figure they know what they're doing.
#
# With Autoconf 2.5x, this needs to come very early on, but *after*
# the INIT macro. Sigh.

if test "x$INSTALL" = "x"
then
	INSTALL="$srcdir/install-sh -c"
	export INSTALL
fi

AC_PREREQ(2.57)
AC_CONFIG_HEADERS([config.h:configh.in], [cat $srcdir/custom.h >> config.h])
AM_INIT_AUTOMAKE(gawk, 3.1.3)

dnl Additional argument stuff
AC_ARG_ENABLE(portals, [  --enable-portals	Enable /p as path prefix for portals],
	if test "$enableval" = yes
	then
		AC_DEFINE(HAVE_PORTALS, 1, [we have portals on /p on this system])
	fi
)
AC_ARG_WITH(whiny-user-strftime, [  --with-whiny-user-strftime	Force use of included version of strftime for deficient systems],
	if test "$withval" = yes
	then
		AC_DEFINE(USE_INCLUDED_STRFTIME, 1,
			  [force use of our version of strftime])
	fi
)
AC_ARG_ENABLE([lint], [  --disable-lint	Disable gawk lint checking],
	if test "$enableval" = no
	then
		AC_DEFINE(NO_LINT, 1, [disable lint checks])
	fi
)
AC_ARG_ENABLE(switch, [  --enable-switch	Enable switch statements for awk programs],
	if test "$enableval" = yes
	then
		AC_DEFINE(ALLOW_SWITCH, 1, [switch statements are enabled in awk programs])
	fi
)

dnl checks for programs
AC_PROG_EGREP
AC_PROG_YACC
AC_PROG_LN_S
AC_PROG_CC
AC_PROG_CPP
AC_PROG_RANLIB

AC_OBJEXT
AC_EXEEXT

AC_PROG_INSTALL

AC_PROG_MAKE_SET

# This is mainly for my use during testing and development.
# Yes, it's a bit of a hack.
AC_MSG_CHECKING([for special development options])
if test -f $srcdir/.developing
then
	# add other debug flags as appropriate, save GAWKDEBUG for emergencies
	CFLAGS="$CFLAGS -DARRAYDEBUG"
	# turn on compiler warnings if we're doing development
	if test "$GCC" = yes
	then
		CFLAGS="$CFLAGS -Wall"
	fi
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi

AC_SUBST(CFLAGS)

dnl checks for systems
AC_AIX
AC_ISC_POSIX
AC_MINIX
AC_SYS_LARGEFILE
GAWK_AC_AIX_TWEAK
GAWK_AC_LINUX_ALPHA

if test "$ISC" = 1	# will be set by test for ISC
then
dnl	need -D_SYSV3 for ISC
	CFLAGS="$CFLAGS -D_SYSV3"
fi

dnl check for C compiler for automake
AM_PROG_CC_STDC
AM_C_PROTOTYPES

dnl Cygwin doesn't like to get libs with full paths
dnl since that overrides linking against DLLs.
case `(uname) 2> /dev/null` in
*CYGWIN*)
	with_libiconv_prefix=no
	with_libintl_prefix=no
	;;
*)
	;;
esac

dnl initialize GNU gettext
AM_GNU_GETTEXT([no-libtool], [need-ngettext])
AM_GNU_GETTEXT_VERSION(0.12.1)

dnl checks for header files
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
AC_CHECK_HEADERS(fcntl.h limits.h locale.h libintl.h mcheck.h \
	netdb.h netinet/in.h  signum.h stdarg.h string.h \
	sys/param.h sys/socket.h sys/time.h unistd.h \
	termios.h stropts.h wchar.h wctype.h)

if test "$ac_cv_header_string_h" = yes
then
	AC_CHECK_HEADERS(memory.h)
else
	AC_CHECK_HEADERS(strings.h)
fi

dnl checks for typedefs
AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_SIZE_T
AC_TYPE_GETGROUPS
jm_AC_TYPE_LONG_LONG
jm_AC_TYPE_UNSIGNED_LONG_LONG
jm_AC_TYPE_INTMAX_T
jm_AC_TYPE_UINTMAX_T
AC_CHECK_TYPE(ssize_t, int)
AC_EGREP_HEADER([int.*sprintf], stdio.h,
	AC_DEFINE(SPRINTF_RET, int, [return type of sprintf]),
	AC_DEFINE(SPRINTF_RET, char *))
dnl see if time_t is defined in <sys/types.h>
AC_TRY_COMPILE([#include <sys/types.h>],[
	time_t foo;
	foo = 0;
],
	AC_DEFINE(TIME_T_IN_SYS_TYPES_H, 1,
		  [some systems define this type here]))

dnl checks for functions
AC_DEFINE(REGEX_MALLOC, 1, [use malloc instead of alloca in regex.c])
AC_FUNC_VPRINTF
GAWK_AC_FUNC_STRTOD_C89
AC_FUNC_MKTIME
case "$ac_cv_func_working_mktime" in
yes)	AC_DEFINE(HAVE_MKTIME, 1, [we have the mktime function])
	;;
esac

AC_CHECK_LIB(m, fmod)
AC_CHECK_FUNCS(fmod getgrent getgroups grantpt mbrlen mbrtowc memcmp \
		memcpy memset setlocale snprintf strchr strerror \
		strftime strncasecmp strtod system tzset wcrtomb wcscoll)

dnl check for dynamic linking
dnl This is known to be very primitive
AC_CHECK_HEADER(dlfcn.h,
	[AC_DEFINE([DYNAMIC], 1, [dynamic loading is possible])
	if test "$GCC" = yes
	then
		# Add others here as appropriate,
		# one day use GNU libtool.
		if uname | $EGREP -i 'linux|freebsd|cygwin' > /dev/null
		then
			LDFLAGS="$LDFLAGS -export-dynamic"
		fi
	fi

	# Check this separately. Some systems have dlopen
	# in libc. Notably freebsd and cygwin.
	AC_CHECK_LIB(dl, dlopen)
])

dnl check for how to use getpgrp
dnl have to hardwire it for VMS POSIX. Sigh.
dnl ditto for BeOS, OS/2, and MS-DOS.
case `(uname) 2> /dev/null` in
*VMS*|*BeOS*|*OS/2*|*MS-DOS*)
	AC_DEFINE(GETPGRP_VOID, 1,
	  [Define to 1 if the getpgrp function requires zero arguments.])
	;;
*)	AC_FUNC_GETPGRP
	;;
esac

dnl check for sockets
GAWK_AC_LIB_SOCKETS

dnl checks for structure members
AC_STRUCT_ST_BLKSIZE
AC_HEADER_TIME
AC_STRUCT_TM
AC_STRUCT_TIMEZONE

dnl checks for compiler characteristics
AC_C_CHAR_UNSIGNED
AC_C_CONST
AC_C_INLINE
AC_C_STRINGIZE

AC_OUTPUT(Makefile \
	awklib/Makefile \
	doc/Makefile \
	intl/Makefile \
	po/Makefile.in \
	test/Makefile)
