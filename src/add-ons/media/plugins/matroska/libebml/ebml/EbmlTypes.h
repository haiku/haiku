/****************************************************************************
** libebml : parse EBML files, see http://embl.sourceforge.net/
**
** <file/class description>
**
** Copyright (C) 2002-2004 Steve Lhomme.  All rights reserved.
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
	\file
	\version \$Id: EbmlTypes.h 639 2004-07-09 20:59:14Z mosu $
*/
#ifndef LIBEBML_TYPES_H
#define LIBEBML_TYPES_H

#include <clocale>
#include <string>

#include "c/libebml_t.h"
#include "EbmlConfig.h"
#include "EbmlEndian.h" // binary needs to be defined

START_LIBEBML_NAMESPACE

typedef wchar_t utf16;
typedef uint32 utf32;
typedef char utf8;

typedef binary bits80[10];

typedef Endian<int16,little_endian>  lil_int16;
typedef Endian<int32,little_endian>  lil_int32;
typedef Endian<int64,little_endian>  lil_int64;
typedef Endian<uint16,little_endian> lil_uint16;
typedef Endian<uint32,little_endian> lil_uint32;
typedef Endian<uint64,little_endian> lil_uint64;
typedef Endian<int16,big_endian>     big_int16;
typedef Endian<int32,big_endian>     big_int32;
typedef Endian<int64,big_endian>     big_int64;
typedef Endian<uint16,big_endian>    big_uint16;
typedef Endian<uint32,big_endian>    big_uint32;
typedef Endian<uint64,big_endian>    big_uint64;
typedef Endian<uint32,big_endian>    checksum;
typedef Endian<bits80,big_endian>    big_80bits;


enum ScopeMode {
	SCOPE_PARTIAL_DATA = 0,
	SCOPE_ALL_DATA,
	SCOPE_NO_DATA
};

END_LIBEBML_NAMESPACE

#endif
