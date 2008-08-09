@echo off
rem Configure batch file for `Wget' utility
rem Copyright (C) 1995, 1996, 1997, 2007, 2008 Free Software Foundation, Inc.

rem This program is free software; you can redistribute it and/or modify
rem it under the terms of the GNU General Public License as published by
rem the Free Software Foundation; either version 3 of the License, or
rem (at your option) any later version.

rem This program is distributed in the hope that it will be useful,
rem but WITHOUT ANY WARRANTY; without even the implied warranty of
rem MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
rem GNU General Public License for more details.

rem You should have received a copy of the GNU General Public License
rem along with this program.  If not, see <http://www.gnu.org/licenses/>.

rem Additional permission under GNU GPL version 3 section 7

rem If you modify this program, or any covered work, by linking or
rem combining it with the OpenSSL project's OpenSSL library (or a
rem modified version of that library), containing parts covered by the
rem terms of the OpenSSL or SSLeay licenses, the Free Software Foundation
rem grants you additional permission to convey the resulting work.
rem Corresponding Source for a non-source form of such a combination
rem shall include the source code for the parts of OpenSSL used as well
rem as that of the covered work.

if .%1 == .--borland goto :borland
if .%1 == .--mingw goto :mingw
if .%1 == .--msvc goto :msvc
goto :usage

:msvc
copy windows\Makefile.top Makefile > nul
copy windows\Makefile.src src\Makefile > nul
copy windows\Makefile.doc doc\Makefile > nul

echo Type NMAKE to start compiling.
echo If it doesn't work, try executing MSDEV\BIN\VCVARS32.BAT first,
echo and then NMAKE.
goto :end

:borland
copy windows\Makefile.top.bor Makefile > nul
copy windows\Makefile.src.bor src\Makefile > nul
copy windows\Makefile.doc doc\Makefile > nul

echo Type MAKE to start compiling.
goto :end

:mingw
copy windows\Makefile.top.mingw Makefile > nul
copy windows\Makefile.src.mingw src\Makefile > nul
copy windows\Makefile.doc doc\Makefile > nul

echo Type mingw32-make to start compiling.
goto :end

:usage
echo "Usage: configure [--borland | --mingw | --msvc]"
:end

copy windows\config.h src\config.h > nul
copy windows\config-compiler.h src\config-compiler.h > nul
