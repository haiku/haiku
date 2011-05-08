dnl $Id: configure.ac,v 1.224 2011/05/02 01:20:49 rlk Exp $
dnl Copyright (C) 2001-2004 Roger Leigh
dnl 
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2, or (at your option)
dnl any later version.
dnl 
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

dnl Process this file with autoconf to produce a configure script.

dnl MAKING RELEASES (a step-by-step guide!)
dnl ===============
dnl
dnl Since the last release:
dnl 1. if only source code (not the interface) has changed, set
dnl      GUTENPRINT_MICRO_VERSION += 1;
dnl      GUTENPRINT_INTERFACE_AGE += 1;
dnl 2. if any functions have been added, removed, or changed, set
dnl      GUTENPRINT_INTERFACE_AGE = 0;
dnl      GUTENPRINT_CURRENT_INTERFACE += 1;
dnl 3. if interfaces have been added, set
dnl      GUTENPRINT_BINARY_AGE += 1;
dnl 4. if interfaces have been removed, set
dnl      GUTENPRINT_BINARY_AGE = 0;
dnl
dnl For more detailed information, see the libtool info documentation.
dnl
pushdef([GUTENPRINT_NAME],              [gutenprint])
pushdef([GUTENPRINT_MAJOR_VERSION],     [5])
pushdef([GUTENPRINT_MINOR_VERSION],     [2])
pushdef([GUTENPRINT_MICRO_VERSION],     [7])
pushdef([GUTENPRINT_EXTRA_VERSION],     [])
pushdef([GUTENPRINT_CURRENT_INTERFACE], [2])
pushdef([GUTENPRINT_INTERFACE_AGE],     [8])
pushdef([GUTENPRINT_BINARY_AGE],        [0])
pushdef([GUTENPRINTUI2_CURRENT_INTERFACE], [1])
pushdef([GUTENPRINTUI2_INTERFACE_AGE],     [0])
pushdef([GUTENPRINTUI2_BINARY_AGE],        [0])
pushdef([GUTENPRINT_VERSION], GUTENPRINT_MAJOR_VERSION.GUTENPRINT_MINOR_VERSION.GUTENPRINT_MICRO_VERSION[]GUTENPRINT_EXTRA_VERSION)

AC_INIT(GUTENPRINT_NAME, GUTENPRINT_VERSION, [gimp-print-devel@lists.sourceforge.net])
AC_PREREQ(2.53)
AC_REVISION($Revision: 1.224 $)
AC_CONFIG_SRCDIR(src/main/print-vars.c)
AC_CONFIG_AUX_DIR(scripts)
STP_INIT
AC_COPYRIGHT(
[
Parts of this configure script come from the 'configure.ac' file, and
are not based on freely-redistributable m4 macros; you can redistribute
them and/or modify them under the terms of the GNU General Public
as published by the Free Software Foundation; either version 2, or (at
your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
])

[GUTENPRINT_MAJOR_VERSION]=GUTENPRINT_MAJOR_VERSION
[GUTENPRINT_MINOR_VERSION]=GUTENPRINT_MINOR_VERSION
[GUTENPRINT_MICRO_VERSION]=GUTENPRINT_MICRO_VERSION
[GUTENPRINT_EXTRA_VERSION]=GUTENPRINT_EXTRA_VERSION
[GUTENPRINT_CURRENT_INTERFACE]=GUTENPRINT_CURRENT_INTERFACE
[GUTENPRINT_INTERFACE_AGE]=GUTENPRINT_INTERFACE_AGE
[GUTENPRINT_BINARY_AGE]=GUTENPRINT_BINARY_AGE
[GUTENPRINT_VERSION]=GUTENPRINT_VERSION
[GUTENPRINTUI2_CURRENT_INTERFACE]=GUTENPRINTUI2_CURRENT_INTERFACE
[GUTENPRINTUI2_INTERFACE_AGE]=GUTENPRINTUI2_INTERFACE_AGE
[GUTENPRINTUI2_BINARY_AGE]=GUTENPRINTUI2_BINARY_AGE
popdef([GUTENPRINT_MAJOR_VERSION])
popdef([GUTENPRINT_MINOR_VERSION])
popdef([GUTENPRINT_MICRO_VERSION])
popdef([GUTENPRINT_EXTRA_VERSION])
popdef([GUTENPRINT_CURRENT_INTERFACE])
popdef([GUTENPRINT_INTERFACE_AGE])
popdef([GUTENPRINT_BINARY_AGE])
popdef([GUTENPRINTUI2_CURRENT_INTERFACE])
popdef([GUTENPRINTUI2_INTERFACE_AGE])
popdef([GUTENPRINTUI2_BINARY_AGE])

AC_SUBST([GUTENPRINT_MAJOR_VERSION])
AC_SUBST([GUTENPRINT_MINOR_VERSION])
AC_SUBST([GUTENPRINT_MICRO_VERSION])
AC_SUBST([GUTENPRINT_CURRENT_INTERFACE])
AC_SUBST([GUTENPRINT_INTERFACE_AGE])
AC_SUBST([GUTENPRINT_BINARY_AGE])
AC_SUBST([GUTENPRINT_VERSION])
AC_SUBST([GUTENPRINTUI2_CURRENT_INTERFACE])
AC_SUBST([GUTENPRINTUI2_INTERFACE_AGE])
AC_SUBST([GUTENPRINTUI2_BINARY_AGE])

dnl Make sure we inherit common compiler options from the environment...
CFLAGS="${CFLAGS:=}"
CPPFLAGS="${CPPFLAGS:=}"
CXXFLAGS="${CXXFLAGS:=}"
LDFLAGS="${LDFLAGS:=}"
LIBS="${LIBS:=}"

dnl Initialize automake stuff.
AM_INIT_AUTOMAKE(1.9 dist-bzip2 no-dist-gzip tar-ustar check-news gnu)
popdef([GUTENPRINT_NAME])
popdef([GUTENPRINT_VERSION])


dnl In the following script, there are a the following variants
dnl of cflags and variables
dnl
dnl *_CFLAGS:   cflags for compiling libraries and example progs
dnl *_LIBS:     libraries for linking programs. ONLY to be used
dnl             to generate ${gutenprint_libs}
dnl *_LIBDEPS:  libraries for linking libraries against
dnl *_libdeps:  library dependencies for pkg-config
dnl *_cflags:   cflags for pkg-config
dnl *_libs:     libs for pkg-config

GUTENPRINT_LIBS="-lgutenprint"
GUTENPRINTUI2_LIBS="-lgutenprintui2"
gutenprint_version=${GUTENPRINT_VERSION}
gutenprintui2_version=${GUTENPRINT_VERSION}

GUTENPRINTUI2_LIBDEPS="${GUTENPRINTUI2_LIBDEPS} \$(GUTENPRINT_LIBS)"

dnl Specify a configuration file.
AM_CONFIG_HEADER([config.h])

dnl Available languages (translations)
ALL_LINGUAS="cs da de el en_GB es fi fr hu it ja nb nl pl pt ru sk sl sv zh_TW"
AC_SUBST(ALL_LINGUAS)

dnl Set up libtool scripts.
dnl Disable static library building to speed up the build
AC_LIBTOOL_DLOPEN
# We need static libraries for CUPS genppd program for now, for bug 865265
# --rlk 20031223
AC_ENABLE_STATIC(yes)
#AC_DISABLE_SHARED
AM_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)
dnl define HAVE_GNU_LD if GNU ld is present
if test "$ac_cv_prog_gnu_ld" = "yes" ; then
  AC_DEFINE(HAVE_GNU_LD,, [Define if GNU ld is present.])
