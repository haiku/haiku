@echo off

echo Configuring GNU Diffutils for DJGPP v2.x...

Rem Copyright (C) 2001 Free Software Foundation, Inc.

Rem This program is free software; you can redistribute it and/or modify
Rem it under the terms of the GNU General Public License as published by
Rem the Free Software Foundation; either version 2, or (at your option)
Rem any later version.

Rem This program is distributed in the hope that it will be useful,
Rem but WITHOUT ANY WARRANTY; without even the implied warranty of
Rem MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Rem GNU General Public License for more details.

Rem You should have received a copy of the GNU General Public License
Rem along with this program; if not, write to the Free Software
Rem Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
Rem 02111-1307, USA.

Rem Written by Eli Zaretskii.


Rem The small_env tests protect against fixed and too small size
Rem of the environment in stock DOS shell.

Rem Find out if NLS is wanted or not,
Rem if dependency-tracking is wanted or not,
Rem if cache is wanted or not
Rem and where the sources are.
set ARGS=
set NLS=disabled
if not "%NLS%" == "disabled" goto small_env
set CACHE=enabled
if not "%CACHE%" == "enabled" goto small_env
set DEPENDENCY_TRACKING=disabled
if not "%DEPENDENCY_TRACKING%" == "disabled" goto small_env
set XSRC=.
if not "%XSRC%" == "." goto small_env

Rem Loop over all arguments.
Rem Special arguments are: NLS, XSRC CACHE and DEPS.
Rem All other arguments are stored into ARGS.
:arg_loop
set SPECIAL_ARG_SEEN=0
if not "%SPECIAL_ARG_SEEN%" == "0" goto small_env
if "%1" == "NLS" goto nls_on
if not "%1" == "nls" goto CacheOpt
:nls_on
if "%1" == "nls" set NLS=enabled
if "%1" == "NLS" set NLS=enabled
if not "%NLS%" == "enabled" goto small_env
set SPECIAL_ARG_SEEN=1
if not "%SPECIAL_ARG_SEEN%" == "1" goto small_env
shift
:CacheOpt
set SPECIAL_ARG_SEEN=0
if not "%SPECIAL_ARG_SEEN%" == "0" goto small_env
if "%1" == "no-cache" goto cache_off
if "%1" == "no-CACHE" goto cache_off
if not "%1" == "NO-CACHE" goto dependency_opt
:cache_off
if "%1" == "no-cache" set CACHE=disabled
if "%1" == "no-CACHE" set CACHE=disabled
if "%1" == "NO-CACHE" set CACHE=disabled
if not "%CACHE%" == "disabled" goto small_env
set SPECIAL_ARG_SEEN=1
if not "%SPECIAL_ARG_SEEN%" == "1" goto small_env
shift
:dependency_opt
set SPECIAL_ARG_SEEN=0
if not "%SPECIAL_ARG_SEEN%" == "0" goto small_env
if "%1" == "dep" goto dep_off
if not "%1" == "DEP" goto src_dir_opt
:dep_off
if "%1" == "dep" set DEPENDENCY_TRACKING=enabled
if "%1" == "DEP" set DEPENDENCY_TRACKING=enabled
if not "%DEPENDENCY_TRACKING%" == "enabled" goto small_env
set SPECIAL_ARG_SEEN=1
if not "%SPECIAL_ARG_SEEN%" == "1" goto small_env
shift
:src_dir_opt
set SPECIAL_ARG_SEEN=0
if not "%SPECIAL_ARG_SEEN%" == "0" goto small_env
echo %1 | grep -q "/"
if errorlevel 1 goto collect_arg
set XSRC=%1
if not "%XSRC%" == "%1" goto small_env
set SPECIAL_ARG_SEEN=1
if not "%SPECIAL_ARG_SEEN%" == "1" goto small_env
:collect_arg
if "%SPECIAL_ARG_SEEN%" == "0" set _ARGS=%ARGS% %1
if "%SPECIAL_ARG_SEEN%" == "0" if not "%_ARGS%" == "%ARGS% %1" goto small_env
echo %_ARGS% | grep -q "[^ ]"
if not errorlevel 0 set ARGS=%_ARGS%
set _ARGS=
shift
if not "%1" == "" goto arg_loop
set SPECIAL_ARG_SEEN=

Rem Create a response file for the configure script.
echo --srcdir=%XSRC% > arguments
if "%CACHE%" == "enabled"                echo --config-cache >> arguments
if "%DEPENDENCY_TRACKING%" == "enabled"  echo --enable-dependency-tracking >> arguments
if "%DEPENDENCY_TRACKING%" == "disabled" echo --disable-dependency-tracking >> arguments
if not "%ARGS%" == ""                    echo %ARGS% >> arguments
set ARGS=
set CACHE=
set DEPENDENCY_TRACKING=

if "%XSRC%" == "." goto in_place

:not_in_place
redir -e /dev/null update %XSRC%/configure.orig ./configure
test -f ./configure
if errorlevel 1 update %XSRC%/configure ./configure

:in_place
Rem Update configuration files
echo Updating configuration scripts...
test -f ./configure.orig
if errorlevel 1 update configure configure.orig
sed -f %XSRC%/ms/config.sed configure.orig > configure
if errorlevel 1 goto sed_error

