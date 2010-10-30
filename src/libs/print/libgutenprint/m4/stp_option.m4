# This file is part of Gutenprint.                     -*- Autoconf -*-
# Convenience macros.
# Copyright 2001-2002 Roger Leigh
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.


## Table of Contents:
## 1. Option handling
## 2. Conditionals


## ------------------ ##
## 1. Option handling ##
## ------------------ ##


## STP_ARG_WITH_DETAILED [(NAME, [NAME_OPTION], HELP, [HELP_OPTION],
## VARIABLE, DEFAULT, [OPTION_TRUE], [OPTION_FALSE])]
## -------------------------------------------------------------------
## Create a standardised configure option.
## NAME		Option name i.e. --with-foo or --enable-foo.
## NAME_OPTION	Optional input for use with NAME (informational).
## HELP		Help string to display.
## HELP_OPTION	Additional defaults information in addition to
##		DEFAULT.
## VARIABLE	Variable to set with command-line results.
## DEFAULT	Default value of VARIABLE.
## OPTION_TRUE	case statement body if an option is supplied.
##		This replaces the default actions.
## OPTION_FALSE Shell commands to run if no option is supplied.

AC_DEFUN([STP_ARG_WITH_DETAILED],
[# create custom command-line --with option
AC_MSG_CHECKING([whether to $3])
AC_ARG_WITH($1,
            [AC_HELP_STRING([--with-$1]m4_if([$2], , , [$2]),
              [$3 ]m4_if([$4], , , [$4 ])[@<:@]$6[@:>@])],
            [case "${withval}" in
            m4_if([$7], ,[
                yes) [$5]="yes" ; AC_MSG_RESULT(${[$5]}) ;;
                no) [$5]="no" ; AC_MSG_RESULT(${[$5]}) ;;
                *) AC_MSG_RESULT([unknown])
                   AC_MSG_ERROR([bad value ${withval} for --with-$1]) ;;
	    ], [$7])
	    esac],
            [if test -z "${[$5]}" ; then
               [$5]="$6"
             fi
             AC_MSG_RESULT(${[$5]}) ; m4_if([$8], , , [$8])])
if test -z "${[$5]}" ; then
  [$5]="$6"
fi
])


## STP_ARG_ENABLE_DETAILED [(NAME, [NAME_OPTION], HELP, [HELP_OPTION],
## VARIABLE, DEFAULT, [OPTION_TRUE], [OPTION_FALSE])]
## -------------------------------------------------------------------
## Create a standardised configure option.
## Identical to ATP_ARG_WITH_DETAILED.
AC_DEFUN([STP_ARG_ENABLE_DETAILED],
[# create custom command-line --enable option
AC_MSG_CHECKING([whether to $3])
AC_ARG_ENABLE($1,
            [AC_HELP_STRING([--enable-$1]m4_if([$2], , , [$2]),
              [$3 ]m4_if([$4], , , [$4 ])[@<:@]$6[@:>@])],
            [case "${enableval}" in
            m4_if([$7], ,[
                yes) [$5]="yes" ; AC_MSG_RESULT(${[$5]}) ;;
                no) [$5]="no" ; AC_MSG_RESULT(${[$5]}) ;;
                *) AC_MSG_RESULT([unknown])
                   AC_MSG_ERROR([bad value ${enableval} for --enable-$1]) ;;
	    ], [$7])
	    esac],
            [if test -z "${[$5]}" ; then
               [$5]="$6"
             fi
             AC_MSG_RESULT(${[$5]}) ; m4_if([$8], , , [$8])])
if test -z "${[$5]}" ; then
  [$5]="$6"
fi
])


## STP_ARG_WITH (NAME, HELP, VARIABLE, DEFAULT, [OPTION_TRUE], [OPTION_FALSE])
## ---------------------------------------------------------------------------
## NAME           Option name i.e. --with-foo or --enable-foo.
## HELP           Help string to display.
## VARIABLE       Variable to set with command-line results.
## DEFAULT        Default value of VARIABLE.
## OPTION_TRUE    case statement body if an option is supplied.
##                This replaces the default actions.
## OPTION_FALSE   shell commands to run if no option is supplied.
AC_DEFUN([STP_ARG_WITH],
[STP_ARG_WITH_DETAILED([$1], , [$2], , [$3], [$4], [$5], [$6])])

## STP_ARG_ENABLE (NAME, HELP, VARIABLE, DEFAULT, [OPTION_TRUE], [OPTION_FALSE])
## -----------------------------------------------------------------------------
## Identical to STP_ARG_WITH.
AC_DEFUN([STP_ARG_ENABLE],
[STP_ARG_ENABLE_DETAILED([$1], , [$2], , [$3], [$4], [$5], [$6])])




## --------------- ##
## 2. Conditionals ##
## --------------- ##


# STP_CONDITIONAL (OPTION)
# ------------------------
# Define automake conditional based on STP_ARG* results.  Until
# automake uses autoconf traces, this breaks automake
AC_DEFUN([STP_CONDITIONAL],
[AM_CONDITIONAL(BUILD_$1, test x${BUILD_$1} = xyes)])


## STP_ADD_COMPILER_ARG (ARG, COMPILER, VARIABLE)
## ---------------------------------------------------------------------------
## ARG            Compiler option
## COMPILER       Compiler name
## VARIABLE       Variable to add option to (default CFLAGS).
AC_DEFUN([STP_ADD_COMPILER_ARG],[
  AC_MSG_CHECKING(if m4_ifval([$2], [$2 ], [${CC} ])supports $1)
  stp_acOLDCFLAGS="${CFLAGS}"
  CFLAGS="${m4_ifval([$3], [$3], [CFLAGS])} $1"
  AC_TRY_COMPILE(,,
      [ AC_MSG_RESULT(yes);
        stp_newCFLAGS="$CFLAGS"],
      [ AC_MSG_RESULT(no);
	stp_newCFLAGS="$stp_acOLDCFLAGS"])
  CFLAGS="$stp_acOLDCFLAGS"
  m4_ifval([$3], [$3], [CFLAGS])="${stp_newCFLAGS}"
])

## STP_ADD_FIRST_COMPILER_ARG (ARGS, COMPILER, VARIABLE)
## ---------------------------------------------------------------------------
## ARGS           Compiler options
## COMPILER       Compiler name
## VARIABLE       Variable to add option to (default CFLAGS).
AC_DEFUN([STP_ADD_FIRST_COMPILER_ARG],[
  for stp_ac_arg in $1 ; do
    stp_ac_save_CFLAGS="${m4_ifval([$3], [$3], [CFLAGS])}"
    STP_ADD_COMPILER_ARG([${stp_ac_arg}], [$2], [$3])
    test "${stp_ac_save_CFLAGS}" != "${m4_ifval([$3], [$3], [CFLAGS])}" && break
  done
])

## STP_ADD_COMPILER_ARGS (ARGS, COMPILER, VARIABLE)
## ---------------------------------------------------------------------------
## ARGS           Compiler options
## COMPILER       Compiler name
## VARIABLE       Variable to add option to (default CFLAGS).
AC_DEFUN([STP_ADD_COMPILER_ARGS],[
  for stp_ac_arg in $1 ; do
    STP_ADD_COMPILER_ARG([${stp_ac_arg}], [$2], [$3])
  done
])