fi

dnl Set up maintainer mode
AM_MAINTAINER_MODE
if test x${USE_MAINTAINER_MODE} = xyes ; then
  MAINTAINER_MODE="true"
  BUILD_TEST="yes"
  AC_DEFINE(HAVE_MAINTAINER_MODE,, [Define if maintainer-mode is to be used.])
fi

dnl Check canonical system/host (a kludge: replace me with something better).
if test x${host_os} = xlinux-gnu ; then
  OSTYPE="linux"
elif test "`echo ${host_os} | sed 's/^\(linux\).*$/\1/'`" = "linux" ; then
  OSTYPE="linux"
elif test "`echo ${host_os} | sed 's/^\(irix\).*$/\1/'`" = "irix" ; then
  OSTYPE="irix"
else
  OSTYPE="${host_os}"
fi
AC_DEFINE_UNQUOTED(OSTYPE, ["${OSTYPE}"], [The operating system to build for])


dnl Command line options.
AC_PATH_PROG(CUPS_CONFIG, cups-config)
if test -z "${CUPS_CONFIG}" ; then
  BUILD_CUPS="no"
else
  BUILD_CUPS="yes"
fi
STP_ARG_WITH_DETAILED(
  [cups],
  [=PREFIX],
  [whether to build CUPS driver],
  [(default PREFIX=/usr)],
  [BUILD_CUPS],
  [(automatic)],
  [
    yes) [BUILD_CUPS]="yes" ; AC_MSG_RESULT([yes]) ; cups_prefix="/usr" ;;
    no) [BUILD_CUPS]="no" ; AC_MSG_RESULT([no]) ; cups_prefix="/usr" ;;
    *) if test -d ${withval} ; then
         [BUILD_CUPS]="yes"
	 AC_MSG_RESULT([yes (using specified prefix)])
         cups_prefix="${withval}"
       else
         AC_MSG_RESULT([unknown])
         AC_MSG_ERROR([${withval}: no such directory for --with-cups])
       fi ;;
  ],[
     cups_prefix="${prefix}"
  ])

dnl GNU gettext checks.
AM_GNU_GETTEXT_VERSION([0.16])
AM_GNU_GETTEXT([external])

AM_CONDITIONAL(USE_NLS, test x${USE_NLS} = xyes)

STP_ARG_ENABLE([nls-macosx],
               [build CUPS PPD files],
               [USE_NLS_OSX],
               [(automatic)])

USE_LEVEL3_PS=no
BUILD_CUPS_PPDS="${BUILD_CUPS}"
BUILD_TRANSLATED_CUPS_PPDS=no
BUILD_CUPS_1_2="no"
if test -n "${CUPS_CONFIG}" -a "${BUILD_CUPS}" = "yes" ; then
  cups_version=`${CUPS_CONFIG} --version | sed -e '1,$s/svn$/.0/'`
dnl Quote to protect sed regexes
  [cups_major=`echo $cups_version | sed -e 's/\([0-9][0-9]*\).\([0-9][0-9]*\).\([0-9][0-9]*\).*/\1/'`]
  [cups_minor=`echo $cups_version | sed -e 's/\([0-9][0-9]*\).\([0-9][0-9]*\).\([0-9][0-9]*\).*/\2/'`]
  [cups_point=`echo $cups_version | sed -e 's/\([0-9][0-9]*\).\([0-9][0-9]*\).\([0-9][0-9]*\).*/\3/'`]
dnl There was no CUPS 1.0.15 or the like
  if test "$cups_major" -gt 1 -o "$cups_minor" -gt 1 -o "$cups_point" -gt 14 ; then
    USE_LEVEL3_PS=yes
  fi
  if test "$cups_major" -gt 1 -o "$cups_minor" -gt 1 ; then
    BUILD_CUPS_PPDS="no"
    BUILD_CUPS_1_2="yes"
  fi
fi

STP_ARG_ENABLE([cups-ppds],
               [build CUPS PPD files],
               [BUILD_CUPS_PPDS],
               [(automatic)])

STP_ARG_ENABLE([cups-1_2-enhancements],
               [build CUPS 1.2 enhancements],
               [BUILD_CUPS_1_2],
               [(automatic)])

if test "$USE_NLS" != no -a "$BUILD_CUPS_PPDS" = yes ; then
   BUILD_TRANSLATED_CUPS_PPDS="yes"
fi

STP_ARG_ENABLE([translated-cups-ppds],
               [build translated CUPS PPD files],
               [BUILD_TRANSLATED_CUPS_PPDS],
               [(automatic)])

if test "$BUILD_CUPS_1_2" = yes -a "$BUILD_TRANSLATED_CUPS_PPDS" = yes ; then
   BUILD_GLOBALIZED_CUPS_PPDS=yes
else
   BUILD_GLOBALIZED_CUPS_PPDS=no
fi

STP_ARG_ENABLE([globalized-cups-ppds],
               [build globalized CUPS PPD files],
               [BUILD_GLOBALIZED_CUPS_PPDS],
               [(automatic)])

STP_ARG_ENABLE_DETAILED([simplified-cups-ppds],
			[],
			[build simplified CUPS PPD files],
			[(options: yes, no, only)],
			[BUILD_SIMPLIFIED_CUPS_PPDS],
			[no],
			[
    yes) [WHICH_PPDS]="-a"; BUILD_SIMPLIFIED_CUPS_PPDS=yes; AC_MSG_RESULT([yes]) ;;
    no) [WHICH_PPDS]=""; BUILD_SIMPLIFIED_CUPS_PPDS=no; AC_MSG_RESULT([no]) ;;
    only) [WHICH_PPDS]="-s"; BUILD_SIMPLIFIED_CUPS_PPDS=only; AC_MSG_RESULT([only]) ;;
    *) AC_MSG_ERROR([${enableval}: not a valid option ('yes', 'no', or 'only')]) ;;
				   ])