Rem Make sure they have a config.site file
set CONFIG_SITE=%XSRC%/ms/config.site
if not "%CONFIG_SITE%" == "%XSRC%/ms/config.site" goto small_env

Rem Make sure crucial file names are not munged by unpacking
test -f %XSRC%/po/Makefile.in.in
if not errorlevel 1 mv -f %XSRC%/po/Makefile.in.in %XSRC%/po/Makefile.in-in
test -f %XSRC%/m4/Makefile.am.in
if not errorlevel 1 mv -f %XSRC%/m4/Makefile.am.in %XSRC%/m4/Makefile.am-in

Rem This is required because DOS/Windows are case-insensitive
Rem to file names, and "make install" will do nothing if Make
Rem finds a file called `install'.
if exist INSTALL ren INSTALL INSTALL.txt

Rem Set HOME to a sane default so configure stops complaining.
if not "%HOME%" == "" goto HostName
set HOME=%XSRC%/ms
if not "%HOME%" == "%XSRC%/ms" goto small_env
echo No HOME found in the environment, using default value

:HostName
Rem Set HOSTNAME so it shows in config.status
if not "%HOSTNAME%" == "" goto hostdone
if "%windir%" == "" goto msdos
set OS=MS-Windows
if not "%OS%" == "MS-Windows" goto small_env
goto haveos
:msdos
set OS=MS-DOS
if not "%OS%" == "MS-DOS" goto small_env
:haveos
if not "%USERNAME%" == "" goto haveuname
if not "%USER%" == "" goto haveuser
echo No USERNAME and no USER found in the environment, using default values
set HOSTNAME=Unknown PC
if not "%HOSTNAME%" == "Unknown PC" goto small_env
goto userdone
:haveuser
set HOSTNAME=%USER%'s PC
if not "%HOSTNAME%" == "%USER%'s PC" goto small_env
goto userdone
:haveuname
set HOSTNAME=%USERNAME%'s PC
if not "%HOSTNAME%" == "%USERNAME%'s PC" goto small_env
:userdone
set _HOSTNAME=%HOSTNAME%, %OS%
if not "%_HOSTNAME%" == "%HOSTNAME%, %OS%" goto small_env
set HOSTNAME=%_HOSTNAME%
:hostdone
set _HOSTNAME=
set OS=

Rem install-sh is required by the configure script but clashes with the
Rem various Makefile install-foo targets, so we MUST have it before the
Rem script runs and rename it afterwards
test -f %XSRC%/install-sh
if not errorlevel 1 goto no_ren0
test -f %XSRC%/install-sh.sh
if not errorlevel 1 mv -f %XSRC%/install-sh.sh %XSRC%/install-sh
:no_ren0

if "%NLS%" == "disabled" goto without_NLS

:with_NLS
Rem Check for the needed libraries and binaries.
test -x /dev/env/DJDIR/bin/msgfmt.exe
if not errorlevel 0 goto missing_NLS_tools
test -x /dev/env/DJDIR/bin/xgettext.exe
if not errorlevel 0 goto missing_NLS_tools
test -f /dev/env/DJDIR/include/libcharset.h
if not errorlevel 0 goto missing_NLS_tools
test -f /dev/env/DJDIR/lib/libcharset.a
if not errorlevel 0 goto missing_NLS_tools
test -f /dev/env/DJDIR/include/iconv.h
if not errorlevel 0 goto missing_NLS_tools
test -f /dev/env/DJDIR/lib/libiconv.a
if not errorlevel 0 goto missing_NLS_tools
test -f /dev/env/DJDIR/include/libintl.h
if not errorlevel 0 goto missing_NLS_tools
test -f /dev/env/DJDIR/lib/libintl.a
if not errorlevel 0 goto missing_NLS_tools

Rem Recreate the files in the %XSRC%/po subdir with our ported tools.
redir -e /dev/null rm %XSRC%/po/*.gmo
redir -e /dev/null rm %XSRC%/po/diffutil*.pot
redir -e /dev/null rm %XSRC%/po/cat-id-tbl.c
redir -e /dev/null rm %XSRC%/po/stamp-cat-id

Rem Update the arguments file for the configure script.
Rem We prefer without-included-gettext because libintl.a from gettext package
Rem is the only one that is guaranteed to have been ported to DJGPP.
echo --enable-nls --without-included-gettext >> arguments
goto configure_package

:missing_NLS_tools
echo Needed libs/tools for NLS not found. Configuring without NLS.
:without_NLS
Rem Update the arguments file for the configure script.
echo --disable-nls >> arguments

:configure_package
echo Running the ./configure script...
sh ./configure @arguments
if errorlevel 1 goto cfg_error
rm arguments
echo Done.
goto End

:sed_error
echo ./configure script editing failed!
goto End

:cfg_error
echo ./configure script exited abnormally!
goto End

:small_env
echo Your environment size is too small.  Enlarge it and run me again.
echo Configuration NOT done!

:End
test -f %XSRC%/install-sh.sh
if not errorlevel 1 goto no_ren1
test -f %XSRC%/install-sh
if not errorlevel 1 mv -f %XSRC%/install-sh %XSRC%/install-sh.sh
:no_ren1
if "%HOME%" == "%XSRC%/ms" set HOME=
set ARGS=
set CONFIG_SITE=
set HOSTNAME=
set NLS=
set CACHE=
set DEPENDENCY_TRACKING=
set XSRC=
