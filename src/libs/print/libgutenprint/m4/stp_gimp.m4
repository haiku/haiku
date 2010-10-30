# This file is part of Gutenprint.                     -*- Autoconf -*-
# GIMP support.
# Copyright 2000-2002 Roger Leigh
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
## 1. GIMP library checks
## 2. gimptool support


## ---------------------- ##
## 1. GIMP library checks ##
## ---------------------- ##


# STP_GIMP2_LIBS
# --------------
# GIMP library checks
AC_DEFUN([STP_GIMP2_LIBS],
[dnl GIMP library checks
if test x${BUILD_GIMP2} = xyes ; then
  GIMP2_DATA_DIR=`$PKG_CONFIG gimp-2.0 --variable=gimpdatadir`
  GIMP2_PLUGIN_DIR=`$PKG_CONFIG gimp-2.0 --variable=gimplibdir`/plug-ins
  gimp2_plug_indir="${GIMP2_PLUGIN_DIR}"
fi
])



## ------------------- ##
## 2. gimptool support ##
## ------------------- ##


# STP_GIMP2_PLUG_IN_DIR
# ---------------------
# Locate the GIMP plugin directory using libtool
AC_DEFUN([STP_GIMP2_PLUG_IN_DIR],
[dnl Extract directory using --dry-run and sed
if test x${BUILD_GIMP2} = xyes ; then
  AC_MSG_CHECKING([for GIMP 2.0 plug-in directory])
# create temporary "plug-in" to install
  touch print
  chmod 755 print
  GIMPTOOL2_OUTPUT=`$GIMPTOOL2_CHECK --dry-run --install-${PLUG_IN_PATH} print | tail -n 1`
  rm print
  GIMP2_PLUGIN_DIR=`echo "$GIMPTOOL2_OUTPUT" | sed -e 's/.* \(.*\)\/print/\1/'`
  AC_MSG_RESULT([$GIMP2_PLUGIN_DIR])
fi
])