STP_ARG_ENABLE([cups-ppds-at-top-level],
	       [place CUPS PPD files at top level],
	       [CUPS_PPDS_AT_TOP_LEVEL],
	       [no])

STP_ARG_ENABLE([cups-level3-ppds],
               [generate PostScript level 3 PPD files],
               [USE_LEVEL3_PS],
               [(automatic)])

STATIC_GENPPD="$enable_static"
STP_ARG_ENABLE([static-genppd],
               [build a statically-linked cups-genppd and rastertogutenprint.  WARNING: Please read the README and NEWS (release notes) CAREFULLY before using --disable-static-genppd!],
	       [STATIC_GENPPD],
	       [(automatic)])

AC_PATH_PROG(FOOMATIC_CONFIGURE, foomatic-configure)
if test -z "${FOOMATIC_CONFIGURE}" ; then
  BUILD_FOOMATIC="no"
else
  BUILD_FOOMATIC="yes"
fi
STP_ARG_WITH([foomatic],
             [build foomatic data files],
             [BUILD_FOOMATIC],
             [(automatic)])


AC_PATH_PROG(FOOMATIC_PPDFILE, foomatic-ppdfile)
if test -z "${FOOMATIC_PPDFILE}" ; then
  BUILD_FOOMATIC_3="no"
else
  BUILD_FOOMATIC_3="yes"
fi
STP_ARG_WITH([foomatic3],
             [make use of foomatic 3 extra features],
             [BUILD_FOOMATIC_3],
             [(automatic)])

AC_PATH_PROG(IJS_CONFIG, ijs-config)

if test -z "${IJS_CONFIG}"  ; then
  BUILD_GHOSTSCRIPT="no"
else
  BUILD_GHOSTSCRIPT="yes"
fi
STP_ARG_WITH([ghostscript],
             [build Ghostscript driver],
             [BUILD_GHOSTSCRIPT],
             [(automatic)])

STP_ARG_WITH_DETAILED([modules],
                      [],
                      [enable loadable modules],
	              [(options: ltdl, dlopen, static)],
	              [WITH_MODULES],
	              [(automatic)],[
    yes) [WITH_MODULES]="yes";  AC_MSG_RESULT([yes]) ;;
    no) [WITH_MODULES]="no";  AC_MSG_RESULT([no]) ;;
    ltdl) [WITH_MODULES]="ltdl";  AC_MSG_RESULT([ltdl]) ;;
    dlopen) [WITH_MODULES]="dlopen";  AC_MSG_RESULT([dlopen]) ;;
    static) [WITH_MODULES]="static";  AC_MSG_RESULT([static]) ;;
    *) AC_MSG_ERROR([${withval}: not a valid module loader]) ;;
                      ],[
    # if static libraries are enabled, modules must be static
    if test x$enable_static = xyes; then
      [WITH_MODULES]="static"
    else
      [WITH_MODULES]="yes"]
    fi)

# Abort if a broken configuration was required
if test x$enable_static = xyes; then
  if test $WITH_MODULES = no; then
    :
  elif test $WITH_MODULES = static; then
    :
  else
    AC_MSG_ERROR([--disable-static must be used with --with-modules])
  fi
fi 

STP_ARG_ENABLE([debug],
               [turn on debugging in build],
               [ENABLE_DEBUG],
               [no])

STP_ARG_ENABLE([profile],
               [turn on profiling in build],
               [ENABLE_PROFILE],
               [no])

STP_ARG_WITH_DETAILED([readline], ,
                      [use readline],
                      [(default tries -lncurses, -lcurses, -ltermcap)],
                      [USE_READLINE],
                      [yes],[
    yes) [USE_READLINE]="yes" ; AC_MSG_RESULT([yes]) ;;
    no) [USE_READLINE]="no" ; AC_MSG_RESULT([no]) ;;
    only) [USE_READLINE]="yes"
          [EXTRA_LIBREADLINE_DEPS]=" "
          AC_MSG_RESULT([yes (using only readline)]) ;;
    *) [USE_READLINE]="yes"
       [EXTRA_LIBREADLINE_DEPS]="${withval}"
       AC_MSG_RESULT([yes (using extra libraries ${withval})]) ;;
             ])
dnl if test -z "${[EXTRA_LIBREADLINE_DEPS]}" ; then
dnl   [EXTRA_LIBREADLINE_DEPS]=""
dnl fi

AC_PATH_PROGS([GIMPTOOL2_CHECK], [gimptool-2.0 gimptool])
if test -z "${GIMPTOOL2_CHECK}" ; then
  BUILD_GIMP2="no"
else
  # Check we have version 2.0
  gimptool_check_version=`${GIMPTOOL2_CHECK} --version`
dnl Quote to protect sed regexes
  [gimptool_check_major=`echo "$gimptool_check_version" | sed -e 's/\([0-9][0-9]*\).\([0-9][0-9]*\).\([0-9][0-9]*\).*/\1/'`]
  [gimptool_check_minor=`echo "$gimptool_check_version" | sed -e 's/\([0-9][0-9]*\).\([0-9][0-9]*\).\([0-9][0-9]*\).*/\2/'`]
  if test "$gimptool_check_major" -eq 2; then
    BUILD_GIMP2="yes"
    if test "$gimptool_check_minor" -ge 4 ; then
      BUILD_GIMP2_AS_GUTENPRINT="yes"
    else
      BUILD_GIMP2_AS_GUTENPRINT="no"
    fi
  else
    BUILD_GIMP2="no"
    BUILD_GIMP2_AS_GUTENPRINT="no"
  fi
fi

PKG_CHECK_MODULES([GTK2], [gtk+-2.0],
                  [HAVE_GTK2=yes
                   BUILD_LIBGUTENPRINTUI2=yes
                   GUTENPRINTUI2_LIBDEPS="${GUTENPRINTUI2_LIBDEPS} ${GTK2_LIBS}"],
                  [HAVE_GTK2=no
	           BUILD_LIBGUTENPRINTUI2=no])
# Require both gimptool and pkg-config to succeed
if test x$BUILD_GIMP2 = xyes; then
  PKG_CHECK_MODULES([GIMP2], [gimpui-2.0], [BUILD_GIMP2=yes], [BUILD_GIMP2=no])
fi

STP_ARG_WITH([gimp2],
             [build GIMP 2.x Print plugin],
             [BUILD_GIMP2],
             [(automatic)])

