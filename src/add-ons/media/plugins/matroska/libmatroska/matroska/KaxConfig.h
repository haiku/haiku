/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2005 Steve Lhomme.  All rights reserved.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
	\file
	\version \$Id: KaxConfig.h,v 1.7 2004/04/14 23:26:17 robux4 Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Moritz Bunkus <moritz @ bunkus.org>
*/
#ifndef LIBMATROSKA_CONFIG_H
#define LIBMATROSKA_CONFIG_H

#define LIBMATROSKA_NAMESPACE       libmatroska

#if defined(NO_NAMESPACE) // for older GCC
# define START_LIBMATROSKA_NAMESPACE
# define END_LIBMATROSKA_NAMESPACE
#else // NO_NAMESPACE
# define START_LIBMATROSKA_NAMESPACE namespace LIBMATROSKA_NAMESPACE {
# define END_LIBMATROSKA_NAMESPACE   };
#endif // NO_NAMESPACE

// There are special implementations for certain platforms. For example on Windows
// we use the Win32 file API. here we set the appropriate macros.
#if defined(_WIN32)||defined(WIN32)

# if defined(MATROSKA_DLL)
#  if defined(MATROSKA_DLL_EXPORT)
#   define MATROSKA_DLL_API __declspec(dllexport)
#  else // MATROSKA_DLL_EXPORT
#   define MATROSKA_DLL_API __declspec(dllimport)
#  endif // MATROSKA_DLL_EXPORT
# else // MATROSKA_DLL
#  define MATROSKA_DLL_API
# endif // MATROSKA_DLL

#else
# define MATROSKA_DLL_API
#endif

#if !defined(MATROSKA_VERSION)
#define MATROSKA_VERSION  2
#endif // MATROSKA_VERSION


#endif // LIBMATROSKA_CONFIG_H
