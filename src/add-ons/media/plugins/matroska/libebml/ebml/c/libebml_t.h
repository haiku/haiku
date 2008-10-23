/****************************************************************************
** LIBEBML : parse EBML files, see http://ebml.sourceforge.net/
**
** <file/class description>
**
** Copyright (C) 2002-2003 Steve Lhomme.  All rights reserved.
**
** This file is part of LIBEBML.
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
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.
**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
    \file libebml_t.h
    \version \$Id: libebml_t.h 1298 2008-02-21 22:14:18Z mosu $
    \author Steve Lhomme     <robux4 @ users.sf.net>
    \author Ingo Ralf Blum   <ingoralfblum @ users.sf.net>
    \author Moritz Bunkus <moritz@bunkus.org>
    
    \brief Misc type definitions for the C API of LIBEBML

    \note These types should be compiler/language independant (just platform dependant)
    \todo recover the sized types (uint16, int32, etc) here too (or maybe here only)
*/

#ifndef _LIBEBML_T_H_INCLUDED_
#define _LIBEBML_T_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

// Changed char is unsigned now (signedness was causing trouble in endil)
#if defined(_WIN32)
# if !defined(__GNUC__)		// Microsoft Visual C++
    typedef signed __int64 int64;
    typedef signed __int32 int32;
    typedef signed __int16 int16;
    typedef signed __int8 int8;
    typedef __int8 character;
    typedef unsigned __int64 uint64;
    typedef unsigned __int32 uint32;
    typedef unsigned __int16 uint16;
    typedef unsigned __int8 uint8;
# else // __GNUC__, this is mingw
#  include <stdint.h>
    typedef int64_t int64;
    typedef int32_t int32;
    typedef int16_t int16;
    typedef int8_t int8;
    typedef int8_t character;
    typedef uint64_t uint64;
    typedef uint32_t uint32;
    typedef uint16_t uint16;
    typedef uint8_t uint8;
# endif // __GNUC__
#elif (defined(__BEOS__) || defined(__HAIKU__))
#include <SupportDefs.h>
#elif defined(DJGPP)				/* SL : DJGPP doesn't support POSIX types ???? */
    typedef signed long long int64;
    typedef signed long int32;
    typedef signed short int16;
    typedef signed char int8;
    typedef char character;
    typedef unsigned long long uint64;
    typedef unsigned long uint32;
    typedef unsigned short uint16;
    typedef unsigned char uint8;
#elif defined(__sun) && (defined(__svr4__) || defined(__SVR4)) // SOLARIS
# include <inttypes.h>
# ifdef _NO_LONGLONG
#  error This compiler does not support 64bit integers.
# endif
    typedef long long int64; // int64_t is not always defined :(
    typedef int32_t int32;
    typedef int16_t int16;
    typedef int8_t int8;
    typedef int8_t character;
    typedef unsigned long long uint64; // uint64_t is not always defined :(
    typedef uint32_t uint32;
    typedef uint16_t uint16;
    typedef uint8_t uint8;
#elif (defined(__BEOS__) || defined(__HAIKU__))
# include <support/SupportDefs.h>
#else // anything else (Linux, BSD, ...)
# include <sys/types.h>
    typedef int64_t int64;
    typedef int32_t int32;
    typedef int16_t int16;
    typedef int8_t int8;
    typedef int8_t character;
    typedef u_int64_t uint64;
    typedef u_int32_t uint32;
    typedef u_int16_t uint16;
    typedef u_int8_t uint8;
#endif /* anything else */

typedef uint8  binary;


typedef enum open_mode {
    MODE_READ,
    MODE_WRITE,
    MODE_CREATE,
    MODE_SAFE
} open_mode;

#define EBML_MIN(x,y) ((x)<(y) ? (x) : (y))

#ifdef __cplusplus
}
#endif

#endif /* _LIBEBML_T_H_INCLUDED_ */