STP_ARG_WITH([gimp2-as-gutenprint],
             [name GIMP 2.x Print plugin gutenprint (requires GIMP 2.2; normally used with GIMP 2.4 or above)],
             [BUILD_GIMP2_AS_GUTENPRINT],
             [(automatic)])

STP_ARG_ENABLE([libgutenprintui2],
               [build Gutenprint GTK+-2.0 user interface library],
               [BUILD_LIBGUTENPRINTUI2],
               [(automatic)])

STP_ARG_ENABLE([samples],
               [install sample images],
               [INSTALL_SAMPLES],
               [yes])

STP_ARG_ENABLE([escputil],
               [build escputil],
               [BUILD_ESCPUTIL],
               [yes])

STP_ARG_ENABLE([test],
               [build test programs],
               [BUILD_TEST],
               [yes])

if test "$BUILD_TEST" = "yes" ; then
  BUILD_TESTPATTERN="yes"
else
  BUILD_TESTPATTERN="no"
fi

STP_ARG_ENABLE([testpattern],
               [build testpattern generator],
               [BUILD_TESTPATTERN],
               [(automatic)])

STP_ARG_ENABLE([user-install],
               [install GIMP plugin in home directory],
               [USER_INSTALL],
               [no])
if test x$USER_INSTALL = xyes ; then
  PLUG_IN_PATH="bin"
else
  PLUG_IN_PATH="admin-bin"
fi

dnl aclocal flags.
dnl AM_ACLOCAL_INCLUDE(m4 m4local)

dnl Checks for programs.
AM_ICONV_LINK
AC_ISC_POSIX
AC_PROG_AWK
AC_PROG_CC
AM_PROG_CC_STDC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PROG_LEX
AC_PROG_YACC
AC_PATH_PROG([TEST], [test])
AC_PATH_PROG([TRUE], [true])
AC_PATH_PROG([FIND], [find])
AC_PATH_PROG([PERL], [perl])
AC_PATH_PROG([GLIB_MKENUMS], [glib-mkenums])
AC_PATH_PROG([CONVERT], [convert])
if test x${USE_MAINTAINER_MODE} = xyes ; then
  if test x${CONVERT} = x ; then
    AC_MSG_WARN([PostScript developer's guide cannot be regenerated!])
  fi
fi
AC_PATH_PROG(TEXI2HTML, texi2html)
if test x${USE_MAINTAINER_MODE} = xyes ; then
  if test x${TEXI2HTML} = x ; then
    AC_MSG_WARN([HTML documentation cannot be regenerated!])
  fi
fi
AC_PATH_PROG(DVIPS, dvips)
if test x${USE_MAINTAINER_MODE} = xyes ; then
  if test x${DVIPS} = x ; then
    AC_MSG_WARN([PostScript documentation cannot be regenerated!])
  fi
fi
AC_PATH_PROG(DB2PDF, db2pdf)
if test x${USE_MAINTAINER_MODE} = xyes ; then
  if test x${DB2PDF} = x ; then
    AC_MSG_WARN([PDF developer's guide cannot be regenerated!])
  fi
fi
AC_PATH_PROGS(DVIPDF, dvipdf dvipdfm)
if test x${USE_MAINTAINER_MODE} = xyes ; then
  if test x${DVIPDF} = x ; then
    AC_MSG_WARN([PDF documentation cannot be regenerated!])
  fi
fi
AC_PATH_PROG(DB2PS, db2ps)
if test x${USE_MAINTAINER_MODE} = xyes ; then
  if test x${DB2PS} = x ; then
    AC_MSG_WARN([PostScript developer's guide cannot be regenerated!])
  fi
fi
AC_PATH_PROG(DB2HTML, db2html)
if test x${USE_MAINTAINER_MODE} = xyes ; then
  if test x${DB2HTML} = x ; then
    AC_MSG_WARN([HTML developer's guide cannot be regenerated!])
  fi
fi
AC_PATH_PROG(DOXYGEN, doxygen)
if test x${USE_MAINTAINER_MODE} = xyes ; then
  if test x${DOXYGEN} = x ; then
    AC_MSG_WARN([HTML developer's guide cannot be regenerated!])
  fi
fi

dnl CUPS checks
STP_CUPS_PROGS

dnl foomatic checks
if test x${BUILD_FOOMATIC} = xyes ; then
  PATH_SBIN=$PATH:/usr/sbin:/usr/local/sbin
  AC_PATH_PROG(FOOMATIC_KITLOAD, foomatic-kitload, , [$PATH_SBIN])
  if test -z "$FOOMATIC_KITLOAD" ; then
    AC_MSG_WARN(Cannot find foomatic-kitload; attempt to install foomatic will fail)
  fi
fi

dnl ijs checks
if test x${BUILD_GHOSTSCRIPT} = xyes ; then
  if test -z "$IJS_CONFIG" ; then
    AC_MSG_ERROR(Cannot find ijs-config; please ensure Ghostscript 6.53 or above is installed);
  fi
  IJS_CFLAGS=`$IJS_CONFIG --cflags`
  IJS_LIBS=`$IJS_CONFIG --libs`
fi

GUTENPRINT_RELEASE_VERSION=${GUTENPRINT_MAJOR_VERSION}.${GUTENPRINT_MINOR_VERSION}
AH_TEMPLATE(GUTENPRINT_RELEASE_VERSION,, [Gutenprint release version])
AC_DEFINE_UNQUOTED(GUTENPRINT_RELEASE_VERSION, "${GUTENPRINT_RELEASE_VERSION}")

dnl Set PACKAGE_LOCALE_DIR in config.h
AH_TEMPLATE(PACKAGE_LOCALE_DIR,, [Package locale directory])
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, ["${ac_default_prefix}/share/locale"])
else
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, ["${prefix}/share/locale"])
fi

dnl Set PACKAGE_DATA_DIR in config.h.
AH_TEMPLATE(PACKAGE_DATA_DIR,, [Package data directory])
if test "x${datadir}" = 'x${prefix}/share'; then
  if test "x${prefix}" = "xNONE"; then
    PACKAGE_DATA_DIR="${ac_default_prefix}/share/${PACKAGE}"
  else
    PACKAGE_DATA_DIR="${prefix}/share/${PACKAGE}"
  fi
elif test "x${datadir}" = 'x${datarootdir}'; then
  if test "x${datarootdir}" = 'x${prefix}/share'; then
    if test "x${prefix}" = "xNONE"; then
      PACKAGE_DATA_DIR="${ac_default_prefix}/share/${PACKAGE}"
    else
      PACKAGE_DATA_DIR="${prefix}/share/${PACKAGE}"
    fi
  else
    PACKAGE_DATA_DIR="${datarootdir}/${PACKAGE}"
  fi
else
  PACKAGE_DATA_DIR="${datadir}/${PACKAGE}"
fi

AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${PACKAGE_DATA_DIR}")

dnl Set PACKAGE_LIB_DIR in config.h.
AH_TEMPLATE(PACKAGE_LIB_DIR,, [Package lib directory])
if test "x${libdir}" = 'x${exec_prefix}/lib'; then
  if test "x${exec_prefix}" = "xNONE"; then
    if test "x${prefix}" = "xNONE"; then
      PACKAGE_LIB_DIR="${ac_default_prefix}/lib/${PACKAGE}"
    else
      PACKAGE_LIB_DIR="${prefix}/lib/${PACKAGE}"
    fi
  else
    PACKAGE_LIB_DIR="${exec_prefix}/lib/${PACKAGE}"
  fi
else
  PACKAGE_LIB_DIR="${libdir}/${PACKAGE}"
fi

AC_DEFINE_UNQUOTED(PACKAGE_LIB_DIR, "${PACKAGE_LIB_DIR}")

AH_TEMPLATE(PKGXMLDATADIR,, [Package XML data directory])
PKGXMLDATADIR="${PACKAGE_DATA_DIR}/${GUTENPRINT_RELEASE_VERSION}/xml"
AC_DEFINE_UNQUOTED(PKGXMLDATADIR, ["$PKGXMLDATADIR"], )

AH_TEMPLATE(PKGMODULEDIR,, [Package module directory])
PKGMODULEDIR="${PACKAGE_LIB_DIR}/${GUTENPRINT_RELEASE_VERSION}/modules"
AC_DEFINE_UNQUOTED(PKGMODULEDIR, ["$PKGMODULEDIR"])

dnl Compiler flags
if test x$ac_compiler_gnu = "xyes"; then
  STP_ADD_COMPILER_ARGS([-Wall -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Wwrite-strings -Werror-implicit-function-declaration -Winline -Wformat=2 -finline-limit=131072],, [GNUCFLAGS])
  if test x${USE_MAINTAINER_MODE} = xyes ; then
    STP_ADD_COMPILER_ARGS([-pedantic -Waggregate-return -Wcast-qual -Wshadow -Wredundant-decls],, [GNUCFLAGS])
  fi
  if test x$ENABLE_DEBUG = xyes ; then
    STP_ADD_COMPILER_ARG([-g])
  else
    STP_ADD_FIRST_COMPILER_ARG([-O6 -O3 -O2 -O1 -O])
  fi
else
  if test x$ENABLE_DEBUG = xyes ; then
    STP_ADD_COMPILER_ARG([-g])
  else
    STP_ADD_FIRST_COMPILER_ARG([-O])
  fi
fi
if test x$ENABLE_PROFILE = xyes ; then
  STP_ADD_COMPILER_ARG([-pg])
fi
AC_SUBST(GNUCFLAGS)

AH_TEMPLATE([HAVE_GCC_ATTRIBUTES],
            [Define to 1 if GCC special attributes are supported])
AC_MSG_CHECKING([if $CC supports __attribute__ syntax])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([typedef struct __attribute__((__packed__)) {
   unsigned char foo;
   char bar;
   long baz;
} test_struct_t;],
                                   [test_struct_t mystruct;])],
                  [AC_MSG_RESULT([yes])]
                   AC_DEFINE([HAVE_GCC_ATTRIBUTES], 1),
                  [AC_MSG_RESULT([no])])

AH_VERBATIM([HAVE_GCC_ATTRIBUTES_BOILERPLATE],
[#if !defined(HAVE_GCC_ATTRIBUTES) && !defined(__attribute__)
/* This should really be a C99 anonymous variadic macro. */
#define __attribute__(attr)
#endif])

dnl Checks for libraries.
dnl When checking readline, check using extra libraries first.
dnl We want to protect against the link somehow succeeding, but only
dnl failing at runtime, as seems to happen on some BSD systems.
AH_TEMPLATE(HAVE_LIBREADLINE, [Is libreadline present?])
if test "$USE_READLINE" = yes ; then
  if test "$EXTRA_LIBREADLINE_DEPS" = "" ; then
    unset ac_cv_lib_readline_readline
    AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
		 HAVE_LIBREADLINE=false, -lncurses)
    if test x${HAVE_LIBREADLINE} = xtrue ; then
      echo "  Using -lreadline -lncurses"
      EXTRA_LIBREADLINE_DEPS=-lncurses
      AC_DEFINE(HAVE_LIBREADLINE, [1])
    else
      unset ac_cv_lib_readline_readline
      AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
		   HAVE_LIBREADLINE=false, -lcurses)
      if test x${HAVE_LIBREADLINE} = xtrue ; then
	echo "  Using -lreadline -lcurses"
	EXTRA_LIBREADLINE_DEPS=-lcurses
	AC_DEFINE(HAVE_LIBREADLINE, [1])
      else
	unset ac_cv_lib_readline_readline
	AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
		     HAVE_LIBREADLINE=false, -ltermcap)
	if test x${HAVE_LIBREADLINE} = xtrue ; then
	  echo "  Using -lreadline -ltermcap"
	  EXTRA_LIBREADLINE_DEPS=-ltermcap
	  AC_DEFINE(HAVE_LIBREADLINE, [1])
	else
	  unset ac_cv_lib_readline_readline
	  AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
		       HAVE_LIBREADLINE=false)
	  if test x${HAVE_LIBREADLINE} = xtrue ; then
	    AC_DEFINE(HAVE_LIBREADLINE, [1])
	  else
	    AC_DEFINE(HAVE_LIBREADLINE, [0])
	  fi
	fi
      fi
    fi
  else
    unset ac_cv_lib_readline_readline
    AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
		 HAVE_LIBREADLINE=false, ${EXTRA_LIBREADLINE_DEPS})
    if test x${HAVE_LIBREADLINE} = xtrue ; then
      echo "  Using -lreadline ${EXTRA_LIBREADLINE_DEPS}"
      AC_DEFINE(HAVE_LIBREADLINE, [1])
    else
      AC_DEFINE(HAVE_LIBREADLINE, [0])
    fi
  fi
else
  HAVE_LIBREADLINE=false
  AC_DEFINE(HAVE_LIBREADLINE, [0])
fi

dnl libltdl, used for dynamic module loading
AC_CHECK_LIB(ltdl, lt_dlopen, [LTDL_LIBS="-lltdl"])

AC_CHECK_LIB(dl, dlopen, [DLOPEN_LIBS="-ldl"])

AC_CHECK_LIB(m,pow,
             GUTENPRINT_LIBDEPS="${GUTENPRINT_LIBDEPS} -lm"
             gutenprint_libdeps="${gutenprint_libdeps} -lm")

STP_CUPS_LIBS

STP_GIMP2_LIBS

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(getopt.h)
AC_CHECK_HEADER(readline/readline.h,
  HAVE_READLINE_READLINE_H=true,
  HAVE_READLINE_READLINE_H=false)
if test x${HAVE_READLINE_READLINE_H} = xtrue ; then
  AC_DEFINE(HAVE_READLINE_READLINE_H,, [Define if libreadline header is present.])
  if test x${HAVE_LIBREADLINE} = xtrue ; then
    LIBREADLINE_DEPS="-lreadline $EXTRA_LIBREADLINE_DEPS"
  fi
fi
AC_CHECK_HEADERS(dlfcn.h, [HAVE_DLFCN_H=true])
AC_CHECK_HEADERS(fcntl.h)
AC_CHECK_HEADERS(limits.h)
AC_CHECK_HEADERS(locale.h)
AC_CHECK_HEADERS(ltdl.h, [HAVE_LTDL_H=true])
AC_CHECK_HEADERS(stdarg.h stdlib.h string.h)
AC_CHECK_HEADERS(sys/time.h sys/types.h)
AC_CHECK_HEADERS(time.h)
AC_CHECK_HEADERS(unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SIGNAL

dnl Checks for library functions.
AC_CHECK_FUNCS([nanosleep poll usleep])
AC_CHECK_FUNCS([getopt_long])

dnl finite() is non-standard, isfinite() is ISO-standard, figure out
dnl which to use...
AC_SEARCH_LIBS(finite, m, CFLAGS="-Disfinite=finite $CFLAGS")

dnl Define what has to be built
AM_CONDITIONAL(BUILD_FOOMATIC, test x${BUILD_FOOMATIC} = xyes)

AM_CONDITIONAL(BUILD_FOOMATIC_3, test x${BUILD_FOOMATIC_3} = xyes)

AM_CONDITIONAL(BUILD_GHOSTSCRIPT, test x${BUILD_GHOSTSCRIPT} = xyes)

AM_CONDITIONAL(BUILD_CUPS, test x${BUILD_CUPS} = xyes)

AM_CONDITIONAL(BUILD_CUPS_1_2, test x${BUILD_CUPS_1_2} = xyes)

AM_CONDITIONAL(BUILD_CUPS_PPDS, test x${BUILD_CUPS_PPDS} = xyes)

AM_CONDITIONAL(CUPS_PPDS_AT_TOP_LEVEL, test x${CUPS_PPDS_AT_TOP_LEVEL} = xyes)

AM_CONDITIONAL(BUILD_TRANSLATED_CUPS_PPDS, test x${BUILD_TRANSLATED_CUPS_PPDS} = xyes )

AM_CONDITIONAL(BUILD_GLOBALIZED_CUPS_PPDS, test x${BUILD_GLOBALIZED_CUPS_PPDS} = xyes )

AM_CONDITIONAL(BUILD_SIMPLIFIED_CUPS_PPDS, test x${BUILD_SIMPLIFIED_CUPS_PPDS} = xyes)

if test x${USE_LEVEL3_PS} = xno ; then
  CUPS_PPD_PS_LEVEL=2
else
  CUPS_PPD_PS_LEVEL=3
fi

AM_CONDITIONAL(USE_LEVEL3_PS, test x${USE_LEVEL3_PS} = xyes)

AM_CONDITIONAL(BUILD_ESCPUTIL, test x${BUILD_ESCPUTIL} = xyes)

AM_CONDITIONAL(BUILD_TEST, test x${BUILD_TEST} = xyes)

AM_CONDITIONAL(BUILD_TESTPATTERN, test x${BUILD_TESTPATTERN} = xyes)

AM_CONDITIONAL(BUILD_LIBGUTENPRINTUI2, test x${BUILD_LIBGUTENPRINTUI2} = xyes)

AM_CONDITIONAL(BUILD_GIMP2, test x${BUILD_GIMP2} = xyes)

AM_CONDITIONAL(BUILD_GIMP2_AS_GUTENPRINT, test x${BUILD_GIMP2_AS_GUTENPRINT} = xyes)

AM_CONDITIONAL(INSTALL_SAMPLES, test x${INSTALL_SAMPLES} = xyes)

AM_CONDITIONAL(INSTALL_USER_GUIDE, test x${INSTALL_USER_GUIDE} = xyes)

STP_CUPS_PATH

# loadable modules
# portable sh has no AND test...
if test -n "$HAVE_LTDL_H"; then
  if test -n "$LTDL_LIBS"; then
    LTDL_POSSIBLE="true"
  fi
fi
if test -n "$HAVE_DLFCN_H"; then
  if test -n "$DLOPEN_LIBS"; then
    DLOPEN_POSSIBLE="true"
  fi
fi
# define what the user chose to build, if possible.
# Prefer dlopen over ltdl.
if test x$WITH_MODULES = xyes; then
  if test x$DLOPEN_POSSIBLE = xtrue; then
    USE_DLOPEN="true"
  elif test x$LDTL_POSSIBLE = xtrue; then
    USE_LTDL="true"
  fi
elif test x$WITH_MODULES = xltdl; then
  if test x$LTDL_POSSIBLE = xtrue; then
    USE_LTDL="true"
  fi
elif test x$WITH_MODULES = xdlopen; then
  if test x$DLOPEN_POSSIBLE = xtrue; then
    USE_DLOPEN="true"
  fi
fi
# define what module system is to be used
AC_MSG_CHECKING([which module system will be used])
if test x$USE_LTDL = xtrue; then
  MODULE="true"
  AC_DEFINE(MODULE, "1", [Build a modular libgutenprint])
  AC_DEFINE(USE_LTDL, "1", [Use GNU libltdl as module loader])
  GUTENPRINT_LIBDEPS="${GUTENPRINT_LIBDEPS} ${LTDL_LIBS}"
  gutenprint_libdeps="${gutenprint_libdeps} ${LTDL_LIBS}"
  AC_MSG_RESULT([ltdl])
elif test x$USE_DLOPEN = xtrue; then
  MODULE="true"
  AC_DEFINE(MODULE, "1", [Build a modular libgutenprint])
  AC_DEFINE(USE_DLOPEN, "1", [Use libdl/dlopen as module loader])
  GUTENPRINT_LIBDEPS="${GUTENPRINT_LIBDEPS} ${DLOPEN_LIBS}"
  gutenprint_libdeps="${gutenprint_libdeps} ${DLOPEN_LIBS}"
  AC_MSG_RESULT([dlopen])
else
  MODULE="false"
  AC_MSG_RESULT([static])
fi
AM_CONDITIONAL(BUILD_MODULES, test x$MODULE = xtrue)

AM_CONDITIONAL(BUILD_GENPPD_STATIC, test x"$STATIC_GENPPD" = xyes)

dnl Define LTLIBOBJS
AC_CONFIG_COMMANDS_PRE(
[LTLIBOBJS=`echo "$LIB@&t@OBJS" | sed 's/\.o/.lo/g'`
AC_SUBST(LTLIBOBJS)
LTALLOCA=`echo "$ALLOCA" | sed 's/\.o/.lo/g'`
AC_SUBST(LTALLOCA)])

dnl Definitions
AC_DEFINE_UNQUOTED(CUPS_DATADIR, "${cups_conf_datadir}", [CUPS data directory.])

AH_TEMPLATE(CUPS_MODELDIR,, [CUPS PPD file directory])
if test "${CUPS_PPDS_AT_TOP_LEVEL}" = yes ; then
  AC_DEFINE_UNQUOTED(CUPS_MODELDIR, "${cups_conf_datadir}/model/")
else
  AC_DEFINE_UNQUOTED(CUPS_MODELDIR, "${cups_conf_datadir}/model/${PACKAGE}/${GUTENPRINT_RELEASE_VERSION}/")
fi

AC_ARG_WITH(cups_nickname, [  --with-cups-nickname="nickname suffix"
                          Suffix to add to PPD NickName, default=" - CUPS+Gutenprint v"],
  CUPS_PPD_NICKNAME_STRING="$withval",
  CUPS_PPD_NICKNAME_STRING=" - CUPS+Gutenprint v")

AH_TEMPLATE(CUPS_PPD_NICKNAME_STRING,, [CUPS PPD Nickname string])
AC_DEFINE_UNQUOTED(CUPS_PPD_NICKNAME_STRING, "$CUPS_PPD_NICKNAME_STRING")


AC_DEFINE_UNQUOTED(CUPS_PPD_PS_LEVEL, ${CUPS_PPD_PS_LEVEL}, [CUPS PPD PostScript level])
STP_RELEASE_DATE

dnl Some versions of make get very grouchy about $(RM)
RM='rm'

dnl Substitutions
AC_SUBST(cups_prefix)
AC_SUBST(cups_exec_prefix)
AC_SUBST(cups_bindir)
AC_SUBST(cups_sbindir)
AC_SUBST(cups_conf_datadir)
AC_SUBST(cups_conf_serverbin)
AC_SUBST(cups_conf_serverroot)
AC_SUBST(CUPS_CFLAGS)
AC_SUBST(CUPS_LIBS)
AC_SUBST(CUPS_PPD_PS_LEVEL)
AC_SUBST(FOOMATIC_KITLOAD)
AC_SUBST(GENPPD_LIBS)
AC_SUBST(IJS_CFLAGS)
AC_SUBST(IJS_LIBS)
AC_SUBST(RM)
AC_SUBST(GIMP2_CFLAGS)
AC_SUBST(GIMP2_LIBS)
AC_SUBST(gimp2_plug_indir)
AC_SUBST(GUTENPRINT_CFLAGS)
AC_SUBST(GUTENPRINT_LIBS)
AC_SUBST(GUTENPRINT_LIBDEPS)
AC_SUBST(GUTENPRINT_VERSION)
AC_SUBST(GUTENPRINTUI2_CFLAGS)
AC_SUBST(GUTENPRINTUI2_LIBS)
AC_SUBST(GUTENPRINTUI2_LIBDEPS)
AC_SUBST(GUTENPRINTUI2_VERSION)
AC_SUBST(gutenprint_cflags)
gutenprint_libs="${GUTENPRINT_LIBS} ${gutenprint_libdeps}"
AC_SUBST(gutenprint_libs)
AC_SUBST(gutenprint_libdeps)
AC_SUBST(gutenprintui2_cflags)
gutenprintui2_libs="${GUTENPRINTUI2_LIBS} ${gutenprintui2_libdeps}"
AC_SUBST(gutenprintui2_libs)
AC_SUBST(gutenprintui2_libdeps)
AC_SUBST(LIBREADLINE_DEPS)
AC_SUBST(MAINTAINER_CFLAGS)
AC_SUBST(PLUG_IN_PATH)
AC_SUBST(WHICH_PPDS)
AC_SUBST(BUILD_CUPS_PPDS)

dnl Optional architecture options to be applied after all autoconf tests
dnl are done...
AC_ARG_WITH(archflags, [  --with-archflags="..."  additional options for CFLAGS and LDFLAGS],
    CFLAGS="$CFLAGS $withval"
    LDFLAGS="$LDFLAGS $withval")

dnl Output files

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([doc/gutenprint.dox])
AC_CONFIG_FILES([doc/gutenprintui2.dox])
AC_CONFIG_FILES([doc/developer/Makefile])
AC_CONFIG_FILES([include/Makefile])
AC_CONFIG_FILES([include/gutenprint/Makefile])
AC_CONFIG_FILES([include/gutenprintui2/Makefile])
AC_CONFIG_FILES([include/gutenprint/gutenprint-version.h])
AC_CONFIG_FILES([man/Makefile])
AC_CONFIG_FILES([man/cups-calibrate.8])
AC_CONFIG_FILES([man/cups-genppd.8])
AC_CONFIG_FILES([man/cups-genppdupdate.8])
AC_CONFIG_FILES([man/escputil.1])
AC_CONFIG_FILES([man/ijsgutenprint.1])
AC_CONFIG_FILES([po/Makefile.in])
AC_CONFIG_FILES([samples/Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/cups/Makefile])
AC_CONFIG_FILES([src/cups/Info.plist])
AC_CONFIG_FILES([src/cups/cups-genppdupdate],
                [chmod +x src/cups/cups-genppdupdate])
AC_CONFIG_FILES([src/escputil/Makefile])
AC_CONFIG_FILES([src/foomatic/Makefile])
AC_CONFIG_FILES([src/foomatic/foomatic-generator],
                [chmod +x src/foomatic/foomatic-generator])
AC_CONFIG_FILES([src/foomatic/mk_printerlist],
                [chmod +x src/foomatic/mk_printerlist])
AC_CONFIG_FILES([src/testpattern/run-testpattern-2],
                [chmod +x src/testpattern/run-testpattern-2])
AC_CONFIG_FILES([src/cups/test-rastertogutenprint],
                [chmod +x src/cups/test-rastertogutenprint])
AC_CONFIG_FILES([src/ghost/Makefile])
AC_CONFIG_FILES([src/testpattern/Makefile])
AC_CONFIG_FILES([src/gimp2/Makefile])
AC_CONFIG_FILES([src/main/Makefile])
AC_CONFIG_FILES([src/main/gutenprint.pc])
AC_CONFIG_FILES([src/xml/Makefile])
AC_CONFIG_FILES([src/xml/escp2/Makefile])
AC_CONFIG_FILES([src/xml/escp2/media/Makefile])
AC_CONFIG_FILES([src/xml/escp2/mediasizes/Makefile])
AC_CONFIG_FILES([src/xml/escp2/model/Makefile])
AC_CONFIG_FILES([src/xml/escp2/inputslots/Makefile])
AC_CONFIG_FILES([src/xml/escp2/inks/Makefile])
AC_CONFIG_FILES([src/xml/escp2/weaves/Makefile])
AC_CONFIG_FILES([src/xml/escp2/qualitypresets/Makefile])
AC_CONFIG_FILES([src/gutenprintui2/Makefile])
AC_CONFIG_FILES([src/gutenprintui2/gutenprintui2.pc])
AC_CONFIG_FILES([test/Makefile])
AC_CONFIG_FILES([scripts/Makefile])
AC_CONFIG_COMMANDS([default], [
echo
],
[GUTENPRINT_RELEASE_VERSION=${GUTENPRINT_RELEASE_VERSION}]
)

AC_OUTPUT

warnings_emitted=0
echo
echo "Configuration Summary:"
echo "------------- --------"
echo
echo "If you have any problems, please report the information below to"
echo "$PACKAGE_BUGREPORT"
echo
echo "================================================================"
echo "  Release: $PACKAGE_STRING generated on $RELEASE_DATE"
echo
echo "  Features:"
if test "$BUILD_CUPS" != "no" ; then
    echo "    Build CUPS:                                 $BUILD_CUPS, installing in $cups_prefix"
    echo "        Build CUPS 1.2 enhancements:            $BUILD_CUPS_1_2"
    echo "        Build CUPS PPD files:                   $BUILD_CUPS_PPDS"
    if test "$BUILD_CUPS_PPDS" != "no" ; then
	echo "            Build translated CUPS PPD files:    $BUILD_TRANSLATED_CUPS_PPDS"
	echo "                Build global CUPS PPD files:    $BUILD_GLOBALIZED_CUPS_PPDS"
	echo "            Build simplified CUPS PPD files:    $BUILD_SIMPLIFIED_CUPS_PPDS"
	echo "            Install CUPS PPDs at top level:     $CUPS_PPDS_AT_TOP_LEVEL"
    fi
    if test "$BUILD_CUPS_PPDS" != "no" -o "$BUILD_CUPS_1_2" != "no" ; then
        echo "        Generate PS level 3 CUPS PPD files:     $USE_LEVEL3_PS"
    fi
    echo "        Build genppd statically:                $STATIC_GENPPD"

    if test "$STATIC_GENPPD" = "no" ; then
        echo "            ***WARNING: Use of --disable-static-genppd or --disable-static"
	echo "                        when building CUPS is very dangerous.  The build may"
	echo "                        fail when building the PPD files, or may *SILENTLY*"
	echo "                        build incorrect PPD files or cause other problems."
	echo "                        Please review the README and release notes carefully!"
	warnings_emitted=1
    fi
else
    echo "    Build CUPS:                                 $BUILD_CUPS"
fi
echo "    Build Ghostscript IJS driver:               $BUILD_GHOSTSCRIPT"
echo "    Build Foomatic data:                        $BUILD_FOOMATIC"
if test "$BUILD_FOOMATIC" != "no" ; then
    echo "        Build Foomatic 3.x data:                $BUILD_FOOMATIC_3"
fi
if test "$BUILD_GIMP2" != "no" ; then
    echo "    Build enhanced Print plugin for GIMP:       yes"
    if test "$BUILD_GIMP2_AS_GUTENPRINT" != "no" ; then
	echo "        GIMP plugin will be named:              gutenprint"
    else
	echo "        GIMP plugin will be named:              print"
    fi
    echo "        Install plugin(s) in home directory:    $USER_INSTALL"
else
    echo "    Build enhanced Print plugin for GIMP:       no"
fi
echo "    Build EPSON Stylus utility:                 $BUILD_ESCPUTIL"
echo "    Build test programs:                        $BUILD_TEST"
echo "    Build testpattern generator:                $BUILD_TESTPATTERN"
echo
echo "  Installation summary:"
echo "    Installation prefix:                        $prefix"
echo "    Data directory:                             $PACKAGE_DATA_DIR"
echo "    Library directory:                          $PACKAGE_LIB_DIR"
echo "    XML data directory:                         $PKGXMLDATADIR"
echo "    Module directory:                           $PKGMODULEDIR"
echo "    Install sample images:                      $INSTALL_SAMPLES"
echo
echo "  General configuration:"
if test x$ac_compiler_gnu = "xyes"; then
    echo "    Compiler options:                           $CFLAGS $GNUCFLAGS"
else
    echo "    Compiler options:                           $CFLAGS"
fi
echo "    Build static libraries:                     $enable_static"
echo "    Build shared libraries:                     $enable_shared"
echo "    Maintainer mode:                            $USE_MAINTAINER_MODE"
echo "    Use i18n:                                   $USE_NLS"
echo "    Generate profiling information:             $ENABLE_PROFILE"
echo "    Generate debugging symbols:                 $ENABLE_DEBUG"
echo "    Use modules:                                $WITH_MODULES"
if test -n "$EXTRA_LIBREADLINE_DEPS" ; then
    echo "    Use readline libraries:                     $USE_READLINE, extra arguments: $EXTRA_LIBREADLINE_DEPS"
else
    echo "    Use readline libraries:                     $USE_READLINE"
fi
echo "    uname -a output:                            `uname -a`"
echo "================================================================"
if test $warnings_emitted -ne 0 ; then
    echo
    echo '*** NOTE: Please review the warnings shown above carefully!'
fi
echo
if test "$BUILD_GIMP2" = "no" -o "$BUILD_CUPS" = "no" ; then
    echo "VERY IMPORTANT:"
    echo "If you expect to build the CUPS driver or the enhanced Print plugin for"
    echo "GIMP, but the output above indicates that that component will not be built,"
    echo "it almost certainly indicates that development packages required to build"
    echo "that component are not installed on your system.  Please read the README"
    echo "file and install the necessary development packages before reporting any"
    echo "error to the development team.  Depending upon your operating system version,"
    echo "you may have to install packages from the installation medium or over"
    echo "the network."
    echo
fi
echo "Finished configuring.  Please review CAREFULLY the configuration summary above."
echo "Type 'make clean' followed by 'make' to build the package"
echo "then 'make install' to install it."
